/*
 * Copyright 2007 by IDIAP Research Institute
 *                   http://www.idiap.ch
 *
 * See the file COPYING for the licence associated with this software.
 */

#ifndef LNASOURCE_H
#define LNASOURCE_H

#include "CachedPlugin.h"
#include "Source.h"
#include "MMap.h"

namespace Tracter
{
    /**
     * Uses an LNA file as a source
     */
    class LNASource : public Source< CachedPlugin<float> >
    {
    public:
        LNASource(const char* iObjectName = "LNASource");
        virtual ~LNASource() throw() {}
        void Open(
            const char* iFileName,
            TimeType iBeginTime = -1,
            TimeType iEndTime = -1
        );

    private:
        MMap mMap;
        void* mMapData;
        int mMapSize;
        bool mLNA16;
        virtual bool UnaryFetch(IndexType iIndex, int iOffset);
    };
}

#endif /* LNASOURCE_H */
