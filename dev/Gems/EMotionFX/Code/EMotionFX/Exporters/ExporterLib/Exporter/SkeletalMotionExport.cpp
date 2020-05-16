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

#include <AzCore/Math/Transform.h>
#include "Exporter.h"
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/SkeletalSubMotion.h>
#include <EMotionFX/Source/MorphSubMotion.h>
#include <EMotionFX/Source/KeyTrackLinear.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Actor.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>

// file format structures
#include <EMotionFX/Source/Importer/SharedFileFormatStructs.h>
#include <EMotionFX/Source/Importer/MotionFileFormat.h>


namespace ExporterLib
{
    void SaveSkeletalSubMotion(MCore::Stream* file, EMotionFX::SkeletalSubMotion* subMotion, MCore::Endian::EEndianType targetEndianType)
    {
        // get the animation start pose and the bind pose transformation information
        AZ::PackedVector3f                  posePosition        = AZ::PackedVector3f(subMotion->GetPosePos());
        MCore::Compressed16BitQuaternion    poseRotation        = subMotion->GetCompressedPoseRot();
        AZ::PackedVector3f                  bindPosePosition    = AZ::PackedVector3f(subMotion->GetBindPosePos());
        MCore::Compressed16BitQuaternion    bindPoseRotation    = subMotion->GetCompressedBindPoseRot();

        #ifndef EMFX_SCALE_DISABLED
            AZ::PackedVector3f                  bindPoseScale       = AZ::PackedVector3f(subMotion->GetBindPoseScale());
            AZ::PackedVector3f                  poseScale           = AZ::PackedVector3f(subMotion->GetPoseScale());
        #else
            AZ::PackedVector3f                  bindPoseScale       = AZ::PackedVector3f(1.0f, 1.0f, 1.0f);
            AZ::PackedVector3f                  poseScale           = AZ::PackedVector3f(1.0f, 1.0f, 1.0f);
        #endif

        EMotionFX::FileFormat::Motion_SkeletalSubMotion subMotionChunk;

        // copy start animation pose transformation to the chunk(time=frameStart)
        CopyVector(subMotionChunk.mPosePos, posePosition);
        Copy16BitQuaternion(subMotionChunk.mPoseRot, poseRotation);
        CopyVector(subMotionChunk.mPoseScale, poseScale);

        // copy the bind pose transformation to the chunk
        CopyVector(subMotionChunk.mBindPosePos, bindPosePosition);
        Copy16BitQuaternion(subMotionChunk.mBindPoseRot, bindPoseRotation);
        CopyVector(subMotionChunk.mBindPoseScale, bindPoseScale);

        // create an uncompressed version of the fixed and compressed rotation quaternions
        AZ::Quaternion uncompressedPoseRot       = MCore::Compressed16BitQuaternion(subMotionChunk.mPoseRot.mX, subMotionChunk.mPoseRot.mY, subMotionChunk.mPoseRot.mZ, subMotionChunk.mPoseRot.mW).ToQuaternion();
        AZ::Quaternion uncompressedBindPoseRot   = MCore::Compressed16BitQuaternion(subMotionChunk.mBindPoseRot.mX, subMotionChunk.mBindPoseRot.mY, subMotionChunk.mBindPoseRot.mZ, subMotionChunk.mBindPoseRot.mW).ToQuaternion();

        // check which components are animated
        bool translationAnimated    = subMotion->GetPosTrack() == nullptr ? false : subMotion->GetPosTrack()->CheckIfIsAnimated(AZ::Vector3(posePosition), MCore::Math::epsilon);
        bool rotationAnimated       = subMotion->GetRotTrack() == nullptr ? false : subMotion->GetRotTrack()->CheckIfIsAnimated(uncompressedPoseRot, 0.0001f);

        #ifndef EMFX_SCALE_DISABLED
            bool scaleAnimated          = subMotion->GetScaleTrack() == nullptr ? false : subMotion->GetScaleTrack()->CheckIfIsAnimated(subMotion->GetPoseScale(), 0.0001f);
            bool isUniformScaled        = subMotion->CheckIfIsUniformScaled();
        #else
            bool scaleAnimated          = false;
            bool isUniformScaled        = true;
        #endif

        // get number of keyframes
        uint32 numPosKeys, numRotKeys, numScaleKeys;
        if (translationAnimated)
        {
            numPosKeys    = subMotion->GetPosTrack()->GetNumKeys();
        }
        else
        {
            numPosKeys     = 0;
        }
        if (rotationAnimated)
        {
            numRotKeys    = subMotion->GetRotTrack()->GetNumKeys();
        }
        else
        {
            numRotKeys     = 0;
        }

        numScaleKeys = 0;
        if (scaleAnimated)
        {
            EMFX_SCALECODE
            (
                numScaleKeys  = subMotion->GetScaleTrack()->GetNumKeys();
            )
        }

        subMotionChunk.mNumPosKeys      = numPosKeys;
        subMotionChunk.mNumRotKeys      = numRotKeys;
        subMotionChunk.mNumScaleKeys    = numScaleKeys;

        MCore::LogDetailedInfo("- SubMotion: %s", subMotion->GetName());
        MCore::LogDetailedInfo("   + Pose Translation: x=%f y=%f z=%f", subMotionChunk.mPosePos.mX, subMotionChunk.mPosePos.mY, subMotionChunk.mPosePos.mZ);
        MCore::LogDetailedInfo("   + Pose Rotation:    x=%f y=%f z=%f w=%f", static_cast<float>(uncompressedPoseRot.GetX()), static_cast<float>(uncompressedPoseRot.GetY()), static_cast<float>(uncompressedPoseRot.GetZ()), static_cast<float>(uncompressedPoseRot.GetW()));
        MCore::LogDetailedInfo("   + Pose Scale:       x=%f y=%f z=%f", subMotionChunk.mPoseScale.mX, subMotionChunk.mPoseScale.mY, subMotionChunk.mPoseScale.mZ);
        MCore::LogDetailedInfo("   + Bind Pose Translation: x=%f y=%f z=%f", subMotionChunk.mBindPosePos.mX, subMotionChunk.mBindPosePos.mY, subMotionChunk.mBindPosePos.mZ);
        MCore::LogDetailedInfo("   + Bind Pose Rotation:    x=%f y=%f z=%f w=%f", static_cast<float>(uncompressedBindPoseRot.GetX()), static_cast<float>(uncompressedBindPoseRot.GetY()), static_cast<float>(uncompressedBindPoseRot.GetZ()), static_cast<float>(uncompressedBindPoseRot.GetW()));
        MCore::LogDetailedInfo("   + Bind Pose Scale:       x=%f y=%f z=%f", subMotionChunk.mBindPoseScale.mX, subMotionChunk.mBindPoseScale.mY, subMotionChunk.mBindPoseScale.mZ);
        if (translationAnimated)
        {
            MCore::LogDetailedInfo("   + Num Position Keys:     %d", subMotionChunk.mNumPosKeys);
        }
        if (rotationAnimated)
        {
            MCore::LogDetailedInfo("   + Num Rotation Keys:     %d", subMotionChunk.mNumRotKeys);
        }
        if (scaleAnimated)
        {
            MCore::LogDetailedInfo("   + Num Scale Keys:        %d (UniformScaling=%s)", subMotionChunk.mNumScaleKeys, AZStd::to_string(isUniformScaled).c_str());
        }

        // convert endian
        ConvertFileVector3(&subMotionChunk.mPosePos, targetEndianType);
        ConvertFile16BitQuaternion(&subMotionChunk.mPoseRot, targetEndianType);
        ConvertFileVector3(&subMotionChunk.mPoseScale, targetEndianType);

        ConvertFileVector3(&subMotionChunk.mBindPosePos, targetEndianType);
        ConvertFile16BitQuaternion(&subMotionChunk.mBindPoseRot, targetEndianType);
        ConvertFileVector3(&subMotionChunk.mBindPoseScale, targetEndianType);

        ConvertUnsignedInt(&subMotionChunk.mNumPosKeys, targetEndianType);
        ConvertUnsignedInt(&subMotionChunk.mNumRotKeys, targetEndianType);
        ConvertUnsignedInt(&subMotionChunk.mNumScaleKeys, targetEndianType);

        // save the submotion chunk
        file->Write(&subMotionChunk, sizeof(EMotionFX::FileFormat::Motion_SkeletalSubMotion));

        //followed by: the submotion name
        SaveString(subMotion->GetName(), file, targetEndianType);

        uint32 k;

        // iterate through all translation keyframes
        for (k = 0; k < numPosKeys; ++k)
        {
            // create and fill the keyframe
            EMotionFX::FileFormat::Motion_Vector3Key keyframe;
            keyframe.mTime = subMotion->GetPosTrack()->GetKey(k)->GetTime();
            CopyVector(keyframe.mValue, AZ::PackedVector3f(subMotion->GetPosTrack()->GetKey(k)->GetValue()));

            //LOG("       + Key#%i: Time=%f, Pos(%f, %f, %f)", k, keyframe.mTime, keyframe.mValue.mX, keyframe.mValue.mY, keyframe.mValue.mZ);

            // convert endian and write the keyframe
            ConvertFloat(&keyframe.mTime, targetEndianType);
            ConvertFileVector3(&keyframe.mValue, targetEndianType);
            file->Write(&keyframe, sizeof(EMotionFX::FileFormat::Motion_Vector3Key));
        }

        // iterate through all rotation keyframes
        for (k = 0; k < numRotKeys; ++k)
        {
            // create and fill the keyframe
            EMotionFX::FileFormat::Motion_16BitQuaternionKey keyframe;
            keyframe.mTime = subMotion->GetRotTrack()->GetKey(k)->GetTime();
            Copy16BitQuaternion(keyframe.mValue, subMotion->GetRotTrack()->GetKey(k)->GetStorageTypeValue());

            //LOG("       + Key#%i: Time=%f, Rot(%i, %i, %i, %i)", k, keyframe.mTime, keyframe.mValue.mX, keyframe.mValue.mY, keyframe.mValue.mZ, keyframe.mValue.mW);

            // convert endian and write the keyframe
            ConvertFloat(&keyframe.mTime, targetEndianType);
            ConvertFile16BitQuaternion(&keyframe.mValue, targetEndianType);
            file->Write(&keyframe, sizeof(EMotionFX::FileFormat::Motion_16BitQuaternionKey));
        }

        // iterate through all scale keyframes
        EMFX_SCALECODE
        (
            for (k = 0; k < numScaleKeys; ++k)
            {
                // create and fill the keyframe
                EMotionFX::FileFormat::Motion_Vector3Key keyframe;
                keyframe.mTime = subMotion->GetScaleTrack()->GetKey(k)->GetTime();
                CopyVector(keyframe.mValue, AZ::PackedVector3f(subMotion->GetScaleTrack()->GetKey(k)->GetValue()));

                //LOG("       + Key#%i: Time=%f, Scale(%f, %f, %f)", k, keyframe.mTime, keyframe.mValue.mX, keyframe.mValue.mY, keyframe.mValue.mZ);

                // convert endian and write the keyframe
                ConvertFloat(&keyframe.mTime, targetEndianType);
                ConvertFileVector3(&keyframe.mValue, targetEndianType);
                file->Write(&keyframe, sizeof(EMotionFX::FileFormat::Motion_Vector3Key));
            }
        )
    }


