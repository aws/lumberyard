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
#include "ExporterFileProcessor.h"
#include "Exporter.h"
#include <AzCore/Debug/Timer.h>
#include <MCore/Source/StringConversions.h>


namespace ExporterLib
{
#define PREPARE_DISKFILE_SAVE(EXTENSION)                      \
    AZ::Debug::Timer saveTimer;                               \
    saveTimer.Stamp();                                        \
    if (filenameWithoutExtension.empty())                     \
    {                                                         \
        MCore::LogError("Cannot save file. Empty FileName."); \
        return false;                                         \
    }                                                         \
    AZStd::string extension;                                  \
    AzFramework::StringFunc::Path::GetExtension(filenameWithoutExtension.c_str(), extension); \
    if (!extension.empty())                                   \
    {                                                         \
        AzFramework::StringFunc::Replace(filenameWithoutExtension, extension.c_str(), EXTENSION(), true, false, true); \
    }                                                         \
    else                                                      \
    {                                                         \
        filenameWithoutExtension += EXTENSION();              \
    }                                                         \
                                                              \
    MCore::MemoryFile memoryFile;                             \
    memoryFile.Open();                                        \


#define FINISH_DISKFILE_SAVE                                                                                                    \
    bool result = memoryFile.SaveToDiskFile(filenameWithoutExtension.c_str());                                                  \
    const float saveTime = saveTimer.GetDeltaTimeInSeconds() * 1000.0f;                                                         \
    MCore::LogInfo("Saved file '%s' in %.2f ms.", filenameWithoutExtension.c_str(), saveTime);                                  \
    return result;


    // constructor
    Exporter::Exporter()
        : EMotionFX::BaseObject()
    {
    }


    // destructor
    Exporter::~Exporter()
    {
    }


    // create the exporter
    Exporter* Exporter::Create()
    {
        return new Exporter();
    }


    void Exporter::Delete()
    {
        delete this;
    }


    // make the memory file ready for saving
    void Exporter::ResetMemoryFile(MCore::MemoryFile* file)
    {
        // make sure the file is valid
        MCORE_ASSERT(file);

        // reset the incoming memory file
        file->Close();
        file->Open();
        file->SetPreAllocSize(262144); // 256kB
        file->Seek(0);
    }


    // actor
    bool Exporter::SaveActor(MCore::MemoryFile* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        ResetMemoryFile(file);
        ExporterLib::SaveActor(file, actor, targetEndianType);
        return true;
    }


    bool Exporter::SaveActor(AZStd::string filenameWithoutExtension, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        PREPARE_DISKFILE_SAVE(GetActorExtension);
        if (SaveActor(&memoryFile, actor, targetEndianType) == false)
        {
            return false;
        }
        FINISH_DISKFILE_SAVE
    }


    // skeletal motion
    bool Exporter::SaveSkeletalMotion(MCore::MemoryFile* file, EMotionFX::SkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType, bool onlyAnimatedMorphs)
    {
        ResetMemoryFile(file);
        ExporterLib::SaveSkeletalMotion(file, motion, targetEndianType, onlyAnimatedMorphs);
        return true;
    }


    bool Exporter::SaveSkeletalMotion(AZStd::string filenameWithoutExtension, EMotionFX::SkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType, bool onlyAnimatedMorphs)
    {
        PREPARE_DISKFILE_SAVE(GetSkeletalMotionExtension);
        if (SaveSkeletalMotion(&memoryFile, motion, targetEndianType, onlyAnimatedMorphs) == false)
        {
            return false;
        }
        FINISH_DISKFILE_SAVE
    }


    // wavelet skeletal motion
    bool Exporter::SaveWaveletSkeletalMotion(MCore::MemoryFile* file, EMotionFX::WaveletSkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        ResetMemoryFile(file);
        ExporterLib::SaveWaveletSkeletalMotion(file, motion, targetEndianType);
        return true;
    }


    bool Exporter::SaveWaveletSkeletalMotion(AZStd::string filenameWithoutExtension, EMotionFX::WaveletSkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        PREPARE_DISKFILE_SAVE(GetSkeletalMotionExtension);
        if (SaveWaveletSkeletalMotion(&memoryFile, motion, targetEndianType) == false)
        {
            return false;
        }
        FINISH_DISKFILE_SAVE
    }
} // namespace ExporterLib