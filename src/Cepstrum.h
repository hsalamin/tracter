#ifndef CEPSTRUM_H
#define CEPSTRUM_H

#include <fftw3.h>
#include "UnaryPlugin.h"

/**
 * Uses FFTW to generate the cepstrum of an input
 */
class Cepstrum : public UnaryPlugin<float, float>
{
public:
    Cepstrum(Plugin<float>* iInput, int iNCepstra, bool iC0=false);
    ~Cepstrum();

protected:
    bool ProcessFrame(IndexType iIndex, int iOffset);

private:
    int mNLogData;
    int mNCepstra;
    float* mLogData;
    float* mCepstra;
    bool mC0;
    fftwf_plan mPlan;
};

#endif /* CEPSTRUM_H */