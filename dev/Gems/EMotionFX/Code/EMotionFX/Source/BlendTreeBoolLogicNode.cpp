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
#include "BlendTreeBoolLogicNode.h"
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    BlendTreeBoolLogicNode::BlendTreeBoolLogicNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();

        // default on logic AND
        mFunctionEnum   = FUNCTION_AND;
        mFunction       = BoolLogicAND;
        SetNodeInfo("x AND y");
    }


    // destructor
    BlendTreeBoolLogicNode::~BlendTreeBoolLogicNode()
    {
    }


    // create
    BlendTreeBoolLogicNode* BlendTreeBoolLogicNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeBoolLogicNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeBoolLogicNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeBoolLogicNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPort("x", INPUTPORT_X, MCore::AttributeFloat::TYPE_ID, PORTID_INPUT_X);
        SetupInputPort("y", INPUTPORT_Y, MCore::AttributeFloat::TYPE_ID, PORTID_INPUT_Y);

        // setup the output ports
        InitOutputPorts(2);
        SetupOutputPort("Float", OUTPUTPORT_VALUE,  MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_VALUE);
        SetupOutputPort("Bool", OUTPUTPORT_BOOL,    MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_BOOL);
    }


    // register the parameters
    void BlendTreeBoolLogicNode::RegisterAttributes()
    {
        // create the function combobox
        MCore::AttributeSettings* functionParam = RegisterAttribute("Logic Function", "logicFunction", "The logic function to use.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        functionParam->SetReinitGuiOnValueChange(true);
        functionParam->ResizeComboValues((uint32)NUM_FUNCTIONS);
        functionParam->SetComboValue(FUNCTION_AND,  "AND");
        functionParam->SetComboValue(FUNCTION_OR,   "OR");
        functionParam->SetComboValue(FUNCTION_XOR,  "XOR");
        functionParam->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // create the static value float spinner
        MCore::AttributeSettings* valueParam = RegisterAttribute("Static Value", "staticValue", "Value used for x or y when the input port has no connection.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        valueParam->ResizeComboValues(2);
        valueParam->SetComboValue(0, "False");
        valueParam->SetComboValue(1, "True");
        valueParam->SetDefaultValue(MCore::AttributeFloat::Create(0));

        MCore::AttributeSettings* trueValueParam = RegisterAttribute("Float Result When True", "trueResult", "The float value returned when the expression is true.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        trueValueParam->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        trueValueParam->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        trueValueParam->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));

        MCore::AttributeSettings* falseValueParam = RegisterAttribute("Float Result When False", "falseResult", "The float value returned when the expression is false.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        falseValueParam->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        falseValueParam->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        falseValueParam->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));
    }


    // get the palette name
    const char* BlendTreeBoolLogicNode::GetPaletteName() const
    {
        return "Bool Logic";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeBoolLogicNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_LOGIC;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeBoolLogicNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeBoolLogicNode* clone = new BlendTreeBoolLogicNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeBoolLogicNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if there are no incoming connections, there is nothing to do
        if (mConnections.GetLength() == 0)
        {
            return;
        }

        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // update the condition function if needed
        SetFunction((EFunction)((uint32)GetAttributeFloat(ATTRIB_FUNCTION)->GetValue()));

        // if both x and y inputs have connections
        bool x, y;
        if (mConnections.GetLength() == 2)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_Y));

            x = GetInputNumberAsBool(animGraphInstance, INPUTPORT_X);
            y = GetInputNumberAsBool(animGraphInstance, INPUTPORT_Y);
        }
        else // only x or y is connected
        {
            // if only x has something plugged in
            if (mConnections[0]->GetTargetPort() == INPUTPORT_X)
            {
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
                x = GetInputNumberAsBool(animGraphInstance, INPUTPORT_X);
                y = GetAttributeFloatAsBool(ATTRIB_STATICVALUE);
            }
            else // only y has an input
            {
                MCORE_ASSERT(mConnections[0]->GetTargetPort() == INPUTPORT_Y);
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_Y));
                x = GetAttributeFloatAsBool(ATTRIB_STATICVALUE);
                y = GetInputNumberAsBool(animGraphInstance, INPUTPORT_Y);
            }
        }

        // execute the logic function
        if (mFunction(x, y))
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_BOOL)->SetValue(1.0f);
            GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(GetAttributeFloat(ATTRIB_TRUEVALUE)->GetValue());
        }
        else
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_BOOL)->SetValue(0.0f);
            GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(GetAttributeFloat(ATTRIB_FALSEVALUE)->GetValue());
        }
    }


    // get the type string
    const char* BlendTreeBoolLogicNode::GetTypeString() const
    {
        return "BlendTreeBoolLogicNode";
    }


    // set the condition function to use
    void BlendTreeBoolLogicNode::SetFunction(EFunction func)
    {
        // if it didn't change, don't update anything
        if (func == mFunctionEnum)
        {
            return;
        }

        mFunctionEnum = func;
        switch (mFunctionEnum)
        {
        case FUNCTION_AND:
            mFunction = BoolLogicAND;
            SetNodeInfo("x AND y");
            return;
        case FUNCTION_OR:
            mFunction = BoolLogicOR;
            SetNodeInfo("x OR y");
            return;
        case FUNCTION_XOR:
            mFunction = BoolLogicXOR;
            SetNodeInfo("x XOR y");
            return;
        default:
            MCORE_ASSERT(false);        // function unknown
        }
        ;
    }


    // update the data
    void BlendTreeBoolLogicNode::OnUpdateAttributes()
    {
        SetFunction((EFunction)((uint32)GetAttributeFloat(ATTRIB_FUNCTION)->GetValue()));
    }


    uint32 BlendTreeBoolLogicNode::GetVisualColor() const
    {
        return MCore::RGBA(50, 255, 50);
    }


    //-----------------------------------------------
    // the condition functions
    //-----------------------------------------------
    bool BlendTreeBoolLogicNode::BoolLogicAND(bool x, bool y)       { return (x && y); }
    bool BlendTreeBoolLogicNode::BoolLogicOR(bool x, bool y)        { return (x || y); }
    bool BlendTreeBoolLogicNode::BoolLogicXOR(bool x, bool y)       { return (x ^ y); }
}   // namespace EMotionFX

