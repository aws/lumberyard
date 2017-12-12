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

// include required headers
#include "EMotionFXConfig.h"
#include "BlendTree.h"
#include "AnimGraphNode.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphMotionNode.h"
#include "MotionInstance.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "AnimGraphSyncTrack.h"


namespace EMotionFX
{
    // default constructor
    BlendTree::BlendTree(AnimGraph* animGraph, const char* name)
        : AnimGraphNode(animGraph, name, TYPE_ID)
    {
        mFinalNode          = nullptr;
        mVirtualFinalNode   = nullptr;

        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTree::~BlendTree()
    {
        // NOTE: child nodes get removed by the base class already
    }


    // create
    BlendTree* BlendTree::Create(AnimGraph* animGraph, const char* name)
    {
        return new BlendTree(animGraph, name);
    }


    // create unique data
    AnimGraphObjectData* BlendTree::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTree::RegisterPorts()
    {
        // setup output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void BlendTree::RegisterAttributes()
    {
    }


    // clone the tree
    AnimGraphObject* BlendTree::Clone(AnimGraph* animGraph)
    {
        BlendTree* clone = BlendTree::Create(animGraph);

        // TODO: clone the rest?
        CopyBaseObjectTo(clone);

        return clone;
    }


    // get the real final node
    AnimGraphNode* BlendTree::GetRealFinalNode() const
    {
        // if there is a virtual final node, use that one
        if (mVirtualFinalNode)
        {
            return mVirtualFinalNode;
        }

        // otherwise get the real final node
        return (mFinalNode->GetNumConnections() > 0) ? mFinalNode : nullptr;
    }


    // process the blend tree and calculate its output
    void BlendTree::Output(AnimGraphInstance* animGraphInstance)
    {
        // get the output pose
        AnimGraphPose* outputPose;

        // if this node is disabled, output the bind pose
        if (mDisabled)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        // output final node
        AnimGraphNode* finalNode = GetRealFinalNode();
        if (finalNode)
        {
            OutputIncomingNode(animGraphInstance, finalNode);

            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *finalNode->GetMainOutputPose(animGraphInstance);

            finalNode->DecreaseRef(animGraphInstance);
        }
        else
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
        }

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }


    // post sync update
    void BlendTree::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if this node is disabled, exit
        if (mDisabled)
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // update the final node
        AnimGraphNode* finalNode = GetRealFinalNode();
        if (finalNode)
        {
            finalNode->IncreaseRefDataRefCount(animGraphInstance);
            finalNode->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

            AnimGraphNodeData* finalNodeUniqueData = finalNode->FindUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* sourceData = finalNodeUniqueData->GetRefCountedData();

            // TODO: this happens somehow for 1 frame when transitioning towards a blend tree (the nullptr)
            if (sourceData)
            {
                data->SetEventBuffer(sourceData->GetEventBuffer());
                data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
                data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
            }

            finalNode->DecreaseRefDataRef(animGraphInstance);
        }
        else
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ZeroTrajectoryDelta();
            data->ClearEventBuffer();
        }
    }


    // update all nodes
    void BlendTree::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if this node is disabled, output the bind pose
        if (mDisabled)
        {
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        // if we have no virtual final node, use the real final node
        AnimGraphNode* finalNode = GetRealFinalNode();

        // update the final node
        if (finalNode)
        {
            finalNode->IncreasePoseRefCount(animGraphInstance);
            finalNode->IncreaseRefDataRefCount(animGraphInstance);
            finalNode->PerformUpdate(animGraphInstance, timePassedInSeconds);
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            uniqueData->Init(animGraphInstance, finalNode);
        }
        else
        {
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
        }
    }


    // rewind the nodes in the tree
    void BlendTree::Rewind(AnimGraphInstance* animGraphInstance)
    {
        // get the number of child nodes, iterate through and rewind them
        const uint32 numChildNodes = mChildNodes.GetLength();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            mChildNodes[i]->Rewind(animGraphInstance);
        }

        // call the base class rewind
        AnimGraphNode::Rewind(animGraphInstance);
    }


    // top down update
    void BlendTree::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // get the final node
        AnimGraphNode* finalNode = GetRealFinalNode();

        // update the final node
        if (finalNode)
        {
            // hierarhical sync update
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            HierarchicalSyncInputNode(animGraphInstance, finalNode, uniqueData);

            // pass the global weight along to the child nodes
            finalNode->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        }
    }


    // recursively set object data flag
    void BlendTree::RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled)
    {
        // set flag for this node
        animGraphInstance->SetObjectFlags(mObjectIndex, flag, enabled);

        // get the final node
        AnimGraphNode* finalNode = GetRealFinalNode();

        // update the final node recursively
        if (finalNode)
        {
            finalNode->RecursiveSetUniqueDataFlag(animGraphInstance, flag, enabled);
        }
    }


    // callback that gets called before a node gets removed
    void BlendTree::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        if (GetVirtualFinalNode() == nodeToRemove)
        {
            SetVirtualFinalNode(nullptr);
        }

        if (GetFinalNode() == nodeToRemove)
        {
            SetFinalNode(nullptr);
        }

        // call it for all children
        AnimGraphNode::OnRemoveNode(animGraph, nodeToRemove);
    }


    // update the final node and virtual final nodes
    void BlendTree::RecursiveClonePostProcess(AnimGraphNode* resultNode)
    {
        // do the same as the base class
        AnimGraphNode::RecursiveClonePostProcess(resultNode);

        // now do custom things
        MCORE_ASSERT(resultNode->GetType() == BlendTree::TYPE_ID);
        BlendTree* resultTree = static_cast<BlendTree*>(resultNode);

        // final node
        if (mFinalNode)
        {
            AnimGraphNode* node = resultTree->RecursiveFindNodeByID(mFinalNode->GetID());
            MCORE_ASSERT(node->GetType() == BlendTreeFinalNode::TYPE_ID);
            resultTree->SetFinalNode(static_cast<BlendTreeFinalNode*>(node));
        }

        // virtual final node
        if (mVirtualFinalNode)
        {
            AnimGraphNode* virtualNode = resultTree->RecursiveFindNodeByID(mVirtualFinalNode->GetID());
            resultTree->SetVirtualFinalNode(virtualNode);
        }
    }


    void BlendTree::SetVirtualFinalNode(AnimGraphNode* node)
    {
        mVirtualFinalNode = node;
    }


    void BlendTree::SetFinalNode(BlendTreeFinalNode* finalNode)
    {
        mFinalNode = finalNode;
    }
}   // namespace EMotionFX