    void SaveSkeletalSubMotions(MCore::Stream* file, EMotionFX::SkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        uint32 i;

        // get the number of submotions
        const uint32 numSubMotions = motion->GetNumSubMotions();

        // the skeletal submotions chunk
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID = EMotionFX::FileFormat::MOTION_CHUNK_SUBMOTIONS;

        // calculate the size of the chunk
        chunkHeader.mSizeInBytes = sizeof(EMotionFX::FileFormat::Motion_SubMotions);
        for (i = 0; i < numSubMotions; ++i)
        {
            EMotionFX::SkeletalSubMotion* subMotion = motion->GetSubMotion(i);

            chunkHeader.mSizeInBytes    += sizeof(EMotionFX::FileFormat::Motion_SkeletalSubMotion);
            chunkHeader.mSizeInBytes    += GetStringChunkSize(subMotion->GetName());

            // TODO: this is not most efficient! this is already done at another time in code when saving the submotion, remove duplicated code
            AZ::Vector3                         posePosition    = subMotion->GetPosePos();
            MCore::Compressed16BitQuaternion    poseRotation    = subMotion->GetCompressedPoseRot();

            #ifndef EMFX_SCALE_DISABLED
                AZ::Vector3 poseScale = subMotion->GetPoseScale();
            #else
                AZ::Vector3 poseScale = AZ::Vector3::CreateOne();
            #endif

            // check which components are animated
            bool translationAnimated    = subMotion->GetPosTrack() == nullptr ? false : subMotion->GetPosTrack()->CheckIfIsAnimated(posePosition, MCore::Math::epsilon);
            bool rotationAnimated       = subMotion->GetRotTrack() == nullptr ? false : subMotion->GetRotTrack()->CheckIfIsAnimated(poseRotation.ToQuaternion(), 0.0001f);

            #ifndef EMFX_SCALE_DISABLED
                bool scaleAnimated = subMotion->GetScaleTrack() == nullptr ? false : subMotion->GetScaleTrack()->CheckIfIsAnimated(poseScale, 0.0001f);
            #else
                bool scaleAnimated = false;
            #endif

            // get number of keyframes
            uint32 numPosKeys, numRotKeys, numScaleKeys;
            if (translationAnimated)
            {
                numPosKeys    = subMotion->GetPosTrack()->GetNumKeys();
            }
            else
            {
                numPosKeys     = 0;
            }
            if (rotationAnimated)
            {
                numRotKeys    = subMotion->GetRotTrack()->GetNumKeys();
            }
            else
            {
                numRotKeys     = 0;
            }

            numScaleKeys = 0;
            if (scaleAnimated)
            {
                EMFX_SCALECODE
                (
                    numScaleKeys  = subMotion->GetScaleTrack()->GetNumKeys();
                )
            }

            chunkHeader.mSizeInBytes    += numPosKeys * sizeof(EMotionFX::FileFormat::Motion_Vector3Key);
            chunkHeader.mSizeInBytes    += numRotKeys * sizeof(EMotionFX::FileFormat::Motion_16BitQuaternionKey);
            chunkHeader.mSizeInBytes    += numScaleKeys * sizeof(EMotionFX::FileFormat::Motion_Vector3Key);
        }

        chunkHeader.mVersion = 1;

        // convert endian and write the chunk header
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // the submotions chunk
        EMotionFX::FileFormat::Motion_SubMotions submotionsChunk;
        submotionsChunk.mNumSubMotions = numSubMotions;

        // convert endian and write the submotons chunk
        ConvertUnsignedInt(&submotionsChunk.mNumSubMotions, targetEndianType);
        file->Write(&submotionsChunk, sizeof(EMotionFX::FileFormat::Motion_SubMotions));

        // iterate through all submotions and write them to the stream
        for (i = 0; i < numSubMotions; i++)
        {
            SaveSkeletalSubMotion(file, motion->GetSubMotion(i), targetEndianType);
        }
    }


