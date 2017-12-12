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
#include "BlendTreeFloatConditionNode.h"
#include <MCore/Source/Compare.h>
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    BlendTreeFloatConditionNode::BlendTreeFloatConditionNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();

        // set the defaults
        mFunctionEnum   = FUNCTION_EQUAL;
        mFunction       = FloatConditionEqual;
        SetNodeInfo("x == y");
    }


    // destructor
    BlendTreeFloatConditionNode::~BlendTreeFloatConditionNode()
    {
    }


    // create
    BlendTreeFloatConditionNode* BlendTreeFloatConditionNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeFloatConditionNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeFloatConditionNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeFloatConditionNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X); // accept float/int/bool values
        SetupInputPortAsNumber("y", INPUTPORT_Y, PORTID_INPUT_Y); // accept float/int/bool values

        // setup the output ports
        InitOutputPorts(2);
        SetupOutputPort("Float", OUTPUTPORT_VALUE,  MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_VALUE);
        SetupOutputPort("Bool",  OUTPUTPORT_BOOL,   MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_BOOL);    // false on default
    }


    // register the parameters
    void BlendTreeFloatConditionNode::RegisterAttributes()
    {
        // create the condition function combobox
        MCore::AttributeSettings* functionParam = RegisterAttribute("Condition Function", "conditionFunction", "The condition function to use.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        functionParam->SetReinitGuiOnValueChange(true);
        functionParam->ResizeComboValues((uint32)NUM_FUNCTIONS);
        functionParam->SetComboValue(FUNCTION_EQUAL,            "Is Equal");
        functionParam->SetComboValue(FUNCTION_GREATER,          "Is Greater");
        functionParam->SetComboValue(FUNCTION_LESS,             "Is Less");
        functionParam->SetComboValue(FUNCTION_GREATEROREQUAL,   "Is Greater Or Equal");
        functionParam->SetComboValue(FUNCTION_LESSOREQUAL,      "Is Less Or Equal");
        functionParam->SetComboValue(FUNCTION_NOTEQUAL,         "Is Not Equal");
        functionParam->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // create the static value float spinner
        MCore::AttributeSettings* valueParam = RegisterAttribute("Static Value", "staticValue", "Value used for x or y when the input port has no connection.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        valueParam->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        valueParam->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        valueParam->SetMaxValue(MCore::AttributeFloat::Create(FLT_MAX));

        MCore::AttributeSettings* trueValueParam = RegisterAttribute("Result When True", "trueResult", "The value returned when the expression is true.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        trueValueParam->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        trueValueParam->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        trueValueParam->SetMaxValue(MCore::AttributeFloat::Create(FLT_MAX));

        MCore::AttributeSettings* falseValueParam = RegisterAttribute("Result When False", "falseResult", "The value returned when the expression is false.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        falseValueParam->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        falseValueParam->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        falseValueParam->SetMaxValue(MCore::AttributeFloat::Create(FLT_MAX));

        // create the true return mode
        MCore::AttributeSettings* trueModeParam = RegisterAttribute("True Return Mode", "trueMode", "What to return when the result is true.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        trueModeParam->ResizeComboValues(3);
        trueModeParam->SetComboValue(MODE_VALUE, "Return True Value");
        trueModeParam->SetComboValue(MODE_X,    "Return X");
        trueModeParam->SetComboValue(MODE_Y,    "Return Y");
        trueModeParam->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // create the false return mode
        MCore::AttributeSettings* falseModeParam = RegisterAttribute("False Return Mode", "falseMode", "What to return when the result is false.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        falseModeParam->ResizeComboValues(3);
        falseModeParam->SetComboValue(MODE_VALUE, "Return False Value");
        falseModeParam->SetComboValue(MODE_X, "Return X");
        falseModeParam->SetComboValue(MODE_Y, "Return Y");
        falseModeParam->SetDefaultValue(MCore::AttributeFloat::Create(0));
    }


    // get the palette name
    const char* BlendTreeFloatConditionNode::GetPaletteName() const
    {
        return "Float Condition";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFloatConditionNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_LOGIC;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeFloatConditionNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeFloatConditionNode* clone = new BlendTreeFloatConditionNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeFloatConditionNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if there are no incoming connections, there is nothing to do
        const uint32 numConnections = mConnections.GetLength();
        if (numConnections == 0)
        {
            return;
        }

        //  AnimGraphNodeData* uniqueData = animGraphInstance->FindUniqueNodeData(this);

        // update the condition function if needed
        SetFunction((EFunction)((uint32)GetAttributeFloat(ATTRIB_FUNCTION)->GetValue()));

        // if both x and y inputs have connections
        float x, y;
        if (numConnections == 2)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_Y));

            x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);
            y = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Y);
        }
        else // only x or y is connected
        {
            // if only x has something plugged in
            if (mConnections[0]->GetTargetPort() == INPUTPORT_X)
            {
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
                x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);
                y = GetAttributeFloat(ATTRIB_STATICVALUE)->GetValue();
            }
            else // only y has an input
            {
                MCORE_ASSERT(mConnections[0]->GetTargetPort() == INPUTPORT_Y);
                x = GetAttributeFloat(ATTRIB_STATICVALUE)->GetValue();
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_Y));
                y = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Y);
            }
        }

        // execute the condition function
        if (mFunction(x, y))
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_BOOL)->SetValue(1.0f); // set to true

            const uint32 mode = GetAttributeFloatAsUint32(ATTRIB_TRUEMODE);
            switch (mode)
            {
            case MODE_VALUE:
                GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(GetAttributeFloat(ATTRIB_TRUEVALUE)->GetValue());
                break;
            case MODE_X:
                GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(x);
                break;
            case MODE_Y:
                GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(y);
                break;
            default:
                MCORE_ASSERT(false);    // should never happen
            }
            ;
        }
        else
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_BOOL)->SetValue(0.0f);

            const uint32 mode = GetAttributeFloatAsUint32(ATTRIB_FALSEMODE);
            switch (mode)
            {
            case MODE_VALUE:
                GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(GetAttributeFloat(ATTRIB_FALSEVALUE)->GetValue());
                break;
            case MODE_X:
                GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(x);
                break;
            case MODE_Y:
                GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(y);
                break;
            default:
                MCORE_ASSERT(false);    // should never happen
            }
            ;
        }
    }


    // get the type string
    const char* BlendTreeFloatConditionNode::GetTypeString() const
    {
        return "BlendTreeFloatConditionNode";
    }


    // set the condition function to use
    void BlendTreeFloatConditionNode::SetFunction(EFunction func)
    {
        // if it didn't change, don't update anything
        if (func == mFunctionEnum)
        {
            return;
        }

        mFunctionEnum = func;
        switch (mFunctionEnum)
        {
        case FUNCTION_EQUAL:
            mFunction = FloatConditionEqual;
            SetNodeInfo("x == y");
            return;
        case FUNCTION_NOTEQUAL:
            mFunction = FloatConditionNotEqual;
            SetNodeInfo("x != y");
            return;
        case FUNCTION_GREATER:
            mFunction = FloatConditionGreater;
            SetNodeInfo("x > y");
            return;
        case FUNCTION_LESS:
            mFunction = FloatConditionLess;
            SetNodeInfo("x < y");
            return;
        case FUNCTION_GREATEROREQUAL:
            mFunction = FloatConditionGreaterOrEqual;
            SetNodeInfo("x >= y");
            return;
        case FUNCTION_LESSOREQUAL:
            mFunction = FloatConditionLessOrEqual;
            SetNodeInfo("x <= y");
            return;
        default:
            MCORE_ASSERT(false);        // function unknown
        }
        ;
    }


    // update the data
    void BlendTreeFloatConditionNode::OnUpdateAttributes()
    {
        // update the condition function if needed
        SetFunction((EFunction)((uint32)GetAttributeFloat(ATTRIB_FUNCTION)->GetValue()));
    }


    uint32 BlendTreeFloatConditionNode::GetVisualColor() const
    {
        return MCore::RGBA(255, 100, 50);
    }



    //-----------------------------------------------
    // the condition functions
    //-----------------------------------------------
    bool BlendTreeFloatConditionNode::FloatConditionEqual(float x, float y)             { return MCore::Compare<float>::CheckIfIsClose(x, y, MCore::Math::epsilon); }
    bool BlendTreeFloatConditionNode::FloatConditionNotEqual(float x, float y)          { return (MCore::Compare<float>::CheckIfIsClose(x, y, MCore::Math::epsilon) == false); }
    bool BlendTreeFloatConditionNode::FloatConditionGreater(float x, float y)           { return (x > y); }
    bool BlendTreeFloatConditionNode::FloatConditionLess(float x, float y)              { return (x < y); }
    bool BlendTreeFloatConditionNode::FloatConditionGreaterOrEqual(float x, float y)    { return (x >= y); }
    bool BlendTreeFloatConditionNode::FloatConditionLessOrEqual(float x, float y)       { return (x <= y); }
}   // namespace EMotionFX

