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
#include "ImporterCommands.h"
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Attachment.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include "CommandManager.h"
#include <AzFramework/API/ApplicationAPI.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // ImportActor
    //--------------------------------------------------------------------------------

    // constructor
    CommandImportActor::CommandImportActor(MCore::Command* orgCommand)
        : MCore::Command("ImportActor", orgCommand)
    {
        mPreviouslyUsedID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandImportActor::~CommandImportActor()
    {
    }


    // execute
    bool CommandImportActor::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the actor id from the parameters
        uint32 actorID = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("actorID"))
        {
            actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);
            if (EMotionFX::GetActorManager().FindActorByID(actorID))
            {
                outResult.Format("Cannot import actor. Actor ID %i is already in use.", actorID);
                return false;
            }
        }

        // get the filename of the actor
        MCore::String filenameValue;
        parameters.GetValue("filename", "", &filenameValue);
        
        AZStd::string filename = filenameValue.AsChar();
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        // check if we have already loaded the actor
        EMotionFX::Actor* actorFromManager = EMotionFX::GetActorManager().FindActorByFileName(filename.c_str());
        if (actorFromManager)
        {
            outResult = MCore::String(actorFromManager->GetID());
            return true;
        }

        // init the settings
        EMotionFX::Importer::ActorSettings settings;

        // extract default values from the command syntax automatically, if they aren't specified explicitly
        settings.mLoadMeshes                    = parameters.GetValueAsBool("loadMeshes",           this);
        settings.mLoadTangents                  = parameters.GetValueAsBool("loadTangents",         this);
        settings.mAutoGenTangents               = parameters.GetValueAsBool("autoGenTangents",      this);
        settings.mLoadLimits                    = parameters.GetValueAsBool("loadLimits",           this);
        settings.mLoadGeometryLODs              = parameters.GetValueAsBool("loadGeomLods",         this);
        settings.mLoadMorphTargets              = parameters.GetValueAsBool("loadMorphTargets",     this);
        settings.mLoadCollisionMeshes           = parameters.GetValueAsBool("loadCollisionMeshes",  this);
        settings.mLoadStandardMaterialLayers    = parameters.GetValueAsBool("loadMaterialLayers",   this);
        settings.mLoadSkinningInfo              = parameters.GetValueAsBool("loadSkinningInfo",     this);
        settings.mLoadSkeletalLODs              = parameters.GetValueAsBool("loadSkeletalLODs",     this);
        settings.mDualQuatSkinning              = parameters.GetValueAsBool("dualQuatSkinning",     this);

        // try to load the actor
        EMotionFX::Actor* actor = EMotionFX::GetImporter().LoadActor(filename.c_str(), &settings);
        if (actor == nullptr)
        {
            outResult.Format("Failed to load actor from '%s'. File may not exist at this path or may have incorrect permissions", filename.c_str());
            return false;
        }

        // set the actor id in case we have specified it as parameter
        if (actorID != MCORE_INVALIDINDEX32)
        {
            actor->SetID(actorID);
        }

        // in case we are in a redo call assign the previously used id
        if (mPreviouslyUsedID != MCORE_INVALIDINDEX32)
        {
            actor->SetID(mPreviouslyUsedID);
        }
        mPreviouslyUsedID = actor->GetID();

        // select the actor automatically
        if (parameters.GetValueAsBool("autoSelect", this))
        {
            GetCommandManager()->ExecuteCommandInsideCommand(MCore::String().Format("Select -actorID %i", actor->GetID()).AsChar(), outResult);
        }

        // update our render actors
        GetCommandManager()->ExecuteCommandInsideCommand("UpdateRenderActors", outResult);

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // return the id of the newly created actor
        outResult = MCore::String(actor->GetID());
        return true;
    }


    // undo the command
    bool CommandImportActor::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the actor ID
        uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);
        if (actorID == MCORE_INVALIDINDEX32)
        {
            actorID = mPreviouslyUsedID;
        }

        // check if we have to unselect the actors created by this command
        const bool unselect = parameters.GetValueAsBool("autoSelect", this);
        if (unselect)
        {
            GetCommandManager()->ExecuteCommandInsideCommand(MCore::String().Format("Unselect -actorID %i", actorID).AsChar(), outResult);
        }

        // find the actor based on the given id
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult.Format("Cannot remove actor. Actor ID %i is not valid.", actorID);
            return false;
        }

        actor->Destroy();

        // update our render actors
        MCore::String updateRenderActorsResult;
        GetCommandManager()->ExecuteCommandInsideCommand("UpdateRenderActors", updateRenderActorsResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return true;
    }


    // init the syntax of the command
    void CommandImportActor::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(17);
        GetSyntax().AddRequiredParameter("filename", "The filename of the actor file to load.", MCore::CommandSyntax::PARAMTYPE_STRING);

        // optional parameters
        GetSyntax().AddParameter("actorID",             "The identification number to give the actor. In case this parameter is not specified the actor manager will automatically set a unique id to the actor.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("loadMeshes",          "Load 3D mesh geometry or not.",                                        MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("loadTangents",        "Load vertex tangents or not.",                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("loadLimits",          "Load node limits or not.",                                             MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("loadGeomLods",        "Load geometry LOD levels or not.",                                     MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("loadMorphTargets",    "Load morph targets or not.",                                           MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("loadCollisionMeshes", "Load collision meshes or not.",                                        MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("loadSkeletalLODs",    "Load skeletal LOD levels.",                                            MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("dualQuatSkinning",    "Enable software skinning using dual quaternions.",                     MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "No");
        GetSyntax().AddParameter("loadSkinningInfo",    "Load skinning information (bone influences) or not.",                  MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("loadMaterialLayers",  "Load standard material layers (textures) or not.",                     MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("autoGenTangents",     "Automatically generate tangents when they are not present or not.",    MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("autoSelect",          "Set the current selected actor to the newly loaded actor or leave selection as it was before.",        MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
    }


    // get the description
    const char* CommandImportActor::GetDescription() const
    {
        return "This command can be used to import EMotion FX actor files. Actor files can represent 3D objects and characters. They can for example contain full 3D character meshes linked to a hierarchy of bones or a complete game level or just a hierarchy of objects.";
    }

    //--------------------------------------------------------------------------------
    // ImportMotion
    //--------------------------------------------------------------------------------

    // constructor
    CommandImportMotion::CommandImportMotion(MCore::Command* orgCommand)
        : MCore::Command("ImportMotion", orgCommand)
    {
        mOldMotionID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandImportMotion::~CommandImportMotion()
    {
    }


    // execute
    bool CommandImportMotion::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the motion id from the parameters
        uint32 motionID = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("motionID"))
        {
            motionID = parameters.GetValueAsInt("motionID", MCORE_INVALIDINDEX32);
            if (EMotionFX::GetMotionManager().FindMotionByID(motionID))
            {
                outResult.Format("Cannot import motion. Motion ID %i is already in use.", motionID);
                return false;
            }
        }

        // get the filename
        MCore::String filename;
        parameters.GetValue("filename", "", &filename);
        
        AZStd::string normalizedFilename = filename.AsChar();
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, normalizedFilename);
        filename = normalizedFilename.c_str();

        const MCore::String extension = filename.ExtractFileExtension();

        // check if we have already loaded the motion
        if (EMotionFX::GetMotionManager().FindMotionByFileName(filename))
        {
            outResult.Format("Motion '%s' has already been loaded. Skipping.", filename.AsChar());
            return true;
        }

        // init the motion pointer
        EMotionFX::Motion* motion = nullptr;

        // check if we are dealing with a skeletal motion
        if (extension.CheckIfIsEqualNoCase("motion"))
        {
            // init the settings
            EMotionFX::Importer::SkeletalMotionSettings settings;

            // extract default values from the command syntax automatically, if they aren't specified explicitly
            settings.mLoadMotionEvents          = parameters.GetValueAsBool("loadMotionEvents",     this);
            settings.mAutoRegisterEvents        = parameters.GetValueAsBool("autoRegisterEvents",   this);

            // try to load the actor
            motion = EMotionFX::GetImporter().LoadSkeletalMotion(filename.AsChar(), &settings);
        }

        // check if the motion is invalid
        if (motion == nullptr)
        {
            outResult.Format("Failed to load motion from file '%s'.", filename.AsChar());
            return false;
        }

        // set the motion id in case we have specified it as parameter
        if (motionID != MCORE_INVALIDINDEX32)
        {
            motion->SetID(motionID);
        }

        // in case we are in a redo call assign the previously used id
        if (mOldMotionID != MCORE_INVALIDINDEX32)
        {
            motion->SetID(mOldMotionID);
        }
        mOldMotionID = motion->GetID();
        mOldFileName = motion->GetFileName();

        // set the motion name
        MCore::String motionName = filename.ExtractFileName();
        motionName.RemoveFileExtension();
        motion->SetName(motionName.AsChar());

        // create the default playback info in case there is none yet
        motion->CreateDefaultPlayBackInfo();

        // select the motion automatically
        if (parameters.GetValueAsBool("autoSelect", this))
        {
            GetCommandManager()->GetCurrentSelection().AddMotion(motion);
        }

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // reset the dirty flag
        motion->SetDirtyFlag(false);
        return true;
    }


    // undo the command
    bool CommandImportMotion::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        // execute the group command
        MCore::String commandString;
        commandString.Format("RemoveMotion -filename \"%s\"", mOldFileName.AsChar());
        bool result = GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandImportMotion::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(7);
        GetSyntax().AddRequiredParameter("filename", "The filename of the motion file to load.", MCore::CommandSyntax::PARAMTYPE_STRING);

        // optional parameters
        GetSyntax().AddParameter("motionID",            "The identification number to give the motion. In case this parameter is not specified the motion will automatically get a unique id.",                 MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("loadMotionEvents",    "Set to false if you wish to disable loading of motion events.",                                                                                        MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("autoRegisterEvents",  "Set to true if you want to automatically register new motion event types.",                                                                            MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("autoSelect",          "Set the current selected actor to the newly loaded actor or leave selection as it was before.",                                                        MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "No");
    }


    // get the description
    const char* CommandImportMotion::GetDescription() const
    {
        return "This command can be used to import EMotion FX motion files. The command can load skeletal as well as morph target motions.";
    }

} // namespace CommandSystem