    void SaveSkeletalMotion(MCore::Stream* file, EMotionFX::SkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType, bool onlyAnimatedMorphs)
    {
        // update the motion
        motion->UpdateMaxTime();

        // save file header chunk
        SaveSkeletalMotionHeader(file, motion, targetEndianType);

        // save motion header info
        SaveSkeletalMotionFileInfo(file, motion, targetEndianType);

        // save all skeletal submotions
        SaveSkeletalSubMotions(file, motion, targetEndianType);

        // save the morph motion data
        SaveMorphSubMotions(file, motion, targetEndianType, onlyAnimatedMorphs);

        // save motion events
        SaveMotionEvents(file, motion->GetEventTable(), targetEndianType);
    }


    void SaveSkeletalMotion(AZStd::string& filename, EMotionFX::SkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType, bool onlyAnimatedMorphs)
    {
        if (filename.empty())
        {
            AZ_Error("EMotionFX", false, "Cannot save actor. Filename is empty.");
            return;
        }

        MCore::MemoryFile memoryFile;
        memoryFile.Open();
        memoryFile.SetPreAllocSize(262144); // 256kb

        // Save the motion to the memory file.
        SaveSkeletalMotion(&memoryFile, motion, targetEndianType, onlyAnimatedMorphs);

        // Make sure the file has the correct extension and write the data from memory to disk.
        AzFramework::StringFunc::Path::ReplaceExtension(filename, GetSkeletalMotionExtension());
        memoryFile.SaveToDiskFile(filename.c_str());
        memoryFile.Close();
    }


