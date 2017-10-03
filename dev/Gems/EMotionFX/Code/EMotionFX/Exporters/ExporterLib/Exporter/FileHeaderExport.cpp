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

#include <EMotionFX/Source/Importer/ActorFileFormat.h>
#include <EMotionFX/Source/Importer/MotionFileFormat.h>
//#include <EMotionFX/Source/Importer/XPMFileFormat.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/WaveletSkeletalMotion.h>
#include <EMotionFX/Source/Actor.h>


namespace ExporterLib
{
    void SaveActorHeader(MCore::Stream* file, MCore::Endian::EEndianType targetEndianType)
    {
        // the header information
        EMotionFX::FileFormat::Actor_Header header;
        header.mFourcc[0] = 'A';
        header.mFourcc[1] = 'C';
        header.mFourcc[2] = 'T';
        header.mFourcc[3] = 'R';
        header.mHiVersion   = static_cast<uint8>(GetFileHighVersion());
        header.mLoVersion   = static_cast<uint8>(GetFileLowVersion());
        header.mEndianType  = static_cast<uint8>(targetEndianType);
        //header.mMulOrder  = EMotionFX::FileFormat::MULORDER_ROT_SCALE_TRANS;

        // write the header to the stream
        file->Write(&header, sizeof(EMotionFX::FileFormat::Actor_Header));
    }


