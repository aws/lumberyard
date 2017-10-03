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
#include "EMotionFXConfig.h"
#include "AnimGraphReferenceNode.h"
#include "AnimGraphInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"
#include "AnimGraph.h"
#include "Importer/Importer.h"
#include <MCore/Source/AttributeSettings.h>

namespace EMotionFX
{
    // constructor
    AnimGraphReferenceNode::AnimGraphReferenceNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        mRefAnimGraph = nullptr;

        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    AnimGraphReferenceNode::~AnimGraphReferenceNode()
    {
    }


    // create
    AnimGraphReferenceNode* AnimGraphReferenceNode::Create(AnimGraph* animGraph)
    {
        return new AnimGraphReferenceNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* AnimGraphReferenceNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr, nullptr);
    }


    // register the ports
    void AnimGraphReferenceNode::RegisterPorts()
    {
        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void AnimGraphReferenceNode::RegisterAttributes()
    {
        // the node we reference
        MCore::AttributeSettings* attributeInfo = RegisterAttribute("Reference Node", "refNodeID", "The node to reference.", ATTRIBUTE_INTERFACETYPE_STATESELECTION);
        attributeInfo->SetDefaultValue(MCore::AttributeString::Create());
        attributeInfo->SetReinitGuiOnValueChange(true);
        attributeInfo->SetReinitObjectOnValueChange(true);

        // TODO: add support for external animGraph files
        // add some attribute to select the animGraph file
    }


    // get the palette name
    const char* AnimGraphReferenceNode::GetPaletteName() const
    {
        return "Reference";
    }


    // get the category
    AnimGraphObject::ECategory AnimGraphReferenceNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // create a clone of this node
    AnimGraphObject* AnimGraphReferenceNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        AnimGraphReferenceNode* clone = new AnimGraphReferenceNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // perform the calculations / actions
    void AnimGraphReferenceNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // get the output pose
        AnimGraphPose* outputPose;

        // check if we already processed it
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        UpdateUniqueData(animGraphInstance, uniqueData);

        // check if we have a valid reference
        if (uniqueData->mInternalAnimGraphInstance == nullptr) // we haven't got a valid reference
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();

            // simply output the bind pose in that case
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());

        #ifdef EMFX_EMSTUDIOBUILD
            SetHasError(animGraphInstance, true);

            // visualize it
            if (GetCanVisualize(animGraphInstance))
            {
                animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
            }
        #endif
            return;
        }
        else
        {
        #ifdef EMFX_EMSTUDIOBUILD
            SetHasError(animGraphInstance, false);
        #endif
        }

        // calculate the output
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
        uniqueData->mInternalAnimGraphInstance->Output(&outputPose->GetPose(), false);

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }


    // get the blend node type string
    const char* AnimGraphReferenceNode::GetTypeString() const
    {
        return "AnimGraphReferenceNode";
    }


    // when we change an attribute value
    void AnimGraphReferenceNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance, nullptr);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        // check if the source node changed
        AnimGraphNode* sourceNode = animGraphInstance->GetAnimGraph()->RecursiveFindNode(GetAttributeString(ATTRIB_REFERENCENODE)->GetValue().AsChar());
        if (uniqueData->mInternalAnimGraphInstance)
        {
            const AnimGraphInstance::ReferenceInfo& referenceInfo = uniqueData->mInternalAnimGraphInstance->GetReferenceInfo();
            if (referenceInfo.mSourceNode != sourceNode)
            {
                uniqueData->mNeedsUpdate = true;

                // log some warning when we would result in an infinite recursion
                if (sourceNode)
                {
                    if (sourceNode->RecursiveFindNodeByID(GetID()))
                    {
                        MCore::LogWarning("AnimGraphReferenceNode::UpdateUniqueData() - The source node '%s' that is set in Reference node '%s' isn't safe to use as it would result in infinite recursion!", sourceNode->GetName(), GetName());
                    }
                }
            }
        }
        else
        {
            uniqueData->mNeedsUpdate = true;
        }


        // make sure we refresh the unique data
        UpdateUniqueData(animGraphInstance, uniqueData);
    }


    // update the unique data
    void AnimGraphReferenceNode::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        if (uniqueData == nullptr)
        {
            return;
        }

        if (uniqueData->mNeedsUpdate)
        {
            // we don't update again for now
            uniqueData->mNeedsUpdate = false;

            // delete any existing anim graph instance
            if (uniqueData->mInternalAnimGraphInstance)
            {
                uniqueData->mInternalAnimGraphInstance->Destroy();
                uniqueData->mInternalAnimGraphInstance = nullptr;
            }

            /*

                    // create a new instance
                    AnimGraphInstance::ReferenceInfo referenceInfo;
                    referenceInfo.mSourceNode       = animGraphInstance->GetAnimGraph()->RecursiveFindNode( GetAttributeString(ATTRIB_REFERENCENODE)->GetValue().AsChar() );
                    if (referenceInfo.mSourceNode == uniqueData->mDeletedNode)
                        referenceInfo.mSourceNode = nullptr;
                    referenceInfo.mParameterSource  = animGraphInstance;
                    referenceInfo.mReferenceNode    = this;
                    if (referenceInfo.mSourceNode)
                    {
                        // if this won't result in infinite recursion by referencing itself
                        if (referenceInfo.mSourceNode->RecursiveFindNodeByID( GetID() ) == nullptr)
                        {
                            // create a new animgraph instance for this reference
                            uniqueData->mInternalAnimGraphInstance = AnimGraphInstance::Create(animGraphInstance->GetAnimGraph(), animGraphInstance->GetActorInstance(), animGraphInstance->GetMotionSet(), nullptr, &referenceInfo);    // TODO: replace nullptr with some other init settings?
                        }
                    }
            */

            if (mRefAnimGraph == nullptr)
            {
                mRefAnimGraph = GetImporter().LoadAnimGraph(mAnimGraph->GetFileName());
                uniqueData->mInternalAnimGraphInstance = AnimGraphInstance::Create(mRefAnimGraph, animGraphInstance->GetActorInstance(), animGraphInstance->GetMotionSet(), nullptr, nullptr);
            }

            uniqueData->mDeletedNode = nullptr;
        }
    }


    // when parameter values have changed through the interface
    void AnimGraphReferenceNode::OnUpdateAttributes()
    {
        SetNodeInfo(GetAttributeString(ATTRIB_REFERENCENODE)->AsChar());
    }


    // set the absolute time
    void AnimGraphReferenceNode::SetCurrentPlayTime(AnimGraphInstance* animGraphInstance, float timeInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (uniqueData->mInternalAnimGraphInstance == nullptr)
        {
            return;
        }

        AnimGraphNode* sourceNode = uniqueData->mInternalAnimGraphInstance->GetRootNode();
        if (sourceNode)
        {
            //MCore::LogInfo("Adjusting playtime of reference node %s which references node %s", GetName(), sourceNode->GetName());
            sourceNode->SetCurrentPlayTime(uniqueData->mInternalAnimGraphInstance, timeInSeconds);
            uniqueData->SetCurrentPlayTime(timeInSeconds);
        }
    }

    /*
    // get the duration
    float AnimGraphReferenceNode::GetDuration(AnimGraphInstance* animGraphInstance) const
    {
        const UniqueData* uniqueData = static_cast<UniqueData*>( FindUniqueNodeData(animGraphInstance) );
        if (uniqueData->mAnimGraphInstance == nullptr)
            return 0.0f;

        const AnimGraphNode* sourceNode = uniqueData->mAnimGraphInstance->GetRootNode();
        if (sourceNode)
            return sourceNode->GetDuration( uniqueData->mAnimGraphInstance );

        return 0.0f;
    }


    // get the current time
    float AnimGraphReferenceNode::GetCurrentPlayTime(AnimGraphInstance* animGraphInstance) const
    {
        const UniqueData* uniqueData = static_cast<UniqueData*>( FindUniqueNodeData(animGraphInstance) );
        if (uniqueData->mAnimGraphInstance == nullptr)
            return 0.0f;

        const AnimGraphNode* refNode = uniqueData->mAnimGraphInstance->GetRootNode();
        if (refNode)
            return refNode->GetCurrentPlayTime( uniqueData->mAnimGraphInstance );

        return 0.0f;
    }


    // get the play speed
    float AnimGraphReferenceNode::GetPlaySpeed(AnimGraphInstance* animGraphInstance) const
    {
        const UniqueData* uniqueData = static_cast<UniqueData*>( FindUniqueNodeData(animGraphInstance) );
        if (uniqueData->mAnimGraphInstance == nullptr)
            return 1.0f;

        const AnimGraphNode* refNode = uniqueData->mAnimGraphInstance->GetRootNode();
        if (refNode)
            return refNode->GetPlaySpeed( uniqueData->mAnimGraphInstance );

        return 1.0f;
    }
    */


    // this function will get called to rewind motion nodes as well as states etc. to reset several settings when a state gets exited
    void AnimGraphReferenceNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        const UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueNodeData(this));
        if (uniqueData->mInternalAnimGraphInstance == nullptr)
        {
            return;
        }

        AnimGraphNode* sourceNode = uniqueData->mInternalAnimGraphInstance->GetRootNode();
        if (sourceNode)
        {
            sourceNode->Rewind(uniqueData->mInternalAnimGraphInstance);
        }
    }


    // adjust the play speed
    void AnimGraphReferenceNode::SetPlaySpeed(AnimGraphInstance* animGraphInstance, float speedFactor)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (uniqueData->mInternalAnimGraphInstance == nullptr)
        {
            return;
        }

        AnimGraphNode* sourceNode = uniqueData->mInternalAnimGraphInstance->GetRootNode();
        if (sourceNode)
        {
            uniqueData->SetPlaySpeed(speedFactor);
            return sourceNode->SetPlaySpeed(uniqueData->mInternalAnimGraphInstance, speedFactor);
        }
    }


    // update
    void AnimGraphReferenceNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (uniqueData->mInternalAnimGraphInstance == nullptr)
        {
            uniqueData->Clear();
            return;
        }

        //  uniqueData->mInternalAnimGraphInstance->Update( timePassedInSeconds );

        AnimGraphNode* sourceNode = uniqueData->mInternalAnimGraphInstance->GetRootNode();
        if (sourceNode)
        {
            sourceNode->RecursiveResetFlags(uniqueData->mInternalAnimGraphInstance);
            UpdateIncomingNode(uniqueData->mInternalAnimGraphInstance, sourceNode, timePassedInSeconds);
            uniqueData->Init(uniqueData->mInternalAnimGraphInstance, sourceNode);
        }
    }


    // topdown update
    void AnimGraphReferenceNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (uniqueData->mInternalAnimGraphInstance == nullptr)
        {
            return;
        }

        AnimGraphNode* sourceNode = uniqueData->mInternalAnimGraphInstance->GetRootNode();
        if (sourceNode)
        {
            sourceNode->PerformTopDownUpdate(uniqueData->mInternalAnimGraphInstance, timePassedInSeconds);
        }
    }


    // post update
    void AnimGraphReferenceNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // clear the event buffer
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));

        if (uniqueData->mInternalAnimGraphInstance == nullptr)
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ZeroTrajectoryDelta();
            data->ClearEventBuffer();
            return;
        }

        // post update the source node and output the events
        AnimGraphNode* sourceNode = uniqueData->mInternalAnimGraphInstance->GetRootNode();
        if (sourceNode)
        {
            //      sourceNode->IncreaseRefDataRefCount( uniqueData->mInternalAnimGraphInstance );
            sourceNode->PerformPostUpdate(uniqueData->mInternalAnimGraphInstance, timePassedInSeconds);

            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

            AnimGraphRefCountedData* sourceData = sourceNode->FindUniqueNodeData(uniqueData->mInternalAnimGraphInstance)->GetRefCountedData();
            data->SetEventBuffer(sourceData->GetEventBuffer());
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());

            //  sourceNode->DecreaseRefDataRef( uniqueData->mInternalAnimGraphInstance );
            //sourceNode->FreeIncomingRefDatas( uniqueData->mInternalAnimGraphInstance );
        }
    }


    // a node got renamed inside the animgraph
    void AnimGraphReferenceNode::OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName)
    {
        MCORE_UNUSED(animGraph);
        MCORE_UNUSED(node);
        MCORE_UNUSED(oldName);
    }


    // a node got removed inside the animgraph
    void AnimGraphReferenceNode::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        // if this isn't a node with the same name as the one we are instancing/referencing, there is nothing to do
        if (nodeToRemove->GetNameString() != GetAttributeString(ATTRIB_REFERENCENODE)->GetValue())
        {
            return;
        }

        // for all animgraph instances that are there
        const uint32 numInstances = GetAnimGraphManager().GetNumAnimGraphInstances();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            // check if this animgraph instance uses this animgraph
            AnimGraphInstance* animGraphInstance = GetAnimGraphManager().GetAnimGraphInstance(i);
            if (animGraphInstance->GetAnimGraph() != animGraph)
            {
                continue;
            }

            // get the unique data for this node in the instance
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            if (uniqueData == nullptr)
            {
                continue;
            }

            // skip when we have no animgraph instance to sample the node
            if (uniqueData->mInternalAnimGraphInstance == nullptr)
            {
                continue;
            }

            // make sure its the same node, just as extra security check
            MCORE_ASSERT(uniqueData->mInternalAnimGraphInstance->GetReferenceInfo().mSourceNode == nodeToRemove);

            // force an update of the unique data
            uniqueData->mDeletedNode = nodeToRemove;
            uniqueData->mNeedsUpdate = true;
            OnUpdateUniqueData(animGraphInstance);
        } // for all animgraph instances
    }


    // a node has been created
    void AnimGraphReferenceNode::OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node)
    {
        // if this isn't a node with the same name as the one we are instancing/referencing, there is nothing to do
        if (node->GetNameString() != GetAttributeString(ATTRIB_REFERENCENODE)->GetValue())
        {
            return;
        }

        // for all animgraph instances that are there
        const uint32 numInstances = GetAnimGraphManager().GetNumAnimGraphInstances();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            // check if this animgraph instance uses this animgraph
            AnimGraphInstance* animGraphInstance = GetAnimGraphManager().GetAnimGraphInstance(i);
            if (animGraphInstance->GetAnimGraph() != animGraph)
            {
                continue;
            }

            // get the unique data for this node in the instance
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            if (uniqueData == nullptr)
            {
                continue;
            }

            // force an update of the unique data
            uniqueData->mNeedsUpdate = true;
            OnUpdateUniqueData(animGraphInstance);
        } // for all animgraph instances
    }
}   // namespace EMotionFX