    // optimize the submotion keytracks
    void OptimizeSkeletalSubMotion(EMotionFX::SkeletalSubMotion* subMotion, bool optimizePos, bool optimizeRot, bool optimizeScale, float maxPosError, float maxRotError, float maxScaleError)
    {
        if (optimizePos || optimizeRot || optimizeScale)
        {
            MCore::LogInfo("Keyframe Reduction Statistics for submotion '%s':", subMotion->GetName());
        }

        // optimize the position keytrack
        if (optimizePos && subMotion->GetPosTrack())
        {
            uint32 numPosKeys       = subMotion->GetPosTrack()->GetNumKeys();
            uint32 numPosRemoved    = subMotion->GetPosTrack()->Optimize(maxPosError);
            if (numPosRemoved > 0)
            {
                MCore::LogInfo("   + Position keys removed = %d out of %d, which is a reduction of %.2f%%", numPosRemoved, numPosKeys, (numPosRemoved / (float)numPosKeys) * 100.0f);
            }
        }

        // optimize the rotation keytrack
        if (optimizeRot && subMotion->GetRotTrack())
        {
            uint32 numRotKeys       = subMotion->GetRotTrack()->GetNumKeys();
            uint32 numRotRemoved    = subMotion->GetRotTrack()->Optimize(maxRotError);
            if (numRotRemoved > 0)
            {
                MCore::LogInfo("   + Rotation keys removed = %d out of %d, which is a reduction of %.2f%%", numRotRemoved, numRotKeys, (numRotRemoved / (float)numRotKeys) * 100.0f);
            }
        }

        EMFX_SCALECODE
        (
            if (optimizeScale && subMotion->GetScaleTrack())
            {
                uint32 numScaleKeys     = subMotion->GetScaleTrack()->GetNumKeys();
                uint32 numScaleRemoved  = subMotion->GetScaleTrack()->Optimize(maxScaleError);
                if (numScaleRemoved > 0)
                {
                    MCore::LogInfo("   + Scale    keys removed = %d out of %d, which is a reduction of %.2f%%", numScaleRemoved, numScaleKeys, (numScaleRemoved / (float)numScaleKeys) * 100.0f);
                }
            }
        )
    }


