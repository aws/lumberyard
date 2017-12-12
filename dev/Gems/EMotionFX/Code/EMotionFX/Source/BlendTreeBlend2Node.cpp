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
#include "BlendTreeBlend2Node.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "AnimGraphMotionNode.h"
#include "ActorInstance.h"
#include "MotionInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "TransformData.h"
#include "Node.h"
#include "AnimGraph.h"
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    BlendTreeBlend2Node::BlendTreeBlend2Node(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeBlend2Node::~BlendTreeBlend2Node()
    {
    }


    // create
    BlendTreeBlend2Node* BlendTreeBlend2Node::Create(AnimGraph* animGraph)
    {
        return new BlendTreeBlend2Node(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeBlend2Node::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the ports
    void BlendTreeBlend2Node::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(3);
        SetupInputPort          ("Pose 1", INPUTPORT_POSE_A, AttributePose::TYPE_ID, PORTID_INPUT_POSE_A);
        SetupInputPort          ("Pose 2", INPUTPORT_POSE_B, AttributePose::TYPE_ID, PORTID_INPUT_POSE_B);
        SetupInputPortAsNumber  ("Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT);  // accept float/int/bool values

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void BlendTreeBlend2Node::RegisterAttributes()
    {
        // sync setting
        RegisterSyncAttribute();

        // event mode
        RegisterEventFilterAttribute();

        // mask
        MCore::AttributeSettings* attrib = RegisterAttribute("Mask", "mask", "The mask to apply on the Pose 1 input port.", ATTRIBUTE_INTERFACETYPE_NODENAMES);
        attrib->SetDefaultValue(AttributeNodeMask::Create());

        // additive blending?
        attrib = RegisterAttribute("Additive Blend", "additive", "Additive blending?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(0));
    }


    // convert attributes for backward compatibility
    // this handles attributes that got renamed or who's types have changed during the development progress
    bool BlendTreeBlend2Node::ConvertAttribute(uint32 attributeIndex, const MCore::Attribute* attributeToConvert, const MCore::String& attributeName)
    {
        // convert things by the base class
        const bool result = AnimGraphObject::ConvertAttribute(attributeIndex, attributeToConvert, attributeName);

        // if we try to convert the old syncMotions setting
        // we renamed the 'syncMotions' into 'sync' and also changed the type from a bool to integer
        if (attributeName == "syncMotions" && attributeIndex == MCORE_INVALIDINDEX32) // the invalid index means it doesn't exist in the current attributes list
        {
            // if its a boolean
            if (attributeToConvert->GetType() == MCore::AttributeBool::TYPE_ID)
            {
                const ESyncMode syncMode = (static_cast<const MCore::AttributeBool*>(attributeToConvert)->GetValue()) ? SYNCMODE_TRACKBASED : SYNCMODE_DISABLED;
                GetAttributeFloat(ATTRIB_SYNC)->SetValue(static_cast<float>(syncMode));
                return true;
            }
        }

        return result;
    }



    // get the palette name
    const char* BlendTreeBlend2Node::GetPaletteName() const
    {
        return "Blend Two";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeBlend2Node::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeBlend2Node::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeBlend2Node* clone = new BlendTreeBlend2Node(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // get the blend nodes
    void BlendTreeBlend2Node::FindBlendNodes(AnimGraphInstance* animGraphInstance, AnimGraphNode** outBlendNodeA, AnimGraphNode** outBlendNodeB, float* outWeight, bool isAdditive)
    {
        // if we have no input poses
        BlendTreeConnection* connectionA = mInputPorts[INPUTPORT_POSE_A].mConnection;
        BlendTreeConnection* connectionB = mInputPorts[INPUTPORT_POSE_B].mConnection;
        if (connectionA == nullptr && connectionB == nullptr)
        {
            *outBlendNodeA  = nullptr;
            *outBlendNodeB  = nullptr;
            *outWeight      = 0.0f;
            return;
        }

        // check if we have both nodes
        if (connectionA && connectionB)
        {
            // get the weight
            *outWeight = (mInputPorts[INPUTPORT_WEIGHT].mConnection) ? GetInputNumberAsFloat(animGraphInstance, INPUTPORT_WEIGHT) : 0.0f;
            *outWeight = MCore::Clamp<float>(*outWeight, 0.0f, 1.0f);

            UniqueData* uniqueData = static_cast<BlendTreeBlend2Node::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
            if (uniqueData->mMask.GetLength() > 0)
            {
                *outBlendNodeA  = connectionA->GetSourceNode();
                *outBlendNodeB  = connectionB->GetSourceNode();
                return;
            }

            if (*outWeight < MCore::Math::epsilon)
            {
                *outBlendNodeA  = connectionA->GetSourceNode();
                *outBlendNodeB  = nullptr;
            }
            else
            if ((*outWeight < 1.0f - MCore::Math::epsilon) || isAdditive)
            {
                *outBlendNodeA  = connectionA->GetSourceNode();
                *outBlendNodeB  = connectionB->GetSourceNode();
            }
            else
            {
                *outBlendNodeA  = connectionB->GetSourceNode();
                *outBlendNodeB  = nullptr;
                *outWeight      = 0.0f;
            }
            return;
        }

        // we have just one input pose, find out which one
        *outBlendNodeA  = (connectionA) ? connectionA->GetSourceNode() : connectionB->GetSourceNode();
        *outBlendNodeB  = nullptr;
        *outWeight      = 1.0f;
    }


    // pre-create the unique data
    void BlendTreeBlend2Node::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data for this node, if it doesn't exist yet, create it
        UniqueData* uniqueData = static_cast<BlendTreeBlend2Node::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(BlendTreeBlend2Node::TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        uniqueData->mMustUpdate = true;
        UpdateUniqueData(animGraphInstance, uniqueData);
    }


    // init (pre-allocate data)
    void BlendTreeBlend2Node::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
        //mOutputPose.Init( animGraphInstance->GetActorInstance() );
    }


    // default update implementation
    void BlendTreeBlend2Node::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if we disabled this node, do nothing
        if (mDisabled)
        {
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        // update the weight node
        AnimGraphNode* weightNode = GetInputNode(INPUTPORT_WEIGHT);
        if (weightNode)
        {
            UpdateIncomingNode(animGraphInstance, weightNode, timePassedInSeconds);
        }

        const bool isAdditive = GetAttributeFloatAsBool(ATTRIB_ADDITIVE);

        // get the input nodes
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, isAdditive);

        // mark the nodes as being synced if needed
        if (nodeA == nullptr)
        {
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        // update the first node
        animGraphInstance->SetObjectFlags(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_IS_SYNCMASTER, true);
        UpdateIncomingNode(animGraphInstance, nodeA, timePassedInSeconds);

        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        uniqueData->Init(animGraphInstance, nodeA);

        // update the second node
        if (nodeB && nodeA != nodeB)
        {
            UpdateIncomingNode(animGraphInstance, nodeB, timePassedInSeconds);

            // output the correct play speed
            float factorA;
            float factorB;
            float playSpeed;
            const ESyncMode syncMode = (ESyncMode)((uint32)GetAttributeFloat(ATTRIB_SYNC)->GetValue());
            AnimGraphNode::CalcSyncFactors(animGraphInstance, nodeA, nodeB, syncMode, weight, &factorA, &factorB, &playSpeed);
            uniqueData->SetPlaySpeed(playSpeed * factorA);
        }
    }



    // perform the calculations / actions
    void BlendTreeBlend2Node::Output(AnimGraphInstance* animGraphInstance)
    {
        // if we disabled this blend node, simply output a bind pose
        if (mDisabled)
        {
            // request poses to use from the pool, so that all output pose ports have a valid pose to output to we reuse them using a pool system to save memory
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        UpdateUniqueData(animGraphInstance, uniqueData);

        // output the weight node
        AnimGraphNode* weightNode = GetInputNode(INPUTPORT_WEIGHT);
        if (weightNode)
        {
            OutputIncomingNode(animGraphInstance, weightNode);
        }

        // get the number of nodes inside the mask and chose the path to go
        const uint32 numNodes = uniqueData->mMask.GetLength();
        if (numNodes == 0)
        {
            OutputNoFeathering(animGraphInstance);
        }
        else
        {
            OutputFeathering(animGraphInstance, uniqueData);
        }

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }


    // perform the calculations / actions
    void BlendTreeBlend2Node::OutputNoFeathering(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // get the output pose
        AnimGraphPose* outputPose;

        const bool isAdditive = GetAttributeFloatAsBool(ATTRIB_ADDITIVE);

        // get the input nodes
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, isAdditive);

        // check if we have three incoming connections, if not, we can't really continue
        if (nodeA == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        // if there is only one pose, we just output that one
        if (nodeB == nullptr || weight < MCore::Math::epsilon)
        {
            OutputIncomingNode(animGraphInstance, nodeA);

            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
            return;
        }

        // if we do not have an additive blend, but a normal one
        if (!isAdditive)
        {
            if (weight < 1.0f - MCore::Math::epsilon) // we are not near 1.0 weight
            {
                // output the nodes
                OutputIncomingNode(animGraphInstance, nodeA);
                OutputIncomingNode(animGraphInstance, nodeB);

                // blend the two poses
                RequestPoses(animGraphInstance);
                outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
                *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
                outputPose->Blend(nodeB->GetMainOutputPose(animGraphInstance), weight);
            }
            else
            {
                OutputIncomingNode(animGraphInstance, nodeB);
                RequestPoses(animGraphInstance);
                outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
                *outputPose = *nodeB->GetMainOutputPose(animGraphInstance);
            }
        }
        else // additive
        {
            // output the nodes
            OutputIncomingNode(animGraphInstance, nodeA);
            OutputIncomingNode(animGraphInstance, nodeB);

            // blend them
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
            outputPose->BlendAdditive(nodeB->GetMainOutputPose(animGraphInstance), weight);
        }
    }


    // perform the calculations / actions
    void BlendTreeBlend2Node::OutputFeathering(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // get the output pose
        AnimGraphPose* outputPose;

        const bool isAdditive = GetAttributeFloatAsBool(ATTRIB_ADDITIVE);

        // get the input nodes
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float blendWeight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &blendWeight, isAdditive);

        // check if we have three incoming connections, if not, we can't really continue
        if (nodeA == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        // get the local pose from the first port
        OutputIncomingNode(animGraphInstance, nodeA);
        if (nodeB == nullptr || blendWeight < MCore::Math::epsilon)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
            return;
        }


        // now go over all nodes in the second pose
        OutputIncomingNode(animGraphInstance, nodeB);
        const AnimGraphPose* maskPose = nodeB->GetMainOutputPose(animGraphInstance);
        const Pose& localMaskPose = maskPose->GetPose();

        // init the output pose to the input pose
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
        Pose& outputLocalPose = outputPose->GetPose(); // a shortcut to the output pose

        // get the number of nodes inside the mask and default them to all nodes in the local pose in case there aren't any selected
        uint32 numNodes = uniqueData->mMask.GetLength();
        if (numNodes > 0)
        {
            Transform transform;

            // if we don't want to additively blend
            if (!isAdditive)
            {
                // for all nodes in the mask, output their transforms
                for (uint32 n = 0; n < numNodes; ++n)
                {
                    const float finalWeight = blendWeight /* * uniqueData->mWeights[n]*/;
                    const uint32 nodeIndex = uniqueData->mMask[n];
                    transform = outputLocalPose.GetLocalTransform(nodeIndex);
                    transform.Blend(localMaskPose.GetLocalTransform(nodeIndex), finalWeight);
                    outputLocalPose.SetLocalTransform(nodeIndex, transform);
                }
            }
            else
            {
                // for all nodes in the mask, output their transforms
                const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
                const Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();

                for (uint32 n = 0; n < numNodes; ++n)
                {
                    const float finalWeight = blendWeight /* * uniqueData->mWeights[n]*/;
                    const uint32 nodeIndex = uniqueData->mMask[n];
                    transform = outputLocalPose.GetLocalTransform(nodeIndex);
                    transform.BlendAdditive(localMaskPose.GetLocalTransform(nodeIndex), bindPose->GetLocalTransform(nodeIndex), finalWeight);
                    outputLocalPose.SetLocalTransform(nodeIndex, transform);
                }
            }
        }
    }


    void BlendTreeBlend2Node::UpdateMotionExtractionDeltaNoFeathering(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float weight, UniqueData* uniqueData)
    {
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ZeroTrajectoryDelta();

        // we have no second input pose
        if (nodeB == nullptr || weight < MCore::Math::epsilon)
        {
            AnimGraphRefCountedData* sourceData = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();

            Transform trajectoryDeltaA;
            trajectoryDeltaA = sourceData->GetTrajectoryDelta();
            data->SetTrajectoryDelta(trajectoryDeltaA);
            trajectoryDeltaA = sourceData->GetTrajectoryDeltaMirrored();
            data->SetTrajectoryDeltaMirrored(trajectoryDeltaA);
            return;
        }

        const bool isAdditive = GetAttributeFloatAsBool(ATTRIB_ADDITIVE);
        if (weight < 1.0f - MCore::Math::epsilon)
        {
            AnimGraphRefCountedData* nodeAData = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
            AnimGraphRefCountedData* nodeBData = nodeB->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();

            // extract motion from both
            Transform trajectoryDeltaA;
            Transform trajectoryDeltaB;
            Transform trajectoryDeltaAMirrored;
            Transform trajectoryDeltaBMirrored;
            trajectoryDeltaA = nodeAData->GetTrajectoryDelta();
            trajectoryDeltaB = nodeBData->GetTrajectoryDelta();
            trajectoryDeltaAMirrored = nodeAData->GetTrajectoryDeltaMirrored();
            trajectoryDeltaBMirrored = nodeBData->GetTrajectoryDeltaMirrored();

            // blend the results
            if (!isAdditive)
            {
                trajectoryDeltaA.Blend(trajectoryDeltaB, weight);
                data->SetTrajectoryDelta(trajectoryDeltaA);

                trajectoryDeltaAMirrored.Blend(trajectoryDeltaBMirrored, weight);
                data->SetTrajectoryDeltaMirrored(trajectoryDeltaAMirrored);
            }
            else // additive blend
            {
                const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
                const Actor* actor = actorInstance->GetActor();
                const Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();

                if (actor->GetMotionExtractionNodeIndex() != MCORE_INVALIDINDEX32)
                {
                    trajectoryDeltaA.BlendAdditive(trajectoryDeltaB, bindPose->GetLocalTransform(actor->GetMotionExtractionNodeIndex()), weight);
                    data->SetTrajectoryDelta(trajectoryDeltaA);

                    trajectoryDeltaAMirrored.BlendAdditive(trajectoryDeltaBMirrored, bindPose->GetLocalTransform(actor->GetMotionExtractionNodeIndex()), weight);
                    data->SetTrajectoryDeltaMirrored(trajectoryDeltaAMirrored);
                }
            }
        }
        else // the weight is near 1.0, use a special optimization case
        {
            if (!isAdditive)
            {
                AnimGraphRefCountedData* nodeBData = nodeB->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();

                Transform trajectoryDeltaB;
                trajectoryDeltaB = nodeBData->GetTrajectoryDelta();
                data->SetTrajectoryDelta(trajectoryDeltaB);

                trajectoryDeltaB = nodeBData->GetTrajectoryDeltaMirrored();
                data->SetTrajectoryDeltaMirrored(trajectoryDeltaB);
            }
            else // additive blend
            {
                AnimGraphRefCountedData* nodeAData = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
                AnimGraphRefCountedData* nodeBData = nodeB->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();

                // extract motion from both
                Transform trajectoryDeltaA;
                Transform trajectoryDeltaB;
                Transform trajectoryDeltaAMirrored;
                Transform trajectoryDeltaBMirrored;
                trajectoryDeltaA = nodeAData->GetTrajectoryDelta();
                trajectoryDeltaB = nodeBData->GetTrajectoryDelta();
                trajectoryDeltaAMirrored = nodeAData->GetTrajectoryDeltaMirrored();
                trajectoryDeltaBMirrored = nodeBData->GetTrajectoryDeltaMirrored();

                const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
                const Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();
                const Actor* actor = actorInstance->GetActor();
                if (actor->GetMotionExtractionNodeIndex() != MCORE_INVALIDINDEX32)
                {
                    trajectoryDeltaA.BlendAdditive(trajectoryDeltaB, bindPose->GetLocalTransform(actor->GetMotionExtractionNodeIndex()), weight);
                    data->SetTrajectoryDelta(trajectoryDeltaA);

                    trajectoryDeltaAMirrored.BlendAdditive(trajectoryDeltaBMirrored, bindPose->GetLocalTransform(actor->GetMotionExtractionNodeIndex()), weight);
                    data->SetTrajectoryDeltaMirrored(trajectoryDeltaAMirrored);
                }
            }
        }
    }


    void BlendTreeBlend2Node::UpdateMotionExtractionDeltaFeathering(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float weight, UniqueData* uniqueData)
    {
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

        data->ZeroTrajectoryDelta();

        // we have no second input pose
        if (nodeB == nullptr || weight < MCore::Math::epsilon)
        {
            AnimGraphRefCountedData* nodeAData = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();

            Transform trajectoryDeltaA;
            trajectoryDeltaA = nodeAData->GetTrajectoryDelta();
            data->SetTrajectoryDelta(trajectoryDeltaA);
            trajectoryDeltaA = nodeAData->GetTrajectoryDeltaMirrored();
            data->SetTrajectoryDeltaMirrored(trajectoryDeltaA);
            return;
        }

        AnimGraphRefCountedData* nodeAData = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
        Transform trajectoryDeltaA = nodeAData->GetTrajectoryDelta();
        Transform trajectoryDeltaAMirrored = nodeAData->GetTrajectoryDeltaMirrored();
        data->SetTrajectoryDelta(trajectoryDeltaA);
        data->SetTrajectoryDeltaMirrored(trajectoryDeltaAMirrored);
    }


    // get the blend node type string
    const char* BlendTreeBlend2Node::GetTypeString() const
    {
        return "BlendTreeBlend2Node";
    }


    // when the attributes change
    void BlendTreeBlend2Node::OnUpdateAttributes()
    {
        // check if there are any attribute values and return directly if not
        if (mAttributeValues.GetIsEmpty()) // TODO: why is this needed?
        {
            return;
        }

        // TODO: this is not a good and efficient way to do this!
        // get the number of anim graph instances and iterate through them
        const uint32 numAnimGraphInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (uint32 b = 0; b < numAnimGraphInstances; ++b)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(b);

            UniqueData* uniqueData = reinterpret_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
            if (uniqueData)
            {
                uniqueData->mMustUpdate = true;
            }
        }
    }


    // update the unique data
    void BlendTreeBlend2Node::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // update the unique data if needed
        if (uniqueData->mMustUpdate)
        {
            // for all mask ports
            Actor* actor = animGraphInstance->GetActorInstance()->GetActor();

            // get the array of strings with node names
            AttributeNodeMask* attrib = static_cast<AttributeNodeMask*>(GetAttribute(ATTRIB_MASK));
            const uint32 numNodes = attrib->GetNumNodes();
            if (numNodes == 0)
            {
                uniqueData->mMask.Clear(false);
                uniqueData->mMustUpdate = false;
                return;
            }

            // update the unique data
            MCore::Array<uint32>& nodeIndices = uniqueData->mMask;
            nodeIndices.Clear(false);
            nodeIndices.Reserve(numNodes);

            // for all nodes in the mask, try to find the related nodes inside the actor
            Skeleton* skeleton = actor->GetSkeleton();
            for (uint32 a = 0; a < numNodes; ++a)
            {
                Node* node = skeleton->FindNodeByName(attrib->GetNode(a).mName.AsChar());
                if (node)
                {
                    nodeIndices.Add(node->GetNodeIndex());
                }
            }

            // don't update the next time again
            uniqueData->mMustUpdate = false;
        }
    }


    // top down update
    void BlendTreeBlend2Node::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if we disabled this node, do nothing
        if (mDisabled)
        {
            return;
        }

        // top down update the weight input
        UniqueData* uniqueData = static_cast<BlendTreeBlend2Node::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        const BlendTreeConnection* con = GetInputPort(INPUTPORT_WEIGHT).mConnection;
        if (con)
        {
            AnimGraphNodeData* sourceNodeUniqueData = con->GetSourceNode()->FindUniqueNodeData(animGraphInstance);
            sourceNodeUniqueData->SetGlobalWeight(uniqueData->GetGlobalWeight());
            sourceNodeUniqueData->SetLocalWeight(1.0f);
            con->GetSourceNode()->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        }

        const bool isAdditive = GetAttributeFloatAsBool(ATTRIB_ADDITIVE);

        // get the input nodes
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, isAdditive);

        // check if we have three incoming connections, if not, we can't really continue
        if (nodeA == nullptr)
        {
            return;
        }

        // if we want to sync the motions
        const ESyncMode syncMode = (ESyncMode)((uint32)GetAttributeFloat(ATTRIB_SYNC)->GetValue());
        if (syncMode != SYNCMODE_DISABLED)
        {
            const bool resync = (uniqueData->mSyncTrackNode != nodeA);
            if (resync)
            {
                nodeA->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_RESYNC, true);
                if (nodeB)
                {
                    nodeB->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_RESYNC, true);
                }

                uniqueData->mSyncTrackNode = nodeA;
            }

            // sync the to the blend node
            nodeA->AutoSync(animGraphInstance, this, 0.0f, SYNCMODE_TRACKBASED, false, false);

            // sync the other motion
            for (uint32 i = 0; i < 2; ++i)
            {
                // check if this port is used
                BlendTreeConnection* connection = mInputPorts[INPUTPORT_POSE_A + i].mConnection;
                if (connection == nullptr)
                {
                    continue;
                }

                // mark this node recursively as synced
                if (animGraphInstance->GetIsObjectFlagEnabled(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_SYNCED) == false)
                {
                    connection->GetSourceNode()->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, true);
                }

                AnimGraphNode* nodeToSync = connection->GetSourceNode();
                if (nodeToSync == nodeA)
                {
                    continue;
                }

                // sync the node to the master node
                nodeToSync->AutoSync(animGraphInstance, nodeA, weight, syncMode, false, false);
            }
        }
        else
        {
            // default input nodes speed propagation in case they are not synced
            nodeA->SetPlaySpeed(animGraphInstance, uniqueData->GetPlaySpeed());

            if (animGraphInstance->GetIsObjectFlagEnabled(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCED))
            {
                nodeA->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, false);
            }

            if (nodeB)
            {
                nodeB->SetPlaySpeed(animGraphInstance, uniqueData->GetPlaySpeed());
                if (animGraphInstance->GetIsObjectFlagEnabled(nodeB->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCED))
                {
                    nodeB->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, false);
                }
            }
        }

        AnimGraphNodeData* uniqueDataNodeA = nodeA->FindUniqueNodeData(animGraphInstance);
        uniqueDataNodeA->SetGlobalWeight(uniqueData->GetGlobalWeight() * (1.0f - weight));
        uniqueDataNodeA->SetLocalWeight(1.0f - weight);
        nodeA->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        if (nodeB && nodeA != nodeB)
        {
            AnimGraphNodeData* uniqueDataNodeB = nodeB->FindUniqueNodeData(animGraphInstance);
            uniqueDataNodeB->SetGlobalWeight(uniqueData->GetGlobalWeight() * weight);
            uniqueDataNodeB->SetLocalWeight(weight);
            nodeB->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        }
    }



    // post sync update
    void BlendTreeBlend2Node::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if we disabled this node
        if (mDisabled)
        {
            // request the reference counted data inside the unique data
            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // top down update the weight input
        const BlendTreeConnection* con = GetInputPort(INPUTPORT_WEIGHT).mConnection;
        if (con)
        {
            con->GetSourceNode()->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        const bool isAdditive = GetAttributeFloatAsBool(ATTRIB_ADDITIVE);

        // get the input nodes
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, isAdditive);

        // check if we have three incoming connections, if not, we can't really continue
        if (nodeA == nullptr)
        {
            // request the reference counted data inside the unique data
            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        nodeA->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        if (nodeB && nodeA != nodeB)
        {
            nodeB->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        // request the reference counted data inside the unique data
        RequestRefDatas(animGraphInstance);
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ClearEventBuffer();
        data->ZeroTrajectoryDelta();

        // pass along the event buffer of the master node
        FilterEvents(animGraphInstance, (EEventMode)((uint32)GetAttributeFloat(ATTRIB_EVENTMODE)->GetValue()), nodeA, nodeB, weight, data);

        // get the number of nodes inside the mask and chose the path to go
        const uint32 numNodes = uniqueData->mMask.GetLength();
        if (numNodes == 0)
        {
            UpdateMotionExtractionDeltaNoFeathering(animGraphInstance, nodeA, nodeB, weight, uniqueData);
        }
        else
        {
            UpdateMotionExtractionDeltaFeathering(animGraphInstance, nodeA, nodeB, weight, uniqueData);
        }
    }
} // namespace EMotionFX

