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

// include the required headers
#include "EMotionFXConfig.h"
#include "BlendTreeDirectionToWeightNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    // constructor
    BlendTreeDirectionToWeightNode::BlendTreeDirectionToWeightNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeDirectionToWeightNode::~BlendTreeDirectionToWeightNode()
    {
    }


    // create
    BlendTreeDirectionToWeightNode* BlendTreeDirectionToWeightNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeDirectionToWeightNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeDirectionToWeightNode::CreateObjectData()
    {
        return AnimGraphNodeData::Create(this, nullptr);
    }


    // register the ports
    void BlendTreeDirectionToWeightNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPort("Direction X", INPUTPORT_DIRECTION_X, MCore::AttributeFloat::TYPE_ID, PORTID_INPUT_DIRECTION_X);
        SetupInputPort("Direction Y", INPUTPORT_DIRECTION_Y, MCore::AttributeFloat::TYPE_ID, PORTID_INPUT_DIRECTION_Y);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Weight", OUTPUTPORT_WEIGHT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_WEIGHT);
    }


    // register the parameters
    void BlendTreeDirectionToWeightNode::RegisterAttributes()
    {
    }


    // get the palette name
    const char* BlendTreeDirectionToWeightNode::GetPaletteName() const
    {
        return "Direction To Weight";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeDirectionToWeightNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeDirectionToWeightNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeDirectionToWeightNode* clone = new BlendTreeDirectionToWeightNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // the update function
    void BlendTreeDirectionToWeightNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        //AnimGraphNodeData* uniqueData = animGraphInstance->FindUniqueNodeData(this);

        // if there are less than two incoming connections, there is nothing to do
        const uint32 numConnections = mConnections.GetLength();
        if (numConnections < 2 || mDisabled)
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_WEIGHT)->SetValue(0.0f);
            return;
        }

        // get the input values as a floats
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_DIRECTION_X));
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_DIRECTION_Y));

        float directionX = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_DIRECTION_X);
        float directionY = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_DIRECTION_Y);

        // in case the direction is very close to the origin, default it to the x-axis
        if (MCore::Compare<float>::CheckIfIsClose(directionX, 0.0f, MCore::Math::epsilon) && MCore::Compare<float>::CheckIfIsClose(directionY, 0.0f, MCore::Math::epsilon))
        {
            directionX = 1.0f;
            directionY = 0.0f;
        }

        // convert it to a vector2 and normalize it
        AZ::Vector2 direction(directionX, directionY);
        direction.Normalize();

        // right side
        float alpha = 0.0f;
        if (directionX >= 0.0f)
        {
            // up-right
            if (directionY >= 0.0f)
            {
                alpha = MCore::Math::ACos(direction.GetX());
            }
            // down-right
            else
            {
                alpha = MCore::Math::twoPi - MCore::Math::ACos(direction.GetX());
            }
        }
        // left side
        else
        {
            // up-left
            if (directionY >= 0.0f)
            {
                alpha = MCore::Math::halfPi + MCore::Math::ACos(direction.GetY());
            }
            // down-left
            else
            {
                alpha = MCore::Math::pi + MCore::Math::ACos(-direction.GetX());
            }
        }

        // rescale from the radian circle to a normalized value
        const float result = alpha / MCore::Math::twoPi;

        // update the output value
        GetOutputFloat(animGraphInstance, OUTPUTPORT_WEIGHT)->SetValue(result);
    }


    // get the blend node type string
    const char* BlendTreeDirectionToWeightNode::GetTypeString() const
    {
        return "BlendTreeDirectionToWeightNode";
    }


    uint32 BlendTreeDirectionToWeightNode::GetVisualColor() const
    {
        return MCore::RGBA(50, 200, 50);
    }
}   // namespace EMotionFX

