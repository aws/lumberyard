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
#include "SkeletalMotion.h"
#include "WaveletCache.h"

#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>
#include <MCore/Source/SmallArray.h>
#include <MCore/Source/Wavelet.h>


namespace EMotionFX
{
    // forward declarations
    class WaveletSkeletalMotion;
    class SkeletalSubMotion;
    class SkeletalMotion;


    /**
     * The wavelet skeletal motion.
     * This is a skeletal motion compressed using wavelet compression.
     * In order to achieve this, the motion is split up into small sections, called chunks, which are represented by the WaveletSkeletalMotion::Chunk class.
     * Each chunk represents a compressed section of the motion. These chunks are being decompressed on the fly when needed.
     * A caching system prevents chunks from being decompressed every single time. The cache is can be controlled by the WaveletCache class, which can be accessed with the
     * global shortcut define GetWaveletCache(). You can setup the maximum size of the decompression cache.
     * Depending on the cache size, performance can be either higher or lower compared to using the traditional SkeletalMotion class. Overall you have to assume using WaveletSkeletalMotions
     * is slower than regular SkeletalMotion's. How much slower depends on the size of the wavelet decompression cache and how many characters are being updated and how many motions they have playing on them.
     * You can also let some motions be compressed with wavelets while the others use regular keyframe optimizations.
     * The memory usage of wavelet motions is a lot lower than standard SkeletalMotion's though. It is always a trade-off between memory usage and performance.
     * Normally you will not really use this class, but the SkeletalMotion base class, from which it has been derived. As a programmer you will not really notice any difference
     * between compressed or uncompressed motions.
     */
    class EMFX_API WaveletSkeletalMotion
        : public SkeletalMotion
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        enum
        {
            TYPE_ID = 0x00000003
        };

        /**
         * A compressed chunk of motion data, which represents a section of a motion.
         * A motion is basically divided into smaller sections, which we call chunks. Each chunk is compressed separately.
         * This class represents such a compressed chunk of motion data.
         */
        class EMFX_API Chunk
            : public BaseObject
        {
            AZ_CLASS_ALLOCATOR_DECL

        public:
            static Chunk* Create();

        public:
            uint8*                  mCompressedRotData;                 /**< The compressed rotation data. */
            uint8*                  mCompressedPosData;                 /**< The compressed position data. */
            EMFX_SCALECODE(uint8 *  mCompressedScaleData; /**< The compressed scale data. */)
            uint8 *                  mCompressedMorphData;
            float                   mRotQuantScale;                     /**< The rotation quantization scale factor. */
            float                   mPosQuantScale;                     /**< The position quantization scale factor. */
            EMFX_SCALECODE(float   mScaleQuantScale; /**< The scale quantization scale factor. */)
            float                   mMorphQuantScale;
            float                   mStartTime;                         /**< The start time of the chunk, in the motion. */
            uint32                  mCompressedRotNumBytes;             /**< The size in bytes, of the compressed rotation data. */
            uint32                  mCompressedPosNumBytes;             /**< The size in bytes, of the compressed position data. */
            EMFX_SCALECODE(uint32  mCompressedScaleNumBytes; /**< The size in bytes, of the compressed scale data. */)
            uint32                  mCompressedMorphNumBytes;
            uint32                  mCompressedRotNumBits;              /**< The size in bits, of the compressed rotation data. This is currently unused. */
            uint32                  mCompressedPosNumBits;              /**< The size in bits, of the compressed position data. This is currently unused. */
            EMFX_SCALECODE(uint32  mCompressedScaleNumBits; /**< The size in bits, of the compressed scale data. This is currently unused. */)
            uint32                  mCompressedMorphNumBits;

        private:
            /**
             * The constructor.
             * Initializes all members.
             */
            Chunk();

            /**
             * The destructor, which deletes all chunk memory.
             */
            ~Chunk();
        };

