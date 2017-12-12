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
#include "BlendTreeVector3Math2Node.h"
#include <MCore/Source/FastMath.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    BlendTreeVector3Math2Node::BlendTreeVector3Math2Node(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();

        // default on sinus calculation
        mMathFunction   = MATHFUNCTION_DOT;
        mCalculateFunc  = CalculateDot;
        SetNodeInfo("dot(x, y)");
    }


    // destructor
    BlendTreeVector3Math2Node::~BlendTreeVector3Math2Node()
    {
    }


    // create
    BlendTreeVector3Math2Node* BlendTreeVector3Math2Node::Create(AnimGraph* animGraph)
    {
        return new BlendTreeVector3Math2Node(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeVector3Math2Node::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeVector3Math2Node::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPort("x", INPUTPORT_X, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_X);
        SetupInputPort("y", INPUTPORT_Y, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_Y);

        // setup the output ports
        InitOutputPorts(2);
        SetupOutputPort("Vector3",  INPUTPORT_X, MCore::AttributeVector3::TYPE_ID,  PORTID_OUTPUT_VECTOR3);
        SetupOutputPort("Float",    INPUTPORT_Y, MCore::AttributeFloat::TYPE_ID,    PORTID_OUTPUT_FLOAT);
    }


    // register the parameters
    void BlendTreeVector3Math2Node::RegisterAttributes()
    {
        // create the math function combobox
        MCore::AttributeSettings* functionParam = RegisterAttribute("Math Function", "mathFunction", "The math function to use.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        functionParam->SetReinitGuiOnValueChange(true);
        functionParam->ResizeComboValues((uint32)MATHFUNCTION_NUMFUNCTIONS);
        functionParam->SetComboValue(MATHFUNCTION_DOT,          "Dot Product");
        functionParam->SetComboValue(MATHFUNCTION_CROSS,        "Cross Product");
        functionParam->SetComboValue(MATHFUNCTION_ADD,          "Add");
        functionParam->SetComboValue(MATHFUNCTION_SUBTRACT,     "Subtract");
        functionParam->SetComboValue(MATHFUNCTION_MULTIPLY,     "Multiply");
        functionParam->SetComboValue(MATHFUNCTION_DIVIDE,       "Divide");
        functionParam->SetComboValue(MATHFUNCTION_ANGLEDEGREES, "AngleDegrees");
        functionParam->SetDefaultValue(MCore::AttributeFloat::Create(0));

        // create the static value for x or y
        MCore::AttributeSettings* staticParam = RegisterAttribute("Static Value", "staticValue", "The static value for x or y when one of them has no incomming connection.", MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3);
        staticParam->SetDefaultValue(MCore::AttributeVector3::Create());
        staticParam->SetMinValue(MCore::AttributeVector3::Create(-FLT_MAX, -FLT_MAX, -FLT_MAX));
        staticParam->SetMaxValue(MCore::AttributeVector3::Create(FLT_MAX,  FLT_MAX,  FLT_MAX));
    }


    // get the palette name
    const char* BlendTreeVector3Math2Node::GetPaletteName() const
    {
        return "Vector3 Math2";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeVector3Math2Node::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeVector3Math2Node::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeVector3Math2Node* clone = new BlendTreeVector3Math2Node(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeVector3Math2Node::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if there are no incoming connections, there is nothing to do
        if (mConnections.GetLength() == 0)
        {
            return;
        }

        //AnimGraphNodeData* uniqueData = animGraphInstance->FindUniqueNodeData(this);

        // update the math function if needed
        SetMathFunction((EMathFunction)((uint32)GetAttributeFloat(ATTRIB_MATHFUNCTION)->GetValue()));

        // if both x and y inputs have connections
        MCore::Vector3 x, y;
        if (mConnections.GetLength() == 2)
        {
            //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_X) );
            //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_Y) );
            x = GetInputVector3(animGraphInstance, INPUTPORT_X)->GetValue();
            y = GetInputVector3(animGraphInstance, INPUTPORT_Y)->GetValue();
        }
        else // only x or y is connected
        {
            // if only x has something plugged in
            if (mConnections[0]->GetTargetPort() == INPUTPORT_X)
            {
                //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_X) );
                x = GetInputVector3(animGraphInstance, INPUTPORT_X)->GetValue();
                y = GetAttributeVector3(ATTRIB_STATICVECTOR)->GetValue();
            }
            else // only y has an input
            {
                MCORE_ASSERT(mConnections[0]->GetTargetPort() == INPUTPORT_Y);
                x = GetAttributeVector3(ATTRIB_STATICVECTOR)->GetValue();
                //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_Y) );
                y = GetInputVector3(animGraphInstance, INPUTPORT_Y)->GetValue();
            }
        }

        // apply the operation
        MCore::Vector3 vectorResult(0.0f, 0.0f, 0.0f);
        float floatResult = 0.0f;
        mCalculateFunc(x, y, &vectorResult, &floatResult);

        // update the output value
        GetOutputVector3(animGraphInstance, OUTPUTPORT_RESULT_VECTOR3)->SetValue(vectorResult);
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT_FLOAT)->SetValue(floatResult);
    }


    // get the type string
    const char* BlendTreeVector3Math2Node::GetTypeString() const
    {
        return "BlendTreeVector3Math2Node";
    }


    // set the math function to use
    void BlendTreeVector3Math2Node::SetMathFunction(EMathFunction func)
    {
        // if it didn't change, don't update anything
        if (func == mMathFunction)
        {
            return;
        }

        mMathFunction = func;
        switch (mMathFunction)
        {
        case MATHFUNCTION_DOT:
            mCalculateFunc = CalculateDot;
            SetNodeInfo("dot(x, y)");
            return;
        case MATHFUNCTION_CROSS:
            mCalculateFunc = CalculateCross;
            SetNodeInfo("cross(x, y)");
            return;
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
        case MATHFUNCTION_ANGLEDEGREES:
            mCalculateFunc = CalculateAngleDegrees;
            SetNodeInfo("Angle Degr.");
            return;

        default:
            MCORE_ASSERT(false);        // function unknown
        }
        ;
    }


    // update the data
    void BlendTreeVector3Math2Node::OnUpdateAttributes()
    {
        // update the math function if needed
        SetMathFunction((EMathFunction)((uint32)GetAttributeFloat(ATTRIB_MATHFUNCTION)->GetValue()));
    }


    //-----------------------------------------------
    // the math functions
    //-----------------------------------------------
    // dot product
    void BlendTreeVector3Math2Node::CalculateDot(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(vectorOutput);
        *floatOutput = inputX.Dot(inputY);
    }


    // cross product
    void BlendTreeVector3Math2Node::CalculateCross(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(floatOutput);
        *vectorOutput = inputX.Cross(inputY);
    }


    // add
    void BlendTreeVector3Math2Node::CalculateAdd(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(floatOutput);
        *vectorOutput = inputX + inputY;
    }


    // subtract
    void BlendTreeVector3Math2Node::CalculateSubtract(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(floatOutput);
        *vectorOutput = inputX - inputY;
    }


    // multiply
    void BlendTreeVector3Math2Node::CalculateMultiply(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(floatOutput);
        *vectorOutput = inputX * inputY;
    }


    // divide
    void BlendTreeVector3Math2Node::CalculateDivide(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(floatOutput);
        vectorOutput->x = MCore::Compare<float>::CheckIfIsClose(inputY.x, 0.0f, MCore::Math::epsilon) == false ? (inputX.x / inputY.x) : 0.0f;
        vectorOutput->y = MCore::Compare<float>::CheckIfIsClose(inputY.y, 0.0f, MCore::Math::epsilon) == false ? (inputX.y / inputY.y) : 0.0f;
        vectorOutput->z = MCore::Compare<float>::CheckIfIsClose(inputY.z, 0.0f, MCore::Math::epsilon) == false ? (inputX.z / inputY.z) : 0.0f;
    }


    // calculate the angle
    void BlendTreeVector3Math2Node::CalculateAngleDegrees(const MCore::Vector3& inputX, const MCore::Vector3& inputY, MCore::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(vectorOutput);
        const float radians = MCore::Math::ACos(inputX.SafeNormalized().Dot(inputY.SafeNormalized()));
        *floatOutput = MCore::Math::RadiansToDegrees(radians);
    }
}   // namespace EMotionFX