    void AddSortedKey(EMotionFX::SkeletalSubMotion* subMotion, float time, const AZ::Vector3& position, const AZ::Quaternion& rotation, const AZ::Vector3& scale)
    {
        if (subMotion->GetPosTrack() == nullptr)
        {
            subMotion->CreatePosTrack();
        }

        if (subMotion->GetRotTrack() == nullptr)
        {
            subMotion->CreateRotTrack();
        }

        EMFX_SCALECODE
        (
            if (subMotion->GetScaleTrack() == nullptr)
            {
                subMotion->CreateScaleTrack();
            }
        )

        // position
        subMotion->GetPosTrack()->AddKeySorted(time, position);

        // rotation
        AZ::Quaternion fixedRot = rotation;
        if (fixedRot.GetW() < 0.0f)
        {
            fixedRot = -fixedRot;
        }
        fixedRot.NormalizeExact();
        subMotion->GetRotTrack()->AddKeySorted(time, fixedRot);

        // scale
        EMFX_SCALECODE
        (
            subMotion->GetScaleTrack()->AddKeySorted(time, scale);
        )
    }


    void FindMorphSubMotionMinMaxWeights(EMotionFX::MorphSubMotion* subMotion, float* outMinWeight, float* outMaxWeight)
    {
        *outMinWeight = subMotion->GetPoseWeight();
        *outMaxWeight = subMotion->GetPoseWeight();

        const uint32 numKeyframes = subMotion->GetKeyTrack()->GetNumKeys();
        for (uint32 i = 0; i < numKeyframes; ++i)
        {
            const float value = subMotion->GetKeyTrack()->GetKey(i)->GetValue();
            *outMinWeight = MCore::Min<float>(*outMinWeight, value);
            *outMaxWeight = MCore::Max<float>(*outMaxWeight, value);
        }

        // make sure the range is big enough
        if (*outMaxWeight - *outMinWeight < 1.0f)
        {
            if (*outMinWeight < 0.0f && *outMinWeight > -1.0f)
            {
                *outMinWeight = -1.0f;
            }

            if (*outMaxWeight > 0.0f && *outMaxWeight < 1.0f)
            {
                *outMaxWeight = 1.0f;
            }
        }
    }