        /**
         * This structure maps global submotion numbers into wavelet submotion numbers.
         * Wavelet submotion numbers exclude submotions that have no animation data. So submotion number 3 in a SkeletalMotion object might not be animated.
         * Therefore the wavelet submotion index for the 4th SkeletalMotion submotion number might be 3 instead of 4 for example, as it will skip the non animated ones.
         */
        struct EMFX_API Mapping
        {
            uint16  mPosIndex;  /**< The position track index. */
            uint16  mRotIndex;  /**< The rotation track index. */

            EMFX_SCALECODE
            (
                //uint16    mScaleRotIndex; /**< The scale rotation track index. */
                uint16  mScaleIndex;    /**< The scale track index. */
            )

            Mapping()
            {
                mPosIndex   = MCORE_INVALIDINDEX16;
                mRotIndex   = MCORE_INVALIDINDEX16;

                EMFX_SCALECODE
                (
                    //mScaleRotIndex    = MCORE_INVALIDINDEX16;
                    mScaleIndex     = MCORE_INVALIDINDEX16;
                )
            }
        };

        /**
         * A collection of buffers that are used during compression and decompression.
         * Each thread will get its own buffers, to eliminate the need for multithreading locks.
         */
        struct EMFX_API BufferInfo
        {
            MCore::Quaternion*  mUncompressedRotations; /**< The buffer with uncompressed rotations. */
            AZ::PackedVector3f* mUncompressedVectors;   /**< The buffer with uncompressed Vector3's used for position and scale values. */
            float*              mUncompressedMorphs;    /**< The buffer with uncompressed morph target weights. */
            float*              mCoeffBuffer;           /**< The buffer that holds the wavelet coefficients. */
            int16*              mQuantBuffer;           /**< The buffer that holds the quantized coefficient values. */
        };

        /**
         * The available wavelet types.
         * Most likely you want to use the Haar wavelet, as this is performance wise the best compared to the others.
         * If you really want to squeeze most out of the comrpession in trade for performance you can try out the other wavelets.
         */
        enum EWaveletType
        {
            WAVELET_HAAR    = 0,    /**< The Haar wavelet, which is most likely what you want to use. It is the fastest also. */
            WAVELET_DAUB4   = 1,    /**< Daubechies 4 wavelet, can result in bit better compression ratios, but slower than Haar. */
            WAVELET_CDF97   = 2     /**< The CDF97 wavelet, used in JPG as well. This is the slowest, but often results in the best compression ratios. */
        };

        /**
         * The settings used to convert a regular keytrack based skeletal motion into a wavelet compressed skeletal motion.
         * It uses the following default values:
         * <pre>
         * mWavelet         = WAVELET_HAAR;
         * mSamplesPerChunk = 8;
         * mSamplesPerSecond= 12;
         * mPositionQuality = 100.0f;
         * mRotationQuality = 100.0f;
         * mScaleQuality    = 100.0f;   // this might not be available if scale support has been disabled inside the API
         * mMorphQuality    = 100.0f;
         * </pre>
         */
        struct EMFX_API Settings
        {
            EWaveletType        mWavelet;               /**< The wavelet type, defaults on WAVELET_HAAR. */
            uint32              mSamplesPerChunk;       /**< Number of samples per chunk, must be power of two, most likely 8 or 16, defaults on 8. Larger values take longer to decompress, but result in better compression ratios. */
            uint32              mSamplesPerSecond;      /**< The number of samples per second. The default is 12. */
            float               mRotationQuality;       /**< The rotation quality, which must be a value between 1 and 100. The default is 100, which is the maximum quality. */
            float               mPositionQuality;       /**< The position quality, which must be a value between 1 and 100. The default is 100, which is the maximum quality. */
            float               mMorphQuality;          /**< The morphing quality, which must be a value between 1 and 100. The default is 100, which is the maximum quality. */
            EMFX_SCALECODE(float   mScaleQuality; /**< The scale quality, which must be a value between 1 and 100. The default is 100, which is the maximum quality. */)

