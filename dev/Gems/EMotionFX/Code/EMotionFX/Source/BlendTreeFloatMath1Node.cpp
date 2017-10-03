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
#include "BlendTreeFloatMath1Node.h"
#include <MCore/Source/Random.h>
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    BlendTreeFloatMath1Node::BlendTreeFloatMath1Node(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();

        // default on sinus calculation
        mMathFunction   = MATHFUNCTION_SIN;
        mCalculateFunc  = CalculateSin;
        SetNodeInfo("Sin");
    }


    // destructor
    BlendTreeFloatMath1Node::~BlendTreeFloatMath1Node()
    {
    }


    // create
    BlendTreeFloatMath1Node* BlendTreeFloatMath1Node::Create(AnimGraph* animGraph)
    {
        return new BlendTreeFloatMath1Node(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeFloatMath1Node::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeFloatMath1Node::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X); // accept float/int/bool values

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Result", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);
    }


    // register the parameters
    void BlendTreeFloatMath1Node::RegisterAttributes()
    {
        MCore::AttributeSettings* functionParam = RegisterAttribute("Math Function", "mathFunction", "The math function to use.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        functionParam->SetReinitGuiOnValueChange(true);
        functionParam->ResizeComboValues((uint32)MATHFUNCTION_NUMFUNCTIONS);
        functionParam->SetComboValue(MATHFUNCTION_SIN,          "Sine");
        functionParam->SetComboValue(MATHFUNCTION_COS,          "Cosine");
        functionParam->SetComboValue(MATHFUNCTION_TAN,          "Tan");
        functionParam->SetComboValue(MATHFUNCTION_SQR,          "Square");
        functionParam->SetComboValue(MATHFUNCTION_SQRT,         "Square Root");
        functionParam->SetComboValue(MATHFUNCTION_ABS,          "Absolute");
        functionParam->SetComboValue(MATHFUNCTION_FLOOR,        "Floor");
        functionParam->SetComboValue(MATHFUNCTION_CEIL,         "Ceil");
        functionParam->SetComboValue(MATHFUNCTION_ONEOVERINPUT, "One Over X");
        functionParam->SetComboValue(MATHFUNCTION_INVSQRT,      "Inverse Square Root");
        functionParam->SetComboValue(MATHFUNCTION_LOG,          "Natural Log");
        functionParam->SetComboValue(MATHFUNCTION_LOG10,        "Log Base 10");
        functionParam->SetComboValue(MATHFUNCTION_EXP,          "Exponent");
        functionParam->SetComboValue(MATHFUNCTION_FRACTION,     "Fraction");
        functionParam->SetComboValue(MATHFUNCTION_SIGN,         "Sign");
        functionParam->SetComboValue(MATHFUNCTION_ISPOSITIVE,   "Is Positive");
        functionParam->SetComboValue(MATHFUNCTION_ISNEGATIVE,   "Is Negative");
        functionParam->SetComboValue(MATHFUNCTION_ISNEARZERO,   "Is Near Zero");
        functionParam->SetComboValue(MATHFUNCTION_RANDOMFLOAT,  "Random Float");
        functionParam->SetComboValue(MATHFUNCTION_RADTODEG,     "Radians to Degrees");
        functionParam->SetComboValue(MATHFUNCTION_DEGTORAD,     "Degrees to Radians");
        functionParam->SetComboValue(MATHFUNCTION_SMOOTHSTEP,   "Smooth Step [0..1]");
        functionParam->SetComboValue(MATHFUNCTION_ACOS,         "Arc Cosine");
        functionParam->SetComboValue(MATHFUNCTION_ASIN,         "Arc Sine");
        functionParam->SetComboValue(MATHFUNCTION_ATAN,         "Arc Tan");
        functionParam->SetComboValue(MATHFUNCTION_NEGATE,       "Negate");
        functionParam->SetDefaultValue(MCore::AttributeFloat::Create(0));
    }


    // get the palette name
    const char* BlendTreeFloatMath1Node::GetPaletteName() const
    {
        return "Float Math1";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFloatMath1Node::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeFloatMath1Node::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeFloatMath1Node* clone = new BlendTreeFloatMath1Node(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeFloatMath1Node::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
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
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
                GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X));
            }

            return;
        }

        // update the math function if needed
        SetMathFunction((EMathFunction)((uint32)GetAttributeFloat(0)->GetValue()));

        // get the input value as a float, convert if needed
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
        const float x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);

        // apply the operation
        const float result = mCalculateFunc(x);

        // update the output value
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(result);
    }



    // get the type string
    const char* BlendTreeFloatMath1Node::GetTypeString() const
    {
        return "BlendTreeFloatMath1Node";
    }


    // set the math function to use
    void BlendTreeFloatMath1Node::SetMathFunction(EMathFunction func)
    {
        // if it didn't change, don't update anything
        if (func == mMathFunction)
        {
            return;
        }

        mMathFunction = func;
        switch (mMathFunction)
        {
        case MATHFUNCTION_SIN:
            mCalculateFunc = CalculateSin;
            SetNodeInfo("Sin");
            return;
        case MATHFUNCTION_COS:
            mCalculateFunc = CalculateCos;
            SetNodeInfo("Cos");
            return;
        case MATHFUNCTION_TAN:
            mCalculateFunc = CalculateTan;
            SetNodeInfo("Tan");
            return;
        case MATHFUNCTION_SQR:
            mCalculateFunc = CalculateSqr;
            SetNodeInfo("Square");
            return;
        case MATHFUNCTION_SQRT:
            mCalculateFunc = CalculateSqrt;
            SetNodeInfo("Sqrt");
            return;
        case MATHFUNCTION_ABS:
            mCalculateFunc = CalculateAbs;
            SetNodeInfo("Abs");
            return;
        case MATHFUNCTION_FLOOR:
            mCalculateFunc = CalculateFloor;
            SetNodeInfo("Floor");
            return;
        case MATHFUNCTION_CEIL:
            mCalculateFunc = CalculateCeil;
            SetNodeInfo("Ceil");
            return;
        case MATHFUNCTION_ONEOVERINPUT:
            mCalculateFunc = CalculateOneOverInput;
            SetNodeInfo("1/x");
            return;
        case MATHFUNCTION_INVSQRT:
            mCalculateFunc = CalculateInvSqrt;
            SetNodeInfo("1.0/sqrt(x)");
            return;
        case MATHFUNCTION_LOG:
            mCalculateFunc = CalculateLog;
            SetNodeInfo("Log");
            return;
        case MATHFUNCTION_LOG10:
            mCalculateFunc = CalculateLog10;
            SetNodeInfo("Log10");
            return;
        case MATHFUNCTION_EXP:
            mCalculateFunc = CalculateExp;
            SetNodeInfo("Exponent");
            return;
        case MATHFUNCTION_FRACTION:
            mCalculateFunc = CalculateFraction;
            SetNodeInfo("Fraction");
            return;
        case MATHFUNCTION_SIGN:
            mCalculateFunc = CalculateSign;
            SetNodeInfo("Sign");
            return;
        case MATHFUNCTION_ISPOSITIVE:
            mCalculateFunc = CalculateIsPositive;
            SetNodeInfo("Is Positive");
            return;
        case MATHFUNCTION_ISNEGATIVE:
            mCalculateFunc = CalculateIsNegative;
            SetNodeInfo("Is Negative");
            return;
        case MATHFUNCTION_ISNEARZERO:
            mCalculateFunc = CalculateIsNearZero;
            SetNodeInfo("Is Near Zero");
            return;
        case MATHFUNCTION_RANDOMFLOAT:
            mCalculateFunc = CalculateRandomFloat;
            SetNodeInfo("Random Float");
            return;
        case MATHFUNCTION_RADTODEG:
            mCalculateFunc = CalculateRadToDeg;
            SetNodeInfo("RadToDeg");
            return;
        case MATHFUNCTION_DEGTORAD:
            mCalculateFunc = CalculateDegToRad;
            SetNodeInfo("DegToRad");
            return;
        case MATHFUNCTION_SMOOTHSTEP:
            mCalculateFunc = CalculateSmoothStep;
            SetNodeInfo("SmoothStep");
            return;
        case MATHFUNCTION_ACOS:
            mCalculateFunc = CalculateACos;
            SetNodeInfo("Arc Cos");
            return;
        case MATHFUNCTION_ASIN:
            mCalculateFunc = CalculateASin;
            SetNodeInfo("Arc Sin");
            return;
        case MATHFUNCTION_ATAN:
            mCalculateFunc = CalculateATan;
            SetNodeInfo("Arc Tan");
            return;
        case MATHFUNCTION_NEGATE:
            mCalculateFunc = CalculateNegate;
            SetNodeInfo("Negate");
            return;
        default:
            MCORE_ASSERT(false);    // function unknown
        }
        ;
    }


    // update the data
    void BlendTreeFloatMath1Node::OnUpdateAttributes()
    {
        // update the math function if needed
        SetMathFunction((EMathFunction)((uint32)GetAttributeFloat(0)->GetValue()));
    }


    uint32 BlendTreeFloatMath1Node::GetVisualColor() const
    {
        return MCore::RGBA(128, 255, 255);
    }


    bool BlendTreeFloatMath1Node::GetSupportsDisable() const
    {
        return true;
    }



    //-----------------------------------------------
    // the math functions
    //-----------------------------------------------
    float BlendTreeFloatMath1Node::CalculateSin(float input)                { return MCore::Math::Sin(input); }
    float BlendTreeFloatMath1Node::CalculateCos(float input)                { return MCore::Math::Cos(input); }
    float BlendTreeFloatMath1Node::CalculateTan(float input)                { return MCore::Math::Tan(input); }
    float BlendTreeFloatMath1Node::CalculateSqr(float input)                { return (input * input); }
    float BlendTreeFloatMath1Node::CalculateSqrt(float input)               { return MCore::Math::SafeSqrt(input); }
    float BlendTreeFloatMath1Node::CalculateAbs(float input)                { return MCore::Math::Abs(input); }
    float BlendTreeFloatMath1Node::CalculateFloor(float input)              { return MCore::Math::Floor(input); }
    float BlendTreeFloatMath1Node::CalculateCeil(float input)               { return MCore::Math::Ceil(input); }
    float BlendTreeFloatMath1Node::CalculateOneOverInput(float input)
    {
        if (input > MCore::Math::epsilon)
        {
            return 1.0f / input;
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateInvSqrt(float input)
    {
        if (input > MCore::Math::epsilon)
        {
            return MCore::Math::InvSqrt(input);
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateLog(float input)
    {
        if (input > MCore::Math::epsilon)
        {
            return MCore::Math::Log(input);
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateLog10(float input)
    {
        if (input > MCore::Math::epsilon)
        {
            return log10f(input);
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateExp(float input)                { return MCore::Math::Exp(input); }
    float BlendTreeFloatMath1Node::CalculateFraction(float input)           { return MCore::Math::FMod(input, 1.0f); }
    float BlendTreeFloatMath1Node::CalculateSign(float input)
    {
        if (input < 0.0f)
        {
            return -1.0f;
        }
        if (input > 0.0f)
        {
            return 1.0f;
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateIsPositive(float input)
    {
        if (input >= 0.0f)
        {
            return 1.0f;
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateIsNegative(float input)
    {
        if (input < 0.0f)
        {
            return 1.0f;
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateIsNearZero(float input)
    {
        if ((input > -MCore::Math::epsilon) && (input < MCore::Math::epsilon))
        {
            return 1.0f;
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateRandomFloat(float input)        { return MCore::Random::RandF(0.0f, input);  }
    float BlendTreeFloatMath1Node::CalculateRadToDeg(float input)           { return MCore::Math::RadiansToDegrees(input); }
    float BlendTreeFloatMath1Node::CalculateDegToRad(float input)           { return MCore::Math::DegreesToRadians(input); }
    float BlendTreeFloatMath1Node::CalculateSmoothStep(float input)         { const float f = MCore::Clamp<float>(input, 0.0f, 1.0f); return MCore::CosineInterpolate<float>(0.0f, 1.0f, f); }
    float BlendTreeFloatMath1Node::CalculateACos(float input)               { return MCore::Math::ACos(input); }
    float BlendTreeFloatMath1Node::CalculateASin(float input)               { return MCore::Math::ASin(input); }
    float BlendTreeFloatMath1Node::CalculateATan(float input)               { return MCore::Math::ATan(input); }
    float BlendTreeFloatMath1Node::CalculateNegate(float input)             { return -input; }
}   // namespace EMotionFX

