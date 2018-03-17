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
#include "BlendTreeSmoothingNode.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"

#include <MCore/Source/AttributeSettings.h>
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    // constructor
    BlendTreeSmoothingNode::BlendTreeSmoothingNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeSmoothingNode::~BlendTreeSmoothingNode()
    {
    }


    // create
    BlendTreeSmoothingNode* BlendTreeSmoothingNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeSmoothingNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeSmoothingNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the ports
    void BlendTreeSmoothingNode::RegisterPorts()
    {
        // create the input ports
        InitInputPorts(1);
        SetupInputPortAsNumber("Dest", INPUTPORT_DEST, PORTID_INPUT_DEST);

        // create the output ports
        InitOutputPorts(1);
        SetupOutputPort("Result", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);
    }


    // register the parameters
    void BlendTreeSmoothingNode::RegisterAttributes()
    {
        // create the interpolation speed attribute
        MCore::AttributeSettings* speedAttribute = RegisterAttribute("Interpolation Speed", "interpolationSpeed", "The interpolation speed where 0.0 means the value won't change at all and 1.0 means the input value will directly be mapped to the output value.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSLIDER);
        speedAttribute->SetDefaultValue(MCore::AttributeFloat::Create(0.75f));
        speedAttribute->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        speedAttribute->SetMaxValue(MCore::AttributeFloat::Create(1.0f));

        // use start value
        MCore::AttributeSettings* useStartAttribute = RegisterAttribute("Use Start Value", "useStartValue", "Enable this to use the start value, otherwise the first input value will be used as start value.", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        useStartAttribute->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // start value
        MCore::AttributeSettings* startValueAttribute = RegisterAttribute("Start Value", "startValue", "When the blend tree gets activated the smoothing node will start interpolating from this value.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        startValueAttribute->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        startValueAttribute->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        startValueAttribute->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));
    }


    // get the palette name
    const char* BlendTreeSmoothingNode::GetPaletteName() const
    {
        return "Smoothing";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeSmoothingNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeSmoothingNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeSmoothingNode* clone = new BlendTreeSmoothingNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // get the type string
    const char* BlendTreeSmoothingNode::GetTypeString() const
    {
        return "BlendTreeSmoothingNode";
    }


    // update the data
    void BlendTreeSmoothingNode::OnUpdateAttributes()
    {
        // disable GUI items that have no influence
    #ifdef EMFX_EMSTUDIOBUILD
        // enable all attributes
        EnableAllAttributes(true);

        // if the interpolation type is linear, disable the ease-in and ease-out values
        if (GetAttributeFloatAsBool(ATTRIB_USESTARTVALUE) == false)
        {
            SetAttributeDisabled(ATTRIB_STARTVALUE);
        }
    #endif
    }


    // update
    void BlendTreeSmoothingNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all incoming nodes
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));

        // if there are no incoming connections, there is nothing to do
        if (mConnections.GetLength() == 0)
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(0.0f);
            return;
        }

        // if we are disabled, output the dest value directly
        //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_DEST) );
        const float destValue = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_DEST);
        if (mDisabled)
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(destValue);
            return;
        }

        // perform interpolation
        const float sourceValue = uniqueData->mCurrentValue;
        const float interpolationSpeed = GetAttributeFloat(ATTRIB_INTERPOLATIONSPEED)->GetValue() * uniqueData->mFrameDeltaTime * 10.0f;
        const float interpolationResult = (interpolationSpeed < 0.99999f) ? MCore::LinearInterpolate<float>(sourceValue, destValue, interpolationSpeed) : destValue;

        // pass the interpolated result to the output port and the current value of the unique data
        uniqueData->mCurrentValue = interpolationResult;
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(interpolationResult);

        uniqueData->mFrameDeltaTime = timePassedInSeconds;
    }


    // rewind
    void BlendTreeSmoothingNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data for this node, if it doesn't exist yet, create it
        UniqueData* uniqueData = static_cast<BlendTreeSmoothingNode::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // check if the current value needs to be reset to the input or the start value when rewinding the node
        const bool useStartValue = GetAttributeFloatAsBool(ATTRIB_USESTARTVALUE);
        if (useStartValue)
        {
            // get the start value from the attribute and use it as current value
            const float startValue = GetAttributeFloat(ATTRIB_STARTVALUE)->GetValue();
            uniqueData->mCurrentValue = startValue;
        }
        else
        {
            // set the current value to the current input value
            UpdateAllIncomingNodes(animGraphInstance, 0.0f);
            //      OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_DEST) );
            uniqueData->mCurrentValue = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_DEST);
        }
    }


    // when attributes have changed their value
    void BlendTreeSmoothingNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data for this node, if it doesn't exist yet, create it
        UniqueData* uniqueData = static_cast<BlendTreeSmoothingNode::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
            uniqueData->mCurrentValue = 0.0f;//GetInputNumber( INPUTPORT_DEST );
        }

        // set the source value to the current input destination value
        //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_DEST) );

        if (GetInputNode(INPUTPORT_DEST) == nullptr)
        {
            uniqueData->mCurrentValue = 0.0f;//GetInputNumber( INPUTPORT_DEST );
        }
    }



    uint32 BlendTreeSmoothingNode::GetVisualColor() const
    {
        return MCore::RGBA(255, 0, 0);
    }


    bool BlendTreeSmoothingNode::GetSupportsDisable() const
    {
        return true;
    }
} // namespace EMotionFX
