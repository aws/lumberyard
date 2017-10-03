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
#include "BlendTreeAccumTransformNode.h"
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
    BlendTreeAccumTransformNode::BlendTreeAccumTransformNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();

        mLastTranslateAxis  = 0;
        mLastRotateAxis     = 0;
        mLastScaleAxis      = 0;
    }


    // destructor
    BlendTreeAccumTransformNode::~BlendTreeAccumTransformNode()
    {
    }


    // create
    BlendTreeAccumTransformNode* BlendTreeAccumTransformNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeAccumTransformNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeAccumTransformNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the ports
    void BlendTreeAccumTransformNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(4);
        SetupInputPort("Input Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPortAsNumber("Translation",  INPUTPORT_TRANSLATE_AMOUNT, PORTID_INPUT_TRANSLATE_AMOUNT);
        SetupInputPortAsNumber("Rotation",     INPUTPORT_ROTATE_AMOUNT,    PORTID_INPUT_ROTATE_AMOUNT);
        SetupInputPortAsNumber("Scale",        INPUTPORT_SCALE_AMOUNT,     PORTID_INPUT_SCALE_AMOUNT);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void BlendTreeAccumTransformNode::RegisterAttributes()
    {
        // the node
        MCore::AttributeSettings* attrib = RegisterAttribute("Node", "node", "The node to apply the transform to.", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetDefaultValue(MCore::AttributeString::Create());
        attrib->SetReinitObjectOnValueChange(true);

        // the translation axis
        attrib = RegisterAttribute("Translation Axis", "translateAxis", "The local axis to translate along.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attrib->ResizeComboValues(3);
        attrib->SetComboValue(0, "X Axis");
        attrib->SetComboValue(1, "Y Axis");
        attrib->SetComboValue(2, "Z Axis");
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // the rotation axis
        attrib = RegisterAttribute("Rotation Axis", "rotateAxis", "The local axis to rotate around.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attrib->ResizeComboValues(3);
        attrib->SetComboValue(0, "X Axis");
        attrib->SetComboValue(1, "Y Axis");
        attrib->SetComboValue(2, "Z Axis");
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // the translation axis
        attrib = RegisterAttribute("Scaling Axis", "scaleAxis", "The local axis to scale along.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attrib->ResizeComboValues(4);
        attrib->SetComboValue(0, "All Axes (uniform scaling)");
        attrib->SetComboValue(1, "X Axis");
        attrib->SetComboValue(2, "Y Axis");
        attrib->SetComboValue(3, "Z Axis");
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // the translation speed factor
        attrib = RegisterAttribute("Translate Speed", "translateSpeed", "The translation speed factor.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        attrib->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        attrib->SetMaxValue(MCore::AttributeFloat::Create(100.0f));

        // the rotation speed factor
        attrib = RegisterAttribute("Rotate Speed", "rotateSpeed", "The rotation speed factor.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        attrib->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        attrib->SetMaxValue(MCore::AttributeFloat::Create(100.0f));

        // the scale speed factor
        attrib = RegisterAttribute("Scale Speed", "scaleSpeed", "The scale speed factor.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        attrib->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        attrib->SetMaxValue(MCore::AttributeFloat::Create(100.0f));

        // invert translation?
        attrib = RegisterAttribute("Invert Translation", "invertTranslate", "Invert the translation direction?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // invert rotation?
        attrib = RegisterAttribute("Invert Rotation", "invertRotate", "Invert the rotation direction?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // invert scale?
        attrib = RegisterAttribute("Invert Scaling", "invertScale", "Invert the scale direction?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(0));
    }


    // get the palette name
    const char* BlendTreeAccumTransformNode::GetPaletteName() const
    {
        return "AccumTransform";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeAccumTransformNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeAccumTransformNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeAccumTransformNode* clone = new BlendTreeAccumTransformNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // perform the calculations / actions
    void BlendTreeAccumTransformNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // check if we already did output
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // get the output pose
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
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
        }

        // get the local transform from our node
        Transform inputTransform;
        outputPose->GetPose().GetLocalTransform(uniqueData->mNodeIndex, &inputTransform);

        Transform outputTransform = inputTransform;

        // process the rotation
        if (GetInputPort(INPUTPORT_ROTATE_AMOUNT).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_ROTATE_AMOUNT));

            const float inputAmount = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_ROTATE_AMOUNT), 0.0f, 1.0f);
            const float factor = GetAttributeFloat(ATTRIB_ROTATE_SPEED)->GetValue();
            float invertFactor = 1.0f;
            if (GetAttributeFloatAsBool(ATTRIB_ROTATE_INVERT))
            {
                invertFactor = -1.0f;
            }

            MCore::Vector3 axis(1.0f, 0.0f, 0.0f);
            const int32 mode = GetAttributeFloatAsInt32(ATTRIB_ROTATE_AXIS);
            switch (mode)
            {
            case 0:
                axis.Set(1.0f, 0.0f, 0.0f);
                break;
            case 1:
                axis.Set(0.0f, 1.0f, 0.0f);
                break;
            case 2:
                axis.Set(0.0f, 0.0f, 1.0f);
                break;
            default:
                MCORE_ASSERT(false);
            }

            const MCore::Quaternion targetRot(axis, MCore::Math::DegreesToRadians(360.0f * (inputAmount - 0.5f) * invertFactor));
            MCore::Quaternion deltaRot = MCore::LinearInterpolate<MCore::Quaternion>(MCore::Quaternion(), targetRot, uniqueData->mDeltaTime * factor);
            deltaRot.Normalize();
            uniqueData->mAdditiveTransform.mRotation = uniqueData->mAdditiveTransform.mRotation * deltaRot;
            outputTransform.mRotation = (inputTransform.mRotation * uniqueData->mAdditiveTransform.mRotation).Normalize();
        }

        // process the translation
        if (GetInputPort(INPUTPORT_TRANSLATE_AMOUNT).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_TRANSLATE_AMOUNT));

            const float inputAmount = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_TRANSLATE_AMOUNT), 0.0f, 1.0f);
            const float factor = GetAttributeFloat(ATTRIB_TRANSLATE_SPEED)->GetValue();
            float invertFactor = 1.0f;
            if (GetAttributeFloatAsBool(ATTRIB_TRANSLATE_INVERT))
            {
                invertFactor = -1.0f;
            }

            MCore::Vector3 axis(1.0f, 0.0f, 0.0f);
            const int32 mode = GetAttributeFloatAsInt32(ATTRIB_TRANSLATE_AXIS);
            switch (mode)
            {
            case 0:
                axis.Set(1.0f, 0.0f, 0.0f);
                break;
            case 1:
                axis.Set(0.0f, 1.0f, 0.0f);
                break;
            case 2:
                axis.Set(0.0f, 0.0f, 1.0f);
                break;
            default:
                MCORE_ASSERT(false);
            }

            axis *= (inputAmount - 0.5f) * invertFactor;
            uniqueData->mAdditiveTransform.mPosition += MCore::LinearInterpolate<MCore::Vector3>(MCore::Vector3(0.0f, 0.0f, 0.0f), axis, uniqueData->mDeltaTime * factor);
            ;
            outputTransform.mPosition = inputTransform.mPosition + uniqueData->mAdditiveTransform.mPosition;
        }


        // process the scale
        EMFX_SCALECODE
        (
            if (GetInputPort(INPUTPORT_SCALE_AMOUNT).mConnection)
            {
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_SCALE_AMOUNT));

                const float inputAmount = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_SCALE_AMOUNT), 0.0f, 1.0f);
                const float factor = GetAttributeFloat(ATTRIB_SCALE_SPEED)->GetValue();
                float invertFactor = 1.0f;
                if (GetAttributeFloatAsBool(ATTRIB_SCALE_INVERT))
                {
                    invertFactor = -1.0f;
                }

                MCore::Vector3 axis(1.0f, 1.0f, 1.0f);
                const int32 mode = GetAttributeFloatAsInt32(ATTRIB_SCALE_AXIS);
                switch (mode)
                {
                case 0:
                    axis.Set(1.0f, 1.0f, 1.0f);
                    break;
                case 1:
                    axis.Set(1.0f, 0.0f, 0.0f);
                    break;
                case 2:
                    axis.Set(0.0f, 1.0f, 0.0f);
                    break;
                case 3:
                    axis.Set(0.0f, 0.0f, 1.0f);
                    break;
                default:
                    MCORE_ASSERT(false);
                }

                axis *= (inputAmount - 0.5f) * invertFactor;
                uniqueData->mAdditiveTransform.mScale += MCore::LinearInterpolate<MCore::Vector3>(MCore::Vector3(0.0f, 0.0f, 0.0f), axis, uniqueData->mDeltaTime * factor);
                outputTransform.mScale = inputTransform.mScale + uniqueData->mAdditiveTransform.mScale;
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
    const char* BlendTreeAccumTransformNode::GetTypeString() const
    {
        return "BlendTreeAccumTransformNode";
    }


    // update the unique data
    void BlendTreeAccumTransformNode::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // update the unique data if needed
        if (uniqueData->mMustUpdate)
        {
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            Actor* actor = actorInstance->GetActor();

            uniqueData->mMustUpdate = false;
            uniqueData->mNodeIndex  = MCORE_INVALIDINDEX32;
            uniqueData->mIsValid    = false;
            //      uniqueData->mAdditiveTransform.Identity();

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

            uniqueData->mNodeIndex = node->GetNodeIndex();
            uniqueData->mIsValid = true;
        }
    }


    // when an attribute value changes
    void BlendTreeAccumTransformNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        //mOutputPose.Init( animGraphInstance->GetActorInstance() );

        // find our unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        uniqueData->mMustUpdate = true;

        // if the axis settings changed
        if (mLastTranslateAxis != GetAttributeFloatAsInt32(ATTRIB_TRANSLATE_AXIS) ||
            mLastRotateAxis != GetAttributeFloatAsInt32(ATTRIB_ROTATE_AXIS) ||
            mLastScaleAxis != GetAttributeFloatAsInt32(ATTRIB_SCALE_AXIS))
        {
            mLastTranslateAxis  = GetAttributeFloatAsInt32(ATTRIB_TRANSLATE_AXIS);
            mLastScaleAxis      = GetAttributeFloatAsInt32(ATTRIB_SCALE_AXIS);
            mLastRotateAxis     = GetAttributeFloatAsInt32(ATTRIB_ROTATE_AXIS);
            uniqueData->mAdditiveTransform.Identity();

            EMFX_SCALECODE(uniqueData->mAdditiveTransform.mScale.Zero();
                )
        }
    }


    // update
    void BlendTreeAccumTransformNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all incoming nodes
        AnimGraphNode::Update(animGraphInstance, timePassedInSeconds);

        // store the passed time
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        uniqueData->mDeltaTime = timePassedInSeconds;
    }
}   // namespace EMotionFX

