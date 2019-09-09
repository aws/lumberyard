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
#include "BlendTreeSmoothingNode.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"

#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeSmoothingNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeSmoothingNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

    const float BlendTreeSmoothingNode::s_targetReachedRelativeTolerance = 0.01f;

    BlendTreeSmoothingNode::BlendTreeSmoothingNode()
        : AnimGraphNode()
        , m_interpolationSpeed(0.75f)
        , m_startValue(0.0f)
        , m_useStartValue(false)
    {
        // create the input ports
        InitInputPorts(1);
        SetupInputPortAsNumber("Dest", INPUTPORT_DEST, PORTID_INPUT_DEST);

        // create the output ports
        InitOutputPorts(1);
        SetupOutputPort("Result", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);
    }


    BlendTreeSmoothingNode::~BlendTreeSmoothingNode()
    {
    }


    bool BlendTreeSmoothingNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeSmoothingNode::GetPaletteName() const
    {
        return "Smoothing";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeSmoothingNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // update
    void BlendTreeSmoothingNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all incoming nodes
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));

        // if there are no incoming connections, there is nothing to do
        if (mConnections.size() == 0)
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(0.0f);
            return;
        }

        // if we are disabled, output the dest value directly
        //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_DEST) );
        const float destValue = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_DEST);
        if (mDisabled)
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(destValue);
            return;
        }

        // perform interpolation
        const float sourceValue = uniqueData->mCurrentValue;
        const float interpolationSpeed = m_interpolationSpeed * uniqueData->mFrameDeltaTime * 10.0f;
        const float interpolationResult = (interpolationSpeed < 0.99999f) ? MCore::LinearInterpolate<float>(sourceValue, destValue, interpolationSpeed) : destValue;
        // If the percentage error is 1%, or less, snap to the destination value
        if (AZ::IsClose((interpolationResult - destValue) / destValue, 0.0f, s_targetReachedRelativeTolerance))
        {
            uniqueData->mCurrentValue = destValue;
        }
        else
        {
            // pass the interpolated result to the output port and the current value of the unique data
            uniqueData->mCurrentValue = interpolationResult;
        }
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(interpolationResult);

        uniqueData->mFrameDeltaTime = timePassedInSeconds;
    }


    // rewind
    void BlendTreeSmoothingNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data for this node, if it doesn't exist yet, create it
        UniqueData* uniqueData = static_cast<BlendTreeSmoothingNode::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // check if the current value needs to be reset to the input or the start value when rewinding the node
        if (m_useStartValue)
        {
            uniqueData->mCurrentValue = m_startValue;
        }
        else
        {
            // set the current value to the current input value
            UpdateAllIncomingNodes(animGraphInstance, 0.0f);
            //      OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_DEST) );
            uniqueData->mCurrentValue = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_DEST);
        }
    }


    // when attributes have changed their value
    void BlendTreeSmoothingNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data for this node, if it doesn't exist yet, create it
        UniqueData* uniqueData = static_cast<BlendTreeSmoothingNode::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = aznew UniqueData(this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
            uniqueData->mCurrentValue = 0.0f;//GetInputNumber( INPUTPORT_DEST );
        }

        // set the source value to the current input destination value
        //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_DEST) );

        if (GetInputNode(INPUTPORT_DEST) == nullptr)
        {
            uniqueData->mCurrentValue = 0.0f;//GetInputNumber( INPUTPORT_DEST );
        }
    }



    AZ::Color BlendTreeSmoothingNode::GetVisualColor() const
    {
        return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
    }


    bool BlendTreeSmoothingNode::GetSupportsDisable() const
    {
        return true;
    }


    AZ::Crc32 BlendTreeSmoothingNode::GetStartValueVisibility() const
    {
        return m_useStartValue ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void BlendTreeSmoothingNode::SetInterpolationSpeed(float interpolationSpeed)
    {
        m_interpolationSpeed = interpolationSpeed;
    }

    void BlendTreeSmoothingNode::SetStartVAlue(float startValue)
    {
        m_startValue = startValue;
    }

    void BlendTreeSmoothingNode::SetUseStartVAlue(bool useStartValue)
    {
        m_useStartValue = useStartValue;
    }


    void BlendTreeSmoothingNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeSmoothingNode, AnimGraphNode>()
            ->Version(1)
            ->Field("interpolationSpeed", &BlendTreeSmoothingNode::m_interpolationSpeed)
            ->Field("useStartValue", &BlendTreeSmoothingNode::m_useStartValue)
            ->Field("startValue", &BlendTreeSmoothingNode::m_startValue)
        ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeSmoothingNode>("Smoothing", "Smoothing attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Slider, &BlendTreeSmoothingNode::m_interpolationSpeed, "Interpolation Speed", "The interpolation speed where 0.0 means the value won't change at all and 1.0 means the input value will directly be mapped to the output value.")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeSmoothingNode::m_useStartValue, "Use Start Value", "Enable this to use the start value, otherwise the first input value will be used as start value.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeSmoothingNode::m_startValue, "Start Value", "When the blend tree gets activated the smoothing node will start interpolating from this value.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeSmoothingNode::GetStartValueVisibility)
            ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
            ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
        ;
    }
} // namespace EMotionFX