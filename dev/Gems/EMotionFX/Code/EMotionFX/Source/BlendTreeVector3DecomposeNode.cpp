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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeVector3DecomposeNode.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeVector3DecomposeNode, AnimGraphAllocator, 0)


    BlendTreeVector3DecomposeNode::BlendTreeVector3DecomposeNode()
        : AnimGraphNode()
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

    
    BlendTreeVector3DecomposeNode::~BlendTreeVector3DecomposeNode()
    {
    }


    bool BlendTreeVector3DecomposeNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
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


    // the update function
    void BlendTreeVector3DecomposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if there are no incoming connections, there is nothing to do
        if (mConnections.size() == 0)
        {
            return;
        }

        AZ::Vector3 value(GetInputVector3(animGraphInstance, INPUTPORT_VECTOR)->GetValue());
        GetOutputFloat(animGraphInstance, OUTPUTPORT_X)->SetValue(value.GetX());
        GetOutputFloat(animGraphInstance, OUTPUTPORT_Y)->SetValue(value.GetY());
        GetOutputFloat(animGraphInstance, OUTPUTPORT_Z)->SetValue(value.GetZ());
    }


    void BlendTreeVector3DecomposeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeVector3DecomposeNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeVector3DecomposeNode>("Vector3 Decompose", "Vector3 decompose attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX