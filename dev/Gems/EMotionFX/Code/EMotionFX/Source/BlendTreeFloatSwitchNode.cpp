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
#include "BlendTreeFloatSwitchNode.h"
#include <MCore/Source/Random.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    BlendTreeFloatSwitchNode::BlendTreeFloatSwitchNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeFloatSwitchNode::~BlendTreeFloatSwitchNode()
    {
    }


    // create
    BlendTreeFloatSwitchNode* BlendTreeFloatSwitchNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeFloatSwitchNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeFloatSwitchNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeFloatSwitchNode::RegisterPorts()
    {
        // create the input ports
        InitInputPorts(6);
        SetupInputPortAsNumber("Case 0", INPUTPORT_0, PORTID_INPUT_0);  // accept float/int/bool values
        SetupInputPortAsNumber("Case 1", INPUTPORT_1, PORTID_INPUT_1);  // accept float/int/bool values
        SetupInputPortAsNumber("Case 2", INPUTPORT_2, PORTID_INPUT_2);  // accept float/int/bool values
        SetupInputPortAsNumber("Case 3", INPUTPORT_3, PORTID_INPUT_3);  // accept float/int/bool values
        SetupInputPortAsNumber("Case 4", INPUTPORT_4, PORTID_INPUT_4);  // accept float/int/bool values
        SetupInputPortAsNumber("Decision Value", INPUTPORT_DECISION, PORTID_INPUT_DECISION);

        // create the output ports
        InitOutputPorts(1);
        SetupOutputPort("Result", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);
    }


    // register the parameters
    void BlendTreeFloatSwitchNode::RegisterAttributes()
    {
        MCore::AttributeSettings* valueParam0 = RegisterAttribute("Static Case 0", "case0Value", "Value used for case 0, when it has no input.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        valueParam0->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        valueParam0->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        valueParam0->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));

        MCore::AttributeSettings* valueParam1 = RegisterAttribute("Static Case 1", "case1Value", "Value used for case 1, when it has no input.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        valueParam1->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        valueParam1->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        valueParam1->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));

        MCore::AttributeSettings* valueParam2 = RegisterAttribute("Static Case 2", "case2Value", "Value used for case 2, when it has no input.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        valueParam2->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        valueParam2->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        valueParam2->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));

        MCore::AttributeSettings* valueParam3 = RegisterAttribute("Static Case 3", "case3Value", "Value used for case 3, when it has no input.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        valueParam3->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        valueParam3->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        valueParam3->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));

        MCore::AttributeSettings* valueParam4 = RegisterAttribute("Static Case 4", "case4Value", "Value used for case 4, when it has no input.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        valueParam4->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        valueParam4->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        valueParam4->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));
    }


    // get the palette name
    const char* BlendTreeFloatSwitchNode::GetPaletteName() const
    {
        return "Float Switch";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFloatSwitchNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_LOGIC;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeFloatSwitchNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeFloatSwitchNode* clone = new BlendTreeFloatSwitchNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeFloatSwitchNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if the decision port has no incomming connection, there is nothing we can do
        if (mInputPorts[INPUTPORT_DECISION].mConnection == nullptr)
        {
            return;
        }

        // get the index we choose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_DECISION));
        const int32 decisionValue = MCore::Clamp<int32>(GetInputNumberAsInt32(animGraphInstance, INPUTPORT_DECISION), 0, 4); // max 5 cases

        // return the value for that port
        //AnimGraphNodeData* uniqueData = animGraphInstance->FindUniqueNodeData(this);
        if (mInputPorts[INPUTPORT_0 + decisionValue].mConnection)
        {
            //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_0 + decisionValue) );
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_0 + decisionValue));
        }
        else
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(GetAttributeFloat(ATTRIB_STATICVALUE_0 + decisionValue)->GetValue());
        }
    }


    // get the type string
    const char* BlendTreeFloatSwitchNode::GetTypeString() const
    {
        return "BlendTreeFloatSwitchNode";
    }


    // an attribute has changed
    void BlendTreeFloatSwitchNode::OnUpdateAttributes()
    {
    }


    uint32 BlendTreeFloatSwitchNode::GetVisualColor() const
    {
        return MCore::RGBA(50, 200, 50);
    }
}   // namespace EMotionFX

