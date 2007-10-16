#include <math.h>
#include "Periodogram.h"

Periodogram::Periodogram(
    Plugin<float>* iInput, int iFrameSize, int iFramePeriod
)
    : UnaryPlugin<float, float>(iInput, iFrameSize/2+1)
{
    assert(iFrameSize > 0);
    assert(iFramePeriod > 0);
    mFrameSize = iFrameSize;
    mFramePeriod = iFramePeriod;
    PluginObject::MinSize(mInput, mFrameSize);

    mRealData = (float*)fftwf_malloc(mFrameSize * sizeof(float));
    mComplexData =
        (fftwf_complex*)fftwf_malloc(mArraySize * sizeof(fftwf_complex));
    assert(mRealData);
    assert(mComplexData);
    mPlan = fftwf_plan_dft_r2c_1d(mFrameSize, mRealData, mComplexData, 0);

    // Hardwire a Hamming window.  Could be generalised much better.
    // This one is asymmetric.  Should it be symmetric?
    const float PI = 3.14159265358979323846;
    mWindow.resize(mFrameSize);
    for (int i=0; i<mFrameSize; i++)
        mWindow[i] = 0.54f - 0.46f * cosf(PI * 2.0f * i / (mFrameSize - 1));
}

Periodogram::~Periodogram()
{
    fftwf_destroy_plan(mPlan);
    fftwf_free(mRealData);
    fftwf_free(mComplexData);
}

bool Periodogram::ProcessFrame(IndexType iIndex, int iOffset)
{
    assert(iIndex >= 0);
    CacheArea inputArea;

    // Read the input frame
    int readIndex = iIndex * mFramePeriod;
    int got = mInput->Read(inputArea, readIndex, mFrameSize);
    if (got < mFrameSize)
        return false;

    // Copy the frame into the contiguous array
    float* p = mInput->GetPointer();
    for (int i=0; i<inputArea.len[0]; i++)
        mRealData[i] = p[inputArea.offset+i] * mWindow[i];
    for (int i=0; i<inputArea.len[1]; i++)
        mRealData[i+inputArea.len[0]] = p[i] * mWindow[i+inputArea.len[0]];

    // Do the DFT
    fftwf_execute(mPlan);

    // Compute periodogram
    float* cache = GetPointer(iOffset);
    for (int i=0; i<mArraySize; i++)
        cache[i] =
                mComplexData[i][0] * mComplexData[i][0] +
                mComplexData[i][1] * mComplexData[i][1];

    return true;
}