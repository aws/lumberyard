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
#include "AnimGraphCommands.h"
#include "CommandManager.h"
#include <MCore/Source/FileSystem.h>
#include <MCore/Source/UnicodeCharacter.h>
#include <MCore/Source/UnicodeStringIterator.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/ExporterFileProcessor.h>
#include <AzFramework/API/ApplicationAPI.h>


namespace CommandSystem
{
    //-------------------------------------------------------------------------------------
    // Load the given anim graph
    //-------------------------------------------------------------------------------------

    // constructor
    CommandLoadAnimGraph::CommandLoadAnimGraph(MCore::Command* orgCommand)
        : MCore::Command("LoadAnimGraph", orgCommand)
    {
        mOldAnimGraphID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandLoadAnimGraph::~CommandLoadAnimGraph()
    {
    }


    // execute
    bool CommandLoadAnimGraph::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the id from the parameters
        uint32 animGraphID = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("animGraphID"))
        {
            animGraphID = parameters.GetValueAsInt("animGraphID", MCORE_INVALIDINDEX32);
            if (EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID))
            {
                outResult.Format("Cannot import anim graph. Anim graph ID %i is already in use.", animGraphID);
                return false;
            }
        }

        // get the filename and set it for the anim graph
        MCore::String filenameValue;
        parameters.GetValue("filename", this, &filenameValue);

        AZStd::string filename = filenameValue.AsChar();
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        // check if we have already loaded the anim graph
        if (EMotionFX::GetAnimGraphManager().FindAnimGraphByFileName(filename.c_str()))
        {
            outResult.Format("Anim graph '%s' has already been loaded. Skipping.", filename.c_str());
            return true;
        }

        // load anim graph from file
        EMotionFX::Importer::AnimGraphSettings settings;
        settings.mDisableNodeVisualization = false;
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetImporter().LoadAnimGraph(filename.c_str(), &settings);
        if (animGraph == nullptr)
        {
            outResult.Format("Failed to load anim graph.", filename.c_str());
            return false;
        }

        // set the id in case we have specified it as parameter
        if (animGraphID != MCORE_INVALIDINDEX32)
        {
            animGraph->SetID(animGraphID);
        }

        // in case we are in a redo call assign the previously used id
        if (mOldAnimGraphID != MCORE_INVALIDINDEX32)
        {
            animGraph->SetID(mOldAnimGraphID);
        }
        mOldAnimGraphID = animGraph->GetID();

        // recursively update attributes of all nodes
        animGraph->RecursiveUpdateAttributes();

        // update unique datas recursively for all anim graph instances
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
            {
                animGraphInstance->OnUpdateUniqueData();
            }
        }

        // return the id of the newly created anim graph
        outResult = MCore::String(animGraph->GetID());

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // automatically select the anim graph after loading it
        MCore::String resultString;
        GetCommandManager()->ExecuteCommandInsideCommand("Unselect -animGraphIndex SELECT_ALL", resultString);
        GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -animGraphID %d", animGraph->GetID()), resultString);

        return true;
    }


    // undo the command
    bool CommandLoadAnimGraph::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph the command created
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mOldAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("Cannot undo load anim graph command. Previously used anim graph id '%i' is not valid.", mOldAnimGraphID);
            return false;
        }

        // Remove the newly created anim graph.
        const bool result = GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("RemoveAnimGraph -animGraphID %i", mOldAnimGraphID), outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandLoadAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("filename", "The filename of the anim graph file.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("animGraphID", "The id to assign to the newly loaded anim graph.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("autoActivate", "True in case this anim graph should be automatically selected, false if not.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    // get the description
    const char* CommandLoadAnimGraph::GetDescription() const
    {
        return "This command loads a anim graph to the given file.";
    }


    //-------------------------------------------------------------------------------------
    // Adjust the given anim graph
    //-------------------------------------------------------------------------------------

    // constructor
    CommandAdjustAnimGraph::CommandAdjustAnimGraph(MCore::Command* orgCommand)
        : MCore::Command("AdjustAnimGraph", orgCommand)
    {
    }


    // destructor
    CommandAdjustAnimGraph::~CommandAdjustAnimGraph()
    {
    }


    // execute
    bool CommandAdjustAnimGraph::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph id from the string and check if it is valid
        const uint32            animGraphID    = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph*  animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("Cannot adjust anim graph. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        // set the new name of the anim graph
        MCore::String valueString;
        if (parameters.CheckIfHasParameter("name"))
        {
            parameters.GetValue("name", this, &valueString);
            mOldName        = animGraph->GetName();
            animGraph->SetName(valueString.AsChar());
        }

        // set the new description of the anim graph
        if (parameters.CheckIfHasParameter("description"))
        {
            parameters.GetValue("description", this, &valueString);
            mOldDescription = animGraph->GetDescription();
            animGraph->SetDescription(valueString.AsChar());
        }

        // set if retargeting shall be enabled
        if (parameters.CheckIfHasParameter("retargetingEnabled"))
        {
            // store the old retargeting flag, get and set the new one
            mOldRetargetingEnabled = animGraph->GetRetargetingEnabled();
            const bool retargetingEnabled = parameters.GetValueAsBool("retargetingEnabled", this);
            animGraph->SetRetargetingEnabled(retargetingEnabled);

            // sync anim graph instances
            const uint32 numAnimGraphInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
            for (uint32 i = 0; i < numAnimGraphInstances; ++i)
            {
                // sync the retargeting flag for the anim graph instances
                EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);
                if (animGraphInstance->GetAnimGraph() == animGraph)
                {
                    animGraphInstance->SetRetargetingEnabled(retargetingEnabled);
                }
            }
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // recursively update attributes of all nodes
        animGraph->RecursiveUpdateAttributes();

        // update unique datas recursively for all anim graph instances
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
            {
                animGraphInstance->OnUpdateUniqueData();
            }
        }

        return true;
    }


    // undo the command
    bool CommandAdjustAnimGraph::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph id from the string and check if it is valid
        const uint32                animGraphID    = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph*      animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("Cannot adjust anim graph. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        // set the new name of the anim graph
        if (parameters.CheckIfHasParameter("name"))
        {
            animGraph->SetName(mOldName.AsChar());
        }

        // set the old description of the anim graph
        if (parameters.CheckIfHasParameter("description"))
        {
            animGraph->SetDescription(mOldDescription.AsChar());
        }

        // set if retargeting shall be enabled
        if (parameters.CheckIfHasParameter("retargetingEnabled"))
        {
            animGraph->SetRetargetingEnabled(mOldRetargetingEnabled);

            // sync anim graph instances
            const uint32 numAnimGraphInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
            for (uint32 i = 0; i < numAnimGraphInstances; ++i)
            {
                // sync the retargeting flag for the anim graph instances
                EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);
                if (animGraphInstance->GetAnimGraph() == animGraph)
                {
                    animGraphInstance->SetRetargetingEnabled(mOldRetargetingEnabled);
                }
            }
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);

        // recursively update attributes of all nodes
        animGraph->RecursiveUpdateAttributes();

        // update unique datas recursively for all anim graph instances
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
            {
                animGraphInstance->OnUpdateUniqueData();
            }
        }

        return true;
    }


    // init the syntax of the command
    void CommandAdjustAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddRequiredParameter("animGraphID",    "The id of the anim graph to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("name",                    "The new name of the anim graph.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("description",             "The new description of the anim graph.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("retargetingEnabled",      "True in case retargeting of the anim graph as well as for all linked anim graph instances shall be enabled, false if not.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "");
    }


    // get the description
    const char* CommandAdjustAnimGraph::GetDescription() const
    {
        return "This command adjusts the attributes of a anim graph.";
    }


    //-------------------------------------------------------------------------------------
    // Create a new anim graph
    //-------------------------------------------------------------------------------------

    // constructor
    CommandCreateAnimGraph::CommandCreateAnimGraph(MCore::Command* orgCommand)
        : MCore::Command("CreateAnimGraph", orgCommand)
    {
        mPreviouslyUsedID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandCreateAnimGraph::~CommandCreateAnimGraph()
    {
    }


    // execute
    bool CommandCreateAnimGraph::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCore::String resultString;

        // find a unique anim graph name
        int animGraphIndex = 0;
        MCore::String animGraphIndexString;
        MCore::String animGraphName = "AnimGraph0";
        while (EMotionFX::GetAnimGraphManager().FindAnimGraphByName(animGraphName))
        {
            animGraphIndexString.FromInt(++animGraphIndex);
            animGraphName = "AnimGraph" + animGraphIndexString;
        }

        // get the new name of the anim graph to create
        if (parameters.CheckIfHasParameter("name"))
        {
            parameters.GetValue("name", this, &animGraphName);
        }
        if (mOldName.GetIsEmpty() == false)
        {
            animGraphName = mOldName;
        }
        mOldName = animGraphName;

        // create the anim graph
        EMotionFX::AnimGraph* animGraph = EMotionFX::AnimGraph::Create(animGraphName.AsChar());

        // create the root state machine object
        EMotionFX::AnimGraphObject* rootSMObject = EMotionFX::GetAnimGraphManager().GetObjectFactory()->CreateObjectByTypeID(animGraph, EMotionFX::AnimGraphStateMachine::TYPE_ID);
        if (rootSMObject == nullptr)
        {
            MCore::LogWarning("Cannot instantiate root state machine for anim graph '%s'.", animGraphName.AsChar());
            return false;
        }

        // type-cast the object to a state machine and set it as root state machine
        EMotionFX::AnimGraphStateMachine* rootSM = static_cast<EMotionFX::AnimGraphStateMachine*>(rootSMObject);
        animGraph->SetRootStateMachine(rootSM);

        animGraph->SetDirtyFlag(true);

        // in case we are in a redo call assign the previously used id
        if (parameters.CheckIfHasParameter("animGraphID"))
        {
            animGraph->SetID(parameters.GetValueAsInt("animGraphID", this));
        }
        if (mPreviouslyUsedID != MCORE_INVALIDINDEX32)
        {
            animGraph->SetID(mPreviouslyUsedID);
        }
        mPreviouslyUsedID = animGraph->GetID();

        // recursively update attributes of all nodes
        animGraph->RecursiveUpdateAttributes();

        // update unique datas recursively for all anim graph instances
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
            {
                animGraphInstance->OnUpdateUniqueData();
            }
        }

        // register master animgraph
        GetCommandManager()->ExecuteCommandInsideCommand("Unselect -animGraphIndex SELECT_ALL", resultString);
        GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -animGraphID %d", animGraph->GetID()), resultString);

        // return the id of the newly created anim graph
        outResult = MCore::String(animGraph->GetID());

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandCreateAnimGraph::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph the command created
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mPreviouslyUsedID);
        if (animGraph == nullptr)
        {
            outResult.Format("Cannot undo create anim graph command. Previously used anim graph id '%i' is not valid.", mPreviouslyUsedID);
            return false;
        }

        // remove the newly created anim graph again
        const bool result = GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("RemoveAnimGraph -animGraphID %i", mPreviouslyUsedID), outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandCreateAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddParameter("name",            "The name of the anim graph.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("animGraphID",    "The id of the anim graph to remove.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }


    // get the description
    const char* CommandCreateAnimGraph::GetDescription() const
    {
        return "This command creates a new anim graph.";
    }


    //-------------------------------------------------------------------------------------
    // Remove the given anim graph
    //-------------------------------------------------------------------------------------

    // constructor
    CommandRemoveAnimGraph::CommandRemoveAnimGraph(MCore::Command* orgCommand)
        : MCore::Command("RemoveAnimGraph", orgCommand)
    {
    }


    // destructor
    CommandRemoveAnimGraph::~CommandRemoveAnimGraph()
    {
    }


    // execute
    bool CommandRemoveAnimGraph::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph id from the string and check if it is valid
        const uint32                animGraphID    = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph*      animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("Cannot remove anim graph. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        // remove the given anim graph
        mOldName    = animGraph->GetName();
        mOldFileName = animGraph->GetFileName();
        mOldID      = animGraph->GetID();
        mOldIndex   = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);

        // iterate through all anim graph instances and remove the ones that depend on the anim graph to be removed
        for (uint32 i = 0; i < EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances(); )
        {
            // in case the assigned anim graph instance belongs to the anim graph to remove, remove it and unassign it from all actor instances
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);
            if (animGraphInstance->GetAnimGraph() == animGraph)
            {
                EMotionFX::GetAnimGraphManager().RemoveAnimGraphInstance(animGraphInstance);
            }
            else
            {
                i++;
            }
        }

        // get rid of the anim graph
        EMotionFX::GetAnimGraphManager().RemoveAnimGraph(animGraph);

        // unselect all anim graphs
        GetCommandManager()->ExecuteCommandInsideCommand("Unselect -animGraphIndex SELECT_ALL", outResult);

        // Reselect the anim graph at the index of the removed one if possible.
        const int numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (int indexToSelect = mOldIndex; indexToSelect >= 0; indexToSelect--)
        {
            // Is the index to select in a valid range?
            if (indexToSelect >= numAnimGraphs)
            {
                break;
            }

            EMotionFX::AnimGraph* selectionCandiate = EMotionFX::GetAnimGraphManager().GetAnimGraph(indexToSelect);
            if (!selectionCandiate->GetIsOwnedByRuntime())
            {
                const EMotionFX::AnimGraph* newSelectedAnimGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(indexToSelect);
                GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -animGraphID %d", newSelectedAnimGraph->GetID()), outResult);
                break;
            }
        }

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandRemoveAnimGraph::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        if (mOldFileName.GetIsEmpty() == false)
        {
            return GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("LoadAnimGraph -filename \"%s\" -animGraphID %i", mOldFileName.AsChar(), mOldID), outResult);
        }

        const bool result = GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("CreateAnimGraph -animGraphID %i -name \"%s\"", mOldID, mOldName.AsChar()), outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandRemoveAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandRemoveAnimGraph::GetDescription() const
    {
        return "This command removes the given anim graph.";
    }


    //-------------------------------------------------------------------------------------
    // Clone the given anim graph
    //-------------------------------------------------------------------------------------

    // constructor
    CommandCloneAnimGraph::CommandCloneAnimGraph(MCore::Command* orgCommand)
        : MCore::Command("CloneAnimGraph", orgCommand)
    {
        mOldAnimGraphID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandCloneAnimGraph::~CommandCloneAnimGraph()
    {
    }


    // execute
    bool CommandCloneAnimGraph::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the anim graph id from the string and check if it is valid
        const uint32                animGraphID    = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph*      animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("Cannot clone anim graph. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        MCore::String resultString;

        // generate the unique name string
        MCore::String uniqueName;
        uniqueName.GenerateUniqueString(animGraph->GetName(),
            [&](const MCore::String& value)
            {
                return (EMotionFX::GetAnimGraphManager().FindAnimGraphByName(value.AsChar()) == nullptr);
            });

        // clone the anim graph
        EMotionFX::AnimGraph* clonedAnimGraph = animGraph->Clone(uniqueName.AsChar());
        clonedAnimGraph->SetDirtyFlag(true);
        clonedAnimGraph->SetFileName("");

        // in case we are in a redo call assign the previously used id
        if (mOldAnimGraphID != MCORE_INVALIDINDEX32)
        {
            clonedAnimGraph->SetID(mOldAnimGraphID);
        }
        mOldAnimGraphID = clonedAnimGraph->GetID();

        // select the newly created anim graph
        GetCommandManager()->ExecuteCommandInsideCommand("Unselect -animGraphIndex SELECT_ALL", resultString);
        GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -animGraphID %d", clonedAnimGraph->GetID()), resultString);

        // return the id of the newly created anim graph
        outResult = MCore::String(clonedAnimGraph->GetID());

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandCloneAnimGraph::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph the command created
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mOldAnimGraphID);
        if (animGraph == nullptr)
        {
            outResult.Format("Cannot undo clone anim graph command. Previously used anim graph id '%i' is not valid.", mOldAnimGraphID);
            return false;
        }

        // Remove the newly created anim graph.
        const bool result = GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("RemoveAnimGraph -animGraphID %i", mOldAnimGraphID), outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandCloneAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph to clone.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandCloneAnimGraph::GetDescription() const
    {
        return "This command clones the given anim graph.";
    }


    //-------------------------------------------------------------------------------------
    // Activate the given anim graph
    //-------------------------------------------------------------------------------------
    CommandActivateAnimGraph::CommandActivateAnimGraph(MCore::Command* orgCommand)
        : MCore::Command("ActivateAnimGraph", orgCommand)
    {
        mActorInstanceID  = MCORE_INVALIDINDEX32;
        mOldAnimGraphUsed = MCORE_INVALIDINDEX32;
        mOldMotionSetUsed = MCORE_INVALIDINDEX32;
    }


    CommandActivateAnimGraph::~CommandActivateAnimGraph()
    {
    }


    bool CommandActivateAnimGraph::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Get the actor instance either by index or id.
        EMotionFX::ActorInstance* actorInstance = nullptr;
        if (parameters.CheckIfHasParameter("actorInstanceID"))
        {
            const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", this);
            actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
            if (!actorInstance)
            {
                outResult.Format("Cannot activate anim graph. Actor instance id '%i' is not valid.", actorInstanceID);
                return false;
            }
        }
        else if (parameters.CheckIfHasParameter("actorInstanceIndex"))
        {
            const uint32 actorInstanceIndex = parameters.GetValueAsInt("actorInstanceIndex", this);
            if (actorInstanceIndex < EMotionFX::GetActorManager().GetNumActorInstances())
            {
                actorInstance = EMotionFX::GetActorManager().GetActorInstance(actorInstanceIndex);
            }
            else
            {
                outResult.Format("Cannot activate anim graph. Actor instance index '%i' is not valid.", actorInstanceIndex);
                return false;
            }
        }
        else
        {
            outResult.Format("Cannot activate anim graph. Actor instance parameter must be specified.");
            return false;
        }

        // Get the anim graph to activate.
        EMotionFX::AnimGraph* animGraph = nullptr;
        if (parameters.CheckIfHasParameter("animGraphID"))
        {
            const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
            if (animGraphID != MCORE_INVALIDINDEX32)
            {
                animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
                if (!animGraph)
                {
                    outResult.Format("Cannot activate anim graph. Anim graph id '%i' is not valid.", animGraphID);
                    return false;
                }
            }
        }
        else if (parameters.CheckIfHasParameter("animGraphIndex"))
        {
            const uint32 animGraphIndex = parameters.GetValueAsInt("animGraphIndex", this);
            if (animGraphIndex < EMotionFX::GetAnimGraphManager().GetNumAnimGraphs())
            {
                animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(animGraphIndex);
            }
            else
            {
                outResult.Format("Cannot activate anim graph. Anim graph index '%i' is not valid.", animGraphIndex);
                return false;
            }
        }

        // Get the motion set to use.
        EMotionFX::MotionSet* motionSet = nullptr;
        if (parameters.CheckIfHasParameter("motionSetID"))
        {
            const uint32 motionSetID = parameters.GetValueAsInt("motionSetID", this);
            motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
            if (!motionSet)
            {
                outResult.Format("Cannot activate anim graph. Motion set id '%i' is not valid.", motionSetID);
                return false;
            }
        }
        else if (parameters.CheckIfHasParameter("motionSetIndex"))
        {
            const uint32 motionSetIndex = parameters.GetValueAsInt("motionSetIndex", this);
            if (motionSetIndex < EMotionFX::GetMotionManager().GetNumMotionSets())
            {
                motionSet = EMotionFX::GetMotionManager().GetMotionSet(motionSetIndex);
            }
            else
            {
                outResult.Format("Cannot activate anim graph. Motion set index '%i' is not valid.", motionSetIndex);
                return false;
            }
        }

        // store the actor instance ID
        mActorInstanceID = actorInstance->GetID();

        // get the motion system from the actor instance
        EMotionFX::MotionSystem* motionSystem = actorInstance->GetMotionSystem();

        // remove all motion instances from this motion system
        const uint32 numMotionInstances = motionSystem->GetNumMotionInstances();
        for (uint32 j = 0; j < numMotionInstances; ++j)
        {
            EMotionFX::MotionInstance* motionInstance = motionSystem->GetMotionInstance(j);
            motionSystem->RemoveMotionInstance(motionInstance);
        }

        // get the visualize scale from the string
        const float visualizeScale = parameters.GetValueAsFloat("visualizeScale", this);

        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance)
        {
            // get the anim graph instance anim graph and motion set
            EMotionFX::AnimGraph* animGraphInstanceAnimGraph = animGraphInstance->GetAnimGraph();
            EMotionFX::MotionSet* animGraphInstanceMotionSet = animGraphInstance->GetMotionSet();

            // store the currently used anim graph ID, motion set ID and the visualize scale
            mOldAnimGraphUsed = animGraphInstanceAnimGraph->GetID();
            mOldMotionSetUsed = (animGraphInstanceMotionSet) ? animGraphInstanceMotionSet->GetID() : MCORE_INVALIDINDEX32;
            mOldVisualizeScaleUsed = animGraphInstance->GetVisualizeScale();

            // check if the anim graph is valid
            if (animGraph)
            {
                // check if the anim graph is not the same or if the motion set is not the same
                // This following line is currently commented, as when we Stop an anim graph there is no other way to start it nicely.
                // So we just recreate it for now.
                // At a later stage we can make proper starting support to prevent recreation.
                //if (animGraphInstanceAnimGraph != animGraph || animGraphInstanceMotionSet != motionSet)
                {
                    // destroy the current anim graph instance
                    animGraphInstance->Destroy();

                    // create a new anim graph instance
                    animGraphInstance = EMotionFX::AnimGraphInstance::Create(animGraph, actorInstance, motionSet);
                    animGraphInstance->SetVisualizeScale(visualizeScale);

                    // set the anim graph instance
                    actorInstance->SetAnimGraphInstance(animGraphInstance);

                    // prepare, init and update
                    animGraphInstance->RecursivePrepareNodes();
                    animGraphInstance->Init();
                    animGraphInstance->OnUpdateUniqueData();
                }
            }
            else
            {
                animGraphInstance->Destroy();
                actorInstance->SetAnimGraphInstance(nullptr);
            }
        }
        else // no one anim graph instance set on the actor instance, create a new one
        {
            // store the currently used ID as invalid
            mOldAnimGraphUsed = MCORE_INVALIDINDEX32;
            mOldMotionSetUsed = MCORE_INVALIDINDEX32;

            // check if the anim graph is valid
            if (animGraph)
            {
                // create a new anim graph instance
                animGraphInstance = EMotionFX::AnimGraphInstance::Create(animGraph, actorInstance, motionSet);
                animGraphInstance->SetVisualizeScale(visualizeScale);

                // set the anim graph instance
                actorInstance->SetAnimGraphInstance(animGraphInstance);

                // prepare, init and update
                animGraphInstance->RecursivePrepareNodes();
                animGraphInstance->Init();
                animGraphInstance->OnUpdateUniqueData();
            }
        }

        // return the id of the newly created anim graph
        outResult = MCore::String(animGraph ? animGraph->GetID() : MCORE_INVALIDINDEX32);

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // done
        return true;
    }


    // undo the command
    bool CommandActivateAnimGraph::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the actor instance id and check if it is valid
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(mActorInstanceID);
        if (actorInstance == nullptr)
        {
            outResult.Format("Cannot undo activate anim graph. Actor instance id '%i' is not valid.", mActorInstanceID);
            return false;
        }

        // get the anim graph, invalid index is a special case to allow the anim graph to be nullptr
        EMotionFX::AnimGraph* animGraph;
        if (mOldAnimGraphUsed == MCORE_INVALIDINDEX32)
        {
            animGraph = nullptr;
        }
        else
        {
            animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mOldAnimGraphUsed);
            if (animGraph == nullptr)
            {
                outResult.Format("Cannot undo activate anim graph. Anim graph id '%i' is not valid.", mOldAnimGraphUsed);
                return false;
            }
        }

        // get the motion set, invalid index is a special case to allow the motion set to be nullptr
        EMotionFX::MotionSet* motionSet;
        if (mOldMotionSetUsed == MCORE_INVALIDINDEX32)
        {
            motionSet = nullptr;
        }
        else
        {
            motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(mOldMotionSetUsed);
            if (motionSet == nullptr)
            {
                outResult.Format("Cannot undo activate anim graph. Motion set id '%i' is not valid.", mOldMotionSetUsed);
                return false;
            }
        }

        // get the motion system from the actor instance
        EMotionFX::MotionSystem* motionSystem = actorInstance->GetMotionSystem();

        // remove all motion instances from this motion system
        const uint32 numMotionInstances = motionSystem->GetNumMotionInstances();
        for (uint32 j = 0; j < numMotionInstances; ++j)
        {
            EMotionFX::MotionInstance* motionInstance = motionSystem->GetMotionInstance(j);
            motionSystem->RemoveMotionInstance(motionInstance);
        }

        // get the current anim graph instance
        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

        // check if one anim graph instance is set on the actor instance
        if (animGraphInstance)
        {
            // check if the anim graph is valid
            if (animGraph)
            {
                // check if the anim graph is not the same or if the motion set is not the same
                if (animGraphInstance->GetAnimGraph() != animGraph || animGraphInstance->GetMotionSet() != motionSet)
                {
                    // destroy the current anim graph instance
                    animGraphInstance->Destroy();

                    // create a new anim graph instance
                    animGraphInstance = EMotionFX::AnimGraphInstance::Create(animGraph, actorInstance, motionSet);
                    animGraphInstance->SetVisualizeScale(mOldVisualizeScaleUsed);

                    // set the anim graph instance
                    actorInstance->SetAnimGraphInstance(animGraphInstance);

                    // prepare, init and update
                    animGraphInstance->RecursivePrepareNodes();
                    animGraphInstance->Init();
                    animGraphInstance->OnUpdateUniqueData();
                }
            }
            else
            {
                animGraphInstance->Destroy();
                actorInstance->SetAnimGraphInstance(nullptr);
            }
        }
        else // no one anim graph instance set on the actor instance, create a new one
        {
            // check if the anim graph is valid
            if (animGraph)
            {
                // create a new anim graph instance
                animGraphInstance = EMotionFX::AnimGraphInstance::Create(animGraph, actorInstance, motionSet);
                animGraphInstance->SetVisualizeScale(mOldVisualizeScaleUsed);

                // set the anim graph instance
                actorInstance->SetAnimGraphInstance(animGraphInstance);

                // prepare, init and update
                animGraphInstance->RecursivePrepareNodes();
                animGraphInstance->Init();
                animGraphInstance->OnUpdateUniqueData();
            }
        }

        // return the id of the newly created anim graph
        outResult = MCore::String(animGraph ? animGraph->GetID() : MCORE_INVALIDINDEX32);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        // done
        return true;
    }


    // init the syntax of the command
    void CommandActivateAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddParameter("actorInstanceID",     "The id of the actor instance.",    MCore::CommandSyntax::PARAMTYPE_INT,    "-1");
        GetSyntax().AddParameter("actorInstanceIndex",  "The index of the actor instance.", MCore::CommandSyntax::PARAMTYPE_INT,    "-1");
        GetSyntax().AddParameter("animGraphID",        "The id of the anim graph.",       MCore::CommandSyntax::PARAMTYPE_INT,    "-1");
        GetSyntax().AddParameter("animGraphIndex",     "The index of the anim graph.",    MCore::CommandSyntax::PARAMTYPE_INT,    "-1");
        GetSyntax().AddParameter("motionSetID",         "The id of the motion set.",        MCore::CommandSyntax::PARAMTYPE_INT,    "-1");
        GetSyntax().AddParameter("motionSetIndex",      "The index of the motion set.",     MCore::CommandSyntax::PARAMTYPE_INT,    "-1");
        GetSyntax().AddParameter("visualizeScale",      "The visualize scale.",             MCore::CommandSyntax::PARAMTYPE_FLOAT,  "1.0");
    }


    // get the description
    const char* CommandActivateAnimGraph::GetDescription() const
    {
        return "This command activate the given anim graph.";
    }


    //--------------------------------------------------------------------------------
    // CommandScaleMotionData
    //--------------------------------------------------------------------------------

    // constructor
    CommandScaleAnimGraphData::CommandScaleAnimGraphData(MCore::Command* orgCommand)
        : MCore::Command("ScaleAnimGraphData", orgCommand)
    {
        mID             = MCORE_INVALIDINDEX32;
        mScaleFactor    = 1.0f;
        mOldDirtyFlag   = false;
    }


    // destructor
    CommandScaleAnimGraphData::~CommandScaleAnimGraphData()
    {
    }


    // execute
    bool CommandScaleAnimGraphData::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        EMotionFX::AnimGraph* animGraph;
        if (parameters.CheckIfHasParameter("id"))
        {
            uint32 animGraphID = parameters.GetValueAsInt("id", MCORE_INVALIDINDEX32);

            animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
            if (animGraph == nullptr)
            {
                outResult.Format("Cannot get the animgraph, with ID %d.", animGraphID);
                return false;
            }
        }
        else
        {
            // check if there is any actor selected at all
            SelectionList& selection = GetCommandManager()->GetCurrentSelection();
            if (selection.GetNumSelectedAnimGraphs() == 0)
            {
                outResult = "No animgraph has been selected, please select one first.";
                return false;
            }

            // get the first selected motion
            animGraph = selection.GetAnimGraph(0);
        }

        if (parameters.CheckIfHasParameter("unitType") == false && parameters.CheckIfHasParameter("scaleFactor") == false)
        {
            outResult = "You have to either specify -unitType or -scaleFactor.";
            return false;
        }

        mID = animGraph->GetID();
        mScaleFactor = parameters.GetValueAsFloat("scaleFactor", 1.0f);

        MCore::String targetUnitTypeString;
        parameters.GetValue("unitType", this, &targetUnitTypeString);
        mUseUnitType = parameters.CheckIfHasParameter("unitType");

        MCore::Distance::EUnitType targetUnitType;
        bool stringConvertSuccess = MCore::Distance::StringToUnitType(targetUnitTypeString, &targetUnitType);
        if (mUseUnitType && stringConvertSuccess == false)
        {
            outResult.Format("The passed unitType '%s' is not a valid unit type.", targetUnitTypeString.AsChar());
            return false;
        }
        mOldUnitType = MCore::Distance::UnitTypeToString(animGraph->GetUnitType());

        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // perform the scaling
        if (mUseUnitType == false)
        {
            animGraph->Scale(mScaleFactor);
        }
        else
        {
            animGraph->ScaleToUnitType(targetUnitType);
        }

        return true;
    }


    // undo the command
    bool CommandScaleAnimGraphData::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        if (mUseUnitType == false)
        {
            const AZStd::string commandString = AZStd::string::format("ScaleAnimGraphData -id %d -scaleFactor %.8f", mID, 1.0f / mScaleFactor);
            GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);
        }
        else
        {
            const AZStd::string commandString = AZStd::string::format("ScaleAnimGraphData -id %d -unitType \"%s\"", mID, mOldUnitType.AsChar());
            GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);
        }

        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mID);
        if (animGraph)
        {
            animGraph->SetDirtyFlag(mOldDirtyFlag);
        }

        return true;
    }


    // init the syntax of the command
    void CommandScaleAnimGraphData::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddParameter("id",          "The identification number of the anim graph we want to scale.",                   MCore::CommandSyntax::PARAMTYPE_INT,    "-1");
        GetSyntax().AddParameter("scaleFactor", "The scale factor, for example 10.0 to make the positional data 10x as large.",     MCore::CommandSyntax::PARAMTYPE_FLOAT,  "1.0");
        GetSyntax().AddParameter("unitType",    "The unit type to convert to, for example 'meters'.",                               MCore::CommandSyntax::PARAMTYPE_STRING, "meters");
    }


    // get the description
    const char* CommandScaleAnimGraphData::GetDescription() const
    {
        return "This command can be used to scale all internal anim graph data. This means positional data such as IK handle positions etc.";
    }




    //-------------------------------------------------------------------------------------
    // Helper Functions
    //-------------------------------------------------------------------------------------

    void ClearAnimGraphsCommand(MCore::CommandGroup* commandGroup)
    {
        MCore::CommandGroup internalCommandGroup("Clear anim graphs");
        AZStd::string command;

        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            const EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
            command = AZStd::string::format("RemoveAnimGraph -animGraphID %i", animGraph->GetID());

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (!commandGroup)
            {
                internalCommandGroup.AddCommandString(command);
            }
            else
            {
                commandGroup->AddCommandString(command);
            }
        }

        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    void LoadAnimGraphsCommand(const AZStd::vector<AZStd::string>& filenames, bool reload)
    {
        if (filenames.empty())
        {
            return;
        }

        const size_t numFileNames = filenames.size();

        AZStd::string commandString = AZStd::string::format("%s %d anim graph%s", reload ? "Reload" : "Load", numFileNames, numFileNames > 1 ? "s" : "");
        MCore::CommandGroup commandGroup(commandString.c_str());

        // Iterate over all filenames and load the anim graphs.
        for (size_t i = 0; i < numFileNames; ++i)
        {
            // In case we want to reload the same anim graph remove the old version first.
            if (reload)
            {
                // Remove all anim graphs with the given filename.
                const AZ::u32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
                for (AZ::u32 j = 0; j < numAnimGraphs; ++j)
                {
                    const EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(j);

                    if (animGraph &&
                        !animGraph->GetIsOwnedByRuntime() &&
                        animGraph->GetFileNameString().CheckIfIsEqualNoCase(filenames[i].c_str()))
                    {
                        commandString = AZStd::string::format("RemoveAnimGraph -animGraphID %d", animGraph->GetID());
                        commandGroup.AddCommandString(commandString);
                    }
                }
            }

            commandString = AZStd::string::format("LoadAnimGraph -filename \"%s\"", filenames[i].c_str());
            commandGroup.AddCommandString(commandString);
        }

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }
} // namesapce EMotionFX