    const char* GetMorphSubMotionName(EMotionFX::MorphSubMotion* subMotion)
    {
        return MCore::GetStringIdPool().GetName(subMotion->GetID()).c_str();
    }


    void SaveMorphSubMotion(MCore::Stream* file, EMotionFX::MorphSubMotion* subMotion, MCore::Endian::EEndianType targetEndianType)
    {
        MCore::LogInfo("Saving morph sub motion with ID %d", subMotion->GetID());

        // get the keytrack of the submotion
        EMotionFX::KeyTrackLinear<float, MCore::Compressed16BitFloat>* keytrack = subMotion->GetKeyTrack();

        // if there are no keys we don't need to save the submotion
        //if (keytrack->GetNumKeys() <= 0)
        //return;

        // find the min and max weight values
        float minWeight, maxWeight;
        FindMorphSubMotionMinMaxWeights(subMotion, &minWeight, &maxWeight);

        const uint32                numKeyframes    = keytrack->GetNumKeys();
        const AZStd::string         subMotionName   = GetMorphSubMotionName(subMotion);
        EMotionFX::MorphTarget::EPhonemeSet phonemeSet      = subMotion->GetPhonemeSet();

        // check if the name is valid
        if (subMotionName.empty() && phonemeSet == 0)
        {
            MCore::LogError("Cannot save  morph submotion with empty name and no phoneme set applied.");
            return;
        }

        // the submotion chunk
        EMotionFX::FileFormat::Motion_MorphSubMotion subMotionChunk;
        subMotionChunk.mNumKeys     = numKeyframes;
        subMotionChunk.mPoseWeight  = subMotion->GetPoseWeight();
        subMotionChunk.mMinWeight   = minWeight;
        subMotionChunk.mMaxWeight   = maxWeight;
        subMotionChunk.mPhonemeSet  = (uint32)phonemeSet;

        // log info
        MCore::LogDetailedInfo("    - Morph Submotion: '%s'", subMotionName.c_str());
        MCore::LogDetailedInfo("       + NrKeys             = %d", subMotionChunk.mNumKeys);
        MCore::LogDetailedInfo("       + Pose Weight        = %f", subMotionChunk.mPoseWeight);
        MCore::LogDetailedInfo("       + Minumum Weight     = %f", subMotionChunk.mMinWeight);
        MCore::LogDetailedInfo("       + Maximum Weight     = %f", subMotionChunk.mMaxWeight);
        if (phonemeSet != EMotionFX::MorphTarget::PHONEMESET_NONE)
        {
            MCore::LogDetailedInfo("       + PhonemeSet         = %s", EMotionFX::MorphTarget::GetPhonemeSetString((EMotionFX::MorphTarget::EPhonemeSet)subMotionChunk.mPhonemeSet).c_str());
        }

        // convert endian
        ConvertUnsignedInt(&subMotionChunk.mNumKeys, targetEndianType);
        ConvertFloat(&subMotionChunk.mPoseWeight, targetEndianType);
        ConvertFloat(&subMotionChunk.mMinWeight, targetEndianType);
        ConvertFloat(&subMotionChunk.mMaxWeight, targetEndianType);
        ConvertUnsignedInt(&subMotionChunk.mPhonemeSet, targetEndianType);

        // save the chunk, followed by the name of the submotion
        file->Write(&subMotionChunk, sizeof(EMotionFX::FileFormat::Motion_MorphSubMotion));
        SaveString(subMotionName, file, targetEndianType);

        // iterate through all keyframes
        for (uint32 i = 0; i < numKeyframes; ++i)
        {
            EMotionFX::KeyFrame<float, MCore::Compressed16BitFloat>* keyframe = keytrack->GetKey(i);

            // create the keyframe chunk
            EMotionFX::FileFormat::Motion_UnsignedShortKey keyChunk;
            memset(&keyChunk, 0, sizeof(EMotionFX::FileFormat::Motion_UnsignedShortKey));
            keyChunk.mTime  = keyframe->GetTime();
            keyChunk.mValue = keyframe->GetStorageTypeValue().mValue;

            //const float range = maxWeight - minWeight;
            //MCORE_ASSERT( range > Math::epsilon );
            //const float normalizedWeight = (keyframe->GetValue() - minWeight) / range;
            //const uint16 value = (uint16)(normalizedWeight * 65535);
            //MCORE_ASSERT( keyChunk.mValue == value );
            //LogDetailedInfo("      - Key #%i: Time=%f, Value=%f, NormalizedVal=%f", i, keyframe->GetTime(), keyframe->GetValue(), normalizedWeight);
            //LOG("     + Key#%i: Time=%f Value=%i", i, keyChunk.mTime, keyChunk.mValue);

            // convert endian and save
            ConvertFloat(&keyChunk.mTime, targetEndianType);
            ConvertUnsignedShort(&keyChunk.mValue, targetEndianType);
            file->Write(&keyChunk, sizeof(EMotionFX::FileFormat::Motion_UnsignedShortKey));
        }
    }


