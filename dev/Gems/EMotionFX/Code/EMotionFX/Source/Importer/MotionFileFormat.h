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

// include the shared structs
#include "SharedFileFormatStructs.h"

#include <AzCore/base.h>

namespace EMotionFX
{
    namespace FileFormat // so now we are in the namespace EMotionFX::FileFormat
    {
        // the chunks
        enum
        {
            MOTION_CHUNK_SUBMOTION             = 200,
            MOTION_CHUNK_INFO                  = 201,
            MOTION_CHUNK_MOTIONEVENTTABLE      = SHARED_CHUNK_MOTIONEVENTTABLE,
            MOTION_CHUNK_SUBMOTIONS            = 202,
            MOTION_CHUNK_WAVELETINFO           = 203,// see Motion_WaveletInfo
            MOTION_CHUNK_MORPHSUBMOTIONS       = 204,
            MOTION_FORCE_32BIT                 = 0xFFFFFFFF
        };

        // the wavelet type used during compression
        enum
        {
            MOTION_WAVELET_HAAR    = 0,
            MOTION_WAVELET_D4      = 1,
            MOTION_WAVELET_CDF97   = 2
        };

        // the compressor used to compress the quantized data
        enum
        {
            MOTION_COMPRESSOR_HUFFMAN  = 0
        };

        // motion file header
        // (not aligned)
        struct Motion_Header
        {
            uint8 mFourcc[4];   // must be "MOT " or "MOTW"
            uint8 mHiVersion;   // high version (2  in case of v2.34)
            uint8 mLoVersion;   // low version  (34 in case of v2.34)
            uint8 mEndianType;  // the endian in which the data is saved [0=little, 1=big]
        };

        
        // information chunk
        // (not aligned)
        struct Motion_Info
        {
            uint32  mMotionExtractionMask;      // motion extraction mask
            uint32  mMotionExtractionNodeIndex; // motion extraction node index
            uint8   mUnitType;                  // maps to EMotionFX::EUnitType
        };

        // information chunk
        // (not aligned)
        struct Motion_Info2
        {
            uint32  mMotionExtractionFlags;     // motion extraction flags
            uint32  mMotionExtractionNodeIndex; // motion extraction node index
            uint8   mUnitType;                  // maps to EMotionFX::EUnitType
        };

        // information chunk
        // (not aligned)
        struct Motion_Info3
        {
            uint32  mMotionExtractionFlags;     // motion extraction flags
            uint32  mMotionExtractionNodeIndex; // motion extraction node index
            uint8   mUnitType;                  // maps to EMotionFX::EUnitType
            uint8   mIsAdditive;                // if the motion is an additive motion [0=false, 1=true]
        };

        // skeletal submotion
        // (aligned)       
        struct Motion_SkeletalSubMotion
        {
            File16BitQuaternion mPoseRot;       // initial pose rotation
            File16BitQuaternion mBindPoseRot;   // bind pose rotation
            FileVector3         mPosePos;       // initial pose position
            FileVector3         mPoseScale;     // initial pose scale
            FileVector3         mBindPosePos;   // bind pose position
            FileVector3         mBindPoseScale; // bind pose scale
            uint32              mNumPosKeys;    // number of position keyframes to follow
            uint32              mNumRotKeys;    // number of rotation keyframes to follow
            uint32              mNumScaleKeys;  // number of scale keyframes to follow

            // followed by:
            // string : motion part name
            // Motion_Vector3Key[ mNumPosKeys ]
            // Motion_16BitQuaternionKey[ mNumRotKeys ]
            // Motion_Vector3Key[ mNumScaleKeys ]
        };


        // a 3D vector key
        // (aligned)
        struct Motion_Vector3Key
        {
            FileVector3     mValue;     // the value
            float           mTime;      // the time in seconds
        };


        // a quaternion key
        // (aligned)
        struct Motion_QuaternionKey
        {
            FileQuaternion  mValue;     // the value
            float           mTime;      // the time in seconds
        };


        // a 16-bit compressed quaternion key
        // (aligned)
        struct Motion_16BitQuaternionKey
        {
            File16BitQuaternion mValue; // the value
            float               mTime;  // the time in seconds
        };


        // regular submotion header
        // (aligned)
        struct Motion_SubMotions
        {
            uint32  mNumSubMotions;// the number of skeletal motions

            // followed by:
            // Motion_SkeletalSubMotion[ mNumSubMotions ]
        };


        // wavelet submotion mapping entry
        // (not aligned)
        struct Motion_WaveletMapping
        {
            uint16  mPosIndex;
            uint16  mRotIndex;
            uint16  mScaleIndex;
        };


