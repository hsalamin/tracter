/*
 * Copyright 2007 by IDIAP Research Institute
 *                   http://www.idiap.ch
 *
 * See the file COPYING for the licence associated with this software.
 */

#include "SpectralSubtract.h"

Tracter::SpectralSubtract::SpectralSubtract(
    Component<float>* iInput1,
    Component<float>* iInput2,
    const char* iObjectName
)
{
    mObjectName = iObjectName;
    mInput1 = iInput1;
    mInput2 = iInput2;
    Connect(iInput1);
    Connect(iInput2);

    mFrame.size = iInput1->Frame().size;

    mAlpha = GetEnv("Alpha", 1.0f);
    mBeta = GetEnv("Beta", 0.0f);
}

bool Tracter::SpectralSubtract::UnaryFetch(IndexType iIndex, float* oData)
{
    assert(iIndex >= 0);
    assert(oData);
    CacheArea inputArea;

    // Start with the second input, likely to be a cepstral or
    // spectral mean
    if (mInput2->Read(inputArea, iIndex) == 0)
        return false;
    float *p2 = mInput2->GetPointer(inputArea.offset);

    // Now the first input
    if (mInput1->Read(inputArea, iIndex) == 0)
        return false;
    float *p1 = mInput1->GetPointer(inputArea.offset);

    // Do the subtraction
    for (int i=0; i<mFrame.size; i++)
    {
        float weighted = p1[i] - mAlpha * p2[i];
        float floored  = p2[i] * mBeta;
        oData[i] = (weighted > floored) ? weighted : floored;
    }

    return true;
}
