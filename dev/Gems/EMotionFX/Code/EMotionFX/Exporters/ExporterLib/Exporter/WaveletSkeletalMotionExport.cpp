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

#include "Exporter.h"
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/SkeletalSubMotion.h>
#include <EMotionFX/Source/MorphSubMotion.h>
#include <MCore/Source/HaarWavelet.h>
#include <MCore/Source/D4Wavelet.h>
#include <MCore/Source/CDF97Wavelet.h>
#include <MCore/Source/DiskFile.h>
#include <EMotionFX/Source/WaveletSkeletalMotion.h>
#include <EMotionFX/Source/Importer/Importer.h>

// file format structures
#include <EMotionFX/Source/Importer/SharedFileFormatStructs.h>
#include <EMotionFX/Source/Importer/MotionFileFormat.h>


namespace ExporterLib
{
    // save the mapping for the given submotion
    void SaveWaveletMapping(MCore::Stream* file, EMotionFX::WaveletSkeletalMotion* motion, uint32 subMotionIndex, MCore::Endian::EEndianType targetEndianType)
    {
        const EMotionFX::WaveletSkeletalMotion::Mapping& mapping = motion->GetSubMotionMapping(subMotionIndex);

        // the mapping chunk
        EMotionFX::FileFormat::Motion_WaveletMapping mappingChunk;

        mappingChunk.mPosIndex      = mapping.mPosIndex;
        mappingChunk.mRotIndex      = mapping.mRotIndex;
        //mappingChunk.mScaleRotIndex   = MCORE_INVALIDINDEX16;//mapping.mScaleRotIndex;
        mappingChunk.mScaleIndex    = mapping.mScaleIndex;

        // log it
        MCore::LogDetailedInfo("    + SubMotion #%d = (posIndex:%d, rotIndex:%d, scaleIndex:%d", subMotionIndex, mappingChunk.mPosIndex, mappingChunk.mRotIndex, mappingChunk.mScaleIndex);

        // convert endian
        ConvertUnsignedShort(&mappingChunk.mPosIndex, targetEndianType);
        ConvertUnsignedShort(&mappingChunk.mRotIndex, targetEndianType);
        //ConvertUnsignedShort( &mappingChunk.mScaleRotIndex, targetEndianType );
        ConvertUnsignedShort(&mappingChunk.mScaleIndex, targetEndianType);

        // save the mapping chunk
        file->Write(&mappingChunk, sizeof(EMotionFX::FileFormat::Motion_WaveletMapping));
    }


