/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"
#include "Transform.h"

#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/CompressedQuaternion.h>
#include <MCore/Source/Huffman.h>
#include <MCore/Source/Wavelet.h>
#include <MCore/Source/MultiThreadManager.h>


namespace EMotionFX
{
    // forward declarations
    class WaveletSkeletalMotion;


    /**
     * The wavelet decompression cache.
     * This is used to decompress the compressed chunks of wavelet skeletal motions.
     * You can access the cache used by EMotion Studio with the GetWaveletCache() define macro.
     * The function you most likely only will use is WaveletCache::SetMaxCacheSize(), using GetWaveletCache().SetMaxCacheSize(...).
     * It is important to know that the size of the cache can have dramatic impact on performance. Using a too small cache size with many characters and different motions
     * can lead to very poor performance. The default cache size is set to 2 megabyte.
     * It is possible that in a multithreaded environment the total cache usage goes a bit over the maximum allowed cache size.
     * This is done for performance reasons, as shrinking the cache in multiple threads would stall things. The amount going over the maximum size is not much though.
     * If you are NOT using our actor update scheduler system, so not using the callback system to update and render the actor instances, then please have a look at the
     * WaveletCache::Shrink(...) and WaveletCache::Update(...) methods, as you will need to call them manually.
     */
    class EMFX_API WaveletCache
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

        friend class WaveletSkeletalMotion;

    public:
        /**
         * A decompressed chunk of wavelet compressed motion.
         * This stores all data uncompressed, except for the rotations, which are stored quantized still.
         */
        class EMFX_API DecompressedChunk
            : public BaseObject
        {
            AZ_CLASS_ALLOCATOR_DECL

        public:
            static DecompressedChunk* Create();

            /**
             * Get the transformation for a given submotion at a given time.
             * @param subMotionIndex The submotion to get the transformation for.
             * @param timeValue The time, in seconds.
             * @param outTransform The output transformation in which the transformation data will be stored.
             */
            void GetTransformAtTime(uint32 subMotionIndex, float timeValue, Transform* outTransform);

            /**
             * Calculate the interpolation values, to use during interpolation between two keyframes.
             * Since this will be the same for all submotions at a given time value, because all keyframes are uniformly spaced, we can precalculate and reuse the interpolation factor.
             * @param timeValue The time value you get the tranformation for.
             * @param outFirstSampleIndex The variable in which to store the first keyframe sample index.
             * @param outSecondSampleIndex The variable in which to store the second keyframe sample index.
             * @param outT The output interpolation value, which will be a value between 0 and 1.
             */
            void CalcInterpolationValues(float timeValue, uint32* outFirstSampleIndex, uint32* outSecondSampleIndex, float* outT);

            /**
             * Get the transformation at a given time, using previously calculated interpolation values.
             * @param subMotionIndex The submotion to calculate the transformation for.
             * @param t The interpolation value, which is a value between 0 and 1.
             * @param firstSampleIndex The first sample index.
             * @param secondSampleIndex The second sample index.
             * @param outTransform The output transformation.
             */
            void GetTransformAtTime(uint32 subMotionIndex, float t, uint32 firstSampleIndex, uint32 secondSampleIndex, Transform* outTransform);

            /**
             * Get the morph target weight at a given time, using previously calculated interpolation values.
             * @param morphSubMotionIndex The morph submotion to calculate the weight for.
             * @param t The interpolation value, which is a value between 0 and 1.
             * @param firstSampleIndex The first sample index.
             * @param secondSampleIndex The second sample index.
             * @param outWeight The output weight value.
             */
            void GetMorphWeightAtTime(uint32 morphSubMotionIndex, float t, uint32 firstSampleIndex, uint32 secondSampleIndex, float* outWeight);

            /**
             * Get the morph target weight for a given morph submotion at a given time.
             * @param morphSubMotionIndex The submotion to get the weight for.
             * @param timeValue The time, in seconds.
             * @param outWeight The output float in which the weight value will be stored.
             */
            void GetMorphWeightAtTime(uint32 morphSubMotionIndex, float timeValue, float* outWeight);

            /**
             * Get the size of the decompressed data, in bytes.
             * @result The size of the decompressed data, in bytes.
             */
            MCORE_INLINE uint32 GetSizeInBytes() const          { return mSizeInBytes; }

        public:
            MCore::Compressed16BitQuaternion*   mRotations;         /**< The quantized, but decompressed rotation values. */
            AZ::PackedVector3f*                 mPositions;         /**< The decompressed position values. */
            float*                              mMorphWeights;      /**< The decompressed morph values. */
            float                               mStartTime;         /**< The start time of the decompressed chunk. */
            float                               mTimeUnused;        /**< The total time, in seconds, that this decompressed chunk was not accessed. */
            uint32                              mNumSamples;        /**< The number of samples stored inside this chunk. */
            uint32                              mSizeInBytes;       /**< The memory size of the chunk, in bytes. */
            WaveletSkeletalMotion*              mMotion;            /**< The wavelet compressed motion where this chunk was decompressed from. */

            EMFX_SCALECODE
            (
                //MCore::Compressed16BitQuaternion* mScaleRotations;    /**< The decompressed, but quantized scale rotation values. */
                AZ::PackedVector3f *             mScales;               /**< The decompressed scale values. */
            )

            DecompressedChunk();
            ~DecompressedChunk();
        };

