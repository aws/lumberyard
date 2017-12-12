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
#include "ActorInstanceCommands.h"
#include <EMotionFX/Source/Attachment.h>
#include <EMotionFX/Source/ActorManager.h>
#include "CommandManager.h"


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CreateActorInstance
    //--------------------------------------------------------------------------------

    // constructor
    CommandCreateActorInstance::CommandCreateActorInstance(MCore::Command* orgCommand)
        : MCore::Command("CreateActorInstance", orgCommand)
    {
        mPreviouslyUsedID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandCreateActorInstance::~CommandCreateActorInstance()
    {
    }


    // execute the command
    bool CommandCreateActorInstance::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        EMotionFX::Actor* actor;
        if (parameters.CheckIfHasParameter("actorID"))
        {
            const uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

            // find the actor based on the given id
            actor = EMotionFX::GetActorManager().FindActorByID(actorID);
            if (actor == nullptr)
            {
                outResult.Format("Cannot create actor instance. Actor ID %i is not valid.", actorID);
                return false;
            }
        }
        else
        {
            // check if there is any actor selected at all
            SelectionList& selection = GetCommandManager()->GetCurrentSelection();
            if (selection.GetNumSelectedActors() == 0)
            {
                outResult = "No actor has been selected, please select one first.";
                return false;
            }

            // get the first selected actor
            actor = selection.GetActor(0);
        }

        // get the other parameter info
        //const float posX          = parameters.GetValueAsFloat("xPos",    this);
        //const float posY          = parameters.GetValueAsFloat("yPos",    this);
        //const float posZ          = parameters.GetValueAsFloat("zPos",    this);
        const float scaleX          = parameters.GetValueAsFloat("xScale",  this);
        const float scaleY          = parameters.GetValueAsFloat("yScale",  this);
        const float scaleZ          = parameters.GetValueAsFloat("zScale",  this);
        const AZ::Vector4 rotV4     = parameters.GetValueAsVector4("rot",   this);

        // validate the scale values, don't allow zero or negative scaling
        if (MCore::InRange(scaleX, 0.0001f, 10000.0f) == false ||
            MCore::InRange(scaleY, 0.0001f, 10000.0f) == false ||
            MCore::InRange(scaleZ, 0.0001f, 10000.0f) == false)
        {
            outResult = "The scale values must be between 0.0001 and 10000.";
            return false;
        }

        // convert into vectors/quaternions
        //MCore::Vector3 pos(posX, posY, posZ);
        MCore::Vector3 scale(scaleX, scaleY, scaleZ);
        MCore::Quaternion rot(rotV4.GetX(), rotV4.GetY(), rotV4.GetZ(), rotV4.GetW());

        // check if we have to select the new actor instances created by this command automatically
        const bool select = parameters.GetValueAsBool("autoSelect", this);

        // create the instance
        EMotionFX::ActorInstance* newInstance = EMotionFX::ActorInstance::Create(actor);
        newInstance->UpdateVisualizeScale();

        // get the actor instance ID to give the actor instance
        uint32 actorInstanceID = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("actorInstanceID"))
        {
            actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);
            if (EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID))
            {
                outResult.Format("Cannot create actor instance. Actor instance ID %i is already in use.", actorInstanceID);
                return false;
            }
        }

        // set the actor instance id in case we have specified it as parameter
        if (actorInstanceID != MCORE_INVALIDINDEX32)
        {
            newInstance->SetID(actorInstanceID);
        }

        // in case of redoing the command set the previously used id
        if (mPreviouslyUsedID != MCORE_INVALIDINDEX32)
        {
            newInstance->SetID(mPreviouslyUsedID);
        }

        mPreviouslyUsedID = newInstance->GetID();

        // setup the position, rotation and scale
        MCore::Vector3 newPos = newInstance->GetLocalPosition();
        if (parameters.CheckIfHasParameter("xPos"))
        {
            newPos.x = parameters.GetValueAsFloat("xPos", this);
        }
        if (parameters.CheckIfHasParameter("yPos"))
        {
            newPos.y = parameters.GetValueAsFloat("yPos", this);
        }
        if (parameters.CheckIfHasParameter("zPos"))
        {
            newPos.z = parameters.GetValueAsFloat("zPos", this);
        }
        newInstance->SetLocalPosition(newPos);

        newInstance->SetLocalRotation(rot);
        newInstance->SetLocalScale(scale);

        // setup bounds update mode to mesh based if it has meshes
        if (newInstance->GetActor()->CheckIfHasMeshes(0))
        {
            newInstance->SetupAutoBoundsUpdate(0.0f, EMotionFX::ActorInstance::BOUNDS_MESH_BASED);
        }
        else
        {
            newInstance->SetupAutoBoundsUpdate(0.0f, EMotionFX::ActorInstance::BOUNDS_NODE_BASED);
        }

        // add the actor instance to the selection
        if (select)
        {
            GetCommandManager()->ExecuteCommandInsideCommand(MCore::String().Format("Select -actorInstanceID %i", newInstance->GetID()).AsChar(), outResult);

            if (EMotionFX::GetActorManager().GetNumActorInstances() == 1 && GetCommandManager()->GetLockSelection() == false)
            {
                GetCommandManager()->ExecuteCommandInsideCommand("ToggleLockSelection", outResult);
            }

            if (EMotionFX::GetActorManager().GetNumActorInstances() > 1 && GetCommandManager()->GetLockSelection())
            {
                GetCommandManager()->ExecuteCommandInsideCommand("ToggleLockSelection", outResult);
            }
        }

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // return the id of the newly created actor instance
        outResult = MCore::String(newInstance->GetID());
        return true;
    }


    // undo the command
    bool CommandCreateActorInstance::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // check if we have to unselect the actors created by this command
        const bool unselect = parameters.GetValueAsBool("autoSelect", this);

        // get the actor instance ID
        uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);
        if (actorInstanceID == MCORE_INVALIDINDEX32)
        {
            actorInstanceID = mPreviouslyUsedID;
        }

        // find the actor intance based on the given id
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            outResult.Format("Cannot undo create actor instance command. Actor instance ID %i is not valid.", actorInstanceID);
            return false;
        }

        // remove the actor instance from the selection
        if (unselect)
        {
            GetCommandManager()->ExecuteCommandInsideCommand(MCore::String().Format("Unselect -actorInstanceID %i", actorInstanceID).AsChar(), outResult);

            if (EMotionFX::GetActorManager().GetNumActorInstances() == 1 && GetCommandManager()->GetLockSelection() == false)
            {
                GetCommandManager()->ExecuteCommandInsideCommand("ToggleLockSelection", outResult);
            }

            if (EMotionFX::GetActorManager().GetNumActorInstances() > 1 && GetCommandManager()->GetLockSelection())
            {
                GetCommandManager()->ExecuteCommandInsideCommand("ToggleLockSelection", outResult);
            }
        }

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        // get rid of the actor instance
        if (actorInstance)
        {
            actorInstance->Destroy();
        }
        return true;
    }


    // init the syntax of the command
    void CommandCreateActorInstance::InitSyntax()
    {
        // optional parameters
        GetSyntax().ReserveParameters(13);
        GetSyntax().AddParameter("actorID",         "The identification number of the actor for which we want to create an actor instance.",                                                                                                            MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("actorInstanceID", "The actor instance identification number to give the new actor instance. In case this parameter is not specified the IDGenerator will automatically assign a unique ID to the actor instance.",    MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("xPos",            "The x-axis of the position of the instance.",                                                                                                                                                      MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("yPos",            "The y-axis of the position of the instance.",                                                                                                                                                      MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("zPos",            "The z-axis of the position of the instance.",                                                                                                                                                      MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("rot",             "The rotation of the actor instance.",                                                                                                                                                              MCore::CommandSyntax::PARAMTYPE_VECTOR4,    "0.0,0.0,0.0,1.0");
        GetSyntax().AddParameter("xScale",          "The x-axis scale of the instances.",                                                                                                                                                               MCore::CommandSyntax::PARAMTYPE_FLOAT,      "1.0");
        GetSyntax().AddParameter("yScale",          "The y-axis scale of the instances.",                                                                                                                                                               MCore::CommandSyntax::PARAMTYPE_FLOAT,      "1.0");
        GetSyntax().AddParameter("zScale",          "The z-axis scale of the instances.",                                                                                                                                                               MCore::CommandSyntax::PARAMTYPE_FLOAT,      "1.0");
        //GetSyntax().AddParameter("xSpacing",      "The x-axis spacing between the instances.",                                                                                                                                                        MCore::CommandSyntax::PARAMTYPE_FLOAT,      "5.0");
        //GetSyntax().AddParameter("ySpacing",      "The y-axis spacing between the instances.",                                                                                                                                                        MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        //GetSyntax().AddParameter("zSpacing",      "The z-axis spacing between the instances.",                                                                                                                                                        MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("autoSelect",      "Automatically add the the newly created actor instance to the selection.",                                                                                                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "Yes");
    }


    // get the description
    const char* CommandCreateActorInstance::GetDescription() const
    {
        return "This command can be used to create an actor instance from the selected Actor object.";
    }

    //--------------------------------------------------------------------------------
    // CommandAdjustActorInstance
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustActorInstance::CommandAdjustActorInstance(MCore::Command* orgCommand)
        : MCore::Command("AdjustActorInstance", orgCommand)
    {
    }


    // destructor
    CommandAdjustActorInstance::~CommandAdjustActorInstance()
    {
    }


    // execute
    bool CommandAdjustActorInstance::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);

        // find the actor intance based on the given id
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            outResult.Format("Cannot adjust actor instance. Actor instance ID %i is not valid.", actorInstanceID);
            return false;
        }

        // set the position
        if (parameters.CheckIfHasParameter("pos"))
        {
            MCore::Vector3 value    = parameters.GetValueAsVector3("pos", this);
            mOldPosition            = actorInstance->GetLocalPosition();
            actorInstance->SetLocalPosition(value);
        }

        // set the rotation
        if (parameters.CheckIfHasParameter("rot"))
        {
            AZ::Vector4 value   = parameters.GetValueAsVector4("rot", this);
            mOldRotation            = actorInstance->GetLocalRotation();
            actorInstance->SetLocalRotation(MCore::Quaternion(value.GetX(), value.GetY(), value.GetZ(), value.GetW()));
        }

        // set the scale
        if (parameters.CheckIfHasParameter("scale"))
        {
            MCore::Vector3 value    = parameters.GetValueAsVector3("scale", this);
            mOldScale               = actorInstance->GetLocalScale();
            actorInstance->SetLocalScale(value);
        }

        // set the LOD level
        if (parameters.CheckIfHasParameter("lodLevel"))
        {
            uint32 value    = parameters.GetValueAsInt("lodLevel", this);
            mOldLODLevel    = actorInstance->GetLODLevel();
            actorInstance->SetLODLevel(value);
        }

        // set the visibility flag
        if (parameters.CheckIfHasParameter("isVisible"))
        {
            bool value      = parameters.GetValueAsBool("isVisible", this);
            mOldIsVisible   = actorInstance->GetIsVisible();
            actorInstance->SetIsVisible(value);
        }

        // set the rendering flag
        if (parameters.CheckIfHasParameter("doRender"))
        {
            bool value      = parameters.GetValueAsBool("doRender", this);
            mOldDoRender    = actorInstance->GetRender();
            actorInstance->SetRender(value);
        }

        // set the attachment fast updating flag
        if (parameters.CheckIfHasParameter("attachmentFastUpdate"))
        {
            EMotionFX::Attachment* attachment = actorInstance->GetSelfAttachment();
            if (attachment)
            {
                const bool value            = parameters.GetValueAsBool("attachmentFastUpdate", this);
                mOldAttachmentFastUpdate    = attachment->GetAllowFastUpdates();
                attachment->SetAllowFastUpdates(value);
            }
        }

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandAdjustActorInstance::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);

        // find the actor intance based on the given id
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            outResult.Format("Cannot adjust actor instance. Actor instance ID %i is not valid.", actorInstanceID);
            return false;
        }

        // set the position
        if (parameters.CheckIfHasParameter("pos"))
        {
            actorInstance->SetLocalPosition(mOldPosition);
        }

        // set the rotation
        if (parameters.CheckIfHasParameter("rot"))
        {
            actorInstance->SetLocalRotation(mOldRotation);
        }

        // set the scale
        if (parameters.CheckIfHasParameter("scale"))
        {
            actorInstance->SetLocalScale(mOldScale);
        }

        // set the LOD level
        if (parameters.CheckIfHasParameter("lodLevel"))
        {
            actorInstance->SetLODLevel(mOldLODLevel);
        }

        // set the visibility flag
        if (parameters.CheckIfHasParameter("isVisible"))
        {
            actorInstance->SetIsVisible(mOldIsVisible);
        }

        // set the rendering flag
        if (parameters.CheckIfHasParameter("doRender"))
        {
            actorInstance->SetRender(mOldDoRender);
        }

        // set the attachment fast updating flag
        if (parameters.CheckIfHasParameter("attachmentFastUpdate"))
        {
            EMotionFX::Attachment* attachment = actorInstance->GetSelfAttachment();
            if (attachment)
            {
                attachment->SetAllowFastUpdates(mOldAttachmentFastUpdate);
            }
        }

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return true;
    }


    // init the syntax of the command
    void CommandAdjustActorInstance::InitSyntax()
    {
        GetSyntax().ReserveParameters(8);
        GetSyntax().AddRequiredParameter("actorInstanceID", "The actor instance identification number of the actor instance to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("pos", "The position of the actor instance.", MCore::CommandSyntax::PARAMTYPE_VECTOR3, "0.0,0.0,0.0");
        GetSyntax().AddParameter("rot", "The rotation of the actor instance.", MCore::CommandSyntax::PARAMTYPE_VECTOR4, "0.0,0.0,0.0,1.0");
        GetSyntax().AddParameter("scale", "The scale of the actor instance.", MCore::CommandSyntax::PARAMTYPE_VECTOR3, "0.0,0.0,0.0");
        GetSyntax().AddParameter("lodLevel", "The LOD level. Values higher than [GetNumLODLevels()-1] will be clamped to the maximum LOD.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("isVisible", "The visibility flag. In case of true the actor instance is getting updated, in case of false the OnUpdate() will be skipped.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("doRender", "This flag specifies if the actor instance is getting rendered or not. In case of true the actor instance is rendered, in case of false it will not be visible.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");
        GetSyntax().AddParameter("attachmentFastUpdate", "This flag specifies if the actor instance is allowed to get updated fastly in case it is an attachment. In case of true the actor instance attachment is getting updated in the fast mode, in case of false it will be updated in the normal way.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "No");
    }


    // get the description
    const char* CommandAdjustActorInstance::GetDescription() const
    {
        return "This command can be used to adjust the attributes of the currently selected actor instance.";
    }


    //--------------------------------------------------------------------------------
    // CommandRemoveActorInstance
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveActorInstance::CommandRemoveActorInstance(MCore::Command* orgCommand)
        : MCore::Command("RemoveActorInstance", orgCommand)
    {
    }


    // destructor
    CommandRemoveActorInstance::~CommandRemoveActorInstance()
    {
    }


    // execute
    bool CommandRemoveActorInstance::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);

        // find the actor intance based on the given id
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            outResult.Format("Cannot remove actor instance. Actor instance ID %i is not valid.", actorInstanceID);
            return false;
        }

        // store the old values before removing the instance
        mOldPosition            = actorInstance->GetLocalPosition();
        mOldRotation            = actorInstance->GetLocalRotation();
        mOldScale               = actorInstance->GetLocalScale();
        mOldLODLevel            = actorInstance->GetLODLevel();
        mOldIsVisible           = actorInstance->GetIsVisible();
        mOldDoRender            = actorInstance->GetRender();

        // fast update flag for the self attachment
        EMotionFX::Attachment* attachment = actorInstance->GetSelfAttachment();
        if (attachment)
        {
            mOldAttachmentFastUpdate = attachment->GetAllowFastUpdates();
        }
        else
        {
            mOldAttachmentFastUpdate = false;
        }

        // remove the actor instance from the selection
        if (GetCommandManager()->GetLockSelection())
        {
            GetCommandManager()->ExecuteCommandInsideCommand("ToggleLockSelection", outResult);
        }

        // get the id from the corresponding actor and save it for undo
        EMotionFX::Actor* actor = actorInstance->GetActor();
        mOldActorID = actor->GetID();

        // get rid of the actor instance
        if (actorInstance)
        {
            actorInstance->Destroy();
        }

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandRemoveActorInstance::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the actor instance ID and check if it is still available
        const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);
        if (EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID))
        {
            outResult.Format("Cannot undo remove actor instance. Actor instance ID %i is already in use.", actorInstanceID);
            return false;
        }

        // create command group for creating/adjusting the actor instance
        MCore::String commandString;
        MCore::CommandGroup commandGroup("Undo remove actor instance", 2);

        commandString.Format("CreateActorInstance -actorID %i -actorInstanceID %i", mOldActorID, actorInstanceID);
        commandGroup.AddCommandString(commandString.AsChar());

        commandString.Format("AdjustActorInstance -actorInstanceID %i -pos \"%s\" -rot \"%s\" -scale \"%s\" -lodLevel %d -isVisible \"%s\" -doRender \"%s\" -attachmentFastUpdate \"%s\"",
            actorInstanceID,
            MCore::String(mOldPosition).AsChar(),
            MCore::String(AZ::Vector4(mOldRotation.x, mOldRotation.y, mOldRotation.z, mOldRotation.w)).AsChar(),
            MCore::String(mOldScale).AsChar(),
            mOldLODLevel,
            mOldIsVisible ? "true" : "false",
            mOldDoRender ? "true" : "false",
            mOldAttachmentFastUpdate ? "true" : "false"
            );
        commandGroup.AddCommandString(commandString.AsChar());

        // execute the command group
        bool result = GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandRemoveActorInstance::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("actorInstanceID", "The actor instance identification number of the actor instance to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandRemoveActorInstance::GetDescription() const
    {
        return "This command can be used to ";
    }

    //-------------------------------------------------------------------------------------
    // Helper Functions
    //-------------------------------------------------------------------------------------
    void CloneActorInstance(EMotionFX::ActorInstance* actorInstance, MCore::CommandGroup* commandGroup)
    {
        // return if actorinstance was not set
        if (actorInstance == nullptr)
        {
            MCore::LogError("Actor instance was not set. Unable to generate new instance.");
            return;
        }

        // get the transformation values
        const MCore::Vector3& pos           = actorInstance->GetLocalPosition();
        const MCore::Vector3& scale         = actorInstance->GetLocalScale();
        const MCore::Quaternion& rotQuat    = actorInstance->GetLocalRotation();
        const AZ::Vector4 rot           = AZ::Vector4(rotQuat.x, rotQuat.y, rotQuat.z, rotQuat.w);

        // create the command
        MCore::String command;
        command.Format("CreateActorInstance -actorID %i -xPos %f -yPos %f -zPos %f -xScale %f -yScale %f -zScale %f -rot \"%s\"", actorInstance->GetActor()->GetID(), pos.x, pos.y, pos.z, scale.x, scale.y, scale.z, MCore::String(rot).AsChar());

        // execute the command or add it to the given command group
        if (commandGroup == nullptr)
        {
            MCore::String outResult;
            if (GetCommandManager()->ExecuteCommand(command.AsChar(), outResult) == false)
            {
                MCore::LogError(outResult.AsChar());
            }
        }
        else
        {
            commandGroup->AddCommandString(command);
        }
    }


    // slots for cloning all selected actor instances
    void CloneSelectedActorInstances()
    {
        // get the selection and number of selected actor instances
        const SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selection.GetNumSelectedActorInstances();

        // create the command group
        MCore::CommandGroup commandGroup("Clone actor instances", numActorInstances);

        // unselect all actors and actor instances
        commandGroup.AddCommandString("Unselect -actorInstanceID SELECT_ALL -actorID SELECT_ALL");

        // iterate over the selected instances and clone them
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the current actor instance
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (actorInstance == nullptr)
            {
                continue;
            }

            // add the clone command
            CloneActorInstance(actorInstance, &commandGroup);
        }

        // execute the commandgroup
        MCore::String outResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult, true) == false)
        {
            MCore::LogError(outResult.AsChar());
        }
    }


    // call reset to bind pose
    void ResetToBindPose()
    {
        // execute the command
        MCore::String outResult;
        if (GetCommandManager()->ExecuteCommand("ResetToBindPose", outResult) == false)
        {
            if (outResult.GetIsEmpty() == false)
            {
                MCore::LogError(outResult.AsChar());
            }
        }
    }


    // slot for removing all selected actor instances
    void RemoveSelectedActorInstances()
    {
        // get the selection and number of selected actor instances
        const SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selection.GetNumSelectedActorInstances();

        // create the command group
        MCore::CommandGroup commandGroup("Remove actor instances", numActorInstances);
        MCore::String tempString;

        // iterate over the selected instances and clone them
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the current actor instance
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (actorInstance == nullptr)
            {
                continue;
            }

            tempString.Format("RemoveActorInstance -actorInstanceID %i", actorInstance->GetID());
            commandGroup.AddCommandString(tempString.AsChar());
        }

        // execute the commandgroup
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, tempString) == false)
        {
            MCore::LogError(tempString.AsChar());
        }
    }


    // slot for making actor instances invisible
    void MakeSelectedActorInstancesInvisible()
    {
        // get the selection and number of selected actor instances
        const SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selection.GetNumSelectedActorInstances();

        // create the command group
        MCore::String outResult;
        MCore::String command;
        MCore::CommandGroup commandGroup("Hide actor instances", numActorInstances * 2);

        // iterate over the selected instances
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the current actor instance
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (actorInstance == nullptr)
            {
                continue;
            }

            command.Format("Unselect -actorInstanceID %i", actorInstance->GetID());
            commandGroup.AddCommandString(command.AsChar());

            command.Format("AdjustActorInstance -actorInstanceID %i -doRender \"No\"", actorInstance->GetID());
            commandGroup.AddCommandString(command.AsChar());
        }

        // execute the commandgroup
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            if (outResult.GetIsEmpty() == false)
            {
                MCore::LogError(outResult.AsChar());
            }
        }
    }


    // slot for making actor instances visible
    void MakeSelectedActorInstancesVisible()
    {
        // get the selection and number of selected actor instances
        const SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selection.GetNumSelectedActorInstances();

        // create the command group
        MCore::String outResult;
        MCore::String command;
        MCore::CommandGroup commandGroup("Unhide actor instances", numActorInstances * 2);

        // iterate over the selected instances
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the current actor instance
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (actorInstance == nullptr)
            {
                continue;
            }

            command.Format("AdjustActorInstance -actorInstanceID %i -doRender \"Yes\"", actorInstance->GetID());
            commandGroup.AddCommandString(command.AsChar());
        }

        // execute the commandgroup
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            if (outResult.GetIsEmpty() == false)
            {
                MCore::LogError(outResult.AsChar());
            }
        }
    }


    // unselect the currently selected actors
    void UnselectSelectedActorInstances()
    {
        // get the selection and number of selected actor instances
        SelectionList selection = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selection.GetNumSelectedActorInstances();

        // create the command group
        MCore::String outResult;
        MCore::String command;
        MCore::CommandGroup commandGroup("Unselect all actor instances", numActorInstances + 1);

        // iterate over the selected instances and clone them
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the current actor instance
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (actorInstance == nullptr)
            {
                continue;
            }

            // add the remove command
            command.Format("Unselect -actorInstanceID %i", actorInstance->GetID());
            commandGroup.AddCommandString(command.AsChar());
        }

        // disable selection lock, if actor has been deselected
        if (GetCommandManager()->GetLockSelection())
        {
            commandGroup.AddCommandString("ToggleLockSelection");
        }

        // execute the commandgroup
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.AsChar());
        }
    }
} // namespace CommandSystem
