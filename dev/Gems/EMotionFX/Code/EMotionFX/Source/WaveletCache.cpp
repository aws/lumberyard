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

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/WaveletHelper.h>
#include <MCore/Source/HaarWavelet.h>
#include <MCore/Source/D4Wavelet.h>
#include <MCore/Source/CDF97Wavelet.h>
#include "WaveletSkeletalMotion.h"
#include "SkeletalSubMotion.h"
#include "MorphSubMotion.h"
#include "SkeletalMotion.h"
#include "MotionInstance.h"
#include "ActorInstance.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(WaveletCache, WaveletCacheAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(WaveletCache::DecompressedChunk, WaveletCacheAllocator, 0)

    // huffman stats table for wavelet compressed motions
    uint32 WaveletCache::mHuffmanFrequencies[258] = { 213661, 44123, 25311, 16472, 11848, 8993, 7390, 5852, 4685, 4240, 3596, 3221, 2984, 2938, 2504, 2468, 2479, 2136, 1872, 1962, 1923, 1868, 1894, 1735, 1895, 1751, 1630, 1711, 1697, 1545, 1691, 1716, 1570, 1591, 1849, 1975, 2219, 2329, 2229, 2458, 2339, 2291, 2245, 2340, 2473, 3483, 2506, 2478, 2368, 2347, 2558, 2346, 2152, 2262, 2221, 2210, 2195, 2075, 2204, 2219, 2468, 2719, 3125, 5615, 7796, 3211, 2751, 2562, 2202, 2258, 2306, 2400, 2318, 2272, 2255, 2467, 2143, 2262, 2462, 2172, 2028, 2073, 2103, 2285, 2167, 2149, 2137, 2037, 2266, 1963, 3057, 2064, 1936, 2177, 1963, 1998, 1931, 2061, 1773, 1940, 2093, 1951, 2022, 2017, 1914, 1959, 1854, 2057, 1975, 2080, 1976, 2024, 2267, 2277, 1911, 1835, 2135, 2012, 1903, 1948, 2069, 1871, 2072, 1955, 2114, 2276, 2875, 4846, 1979, 1936, 2081, 1957, 1969, 1994, 2050, 1889, 1960, 1831, 1923, 1779, 1942, 1868, 1953, 2187, 2114, 1843, 1804, 1968, 1873, 1899, 2091, 1981, 1954, 2034, 1831, 1892, 2146, 1949, 1947, 1954, 2067, 2031, 1898, 1998, 2023, 2637, 2214, 2026, 1967, 2182, 2092, 2059, 2004, 2071, 2218, 2249, 2158, 2087, 1959, 1924, 2167, 2157, 2193, 2099, 2155, 2196, 2241, 2295, 1895, 2143, 2096, 2187, 2162, 2172, 2090, 2121, 2101, 2007, 1990, 2026, 2207, 2086, 2157, 2002, 2038, 2112, 2474, 2328, 2590, 2614, 3642, 2234, 2158, 2339, 2218, 2160, 2225, 2227, 2259, 2266, 2070, 2200, 1830, 1705, 1576, 1672, 1675, 1574, 1635, 1763, 1824, 1942, 1911, 1798, 2039, 1906, 1895, 1971, 2123, 2474, 2500, 2634, 3123, 3167, 3387, 4086, 4176, 5173, 5518, 7090, 8703, 12673, 16477, 25415, 43613, 99964, 165407, 1 };

    //-------------------------------------------------------------------------------------------------
    // class WaveletCache::DecompressedChunk
    //-------------------------------------------------------------------------------------------------

    // constructor
    WaveletCache::DecompressedChunk::DecompressedChunk()
        : BaseObject()
    {
        mRotations      = nullptr;
        mPositions      = nullptr;
        mMorphWeights   = nullptr;
        mMotion         = nullptr;
        mSizeInBytes    = 0;
        mTimeUnused     = 0.0f;
        mStartTime      = 0.0f;

        EMFX_SCALECODE
        (
            mScales         = nullptr;
            //mScaleRotations = nullptr;
        )
    }


    // destructor
    WaveletCache::DecompressedChunk::~DecompressedChunk()
    {
        MCore::Free(mRotations);
        MCore::Free(mPositions);
        MCore::Free(mMorphWeights);

        EMFX_SCALECODE
        (
            //MCore::Free( mScaleRotations );
            MCore::Free(mScales);
        )
    }


    // create
    WaveletCache::DecompressedChunk* WaveletCache::DecompressedChunk::Create()
    {
        return aznew DecompressedChunk();
    }


    // calculate the interpolation values
    void WaveletCache::DecompressedChunk::CalcInterpolationValues(float timeValue, uint32* outFirstSampleIndex, uint32* outSecondSampleIndex, float* outT)
    {
        // calculate the interpolation values
        const float sampleSpacing = mMotion->GetSampleSpacing();
        const float localTime = timeValue - mStartTime;
        *outFirstSampleIndex = (uint32)MCore::Math::Floor(localTime / sampleSpacing);
        *outSecondSampleIndex = *outFirstSampleIndex + 1;
        if (*outSecondSampleIndex > mNumSamples - 1)
        {
            *outSecondSampleIndex = *outFirstSampleIndex;
        }

        *outT = (localTime - (sampleSpacing * *outFirstSampleIndex)) / sampleSpacing;
    }


    // get the transformation
    void WaveletCache::DecompressedChunk::GetTransformAtTime(uint32 subMotionIndex, float t, uint32 firstSampleIndex, uint32 secondSampleIndex, Transform* outTransform)
    {
        // get the local submotion numbers and the submotion
        const WaveletSkeletalMotion::Mapping& mapping = mMotion->GetSubMotionMapping(subMotionIndex);
        const SkeletalSubMotion* subMotion = mMotion->GetSubMotion(subMotionIndex);

        // position
        if (mapping.mPosIndex != MCORE_INVALIDINDEX16)
        {
            const uint32 offset = mapping.mPosIndex * mNumSamples;
            const AZ::Vector3& posA = AZ::Vector3(mPositions[offset + firstSampleIndex]);
            const AZ::Vector3& posB = AZ::Vector3(mPositions[offset + secondSampleIndex]);
            outTransform->mPosition = MCore::LinearInterpolate<AZ::Vector3>(posA, posB, t) + subMotion->GetPosePos();
        }
        else
        {
            outTransform->mPosition = subMotion->GetPosePos();
        }

        // rotation
        if (mapping.mRotIndex != MCORE_INVALIDINDEX16)
        {
            const uint32 offset = mapping.mRotIndex * mNumSamples;
            const MCore::Quaternion rotationA = mRotations[offset + firstSampleIndex].ToQuaternion();
            const MCore::Quaternion rotationB = mRotations[offset + secondSampleIndex].ToQuaternion();
            outTransform->mRotation = rotationA.NLerp(rotationB, t);
        }
        else
        {
            outTransform->mRotation = subMotion->GetPoseRot();
        }

        EMFX_SCALECODE
        (
            /*      // scale rotation
                    if (mapping.mScaleRotIndex != MCORE_INVALIDINDEX16)
                    {
                        const uint32 offset = mapping.mScaleRotIndex * mNumSamples;
                        const MCore::Quaternion rotationA = mScaleRotations[offset + firstSampleIndex].ToQuaternion();
                        const MCore::Quaternion rotationB = mScaleRotations[offset + secondSampleIndex].ToQuaternion();
                        outTransform->mScaleRotation = rotationA.NLerp( rotationB, t );
                    }
                    else
                        outTransform->mScaleRotation = subMotion->GetPoseScaleRot();
            */
            // scale
            if (mapping.mScaleIndex != MCORE_INVALIDINDEX16)
            {
                const uint32 offset = mapping.mScaleIndex * mNumSamples;
                const AZ::Vector3& scaleA = AZ::Vector3(mScales[offset + firstSampleIndex]);
                const AZ::Vector3& scaleB = AZ::Vector3(mScales[offset + secondSampleIndex]);
                outTransform->mScale  = MCore::LinearInterpolate<AZ::Vector3>(scaleA, scaleB, t) + subMotion->GetPoseScale();
            }
            else
            {
                outTransform->mScale = subMotion->GetPoseScale();
            }
        ) // EMFX_SCALECODE
    }



    // get the morph weight
    void WaveletCache::DecompressedChunk::GetMorphWeightAtTime(uint32 morphSubMotionIndex, float t, uint32 firstSampleIndex, uint32 secondSampleIndex, float* outWeight)
    {
        // get the local submotion numbers and the submotion
        uint16 mapping = mMotion->GetMorphSubMotionMapping(morphSubMotionIndex);
        const MorphSubMotion* subMotion = mMotion->GetMorphSubMotion(morphSubMotionIndex);

        if (mapping != MCORE_INVALIDINDEX16)
        {
            const uint32 offset = mapping * mNumSamples;
            const float weightA = mMorphWeights[offset + firstSampleIndex];
            const float weightB = mMorphWeights[offset + secondSampleIndex];
            *outWeight = MCore::LinearInterpolate<float>(weightA, weightB, t) + subMotion->GetPoseWeight();
        }
        else
        {
            *outWeight = subMotion->GetPoseWeight();
        }
    }


    // get the transformation at a given time
    void WaveletCache::DecompressedChunk::GetTransformAtTime(uint32 subMotionIndex, float timeValue, Transform* outTransform)
    {
        // calculate the sample numbers to interpolate between
        const float sampleSpacing = mMotion->GetSampleSpacing();
        const float localTime = timeValue - mStartTime;
        const uint32 firstSampleIndex = (uint32)MCore::Math::Floor(localTime / sampleSpacing);
        uint32 secondSampleIndex = firstSampleIndex + 1;
        if (secondSampleIndex > mNumSamples - 1)
        {
            secondSampleIndex = firstSampleIndex;
        }

        // get the local submotion numbers
        const WaveletSkeletalMotion::Mapping& mapping = mMotion->GetSubMotionMapping(subMotionIndex);

        // calculate interpolation fraction
        const float t = (localTime - (sampleSpacing * firstSampleIndex)) / sampleSpacing;

        // get the submotion
        SkeletalSubMotion* subMotion = mMotion->GetSubMotion(subMotionIndex);

        // position
        if (mapping.mPosIndex != MCORE_INVALIDINDEX16)
        {
            const uint32 offset = mapping.mPosIndex * mNumSamples;
            const AZ::Vector3& posA = AZ::Vector3(mPositions[offset + firstSampleIndex]);
            const AZ::Vector3& posB = AZ::Vector3(mPositions[offset + secondSampleIndex]);
            outTransform->mPosition = MCore::LinearInterpolate<AZ::Vector3>(posA, posB, t) + subMotion->GetPosePos();
        }
        else
        {
            outTransform->mPosition = subMotion->GetPosePos();
        }

        // rotation
        if (mapping.mRotIndex != MCORE_INVALIDINDEX16)
        {
            const uint32 offset = mapping.mRotIndex * mNumSamples;
            const MCore::Quaternion rotationA = mRotations[offset + firstSampleIndex].ToQuaternion();
            const MCore::Quaternion rotationB = mRotations[offset + secondSampleIndex].ToQuaternion();
            outTransform->mRotation = rotationA.NLerp(rotationB, t);
        }
        else
        {
            outTransform->mRotation = subMotion->GetPoseRot();
        }

        EMFX_SCALECODE
        (
            /*      // scale rotation
                    if (mapping.mScaleRotIndex != MCORE_INVALIDINDEX16)
                    {
                        const uint32 offset = mapping.mScaleRotIndex * mNumSamples;
                        const MCore::Quaternion rotationA = mScaleRotations[offset + firstSampleIndex].ToQuaternion();
                        const MCore::Quaternion rotationB = mScaleRotations[offset + secondSampleIndex].ToQuaternion();
                        outTransform->mScaleRotation = rotationA.NLerp( rotationB, t );
                    }
                    else
                        outTransform->mScaleRotation = subMotion->GetPoseScaleRot();*/

            // scale
            if (mapping.mScaleIndex != MCORE_INVALIDINDEX16)
            {
                const uint32 offset = mapping.mScaleIndex * mNumSamples;
                const AZ::Vector3& scaleA = AZ::Vector3(mScales[offset + firstSampleIndex]);
                const AZ::Vector3& scaleB = AZ::Vector3(mScales[offset + secondSampleIndex]);
                outTransform->mScale  = MCore::LinearInterpolate<AZ::Vector3>(scaleA, scaleB, t) + subMotion->GetPoseScale();
            }
            else
            {
                outTransform->mScale = subMotion->GetPoseScale();
            }
        ) // EMFX_SCALECODE
    }



    // get the morph weights at a given time
    void WaveletCache::DecompressedChunk::GetMorphWeightAtTime(uint32 morphSubMotionIndex, float timeValue, float* outWeight)
    {
        // calculate the sample numbers to interpolate between
        const float sampleSpacing = mMotion->GetSampleSpacing();
        const float localTime = timeValue - mStartTime;
        const uint32 firstSampleIndex = (uint32)MCore::Math::Floor(localTime / sampleSpacing);
        uint32 secondSampleIndex = firstSampleIndex + 1;
        if (secondSampleIndex > mNumSamples - 1)
        {
            secondSampleIndex = firstSampleIndex;
        }

        // get the local submotion numbers
        const uint16 mapping = mMotion->GetMorphSubMotionMapping(morphSubMotionIndex);

        // calculate interpolation fraction
        const float t = (localTime - (sampleSpacing * firstSampleIndex)) / sampleSpacing;

        // get the submotion
        MorphSubMotion* subMotion = mMotion->GetMorphSubMotion(morphSubMotionIndex);

        if (mapping != MCORE_INVALIDINDEX16)
        {
            const uint32 offset = mapping * mNumSamples;
            const float weightA = mMorphWeights[offset + firstSampleIndex];
            const float weightB = mMorphWeights[offset + secondSampleIndex];
            *outWeight = MCore::LinearInterpolate<float>(weightA, weightB, t) + subMotion->GetPoseWeight();
        }
        else
        {
            *outWeight = subMotion->GetPoseWeight();
        }
    }



    //-------------------------------------------------------------------------------------------------
    // class WaveletCache
    //-------------------------------------------------------------------------------------------------

    // constructor
    WaveletCache::WaveletCache()
        : BaseObject()
    {
        ReleaseThreads();

        mTotalSizeInBytes   = 0;
        mMaxSizeInBytes     = (1024 * 1024) * 2;// 2 mb cache default size
        mCacheHits          = 0;
        mCacheMisses        = 0;
        mThreads.SetMemoryCategory(EMFX_MEMCATEGORY_WAVELETCACHE);
        mEntries.SetMemoryCategory(EMFX_MEMCATEGORY_WAVELETCACHE);

        InitThreads(GetEMotionFX().GetNumThreads());

        // init the huffman compressor
        mHuffmanCompressor.InitFromStats(mHuffmanFrequencies, 258);

        // reserve space for 1000 different motions, to prevent any re-allocs
        ReserveCacheSize(1000);
    }


    // the destructor
    WaveletCache::~WaveletCache()
    {
        ReleaseThreads();
        Release();
    }


    // create
    WaveletCache* WaveletCache::Create()
    {
        return aznew WaveletCache();
    }


    // init the buffers
    void WaveletCache::InitThreads(uint32 numThreads)
    {
        ReleaseThreads();

        mThreads.Resize(numThreads);

        // init all threads
        for (uint32 i = 0; i < numThreads; ++i)
        {
            ThreadInfo& thread = mThreads[i];
            thread.mDecompressBuffer    = nullptr;
            thread.mDataBuffer          = nullptr;
            thread.mDataBufferSize      = 0;
            thread.mDecompressBufferSize = 0;
            thread.mWavelets[WaveletSkeletalMotion::WAVELET_HAAR]  = new MCore::HaarWavelet();
            thread.mWavelets[WaveletSkeletalMotion::WAVELET_DAUB4] = new MCore::D4Wavelet();
            thread.mWavelets[WaveletSkeletalMotion::WAVELET_CDF97] = new MCore::CDF97Wavelet();
        }
    }


    // release thread unique data from memory
    void WaveletCache::ReleaseThreads()
    {
        // release memory
        const uint32 numThreads = mThreads.GetLength();
        for (uint32 i = 0; i < numThreads; ++i)
        {
            ThreadInfo& thread = mThreads[i];
            MCore::Free(thread.mDataBuffer);
            MCore::Free(thread.mDecompressBuffer);
            for (uint32 w = 0; w < 3; ++w)
            {
                delete thread.mWavelets[w];
            }
        }

        // clear the buffers
        mThreads.Clear();
    }


    // release all entries
    void WaveletCache::Release()
    {
        // remove all cache entries
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            CacheEntry& entry = mEntries[i];
            const uint32 numChunks = entry.mChunks.GetLength();
            for (uint32 c = 0; c < numChunks; ++c)
            {
                entry.mChunks[c]->Destroy();
            }
            entry.mChunks.Clear();
        }

        mEntries.Clear();
    }


    // make sure the decompression buffer is large enough
    uint8* WaveletCache::AssureDecompressBufferSize(uint32 threadIndex, uint32 numBytes)
    {
        MCORE_ASSERT(mThreads.GetLength() > threadIndex);

        // get the thread info
        ThreadInfo& thread = mThreads[threadIndex];

        // if we need to resize it
        if (thread.mDecompressBufferSize < numBytes)
        {
            thread.mDecompressBuffer = (uint8*)MCore::Realloc(thread.mDecompressBuffer, numBytes);
            thread.mDecompressBufferSize = numBytes;
        }

        return thread.mDecompressBuffer;
    }



    // make sure the data buffer can contain a given number of floats
    float* WaveletCache::AssureDataBufferSize(uint32 threadIndex, uint32 numFloats)
    {
        MCORE_ASSERT(mThreads.GetLength() > threadIndex);

        // get the buffer info
        ThreadInfo& thread = mThreads[threadIndex];

        // if we need to resize
        if (thread.mDataBufferSize < numFloats)
        {
            thread.mDataBuffer = (float*)MCore::Realloc(thread.mDataBuffer, numFloats * sizeof(float));
            thread.mDataBufferSize = numFloats;
        }

        return thread.mDataBuffer;
    }



    // get a given chunk at a given time from the cache, and decompress it if needed
    WaveletCache::DecompressedChunk* WaveletCache::GetChunkAtTime(float timeValue, WaveletSkeletalMotion* motion, uint32 threadIndex)
    {
        // check if we already decompressed this chunk
        LockChunks();
        DecompressedChunk* chunk = FindChunkAtTime(timeValue, motion);
        if (chunk)
        {
            mCacheHits++;
            chunk->mTimeUnused = 0.0f; // reset the unused timer
            UnlockChunks();
            return chunk;
        }

        mCacheMisses++;

        // find the chunk we need to decompress
        WaveletSkeletalMotion::Chunk* compressedChunk = motion->FindChunkAtTime(timeValue);
        MCORE_ASSERT(compressedChunk);
        UnlockChunks();

        // decompress the chunk
        DecompressedChunk* decompressedChunk = DecompressedChunk::Create();
        motion->DecompressChunk(compressedChunk, decompressedChunk, threadIndex);

        // add it to the list of chunks in the cache
        LockChunks();
        AddChunk(decompressedChunk);
        UnlockChunks();

        //MCore::LogDetailedInfo("Cache size = %d bytes (%d kb) - entries = %d", mTotalSizeInBytes, mTotalSizeInBytes / 1024, mEntries.GetLength());
        return decompressedChunk;
    }


    // shrink the cache
    void WaveletCache::Shrink(WaveletCache::DecompressedChunk* chunkToIgnore)
    {
        if (mTotalSizeInBytes <= mMaxSizeInBytes)
        {
            return;
        }

        // make sure we don't go over the allowed maximum cache size
        uint32 entryIndex = 0;
        uint32 chunkIndex = 0;
        while (mTotalSizeInBytes > mMaxSizeInBytes)
        {
            // find the least recently used item
            if (FindLeastRecentUsedChunk(&entryIndex, &chunkIndex) == false)
            {
                break;
            }

            // prevent deleting itself
            if (mEntries[entryIndex].mChunks[chunkIndex] == chunkToIgnore)
            {
                break;
            }

            // remove the entry
            //MCore::LogDetailedInfo("removed item %d, %d unused time=%f", entryIndex, chunkIndex, mEntries[entryIndex].mChunks[chunkIndex]->mTimeUnused);
            RemoveChunk(entryIndex, chunkIndex, true);
        }
    }



    // get the chunk at a given time from the cache
    WaveletCache::DecompressedChunk* WaveletCache::FindChunkAtTime(float timeValue, WaveletSkeletalMotion* motion)
    {
        // for all entries
        const uint32 entryIndex = FindCacheEntry(motion);
        if (entryIndex == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        // get a shortcut to the cache entry
        const CacheEntry& entry = mEntries[entryIndex];

        // for all chunks inside the list of chunks for this motions
        const uint32 numChunks = entry.mChunks.GetLength();
        for (uint32 i = 0; i < numChunks; ++i)
        {
            WaveletCache::DecompressedChunk* curChunk = entry.mChunks[i];

            // check if the time value is in range
            if (timeValue >= curChunk->mStartTime && timeValue < curChunk->mStartTime + ((curChunk->mNumSamples - 1) * motion->GetSampleSpacing()))
            {
                return curChunk;
            }
        }

        return nullptr;
    }


    // find the cache entry index for a given motion
    uint32 WaveletCache::FindCacheEntry(WaveletSkeletalMotion* motion) const
    {
        // check all entries
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            if (mEntries[i].mMotion->GetID() == motion->GetID())
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // add the decompressed chunk to the cache
    void WaveletCache::AddChunk(DecompressedChunk* chunk)
    {
        // find out if there is already an existing cache entry for this motion
        uint32 index = FindCacheEntry(chunk->mMotion);
        if (index != MCORE_INVALIDINDEX32)
        {
            mEntries[index].mChunks.Add(chunk);
        }
        else
        {
            // create a new entry
            mEntries.AddEmpty();
            CacheEntry& entry = mEntries.GetLast();
            entry.mMotion = chunk->mMotion;
            entry.mChunks.Reserve(chunk->mMotion->GetNumChunks());  // reserve space for all its decompressed chunks
            entry.mChunks.Add(chunk); // add the chunk to the list
        }

        mTotalSizeInBytes += chunk->GetSizeInBytes();
    }


    // find the indices for a given chunk
    bool WaveletCache::FindChunk(DecompressedChunk* chunk, uint32* outEntryNumber, uint32* outChunkNumber) const
    {
        WaveletSkeletalMotion* motion = chunk->mMotion;

        // check all entries
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            const CacheEntry& entry = mEntries[i];

            // if this entry uses the motion we're looking for
            if (entry.mMotion->GetID() == motion->GetID())
            {
                // check all chunks
                const uint32 numChunks = entry.mChunks.GetLength();
                for (uint32 c = 0; c < numChunks; ++c)
                {
                    if (entry.mChunks[c] == chunk)
                    {
                        *outEntryNumber = i;
                        *outChunkNumber = c;
                        return true;
                    }
                }

                // this motion hasn't got the chunk registered
                return false;
            }
        }

        // none of the entries use the given motion
        return false;
    }


    // remove the chunk
    bool WaveletCache::RemoveChunk(DecompressedChunk* chunk, bool delFromMem)
    {
        uint32 entryIndex;
        uint32 chunkIndex;

        // try to find the chunk location in the cache
        if (FindChunk(chunk, &entryIndex, &chunkIndex) == false) // if it isn't in the cache, we can't remove it from it
        {
            return false;
        }

        // remove the chunk by its indices
        RemoveChunk(entryIndex, chunkIndex, delFromMem);

        return true;
    }


    // remove a chunk by its indices
    void WaveletCache::RemoveChunk(uint32 entryIndex, uint32 chunkIndex, bool delFromMem)
    {
        // get the chunk pointer
        DecompressedChunk* chunk = mEntries[entryIndex].mChunks[chunkIndex];

        // reduce mem usage
        mTotalSizeInBytes -= chunk->GetSizeInBytes();

        // remove it from the cache
        mEntries[entryIndex].mChunks.Remove(chunkIndex);

        // if there are no other chunks left, let's free up this entry
        if (mEntries[entryIndex].mChunks.GetLength() == 0)
        {
            mEntries.Remove(entryIndex);
        }

        // remove it from memory if we want to
        if (delFromMem)
        {
            chunk->Destroy();
        }
    }


    // find the least recently used chunk
    bool WaveletCache::FindLeastRecentUsedChunk(uint32* outEntryIndex, uint32* outChunkIndex) const
    {
        DecompressedChunk* chunk = nullptr;

        float maxTime = -1.0f;

        // for all entries in the cache
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            const CacheEntry& entry = mEntries[i];

            // for all chunks in the entry
            const uint32 numChunks = entry.mChunks.GetLength();
            for (uint32 c = 0; c < numChunks; ++c)
            {
                if (entry.mChunks[c]->mTimeUnused > maxTime)
                {
                    chunk   = entry.mChunks[c];
                    maxTime = chunk->mTimeUnused;
                    *outEntryIndex = i;
                    *outChunkIndex = c;
                }
            }
        }

        // check if we found a chunk
        return (chunk != nullptr);
    }


    // update the wavelet cache
    void WaveletCache::Update(float timeInSeconds)
    {
        mCacheHits      = 0;
        mCacheMisses    = 0;

        // for all entries in the cache
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            const CacheEntry& entry = mEntries[i];

            // for all chunks in the entry
            const uint32 numChunks = entry.mChunks.GetLength();
            for (uint32 c = 0; c < numChunks; ++c)
            {
                entry.mChunks[c]->mTimeUnused += timeInSeconds;
            }
        }

        // shrink the cache
        Shrink();
    }


    // remove the chunks that use a given motion
    void WaveletCache::RemoveChunksForMotion(WaveletSkeletalMotion* motion)
    {
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            CacheEntry& entry = mEntries[i];

            // if this entry is for the motion we're searching for
            if (mEntries[i].mMotion->GetID() == motion->GetID())
            {
                // delete all chunks in the entry
                const uint32 numChunks = entry.mChunks.GetLength();
                for (uint32 c = 0; c < numChunks; ++c)
                {
                    mTotalSizeInBytes -= entry.mChunks[c]->GetSizeInBytes();
                    entry.mChunks[c]->Destroy();
                }
                entry.mChunks.Clear();

                // remove the entry itself
                mEntries.Remove(i);
                return;
            }
        }
    }


    // lock the chunks
    void WaveletCache::LockChunks()
    {
        mChunkLock.Lock();
    }


    // unlock the chunks
    void WaveletCache::UnlockChunks()
    {
        mChunkLock.Unlock();
    }


    void WaveletCache::ReserveCacheSize(uint32 numMotions)
    {
        mEntries.Reserve(numMotions);
    }


    void WaveletCache::SetMaxCacheSize(uint32 numBytes)
    {
        mMaxSizeInBytes = numBytes;
    }


    uint32 WaveletCache::GetMaxCacheSize() const
    {
        return mMaxSizeInBytes;
    }


    uint32 WaveletCache::GetMemUsage() const
    {
        return mTotalSizeInBytes;
    }


    uint32 WaveletCache::GetNumEntries() const
    {
        return mEntries.GetLength();
    }


    uint32 WaveletCache::GetNumCacheHits() const
    {
        return mCacheHits;
    }


    uint32 WaveletCache::GetNumCacheMisses() const
    {
        return mCacheMisses;
    }


    float WaveletCache::CalcCacheHitPercentage() const
    {
        if ((mCacheHits + mCacheMisses) > 0)
        {
            return mCacheHits / (float)(mCacheHits + mCacheMisses) * 100.0f;
        }

        return 0.0f;
    }


    MCore::Wavelet* WaveletCache::GetWavelet(uint32 threadNumber, uint32 waveletIndex)
    {
        return mThreads[threadNumber].mWavelets[waveletIndex];
    }


    const MCore::HuffmanCompressor& WaveletCache::GetCompressor() const
    {
        return mHuffmanCompressor;
    }


    MCore::HuffmanCompressor& WaveletCache::GetCompressor()
    {
        return mHuffmanCompressor;
    }
}   // namespace EMotionFX