    void SaveMorphSubMotions(MCore::Stream* file, EMotionFX::SkeletalMotion* motion, MCore::Endian::EEndianType targetEndianType, bool onlyAnimated)
    {
        uint32 i;

        // get the number of  morph submotions
        const uint32 numSubMotions = motion->GetNumMorphSubMotions();
        uint32 numSubMotionsToSave = 0;

        // update and get the maximum time of the morph motion
        motion->UpdateMaxTime();
        const float maxTime = motion->GetMaxTime();

        // pre-saving phase
        for (i = 0; i < numSubMotions; ++i)
        {
            EMotionFX::MorphSubMotion* subMotion = motion->GetMorphSubMotion(i);
            EMotionFX::KeyTrackLinear<float, MCore::Compressed16BitFloat>* keytrack = subMotion->GetKeyTrack();

            if (keytrack->GetNumKeys() > 0)
            {
                // check if the animation starts at time=0.0, if not add a key at 0.0
                if (keytrack->GetFirstKey()->GetTime() > 0.0f)
                {
                    subMotion->GetKeyTrack()->AddKeySorted(0.0f, 0.0f);
                }

                // add key at maxtime
                if (keytrack->GetLastKey()->GetTime() < maxTime)
                {
                    subMotion->GetKeyTrack()->AddKeySorted(maxTime, keytrack->GetLastKey()->GetValue());
                }

                // check if the submotion is animated, if not remove all its keyframes
                if (keytrack->CheckIfIsAnimated(subMotion->GetPoseWeight(), 0.001f) == false)
                {
                    keytrack->ClearKeys();
                }
            }
        }


        // the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID = EMotionFX::FileFormat::MOTION_CHUNK_MORPHSUBMOTIONS;
        chunkHeader.mVersion = 1;

        // calculate the size of the chunk
        chunkHeader.mSizeInBytes = sizeof(EMotionFX::FileFormat::Motion_MorphSubMotions);
        for (i = 0; i < numSubMotions; ++i)
        {
            EMotionFX::MorphSubMotion* subMotion = motion->GetMorphSubMotion(i);

            bool include = true;
            if (onlyAnimated && (subMotion->GetKeyTrack()->GetNumKeys() == 0))
            {
                include = false;
            }

            // if there are no keys we don't need to save the submotion

            if (include)
            {
                numSubMotionsToSave++;
                chunkHeader.mSizeInBytes += sizeof(EMotionFX::FileFormat::Motion_MorphSubMotion);
                chunkHeader.mSizeInBytes += subMotion->GetKeyTrack()->GetNumKeys() * sizeof(EMotionFX::FileFormat::Motion_UnsignedShortKey);
                chunkHeader.mSizeInBytes += GetStringChunkSize(GetMorphSubMotionName(subMotion));
            }
        }

        // convert endian and save it
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // the submotions chunk
        EMotionFX::FileFormat::Motion_SubMotions submotionsChunk;
        submotionsChunk.mNumSubMotions = numSubMotionsToSave;

        // convert endian and save it
        ConvertUnsignedInt(&submotionsChunk.mNumSubMotions, targetEndianType);
        file->Write(&submotionsChunk, sizeof(EMotionFX::FileFormat::Motion_SubMotions));

        // iterate through the submotions and save them
        for (i = 0; i < numSubMotions; ++i)
        {
            EMotionFX::MorphSubMotion* subMotion = motion->GetMorphSubMotion(i);
            bool include = true;
            if (onlyAnimated && (subMotion->GetKeyTrack()->GetNumKeys() == 0))
            {
                include = false;
            }

            if (include)
            {
                SaveMorphSubMotion(file, motion->GetMorphSubMotion(i), targetEndianType);
            }
        }
    }


