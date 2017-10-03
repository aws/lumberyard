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
#include "BlendTreeVector4ComposeNode.h"


namespace EMotionFX
{
    // constructor
    BlendTreeVector4ComposeNode::BlendTreeVector4ComposeNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeVector4ComposeNode::~BlendTreeVector4ComposeNode()
    {
    }


    // create
    BlendTreeVector4ComposeNode* BlendTreeVector4ComposeNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeVector4ComposeNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeVector4ComposeNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeVector4ComposeNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(4);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X);
        SetupInputPortAsNumber("y", INPUTPORT_Y, PORTID_INPUT_Y);
        SetupInputPortAsNumber("z", INPUTPORT_Z, PORTID_INPUT_Z);
        SetupInputPortAsNumber("w", INPUTPORT_W, PORTID_INPUT_W);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Vector", OUTPUTPORT_VECTOR, MCore::AttributeVector4::TYPE_ID, PORTID_OUTPUT_VECTOR);
    }


    // register the parameters
    void BlendTreeVector4ComposeNode::RegisterAttributes()
    {
    }


    // get the palette name
    const char* BlendTreeVector4ComposeNode::GetPaletteName() const
    {
        return "Vector4 Compose";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeVector4ComposeNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeVector4ComposeNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeVector4ComposeNode* clone = new BlendTreeVector4ComposeNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeVector4ComposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        const float x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);
        const float y = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Y);
        const float z = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Z);
        const float w = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_W);
        GetOutputVector4(animGraphInstance, OUTPUTPORT_VECTOR)->SetValue(AZ::Vector4(x, y, z, w));
    }


    // get the type string
    const char* BlendTreeVector4ComposeNode::GetTypeString() const
    {
        return "BlendTreeVector4ComposeNode";
    }
}   // namespace EMotionFX
