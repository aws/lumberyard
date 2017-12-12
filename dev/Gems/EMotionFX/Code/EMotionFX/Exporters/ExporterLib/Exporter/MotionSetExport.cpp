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
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Importer/SharedFileFormatStructs.h>
#include <EMotionFX/Source/Importer/MotionSetFileFormat.h>


namespace ExporterLib
{
    uint32 GetMotionSetSize(EMotionFX::MotionSet* motionSet)
    {
        uint32 result = sizeof(EMotionFX::FileFormat::MotionSetChunk);
        result += GetStringChunkSize(motionSet->GetName());
        result += GetStringChunkSize("");

        const EMotionFX::MotionSet::EntryMap& motionEntries = motionSet->GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

            result += GetStringChunkSize(motionEntry->GetFilename());
            result += GetStringChunkSize(motionEntry->GetID().c_str());
        }

        return result;
    }


    void SaveMotionSet(MCore::Stream* file, EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet* parentSet, MCore::Endian::EEndianType targetEndianType)
    {
        EMotionFX::FileFormat::MotionSetChunk motionSetChunk;

        const AZ::u32 numMotionEntries      = static_cast<AZ::u32>(motionSet->GetNumMotionEntries());
        motionSetChunk.mNumChildSets        = motionSet->GetNumChildSets();
        motionSetChunk.mNumMotionEntries    = numMotionEntries;

        ConvertUnsignedInt(&motionSetChunk.mNumChildSets,      targetEndianType);
        ConvertUnsignedInt(&motionSetChunk.mNumMotionEntries,  targetEndianType);

        file->Write(&motionSetChunk, sizeof(EMotionFX::FileFormat::MotionSetChunk));

        // Save the parent set name.
        if (!parentSet)
        {
            SaveString("", file, targetEndianType);
        }
        else
        {
            SaveString(parentSet->GetName(), file, targetEndianType);
        }

        // Save the name and filename of the motion set.
        SaveString(motionSet->GetName(), file, targetEndianType);
        SaveString("", file, targetEndianType);

        // Save all motion entries.
        const EMotionFX::MotionSet::EntryMap& motionEntries = motionSet->GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

            SaveString(motionEntry->GetFilename(), file, targetEndianType);
            SaveString(motionEntry->GetID().c_str(), file, targetEndianType);
        }
    }


    uint32 GetMotionSetsSize(const AZStd::vector<EMotionFX::MotionSet*>& motionSets)
    {
        uint32 result = sizeof(EMotionFX::FileFormat::MotionSetsChunk);

        const size_t numMotionSets  = motionSets.size();
        for (size_t i = 0; i < numMotionSets; ++i)
        {
            result += GetMotionSetSize(motionSets[i]);
        }

        return result;
    }


    void SaveMotionSetHeader(MCore::Stream* file, MCore::Endian::EEndianType targetEndianType)
    {
        EMotionFX::FileFormat::MotionSet_Header header;
        header.mFourCC[0] = 'M';
        header.mFourCC[1] = 'O';
        header.mFourCC[2] = 'S';
        header.mFourCC[3] = ' ';
        header.mHiVersion   = static_cast<uint8>(GetFileHighVersion());
        header.mLoVersion   = static_cast<uint8>(GetFileLowVersion());
        header.mEndianType  = static_cast<uint8>(targetEndianType);

        file->Write(&header, sizeof(EMotionFX::FileFormat::MotionSet_Header));
    }


    void SaveMotionSets(MCore::Stream* file, const AZStd::vector<EMotionFX::MotionSet*>& motionSets, MCore::Endian::EEndianType targetEndianType)
    {
        SaveMotionSetHeader(file, targetEndianType);

        // Prepare and save the motion sets chunk.
        EMotionFX::FileFormat::MotionSetsChunk motionSetsChunk;
        const size_t numMotionSets  = motionSets.size();
        motionSetsChunk.mNumSets    = static_cast<uint32>(numMotionSets);

        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::CHUNK_MOTIONSET;
        chunkHeader.mSizeInBytes    = GetMotionSetsSize(motionSets);
        chunkHeader.mVersion        = 1;

        ConvertUnsignedInt(&motionSetsChunk.mNumSets, targetEndianType);
        ConvertFileChunk(&chunkHeader, targetEndianType);

        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));
        file->Write(&motionSetsChunk, sizeof(EMotionFX::FileFormat::MotionSetsChunk));

        // Save all motion sets.
        for (size_t i = 0; i < numMotionSets; ++i)
        {
            EMotionFX::MotionSet* motionSet = motionSets[i];
            EMotionFX::MotionSet* parentSet = motionSet->GetParentSet();

            SaveMotionSet(file, motionSet, parentSet, targetEndianType);
        }
    }


    const char* GetMotionSetFileExtension(bool includingDot)
    {
        if (includingDot)
        {
            return ".motionset";
        }

        return "motionset";
    }
} // namespace ExporterLib