    void SaveActorFileInfo(MCore::Stream* file,
        uint32 numLODLevels,
        uint32 motionExtractionNodeIndex,
        const char* sourceApp,
        const char* orgFileName,
        const char* actorName,
        float retargetRootOffset,
        MCore::Distance::EUnitType unitType,
        MCore::Endian::EEndianType targetEndianType)
    {
        // chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ACTOR_CHUNK_INFO;

        chunkHeader.mSizeInBytes    = sizeof(EMotionFX::FileFormat::Actor_Info);
        chunkHeader.mSizeInBytes    += GetStringChunkSize(sourceApp);
        chunkHeader.mSizeInBytes    += GetStringChunkSize(orgFileName);
        chunkHeader.mSizeInBytes    += GetStringChunkSize(GetCompilationDate());
        chunkHeader.mSizeInBytes    += GetStringChunkSize(actorName);

        chunkHeader.mVersion        = 1;

        EMotionFX::FileFormat::Actor_Info infoChunk;
        infoChunk.mNumLODs                      = numLODLevels;
        infoChunk.mMotionExtractionNodeIndex    = motionExtractionNodeIndex;
        infoChunk.mTrajectoryNodeIndex          = MCORE_INVALIDINDEX32;
        infoChunk.mExporterHighVersion          = static_cast<uint8>(EMotionFX::GetEMotionFX().GetHighVersion());
        infoChunk.mExporterLowVersion           = static_cast<uint8>(EMotionFX::GetEMotionFX().GetLowVersion());
        infoChunk.mRetargetRootOffset           = retargetRootOffset;
        infoChunk.mUnitType                     = static_cast<uint8>(unitType);
        /*
            // retrieve the individual components of the motion extraction mask
            bool capturePosX    = motionExtractionMask & Actor::EXTRACT_POSITION_X;
            bool capturePosY    = motionExtractionMask & Actor::EXTRACT_POSITION_Y;
            bool capturePosZ    = motionExtractionMask & Actor::EXTRACT_POSITION_Z;
            bool captureRotX    = motionExtractionMask & Actor::EXTRACT_ROTATION_X;
            bool captureRotY    = motionExtractionMask & Actor::EXTRACT_ROTATION_Y;
            bool captureRotZ    = motionExtractionMask & Actor::EXTRACT_ROTATION_Z;
        */
        // print repositioning node information
        MCore::LogDetailedInfo("- File Info");
        MCore::LogDetailedInfo("   + Actor Name: '%s'", actorName);
        MCore::LogDetailedInfo("   + Source Application: '%s'", sourceApp);
        MCore::LogDetailedInfo("   + Original File: '%s'", orgFileName);
        MCore::LogDetailedInfo("   + Exporter Version: v%i.%i", infoChunk.mExporterHighVersion, infoChunk.mExporterLowVersion);
        MCore::LogDetailedInfo("   + Exporter Compilation Date: '%s'", GetCompilationDate());
        MCore::LogDetailedInfo("   + Num LODs = %i", infoChunk.mNumLODs);
        MCore::LogDetailedInfo("   + Retarget root offset: %f", infoChunk.mRetargetRootOffset);
        MCore::LogDetailedInfo("   + Motion extraction node index = %i", infoChunk.mMotionExtractionNodeIndex);
        /*  MCore::LogDetailedInfo("   - Motion Extraction mask:");
            MCore::LogDetailedInfo("      + Capture Position: (x=%s, y=%s, z=%s)", capturePosX ? "true" : "false", capturePosY ? "true" : "false", capturePosZ ? "true" : "false");
            MCore::LogDetailedInfo("      + Capture Rotation: (x=%s, y=%s, z=%s)", captureRotX ? "true" : "false", captureRotY ? "true" : "false", captureRotZ ? "true" : "false");
        */
        // endian conversion
        ConvertFileChunk(&chunkHeader, targetEndianType);
        ConvertUnsignedInt(&infoChunk.mMotionExtractionNodeIndex, targetEndianType);
        ConvertUnsignedInt(&infoChunk.mTrajectoryNodeIndex, targetEndianType);
        ConvertUnsignedInt(&infoChunk.mNumLODs, targetEndianType);
        ConvertFloat(&infoChunk.mRetargetRootOffset, targetEndianType);

        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));
        file->Write(&infoChunk, sizeof(EMotionFX::FileFormat::Actor_Info));

        SaveString(sourceApp, file, targetEndianType);
        SaveString(orgFileName, file, targetEndianType);
        SaveString(GetCompilationDate(), file, targetEndianType);
        SaveString(actorName, file, targetEndianType);
    }


    void SaveSkeletalMotionHeader(MCore::Stream* file, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        // the header information
        EMotionFX::FileFormat::Motion_Header header;

        header.mFourcc[0]   = 'M';
        header.mFourcc[1]   = 'O';
        header.mFourcc[2]   = 'T';

        switch (motion->GetType())
        {
        case EMotionFX::SkeletalMotion::TYPE_ID:
        {
            header.mFourcc[3] = ' ';
            break;
        }
        case EMotionFX::WaveletSkeletalMotion::TYPE_ID:
        {
            header.mFourcc[3] = 'W';
            break;
        }
        default:
        {
            header.mFourcc[3] = '?';
        }
        }

        header.mHiVersion   = static_cast<uint8>(GetFileHighVersion());
        header.mLoVersion   = static_cast<uint8>(GetFileLowVersion());
        header.mEndianType  = static_cast<uint8>(targetEndianType);

        // write the header to the stream
        file->Write(&header, sizeof(EMotionFX::FileFormat::Motion_Header));
    }


    void SaveSkeletalMotionFileInfo(MCore::Stream* file, EMotionFX::SkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        // chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID      = EMotionFX::FileFormat::MOTION_CHUNK_INFO;
        chunkHeader.mSizeInBytes  = sizeof(EMotionFX::FileFormat::Motion_Info2);
        chunkHeader.mVersion      = 2;

        EMotionFX::FileFormat::Motion_Info2 infoChunk;
        infoChunk.mMotionExtractionFlags    = motion->GetMotionExtractionFlags();
        infoChunk.mMotionExtractionNodeIndex= MCORE_INVALIDINDEX32; // not used anymore
        infoChunk.mUnitType                 = static_cast<uint8>(motion->GetUnitType());

        MCore::LogDetailedInfo("- File Info");
        MCore::LogDetailedInfo("   + Exporter Compilation Date    = '%s'", GetCompilationDate());
        MCore::LogDetailedInfo("   + Motion Extraction Flags      = 0x%x [capZ=%d]", infoChunk.mMotionExtractionFlags, (infoChunk.mMotionExtractionFlags & EMotionFX::MOTIONEXTRACT_CAPTURE_Z) ? 1 : 0);

        // endian conversion
        ConvertFileChunk(&chunkHeader, targetEndianType);
        ConvertUnsignedInt(&infoChunk.mMotionExtractionFlags, targetEndianType);
        ConvertUnsignedInt(&infoChunk.mMotionExtractionNodeIndex, targetEndianType);

        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));
        file->Write(&infoChunk, sizeof(EMotionFX::FileFormat::Motion_Info2));
    }
} // namespace ExporterLib
