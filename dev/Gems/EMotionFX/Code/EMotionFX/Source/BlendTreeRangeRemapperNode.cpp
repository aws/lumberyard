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
#include "BlendTreeRangeRemapperNode.h"
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    BlendTreeRangeRemapperNode::BlendTreeRangeRemapperNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeRangeRemapperNode::~BlendTreeRangeRemapperNode()
    {
    }


    // create
    BlendTreeRangeRemapperNode* BlendTreeRangeRemapperNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeRangeRemapperNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeRangeRemapperNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeRangeRemapperNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X); // accept float/int/bool values

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Result", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);
    }


    // register the parameters
    void BlendTreeRangeRemapperNode::RegisterAttributes()
    {
        // input min
        MCore::AttributeSettings* param = RegisterAttribute("Input Min", "inputMin", "The minimum incoming value. Values smaller than this will be clipped.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        param->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        param->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));

        // input max
        param = RegisterAttribute("Input Max", "inputMax", "The maximum incoming value. Values bigger than this will be clipped.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        param->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        param->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        param->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));

        // output min
        param = RegisterAttribute("Output Min", "outputMin", "The minimum outcoming value. The minimum incoming value will be mapped to the minimum outcoming value. The output port can't hold a smaller value than this.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        param->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        param->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));

        // output max
        param = RegisterAttribute("Output Max", "outputMax", "The maximum outcoming value. The maximum incoming value will be mapped to the maximum outcoming value. The output port can't hold a bigger value than this.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        param->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        param->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        param->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));
    }


    // get the palette name
    const char* BlendTreeRangeRemapperNode::GetPaletteName() const
    {
        return "Range Remapper";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeRangeRemapperNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeRangeRemapperNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeRangeRemapperNode* clone = new BlendTreeRangeRemapperNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeRangeRemapperNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        //AnimGraphNodeData* uniqueData = animGraphInstance->FindUniqueNodeData(this);
        // if there are no incoming connections, there is nothing to do
        const uint32 numConnections = mConnections.GetLength();
        if (numConnections == 0 || mDisabled)
        {
            if (numConnections > 0) // pass the input value as output in case we are disabled
            {
                //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_X) );
                GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X));
            }

            return;
        }

        // get the input value and the input and output ranges as a float, convert if needed
        //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_X) );
        float x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);

        // output the original input, so without remapping, if this node is disabled
        if (mDisabled)
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(x);
            return;
        }

        const float minInput    = GetAttributeFloat(ATTRIB_INPUTMIN)->GetValue();
        const float maxInput    = GetAttributeFloat(ATTRIB_INPUTMAX)->GetValue();
        const float minOutput   = GetAttributeFloat(ATTRIB_OUTPUTMIN)->GetValue();
        const float maxOutput   = GetAttributeFloat(ATTRIB_OUTPUTMAX)->GetValue();

        // clamp it so that the value is in the valid input range
        x = MCore::Clamp<float>(x, minInput, maxInput);

        // apply the simple linear conversion
        float result;
        if (MCore::Math::Abs(maxInput - minInput) > MCore::Math::epsilon)
        {
            result = ((x - minInput) / (maxInput - minInput)) * (maxOutput - minOutput) + minOutput;
        }
        else
        {
            result = minOutput;
        }

        // update the output value
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(result);
    }


    // get the blend node type string
    const char* BlendTreeRangeRemapperNode::GetTypeString() const
    {
        return "BlendTreeRangeRemapperNode";
    }
}   // namespace EMotionFX