            /**
             * The constructor which initializes the following default settings:
             * <pre>
             * mWavelet         = WAVELET_HAAR;
             * mSamplesPerChunk = 8;
             * mSamplesPerSecond= 12;
             * mPositionQuality = 100.0f;
             * mRotationQuality = 100.0f;
             * mScaleQuality    = 100.0f;   // this might not be available if scale support has been disabled inside the API
             * </pre>
             */
            Settings()
            {
                mWavelet            = WAVELET_HAAR;
                mSamplesPerChunk    = 8;
                mSamplesPerSecond   = 12;
                mRotationQuality    = 100.0f;
                mPositionQuality    = 100.0f;
                mMorphQuality       = 100.0f;
                EMFX_SCALECODE(mScaleQuality   = 100.0f;
                    )
            }
        };

        /**
         * The create method.
         * Don't forget to call the Init() method before using this motion object.
         * @param name The name of the motion.
         */
        static WaveletSkeletalMotion* Create(const char* name);

        /**
         * Returns the type identification number of the motion class.
         * @result The type identification number.
         */
        uint32 GetType() const override;

        /**
         * Gets the type as a description.
         * @result The string containing the type of the motion.
         */
        const char* GetTypeString() const override;

        /**
         * Initialize this wavelet skeletal motion from a regular skeletal motion.
         * This basically creates a wavelet compressed version of the skeletal motion that you provide as parameter.
         * After calling this Init function you are ready to use all other methods of this class.
         * @param skeletalMotion The skeletal motion to create a compressed version from. The skeletal motion you provide in this parameter will not be modified.
         * @param settings The compression settings to use, or set to nullptr in case you want to use the default settings. See the WaveletSkeletalMotion::Settings struct for more information.
         */
        void Init(SkeletalMotion* skeletalMotion, Settings* settings = nullptr);

        /**
         * Decompress a given compressed chunk inside this motion.
         * @param chunk The chunk to decompress. Please keep in mind that this chunk has to be part of this motion!
         * @param targetChunk The output decompressed chunk. The decompressed data will be stored in this object.
         * @param threadIndex The thread number that this decompression is being executed in. This is used for multithreading support.
         */
        void DecompressChunk(Chunk* chunk, WaveletCache::DecompressedChunk* targetChunk, uint32 threadIndex = 0);

        /**
         * Calculates the node transformation of the given node for this motion.
         * @param instance The motion instance that contains the motion links to this motion.
         * @param outTransform The node transformation that will be the output of this function.
         * @param actor The actor to apply the motion to.
         * @param node The node to apply the motion to.
         * @param timeValue The time value.
         * @param enableRetargeting Set to true if you like to enable motion retargeting, otherwise set to false.
         */
        void CalcNodeTransform(const MotionInstance* instance, Transform* outTransform, Actor* actor, Node* node, float timeValue, bool enableRetargeting) override;

        /**
         * The main update method, which outputs the result for a given motion instance into a given output local pose.
         * @param inPose The current pose, as it is currently in the blending pipeline.
         * @param outPose The output pose, which this motion will modify and write its output into.
         * @param instance The motion instance to calculate the pose for.
         */
        void Update(const Pose* inPose, Pose* outPose, MotionInstance* instance) override;

        /**
         * Update the maximum time value of this motion.
         * This does nothing in this class.
         */
        void UpdateMaxTime() override;

        /**
         * Check if this motion supports the CalcNodeTransform method.
         * This is for example used in the motion based actor repositioning code.
         * @result Returns true when the CalcNodeTransform method is supported, otherwise false is returned.
         */
        bool GetSupportsCalcNodeTransform() const override                              { return true; }

        /**
         * Find a given chunk that stores the data for a given time value.
         * @param timeValue The time value, which must be a value in range of the time range of this motion.
         * @result Returns nullptr when no chunk found at this time value, or a pointer to the chunk in any other case.
         */
        Chunk* FindChunkAtTime(float timeValue) const;

