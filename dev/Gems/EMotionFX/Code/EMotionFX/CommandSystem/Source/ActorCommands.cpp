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
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/ExporterFileProcessor.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Mesh.h>
#include <MCore/Source/AttributeSet.h>
#include <MCore/Source/FileSystem.h>
#include "ActorCommands.h"
#include "CommandManager.h"
#include <AzFramework/API/ApplicationAPI.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandAdjustActor
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustActor::CommandAdjustActor(MCore::Command* orgCommand)
        : MCore::Command("AdjustActor", orgCommand)
    {
    }


    // destructor
    CommandAdjustActor::~CommandAdjustActor()
    {
    }


    // execute
    bool CommandAdjustActor::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

        // find the actor intance based on the given id
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult.Format("Cannot adjust actor. Actor ID %i is not valid.", actorID);
            return false;
        }

        MCore::String valueString;

        // set motion extraction node
        if (parameters.CheckIfHasParameter("motionExtractionNodeName"))
        {
            mOldMotionExtractionNodeIndex = actor->GetMotionExtractionNodeIndex();

            parameters.GetValue("motionExtractionNodeName", this, &valueString);
            if (valueString.GetIsEmpty() || valueString == "$NULL$")
            {
                actor->SetMotionExtractionNode(nullptr);
            }
            else
            {
                EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(valueString.AsChar());
                actor->SetMotionExtractionNode(node);
            }

            // inform all animgraph nodes about this
            const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
            for (uint32 i = 0; i < numAnimGraphs; ++i)
            {
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
                if (animGraph->GetIsOwnedByRuntime())
                {
                    continue;
                }
                const uint32 numObjects = animGraph->GetNumObjects();
                for (uint32 n = 0; n < numObjects; ++n)
                {
                    animGraph->GetObject(n)->OnActorMotionExtractionNodeChanged();
                }
            }
        }

        // set actor name
        if (parameters.CheckIfHasParameter("name"))
        {
            mOldName = actor->GetName();
            parameters.GetValue("name", this, &valueString);
            actor->SetName(valueString.AsChar());
        }

        // adjust the attachment nodes
        if (parameters.CheckIfHasParameter("attachmentNodes"))
        {
            // store old attachment nodes for undo
            mOldAttachmentNodes = "";
            const uint32 numOriginalNodes = actor->GetNumNodes();
            for (uint32 i = 0; i < numOriginalNodes; ++i)
            {
                // get the current node
                EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
                if (node == nullptr)
                {
                    continue;
                }

                // add node to the old attachment node list
                if (node->GetIsAttachmentNode())
                {
                    mOldAttachmentNodes.FormatAdd("%s;", node->GetName());
                }
            }

            // get the node names and split the string
            parameters.GetValue("nodeAction", "select", &valueString);

            MCore::String attachmentNodes;
            parameters.GetValue("attachmentNodes", this, &attachmentNodes);

            MCore::Array<MCore::String> nodeNames = attachmentNodes.Split(MCore::UnicodeCharacter(';'));

            // get the number of nodes
            const uint32 numNodes = nodeNames.GetLength();

            // remove the selected nodes from the node group
            if (valueString.CheckIfIsEqualNoCase("remove"))
            {
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    // get the node
                    EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(nodeNames[i].AsChar());
                    if (node == nullptr)
                    {
                        continue;
                    }

                    // remove the node
                    node->SetIsAttachmentNode(false);
                }
            }
            else if (valueString.CheckIfIsEqualNoCase("add")) // add the selected nodes to the node group
            {
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    // get the node
                    EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(nodeNames[i].AsChar());
                    if (node == nullptr)
                    {
                        continue;
                    }

                    // add the node
                    node->SetIsAttachmentNode(true);
                }
            }
            else // selected nodes form the new node group
            {
                // disable attachment node flag for all nodes of the actor
                SetIsAttachmentNode(actor, false);

                // set attachment node flag for the selected nodes
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    // get the node
                    EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(nodeNames[i].AsChar());
                    if (node == nullptr)
                    {
                        continue;
                    }

                    // add the node
                    node->SetIsAttachmentNode(true);
                }
            }
        }

        // adjust the nodes that are excluded from the bounding volume calculations
        if (parameters.CheckIfHasParameter("nodesExcludedFromBounds"))
        {
            // store old attachment nodes for undo
            mOldExcludedFromBoundsNodes = "";
            const uint32 numOriginalNodes = actor->GetNumNodes();
            for (uint32 i = 0; i < numOriginalNodes; ++i)
            {
                // get the current node
                EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
                if (node == nullptr)
                {
                    continue;
                }

                // add node to the old node list
                if (node->GetIncludeInBoundsCalc() == false)
                {
                    mOldExcludedFromBoundsNodes.FormatAdd("%s;", node->GetName());
                }
            }

            // get the node names and split the string
            parameters.GetValue("nodeAction", "select", &valueString);

            MCore::String nodesExcludedFromBounds;
            parameters.GetValue("nodesExcludedFromBounds", this, &nodesExcludedFromBounds);

            MCore::Array<MCore::String> nodeNames = nodesExcludedFromBounds.Split(MCore::UnicodeCharacter(';'));

            // get the number of nodes
            const uint32 numNodes = nodeNames.GetLength();

            // remove the selected nodes from the node group
            if (valueString.CheckIfIsEqualNoCase("remove"))
            {
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    // get the node
                    EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(nodeNames[i].AsChar());
                    if (node == nullptr)
                    {
                        continue;
                    }

                    // remove the node
                    node->SetIncludeInBoundsCalc(true);
                }
            }
            else if (valueString.CheckIfIsEqualNoCase("add")) // add the selected nodes to the node group
            {
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    // get the node
                    EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(nodeNames[i].AsChar());
                    if (node == nullptr)
                    {
                        continue;
                    }

                    // add the node
                    node->SetIncludeInBoundsCalc(false);
                }
            }
            else // selected nodes form the new node group
            {
                // disable attachment node flag for all nodes of the actor
                SetIsExcludedFromBoundsNode(actor, false);

                // set attachment node flag for the selected nodes
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    // get the node
                    EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(nodeNames[i].AsChar());
                    if (node == nullptr)
                    {
                        continue;
                    }

                    // add the node
                    node->SetIncludeInBoundsCalc(false);
                }
            }
        }

        // adjust the mirror setup
        if (parameters.CheckIfHasParameter("mirrorSetup"))
        {
            mOldMirrorSetup = actor->GetNodeMirrorInfos();

            MCore::String mirrorSetupString;
            parameters.GetValue("mirrorSetup", this, &mirrorSetupString);

            if (mirrorSetupString.GetLength() > 0)
            {
                // allocate the node mirror info table
                actor->AllocateNodeMirrorInfos();

                // parse the mirror setup string, which is like "nodeA,nodeB;nodeC,nodeD;"
                MCore::Array<MCore::String> pairs = mirrorSetupString.Split(MCore::UnicodeCharacter::semiColon); // split on ;
                MCore::Array<MCore::String> pairValues;
                for (uint32 p = 0; p < pairs.GetLength(); ++p)
                {
                    // split the pairs
                    pairValues = pairs[p].Split(MCore::UnicodeCharacter::comma);
                    if (pairValues.GetLength() != 2)
                    {
                        continue;
                    }

                    EMotionFX::Node* nodeA = actor->GetSkeleton()->FindNodeByName(pairValues[0].AsChar());
                    EMotionFX::Node* nodeB = actor->GetSkeleton()->FindNodeByName(pairValues[1].AsChar());
                    if (nodeA && nodeB)
                    {
                        actor->GetNodeMirrorInfo(nodeA->GetNodeIndex()).mSourceNode = static_cast<uint16>(nodeB->GetNodeIndex());
                        actor->GetNodeMirrorInfo(nodeB->GetNodeIndex()).mSourceNode = static_cast<uint16>(nodeA->GetNodeIndex());
                    }
                }

                // update the mirror axes
                actor->AutoDetectMirrorAxes();
            } // if mirror string > 0 length
            else
            {
                actor->RemoveNodeMirrorInfos();
            }
        } // if has mirrorSetup parameter

        // save the current dirty flag and tell the actor that something got changed
        mOldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }


    // undo the command
    bool CommandAdjustActor::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

        // find the actor intance based on the given id
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult.Format("Cannot adjust actor. Actor ID %i is not valid.", actorID);
            return false;
        }

        // set motion extraction node
        if (parameters.CheckIfHasParameter("motionExtractionNodeName"))
        {
            actor->SetMotionExtractionNodeIndex(mOldMotionExtractionNodeIndex);
        }

        // actor name
        if (parameters.CheckIfHasParameter("name"))
        {
            actor->SetName(mOldName.AsChar());
        }

        if (parameters.CheckIfHasParameter("mirrorSetup"))
        {
            actor->SetNodeMirrorInfos(mOldMirrorSetup);
            actor->AutoDetectMirrorAxes();
        }

        // set the attachment nodes
        if (parameters.CheckIfHasParameter("attachmentNodes"))
        {
            MCore::String command;
            command.Format("AdjustActor -actorID %i -nodeAction \"select\" -attachmentNodes \"%s\"", actorID, mOldAttachmentNodes.AsChar());
            if (GetCommandManager()->ExecuteCommandInsideCommand(command.AsChar(), outResult) == false)
            {
                MCore::LogError(outResult.AsChar());
                return false;
            }
        }

        // set the nodes that are not taken into account in the bounding volume calculations
        if (parameters.CheckIfHasParameter("nodesExcludedFromBounds"))
        {
            MCore::String command;
            command.Format("AdjustActor -actorID %i -nodeAction \"select\" -nodesExcludedFromBounds \"%s\"", actorID, mOldExcludedFromBoundsNodes.AsChar());
            if (GetCommandManager()->ExecuteCommandInsideCommand(command.AsChar(), outResult) == false)
            {
                MCore::LogError(outResult.AsChar());
                return false;
            }
        }

        // set the dirty flag back to the old value
        actor->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAdjustActor::InitSyntax()
    {
        GetSyntax().ReserveParameters(7);
        GetSyntax().AddRequiredParameter("actorID",            "The actor identification number of the actor to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("motionExtractionNodeName",   "The node from which to transfer a filtered part of the motion onto the actor instance.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("attachmentNodes",            "A list of nodes that should be set to attachment nodes.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("nodesExcludedFromBounds",    "A list of nodes that are excluded from all bounding volume calculations.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("name",                       "The name of the actor.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("nodeAction",                 "The action to perform with the nodes passed to the command.", MCore::CommandSyntax::PARAMTYPE_STRING, "select");
        GetSyntax().AddParameter("mirrorSetup",                "The list of mirror pairs in form of \"leftFoot,rightFoot;leftArm,rightArm;\". Or an empty string to clear the mirror table", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    // get the description
    const char* CommandAdjustActor::GetDescription() const
    {
        return "This command can be used to adjust the attributes of the given actor.";
    }


    // static function to set all IsAttachmentNode flags of the actor to one value
    void CommandAdjustActor::SetIsAttachmentNode(EMotionFX::Actor* actor, bool isAttachmentNode)
    {
        // iterate over all nodes and set attachmentNode flag
        const uint32 numNodes = actor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the current node
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            if (node == nullptr)
            {
                continue;
            }

            // set the attachment node flag
            node->SetIsAttachmentNode(isAttachmentNode);
        }
    }


    // static function to set all IsAttachmentNode flags of the actor to one value
    void CommandAdjustActor::SetIsExcludedFromBoundsNode(EMotionFX::Actor* actor, bool excludedFromBounds)
    {
        // iterate over all nodes and set attachmentNode flag
        const uint32 numNodes = actor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the current node
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            if (node == nullptr)
            {
                continue;
            }

            node->SetIncludeInBoundsCalc(!excludedFromBounds);
        }
    }


    //--------------------------------------------------------------------------------
    // CommandActorSetCollisionMeshes
    //--------------------------------------------------------------------------------

    // constructor
    CommandActorSetCollisionMeshes::CommandActorSetCollisionMeshes(MCore::Command* orgCommand)
        : MCore::Command("ActorSetCollisionMeshes", orgCommand)
    {
    }


    // destructor
    CommandActorSetCollisionMeshes::~CommandActorSetCollisionMeshes()
    {
    }


    // execute
    bool CommandActorSetCollisionMeshes::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the actor ID
        const uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

        // find the actor based on the given id
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult.Format("Cannot set collision meshes. Actor ID %i is not valid.", actorID);
            return false;
        }

        // get the lod
        const uint32 lod = parameters.GetValueAsInt("lod", MCORE_INVALIDINDEX32);

        // check if the lod is invalid
        if (lod > (actor->GetNumLODLevels() - 1))
        {
            outResult.Format("Cannot set collision meshes. LOD %i is not valid.", lod);
            return false;
        }

        // store the old nodes for the undo
        mOldNodeList = "";
        const uint32 numNodes = actor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::Mesh* mesh = actor->GetMesh(lod, i);
            if (mesh && mesh->GetIsCollisionMesh())
            {
                mOldNodeList.FormatAdd("%s;", actor->GetSkeleton()->GetNode(i)->GetName());
            }
        }

        // make sure there is no semicolon at the end
        mOldNodeList.TrimRight(MCore::UnicodeCharacter(';'));

        // get the node list
        MCore::String nodeList;
        parameters.GetValue("nodeList", this, &nodeList);

        // split the node list
        const MCore::Array<MCore::String> nodeNames = nodeList.Split(MCore::UnicodeCharacter(';'));

        // update the collision flag of each mesh
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the mesh
            EMotionFX::Mesh* mesh = actor->GetMesh(lod, i);
            if (mesh == nullptr)
            {
                continue;
            }

            // update the collision mesh flag
            const bool isCollisionMesh = nodeNames.Contains(actor->GetSkeleton()->GetNode(i)->GetNameString());
            mesh->SetIsCollisionMesh(isCollisionMesh);
        }

        // save the current dirty flag and tell the actor that something got changed
        mOldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);

        // reinit the renderable actors
        MCore::String reinitResult;
        GetCommandManager()->ExecuteCommandInsideCommand("ReInitRenderActors -resetViewCloseup false", reinitResult);

        // done
        return true;
    }


    // undo the command
    bool CommandActorSetCollisionMeshes::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the actor ID
        const uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

        // find the actor based on the given id
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult.Format("Cannot undo set collision meshes. Actor ID %i is not valid.", actorID);
            return false;
        }

        // get the lod
        const uint32 lod = parameters.GetValueAsInt("lod", MCORE_INVALIDINDEX32);
        if (lod > (actor->GetNumLODLevels() - 1))
        {
            outResult.Format("Cannot undo set collision meshes. LOD %i is not valid.", lod);
            return false;
        }

        // split the node list
        const MCore::Array<MCore::String> nodeNames = mOldNodeList.Split(MCore::UnicodeCharacter(';'));

        // update the collision flag of each mesh
        const uint32 numNodes = actor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the mesh
            EMotionFX::Mesh* mesh = actor->GetMesh(lod, i);
            if (mesh == nullptr)
            {
                continue;
            }

            // update the collision mesh flag
            const bool isCollisionMesh = nodeNames.Contains(actor->GetSkeleton()->GetNode(i)->GetNameString());
            mesh->SetIsCollisionMesh(isCollisionMesh);
        }

        // set the dirty flag back to the old value
        actor->SetDirtyFlag(mOldDirtyFlag);

        // reinit the renderable actors
        MCore::String reinitResult;
        GetCommandManager()->ExecuteCommandInsideCommand("ReInitRenderActors -resetViewCloseup false", reinitResult);

        // done
        return true;
    }


    // init the syntax of the command
    void CommandActorSetCollisionMeshes::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddParameter("actorID",     "The identification number of the actor we want to adjust.",    MCore::CommandSyntax::PARAMTYPE_INT,    "-1");
        GetSyntax().AddParameter("lod",         "The lod of the actor we want to adjust.",                      MCore::CommandSyntax::PARAMTYPE_INT,    "0");
        GetSyntax().AddParameter("nodeList",    "The node list.",                                               MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    // get the description
    const char* CommandActorSetCollisionMeshes::GetDescription() const
    {
        return "This command can be used to set the collision meshes of the given actor.";
    }


    //--------------------------------------------------------------------------------
    // CommandResetToBindPose
    //--------------------------------------------------------------------------------

    // execute
    bool CommandResetToBindPose::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        // iterate through all selected actor instances
        const uint32 numSelectedActorInstances = GetCommandManager()->GetCurrentSelection().GetNumSelectedActorInstances();
        for (uint32 i = 0; i < numSelectedActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetActorInstance(i);

            actorInstance->GetTransformData()->ResetToBindPoseTransformations();
            actorInstance->SetLocalPosition(MCore::Vector3(0.0f, 0.0f, 0.0f));
            actorInstance->SetLocalRotation(MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
            actorInstance->SetLocalScale(MCore::Vector3(1.0f, 1.0f, 1.0f));
        }

        // verify if we actually have selected an actor instance
        if (numSelectedActorInstances == 0)
        {
            outResult = "Cannot reset actor instances to bind pose. No actor instance selected.";
            return false;
        }

        return true;
    }


    // undo the command
    bool CommandResetToBindPose::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandResetToBindPose::InitSyntax()
    {
    }


    // get the description
    const char* CommandResetToBindPose::GetDescription() const
    {
        return "This command can be used to reset the actor instance back to the bind pose.";
    }


    //--------------------------------------------------------------------------------
    // CommandReInitRenderActors
    //--------------------------------------------------------------------------------

    // execute
    bool CommandReInitRenderActors::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // undo the command
    bool CommandReInitRenderActors::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandReInitRenderActors::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddParameter("resetViewCloseup", "", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    // get the description
    const char* CommandReInitRenderActors::GetDescription() const
    {
        return "This command will be automatically called by the system in case all render actors need to get removed and reconstructed completely.";
    }


    //--------------------------------------------------------------------------------
    // CommandUpdateRenderActors
    //--------------------------------------------------------------------------------

    // execute
    bool CommandUpdateRenderActors::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // undo the command
    bool CommandUpdateRenderActors::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandUpdateRenderActors::InitSyntax()
    {
    }


    // get the description
    const char* CommandUpdateRenderActors::GetDescription() const
    {
        return "This command will be automatically called by the system in case an actor got removed and we have to remove a render actor or in case there is a new actor we need to create a render actor for, all current render actors won't get touched.";
    }


    //--------------------------------------------------------------------------------
    // CommandRemoveActor
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveActor::CommandRemoveActor(MCore::Command* orgCommand)
        : MCore::Command("RemoveActor", orgCommand)
    {
        mPreviouslyUsedID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandRemoveActor::~CommandRemoveActor()
    {
    }


    // execute
    bool CommandRemoveActor::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        EMotionFX::Actor* actor;
        if (parameters.CheckIfHasParameter("actorID"))
        {
            uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

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

        // get rid of all actor instances which belong to the given filename
        /*for (i=0; i<EMotionFX::GetActorManager().GetNumActorInstances();)
        {
            // get the actor instance and its corresponding actor
            ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);
            Actor* actor = actorInstance->GetActor();

            // get rid of the actor instance in case the given filename equals to the actor's one
            if (filename.CheckIfIsEqual( actor->GetName() ))
                delete actorInstance;
            else
                i++;
        }*/

        // store the previously used id and the actor filename
        mPreviouslyUsedID   = actor->GetID();
        mData               = actor->GetName();
        mOldFileName        = actor->GetFileName();
        mOldDirtyFlag       = actor->GetDirtyFlag();
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();

        // get rid of the actor
        actor->Destroy();

        // mark the workspace as dirty
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // update our render actors
        MCore::String updateRenderActorsResult;
        GetCommandManager()->ExecuteCommandInsideCommand("UpdateRenderActors", updateRenderActorsResult);
        return true;
    }


    // undo the command
    bool CommandRemoveActor::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        MCore::String commandString;
        commandString.Format("ImportActor -filename \"%s\" -actorID %i", mOldFileName.AsChar(), mPreviouslyUsedID);
        GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult);

        // update our render actors
        MCore::String updateRenderActorsResult;
        GetCommandManager()->ExecuteCommandInsideCommand("UpdateRenderActors", updateRenderActorsResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return true;
    }


    // init the syntax of the command
    void CommandRemoveActor::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("actorID", "The identification number of the actor we want to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandRemoveActor::GetDescription() const
    {
        return "This command can be used to destruct an actor and all the corresponding actor instances.";
    }

    //-------------------------------------------------------------------------------------
    // Helper Functions
    //-------------------------------------------------------------------------------------
    void ClearScene(bool deleteActors, bool deleteActorInstances, MCore::CommandGroup* commandGroup)
    {
        // return if actorinstance was not set
        if (deleteActors == false && deleteActorInstances == false)
        {
            return;
        }

        // get number of actors and instances
        const uint32 numActors          = EMotionFX::GetActorManager().GetNumActors();
        const uint32 numActorInstances  = EMotionFX::GetActorManager().GetNumActorInstances();

        // create the command group
        MCore::String valueString;
        MCore::CommandGroup internalCommandGroup("Clear scene");

        valueString.Format("RecorderClear");
        if (commandGroup == nullptr)
        {
            internalCommandGroup.AddCommandString(valueString.AsChar());
        }
        else
        {
            commandGroup->AddCommandString(valueString.AsChar());
        }

        // get rid of all actor instances in the scene
        if (deleteActors || deleteActorInstances)
        {
            // get rid of all actor instances
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                // get pointer to the current actor instance
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

                // ignore runtime-owned actors
                if (actorInstance->GetIsOwnedByRuntime())
                {
                    continue;
                }

                // ignore visualization actor instances
                if (actorInstance->GetIsUsedForVisualization())
                {
                    continue;
                }

                // generate command to remove the actor instance
                valueString.Format("RemoveActorInstance -actorInstanceID %i", actorInstance->GetID());

                // add the command to the command group
                if (commandGroup == nullptr)
                {
                    internalCommandGroup.AddCommandString(valueString.AsChar());
                }
                else
                {
                    commandGroup->AddCommandString(valueString.AsChar());
                }
            }
        }

        // get rid of all actors in the scene
        if (deleteActors)
        {
            // iterate through all available actors
            for (uint32 i = 0; i < numActors; ++i)
            {
                // get the current actor
                EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

                // ignore runtime-owned actors
                if (actor->GetIsOwnedByRuntime())
                {
                    continue;
                }

                // ignore visualization actors
                if (actor->GetIsUsedForVisualization())
                {
                    continue;
                }

                // create the command to remove the actor
                valueString.Format("RemoveActor -actorID %i", actor->GetID());

                // add the command to the command group
                if (commandGroup == nullptr)
                {
                    internalCommandGroup.AddCommandString(valueString.AsChar());
                }
                else
                {
                    commandGroup->AddCommandString(valueString.AsChar());
                }
            }
        }

        // clear the existing selection
        valueString.Format("Unselect -actorID SELECT_ALL -actorInstanceID SELECT_ALL");
        if (commandGroup == nullptr)
        {
            internalCommandGroup.AddCommandString(valueString);
        }
        else
        {
            commandGroup->AddCommandString(valueString);
        }

        // execute the command or add it to the given command group
        if (commandGroup == nullptr)
        {
            if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, valueString) == false)
            {
                MCore::LogError(valueString.AsChar());
            }
        }
    }


    // walk over the meshes and check which of them we want to set as collision mesh
    void PrepareCollisionMeshesNodesString(EMotionFX::Actor* actor, uint32 lod, MCore::String* outNodeNames)
    {
        // reset the resulting string
        outNodeNames->Clear();
        outNodeNames->Reserve(16384);

        // check if the actor is invalid
        if (actor == nullptr)
        {
            return;
        }

        // check if the lod is invalid
        if (lod > (actor->GetNumLODLevels() - 1))
        {
            return;
        }

        // get the number of nodes and iterate through them
        const uint32 numNodes = actor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::Mesh* mesh = actor->GetMesh(lod, i);
            if (mesh && mesh->GetIsCollisionMesh())
            {
                outNodeNames->FormatAdd("%s;", actor->GetSkeleton()->GetNode(i)->GetName());
            }
        }

        // make sure there is no semicolon at the end
        outNodeNames->TrimRight(MCore::UnicodeCharacter(';'));
    }


    // walk over the actor nodes and check which of them we want to exclude from the bounding volume calculations
    void PrepareExcludedNodesString(EMotionFX::Actor* actor, MCore::String* outNodeNames)
    {
        // reset the resulting string
        outNodeNames->Clear();
        outNodeNames->Reserve(16384);

        // check if the actor is valid
        if (actor == nullptr)
        {
            return;
        }

        // get the number of nodes and iterate through them
        const uint32 numNodes = actor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);

            if (node->GetIncludeInBoundsCalc() == false)
            {
                *outNodeNames += node->GetName();
                *outNodeNames += ";";
            }
        }

        // make sure there is no semicolon at the end
        outNodeNames->TrimRight(MCore::UnicodeCharacter(';'));
    }


    //--------------------------------------------------------------------------------
    // CommandScaleActorData
    //--------------------------------------------------------------------------------

    // constructor
    CommandScaleActorData::CommandScaleActorData(MCore::Command* orgCommand)
        : MCore::Command("ScaleActorData", orgCommand)
    {
        mActorID            = MCORE_INVALIDINDEX32;
        mScaleFactor        = 1.0f;
        mOldActorDirtyFlag  = false;
        mUseUnitType        = false;
    }


    // destructor
    CommandScaleActorData::~CommandScaleActorData()
    {
    }


    // execute
    bool CommandScaleActorData::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        EMotionFX::Actor* actor;
        if (parameters.CheckIfHasParameter("id"))
        {
            uint32 actorID = parameters.GetValueAsInt("id", MCORE_INVALIDINDEX32);

            // find the actor based on the given id
            actor = EMotionFX::GetActorManager().FindActorByID(actorID);
            if (actor == nullptr)
            {
                outResult.Format("Cannot find actor with ID %d.", actorID);
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

        if (parameters.CheckIfHasParameter("unitType") == false && parameters.CheckIfHasParameter("scaleFactor") == false)
        {
            outResult = "You have to either specify -unitType or -scaleFactor.";
            return false;
        }

        mActorID = actor->GetID();
        mScaleFactor = parameters.GetValueAsFloat("scaleFactor", this);

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

        MCore::Distance::EUnitType beforeUnitType = actor->GetUnitType();
        mOldUnitType = MCore::Distance::UnitTypeToString(beforeUnitType);

        mOldActorDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);

        // perform the scaling
        if (mUseUnitType == false)
        {
            actor->Scale(mScaleFactor);
        }
        else
        {
            actor->ScaleToUnitType(targetUnitType);
        }

        // update the static aabb's of all actor instances
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);
            if (actorInstance->GetActor() != actor)
            {
                continue;
            }

            MCore::AABB newAABB;
            actorInstance->SetStaticBasedAABB(actor->GetStaticAABB());  // this is needed as the CalcStaticBasedAABB uses the current AABB as starting point
            actorInstance->CalcStaticBasedAABB(&newAABB);
            actorInstance->SetStaticBasedAABB(newAABB);
            //actorInstance->UpdateVisualizeScale();

            const float factor = (float)MCore::Distance::GetConversionFactor(beforeUnitType, targetUnitType);
            actorInstance->SetVisualizeScale(actorInstance->GetVisualizeScale() * factor);
        }

        // reinit the renderable actors
        MCore::String reinitResult;
        GetCommandManager()->ExecuteCommandInsideCommand("ReInitRenderActors -resetViewCloseup false", reinitResult);

        return true;
    }


    // undo the command
    bool CommandScaleActorData::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        if (mUseUnitType == false)
        {
            MCore::String commandString;
            commandString.Format("ScaleActorData -id %d -scaleFactor %.8f", mActorID, 1.0f / mScaleFactor);
            GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult);
        }
        else
        {
            MCore::String commandString;
            commandString.Format("ScaleActorData -id %d -unitType \"%s\"", mActorID, mOldUnitType.AsChar());
            GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult);
        }

        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(mActorID);
        if (actor)
        {
            actor->SetDirtyFlag(mOldActorDirtyFlag);
        }

        return true;
    }


    // init the syntax of the command
    void CommandScaleActorData::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddParameter("id",          "The identification number of the actor we want to scale.",             MCore::CommandSyntax::PARAMTYPE_INT,    "-1");
        GetSyntax().AddParameter("scaleFactor", "The scale factor, for example 10.0 to make the actor 10x as large.",   MCore::CommandSyntax::PARAMTYPE_FLOAT,  "1.0");
        GetSyntax().AddParameter("unitType",    "The unit type to convert to, for example 'meters'.",                   MCore::CommandSyntax::PARAMTYPE_STRING, "meters");
    }


    // get the description
    const char* CommandScaleActorData::GetDescription() const
    {
        return "This command can be used to scale all internal actor data. This includes vertex positions, morph targets, bounding volumes, bind pose transforms, etc.";
    }
} // namespace CommandSystem