    // save all wavelet mappings
    void SaveWaveletMappings(MCore::Stream* file, EMotionFX::WaveletSkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::LogDetailedInfo("- Wavelet Submotion Map:");

        // get the number of submotions, iterate through them and save the mappings
        const uint32 numSubMotions = motion->GetNumSubMotions();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            SaveWaveletMapping(file, motion, i, targetEndianType);
        }
    }


    // save the given wavelet submotion
    void SaveWaveletSkeletalSubMotion(MCore::Stream* file, EMotionFX::SkeletalSubMotion* subMotion, MCore::Endian::EEndianType targetEndianType, float scale)
    {
        const float invScale = 1.0f / scale;

        // get the animation start pose and the bind pose transformation information
        AZ::PackedVector3f                  posePosition    = AZ::PackedVector3f(subMotion->GetPosePos() * invScale);
        MCore::Compressed16BitQuaternion    poseRotation    = subMotion->GetCompressedPoseRot();
        AZ::PackedVector3f                  poseScale       = AZ::PackedVector3f(subMotion->GetPoseScale());
        AZ::PackedVector3f                  bindPosePosition = AZ::PackedVector3f(subMotion->GetBindPosePos() * invScale);
        MCore::Compressed16BitQuaternion    bindPoseRotation = subMotion->GetCompressedBindPoseRot();
        AZ::PackedVector3f                  bindPoseScale   = AZ::PackedVector3f(subMotion->GetBindPoseScale());

        EMotionFX::FileFormat::Motion_WaveletSkeletalSubMotion subMotionChunk;

        // copy start animation pose transformation to the chunk(time=frameStart)
        CopyVector(subMotionChunk.mPosePos, posePosition);
        Copy16BitQuaternion(subMotionChunk.mPoseRot, poseRotation);
        CopyVector(subMotionChunk.mPoseScale, poseScale);

        // copy the bind pose transformation to the chunk
        CopyVector(subMotionChunk.mBindPosePos, bindPosePosition);
        Copy16BitQuaternion(subMotionChunk.mBindPoseRot, bindPoseRotation);
        CopyVector(subMotionChunk.mBindPoseScale, bindPoseScale);

        // create an uncompressed version of the fixed and compessed rotation quaternions
        MCore::Quaternion uncompressedPoseRot       = MCore::Compressed16BitQuaternion(subMotionChunk.mPoseRot.mX, subMotionChunk.mPoseRot.mY, subMotionChunk.mPoseRot.mZ, subMotionChunk.mPoseRot.mW).ToQuaternion();
        MCore::Quaternion uncompressedBindPoseRot   = MCore::Compressed16BitQuaternion(subMotionChunk.mBindPoseRot.mX, subMotionChunk.mBindPoseRot.mY, subMotionChunk.mBindPoseRot.mZ, subMotionChunk.mBindPoseRot.mW).ToQuaternion();

        //subMotionChunk.mMaxError = subMotion->GetMotionLODMaxError() * motionImportanceFactor;

        MCore::LogDetailedInfo("- Wavelet Skeletal SubMotion = '%s'", subMotion->GetName());
        MCore::LogDetailedInfo("    + Pose Position:         x=%f, y=%f, z=%f", subMotionChunk.mPosePos.mX, subMotionChunk.mPosePos.mY, subMotionChunk.mPosePos.mZ);
        MCore::LogDetailedInfo("    + Pose Rotation:         x=%f, y=%f, z=%f, w=%f", uncompressedPoseRot.x, uncompressedPoseRot.y, uncompressedPoseRot.z, uncompressedPoseRot.w);
        MCore::LogDetailedInfo("    + Pose Scale:            x=%f, y=%f, z=%f", subMotionChunk.mPoseScale.mX, subMotionChunk.mPoseScale.mY, subMotionChunk.mPoseScale.mZ);
        MCore::LogDetailedInfo("    + Bind Pose Position:    x=%f, y=%f, z=%f", subMotionChunk.mBindPosePos.mX, subMotionChunk.mBindPosePos.mY, subMotionChunk.mBindPosePos.mZ);
        MCore::LogDetailedInfo("    + Bind Pose Rotation:    x=%f, y=%f, z=%f, w=%f", uncompressedBindPoseRot.x, uncompressedBindPoseRot.y, uncompressedBindPoseRot.z, uncompressedBindPoseRot.w);
        MCore::LogDetailedInfo("    + Bind Pose Scale:       x=%f, y=%f, z=%f", subMotionChunk.mBindPoseScale.mX, subMotionChunk.mBindPoseScale.mY, subMotionChunk.mBindPoseScale.mZ);

        // convert endian
        ConvertFileVector3(&subMotionChunk.mPosePos, targetEndianType);
        ConvertFile16BitQuaternion(&subMotionChunk.mPoseRot, targetEndianType);
        ConvertFileVector3(&subMotionChunk.mPoseScale, targetEndianType);

        ConvertFileVector3(&subMotionChunk.mBindPosePos, targetEndianType);
        ConvertFile16BitQuaternion(&subMotionChunk.mBindPoseRot, targetEndianType);
        ConvertFileVector3(&subMotionChunk.mBindPoseScale, targetEndianType);

        // save the submotion chunk
        file->Write(&subMotionChunk, sizeof(EMotionFX::FileFormat::Motion_WaveletSkeletalSubMotion));

        //followed by: the submotion name
        SaveString(subMotion->GetName(), file, targetEndianType);
    }


    // save all wavelet submotions
    void SaveWaveletSkeletalSubMotions(MCore::Stream* file, EMotionFX::WaveletSkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of submotions, iterate through all submotions and write them to the stream
        const uint32 numSubMotions = motion->GetNumSubMotions();
        for (uint32 i = 0; i < numSubMotions; i++)
        {
            SaveWaveletSkeletalSubMotion(file, motion->GetSubMotion(i), targetEndianType, motion->GetScale());
        }
    }


    // calculate the size in bytes inside the file of the wavelet submotions
    uint32 GetWaveletSubMotionsSizeInBytes(EMotionFX::WaveletSkeletalMotion* motion)
    {
        uint32 sizeInBytes = motion->GetNumSubMotions() * sizeof(EMotionFX::FileFormat::Motion_WaveletSkeletalSubMotion);

        // get the number of submotions, iterate through them and save the mappings
        const uint32 numSubMotions = motion->GetNumSubMotions();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            sizeInBytes += GetStringChunkSize(motion->GetSubMotion(i)->GetName());
        }

        return sizeInBytes;
    }


    // save all wavelet morph submotions
    void SaveWaveletMorphSubMotions(MCore::Stream* file, EMotionFX::WaveletSkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of submotions, iterate through all submotions and write them to the stream
        const uint32 numMorphSubMotions = motion->GetNumMorphSubMotions();
        for (uint32 i = 0; i < numMorphSubMotions; i++)
        {
            EMotionFX::MorphSubMotion* subMotion = motion->GetMorphSubMotion(i);
            const AZStd::string& nameString = MCore::GetStringIdPool().GetName(subMotion->GetID());

            EMotionFX::FileFormat::Motion_WaveletMorphSubMotion fileSubMorphMotion;
            fileSubMorphMotion.mPoseWeight = subMotion->GetPoseWeight();

            MCore::LogDetailedInfo("- Wavelet Morph SubMotion:");
            MCore::LogDetailedInfo("    + Name:         %s", nameString.c_str());
            MCore::LogDetailedInfo("    + Pose Weight:  %f", fileSubMorphMotion.mPoseWeight);

            // convert endian
            ConvertFloat(&fileSubMorphMotion.mPoseWeight, targetEndianType);

            file->Write(&fileSubMorphMotion, sizeof(EMotionFX::FileFormat::Motion_WaveletMorphSubMotion));

            // save hte name
            SaveString(nameString, file, targetEndianType);
        }
    }


    // calculate the size in bytes inside the file of the wavelet morph submotions
    uint32 GetWaveletMorphSubMotionsSizeInBytes(EMotionFX::WaveletSkeletalMotion* motion)
    {
        uint32 sizeInBytes = motion->GetNumMorphSubMotions() * sizeof(EMotionFX::FileFormat::Motion_WaveletMorphSubMotion);

        const uint32 numSubMotions = motion->GetNumMorphSubMotions();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            const AZStd::string& name = MCore::GetStringIdPool().GetName(motion->GetMorphSubMotion(i)->GetID());
            sizeInBytes += GetStringChunkSize(name);
        }

        return sizeInBytes;
    }


    // save the given wavelet chunk
    void SaveWaveletChunk(MCore::Stream* file, EMotionFX::WaveletSkeletalMotion::Chunk* chunk, MCore::Endian::EEndianType targetEndianType)
    {
        // the wavelet chunk
        EMotionFX::FileFormat::Motion_WaveletChunk waveletChunk;

        waveletChunk.mRotQuantScale             = chunk->mRotQuantScale;
        waveletChunk.mPosQuantScale             = chunk->mPosQuantScale;
        waveletChunk.mScaleQuantScale           = chunk->mScaleQuantScale;
        waveletChunk.mMorphQuantScale           = chunk->mMorphQuantScale;
        waveletChunk.mStartTime                 = chunk->mStartTime;
        waveletChunk.mCompressedRotNumBytes     = chunk->mCompressedRotNumBytes;
        waveletChunk.mCompressedPosNumBytes     = chunk->mCompressedPosNumBytes;
        waveletChunk.mCompressedScaleNumBytes   = chunk->mCompressedScaleNumBytes;
        waveletChunk.mCompressedMorphNumBytes   = chunk->mCompressedMorphNumBytes;
        waveletChunk.mCompressedPosNumBits      = chunk->mCompressedPosNumBits;
        waveletChunk.mCompressedRotNumBits      = chunk->mCompressedRotNumBits;
        waveletChunk.mCompressedScaleNumBits    = chunk->mCompressedScaleNumBits;
        waveletChunk.mCompressedMorphNumBits    = chunk->mCompressedMorphNumBits;

        // log it
        MCore::LogDetailedInfo("- Wavelet Chunk");
        MCore::LogDetailedInfo("    + RotQuantScale:           %f", waveletChunk.mRotQuantScale);
        MCore::LogDetailedInfo("    + PosQuantScale:           %f", waveletChunk.mPosQuantScale);
        MCore::LogDetailedInfo("    + ScaleQuantScale:         %f", waveletChunk.mScaleQuantScale);
        MCore::LogDetailedInfo("    + MorphQuantScale:         %f", waveletChunk.mMorphQuantScale);
        MCore::LogDetailedInfo("    + StartTime:               %f", waveletChunk.mStartTime);
        MCore::LogDetailedInfo("    + CompressedRotNumBytes:   %i", waveletChunk.mCompressedRotNumBytes);
        MCore::LogDetailedInfo("    + CompressedPosNumBytes:   %i", waveletChunk.mCompressedPosNumBytes);
        MCore::LogDetailedInfo("    + CompressedScaleNumBytes: %i", waveletChunk.mCompressedScaleNumBytes);
        MCore::LogDetailedInfo("    + CompressedMorphNumBytes: %i", waveletChunk.mCompressedMorphNumBytes);
        MCore::LogDetailedInfo("    + CompressedPosNumBits:    %i", waveletChunk.mCompressedPosNumBits);
        MCore::LogDetailedInfo("    + CompressedRotNumBits:    %i", waveletChunk.mCompressedRotNumBits);
        MCore::LogDetailedInfo("    + CompressedScaleNumBits:  %i", waveletChunk.mCompressedScaleNumBits);
        MCore::LogDetailedInfo("    + CompressedMorphNumBits:  %i", waveletChunk.mCompressedMorphNumBits);

        // convert endian
        ConvertFloat(&waveletChunk.mRotQuantScale, targetEndianType);
        ConvertFloat(&waveletChunk.mPosQuantScale, targetEndianType);
        ConvertFloat(&waveletChunk.mScaleQuantScale, targetEndianType);
        ConvertFloat(&waveletChunk.mMorphQuantScale, targetEndianType);
        ConvertFloat(&waveletChunk.mStartTime, targetEndianType);
        ConvertUnsignedInt(&waveletChunk.mCompressedRotNumBytes, targetEndianType);
        ConvertUnsignedInt(&waveletChunk.mCompressedPosNumBytes, targetEndianType);
        ConvertUnsignedInt(&waveletChunk.mCompressedScaleNumBytes, targetEndianType);
        ConvertUnsignedInt(&waveletChunk.mCompressedMorphNumBytes, targetEndianType);
        ConvertUnsignedInt(&waveletChunk.mCompressedPosNumBits, targetEndianType);
        ConvertUnsignedInt(&waveletChunk.mCompressedRotNumBits, targetEndianType);
        ConvertUnsignedInt(&waveletChunk.mCompressedScaleNumBits, targetEndianType);
        ConvertUnsignedInt(&waveletChunk.mCompressedMorphNumBits, targetEndianType);

        // save the submotion chunk
        file->Write(&waveletChunk, sizeof(EMotionFX::FileFormat::Motion_WaveletChunk));

        // followed by:
        file->Write(chunk->mCompressedRotData, chunk->mCompressedRotNumBytes);
        file->Write(chunk->mCompressedPosData, chunk->mCompressedPosNumBytes);
        file->Write(chunk->mCompressedMorphData, chunk->mCompressedMorphNumBytes);
        file->Write(chunk->mCompressedScaleData, chunk->mCompressedScaleNumBytes);
    }


    // save all wavelet chunks
    void SaveWaveletChunks(MCore::Stream* file, EMotionFX::WaveletSkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of chunks, iterate through and save them
        const uint32 numWaveletChunks = motion->GetNumChunks();
        for (uint32 i = 0; i < numWaveletChunks; ++i)
        {
            SaveWaveletChunk(file, motion->GetChunk(i), targetEndianType);
        }
    }


    // save the morph mappings
    void SaveWaveletMorphMappings(MCore::Stream* file, EMotionFX::WaveletSkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        const uint32 numMorphMappings = motion->GetNumMorphSubMotions();
        for (uint32 i = 0; i < numMorphMappings; ++i)
        {
            uint16 value = motion->GetMorphSubMotionMapping(i);
            ConvertUnsignedShort(&value, targetEndianType);
            file->Write(&value, sizeof(uint16));
        }
    }


    // calculate the size in bytes inside the file of the wavelet chunks
    uint32 GetWaveletChunksSizeInBytes(EMotionFX::WaveletSkeletalMotion* motion)
    {
        uint32 sizeInBytes = 0;

        // get the number of chunks, iterate through and save them
        const uint32 numWaveletChunks = motion->GetNumChunks();
        for (uint32 i = 0; i < numWaveletChunks; ++i)
        {
            EMotionFX::WaveletSkeletalMotion::Chunk* chunk = motion->GetChunk(i);

            sizeInBytes += sizeof(EMotionFX::FileFormat::Motion_WaveletChunk);
            sizeInBytes += chunk->mCompressedRotNumBytes;
            sizeInBytes += chunk->mCompressedPosNumBytes;
            sizeInBytes += chunk->mCompressedScaleNumBytes;
            sizeInBytes += chunk->mCompressedMorphNumBytes;
        }

        return sizeInBytes;
    }


    // save the header chunk of the anim graph file
    void SaveWaveletSkeletalMotionInfo(MCore::Stream* file, EMotionFX::WaveletSkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        // the wavelet skeletal info chunk
        EMotionFX::FileFormat::Motion_WaveletInfo waveletInfoChunk;

        waveletInfoChunk.mNumChunks                     = motion->GetNumChunks();
        waveletInfoChunk.mSamplesPerChunk               = motion->GetSamplesPerChunk();
        waveletInfoChunk.mDecompressedRotNumBytes       = motion->GetNumDecompressedRotBytes();
        waveletInfoChunk.mDecompressedPosNumBytes       = motion->GetNumDecompressedPosBytes();
        waveletInfoChunk.mDecompressedScaleNumBytes     = motion->GetNumDecompressedScaleBytes();
        waveletInfoChunk.mDecompressedMorphNumBytes     = motion->GetNumDecompressedMorphBytes();
        waveletInfoChunk.mPosQuantFactor                = motion->GetPosQuantFactor();
        waveletInfoChunk.mRotQuantFactor                = motion->GetRotQuantFactor();
        waveletInfoChunk.mScaleQuantFactor              = motion->GetScaleQuantFactor();
        waveletInfoChunk.mMorphQuantFactor              = motion->GetMorphQuantFactor();
        waveletInfoChunk.mNumRotTracks                  = motion->GetNumRotTracks();
        waveletInfoChunk.mNumScaleTracks                = motion->GetNumScaleTracks();
        waveletInfoChunk.mNumPosTracks                  = motion->GetNumPosTracks();
        waveletInfoChunk.mNumMorphTracks                = motion->GetNumMorphTracks();
        waveletInfoChunk.mChunkOverhead                 = motion->GetChunkOverhead();
        waveletInfoChunk.mCompressedSize                = motion->GetCompressedSize();
        waveletInfoChunk.mOptimizedSize                 = motion->GetOptimizedSize();
        waveletInfoChunk.mUncompressedSize              = motion->GetUncompressedSize();
        waveletInfoChunk.mNumSubMotions                 = motion->GetNumSubMotions();
        waveletInfoChunk.mNumMorphSubMotions            = motion->GetNumMorphSubMotions();
        waveletInfoChunk.mSampleSpacing                 = motion->GetSampleSpacing();
        waveletInfoChunk.mMaxTime                       = motion->GetMaxTime();
        waveletInfoChunk.mSecondsPerChunk               = motion->GetSecondsPerChunk();
        waveletInfoChunk.mCompressorID                  = EMotionFX::FileFormat::MOTION_COMPRESSOR_HUFFMAN;
        waveletInfoChunk.mUnitType                      = static_cast<uint8>(motion->GetUnitType());
        waveletInfoChunk.mScale                         = motion->GetScale();

        // get the wavelet type
        EMotionFX::WaveletSkeletalMotion::EWaveletType wavelet = motion->GetWavelet();
        switch (wavelet)
        {
        case EMotionFX::WaveletSkeletalMotion::WAVELET_HAAR:
        {
            waveletInfoChunk.mWaveletID = EMotionFX::FileFormat::MOTION_WAVELET_HAAR;
            break;
        }
        case EMotionFX::WaveletSkeletalMotion::WAVELET_DAUB4:
        {
            waveletInfoChunk.mWaveletID = EMotionFX::FileFormat::MOTION_WAVELET_D4;
            break;
        }
        case EMotionFX::WaveletSkeletalMotion::WAVELET_CDF97:
        {
            waveletInfoChunk.mWaveletID = EMotionFX::FileFormat::MOTION_WAVELET_CDF97;
            break;
        }
        default:
        {
            waveletInfoChunk.mWaveletID = EMotionFX::FileFormat::MOTION_WAVELET_HAAR;
            break;
        }
        }

        // log it
        MCore::LogDetailedInfo(" - Wavelet Skeletal Motion Info:");
        MCore::LogDetailedInfo("  + Number of chunks     = %d", waveletInfoChunk.mNumChunks);
        MCore::LogDetailedInfo("  + Samples per chunk    = %d", waveletInfoChunk.mSamplesPerChunk);
        MCore::LogDetailedInfo("  + Decompressed rot     = %d bytes", waveletInfoChunk.mDecompressedRotNumBytes);
        MCore::LogDetailedInfo("  + Decompressed pos     = %d bytes", waveletInfoChunk.mDecompressedPosNumBytes);
        MCore::LogDetailedInfo("  + Decompressed scale   = %d bytes", waveletInfoChunk.mDecompressedScaleNumBytes);
        MCore::LogDetailedInfo("  + Decompressed morph   = %d bytes", waveletInfoChunk.mDecompressedMorphNumBytes);
        MCore::LogDetailedInfo("  + Num pos tracks       = %d", waveletInfoChunk.mNumPosTracks);
        MCore::LogDetailedInfo("  + Num rot tracks       = %d", waveletInfoChunk.mNumRotTracks);
        MCore::LogDetailedInfo("  + Num scale tracks     = %d", waveletInfoChunk.mNumScaleTracks);
        MCore::LogDetailedInfo("  + Num morph tracks     = %d", waveletInfoChunk.mNumMorphTracks);
        MCore::LogDetailedInfo("  + Chunk overhead       = %d", waveletInfoChunk.mChunkOverhead);
        MCore::LogDetailedInfo("  + Compressed size      = %d bytes", waveletInfoChunk.mCompressedSize);
        MCore::LogDetailedInfo("  + Optimized size       = %d bytes", waveletInfoChunk.mOptimizedSize);
        MCore::LogDetailedInfo("  + Uncompressed size    = %d bytes", waveletInfoChunk.mUncompressedSize);
        MCore::LogDetailedInfo("  + Max time             = %f", waveletInfoChunk.mMaxTime);
        MCore::LogDetailedInfo("  + Num submotions       = %d", waveletInfoChunk.mNumSubMotions);
        MCore::LogDetailedInfo("  + Num morph submotions = %d", waveletInfoChunk.mNumMorphSubMotions);
        MCore::LogDetailedInfo("  + Sample spacing       = %f", waveletInfoChunk.mSampleSpacing);
        MCore::LogDetailedInfo("  + Seconds per chunk    = %f", waveletInfoChunk.mSecondsPerChunk);
        MCore::LogDetailedInfo("  + Pos quant factor     = %f", waveletInfoChunk.mPosQuantFactor);
        MCore::LogDetailedInfo("  + Rot quant factor     = %f", waveletInfoChunk.mRotQuantFactor);
        MCore::LogDetailedInfo("  + Scale quant factor   = %f", waveletInfoChunk.mScaleQuantFactor);
        MCore::LogDetailedInfo("  + Morph quant factor   = %f", waveletInfoChunk.mMorphQuantFactor);
        MCore::LogDetailedInfo("  + Wavelet ID           = %d", waveletInfoChunk.mWaveletID);
        MCore::LogDetailedInfo("  + Compressor ID        = %d", waveletInfoChunk.mCompressorID);
        MCore::LogDetailedInfo("  + Unit Type            = %d", waveletInfoChunk.mUnitType);
        MCore::LogDetailedInfo("  + Scale                = %f", waveletInfoChunk.mScale);

        // convert endian
        ConvertUnsignedInt(&waveletInfoChunk.mNumChunks, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mSamplesPerChunk, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mDecompressedRotNumBytes, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mDecompressedPosNumBytes, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mDecompressedScaleNumBytes, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mDecompressedMorphNumBytes, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mNumRotTracks, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mNumScaleTracks, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mNumPosTracks, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mNumMorphTracks, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mChunkOverhead, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mCompressedSize, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mOptimizedSize, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mUncompressedSize, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mNumSubMotions, targetEndianType);
        ConvertUnsignedInt(&waveletInfoChunk.mNumMorphSubMotions, targetEndianType);
        ConvertFloat(&waveletInfoChunk.mPosQuantFactor, targetEndianType);
        ConvertFloat(&waveletInfoChunk.mRotQuantFactor, targetEndianType);
        ConvertFloat(&waveletInfoChunk.mScaleQuantFactor, targetEndianType);
        ConvertFloat(&waveletInfoChunk.mMorphQuantFactor, targetEndianType);
        ConvertFloat(&waveletInfoChunk.mSampleSpacing, targetEndianType);
        ConvertFloat(&waveletInfoChunk.mSecondsPerChunk, targetEndianType);
        ConvertFloat(&waveletInfoChunk.mMaxTime, targetEndianType);
        ConvertFloat(&waveletInfoChunk.mScale, targetEndianType);

        // the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID = EMotionFX::FileFormat::MOTION_CHUNK_WAVELETINFO;
        chunkHeader.mVersion = 1;

        // calculate size of the chunk
        chunkHeader.mSizeInBytes = sizeof(EMotionFX::FileFormat::Motion_WaveletInfo); // the info chunk itself
        chunkHeader.mSizeInBytes += sizeof(EMotionFX::FileFormat::Motion_WaveletMapping) * motion->GetNumSubMotions(); // wavelet mappings
        chunkHeader.mSizeInBytes += sizeof(uint16) * motion->GetNumMorphSubMotions(); // wavelet morph mappings
        chunkHeader.mSizeInBytes += GetWaveletSubMotionsSizeInBytes(motion); // submotions
        chunkHeader.mSizeInBytes += GetWaveletMorphSubMotionsSizeInBytes(motion); // morph submotions
        chunkHeader.mSizeInBytes += GetWaveletChunksSizeInBytes(motion); // wavelet chunks

        // convert endian and save it
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // save the info chunk
        file->Write(&waveletInfoChunk, sizeof(EMotionFX::FileFormat::Motion_WaveletInfo));

        // followed by:
        SaveWaveletMappings(file, motion, targetEndianType);
        SaveWaveletSkeletalSubMotions(file, motion, targetEndianType);
        SaveWaveletMorphSubMotions(file, motion, targetEndianType);
        SaveWaveletMorphMappings(file, motion, targetEndianType);

        SaveWaveletChunks(file, motion, targetEndianType);
    }


    // save a wavelet skeletal motion
    void SaveWaveletSkeletalMotion(MCore::Stream* file, EMotionFX::WaveletSkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        // swap endian in case we're saving for big endian
        if (targetEndianType == MCore::Endian::ENDIAN_BIG)
        {
            motion->SwapChunkDataEndian();
        }

        // save file header chunk
        SaveSkeletalMotionHeader(file, motion, targetEndianType);

        // save motion header info
        SaveWaveletSkeletalMotionInfo(file, motion, targetEndianType);

        // save motion events
        SaveMotionEvents(file, motion->GetEventTable(), targetEndianType);

        // undo the swap, if we swapped endian
        if (targetEndianType == MCore::Endian::ENDIAN_BIG)
        {
            motion->SwapChunkDataEndian();
        }
    }



    // convert the given skeletal motion file to a wavelet motion
    bool ConvertToWaveletSkeletalMotion(const AZStd::string& fileName, EMotionFX::WaveletSkeletalMotion::Settings* settings, MCore::Endian::EEndianType targetEndianType)
    {
        // load the skeletal motion from disk
        EMotionFX::Importer::SkeletalMotionSettings loadSettings;
        loadSettings.mForceLoading = true;
        loadSettings.mUnitTypeConvert = false;
        EMotionFX::SkeletalMotion* skeletalMotion = EMotionFX::GetImporter().LoadSkeletalMotion(fileName.c_str(), &loadSettings);
        if (skeletalMotion == nullptr)
        {
            return false;
        }

        // create the wavelet motion and convert the skeletal motion
        EMotionFX::WaveletSkeletalMotion* waveletMotion = EMotionFX::WaveletSkeletalMotion::Create(skeletalMotion->GetName());
        waveletMotion->Init(skeletalMotion, settings);
        waveletMotion->SetUnitType(skeletalMotion->GetUnitType());

        MCore::LogInfo("Compression ratio = %.2f (%.2f kb)  -  Compared to optimized ratio = %.2f (%.2f kb)  -  30 fps uncompressed = %.2f kb", waveletMotion->GetUncompressedSize() / (float)waveletMotion->GetCompressedSize(), waveletMotion->GetCompressedSize() / 1024.0f, waveletMotion->GetOptimizedSize() / (float)waveletMotion->GetCompressedSize(), waveletMotion->GetOptimizedSize() / 1024.0f, waveletMotion->GetUncompressedSize() / 1024.0f);

        // overwrite the disk file with the new wavelet motion
        MCore::DiskFile diskFile;
        if (diskFile.Open(fileName.c_str(), MCore::DiskFile::WRITE) == false)
        {
            return false;
        }

        SaveWaveletSkeletalMotion(&diskFile, waveletMotion, targetEndianType);

        diskFile.Close();

        if (waveletMotion)
        {
            waveletMotion->Destroy();
        }

        return true;
    }
} // namespace ExporterLib