        void SetSubMotionMapping(uint32 index, const Mapping& mapping);
        void ResizeMappingArray(uint32 numItems);
        void SetMorphSubMotionMapping(uint32 index, uint16 mapping);
        void ResizeMorphMappingArray(uint32 numItems);

        MCORE_INLINE const Mapping& GetSubMotionMapping(uint32 subMotion) const     { return mSubMotionMap[subMotion]; }
        MCORE_INLINE uint16 GetMorphSubMotionMapping(uint32 morphSubMotion) const   { return mMorphSubMotionMap[morphSubMotion]; }

        MCORE_INLINE EWaveletType GetWavelet() const                    { return mWavelet; }
        MCORE_INLINE float GetSampleSpacing() const                     { return mSampleSpacing; }
        MCORE_INLINE float GetSecondsPerChunk() const                   { return mSecondsPerChunk; }
        MCORE_INLINE float GetPosQuantFactor() const                    { return mPosQuantizeFactor; }
        MCORE_INLINE float GetRotQuantFactor() const                    { return mRotQuantizeFactor; }
        MCORE_INLINE float GetMorphQuantFactor() const                  { return mMorphQuantizeFactor; }
        MCORE_INLINE uint32 GetCompressedSize() const                   { return mCompressedSize; }
        MCORE_INLINE uint32 GetOptimizedSize() const                    { return mOptimizedSize; }
        MCORE_INLINE uint32 GetUncompressedSize() const                 { return mUncompressedSize; }
        MCORE_INLINE uint32 GetNumPosTracks() const                     { return mNumPosTracks; }
        MCORE_INLINE uint32 GetNumRotTracks() const                     { return mNumRotTracks; }
        MCORE_INLINE uint32 GetNumMorphTracks() const                   { return mNumMorphTracks; }
        MCORE_INLINE uint32 GetNumDecompressedRotBytes() const          { return mDecompressedRotNumBytes; }
        MCORE_INLINE uint32 GetNumDecompressedPosBytes() const          { return mDecompressedPosNumBytes; }
        MCORE_INLINE uint32 GetNumDecompressedMorphBytes() const        { return mDecompressedMorphNumBytes; }
        MCORE_INLINE uint32 GetChunkOverhead() const                    { return mChunkOverhead; }
        MCORE_INLINE uint32 GetSamplesPerChunk() const                  { return mSamplesPerChunk; }
        MCORE_INLINE uint32 GetNumChunks() const                        { return mChunks.GetLength(); }
        MCORE_INLINE Chunk* GetChunk(uint32 index)                      { return mChunks[index]; }

        void SetWavelet(EWaveletType waveletType);
        void SetSampleSpacing(float spacing);
        void SetSecondsPerChunk(float secs);
        void SetCompressedSize(uint32 numBytes);
        void SetOptimizedSize(uint32 numBytes);
        void SetUncompressedSize(uint32 numBytes);
        void SetNumPosTracks(uint32 numTracks);
        void SetNumRotTracks(uint32 numTracks);
        void SetNumMorphTracks(uint32 numTracks);
        void SetChunkOverhead(uint32 numBytes);
        void SetSamplesPerChunk(uint32 numSamples);
        void SetNumChunks(uint32 numChunks);
        void SetChunk(uint32 index, Chunk* chunk);
        void SetDecompressedRotNumBytes(uint32 numBytes);
        void SetDecompressedPosNumBytes(uint32 numBytes);
        void SetDecompressedMorphNumBytes(uint32 numBytes);
        void SetPosQuantFactor(float factor);
        void SetRotQuantFactor(float factor);
        void SetMorphQuantFactor(float factor);

        // scale related methods
        EMFX_SCALECODE
        (
            void SetNumScaleTracks(uint32 numTracks);
            void SetDecompressedScaleNumBytes(uint32 numBytes);
            void SetScaleQuantFactor(float factor);

            MCORE_INLINE uint32 GetNumScaleTracks() const               { return mNumScaleTracks;
            }
            MCORE_INLINE uint32 GetNumDecompressedScaleBytes() const    { return mDecompressedScaleNumBytes;
            }
            MCORE_INLINE float  GetScaleQuantFactor() const             { return mScaleQuantizeFactor;
            }
        )

