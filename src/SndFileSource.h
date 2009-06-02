/*
 * Copyright 2008 by Idiap Research Institute
 *                   http://www.idiap.ch
 *
 * See the file COPYING for the licence associated with this software.
 */

#ifndef SNDFILESOURCE_H
#define SNDFILESOURCE_H

#include <sndfile.h>

#include "CachedPlugin.h"
#include "Source.h"

namespace Tracter
{
    /**
     * Plugin to deal with audio files via libsndfile
     */
    class SndFileSource : public Source< CachedPlugin<float> >
    {
    public:
        SndFileSource(const char* iObjectName = "SndFileSource");
        virtual ~SndFileSource() throw();
        void Open(
            const char* iFileName,
            TimeType iBeginTime = -1,
            TimeType iEndTime = -1
        );

    private:
        virtual int Fetch(IndexType iIndex, CacheArea& iOutputArea);
        SNDFILE* mSndFile;
        IndexType mNFrames;
        bool mSoxHack;
    };
}

#endif /* SNDFILESOURCE_H */