    void OptimizeMorphSubMotions(EMotionFX::SkeletalMotion* motion, float maxError)
    {
        // get the number of submotions and iterate through them
        const uint32 numSubMotions = motion->GetNumMorphSubMotions();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            EMotionFX::MorphSubMotion* subMotion = motion->GetMorphSubMotion(i);

            uint32 numKeys      = subMotion->GetKeyTrack()->GetNumKeys();
            uint32 numRemoved   = subMotion->GetKeyTrack()->Optimize(maxError);

            AZStd::string subMotionName = GetMorphSubMotionName(subMotion);
            if (subMotionName.empty())
            {
                subMotionName = EMotionFX::MorphTarget::GetPhonemeSetString(subMotion->GetPhonemeSet());
            }
            if (numRemoved > 0)
            {
                MCore::LogInfo(" - Keyframe Reduction Statistics for morph submotion '%s': keys removed = %d out of %d, which is a reduction of %.2f%%", subMotionName.c_str(), numRemoved, numKeys, (numRemoved / (float)numKeys) * 100.0f);
            }
        }
    }



    void AddSortedKey(EMotionFX::MorphSubMotion* subMotion, float time, float value)
    {
        // TODO: hack! fix me! this is becaused the compressed 16 bit float's range is currently hardcoded to range [0.0, 1.0]
        if (value < 0.0f)
        {
            value = 0.0f;
        }

        if (value > 1.0f)
        {
            value = 1.0f;
        }

        subMotion->GetKeyTrack()->AddKeySorted(time, value);
    }


    EMotionFX::MorphSubMotion* GetMorphSubMotionByName(EMotionFX::SkeletalMotion* motion, const AZStd::string& name)
    {
        if (name.empty())
        {
            return nullptr;
        }

        // get the number of submotions and iterate through them
        const uint32 numMorphSubMotions = motion->GetNumMorphSubMotions();
        for (uint32 i = 0; i < numMorphSubMotions; i++)
        {
            EMotionFX::MorphSubMotion* subMotion = motion->GetMorphSubMotion(i);
            if (name == GetMorphSubMotionName(subMotion))
            {
                return subMotion;
            }
        }

        return nullptr;
    }


    EMotionFX::MorphSubMotion* CreateAddMorphSubMotion(EMotionFX::SkeletalMotion* motion, const AZStd::string& subMotionName)
    {
        // calculate the ID
        const uint32 nameID = MCore::GetStringIdPool().GenerateIdForString(subMotionName);

        EMotionFX::MorphSubMotion* subMotion = EMotionFX::MorphSubMotion::Create(nameID);
        motion->AddMorphSubMotion(subMotion);

        return subMotion;
    }
} // namespace ExporterLib
