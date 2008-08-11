/*
 * Copyright 2008 by IDIAP Research Institute
 *                   http://www.idiap.ch
 *
 * See the file COPYING for the licence associated with this software.
 */

#include "config.h"

#include "ASRFactory.h"

#include "FileSource.h"
#include "SocketSource.h"

#ifdef HAVE_ALSA
# include "ALSASource.h"
#endif

#include "Normalise.h"
#include "Mean.h"
#include "Subtract.h"
#include "Concatenate.h"
#include "Delta.h"
#include "Variance.h"
#include "Divide.h"
#include "ZeroFilter.h"
#include "Periodogram.h"
#include "MelFilter.h"
#include "Cepstrum.h"

#include "LPCepstrum.h"

#ifdef HAVE_HTK
# include "HCopyWrapper.h"
#endif

#include "Energy.h"
#include "ModulationVAD.h"
#include "VADGate.h"

#include "SocketTee.h"

Tracter::ASRFactory::ASRFactory(const char* iObjectName)
{
    mObjectName = iObjectName;

    // List all sources
    mSource["File"] = &Tracter::ASRFactory::fileSource;
    mSource["Socket"] = &Tracter::ASRFactory::socketSource;
#ifdef HAVE_ALSA
    mSource["ALSA"] = &Tracter::ASRFactory::alsaSource;
#endif

    // List all available front-ends
    mFrontend["Basic"] = &Tracter::ASRFactory::basicFrontend;
    mFrontend["BasicVAD"] = &Tracter::ASRFactory::basicVADFrontend;
    mFrontend["PLP"] = &Tracter::ASRFactory::plpFrontend;
#ifdef HAVE_HTK
    mFrontend["HTK"] = &Tracter::ASRFactory::htkFrontend;
#endif
}

Tracter::Plugin<float>* Tracter::ASRFactory::CreateSource(Source*& iSource)
{
    Plugin<float> *plugin = 0;

    const char* source = GetEnv("Source", "File");
    if (mSource[source])
        plugin = (this->*mSource[source])(iSource);
    else
        throw Exception("ASRFactory: Unknown source %s\n", source);

    return plugin;
}

Tracter::Plugin<float>*
Tracter::ASRFactory::CreateFrontend(Plugin<float>* iPlugin)
{
    Plugin<float> *plugin = 0;

    const char* frontend = GetEnv("Frontend", "Basic");
    if (mFrontend[frontend])
        plugin = (this->*mFrontend[frontend])(iPlugin);
    else
        throw Exception("ASRFactory: Unknown frontend %s\n", frontend);

    return plugin;
}


Tracter::Plugin<float>* Tracter::ASRFactory::fileSource(Source*& iSource)
{
    FileSource<short>* s = new FileSource<short>();
    Normalise* n = new Normalise(s);
    iSource = s;
    return n;
}

#ifdef HAVE_ALSA
Tracter::Plugin<float>* Tracter::ASRFactory::alsaSource(Source*& iSource)
{
    ALSASource* s = new ALSASource();
    Normalise* n = new Normalise(s);
    iSource = s;
    return n;
}
#endif

Tracter::Plugin<float>* Tracter::ASRFactory::socketSource(Source*& iSource)
{
    SocketSource* s = new SocketSource();
    iSource = s;
    return s;
}

Tracter::Plugin<float>* Tracter::ASRFactory::deltas(Plugin<float>* iPlugin)
{
    Plugin<float>* plugin = iPlugin;
    int deltaOrder = GetEnv("DeltaOrder", 0);
    if (deltaOrder > 0)
    {
        Concatenate* c = new Concatenate();
        c->Add(plugin);
        for (int i=0; i<deltaOrder; i++)
        {
            Delta* d = new Delta(plugin);
            c->Add(d);
            plugin = d;
        }
        plugin = c;
    }
    return plugin;
}

Tracter::Plugin<float>*
Tracter::ASRFactory::normaliseMean(Plugin<float>* iPlugin)
{
    Plugin<float>* plugin = iPlugin;
    bool cmn = GetEnv("NormaliseMean", 1);
    if (cmn)
    {
        Mean* m = new Mean(iPlugin);
        Subtract* s = new Subtract(iPlugin, m);
        plugin = s;
    }
    return plugin;
}

Tracter::Plugin<float>*
Tracter::ASRFactory::normaliseVariance(Plugin<float>* iPlugin)
{
    Plugin<float>* plugin = iPlugin;
    bool cvn = GetEnv("NormaliseVariance", 0);
    if (cvn)
    {
        Variance* v = new Variance(iPlugin);
        Divide* d = new Divide(iPlugin, v);
        plugin = d;
    }
    return plugin;
}

Tracter::Plugin<float>*
Tracter::ASRFactory::basicFrontend(Plugin<float>* iPlugin)
{
    Plugin<float>* p = iPlugin;
    p = new ZeroFilter(p);
    p = new Periodogram(p);
    p = new MelFilter(p);
    p = new Cepstrum(p);
    p = normaliseMean(p);
    p = deltas(p);
    p = normaliseVariance(p);
    return p;
}

Tracter::Plugin<float>*
Tracter::ASRFactory::basicVADFrontend(Plugin<float>* iPlugin)
{
    /* Basic signal processing chain */
    Plugin<float>* p = iPlugin;
    p = new ZeroFilter(p);
    p = new Periodogram(p);
    p = new MelFilter(p);
    p = new Cepstrum(p);
    p = normaliseMean(p);
    p = deltas(p);
    p = normaliseVariance(p);

    /* VAD */
    Plugin<float>* v = iPlugin;
    v = new Energy(v);
    ModulationVAD* mv = new ModulationVAD(v);
    v = new VADGate(p, mv);

    return v;
}

Tracter::Plugin<float>*
Tracter::ASRFactory::plpFrontend(Plugin<float>* iPlugin)
{
    Plugin<float>* p = iPlugin;
    p = new ZeroFilter(p);
    p = new Periodogram(p);
    p = new MelFilter(p);
    p = new LPCepstrum(p);
    p = normaliseMean(p);
    p = deltas(p);
    p = normaliseVariance(p);
    return p;
}

#ifdef HAVE_HTK
Tracter::Plugin<float>*
Tracter::ASRFactory::htkFrontend(Plugin<float>* iPlugin)
{
    Plugin<float>* p = iPlugin;
    p = new HCopyWrapper(p);
    return p;
}
#endif
