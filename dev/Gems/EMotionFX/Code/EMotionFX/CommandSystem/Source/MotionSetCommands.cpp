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

#include "MotionSetCommands.h"
#include <AzFramework/StringFunc/StringFunc.h>
#include "CommandManager.h"
#include <MCore/Source/Compare.h>
#include <MCore/Source/FileSystem.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/WaveletSkeletalMotion.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/ExporterFileProcessor.h>
#include <AzFramework/API/ApplicationAPI.h>


namespace CommandSystem
{
    // Custom motion set motion loading.
    EMotionFX::Motion* CommandSystemMotionSetCallback::LoadMotion(EMotionFX::MotionSet::MotionEntry* entry)
    {
        AZ_Assert(m_motionSet, "Motion set is nullptr.");

        // Get the full filename and file extension.
        const AZStd::string filename = m_motionSet->ConstructMotionFilename(entry);
        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false);

        // Are we dealing with a valid motion file?
        EMotionFX::Motion* motion = nullptr;
        if (AzFramework::StringFunc::Equal(extension.c_str(), "motion"))
        {
            motion = (EMotionFX::Motion*)EMotionFX::GetImporter().LoadSkeletalMotion(filename.c_str(), nullptr); // TODO: what about motion load settings?
        }
        else
        {
            MCore::LogWarning("MotionSet - Loading motion '%s' (id=%s) failed as the file extension isn't known.", filename.c_str(), entry->GetID().c_str());
        }

        // Create and set the motion name.
        if (motion)
        {
            AZStd::string motionName;
            AzFramework::StringFunc::Path::GetFileName(filename.c_str(), motionName);
            motion->SetName(motionName.c_str());
        }

