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
#include "BlendTreeFloatMath2Node.h"
#include <MCore/Source/Random.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    BlendTreeFloatMath2Node::BlendTreeFloatMath2Node(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();

        // default on sinus calculation
        mMathFunction   = MATHFUNCTION_ADD;
        mCalculateFunc  = CalculateAdd;
        SetNodeInfo("x + y");
    }


    // destructor
    BlendTreeFloatMath2Node::~BlendTreeFloatMath2Node()
    {
    }


    // create
    BlendTreeFloatMath2Node* BlendTreeFloatMath2Node::Create(AnimGraph* animGraph)
    {
        return new BlendTreeFloatMath2Node(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeFloatMath2Node::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeFloatMath2Node::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X); // accept float/int/bool values
        SetupInputPortAsNumber("y", INPUTPORT_Y, PORTID_INPUT_Y); // accept float/int/bool values

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Result", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);
    }


    // register the parameters
    void BlendTreeFloatMath2Node::RegisterAttributes()
    {
        // create the math function combobox
        MCore::AttributeSettings* functionParam = RegisterAttribute("Math Function", "mathFunction", "The math function to use.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        functionParam->SetReinitGuiOnValueChange(true);
        functionParam->ResizeComboValues((uint32)MATHFUNCTION_NUMFUNCTIONS);
        functionParam->SetComboValue(MATHFUNCTION_ADD,      "Add");
        functionParam->SetComboValue(MATHFUNCTION_SUBTRACT, "Subtract");
        functionParam->SetComboValue(MATHFUNCTION_MULTIPLY, "Multiply");
        functionParam->SetComboValue(MATHFUNCTION_DIVIDE,   "Divide");
        functionParam->SetComboValue(MATHFUNCTION_AVERAGE,  "Average");
        functionParam->SetComboValue(MATHFUNCTION_RANDOMFLOAT, "Random Float");
        functionParam->SetComboValue(MATHFUNCTION_MOD,  "Mod");
        functionParam->SetComboValue(MATHFUNCTION_MIN,  "Minimum");
        functionParam->SetComboValue(MATHFUNCTION_MAX,  "Maximum");
        functionParam->SetComboValue(MATHFUNCTION_POW,  "Power");
        functionParam->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // create the static value float spinner
        MCore::AttributeSettings* valueParam = RegisterAttribute("Static Value", "staticValue", "Value used for x or y when the input port has no connection.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        valueParam->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        valueParam->SetMinValue(MCore::AttributeFloat::Create(-FLT_MAX));
        valueParam->SetMaxValue(MCore::AttributeFloat::Create(+FLT_MAX));
    }


    // get the palette name
    const char* BlendTreeFloatMath2Node::GetPaletteName() const
    {
        return "Float Math2";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFloatMath2Node::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeFloatMath2Node::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeFloatMath2Node* clone = new BlendTreeFloatMath2Node(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeFloatMath2Node::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if there are no incoming connections, there is nothing to do
        if (mConnections.GetLength() == 0)
        {
            return;
        }

        // update the math function if needed
        SetMathFunction((EMathFunction)((uint32)GetAttributeFloat(ATTRIB_MATHFUNCTION)->GetValue()));

        // if both x and y inputs have connections
        float x, y;
        if (mConnections.GetLength() == 2)
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

        // apply the operation
        const float result = mCalculateFunc(x, y);

        // update the output value
        //AnimGraphNodeData* uniqueData = animGraphInstance->FindUniqueNodeData(this);
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(result);

        //if (rand() % 50 == 0)
        //MCore::LogDebug("func(%.4f, %.4f) = %.4f", x, y, result);
    }


    // get the type string
    const char* BlendTreeFloatMath2Node::GetTypeString() const
    {
        return "BlendTreeFloatMath2Node";
    }


    // set the math function to use
    void BlendTreeFloatMath2Node::SetMathFunction(EMathFunction func)
    {
        // if it didn't change, don't update anything
        if (func == mMathFunction)
        {
            return;
        }

        mMathFunction = func;
        switch (mMathFunction)
        {
        case MATHFUNCTION_ADD:
            mCalculateFunc = CalculateAdd;
            SetNodeInfo("x + y");
            return;
        case MATHFUNCTION_SUBTRACT:
            mCalculateFunc = CalculateSubtract;
            SetNodeInfo("x - y");
            return;
        case MATHFUNCTION_MULTIPLY:
            mCalculateFunc = CalculateMultiply;
            SetNodeInfo("x * y");
            return;
        case MATHFUNCTION_DIVIDE:
            mCalculateFunc = CalculateDivide;
            SetNodeInfo("x / y");
            return;
        case MATHFUNCTION_AVERAGE:
            mCalculateFunc = CalculateAverage;
            SetNodeInfo("Average");
            return;
        case MATHFUNCTION_RANDOMFLOAT:
            mCalculateFunc = CalculateRandomFloat;
            SetNodeInfo("Random[x..y]");
            return;
        case MATHFUNCTION_MOD:
            mCalculateFunc = CalculateMod;
            SetNodeInfo("x MOD y");
            return;
        case MATHFUNCTION_MIN:
            mCalculateFunc = CalculateMin;
            SetNodeInfo("Min(x, y)");
            return;
        case MATHFUNCTION_MAX:
            mCalculateFunc = CalculateMax;
            SetNodeInfo("Max(x, y)");
            return;
        case MATHFUNCTION_POW:
            mCalculateFunc = CalculatePow;
            SetNodeInfo("Pow(x, y)");
            return;
        default:
            MCORE_ASSERT(false);        // function unknown
        }
        ;
    }


    // update the data
    void BlendTreeFloatMath2Node::OnUpdateAttributes()
    {
        // update the math function if needed
        SetMathFunction((EMathFunction)((uint32)GetAttributeFloat(0)->GetValue()));
    }


    uint32 BlendTreeFloatMath2Node::GetVisualColor() const
    {
        return MCore::RGBA(128, 255, 128);
    }


    //-----------------------------------------------
    // the math functions
    //-----------------------------------------------
    float BlendTreeFloatMath2Node::CalculateAdd(float x, float y)               { return x + y; }
    float BlendTreeFloatMath2Node::CalculateSubtract(float x, float y)          { return x - y; }
    float BlendTreeFloatMath2Node::CalculateMultiply(float x, float y)          { return x * y; }
    float BlendTreeFloatMath2Node::CalculateDivide(float x, float y)
    {
        if (MCore::Compare<float>::CheckIfIsClose(y, 0.0f, MCore::Math::epsilon) == false)
        {
            return x / y;
        }
        return 0.0f;
    }
    float BlendTreeFloatMath2Node::CalculateAverage(float x, float y)           { return (x + y) * 0.5f; }
    float BlendTreeFloatMath2Node::CalculateRandomFloat(float x, float y)       { return MCore::Random::RandF(x, y); }
    float BlendTreeFloatMath2Node::CalculateMod(float x, float y)
    {
        if (MCore::Compare<float>::CheckIfIsClose(y, 0.0f, MCore::Math::epsilon) == false)
        {
            return MCore::Math::FMod(x, y);
        }
        return 0.0f;
    }
    float BlendTreeFloatMath2Node::CalculateMin(float x, float y)               { return MCore::Min<float>(x, y); }
    float BlendTreeFloatMath2Node::CalculateMax(float x, float y)               { return MCore::Max<float>(x, y); }
    float BlendTreeFloatMath2Node::CalculatePow(float x, float y)
    {
        if (MCore::Compare<float>::CheckIfIsClose(x, 0.0f, MCore::Math::epsilon) && y < 0.0f)
        {
            return 0.0f;
        }
        return MCore::Math::Pow(x, y);
    }
}   // namespace EMotionFX