        /**
         * The create method.
         * Please note that you shouldn't create the cache yourself. It is already created for you at EMotion FX initialization time.
         * You can access the global wavelet cache with GetWaveletCache().
         */
        static WaveletCache* Create();

        /**
         * Shrink the wavelet cache, making sure the total used size does not go over the maximum allowed cache size.
         * The Shrink function is automatically called by the schedulers of EMotion FX. If you do not use the actor update scheduler system, you have to call this
         * shrink method yourself at the moment you want, for example after updating each ActorInstance's transformations.
         * @param chunkToIgnore A pointer to the chunk that will not get removed in any case, or nullptr when there is no such chunk.
         */
        void Shrink(DecompressedChunk* chunkToIgnore = nullptr);

        /**
         * Reserve space for a given amount of cache entries.
         * This prevents some re-allocs to store the array of decompressed chunks.
         * The default value is 1000, which should be enough.
         * @param numMotions The number of motions that can exist, without having to perform a realloc.
         */
        void ReserveCacheSize(uint32 numMotions);

        /**
         * Get a decompressed chunk at a given time.
         * When this decompressed chunk does not exist yet, it will create one by decomrpessing the related compressed chunk.
         * It does not shrink the cache if its memory usage goes over the maximum allowed.
         * The Shrink method is called by the actor update schedulers.
         */
        DecompressedChunk* GetChunkAtTime(float timeValue, WaveletSkeletalMotion* motion, uint32 threadIndex);

        /**
         * Set the maximum cache size, in bytes.
         * The default size is set to two megabyte.
         * It is still possible that the total memory usage at some points will go a little bit over the maximum allowed size though.
         * This happens only in multithreaded environments and is done for performance reasons, to prevent locks. This is a really small amount though, which can be ignored.
         * @param numBytes The maximum allowed cache size.
         */
        void SetMaxCacheSize(uint32 numBytes);

        /**
         * Get the maximum allowed cache size, in bytes.
         * The default size is set to two megabyte.
         * It is still possible that the total memory usage at some points will go a little bit over the maximum allowed size though.
         * This happens only in multithreaded environments and is done for performance reasons, to prevent locks. This is a really small amount though, which can be ignored.
         * @result The maximum allowed cache size, in bytes.
         */
        uint32 GetMaxCacheSize() const;

        /**
         * Get the currently used amount of memory by the cache.
         * @result The total used cache memory, in bytes.
         */
        uint32 GetMemUsage() const;

        /**
         * Get the total number of cache entries.
         * This is NOT the number of decompressed chunks, but the number of motions that have decompressed chunks inside the cache.
         * @result The number of motions that have decompressed chunks inside the cache.
         */
        uint32 GetNumEntries() const;

        /**
         * Get the number of cache hits.
         * Each hit means that we could reuse an already decompressed chunk.
         * @result The number of cache hits.
         */
        uint32 GetNumCacheHits() const;

        /**
         * Get the number of cache misses.
         * Each miss means that we had to decompress a compressed chunk, as it didn't exist in the cache yet.
         * Cache misses are very expensive as you can imagine.
         * @result The number of cache misses.
         */
        uint32 GetNumCacheMisses() const;

        /**
         * The cache hit percentage.
         * This is a value between 0 and 100, where 0 would mean there have been no cache hits at all, and a value of 100 would mean there have only been cache hits.
         * The higher this value, the faster performance will be. If you see very low numbers or have low performance, try increasing the cache size with the SetMaxCacheSize(...) method.
         * @result The cache hit percentage.
         */
        float CalcCacheHitPercentage() const;

        /**
         * Update the cache. This increases the timers of all decompressed chunks.
         * Because this is a least-recently-used (LRU) cache, it will kick out the decompressed chunks that haven't been used for the longest time when the cache is full.
         * This is done automatically by the actor update schedulers. If you do not use the update schedulers (using the OnRender and OnUpdateTransformations callbacks) then you should
         * manually call this method every frame.
         * @param timeInSeconds The time in seconds since the last update or frame.
         */
        void Update(float timeInSeconds);