        return motion;
    }


    //--------------------------------------------------------------------------------
    // CommandCreateMotionSet
    //--------------------------------------------------------------------------------
    CommandCreateMotionSet::CommandCreateMotionSet(MCore::Command* orgCommand)
        : MCore::Command("CreateMotionSet", orgCommand)
    {
        mPreviouslyUsedID = MCORE_INVALIDINDEX32;
    }


    CommandCreateMotionSet::~CommandCreateMotionSet()
    {
    }


    bool CommandCreateMotionSet::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        AZStd::string motionSetName;
        parameters.GetValue("name", this, motionSetName);

        // Does a motion set with the given name already exist?
        if (EMotionFX::GetMotionManager().FindMotionSetByName(motionSetName.c_str()))
        {
            outResult.Format("Cannot create motion set. A motion set with name '%s' already exists.", motionSetName.c_str());
            return false;
        }

        // Get the parent motion set.
        EMotionFX::MotionSet* parentSet = nullptr;
        if (parameters.CheckIfHasParameter("parentSetID"))
        {
            const uint32 parentSetID = parameters.GetValueAsInt("parentSetID", this);

            // Find the parent set by id.
            parentSet = EMotionFX::GetMotionManager().FindMotionSetByID(parentSetID);
            if (!parentSet)
            {
                outResult.Format("Cannot create motion set. The parent motion set with id %i does not exist.", parentSetID);
                return false;
            }
        }

        // Get the motion set id.
        uint32 motionSetID = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("motionSetID"))
        {
            motionSetID = parameters.GetValueAsInt("motionSetID", this);
            if (EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID))
            {
                outResult.Format("Cannot create motion set. A motion set with given ID '%i' already exists.", motionSetID);
                return false;
            }
        }

        // Create the new motion set.
        EMotionFX::MotionSet* motionSet = EMotionFX::MotionSet::Create(motionSetName.c_str(), parentSet);
        if (parentSet)
        {
            parentSet->AddChildSet(motionSet);
        }

        // Set the motion set id in case the parameter is specified.
        if (motionSetID != MCORE_INVALIDINDEX32)
        {
            motionSet->SetID(motionSetID);
        }

        // In case of redoing the command set the previously used id.
        if (mPreviouslyUsedID != MCORE_INVALIDINDEX32)
        {
            motionSet->SetID(mPreviouslyUsedID);
        }

        // Set the filename.
        if (parameters.CheckIfHasParameter("fileName"))
        {
            AZStd::string filename;
            parameters.GetValue("fileName", this, filename);
            EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);
            motionSet->SetFilename(filename.c_str());
        }

        // Store info for undo.
        mPreviouslyUsedID   = motionSet->GetID();
        outResult           = MCore::String(mPreviouslyUsedID);

        // Set the motion set callback for custom motion loading.
        motionSet->SetCallback(new CommandSystemMotionSetCallback(motionSet), true);

        // Set the dirty flag.
        const AZStd::string commandString = AZStd::string::format("AdjustMotionSet -motionSetID %i -dirtyFlag true", mPreviouslyUsedID);
        GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);

        // Seturn the id of the newly created motion set.
        outResult = MCore::String(motionSet->GetID());

        // Recursively update attributes of all nodes.
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            animGraph->RecursiveUpdateAttributes();
        }

        // Update unique datas for all anim graph instances using the given motion set.
        EMotionFX::GetAnimGraphManager().UpdateInstancesUniqueDataUsingMotionSet(motionSet);

        // Mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);
        return true;
    }


    bool CommandCreateMotionSet::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        const AZStd::string commandString = AZStd::string::format("RemoveMotionSet -motionSetID %i", mPreviouslyUsedID);
        bool result = GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);

        // Restore the workspace dirty flag.
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    void CommandCreateMotionSet::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddRequiredParameter("name", "The name of the motion set.",                          MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("parentSetID",  "The name of the parent motion set.",                   MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("motionSetID",  "The unique identification number of the motion set.",  MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("fileName",     "The absolute filename of the motion set.",             MCore::CommandSyntax::PARAMTYPE_STRING,     "");
    }


    const char* CommandCreateMotionSet::GetDescription() const
    {
        return "Create a motion set with the given parameters.";
    }


    //--------------------------------------------------------------------------------
    // CommandRemoveMotionSet
    //--------------------------------------------------------------------------------
    CommandRemoveMotionSet::CommandRemoveMotionSet(MCore::Command* orgCommand)
        : MCore::Command("RemoveMotionSet", orgCommand)
    {
        mPreviouslyUsedID = MCORE_INVALIDINDEX32;
    }


    CommandRemoveMotionSet::~CommandRemoveMotionSet()
    {
    }


    bool CommandRemoveMotionSet::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Does a motion set with the given id exist?
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult.Format("Cannot remove motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        // Store information used by undo.
        mPreviouslyUsedID   = motionSet->GetID();
        mOldName            = motionSet->GetName();
        mOldFileName        = motionSet->GetFilename();

        if (!motionSet->GetParentSet())
        {
            mOldParentSetID = MCORE_INVALIDINDEX32;
        }
        else
        {
            mOldParentSetID = motionSet->GetParentSet()->GetID();
        }

        // Destroy the motion set.
        motionSet->Destroy();

        // Recursively update attributes of all nodes.
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            animGraph->RecursiveUpdateAttributes();
        }

        // Update unique datas for all anim graph instances using the given motion set.
        EMotionFX::GetAnimGraphManager().UpdateInstancesUniqueDataUsingMotionSet(motionSet);

        // Mark the workspace as dirty.
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        return true;
    }


    bool CommandRemoveMotionSet::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        AZStd::string commandString = AZStd::string::format("CreateMotionSet -name \"%s\" -motionSetID %i", mOldName.c_str(), mPreviouslyUsedID);

        if (!mOldFileName.empty())
        {
            commandString += AZStd::string::format(" -fileName \"%s\"", mOldFileName.c_str());
        }

        if (mOldParentSetID != MCORE_INVALIDINDEX32)
        {
            commandString += AZStd::string::format(" -parentSetID %i", mOldParentSetID);
        }

        const bool result = GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);

        // Restore the workspace dirty flag.
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    void CommandRemoveMotionSet::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("motionSetID", "The unique identification number of the motion set.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    const char* CommandRemoveMotionSet::GetDescription() const
    {
        return "Remove the given motion set.";
    }

    //--------------------------------------------------------------------------------
    // CommandAdjustMotionSet
    //--------------------------------------------------------------------------------
    CommandAdjustMotionSet::CommandAdjustMotionSet(MCore::Command* orgCommand)
        : MCore::Command("AdjustMotionSet", orgCommand)
    {
    }


    CommandAdjustMotionSet::~CommandAdjustMotionSet()
    {
    }


    bool CommandAdjustMotionSet::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Does a motion set with the given id exist?
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult.Format("Cannot adjust motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        // Adjust the dirty flag.
        if (parameters.CheckIfHasParameter("dirtyFlag"))
        {
            mOldDirtyFlag           = motionSet->GetDirtyFlag();
            const bool dirtyFlag    = parameters.GetValueAsBool("dirtyFlag", this);
            motionSet->SetDirtyFlag(dirtyFlag);
        }

        // Set the new name in case the name parameter is specified.
        if (parameters.CheckIfHasParameter("newName"))
        {
            mOldSetName = motionSet->GetName();
            AZStd::string name;
            parameters.GetValue("newName", this, name);
            motionSet->SetName(name.c_str());

            mOldDirtyFlag = motionSet->GetDirtyFlag();
            motionSet->SetDirtyFlag(true);
        }

        return true;
    }


    bool CommandAdjustMotionSet::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Does a motion set with the given id exist?
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult.Format("Cannot adjust motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        // Adjust the dirty flag
        if (parameters.CheckIfHasParameter("dirtyFlag"))
        {
            motionSet->SetDirtyFlag(mOldDirtyFlag);
        }

        // Adjust the name.
        if (parameters.CheckIfHasParameter("newName"))
        {
            motionSet->SetName(mOldSetName.c_str());
        }

        return true;
    }


    void CommandAdjustMotionSet::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("motionSetID", "The unique identification number of the motion set.",                                  MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("newName",             "The new name of the motion set.",                                                      MCore::CommandSyntax::PARAMTYPE_STRING,     "");
        GetSyntax().AddParameter("dirtyFlag",           "The dirty flag indicates whether the user has made changes to the motion set or not.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "false");
    }


    const char* CommandAdjustMotionSet::GetDescription() const
    {
        return "Adjust the attributes of a given motion set.";
    }


    //--------------------------------------------------------------------------------
    // CommandMotionSetAddMotion
    //--------------------------------------------------------------------------------
    CommandMotionSetAddMotion::CommandMotionSetAddMotion(MCore::Command* orgCommand)
        : MCore::Command("MotionSetAddMotion", orgCommand)
    {
    }


    CommandMotionSetAddMotion::~CommandMotionSetAddMotion()
    {
    }


    bool CommandMotionSetAddMotion::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Find the motion set with the given id.
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult.Format("Cannot add motion entry to motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        // Get the motion filename and the identification string.
        AZStd::string motionFilename, idString;
        parameters.GetValue("motionFileName", this, motionFilename);
        parameters.GetValue("idString", this, idString);

        // Create the motion entry and add it to the motion set.
        EMotionFX::MotionSet::MotionEntry* newMotionEntry = EMotionFX::MotionSet::MotionEntry::Create(motionFilename.c_str(), idString.c_str(), nullptr);
        motionSet->AddMotionEntry(newMotionEntry);

        // Set the dirty flag.
        AZStd::string commandString = AZStd::string::format("AdjustMotionSet -motionSetID %i -dirtyFlag true", motionSetID);
        GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);

        // Return the id of the newly created motion set.
        outResult = MCore::String(motionSet->GetID());

        // Recursively update attributes of all nodes.
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            animGraph->RecursiveUpdateAttributes();
        }

        // Update unique datas for all anim graph instances using the given motion set.
        EMotionFX::GetAnimGraphManager().UpdateInstancesUniqueDataUsingMotionSet(motionSet);

        return true;
    }


    bool CommandMotionSetAddMotion::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Get the motion id string;
        AZStd::string idString;
        parameters.GetValue("idString", this, idString);

        AZStd::string commandString = AZStd::string::format("MotionSetRemoveMotion -motionSetID %i -idString \"%s\"", motionSetID, idString.c_str());

        return GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);
    }


    void CommandMotionSetAddMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddRequiredParameter("motionSetID",      "The unique identification number of the motion set.",                                                                                              MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("idString",         "The identification string that is assigned to the motion.",                                                                                        MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("motionFileName",           "The local filename of the motion. An example would be \"Walk.motion\" without any path information.",                                              MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    const char* CommandMotionSetAddMotion::GetDescription() const
    {
        return "Add a motion to the given motion set.";
    }

    //--------------------------------------------------------------------------------
    // CommandMotionSetRemoveMotion
    //--------------------------------------------------------------------------------
    CommandMotionSetRemoveMotion::CommandMotionSetRemoveMotion(MCore::Command* orgCommand)
        : MCore::Command("MotionSetRemoveMotion", orgCommand)
    {
    }


    CommandMotionSetRemoveMotion::~CommandMotionSetRemoveMotion()
    {
    }


    bool CommandMotionSetRemoveMotion::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Find the motion set with the given id.
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult.Format("Cannot remove motion entry from motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        // Get the motion entry id string.
        AZStd::string valueString;
        parameters.GetValue("idString", this, valueString);

        // Get the motion entry by id string.
        EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryByStringID(valueString.c_str());
        if (!motionEntry)
        {
            outResult.Format("Cannot remove motion entry from motion set. Motion entry '%s' does not exist.", valueString.c_str());
            return false;
        }

        // Save some information for the undo process.
        mOldFileName    = motionEntry->GetFilename();

        // Remove the motion entry from the motion set.
        motionSet->RemoveMotionEntry(motionEntry);

        // Recursively update attributes of all nodes.
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            animGraph->RecursiveUpdateAttributes();
        }

        // Update unique datas for all anim graph instances using the given motion set.
        EMotionFX::GetAnimGraphManager().UpdateInstancesUniqueDataUsingMotionSet(motionSet);

        // Set the dirty flag.
        const AZStd::string commandString = AZStd::string::format("AdjustMotionSet -motionSetID %i -dirtyFlag true", motionSetID);
        return GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);
    }


    bool CommandMotionSetRemoveMotion::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Get the id string.
        AZStd::string idString;
        parameters.GetValue("idString", this, idString);

        AZStd::string commandString = AZStd::string::format("MotionSetAddMotion -motionSetID %i -idString \"%s\" ", motionSetID, idString.c_str());

        if (!mOldFileName.empty())
        {
            commandString += AZStd::string::format(" -motionFileName \"%s\"", mOldFileName.c_str());
        }

        return GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);
    }


    void CommandMotionSetRemoveMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("motionSetID", "The unique identification number of the motion set.",          MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("idString",            "The identification string that is assigned to the motion.",    MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    const char* CommandMotionSetRemoveMotion::GetDescription() const
    {
        return "Remove the given motion from the motion set.";
    }

    //--------------------------------------------------------------------------------
    // CommandMotionSetAdjustMotion
    //--------------------------------------------------------------------------------

    CommandMotionSetAdjustMotion::CommandMotionSetAdjustMotion(MCore::Command* orgCommand)
        : MCore::Command("MotionSetAdjustMotion", orgCommand)
    {
    }


    CommandMotionSetAdjustMotion::~CommandMotionSetAdjustMotion()
    {
    }


    void CommandMotionSetAdjustMotion::UpdateMotionNodes(const char* oldID, const char* newID)
    {
        // iterate through the anim graphs and update all motion nodes
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            // get the current anim graph
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            // collect all motion nodes inside that anim graph
            MCore::Array<EMotionFX::AnimGraphNode*> motionNodes;
            animGraph->RecursiveCollectNodesOfType(EMotionFX::AnimGraphMotionNode::TYPE_ID, &motionNodes);

            // iterate through all motion nodes and update their id as well
            const uint32 numMotionNodes = motionNodes.GetLength();
            for (uint32 j = 0; j < numMotionNodes; ++j)
            {
                EMotionFX::AnimGraphMotionNode* motionNode = static_cast<EMotionFX::AnimGraphMotionNode*>(motionNodes[j]);

                // iterate over all motion ids inside the list for this motion node
                // and update the old to the new ids
                MCore::AttributeArray* arrayAttrib = motionNode->GetAttributeArray(EMotionFX::AnimGraphMotionNode::ATTRIB_MOTION);
                const uint32 numItems = arrayAttrib->GetNumAttributes();
                for (uint32 a = 0; a < numItems; ++a)
                {
                    MCore::Attribute* itemAttrib = arrayAttrib->GetAttribute(a);
                    MCORE_ASSERT(itemAttrib->GetType() == MCore::AttributeString::TYPE_ID);
                    MCore::AttributeString* stringItemAttrib = static_cast<MCore::AttributeString*>(itemAttrib);
                    if (stringItemAttrib->GetValue().CheckIfIsEqual(oldID))
                    {
                        stringItemAttrib->SetValue(newID);
                    }
                }
            }
        }
    }


    bool CommandMotionSetAdjustMotion::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // Find the motion set with the given id.
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult.Format("Cannot remove motion entry from motion set. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        AZStd::string idString;
        parameters.GetValue("idString", this, idString);

        // Get the motion entry.
        EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryByStringID(idString.c_str());
        if (!motionEntry)
        {
            outResult.Format("Cannot adjust motion entry. Motion entry '%s' does not exist.", idString.c_str());
            return false;
        }

        // Save the old infos for undo.
        mOldIdString        = motionEntry->GetID();
        mOldMotionFilename  = motionEntry->GetFilename();

        if (parameters.CheckIfHasParameter("motionFileName"))
        {
            // Get the new motion filename from the parameters and set it to the entry.
            AZStd::string motionFilename;
            parameters.GetValue("motionFileName", this, motionFilename);
            motionEntry->SetFilename(motionFilename.c_str());

            // Reset the motion entry so that it automatically reloads the new motion next time.
            motionEntry->Reset();

            // Reset all motion node unique datas recursively for all anim graph instances.
            const uint32 numAnimGraphInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
            for (uint32 i = 0; i < numAnimGraphInstances; ++i)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);

                // Only continue in case the anim graph instance is using the given motion set.
                if (animGraphInstance->GetMotionSet() != motionSet)
                {
                    continue;
                }

                EMotionFX::AnimGraph* animGraph = animGraphInstance->GetAnimGraph();

                // Recursively get all motion nodes inside the anim graph.
                MCore::Array<EMotionFX::AnimGraphNode*> motionNodes;
                animGraph->RecursiveCollectNodesOfType(EMotionFX::AnimGraphMotionNode::TYPE_ID, &motionNodes);

                // Get the number of motion nodes in the anim graph and iterate through them.
                const uint32 numMotionNodes = motionNodes.GetLength();
                for (uint32 m = 0; m < numMotionNodes; ++m)
                {
                    // Type cast the node to a motion node and reset its unique data so that the motion instance gets recreated.
                    EMotionFX::AnimGraphMotionNode* motionNode = static_cast<EMotionFX::AnimGraphMotionNode*>(motionNodes[m]);
                    motionNode->ResetUniqueData(animGraphInstance);
                }
            }
        }

        if (parameters.CheckIfHasParameter("newIDString"))
        {
            // Get the new id string.
            AZStd::string newId;
            parameters.GetValue("newIDString", this, newId);

            // Build a list of unique string id values from all motion set entries.
            AZStd::vector<AZStd::string> idStrings;
            motionSet->BuildIdStringList(idStrings);

            // Check if the id string is already in the list and return false if yes. The ids have to be unique.
            if (AZStd::find(idStrings.begin(), idStrings.end(), newId) != idStrings.end())
            {
                outResult.Format("Cannot set id '%s' to the motion entry '%s'. The id already exists.", idString.c_str(), motionEntry->GetID().c_str());
                return false;
            }

            motionSet->SetMotionEntryId(motionEntry, newId.c_str());

            // Update all motion nodes and link them to the new motion id.
            if (parameters.GetValueAsBool("updateMotionNodeStringIDs", this) )
            {
                UpdateMotionNodes(mOldIdString.c_str(), newId.c_str());
            }
        }

        // Recursively update attributes of all nodes.
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            animGraph->RecursiveUpdateAttributes();
        }

        // Update unique datas for all anim graph instances using the given motion set.
        EMotionFX::GetAnimGraphManager().UpdateInstancesUniqueDataUsingMotionSet(motionSet);

        // Set the dirty flag.
        const AZStd::string command = AZStd::string::format("AdjustMotionSet -motionSetID %i -dirtyFlag true", motionSetID);
        return GetCommandManager()->ExecuteCommandInsideCommand(command, outResult);
    }


    bool CommandMotionSetAdjustMotion::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        // In case we changed the id string, our current id string is the new id string parameter from the execute call.
        AZStd::string idString;
        const bool idStringChanged = parameters.CheckIfHasParameter("newIDString");
        if (idStringChanged)
        {
            parameters.GetValue("newIDString", this, idString);
        }
        else
        {
            // In case we didn't change the id string, just use normal id string parameter.
            parameters.GetValue("idString", this, idString);
        }

        bool updateMotionNodeStringIDs = parameters.GetValueAsBool("updateMotionNodeStringIDs", this);

        // Construct the undo command.
        AZStd::string command = AZStd::string::format("MotionSetAdjustMotion -motionSetID %i -idString \"%s\" -updateMotionNodeStringIDs %i", motionSetID, idString.c_str(), updateMotionNodeStringIDs);

        if (idStringChanged)
        {
            command += AZStd::string::format(" -newIDString \"%s\"", mOldIdString.c_str());
        }

        if (parameters.CheckIfHasParameter("motionFileName"))
        {
            command += AZStd::string::format(" -motionFileName \"%s\"", mOldMotionFilename.c_str());
        }

        return GetCommandManager()->ExecuteCommandInsideCommand(command, outResult);
    }


    void CommandMotionSetAdjustMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);
        GetSyntax().AddRequiredParameter("motionSetID",          "The unique identification number of the motion set.",                                                 MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("idString",             "The identification string that is assigned to the motion.",                                           MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("motionFileName",               "The local filename of the motion. An example would be \"Walk.motion\" without any path information.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("newIDString",                  "The identification string that is assigned to the motion.",                                           MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("updateMotionNodeStringIDs",    ".",                                                                                                   MCore::CommandSyntax::PARAMTYPE_STRING, "false");
    }


    const char* CommandMotionSetAdjustMotion::GetDescription() const
    {
        return "Adjust the given motion from the motion set.";
    }



    //--------------------------------------------------------------------------------
    // CommandLoadMotionSet
    //--------------------------------------------------------------------------------
    CommandLoadMotionSet::CommandLoadMotionSet(MCore::Command* orgCommand)
        : MCore::Command("LoadMotionSet", orgCommand)
    {
        mOldMotionSetID = MCORE_INVALIDINDEX32;
    }


    CommandLoadMotionSet::~CommandLoadMotionSet()
    {
    }


    bool CommandLoadMotionSet::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Get the filename of the motion set to load.
        AZStd::string filename;
        parameters.GetValue("filename", this, filename);
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        // Get the old log levels.
        MCore::LogCallback::ELogLevel oldLogLevels = MCore::GetLogManager().GetLogLevels();

        // Log warnings and errors only for now.
        MCore::GetLogManager().SetLogLevels((MCore::LogCallback::ELogLevel)(MCore::LogCallback::LOGLEVEL_ERROR | MCore::LogCallback::LOGLEVEL_WARNING));

        // Is the given motion set already loaded?
        if (EMotionFX::GetMotionManager().FindMotionSetByFileName(filename.c_str()))
        {
            outResult.Format("Motion set '%s' has already been loaded. Skipping.", filename.c_str());
            return true;
        }

        // Load the motion set.
        EMotionFX::MotionSet* motionSet = EMotionFX::GetImporter().LoadMotionSet(filename.c_str());
        if (!motionSet)
        {
            outResult.Format("Could not load motion set from file '%s'.", filename.c_str());
            return false;
        }

        // In case we are in a redo call assign the previously used id.
        if (mOldMotionSetID != MCORE_INVALIDINDEX32)
        {
            motionSet->SetID(mOldMotionSetID);
        }
        mOldMotionSetID = motionSet->GetID();

        // Set the custom loading callback and preload all motions.
        motionSet->SetCallback(new CommandSystemMotionSetCallback(motionSet), true);
        motionSet->Preload();

        // Mark the workspace as dirty.
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // Restore original log levels.
        MCore::GetLogManager().SetLogLevels(oldLogLevels);
        return true;
    }


    bool CommandLoadMotionSet::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        // Get the motion set the command created earlier by id.
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(mOldMotionSetID);
        if (motionSet == nullptr)
        {
            outResult.Format("Cannot undo load motion set command. Previously used motion set id '%i' is not valid.", mOldMotionSetID);
            return false;
        }

        // Remove the motion set including all child sets.
        MCore::CommandGroup commandGroup("Remove motion sets");
        RecursivelyRemoveMotionSets(motionSet, commandGroup);
        const bool result = GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, outResult);

        // Restore the workspace dirty flag.
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    void CommandLoadMotionSet::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("filename", "The filename of the motion file.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    const char* CommandLoadMotionSet::GetDescription() const
    {
        return "Load the given motion set from disk.";
    }


    void ClearMotionSetMotions(EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup)
    {
        // Create the command group.
        MCore::String outResult;
        MCore::CommandGroup internalCommandGroup("Motion set clear motions");

        // Iterate through all motion entries and remove them from the motion set.
        AZStd::string commandString;
        const EMotionFX::MotionSet::EntryMap& motionEntries = motionSet->GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;
            commandString = AZStd::string::format("MotionSetRemoveMotion -motionSetID %i -idString \"%s\"", motionSet->GetID(), motionEntry->GetID().c_str());

            if (!commandGroup)
            {
                internalCommandGroup.AddCommandString(commandString);
            }
            else
            {
                commandGroup->AddCommandString(commandString);
            }
        }

        // Execute the internal command group in case the command group parameter is not specified.
        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    void RemoveMotionSet(EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup)
    {
        // Remove all motions from the motion set.
        ClearMotionSetMotions(motionSet, commandGroup);

        // Remove the motion set itself.
        const AZStd::string commandString = AZStd::string::format("RemoveMotionSet -motionSetID %i", motionSet->GetID());
        commandGroup->AddCommandString(commandString);
    }


    void RecursivelyRemoveMotionSets(EMotionFX::MotionSet* motionSet, MCore::CommandGroup& commandGroup)
    {
        if (!motionSet)
        {
            return;
        }

        // Iterate through the child motion sets and recursively remove them.
        const uint32 numChildSets = motionSet->GetNumChildSets();
        for (uint32 i = 0; i < numChildSets; ++i)
        {
            EMotionFX::MotionSet* childSet = motionSet->GetChildSet(i);
            RecursivelyRemoveMotionSets(childSet, commandGroup);
        }

        // Remove the current motion set with all its motions.
        RemoveMotionSet(motionSet, &commandGroup);
    }


    void ClearMotionSetsCommand(MCore::CommandGroup* commandGroup)
    {
        // Create our command group.
        MCore::String outResult;
        MCore::CommandGroup internalCommandGroup("Clear motion sets");

        // Iterate through all root motion sets and remove them.
        const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            // Is the given motion set a root one? Only process root motion sets in the loop and remove all others recursively.
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);
            if (motionSet->GetParentSet())
            {
                continue;
            }

            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }

            // Recursively remove motion sets.
            if (commandGroup == nullptr)
            {
                RecursivelyRemoveMotionSets(motionSet, internalCommandGroup);
            }
            else
            {
                RecursivelyRemoveMotionSets(motionSet, *commandGroup);
            }
        }

        // Execute the internal command group in case the command group parameter is not specified.
        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    void LoadMotionSetsCommand(const AZStd::vector<AZStd::string>& filenames, bool reload, bool clearUpfront)
    {
        if (filenames.empty())
        {
            return;
        }

        // Get the number of motion sets to load.
        const size_t numFilenames = filenames.size();

        // Construct the command.
        const AZStd::string commandGroupName = AZStd::string::format("%s %d motion set%s", reload ? "Reload" : "Load", numFilenames, (numFilenames > 1) ? "s" : "");
        MCore::CommandGroup commandGroup(commandGroupName);

        // Clear all other motion sets first.
        if (clearUpfront)
        {
            ClearMotionSetsCommand(&commandGroup);
        }

        // Iterate over all filenames and load the motion sets.
        AZStd::string commandString;
        for (size_t i = 0; i < numFilenames; ++i)
        {
            // In case we want to reload the same motion set remove the old version first.
            if (reload && !clearUpfront)
            {
                EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByFileName(filenames[i].c_str());
                if (motionSet)
                {
                    RecursivelyRemoveMotionSets(motionSet, commandGroup);
                }
            }

            // Construct the load motion set command and add it to the group.
            commandString = AZStd::string::format("LoadMotionSet -filename \"%s\"", filenames[i].c_str());
            commandGroup.AddCommandString(commandString);
        }

        // Execute the group command.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    AZStd::string AddMotionSetEntry(uint32 motionSetId, const AZStd::string& defaultIdString, const AZStd::vector<AZStd::string>& idStrings, const AZStd::string& motionFilename, MCore::CommandGroup* commandGroup)
    {
        // Internal command group is used in case the given command group parameter is nullptr.
        MCore::CommandGroup internalCommandGroup("Add motion to motion set");

        // Use the filename without extension as id string in case there is no default id string specified.
        AZStd::string idString = defaultIdString;
        if (idString.empty())
        {
            AzFramework::StringFunc::Path::GetFileName(motionFilename.c_str(), idString);
        }

        // As each entry in the motion set needs its' unique id, add a number as post-fix and increase it until we find a non-existing one that we can use.
        const AZStd::string orgIdString = idString;
        AZ::u32 iterationNr = 1;
        while (AZStd::find(idStrings.begin(), idStrings.end(), idString) != idStrings.end())
        {
            idString = AZStd::string::format("%s%i", orgIdString.c_str(), iterationNr);
            iterationNr++;
        }

        // Construct the command and add it to the command group.
        AZStd::string command = AZStd::string::format("MotionSetAddMotion -motionSetID %i -idString \"%s\"", motionSetId, idString.c_str());

        // It is allowed to add entries without a link to a motion (motion set interfaces).
        if (!motionFilename.empty())
        {
            command += AZStd::string::format(" -motionFileName \"%s\"", motionFilename.c_str());
        }

        if (!commandGroup)
        {
            internalCommandGroup.AddCommandString(command);
        }
        else
        {
            commandGroup->AddCommandString(command);
        }

        // Execute the group command.
        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }

        return idString;
    }
} // namespace CommandSystem