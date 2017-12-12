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
#include "BlendTreeVector2ComposeNode.h"


namespace EMotionFX
{
    // constructor
    BlendTreeVector2ComposeNode::BlendTreeVector2ComposeNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeVector2ComposeNode::~BlendTreeVector2ComposeNode()
    {
    }


    // create
    BlendTreeVector2ComposeNode* BlendTreeVector2ComposeNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeVector2ComposeNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeVector2ComposeNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeVector2ComposeNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X);
        SetupInputPortAsNumber("y", INPUTPORT_Y, PORTID_INPUT_Y);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Vector", OUTPUTPORT_VECTOR, MCore::AttributeVector2::TYPE_ID, PORTID_OUTPUT_VECTOR);
    }


    // register the parameters
    void BlendTreeVector2ComposeNode::RegisterAttributes()
    {
    }


    // get the palette name
    const char* BlendTreeVector2ComposeNode::GetPaletteName() const
    {
        return "Vector2 Compose";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeVector2ComposeNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeVector2ComposeNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeVector2ComposeNode* clone = new BlendTreeVector2ComposeNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeVector2ComposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        const float x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);
        const float y = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Y);
        GetOutputVector2(animGraphInstance, OUTPUTPORT_VECTOR)->SetValue(AZ::Vector2(x, y));
    }


    // get the type string
    const char* BlendTreeVector2ComposeNode::GetTypeString() const
    {
        return "BlendTreeVector2ComposeNode";
    }
}   // namespace EMotionFX