        // wavelet skeletal submotions header
        // (not aligned)
        struct Motion_WaveletInfo
        {
            uint32  mNumChunks;
            uint32  mSamplesPerChunk;
            uint32  mDecompressedRotNumBytes;
            uint32  mDecompressedPosNumBytes;
            uint32  mDecompressedScaleNumBytes;
            uint32  mDecompressedMorphNumBytes;
            uint32  mNumRotTracks;
            uint32  mNumScaleTracks;
            uint32  mNumPosTracks;
            uint32  mNumMorphTracks;
            uint32  mChunkOverhead;
            uint32  mCompressedSize;
            uint32  mOptimizedSize;
            uint32  mUncompressedSize;
            uint32  mNumSubMotions;
            uint32  mNumMorphSubMotions;
            float   mPosQuantFactor;
            float   mRotQuantFactor;
            float   mScaleQuantFactor;
            float   mMorphQuantFactor;
            float   mSampleSpacing;
            float   mSecondsPerChunk;
            float   mMaxTime;
            float   mScale;
            uint8   mWaveletID;         // see the enum with MOTION_WAVELET_HAAR etc
            uint8   mCompressorID;      // see the enum with MOTION_COMPRESSOR_HUFFMAN inside
            uint8   mUnitType;          // maps to EMotionFX::EUnitType

            // followed by:
            //      Motion_WaveletMapping[mNumSubMotions]
            //      Motion_WaveletSubMotion[mNumSubMotions]
            //      Motion_WaveletMorphSubMotion[mNumMorphSubMotions]
            //      uint16[mNumMorphSubMotions]     // morph map
            //      Motion_WaveletChunk[mNumChunks]
        };


        // skeletal submotion
        // (aligned)
        struct Motion_WaveletSkeletalSubMotion
        {
            File16BitQuaternion mPoseRot;       // initial pose rotation
            File16BitQuaternion mBindPoseRot;   // bind pose rotation
            FileVector3         mPosePos;       // initial pose position
            FileVector3         mPoseScale;     // initial pose scale
            FileVector3         mBindPosePos;   // bind pose position
            FileVector3         mBindPoseScale; // bind pose scale

            // followed by:
            // string : motion part name
        };


        // skeletal submotion
        // (aligned)
        struct Motion_WaveletMorphSubMotion
        {
            float   mPoseWeight;

            // followed by:
            // string : name
        };


        // a wavelet compressed chunk
        // (aligned)
        struct Motion_WaveletChunk
        {
            float       mRotQuantScale;
            float       mPosQuantScale;
            float       mScaleQuantScale;
            float       mMorphQuantScale;
            float       mStartTime;
            uint32      mCompressedRotNumBytes;
            uint32      mCompressedPosNumBytes;
            uint32      mCompressedScaleNumBytes;
            uint32      mCompressedMorphNumBytes;
            uint32      mCompressedPosNumBits;
            uint32      mCompressedRotNumBits;
            uint32      mCompressedScaleNumBits;
            uint32      mCompressedMorphNumBits;

            // followed by:
            //      uint8   mCompressedRotData[mCompressedRotNumBytes]
            //      uint8   mCompressedPosData[mCompressedPosNumBytes]
            //      uint8   mCompressedMorphData[mCompressedMorphNumBytes]
            //      uint8   mCompressedScaleData[mCompressedScaleNumBytes]
        };


        // morph sub motion
        // (aligned)
        struct Motion_MorphSubMotion
        {
            float   mPoseWeight;// pose weight to use in case no animation data is present
            float   mMinWeight; // minimum allowed weight value (used for unpacking the keyframe weights)
            float   mMaxWeight; // maximum allowed weight value (used for unpacking the keyframe weights)
            uint32  mPhonemeSet;// the phoneme set of the submotion, 0 if this is a normal morph target submotion
            uint32  mNumKeys;   // number of keyframes to follow

            // followed by:
            // string : name (the name of this motion part)
            // Motion_UnsignedShortKey[mNumKeys]
        };


        // a uint16 key
        // (not aligned)
        struct Motion_UnsignedShortKey
        {
            float   mTime;  // the time in seconds
            uint16  mValue; // the value
        };


        // (aligned)
        struct Motion_MorphSubMotions
        {
            uint32  mNumSubMotions;
            // followed by:
            // Motion_MorphSubMotion[ mNumSubMotions ]
        };


        // a motion event version 4
        // (not aligned)
        struct FileMotionEvent
        {
            float   mStartTime;
            float   mEndTime;
            uint32  mEventTypeIndex;// index into the event type string table
            uint32  mMirrorTypeIndex;// index into the event type string table
            uint16  mParamIndex;    // index into the parameter string table
        };


        // motion event track
        // (not aligned)
        struct FileMotionEventTrack
        {
            uint32  mNumEvents;
            uint32  mNumTypeStrings;
            uint32  mNumParamStrings;
            uint32  mNumMirrorTypeStrings;
            uint8   mIsEnabled;

            // followed by:
            // String track name
            // [mNumTypeStrings] string objects
            // [mNumParamStrings] string objects
            // [mNumMirrorTypeStrings] string objects
            // FileMotionEvent[mNumEvents]
        };


        // a motion event table
        // (aligned)
        struct FileMotionEventTable
        {
            uint32  mNumTracks;

            // followed by:
            // FileMotionEventTrack[mNumTracks]
        };

        struct FileMotionEventTableSerialized
        {
            /// Use a fixed size to avoid platform-specific issues with size_t
            AZ::u64 m_size;
        };
    } // namespace FileFormat
}   // namespace EMotionFX
