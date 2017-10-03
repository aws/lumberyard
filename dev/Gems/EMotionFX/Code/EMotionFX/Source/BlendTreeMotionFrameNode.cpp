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
#include "BlendTreeMotionFrameNode.h"
#include "AnimGraphMotionNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "MotionInstance.h"
#include <MCore/Source/AttributeSettings.h>

namespace EMotionFX
{
    // constructor
    BlendTreeMotionFrameNode::BlendTreeMotionFrameNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeMotionFrameNode::~BlendTreeMotionFrameNode()
    {
    }


    // create
    BlendTreeMotionFrameNode* BlendTreeMotionFrameNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeMotionFrameNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeMotionFrameNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the ports
    void BlendTreeMotionFrameNode::RegisterPorts()
    {
        // setup input ports
        InitInputPorts(2);
        SetupInputPort("Motion", INPUTPORT_MOTION, AttributeMotionInstance::TYPE_ID, PORTID_INPUT_MOTION);
        SetupInputPortAsNumber("Time",  INPUTPORT_TIME, PORTID_INPUT_TIME);

        // link the output port value to the local pose object (it stores a pointer to the local pose)
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_RESULT);
    }


    // register the parameters
    void BlendTreeMotionFrameNode::RegisterAttributes()
    {
        // create the static value float spinner
        MCore::AttributeSettings* valueParam = RegisterAttribute("Normalized Time", "timeValue", "The normalized time value, which must be between 0 and 1. This is used when there is no connection plugged into the Time port.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        valueParam->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        valueParam->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        valueParam->SetMaxValue(MCore::AttributeFloat::Create(1.0f));
    }


    // get the palette name
    const char* BlendTreeMotionFrameNode::GetPaletteName() const
    {
        return "Motion Frame";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeMotionFrameNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeMotionFrameNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeMotionFrameNode* clone = new BlendTreeMotionFrameNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }



    // precreate unique datas
    void BlendTreeMotionFrameNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
    }


    // perform the calculations / actions
    void BlendTreeMotionFrameNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // make sure our transform buffer is large enough
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // get the output pose
        AnimGraphPose* outputPose;

        // get the motion instance object
        BlendTreeConnection* motionConnection = mInputPorts[INPUTPORT_MOTION].mConnection;
        if (motionConnection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
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

        // get the motion instance value
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_MOTION));
        MotionInstance* motionInstance = static_cast<MotionInstance*>(GetInputMotionInstance(animGraphInstance, INPUTPORT_MOTION)->GetValue());
        if (motionInstance == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
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

        // make sure the motion instance is ready for sampling
        if (motionInstance->GetIsReadyForSampling() == false)
        {
            motionInstance->InitForSampling();
        }

        // get the time value
        float timeValue = 0.0f;
        BlendTreeConnection* timeConnection = mInputPorts[INPUTPORT_TIME].mConnection;
        if (timeConnection == nullptr) // get it from the parameter value if there is no connection
        {
            timeValue = GetAttributeFloat(ATTRIB_TIME)->GetValue();
        }
        else
        {
            // get the time value and make sure it is in range
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_TIME));
            timeValue = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_TIME);
            timeValue = MCore::Clamp<float>(timeValue, 0.0f, 1.0f);
        }

        // output the transformations
        const float oldTime = motionInstance->GetCurrentTime();
        motionInstance->SetCurrentTimeNormalized(timeValue);
        motionInstance->SetPause(true);

        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();

        // init the output pose with the bind pose for safety
        outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());

        // output the pose
        motionInstance->GetMotion()->Update(&outputPose->GetPose(), &outputPose->GetPose(), motionInstance);

        // restore the time
        motionInstance->SetCurrentTime(oldTime);

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }


    // get the blend node type string
    const char* BlendTreeMotionFrameNode::GetTypeString() const
    {
        return "BlendTreeMotionFrameNode";
    }


    // post sync update
    void BlendTreeMotionFrameNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // clear the event buffer
        if (mDisabled)
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // update the time input
        BlendTreeConnection* timeConnection = mInputPorts[INPUTPORT_TIME].mConnection;
        if (timeConnection)
        {
            timeConnection->GetSourceNode()->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        // update the input motion
        BlendTreeConnection* motionConnection = mInputPorts[INPUTPORT_MOTION].mConnection;
        if (motionConnection)
        {
            motionConnection->GetSourceNode()->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();

            AnimGraphNode* sourceNode = motionConnection->GetSourceNode();
            MCORE_ASSERT(sourceNode->GetType() == AnimGraphMotionNode::TYPE_ID);
            AnimGraphMotionNode* motionNode = static_cast<AnimGraphMotionNode*>(sourceNode);

            const bool triggerEvents = motionNode->GetAttributeFloatAsBool(AnimGraphMotionNode::ATTRIB_EVENTS);
            MotionInstance* motionInstance = motionNode->FindMotionInstance(animGraphInstance);
            if (triggerEvents && motionInstance)
            {
                motionInstance->ExtractEventsNonLoop(uniqueData->mOldTime, uniqueData->mNewTime, &uniqueData->GetRefCountedData()->GetEventBuffer());
                data->GetEventBuffer().UpdateEmitters(this);
            }
        }
        else
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
        }
    }


    // default update implementation
    void BlendTreeMotionFrameNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update the time input
        BlendTreeConnection* timeConnection = mInputPorts[INPUTPORT_TIME].mConnection;
        if (timeConnection)
        {
            UpdateIncomingNode(animGraphInstance, timeConnection->GetSourceNode(), timePassedInSeconds);
        }

        // update the input motion
        BlendTreeConnection* motionConnection = mInputPorts[INPUTPORT_MOTION].mConnection;
        if (motionConnection)
        {
            UpdateIncomingNode(animGraphInstance, motionConnection->GetSourceNode(), timePassedInSeconds);
        }

        // get the time value
        float timeValue = 0.0f;
        if (timeConnection == nullptr) // get it from the parameter value if there is no connection
        {
            timeValue = GetAttributeFloat(ATTRIB_TIME)->GetValue();
        }
        else
        {
            timeValue = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_TIME);
            timeValue = MCore::Clamp<float>(timeValue, 0.0f, 1.0f);
        }

        // output the right synctrack etc
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (motionConnection)
        {
            AnimGraphNode* motionNode = motionConnection->GetSourceNode();
            uniqueData->Init(animGraphInstance, motionNode);
            uniqueData->SetCurrentPlayTime(uniqueData->GetDuration() * timeValue);

            uniqueData->mOldTime = uniqueData->mNewTime;
            uniqueData->mNewTime = uniqueData->GetDuration() * timeValue;
        }
        else
        {
            uniqueData->Clear();
        }
    }


    // when attributes have changed their value
    void BlendTreeMotionFrameNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data for this node, if it doesn't exist yet, create it
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
            uniqueData->mOldTime = 0.0f;
            uniqueData->mNewTime = 0.0f;
        }
    }
}   // namespace EMotionFX
