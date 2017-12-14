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
#include "BlendTreeVector3Math1Node.h"
#include <MCore/Source/Random.h>
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    BlendTreeVector3Math1Node::BlendTreeVector3Math1Node(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();

        // default on sinus calculation
        mMathFunction   = MATHFUNCTION_LENGTH;
        mCalculateFunc  = CalculateLength;
        SetNodeInfo("Length");
    }


    // destructor
    BlendTreeVector3Math1Node::~BlendTreeVector3Math1Node()
    {
    }


    // create
    BlendTreeVector3Math1Node* BlendTreeVector3Math1Node::Create(AnimGraph* animGraph)
    {
        return new BlendTreeVector3Math1Node(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeVector3Math1Node::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeVector3Math1Node::RegisterPorts()
    {
        // create the input ports
        InitInputPorts(1);
        SetupInputPort("x", INPUTPORT_X, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_X);

        // create the output ports
        InitOutputPorts(2);
        SetupOutputPort("Vector3",  OUTPUTPORT_RESULT_VECTOR3,  MCore::AttributeVector3::TYPE_ID,   PORTID_OUTPUT_VECTOR3);
        SetupOutputPort("Float",    OUTPUTPORT_RESULT_FLOAT,    MCore::AttributeFloat::TYPE_ID,     PORTID_OUTPUT_FLOAT);
    }


    // register the parameters
    void BlendTreeVector3Math1Node::RegisterAttributes()
    {
        MCore::AttributeSettings* functionParam = RegisterAttribute("Math Function", "mathFunction", "The math function to use.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        functionParam->SetReinitGuiOnValueChange(true);
        functionParam->ResizeComboValues((uint32)MATHFUNCTION_NUMFUNCTIONS);
        functionParam->SetComboValue(MATHFUNCTION_LENGTH,       "Length");
        functionParam->SetComboValue(MATHFUNCTION_SQUARELENGTH, "Square Length");
        functionParam->SetComboValue(MATHFUNCTION_NORMALIZE,    "Normalize");
        functionParam->SetComboValue(MATHFUNCTION_ZERO,         "Zero");
        functionParam->SetComboValue(MATHFUNCTION_FLOOR,        "Floor");
        functionParam->SetComboValue(MATHFUNCTION_CEIL,         "Ceil");
        functionParam->SetComboValue(MATHFUNCTION_ABS,          "Abs");
        functionParam->SetComboValue(MATHFUNCTION_RANDOM,       "Random Vector [0..1]");
        functionParam->SetComboValue(MATHFUNCTION_RANDOMNEG,    "Random Vector [-1..1]");
        functionParam->SetComboValue(MATHFUNCTION_RANDOMDIRVEC, "Random Direction");
        functionParam->SetComboValue(MATHFUNCTION_NEGATE,       "Negate");
        functionParam->SetDefaultValue(MCore::AttributeFloat::Create(0));
    }


    // get the palette name
    const char* BlendTreeVector3Math1Node::GetPaletteName() const
    {
        return "Vector3 Math1";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeVector3Math1Node::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeVector3Math1Node::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeVector3Math1Node* clone = new BlendTreeVector3Math1Node(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeVector3Math1Node::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // update the math function if needed
        SetMathFunction((EMathFunction)((uint32)GetAttributeFloat(0)->GetValue()));

        // get the input value as a float, convert if needed
        AZ::Vector3 input;
        BlendTreeConnection* connection = mInputPorts[INPUTPORT_X].mConnection;
        if (connection)
        {
            //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_X) );
            input = AZ::Vector3(GetInputVector3(animGraphInstance, INPUTPORT_X)->GetValue());
        }
        else
        {
            input = AZ::Vector3::CreateZero();
        }

        // apply the operation
        AZ::Vector3 vectorResult(0.0f, 0.0f, 0.0f);
        float floatResult = 0.0f;
        mCalculateFunc(input, &vectorResult, &floatResult);

        // update the output value
        //AnimGraphNodeData* uniqueData = animGraphInstance->FindUniqueNodeData(this);
        GetOutputVector3(animGraphInstance, OUTPUTPORT_RESULT_VECTOR3)->SetValue(AZ::PackedVector3f(vectorResult));
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT_FLOAT)->SetValue(floatResult);
    }


    // get the type string
    const char* BlendTreeVector3Math1Node::GetTypeString() const
    {
        return "BlendTreeVector3Math1Node";
    }


    // set the math function to use
    void BlendTreeVector3Math1Node::SetMathFunction(EMathFunction func)
    {
        // if it didn't change, don't update anything
        if (func == mMathFunction)
        {
            return;
        }

        mMathFunction = func;
        switch (mMathFunction)
        {
        case MATHFUNCTION_LENGTH:
            mCalculateFunc = CalculateLength;
            SetNodeInfo("Length");
            return;
        case MATHFUNCTION_SQUARELENGTH:
            mCalculateFunc = CalculateSquareLength;
            SetNodeInfo("Square Length");
            return;
        case MATHFUNCTION_NORMALIZE:
            mCalculateFunc = CalculateNormalize;
            SetNodeInfo("Normalize");
            return;
        case MATHFUNCTION_ZERO:
            mCalculateFunc = CalculateZero;
            SetNodeInfo("Zero");
            return;
        case MATHFUNCTION_FLOOR:
            mCalculateFunc = CalculateFloor;
            SetNodeInfo("Floor");
            return;
        case MATHFUNCTION_CEIL:
            mCalculateFunc = CalculateCeil;
            SetNodeInfo("Ceil");
            return;
        case MATHFUNCTION_ABS:
            mCalculateFunc = CalculateAbs;
            SetNodeInfo("Abs");
            return;
        case MATHFUNCTION_NEGATE:
            mCalculateFunc = CalculateNegate;
            SetNodeInfo("Negate");
            return;
        case MATHFUNCTION_RANDOM:
            mCalculateFunc = CalculateRandomVector;
            SetNodeInfo("Random[0..1]");
            return;
        case MATHFUNCTION_RANDOMNEG:
            mCalculateFunc = CalculateRandomVectorNeg;
            SetNodeInfo("Random[-1..1]");
            return;
        case MATHFUNCTION_RANDOMDIRVEC:
            mCalculateFunc = CalculateRandomVectorDir;
            SetNodeInfo("RandomDirection");
            return;

        default:
            MCORE_ASSERT(false);        // function unknown
        }
        ;
    }


    // update the data
    void BlendTreeVector3Math1Node::OnUpdateAttributes()
    {
        // update the math function if needed
        SetMathFunction((EMathFunction)((uint32)GetAttributeFloat(0)->GetValue()));
    }


    //-----------------------------------------------
    // the math functions
    //-----------------------------------------------
    void BlendTreeVector3Math1Node::CalculateLength(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)          { MCORE_UNUSED(vectorOutput); *floatOutput = MCore::SafeLength(input); }
    void BlendTreeVector3Math1Node::CalculateSquareLength(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)    { MCORE_UNUSED(vectorOutput); *floatOutput = input.GetLengthSq(); }
    void BlendTreeVector3Math1Node::CalculateNormalize(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)       { MCORE_UNUSED(floatOutput); *vectorOutput = input.GetNormalized(); }
    void BlendTreeVector3Math1Node::CalculateZero(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)            { MCORE_UNUSED(floatOutput); *vectorOutput = input; *vectorOutput = AZ::Vector3::CreateZero(); }
    void BlendTreeVector3Math1Node::CalculateAbs(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)             { MCORE_UNUSED(floatOutput); vectorOutput->Set(MCore::Math::Abs(input.GetX()), MCore::Math::Abs(input.GetY()), MCore::Math::Abs(input.GetZ())); }
    void BlendTreeVector3Math1Node::CalculateFloor(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)           { MCORE_UNUSED(floatOutput); vectorOutput->Set(MCore::Math::Floor(input.GetX()), MCore::Math::Floor(input.GetY()), MCore::Math::Floor(input.GetZ())); }
    void BlendTreeVector3Math1Node::CalculateCeil(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)            { MCORE_UNUSED(floatOutput); vectorOutput->Set(MCore::Math::Ceil(input.GetX()), MCore::Math::Ceil(input.GetY()), MCore::Math::Ceil(input.GetZ())); }
    void BlendTreeVector3Math1Node::CalculateRandomVector(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)    { MCORE_UNUSED(floatOutput); MCORE_UNUSED(input); vectorOutput->Set(MCore::Random::RandF(0.0f, 1.0f), MCore::Random::RandF(0.0f, 1.0f), MCore::Random::RandF(0.0f, 1.0f)); }
    void BlendTreeVector3Math1Node::CalculateRandomVectorNeg(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput) { MCORE_UNUSED(floatOutput); MCORE_UNUSED(input); *vectorOutput = MCore::Random::RandomVecF(); }
    void BlendTreeVector3Math1Node::CalculateRandomVectorDir(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput) { MCORE_UNUSED(floatOutput); MCORE_UNUSED(input); *vectorOutput = MCore::Random::RandDirVecF(); }
    void BlendTreeVector3Math1Node::CalculateNegate(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)          { MCORE_UNUSED(floatOutput); vectorOutput->Set(-input.GetX(), -input.GetY(), -input.GetZ()); }
}   // namespace EMotionFX

