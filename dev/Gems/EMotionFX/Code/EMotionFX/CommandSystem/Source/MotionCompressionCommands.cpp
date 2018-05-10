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
#include "MotionCompressionCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/MotionManager.h>
#include <MCore/Source/Compare.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/SkeletalSubMotion.h>
#include <EMotionFX/Source/WaveletSkeletalMotion.h>
#include <MCore/Source/FileSystem.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/ExporterFileProcessor.h>
#include <MCore/Source/StringConversions.h>

namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandKeyframeCompressMotion
    //--------------------------------------------------------------------------------

    // constructor
    CommandKeyframeCompressMotion::CommandKeyframeCompressMotion(MCore::Command* orgCommand)
        : MCore::Command("KeyframeCompressMotion", orgCommand)
    {
    }


    // destructor
    CommandKeyframeCompressMotion::~CommandKeyframeCompressMotion()
    {
    }


    // optimize a skeletal motion
    EMotionFX::Motion* CommandKeyframeCompressMotion::OptimizeSkeletalMotion(EMotionFX::SkeletalMotion* motion, bool optimizePosition, bool optimizeRotation, bool optimizeScale, float maxPositionError, float maxRotationError, float maxScaleError, Statistics* outStatistics, bool removeFromMotionManager)
    {
        // check if filename exists
        if (motion->GetFileNameString().empty())
        {
            return nullptr;
        }

        // clone the motion
        EMotionFX::Importer::SkeletalMotionSettings settings;
        settings.mForceLoading = true;

        EMotionFX::SkeletalMotion* optimizedMotion = (EMotionFX::SkeletalMotion*)EMotionFX::GetImporter().LoadSkeletalMotion(motion->GetFileName(), &settings);
        optimizedMotion->CreateDefaultPlayBackInfo();

        // remove the motion from the motion manager if flag is set
        if (removeFromMotionManager)
        {
            EMotionFX::GetMotionManager().RemoveMotionByID(optimizedMotion->GetID(), false);
        }

        // perform optimization
        const uint32 numSubMotions = motion->GetNumSubMotions();

        // iterate trough all submotions and optimize using the chosen settings
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            // get the submotion
            EMotionFX::SkeletalSubMotion* subMotionOptimized    = optimizedMotion->GetSubMotion(i);
            EMotionFX::SkeletalSubMotion* subMotion             = motion->GetSubMotion(i);

            if (subMotion == nullptr || subMotionOptimized == nullptr)
            {
                continue;
            }

            // optimize the rot scale keytrack
            if (subMotion->GetPosTrack() && optimizePosition == false)
            {
                outStatistics->mNumOriginalKeyframes    += subMotion->GetPosTrack()->GetNumKeys();
                outStatistics->mNumCompressedKeyframes  += subMotion->GetPosTrack()->GetNumKeys();
            }

            // optimize the position keytrack
            if (optimizePosition && subMotion->GetPosTrack() && subMotionOptimized->GetPosTrack())
            {
                uint32 numPosKeys       = subMotion->GetPosTrack()->GetNumKeys();
                uint32 numPosRemoved    = subMotionOptimized->GetPosTrack()->Optimize(maxPositionError);
                outStatistics->mNumOriginalKeyframes += numPosKeys;
                outStatistics->mNumCompressedKeyframes  += numPosRemoved;
            }

            // optimize the rotation keytrack
            if (subMotion->GetRotTrack() && optimizeRotation == false)
            {
                outStatistics->mNumOriginalKeyframes    += subMotion->GetRotTrack()->GetNumKeys();
                outStatistics->mNumCompressedKeyframes  += subMotion->GetRotTrack()->GetNumKeys();
            }

            // optimize the rotation keytrack
            if (optimizeRotation && subMotion->GetRotTrack() && subMotionOptimized->GetRotTrack())
            {
                uint32 numRotKeys       = subMotion->GetRotTrack()->GetNumKeys();
                uint32 numRotRemoved    = subMotionOptimized->GetRotTrack()->Optimize(maxRotationError);
                outStatistics->mNumOriginalKeyframes    += numRotKeys;
                outStatistics->mNumCompressedKeyframes  += numRotRemoved;
            }

        #ifndef EMFX_SCALE_DISABLED
            // optimize the rot scale keytrack
            if (subMotion->GetScaleTrack() && optimizeScale == false)
            {
                outStatistics->mNumOriginalKeyframes    += subMotion->GetScaleTrack()->GetNumKeys();
                outStatistics->mNumCompressedKeyframes  += subMotion->GetScaleTrack()->GetNumKeys();
            }

            // optimize the scale keytrack
            if (optimizeScale && subMotion->GetScaleTrack() && subMotionOptimized->GetScaleTrack())
            {
                uint32 numScaleKeys     = subMotion->GetScaleTrack()->GetNumKeys();
                uint32 numScaleRemoved  = subMotionOptimized->GetScaleTrack()->Optimize(maxScaleError);
                outStatistics->mNumOriginalKeyframes    += numScaleKeys;
                outStatistics->mNumCompressedKeyframes  += numScaleRemoved;
            }
        #endif
        }

        // return the compressed motion
        return optimizedMotion;
    }

    /*
    // optimizes a morph motion.
    Motion* CommandKeyframeCompressMotion::OptimizeMorphMotion(EMotionFX::MorphMotion* motion, bool optimize, float maxError, Statistics* outStatistics, bool removeFromMotionManager)
    {
        // check if filename exists
        if (motion->GetFileNameString().GetIsEmpty())
            return nullptr;

        // clone the motion
        Importer::MorphMotionSettings settings;
        settings.mForceLoading = true;

        MorphMotion* optimizedMotion = new MorphMotion( filename.AsChar() );
        optimizedMotion->CreateDefaultPlayBackInfo();

        // remove the motion from the motion manager if flag is set
        if (removeFromMotionManager)
            EMotionFX::GetMotionManager().RemoveMotionByID( optimizedMotion->GetID(), false );

        // get the number of submotions and iterate through them
        const uint32 numSubMotions = motion->GetNumSubMotions();
        for (uint32 i=0; i<numSubMotions; ++i)
        {
            // get the submotions
            MorphSubMotion* subMotion           = motion->GetSubMotion(i);
            MorphSubMotion* subMotionPreview    = optimizedMotion->GetSubMotion(i);

            if (optimize)
            {
                outStatistics->mNumOriginalKeyframes    = subMotion->GetKeyTrack()->GetNumKeys();
                outStatistics->mNumCompressedKeyframes  = subMotionPreview->GetKeyTrack()->Optimize( maxError );
            }
            else
            {
                outStatistics->mNumOriginalKeyframes    = subMotion->GetKeyTrack()->GetNumKeys();
                outStatistics->mNumCompressedKeyframes  = subMotion->GetKeyTrack()->GetNumKeys();
            }
        }

        // return the compressed motion
        return optimizedMotion;
    }
    */

    // execute
    bool CommandKeyframeCompressMotion::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and the corresponding motion pointer
        int32               motionID    = parameters.GetValueAsInt("motionID", this);
        EMotionFX::Motion*  motion      = EMotionFX::GetMotionManager().FindMotionByID(motionID);

        // check if the motion with the given id exists
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot compress motion. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        EMotionFX::Motion*  optimizedMotion = nullptr;
        Statistics          outStatistics;

        // handle skeletal motions
        if (motion->GetType() == EMotionFX::SkeletalMotion::TYPE_ID)
        {
            // declare compression parameters
            bool    optimizePosition    = false;
            bool    optimizeRotation    = false;
            bool    optimizeScale       = false;
            float   maxPositionError    = 0.0f;
            float   maxRotationError    = 0.0f;
            float   maxScaleError       = 0.0f;

            // check if position should be optimized
            if (parameters.CheckIfHasParameter("optimizePosition"))
            {
                optimizePosition = parameters.GetValueAsBool("optimizePosition", this);
            }

            // check if rotation should be optimized
            if (parameters.CheckIfHasParameter("optimizeRotation"))
            {
                optimizeRotation = parameters.GetValueAsBool("optimizeRotation", this);
            }

            // check if scale should be optimized
            if (parameters.CheckIfHasParameter("optimizeScale"))
            {
                optimizeScale = parameters.GetValueAsBool("optimizeScale", this);
            }

            // get max position error
            if (optimizePosition && parameters.CheckIfHasParameter("maxPositionError"))
            {
                maxPositionError = parameters.GetValueAsFloat("maxPositionError", this);
            }

            // get max rotation error
            if (optimizeRotation && parameters.CheckIfHasParameter("maxRotationError"))
            {
                maxRotationError = parameters.GetValueAsFloat("maxRotationError", this);
            }

            // get max scale error
            if (optimizeScale && parameters.CheckIfHasParameter("maxScaleError"))
            {
                maxScaleError = parameters.GetValueAsFloat("maxScaleError", this);
            }

            // optimize the motion
            optimizedMotion = OptimizeSkeletalMotion((EMotionFX::SkeletalMotion*)motion, optimizePosition, optimizeRotation, optimizeScale, maxPositionError, maxRotationError, maxScaleError, &outStatistics, false);
        }
        /*  else if (motion->GetType() == MorphMotion::TYPE_ID) // handle morph motions
            {
                // declare optimization variables
                bool    optimize = false;
                float   maxError = 0.0f;

                // check if morph motion should be optimized
                if (parameters.CheckIfHasParameter("optimize"))
                    optimize = parameters.GetValueAsBool("optimize", this);

                // get max error
                if (optimize && parameters.CheckIfHasParameter("maxError"))
                    maxError = parameters.GetValueAsFloat("maxError", this);

                // optimize the motion
                optimizedMotion = OptimizeMorphMotion( (MorphMotion*)motion, optimize, maxError, &outStatistics, false );
            }   */
        else // handle unsupported motion types
        {
            outResult = AZStd::string::format("Cannot compress motion. Unsupported motion type: %s", motion->GetTypeString());
            return false;
        }

        // add motion to the motion manager
        if (optimizedMotion)
        {
            // get the index of the original motion (position in the motion manager array)
            const uint32 optimizedMotionID = optimizedMotion->GetID();

            // remove the original motion
            if (motion)
            {
                motion->Destroy();
                motion = nullptr;
            }
            //      EMotionFX::GetMotionManager().RemoveMotionByID( motion->GetID(), false );

            GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", optimizedMotionID).c_str(), outResult);
            GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -motionIndex %i", EMotionFX::GetMotionManager().FindMotionIndexByID(optimizedMotionID)).c_str(), outResult);

            // set the outresult to the id of the compressed motion
            AZStd::to_string(outResult, optimizedMotionID);

            // return true, if command has been executed successfully
            return true;
        }

        return false;
    }


    // undo the command
    bool CommandKeyframeCompressMotion::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandKeyframeCompressMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(9);
        GetSyntax().AddRequiredParameter("motionID",         "The id of the motion to compress.",                                                MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("optimizePosition", "The optimizePosition flag indicates if the position should be optimized or not.",  MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "false");
        GetSyntax().AddParameter("optimizeRotation", "The optimizeRotation flag indicates if the rotation should be optimized or not.",  MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "false");
        GetSyntax().AddParameter("optimizeScale",    "The optimizeScale flag indicates if the scale should be optimized or not.",        MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "false");
        GetSyntax().AddParameter("optimize",         "The optimize flag indicates if the morph motion should be optimized or not.",      MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "false");
        GetSyntax().AddParameter("maxPositionError", "The maximum position error for keyframe optimization of the motion.",              MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("maxRotationError", "The maximum rotation error for keyframe optimization of the motion.",              MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("maxScaleError",    "The maximum scale error for keyframe optimization of the motion.",                 MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("maxError",         "The maximum error for keyframe optimization of the morph motion.",                 MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
    }


    // get the description
    const char* CommandKeyframeCompressMotion::GetDescription() const
    {
        return "This command can be used to compress the given motion using keyframe optimization.";
    }


    //--------------------------------------------------------------------------------
    // CommandWaveletCompressMotion
    //--------------------------------------------------------------------------------


    // constructor
    CommandWaveletCompressMotion::CommandWaveletCompressMotion(MCore::Command* orgCommand)
        : MCore::Command("WaveletCompressMotion", orgCommand)
    {
    }


    // destructor
    CommandWaveletCompressMotion::~CommandWaveletCompressMotion()
    {
    }


    // compress the skeletal motion
    EMotionFX::Motion* CommandWaveletCompressMotion::WaveletCompressSkeletalMotion(EMotionFX::SkeletalMotion* motion, uint32 waveletType, float positionQuality, float rotationQuality, float scaleQuality, uint32 samplesPerSecond, uint32 samplesPerChunk, bool removeFromMotionManager)
    {
        // initialize the wavelet motion, which performs the compression
        EMotionFX::WaveletSkeletalMotion::Settings settings;
        settings.mPositionQuality   = positionQuality;
        settings.mRotationQuality   = rotationQuality;
        EMFX_SCALECODE(settings.mScaleQuality = scaleQuality;
            )
        settings.mSamplesPerChunk   = samplesPerChunk;
        settings.mSamplesPerSecond  = samplesPerSecond;

        // get the wavelet type
        switch (waveletType)
        {
        case 0:
            settings.mWavelet = EMotionFX::WaveletSkeletalMotion::WAVELET_HAAR;
            break;
        case 1:
            settings.mWavelet = EMotionFX::WaveletSkeletalMotion::WAVELET_DAUB4;
            break;
        case 2:
            settings.mWavelet = EMotionFX::WaveletSkeletalMotion::WAVELET_CDF97;
            break;
        default:
            settings.mWavelet = EMotionFX::WaveletSkeletalMotion::WAVELET_HAAR;
        }

        EMotionFX::WaveletSkeletalMotion* optimizedMotion = EMotionFX::WaveletSkeletalMotion::Create(motion->GetFileName());
        optimizedMotion->CreateDefaultPlayBackInfo();

        if (removeFromMotionManager)
        {
            EMotionFX::GetMotionManager().RemoveMotion(optimizedMotion, false);
        }

        // init the motion with the compression settings
        optimizedMotion->Init(motion, &settings);

        // return the motion
        return optimizedMotion;
    }


    // execute
    bool CommandWaveletCompressMotion::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and the corresponding motion pointer
        int32               motionID        = parameters.GetValueAsInt("motionID", this);
        EMotionFX::Motion*  motion          = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        EMotionFX::Motion*  optimizedMotion = nullptr;

        // check if the motion with the given id exists
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot compress motion. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // handle skeletal motions
        if (motion->GetType() == EMotionFX::SkeletalMotion::TYPE_ID)
        {
            // declare compression parameters
            uint32  waveletType         = 0;
            float   positionQuality     = 100.0f;
            float   rotationQuality     = 100.0f;
            float   scaleQuality        = 100.0f;
            uint32  samplesPerSecond    = 12;
            uint32  samplesPerChunk     = 4;

            // check if wavelet type is specified
            if (parameters.CheckIfHasParameter("waveletType"))
            {
                waveletType = parameters.GetValueAsInt("waveletType", this);
            }

            // check if position quality is specified
            if (parameters.CheckIfHasParameter("positionQuality"))
            {
                positionQuality = parameters.GetValueAsFloat("positionQuality", this);
            }

            // check if rotation quality is specified
            if (parameters.CheckIfHasParameter("rotationQuality"))
            {
                rotationQuality = parameters.GetValueAsFloat("rotationQuality", this);
            }

            // check if scale quality is specified
            if (parameters.CheckIfHasParameter("scaleQuality"))
            {
                scaleQuality = parameters.GetValueAsFloat("scaleQuality", this);
            }

            // get samples per second
            if (parameters.CheckIfHasParameter("samplesPerSecond"))
            {
                samplesPerSecond = parameters.GetValueAsInt("samplesPerSecond", this);
            }

            // get samples per chunk
            if (parameters.CheckIfHasParameter("samplesPerChunk"))
            {
                samplesPerChunk = parameters.GetValueAsInt("samplesPerChunk", this);
            }

            // optimize the motion
            optimizedMotion = WaveletCompressSkeletalMotion((EMotionFX::SkeletalMotion*)motion, waveletType, positionQuality, rotationQuality, scaleQuality, samplesPerSecond, samplesPerChunk, false);
        }
        else // handle unsupported motion types
        {
            outResult = AZStd::string::format("Cannot compress wavelet motion. Only skeletal motions can be compressed.");
            return false;
        }

        // add motion to the motion manager
        if (optimizedMotion)
        {
            // get the index of the original motion (position in the motion manager array)
            const uint32 optimizedMotionID = optimizedMotion->GetID();

            // remove the original motion
            //      delete motion;
            //      motion = nullptr;

            GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", optimizedMotionID).c_str(), outResult);
            GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -motionIndex %i", EMotionFX::GetMotionManager().FindMotionIndexByID(optimizedMotionID)).c_str(), outResult);

            // set the outresult to the id of the compressed motion
            AZStd::to_string(outResult, optimizedMotionID);

            // return true, if command has been executed successfully
            return true;
        }

        return false;
    }


    // undo the command
    bool CommandWaveletCompressMotion::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandWaveletCompressMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(7);
        GetSyntax().AddRequiredParameter("motionID",         "The id of the motion to compress.",                                    MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("waveletType",      "The wavelet type used for compressing the motion.",                    MCore::CommandSyntax::PARAMTYPE_INT,    "0");
        GetSyntax().AddParameter("positionQuality",  "The position quality used for the wavelet compression.",               MCore::CommandSyntax::PARAMTYPE_FLOAT,  "100.0");
        GetSyntax().AddParameter("rotationQuality",  "The rotation quality used for the wavelet compression.",               MCore::CommandSyntax::PARAMTYPE_FLOAT,  "100.0");
        GetSyntax().AddParameter("scaleQuality",     "The scale quality used for the wavelet compression.",                  MCore::CommandSyntax::PARAMTYPE_FLOAT,  "100.0");
        GetSyntax().AddParameter("samplesPerSecond", "The number of samples per second used for the wavelet compression.",   MCore::CommandSyntax::PARAMTYPE_INT,    "12");
        GetSyntax().AddParameter("samplesPerChunk",  "The number of samples per chunk used for the wavelet compression.",    MCore::CommandSyntax::PARAMTYPE_INT,    "4");
    }


    // get the description
    const char* CommandWaveletCompressMotion::GetDescription() const
    {
        return "This command can be used to compress a skeletal motion using wavelet compression.";
    }
} // namespace CommandSystem