        void SwapChunkDataEndian();

        /**
         * Scale all motion data.
         * This is a fast operation for wavelet motions because it does not actually adjust the compressed data.
         * @param scaleFactor The scale factor to scale the current data by.
         */
        void Scale(float scaleFactor) override;
        float GetScale() const;

    private:
        MCore::Array<Chunk*>        mChunks;                        /**< The collection of compressed chunks. */
        MCore::SmallArray<Mapping>  mSubMotionMap;                  /**< The sub-motion map, used to filter out keytrack data that has no animation data. */
        MCore::SmallArray<uint16>   mMorphSubMotionMap;
        EWaveletType                mWavelet;                       /**< The wavelet type used to compress this motion. */
        uint32                      mDecompressedRotNumBytes;       /**< The size in bytes of rotation data, of a single decompressed chunk. */
        uint32                      mDecompressedPosNumBytes;       /**< The size in bytes of position data, of a single decompressed chunk. */
        EMFX_SCALECODE(uint32      mDecompressedScaleNumBytes; /**< The size in bytes of scale data, of a single decompresed chunk. */)
        uint32                      mDecompressedMorphNumBytes;
        uint32                      mNumRotTracks;                  /**< The number of animated rotation keytracks. */
        EMFX_SCALECODE(uint32      mNumScaleRotTracks; /**< The number of animated scale rotation keytracks. */)
        uint32                      mNumPosTracks;                  /**< The number of animated position keytracks. */
        EMFX_SCALECODE(uint32      mNumScaleTracks; /**< The number of animated scale keytracks. */)
        uint32                      mNumMorphTracks;
        float                       mSecondsPerChunk;               /**< The number of seconds of motion data each chunk represents. */
        float                       mSampleSpacing;                 /**< The number of seconds between each sample inside a chunk. */
        float                       mPosQuantizeFactor;             /**< The position quantization factor. This controls the quality of the rotations of the motion. */
        float                       mRotQuantizeFactor;             /**< The rotation quantization factor. This controls the quality of the positions in the motion. */
        EMFX_SCALECODE(float       mScaleQuantizeFactor; /**< The scale quantization factor. This controls the quality of the scale values in the motion. */)
        float                       mMorphQuantizeFactor;
        uint32                      mSamplesPerChunk;               /**< The number of samples per chunk. */
        uint32                      mChunkOverhead;                 /**< The overhead in bytes of the chunk data. */
        uint32                      mCompressedSize;                /**< The compressed size of the motion, in bytes. */
        uint32                      mOptimizedSize;                 /**< The input motion size of the motion that was being compresssed. */
        uint32                      mUncompressedSize;              /**< The size of a 30 fps sampled version of the input motion. */
        float                       mScale;                         /**< The scale of the position data (this is applied at decompression time. */

        /**
         * The constructor.
         * Don't forget to call the Init() method before using this motion object.
         * @param name The name of the motion.
         */
        WaveletSkeletalMotion(const char* name);

        /**
         * The destructor.
         * This automatically calls Release().
         */
        ~WaveletSkeletalMotion();

        /**
         * Release the memory allocated by this motion.
         * This is automatically called by the destructor.
         */
        void ReleaseData();

        /**
         * Initalize a given chunk.
         * This performs the actual compression of a given motion chunk/section.
         * @param chunk The output chunk to store the compressed data in.
         * @param skelMotion The uncompressed skeletal motion to compress the data from.
         * @param startTime The start time of the chunk, in seconds.
         * @param duration The duration in seconds, of the chunk.
         * @param buffers The compression buffer information object.
         */
        void InitChunk(Chunk* chunk, SkeletalMotion* skelMotion, float startTime, float duration, BufferInfo& buffers);
    };
}   // namespace EMotionFX
