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
#include "BlendTreeTransformNode.h"
#include "AnimGraphInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphManager.h"
#include "Node.h"
#include "EMotionFXManager.h"
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    BlendTreeTransformNode::BlendTreeTransformNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeTransformNode::~BlendTreeTransformNode()
    {
    }


    // create
    BlendTreeTransformNode* BlendTreeTransformNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeTransformNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeTransformNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the ports
    void BlendTreeTransformNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(4);
        SetupInputPort("Input Pose",   INPUTPORT_POSE,             AttributePose::TYPE_ID,             PORTID_INPUT_POSE);
        SetupInputPortAsNumber("Translation",  INPUTPORT_TRANSLATE_AMOUNT, PORTID_INPUT_TRANSLATE_AMOUNT);
        SetupInputPortAsNumber("Rotation",     INPUTPORT_ROTATE_AMOUNT,    PORTID_INPUT_ROTATE_AMOUNT);
        SetupInputPortAsNumber("Scale",        INPUTPORT_SCALE_AMOUNT,     PORTID_INPUT_SCALE_AMOUNT);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void BlendTreeTransformNode::RegisterAttributes()
    {
        // the node
        MCore::AttributeSettings* attrib = RegisterAttribute("Node", "node", "The node to apply the transform to.", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetReinitObjectOnValueChange(true);
        attrib->SetDefaultValue(MCore::AttributeString::Create());

        // min translation
        attrib = RegisterAttribute("Min Translation", "minTranslation", "The minimum translation value, used when the input translation amount equals zero.", MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO);
        attrib->SetDefaultValue(MCore::AttributeVector3::Create(0.0f, 0.0f, 0.0f));
        attrib->SetMinValue(MCore::AttributeVector3::Create(-FLT_MAX, -FLT_MAX, -FLT_MAX));
        attrib->SetMaxValue(MCore::AttributeVector3::Create(FLT_MAX,  FLT_MAX,  FLT_MAX));

        // max translation
        attrib = RegisterAttribute("Max Translation", "maxTranslation", "The maximum translation value, used when the input translation amount equals one.", MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO);
        attrib->SetDefaultValue(MCore::AttributeVector3::Create(0.0f, 0.0f, 0.0f));
        attrib->SetMinValue(MCore::AttributeVector3::Create(-FLT_MAX, -FLT_MAX, -FLT_MAX));
        attrib->SetMaxValue(MCore::AttributeVector3::Create(FLT_MAX,  FLT_MAX,  FLT_MAX));

        // min rotation
        attrib = RegisterAttribute("Min Rotation", "minRotation", "The minimum rotation value, in degrees, used when the input rotation amount equals zero.", ATTRIBUTE_INTERFACETYPE_ROTATION);
        attrib->SetDefaultValue(AttributeRotation::Create(0.0f, -180.0f, 0.0f));

        // max rotation
        attrib = RegisterAttribute("Max Rotation", "maxRotation", "The maximum rotation value, in degrees, used when the input rotation amount equals one.", ATTRIBUTE_INTERFACETYPE_ROTATION);
        attrib->SetDefaultValue(AttributeRotation::Create(0.0f, 180.0f, 0.0f));

        // min scale
        attrib = RegisterAttribute("Min Scale", "minScale", "The minimum scale value, used when the input scale amount equals zero.", MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3);
        attrib->SetDefaultValue(MCore::AttributeVector3::Create(0.0f, 0.0f, 0.0f));
        attrib->SetMinValue(MCore::AttributeVector3::Create(-FLT_MAX, -FLT_MAX, -FLT_MAX));
        attrib->SetMaxValue(MCore::AttributeVector3::Create(FLT_MAX,  FLT_MAX,  FLT_MAX));

        // max scale
        attrib = RegisterAttribute("Max Scale", "maxScale", "The maximum scale value, used when the input scale amount equals one.", MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3);
        attrib->SetDefaultValue(MCore::AttributeVector3::Create(0.0f, 0.0f, 0.0f));
        attrib->SetMinValue(MCore::AttributeVector3::Create(-FLT_MAX, -FLT_MAX, -FLT_MAX));
        attrib->SetMaxValue(MCore::AttributeVector3::Create(FLT_MAX,  FLT_MAX,  FLT_MAX));
    }


    // get the palette name
    const char* BlendTreeTransformNode::GetPaletteName() const
    {
        return "Transform";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeTransformNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeTransformNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeTransformNode* clone = new BlendTreeTransformNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // perform the calculations / actions
    void BlendTreeTransformNode::Output(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        AnimGraphPose* outputPose;

        // get the unique
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        UpdateUniqueData(animGraphInstance, uniqueData);
        if (uniqueData->mIsValid == false)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(actorInstance);
        #ifdef EMFX_EMSTUDIOBUILD
            SetHasError(animGraphInstance, true);
        #endif
            return;
        }
        else
        {
        #ifdef EMFX_EMSTUDIOBUILD
            SetHasError(animGraphInstance, false);
        #endif
        }

        // make sure we have at least an input pose, otherwise output the bind pose
        if (GetInputPort(INPUTPORT_POSE).mConnection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(actorInstance);
        }
        else // init on the input pose
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            *outputPose = *inputPose;
        }

        // get the local transform from our node
        Transform inputTransform = outputPose->GetPose().GetLocalTransform(uniqueData->mNodeIndex);
        Transform outputTransform = inputTransform;

        // process the rotation
        if (GetInputPort(INPUTPORT_ROTATE_AMOUNT).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_ROTATE_AMOUNT));
            const float rotateFactor = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_ROTATE_AMOUNT), 0.0f, 1.0f);
            const MCore::Vector3& minAngles = static_cast<AttributeRotation*>(GetAttribute(ATTRIB_MINROTATE))->GetRotationAngles();
            const MCore::Vector3& maxAngles = static_cast<AttributeRotation*>(GetAttribute(ATTRIB_MAXROTATE))->GetRotationAngles();
            const MCore::Vector3 newAngles  = MCore::LinearInterpolate<MCore::Vector3>(minAngles, maxAngles, rotateFactor);
            outputTransform.mRotation = inputTransform.mRotation * MCore::Quaternion(MCore::Math::DegreesToRadians(newAngles.x), MCore::Math::DegreesToRadians(newAngles.y), MCore::Math::DegreesToRadians(newAngles.z));
        }

        // process the translation
        if (GetInputPort(INPUTPORT_TRANSLATE_AMOUNT).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_TRANSLATE_AMOUNT));
            const float factor = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_TRANSLATE_AMOUNT), 0.0f, 1.0f);
            const MCore::Vector3& minValue = GetAttributeVector3(ATTRIB_MINTRANSLATE)->GetValue();
            const MCore::Vector3& maxValue = GetAttributeVector3(ATTRIB_MAXTRANSLATE)->GetValue();
            const MCore::Vector3 newValue  = MCore::LinearInterpolate<MCore::Vector3>(minValue, maxValue, factor);
            outputTransform.mPosition = inputTransform.mPosition + newValue;
        }

        // process the scale
        EMFX_SCALECODE
        (
            if (GetInputPort(INPUTPORT_SCALE_AMOUNT).mConnection)
            {
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_SCALE_AMOUNT));
                const float factor = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_SCALE_AMOUNT), 0.0f, 1.0f);
                const MCore::Vector3& minValue = GetAttributeVector3(ATTRIB_MINSCALE)->GetValue();
                const MCore::Vector3& maxValue = GetAttributeVector3(ATTRIB_MAXSCALE)->GetValue();
                const MCore::Vector3 newValue  = MCore::LinearInterpolate<MCore::Vector3>(minValue, maxValue, factor);
                outputTransform.mScale = inputTransform.mScale + newValue;
            }
        )

        // update the transformation of the node
        outputPose->GetPose().SetLocalTransform(uniqueData->mNodeIndex, outputTransform);

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }


    // get the blend node type string
    const char* BlendTreeTransformNode::GetTypeString() const
    {
        return "BlendTreeTransformNode";
    }


    // update the unique data
    void BlendTreeTransformNode::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // update the unique data if needed
        if (uniqueData->mMustUpdate)
        {
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            Actor* actor = actorInstance->GetActor();

            uniqueData->mMustUpdate = false;
            uniqueData->mNodeIndex  = MCORE_INVALIDINDEX32;
            uniqueData->mIsValid    = false;

            // try get the node
            const char* nodeName = GetAttributeString(ATTRIB_NODE)->AsChar();
            if (nodeName == nullptr)
            {
                return;
            }

            const Node* node = actor->GetSkeleton()->FindNodeByName(nodeName);
            if (node == nullptr)
            {
                return;
            }

            uniqueData->mNodeIndex  = node->GetNodeIndex();
            uniqueData->mIsValid = true;
        }
    }


    // init
    void BlendTreeTransformNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
        //mOutputPose.Init( animGraphInstance->GetActorInstance() );
    }


    // when an attribute value changes
    void BlendTreeTransformNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find our unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        uniqueData->mMustUpdate = true;

        UpdateUniqueData(animGraphInstance, uniqueData);
    }
}   // namespace EMotionFX

