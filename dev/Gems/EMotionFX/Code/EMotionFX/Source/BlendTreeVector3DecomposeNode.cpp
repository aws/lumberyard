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
#include "BlendTreeVector3DecomposeNode.h"


namespace EMotionFX
{
    // constructor
    BlendTreeVector3DecomposeNode::BlendTreeVector3DecomposeNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeVector3DecomposeNode::~BlendTreeVector3DecomposeNode()
    {
    }


    // create
    BlendTreeVector3DecomposeNode* BlendTreeVector3DecomposeNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeVector3DecomposeNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeVector3DecomposeNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeVector3DecomposeNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPort("Vector", INPUTPORT_VECTOR, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_VECTOR);

        // setup the output ports
        InitOutputPorts(3);
        SetupOutputPort("x", OUTPUTPORT_X, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_X);
        SetupOutputPort("y", OUTPUTPORT_Y, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_Y);
        SetupOutputPort("z", OUTPUTPORT_Z, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_Z);
    }


    // register the parameters
    void BlendTreeVector3DecomposeNode::RegisterAttributes()
    {
    }


    // get the palette name
    const char* BlendTreeVector3DecomposeNode::GetPaletteName() const
    {
        return "Vector3 Decompose";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeVector3DecomposeNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeVector3DecomposeNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeVector3DecomposeNode* clone = new BlendTreeVector3DecomposeNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeVector3DecomposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if there are no incoming connections, there is nothing to do
        if (mConnections.GetLength() == 0)
        {
            return;
        }

        MCore::Vector3 value = GetInputVector3(animGraphInstance, INPUTPORT_VECTOR)->GetValue();
        GetOutputFloat(animGraphInstance, OUTPUTPORT_X)->SetValue(value.x);
        GetOutputFloat(animGraphInstance, OUTPUTPORT_Y)->SetValue(value.y);
        GetOutputFloat(animGraphInstance, OUTPUTPORT_Z)->SetValue(value.z);
    }


    // get the type string
    const char* BlendTreeVector3DecomposeNode::GetTypeString() const
    {
        return "BlendTreeVector3DecomposeNode";
    }
}   // namespace EMotionFX