        /**
         * Get the wavelet object for a given thread.
         * @param threadNumber The thread index that requests this.
         * @param waveletIndex The wavelet number.
         * @result A pointer to the wavelet object.
         */
        MCore::Wavelet* GetWavelet(uint32 threadNumber, uint32 waveletIndex);

        const MCore::HuffmanCompressor& GetCompressor() const;

        MCore::HuffmanCompressor& GetCompressor();

        void InitThreads(uint32 numThreads);


    private:
        /**
         * A cache entry.
         * This represents an array of decompressed chunks for a given motion.
         */
        struct EMFX_API CacheEntry
        {
            WaveletSkeletalMotion*              mMotion;    /**< The motion where the chunks belong to. */
            MCore::Array<DecompressedChunk*>    mChunks;    /**< The collection of decompressed chunks for this motion. */
        };

        /**
         * The per thread-unique data.
         * This is used to prevent locks.
         */
        struct EMFX_API ThreadInfo
        {
            float*              mDataBuffer;                    /**< The data float data buffer. */
            uint8*              mDecompressBuffer;              /**< The decompression buffer. */
            uint32              mDecompressBufferSize;          /**< The size of the decompression buffer, in bytes. */
            uint32              mDataBufferSize;                /**< The size of the data buffer, in bytes. */
            MCore::Wavelet*     mWavelets[3];                   /**< The wavelet objects. */
        };

        MCore::Array<CacheEntry>            mEntries;           /**< The cache entries. One for each motion that has decompressed chunks in the cache. */
        MCore::Array<ThreadInfo>            mThreads;           /**< The unique data per thread, used to prevent multithreading locks. */

        uint32                              mTotalSizeInBytes;  /**< The total size in bytes, used by the cache right now. */
        uint32                              mMaxSizeInBytes;    /**< The maximum allowed size in bytes. Defaults on 2 MB.*/
        uint32                              mCacheHits;         /**< The number of cache hits. */
        uint32                              mCacheMisses;       /**< The number of cache misses. */

        MCore::HuffmanCompressor            mHuffmanCompressor;         /**< The Huffman compressor object. */
        static uint32                       mHuffmanFrequencies[258];   /**< Global huffman statistics, used to build the huffman tree. */

        MCore::Mutex                        mChunkLock;

        /**
         * The constructor.
         * Please note that you shouldn't create the cache yourself. It is already created for you at EMotion FX initialization time.
         * You can access the global wavelet cache with GetWaveletCache().
         */
        WaveletCache();

        /**
         * The destructor.
         */
        ~WaveletCache();

        WaveletCache::DecompressedChunk* FindChunkAtTime(float timeValue, WaveletSkeletalMotion* motion);
        uint32 FindCacheEntry(WaveletSkeletalMotion* motion) const;
        void AddChunk(DecompressedChunk* chunk);
        bool FindChunk(DecompressedChunk* chunk, uint32* outEntryNumber, uint32* outChunkNumber) const;
        bool RemoveChunk(DecompressedChunk* chunk, bool delFromMem);
        void RemoveChunk(uint32 entryIndex, uint32 chunkIndex, bool delFromMem);
        bool FindLeastRecentUsedChunk(uint32* outEntryIndex, uint32* outChunkIndex) const;
        void ReleaseThreads();
        void Release();

        /**
         * Multithread lock the arrays of decompressed chunks.
         */
        void LockChunks();

        /**
         * Multithread unlock the decompressed chunks.
         */
        void UnlockChunks();

        /**
         * Make sure the decompress buffer sizes for a given thread are large enough to store a given number of bytes.
         * @param threadIndex The thread index.
         * @param numBytes The number of bytes required to store in this buffer.
         * @result A pointer to the buffer which can hold this amount of data.
         */
        uint8* AssureDecompressBufferSize(uint32 threadIndex, uint32 numBytes);

        /**
         * Make sure the data buffer sizes for a given thread are large enough to store a given number of floats.
         * @param threadIndex The thread index.
         * @param numFloats The number of floats this buffer needs to store.
         * @result A pointer to the buffer which can hold this amount of float values.
         */
        float* AssureDataBufferSize(uint32 threadIndex, uint32 numFloats);

        /**
         * Remove all chunks for a given motion object.
         * @param motion The motion to delete all chunks for.
         */
        void RemoveChunksForMotion(WaveletSkeletalMotion* motion);
    };
}   // namespace EMotionFX
