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
#include "BlendTreeBlendNNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphMotionNode.h"
#include "ActorInstance.h"
#include "MotionInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphManager.h"


namespace EMotionFX
{
    // constructor
    BlendTreeBlendNNode::BlendTreeBlendNNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeBlendNNode::~BlendTreeBlendNNode()
    {
    }


    // create
    BlendTreeBlendNNode* BlendTreeBlendNNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeBlendNNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeBlendNNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr, MCORE_INVALIDINDEX32, MCORE_INVALIDINDEX32);
    }


    // register the ports
    void BlendTreeBlendNNode::RegisterPorts()
    {
        // setup input ports
        InitInputPorts(11);
        SetupInputPort("Pose 0", INPUTPORT_POSE_0, AttributePose::TYPE_ID, PORTID_INPUT_POSE_0);
        SetupInputPort("Pose 1", INPUTPORT_POSE_1, AttributePose::TYPE_ID, PORTID_INPUT_POSE_1);
        SetupInputPort("Pose 2", INPUTPORT_POSE_2, AttributePose::TYPE_ID, PORTID_INPUT_POSE_2);
        SetupInputPort("Pose 3", INPUTPORT_POSE_3, AttributePose::TYPE_ID, PORTID_INPUT_POSE_3);
        SetupInputPort("Pose 4", INPUTPORT_POSE_4, AttributePose::TYPE_ID, PORTID_INPUT_POSE_4);
        SetupInputPort("Pose 5", INPUTPORT_POSE_5, AttributePose::TYPE_ID, PORTID_INPUT_POSE_5);
        SetupInputPort("Pose 6", INPUTPORT_POSE_6, AttributePose::TYPE_ID, PORTID_INPUT_POSE_6);
        SetupInputPort("Pose 7", INPUTPORT_POSE_7, AttributePose::TYPE_ID, PORTID_INPUT_POSE_7);
        SetupInputPort("Pose 8", INPUTPORT_POSE_8, AttributePose::TYPE_ID, PORTID_INPUT_POSE_8);
        SetupInputPort("Pose 9", INPUTPORT_POSE_9, AttributePose::TYPE_ID, PORTID_INPUT_POSE_9);
        SetupInputPortAsNumber("Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT); // accept float/int/bool values

        // setup output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void BlendTreeBlendNNode::RegisterAttributes()
    {
        // sync setting
        RegisterSyncAttribute();

        // event mode settings
        RegisterEventFilterAttribute();
    }


    // convert attributes for backward compatibility
    // this handles attributes that got renamed or who's types have changed during the development progress
    bool BlendTreeBlendNNode::ConvertAttribute(uint32 attributeIndex, const MCore::Attribute* attributeToConvert, const MCore::String& attributeName)
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
    const char* BlendTreeBlendNNode::GetPaletteName() const
    {
        return "Blend N";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeBlendNNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeBlendNNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeBlendNNode* clone = new BlendTreeBlendNNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // initialize (pre-allocate data)
    void BlendTreeBlendNNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
    }


    // pre-create the unique data
    void BlendTreeBlendNNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // locate the existing unique data for this node
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData( const_cast<BlendTreeBlendNNode*>(this), animGraphInstance, MCORE_INVALIDINDEX32, MCORE_INVALIDINDEX32);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }
    }


    // find the two blend nodes
    void BlendTreeBlendNNode::FindBlendNodes(AnimGraphInstance* animGraphInstance, AnimGraphNode** outNodeA, AnimGraphNode** outNodeB, uint32* outIndexA, uint32* outIndexB, float* outWeight) const
    {
        // collect the used port numbers
        uint32 numInputPoses = 0;
        uint32 portNumbers[10];
        for (uint32 i = 0; i < 10; ++i)
        {
            EMotionFX::BlendTreeConnection* connection = mInputPorts[INPUTPORT_POSE_0 + i].mConnection;
            if (connection)
            {
                portNumbers[numInputPoses] = i;
                numInputPoses++;
            }
        }

        // if there are no input poses, there is nothing else to do
        if (numInputPoses == 0)
        {
            *outNodeA  = nullptr;
            *outNodeB  = nullptr;
            *outIndexA = MCORE_INVALIDINDEX32;
            *outIndexB = MCORE_INVALIDINDEX32;
            *outWeight = 0.0f;
            return;
        }

        // get the weight value
        float weight = 0.0f;
        if (mDisabled == false)
        {
            if (mInputPorts[INPUTPORT_WEIGHT].mConnection)
            {
                weight = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_WEIGHT);
            }

            // make sure the weight value is in a valid range
            weight = MCore::Clamp<float>(weight, 0.0f, 1.0f);
        }

        // calculate how many poses are used/linked
        const float stepSize = 1.0f / (float)(numInputPoses - 1);

        // calculate the two poses to interpolate between
        const float poseFloatNumber = weight / stepSize;
        const uint32 portIndexA = (uint32)MCore::Math::Floor(poseFloatNumber);  // TODO: floor really needed?
        uint32 poseIndexA = portNumbers[portIndexA];

        // get the next pose index
        uint32 poseIndexB;
        if (portIndexA < numInputPoses - 1)
        {
            poseIndexB = portNumbers[portIndexA + 1];
        }
        else
        {
            poseIndexB = poseIndexA;
        }

        if (weight < MCore::Math::epsilon)
        {
            poseIndexB = poseIndexA;
        }

        if (weight > 1.0f - MCore::Math::epsilon)
        {
            poseIndexA = poseIndexB;
        }

        // calculate the blend weight and get the nodes
        *outWeight  = MCore::Math::FMod(poseFloatNumber, 1.0f);
        *outNodeA   = GetInputPort(INPUTPORT_POSE_0 + poseIndexA).mConnection->GetSourceNode();
        *outNodeB   = GetInputPort(INPUTPORT_POSE_0 + poseIndexB).mConnection->GetSourceNode();
        *outIndexA  = poseIndexA;
        *outIndexB  = poseIndexB;
    }


    // sync the motions
    void BlendTreeBlendNNode::SyncMotions(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, uint32 poseIndexA, uint32 poseIndexB, float blendWeight, ESyncMode syncMode)
    {
        if (nodeA == nullptr || nodeB == nullptr)
        {
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueNodeData(this));

        // check if we need to resync, this indicates the two motions we blend between changed
        bool resync = false;
        if (uniqueData->mIndexA != poseIndexA || uniqueData->mIndexB != poseIndexB)
        {
            resync = true;
            nodeA->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_RESYNC, true);
        }

        // sync to the blend node
        nodeA->AutoSync(animGraphInstance, this, 0.0f, SYNCMODE_TRACKBASED, resync, false);

        // for all input ports (10 motion input poses)
        for (uint32 i = 0; i < 10; ++i)
        {
            // check if this port is used
            BlendTreeConnection* connection = mInputPorts[i].mConnection;
            if (connection == nullptr)
            {
                continue;
            }

            // mark this node recursively as synced
            if (animGraphInstance->GetIsObjectFlagEnabled(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_SYNCED) == false)
            {
                connection->GetSourceNode()->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, true);
            }

            if (connection->GetSourceNode() == nodeA)
            {
                continue;
            }

            // get the node to sync, and check the resync flag
            AnimGraphNode* nodeToSync = connection->GetSourceNode();
            if (resync)
            {
                nodeToSync->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_RESYNC, true);
            }

            // only modify the speed of the master node once
            bool modifyMasterSpeed = false;

            // perform the syncing
            nodeToSync->AutoSync(animGraphInstance, nodeA, blendWeight, syncMode, resync, modifyMasterSpeed);
        }

        uniqueData->mIndexA     = poseIndexA;
        uniqueData->mIndexB     = poseIndexB;
    }


    // perform the calculations / actions
    void BlendTreeBlendNNode::Output(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // get the output pose
        AnimGraphPose* outputPose;

        // if there are no connections, there is nothing to do
        if (mConnections.GetLength() == 0 || mDisabled)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();

            // output a bind pose
            outputPose->InitFromBindPose(actorInstance);

            // visualize it
        #ifdef EMFX_EMSTUDIOBUILD
            if (GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
            }
        #endif

            return;
        }

        // output the input weight node
        BlendTreeConnection* connection = mInputPorts[INPUTPORT_WEIGHT].mConnection;
        if (connection)
        {
            OutputIncomingNode(animGraphInstance, connection->GetSourceNode());
        }

        // get two nodes that we receive input poses from, and get the blend weight
        float blendWeight;
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        uint32 poseIndexA;
        uint32 poseIndexB;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &poseIndexA, &poseIndexB, &blendWeight);

        // if there are no input poses, there is nothing else to do
        if (nodeA == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(actorInstance);

            // visualize it
        #ifdef EMFX_EMSTUDIOBUILD
            if (GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
            }
        #endif

            return;
        }

        // if both nodes are equal we can just output the given pose
        OutputIncomingNode(animGraphInstance, nodeA);
        const AnimGraphPose* poseA = GetInputPose(animGraphInstance, INPUTPORT_POSE_0 + poseIndexA)->GetValue();
        if (nodeA == nodeB || blendWeight < MCore::Math::epsilon || nodeB == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();

            *outputPose = *poseA;
        #ifdef EMFX_EMSTUDIOBUILD
            if (GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
            }
        #endif
            return;
        }

        // get the second pose, and check if blending is still needed
        OutputIncomingNode(animGraphInstance, nodeB);
        const AnimGraphPose* poseB = GetInputPose(animGraphInstance, INPUTPORT_POSE_0 + poseIndexB)->GetValue();
        if (blendWeight > 1.0f - MCore::Math::epsilon)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *poseB;

            // visualize it
        #ifdef EMFX_EMSTUDIOBUILD
            if (GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
            }
        #endif

            return;
        }

        // perform the blend
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *poseA;
        outputPose->Blend(poseB, blendWeight);

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }



    // get the blend node type string
    const char* BlendTreeBlendNNode::GetTypeString() const
    {
        return "BlendTreeBlendNNode";
    }


    // update
    void BlendTreeBlendNNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if the node is disabled
        if (mDisabled)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            uniqueData->Clear();
            return;
        }

        // get the input weight
        BlendTreeConnection* connection = mInputPorts[INPUTPORT_WEIGHT].mConnection;
        if (connection)
        {
            UpdateIncomingNode(animGraphInstance, connection->GetSourceNode(), timePassedInSeconds);
        }

        // get two nodes that we receive input poses from, and get the blend weight
        float blendWeight;
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        uint32 poseIndexA;
        uint32 poseIndexB;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &poseIndexA, &poseIndexB, &blendWeight);

        // if we have no input nodes
        if (nodeA == nullptr)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            uniqueData->Clear();
            return;
        }
        /*
            UpdateIncomingNode(animGraphInstance, nodeA, timePassedInSeconds);
            if (nodeB && nodeA != nodeB)
                UpdateIncomingNode(animGraphInstance, nodeB, timePassedInSeconds);
        */
        // mark all incoming pose nodes as synced, except the main motion
        const ESyncMode syncMode = (ESyncMode) static_cast<uint32>(GetAttributeFloat(ATTRIB_SYNC)->GetValue());
        const bool syncEnabled = (syncMode != SYNCMODE_DISABLED);
        if (syncEnabled)
        {
            for (uint32 i = 0; i < 10; ++i)
            {
                connection = mInputPorts[INPUTPORT_POSE_0 + i].mConnection;
                if (connection)
                {
                    UpdateIncomingNode(animGraphInstance, connection->GetSourceNode(), timePassedInSeconds);
                }
            }
        }
        else
        {
            UpdateIncomingNode(animGraphInstance, nodeA, timePassedInSeconds);
            if (nodeB && nodeA != nodeB)
            {
                UpdateIncomingNode(animGraphInstance, nodeB, timePassedInSeconds);
            }
        }


        // update the sync track
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        uniqueData->Init(animGraphInstance, nodeA);

        // output the correct play speed
        float factorA;
        float factorB;
        float playSpeed;
        //const ESyncMode syncMode = (ESyncMode)((uint32)GetAttributeFloat(ATTRIB_SYNC)->GetValue());
        AnimGraphNode::CalcSyncFactors(animGraphInstance, nodeA, nodeB, syncMode, blendWeight, &factorA, &factorB, &playSpeed);
        uniqueData->SetPlaySpeed(playSpeed * factorA);
    }


    // top down update
    void BlendTreeBlendNNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if the node is disabled
        if (mDisabled)
        {
            return;
        }

        // top down update the weight input
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        const BlendTreeConnection* con = GetInputPort(INPUTPORT_WEIGHT).mConnection;
        if (con)
        {
            con->GetSourceNode()->FindUniqueNodeData(animGraphInstance)->SetGlobalWeight(uniqueData->GetGlobalWeight());
            con->GetSourceNode()->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        }

        // get two nodes that we receive input poses from, and get the blend weight
        float blendWeight;
        AnimGraphNode* nodeA = nullptr;
        AnimGraphNode* nodeB = nullptr;
        uint32 poseIndexA;
        uint32 poseIndexB;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &poseIndexA, &poseIndexB, &blendWeight);

        // check if we want to sync the motions
        const ESyncMode syncMode = (ESyncMode)((uint32)GetAttributeFloat(ATTRIB_SYNC)->GetValue());
        if (nodeA)
        {
            if (syncMode != SYNCMODE_DISABLED)
            {
                SyncMotions(animGraphInstance, nodeA, nodeB, poseIndexA, poseIndexB, blendWeight, syncMode);
            }
            else
            {
                nodeA->SetPlaySpeed(animGraphInstance, uniqueData->GetPlaySpeed() /* * nodeA->GetInternalPlaySpeed(animGraphInstance)*/);
                if (animGraphInstance->GetIsObjectFlagEnabled(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCED))
                {
                    nodeA->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, false);
                }
            }

            nodeA->FindUniqueNodeData(animGraphInstance)->SetGlobalWeight(uniqueData->GetGlobalWeight() * (1.0f - blendWeight));
            nodeA->FindUniqueNodeData(animGraphInstance)->SetLocalWeight(1.0f - blendWeight);
        }

        if (nodeB)
        {
            if (syncMode == SYNCMODE_DISABLED)
            {
                nodeB->SetPlaySpeed(animGraphInstance, uniqueData->GetPlaySpeed() /* * nodeB->GetInternalPlaySpeed(animGraphInstance)*/);
                if (animGraphInstance->GetIsObjectFlagEnabled(nodeB->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCED))
                {
                    nodeB->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, false);
                }
            }

            nodeB->FindUniqueNodeData(animGraphInstance)->SetGlobalWeight(uniqueData->GetGlobalWeight() * blendWeight);
            nodeB->FindUniqueNodeData(animGraphInstance)->SetLocalWeight(blendWeight);
        }

        if (nodeA && nodeA == nodeB)
        {
            if (blendWeight < MCore::Math::epsilon)
            {
                nodeA->FindUniqueNodeData(animGraphInstance)->SetGlobalWeight(uniqueData->GetGlobalWeight());
                nodeA->FindUniqueNodeData(animGraphInstance)->SetLocalWeight(1.0f);
            }
            else
            {
                if (blendWeight > 1.0f - MCore::Math::epsilon)
                {
                    nodeA->FindUniqueNodeData(animGraphInstance)->SetGlobalWeight(0.0f);
                    nodeA->FindUniqueNodeData(animGraphInstance)->SetLocalWeight(0.0f);
                }
            }
        }

        // top down update all inputs
        for (uint32 i = 0; i < 10; ++i)
        {
            EMotionFX::BlendTreeConnection* connection = mInputPorts[INPUTPORT_POSE_0 + i].mConnection;
            if (connection)
            {
                connection->GetSourceNode()->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
            }
        }
    }


    // post sync update
    void BlendTreeBlendNNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if we don't have enough inputs or are disabled, we don't need to update anything
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

        // get the input weight
        BlendTreeConnection* connection = mInputPorts[INPUTPORT_WEIGHT].mConnection;
        if (connection)
        {
            connection->GetSourceNode()->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        // get two nodes that we receive input poses from, and get the blend weight
        float blendWeight;
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        uint32 poseIndexA;
        uint32 poseIndexB;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &poseIndexA, &poseIndexB, &blendWeight);

        // if we have no input nodes
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

        FilterEvents(animGraphInstance, (EEventMode)((uint32)GetAttributeFloat(ATTRIB_EVENTMODE)->GetValue()), nodeA, nodeB, blendWeight, data);

        // if we have just one input node
        if (nodeA == nodeB || nodeB == nullptr)
        {
            AnimGraphRefCountedData* sourceData = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
            return;
        }

        // extract motion from both
        Transform trajectoryDeltaA;
        Transform trajectoryDeltaB;
        Transform motionExtractDeltaA;
        Transform motionExtractDeltaB;

        AnimGraphRefCountedData* nodeAData = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
        AnimGraphRefCountedData* nodeBData = nodeB->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();

        // blend the results
        Transform delta = nodeAData->GetTrajectoryDelta();
        delta.Blend(nodeBData->GetTrajectoryDelta(), blendWeight);
        data->SetTrajectoryDelta(delta);

        // blend the mirrored results
        delta = nodeAData->GetTrajectoryDeltaMirrored();
        delta.Blend(nodeBData->GetTrajectoryDeltaMirrored(), blendWeight);
        data->SetTrajectoryDeltaMirrored(delta);
    }
}   // namespace EMotionFX
