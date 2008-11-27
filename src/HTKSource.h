/*
 * Copyright 2007 by IDIAP Research Institute
 *                   http://www.idiap.ch
 *
 * See the file COPYING for the licence associated with this software.
 */

#ifndef HTKSOURCE_H
#define HTKSOURCE_H

#include "CachedPlugin.h"
#include "ByteOrder.h"
#include "Source.h"
#include "MMap.h"

namespace Tracter
{
    /**
     * Plugin to deal with HTK feature files
     */
    class HTKSource : public CachedPlugin<float>, public Source
    {
    public:
        HTKSource(const char* iObjectName = "HTKSource");
        virtual ~HTKSource() throw() {}
        void Open(const char* iFileName);

    protected:
        /** Diverts basic time stamp requests to the Source base class */
        virtual TimeType TimeStamp(IndexType iIndex)
        {
             return Source::TimeStamp(iIndex);
        }

    private:
        ByteOrder mByteOrder;
        MMap mMap;
        float* mMapData;
        IndexType mNSamples;
        virtual int Fetch(IndexType iIndex, CacheArea& iOutputArea);
    };
}

#endif /* HTKSOURCE_H */
