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
#include "BlendTreeVector4DecomposeNode.h"


namespace EMotionFX
{
    // constructor
    BlendTreeVector4DecomposeNode::BlendTreeVector4DecomposeNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeVector4DecomposeNode::~BlendTreeVector4DecomposeNode()
    {
    }


    // create
    BlendTreeVector4DecomposeNode* BlendTreeVector4DecomposeNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeVector4DecomposeNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeVector4DecomposeNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeVector4DecomposeNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPort("Vector", INPUTPORT_VECTOR, MCore::AttributeVector4::TYPE_ID, PORTID_INPUT_VECTOR);

        // setup the output ports
        InitOutputPorts(4);
        SetupOutputPort("x", OUTPUTPORT_X, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_X);
        SetupOutputPort("y", OUTPUTPORT_Y, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_Y);
        SetupOutputPort("z", OUTPUTPORT_Z, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_Z);
        SetupOutputPort("w", OUTPUTPORT_W, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_W);
    }


    // register the parameters
    void BlendTreeVector4DecomposeNode::RegisterAttributes()
    {
    }


    // get the palette name
    const char* BlendTreeVector4DecomposeNode::GetPaletteName() const
    {
        return "Vector4 Decompose";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeVector4DecomposeNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeVector4DecomposeNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeVector4DecomposeNode* clone = new BlendTreeVector4DecomposeNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeVector4DecomposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if there are no incoming connections, there is nothing to do
        if (mConnections.GetLength() == 0)
        {
            return;
        }

        AZ::Vector4 value = GetInputVector4(animGraphInstance, INPUTPORT_VECTOR)->GetValue();
        GetOutputFloat(animGraphInstance, OUTPUTPORT_X)->SetValue(value.GetX());
        GetOutputFloat(animGraphInstance, OUTPUTPORT_Y)->SetValue(value.GetY());
        GetOutputFloat(animGraphInstance, OUTPUTPORT_Z)->SetValue(value.GetZ());
        GetOutputFloat(animGraphInstance, OUTPUTPORT_W)->SetValue(value.GetW());
    }


    // get the type string
    const char* BlendTreeVector4DecomposeNode::GetTypeString() const
    {
        return "BlendTreeVector4DecomposeNode";
    }
}   // namespace EMotionFX
