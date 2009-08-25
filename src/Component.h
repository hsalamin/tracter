/*
 * Copyright 2007,2008 by IDIAP Research Institute
 *                        http://www.idiap.ch
 *
 * See the file COPYING for the licence associated with this software.
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#include <cassert>
#include <climits>
#include <vector>
#include <algorithm>

#include "TracterObject.h"

namespace Tracter
{
    /**
     * A contiguous area of interest of a circular cache.
     *
     * Represents an area of a cache buffer.  Typically this is a
     * sub-array, but also deals with the case where the sub-array wraps
     * around the end of the circular buffer.  i.e., it is contiguous in
     * data space, but not necessarily in memory.
     */
    class CacheArea
    {
    public:
        int offset;
        int len[2];

        int Length() const
        {
            return len[0] + len[1];
        };

        void Set(int iLength, int iOffset, int iSize);
    };

    /** Information about frame size and period */
    struct FrameInfo
    {
        int size;      ///< Size of each cache frame
        float period;  ///< Frame period of this component
    };

    typedef long IndexType;      ///< Integer type of a frame index
    typedef long long TimeType;  ///< 64 bit type like ASIOTimeStamp
    const TimeType ONEe3 = 1000;
    const TimeType ONEe6 = 1000000;
    const TimeType ONEe9 = 1000000000;

    /** Index / Offset pair for internal cache management */
    struct CachePointer
    {
        IndexType index;     ///< Index in logical space
        int offset;          ///< Index in physical space
    };

    /** Head-tail pair of cache pointers */
    struct CachePair
    {
        CachePointer head;   ///< Next position to write to
        CachePointer tail;   ///< Oldest position written to
    };

    static const CachePair ZEROPAIR = {
        {0, 0},
        {0, 0}
    };

    /** Storage of minimum / maximum */
    class MinMax
    {
    public:
        /* Storage */
        int min;
        int max;

        /** Constructor */
        MinMax()
        {
            min = INT_MAX;
            max = INT_MIN;
        }

        /** Add new datum */
        void Update(int iVal)
        {
            min = std::min(min, iVal);
            max = std::max(min, iVal);
        }
    };

    /** Frame rate type */
    struct ExactRateType
    {
        float rate;   ///< Original rate at the source
        float period; ///< Accumulated period
    };

    /** Range that a Read() can access */
    class ReadRange
    {
    public:
        static const int INFINITE = INT_MAX;
        ReadRange(int iSize = 1)
        {
            mSize = iSize;
            mReadAhead = iSize - 1;
        }

        ReadRange(
            int iSize,     ///< Size of the read window
            int iReadAhead ///< How far ahead in time
        )
        {
            mSize = iSize;
            mReadAhead = iReadAhead;
        }
        int Size() const { return mSize; }
        int Ahead() const { return mReadAhead; }
        int Behind() const
        {
            return mSize == INFINITE ? INFINITE : mSize - mReadAhead - 1;
        }
    private:
        int mSize;
        int mReadAhead;
    };

    /**
     * The type independent root of all components.
     *
     * ComponentBase is designed to be inherited by a type specific
     * implementation.
     */
    class ComponentBase : public Tracter::Object
    {
    public:
        ComponentBase(void);
        virtual ~ComponentBase(void) throw () {};

        int Read(CacheArea& oArea, IndexType iIndex, int iLength = 1);
        virtual void Reset(bool iPropagate = true);
        void Delete();

        /**
         * Frame rate as a float
         *
         * Common audio sample rates don't have 'nice' reciprocals.
         *
         * Originally, I had this concept of there being a single
         * source rate, and all components running at some integer
         * divisor of that.  The problem is that you could have two
         * sources running at different rates, one being downsampled
         * to the the other's rate, then another component connecting
         * to both.
         *
         * So now the rate is a recursive thing.  It could be just a
         * simple recursion, or more complicated components could faff
         * with it more.
         */
        virtual float FrameRate() const
        {
            ExactRateType r = ExactFrameRate();
            return r.rate / r.period;
        }

        /** Exact frame rate as a source rate plus divisor */
        virtual ExactRateType ExactFrameRate() const
        {
            if (mInput.size() <= 0)
                throw Exception("%s: No inputs", mObjectName);
            assert(mInput[0]);
            ExactRateType r = mInput[0]->ExactFrameRate();
            r.period *= mInput[0]->Frame().period;
            return r;
        }

        /** Time stamp for a given frame */
        virtual TimeType TimeStamp(IndexType iIndex = 0) const;

        /**
         * Time in seconds
         *
         * Calls TimeStamp(), which will return an absolute time.
         */
        double Seconds(IndexType iIndex) const
        {
            return (double)TimeStamp(iIndex) * 1.0e-9;
        };

        /**
         * Convert time to Frame index
         *
         * This is a relative call; time zero is frame zero.
         */
        IndexType FrameIndex(TimeType iTime)
        {
            return (IndexType)(iTime * FrameRate() / ONEe9);
        }

        /** Access the Frame structure */
        const FrameInfo& Frame() const
        {
            return mFrame;
        };

        /** Call a recursive chain that outputs a dot graph */
        void Dot();

    protected:

        /**
         * Set the range that will be read from an input component.
         *
         * This should be called by the derived class, which doesn't
         * have permission to call the input component directly.
         */
        void SetReadRange(ComponentBase* iInput, const ReadRange& iReadRange)
        {
            assert(iInput);
            iInput->MinSize(
                iReadRange.Size(), iReadRange.Behind(), iReadRange.Ahead()
            );
        }

        void SetClusterSize(int iSize);

        void MinSize(ComponentBase* iObject, int iMinSize, int iReadAhead = 0);
        void MinSize(
            ComponentBase* iInput, int iMinSize, int iReadBehind, int iReadAhead
        );
        void* Initialise(
            const ComponentBase* iDownStream = 0,
            int iReadBehind = 0, int iReadAhead = 0
        );
        ComponentBase* Connect(ComponentBase* iInput, int iSize = 1);
        ComponentBase* Connect(ComponentBase* iInput,
                              int iSize, int iReadAhead);
        void MovePointer(CachePointer& iPointer, int iLen);

        virtual void MinSize(int iMinSize, int iReadBehind, int iReadAhead);
        virtual void Resize(int iSize) = 0;
        virtual int Fetch(IndexType iIndex, CacheArea& iOutputArea);
        virtual int ContiguousFetch(
            IndexType iIndex, int iLength, int iOffset
        );

        /** Does exactly what it says on the tin */
        int SecondsToFrames(float iSeconds) const
        {
            float samples = iSeconds * FrameRate();
            return (int)(samples + 0.5);
        }

        TimeType TimeOffset(IndexType iIndex) const;

        /*
         * float mSampleFreq -> mFrame.rate -> FrameRate()
         * int mSamplePeriod -> mFrame.period
         * int mArraySize    -> mFrame.size
         */
        FrameInfo mFrame;
        std::vector<ComponentBase*> mInput; ///< Array of input components

        std::vector<CachePair> mCluster; ///< Circular cache maintainance

        int mSize;            ///< Size of the cache counted in frames
        int mNOutputs;        ///< Number of outputs
        bool mIndefinite;     ///< If true, cache grows indefinitely
        bool mAsync;          ///< Flag that the cache is updated asynchronously
        void* mAuxiliary;     ///< Common object for each component chain
        IndexType mEndOfData; ///< Index of the last datum available
        int mNInitialised;    ///< No. of outputs that have called Initialise()

        int mMinSize;         ///< Maximum requested minimum size
        int mMaxReadAhead;    ///< Maximum read-ahead of output buffers
        int mMinReadAhead;
        int mMaxReadBehind;
        int mMinReadBehind;
        int mTotalReadAhead;
        int mTotalReadBehind;
        MinMax mGlobalReadAhead;
        MinMax mGlobalReadBehind;

        virtual void DotHook() {}
        void DotRecord(int iVerbose, const char* iString, ...);

    private:
        int FetchWrapper(IndexType iIndex, CacheArea& iOutputArea);
        void Reset(ComponentBase* iDownStream);
        bool Delete(ComponentBase* iDownStream);

        const ComponentBase* mDownStream;

        int mDot;      ///< Dot index of this component
        struct DotInfo
        {
            int index; ///< Index of this component
            int max;   ///< Maximum upstream index
        };
        DotInfo Dot(int iDot);
    };


    /**
     * An interface for the type specific implementation of the component.
     * The implementation could be a cache or memory map.
     */
    template <class T>
    class Component : public ComponentBase
    {
    public:
        /**
         * Get a pointer to the storage
         * returns a reference.
         */
        virtual T* GetPointer(int iIndex = 0) = 0;

        /**
         * Unary read
         *
         * If we only want to read one item, the syntax can be rather
         * more simple.
         */
        const T* UnaryRead(int iIndex)
        {
            assert(iIndex >= 0);

            // Convert to a full Read(), return null pointer on failure
            CacheArea inputArea;
            int got = Read(inputArea, iIndex);
            if (!got)
                return 0;

            // If successful, return the pointer
            return GetPointer(inputArea.offset);
        }

        /**
         * Contiguous read
         *
         * Many components can work well by breaking down a Read()
         * into a pair of contiguous reads.  In this case, we can
         * return a pointer straight away as with the UnaryRead().  It
         * must be called twice.
         */
        const T* ContiguousRead(int iIndex, int& ioLength)
        {
            assert(iIndex >= 0);

            // Convert to a full Read(), return null pointer on failure
            CacheArea inputArea;
            int got = Read(inputArea, iIndex, ioLength);
            if (!got)
                return 0;

            // If successful, return the pointer and the length
            ioLength = inputArea.len[0];
            return GetPointer(inputArea.offset);
        }

    protected:

        /**
         * ContiguousFetch() is called by ComponentBase's
         * implementation of Fetch().  In turn, ContiguousFetch()
         * calls UnaryFetch().  A component should implement one of
         * these, depending on how it wishes to operate.  The latter
         * two are easier in that they are typed and supply pointers
         * directly.
         *
         * @returns the number of frames successfully fetched.  Fewer
         * than asked for indicates end of data (EOD).
         */
        virtual int ContiguousFetch(IndexType iIndex, int iLength, int iOffset)
        {
            assert(iIndex >= 0);
            assert(iLength >= 0);
            assert(iOffset >= 0);

            // Break the contiguous fetch into unary fetches
            for (int i=0; i<iLength; i++)
            {
                T* output = GetPointer(iOffset+i);
                if (!UnaryFetch(iIndex+i, output))
                    return i;
            }

            return iLength;
        }

        /**
         * UnaryFetch()   If a component does not implement Fetch() then it
         * must implement UnaryFetch().  A UnaryFetch() is only
         * required to return a single datum, but it may need to
         * Read() several input data to do so.
         *
         * @returns true if the fetch was successful, false otherwise,
         * implying end of data (EOD).
         */
        virtual bool UnaryFetch(IndexType iIndex, T* oData)
        {
            throw Exception("Component::UnaryFetch called."
                            " %s missing fetch method?", mObjectName);
            return false;
        }
    };


    /**
     * An iterator-like thing for caches.  This has the functionality of
     * an iterator, if not the standard interface.  It handles the
     * wrap-around nature of circular buffers.
     *
     * The implementation is rather ugly; favour the Unary/Contiguous
     * read methods if at all possible.
     */
    template <class T>
    class CacheIterator
    {
    public:
        CacheIterator(Component<T>* iComponent, CacheArea& iCacheArea)
        {
            mComponent = iComponent;
            mCacheArea = iCacheArea;
            mOffset = iCacheArea.offset;
        }

        /**
         * Index operator.  Returns a (reference to) the Nth object.
         */
        T& operator[](int iIndex)
        {
            assert( mOffset >= mCacheArea.offset ||
                    mOffset <  mCacheArea.len[1] );
            return mComponent->GetPointer(mOffset)[iIndex];
        }

        /**
         * Dereference operator.  Returns a (reference to) the object.
         */
        T& operator*()
        {
            assert( mOffset >= mCacheArea.offset ||
                    mOffset <  mCacheArea.len[1] );
            return *(mComponent->GetPointer(mOffset));
        }

        /**
         * Prefix operator.  Increments the iterator in an efficient way.
         */
        CacheIterator<T>& operator++()
        {
            if (++mOffset >= mCacheArea.offset + mCacheArea.len[0])
                mOffset = 0;
            return *this;
        }

        /**
         * Postfix operator.  Beware; this has to copy the iterator in
         * order to return the old one.  It's quicker to pre-increment if
         * you can.
         */
        CacheIterator<T> operator++(int dummy)
        {
            CacheIterator<T> old = *this; // Copy
            if (++mOffset >= mCacheArea.offset + mCacheArea.len[0])
                mOffset = 0;
            return old;
        }

        void Reset()
        {
            mOffset = mCacheArea.offset;
        }

    private:
        Component<T>* mComponent;
        CacheArea mCacheArea;
        int mOffset;
    };
}

#endif /* PLUGIN_H */
