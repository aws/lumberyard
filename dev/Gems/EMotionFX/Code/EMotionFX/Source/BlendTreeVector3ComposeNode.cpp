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
#include "BlendTreeVector3ComposeNode.h"


namespace EMotionFX
{
    // constructor
    BlendTreeVector3ComposeNode::BlendTreeVector3ComposeNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeVector3ComposeNode::~BlendTreeVector3ComposeNode()
    {
    }


    // create
    BlendTreeVector3ComposeNode* BlendTreeVector3ComposeNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeVector3ComposeNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeVector3ComposeNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeVector3ComposeNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(3);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X);
        SetupInputPortAsNumber("y", INPUTPORT_Y, PORTID_INPUT_Y);
        SetupInputPortAsNumber("z", INPUTPORT_Z, PORTID_INPUT_Z);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Vector", OUTPUTPORT_VECTOR, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_VECTOR);
    }


    // register the parameters
    void BlendTreeVector3ComposeNode::RegisterAttributes()
    {
    }


    // get the palette name
    const char* BlendTreeVector3ComposeNode::GetPaletteName() const
    {
        return "Vector3 Compose";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeVector3ComposeNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeVector3ComposeNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeVector3ComposeNode* clone = new BlendTreeVector3ComposeNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeVector3ComposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        const float x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);
        const float y = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Y);
        const float z = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Z);
        GetOutputVector3(animGraphInstance, OUTPUTPORT_VECTOR)->SetValue(MCore::Vector3(x, y, z));
    }


    // get the type string
    const char* BlendTreeVector3ComposeNode::GetTypeString() const
    {
        return "BlendTreeVector3ComposeNode";
    }
}   // namespace EMotionFX
