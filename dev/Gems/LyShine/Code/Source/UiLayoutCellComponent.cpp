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
#include "LyShine_precompiled.h"
#include "UiLayoutCellComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutCellComponent::UiLayoutCellComponent()
    : m_minWidthOverridden(false)
    , m_minWidth(0.0f)
    , m_minHeightOverridden(false)
    , m_minHeight(0.0f)
    , m_targetWidthOverridden(false)
    , m_targetWidth(0.0f)
    , m_targetHeightOverridden(false)
    , m_targetHeight(0.0f)
    , m_extraWidthRatioOverridden(false)
    , m_extraWidthRatio(0.0f)
    , m_extraHeightRatioOverridden(false)
    , m_extraHeightRatio(0.0f)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutCellComponent::~UiLayoutCellComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetMinWidth()
{
    return m_minWidthOverridden ? m_minWidth : -1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetMinWidth(float width)
{
    if (width >= 0.0f)
    {
        m_minWidth = width;
        m_minWidthOverridden = true;
    }
    else
    {
        m_minWidth = -1.0f;
        m_minWidthOverridden = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetMinHeight()
{
    return m_minHeightOverridden ? m_minHeight : -1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetMinHeight(float height)
{
    if (height >= 0.0f)
    {
        m_minHeight = height;
        m_minHeightOverridden = true;
    }
    else
    {
        m_minHeight = -1.0f;
        m_minHeightOverridden = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetTargetWidth()
{
    return m_targetWidthOverridden ? m_targetWidth : -1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetTargetWidth(float width)
{
    if (width >= 0.0f)
    {
        m_targetWidth = width;
        m_targetWidthOverridden = true;
    }
    else
    {
        m_targetWidth = -1.0f;
        m_targetWidthOverridden = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetTargetHeight()
{
    return m_targetHeightOverridden ? m_targetHeight : -1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetTargetHeight(float height)
{
    if (height >= 0.0f)
    {
        m_targetHeight = height;
        m_targetHeightOverridden = true;
    }
    else
    {
        m_targetHeight = -1.0f;
        m_targetHeightOverridden = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetExtraWidthRatio()
{
    return m_extraWidthRatioOverridden ? m_extraWidthRatio : -1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetExtraWidthRatio(float width)
{
    if (width >= 0.0f)
    {
        m_extraWidthRatio = width;
        m_extraWidthRatioOverridden = true;
    }
    else
    {
        m_extraWidthRatio = -1.0f;
        m_extraWidthRatioOverridden = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutCellComponent::GetExtraHeightRatio()
{
    return m_extraHeightRatioOverridden ? m_extraHeightRatio : -1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::SetExtraHeightRatio(float height)
{
    if (height >= 0.0f)
    {
        m_extraHeightRatio = height;
        m_extraHeightRatioOverridden = true;
    }
    else
    {
        m_extraHeightRatio = -1.0f;
        m_extraHeightRatioOverridden = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiLayoutCellComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiLayoutCellComponent, AZ::Component>()
            ->Version(1)
            ->Field("MinWidthOverridden", &UiLayoutCellComponent::m_minWidthOverridden)
            ->Field("MinWidth", &UiLayoutCellComponent::m_minWidth)
            ->Field("MinHeightOverridden", &UiLayoutCellComponent::m_minHeightOverridden)
            ->Field("MinHeight", &UiLayoutCellComponent::m_minHeight)
            ->Field("TargetWidthOverridden", &UiLayoutCellComponent::m_targetWidthOverridden)
            ->Field("TargetWidth", &UiLayoutCellComponent::m_targetWidth)
            ->Field("TargetHeightOverridden", &UiLayoutCellComponent::m_targetHeightOverridden)
            ->Field("TargetHeight", &UiLayoutCellComponent::m_targetHeight)
            ->Field("ExtraWidthRatioOverridden", &UiLayoutCellComponent::m_extraWidthRatioOverridden)
            ->Field("ExtraWidthRatio", &UiLayoutCellComponent::m_extraWidthRatio)
            ->Field("ExtraHeightRatioOverridden", &UiLayoutCellComponent::m_extraHeightRatioOverridden)
            ->Field("ExtraHeightRatio", &UiLayoutCellComponent::m_extraHeightRatio);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiLayoutCellComponent>("LayoutCell", "Allows default layout cell properties to be overridden.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiLayoutCell.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiLayoutCell.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_minWidthOverridden, "Min Width",
                "Check this box to override the minimum width.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_minWidth, "Value",
                "Specify minimum width.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_minWidthOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_minHeightOverridden, "Min Height",
                "Check this box to override the minimum height.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_minHeight, "Value",
                "Specify minimum height.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_minHeightOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_targetWidthOverridden, "Target Width",
                "Check this box to override the target width.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_targetWidth, "Value",
                "Specify target width.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_targetWidthOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_targetHeightOverridden, "Target Height",
                "Check this box to override the target height.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_targetHeight, "Value",
                "Specify target height.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_targetHeightOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_extraWidthRatioOverridden, "Extra Width Ratio",
                "Check this box to override the extra width ratio.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_extraWidthRatio, "Value",
                "Specify extra width ratio.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_extraWidthRatioOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutCellComponent::m_extraHeightRatioOverridden, "Extra Height Ratio",
                "Check this box to override the extra height ratio.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);

            editInfo->DataElement(0, &UiLayoutCellComponent::m_extraHeightRatio, "Value",
                "Specify extra height ratio.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiLayoutCellComponent::m_extraHeightRatioOverridden)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutCellComponent::InvalidateParentLayout);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiLayoutCellBus>("UiLayoutCellBus")
            ->Event("GetMinWidth", &UiLayoutCellBus::Events::GetMinWidth)
            ->Event("SetMinWidth", &UiLayoutCellBus::Events::SetMinWidth)
            ->Event("GetMinHeight", &UiLayoutCellBus::Events::GetMinHeight)
            ->Event("SetMinHeight", &UiLayoutCellBus::Events::SetMinHeight)
            ->Event("GetTargetWidth", &UiLayoutCellBus::Events::GetTargetWidth)
            ->Event("SetTargetWidth", &UiLayoutCellBus::Events::SetTargetWidth)
            ->Event("GetTargetHeight", &UiLayoutCellBus::Events::GetTargetHeight)
            ->Event("SetTargetHeight", &UiLayoutCellBus::Events::SetTargetHeight)
            ->Event("GetExtraWidthRatio", &UiLayoutCellBus::Events::GetExtraWidthRatio)
            ->Event("SetExtraWidthRatio", &UiLayoutCellBus::Events::SetExtraWidthRatio)
            ->Event("GetExtraHeightRatio", &UiLayoutCellBus::Events::GetExtraHeightRatio)
            ->Event("SetExtraHeightRatio", &UiLayoutCellBus::Events::SetExtraHeightRatio)
            ->VirtualProperty("MinWidth", "GetMinWidth", "SetMinWidth")
            ->VirtualProperty("MinHeight", "GetMinHeight", "SetMinHeight")
            ->VirtualProperty("TargetWidth", "GetTargetWidth", "SetTargetWidth")
            ->VirtualProperty("TargetHeight", "GetTargetHeight", "SetTargetHeight")
            ->VirtualProperty("ExtraWidthRatio", "GetExtraWidthRatio", "SetExtraWidthRatio")
            ->VirtualProperty("ExtraHeightRatio", "GetExtraHeightRatio", "SetExtraHeightRatio");

        behaviorContext->Class<UiLayoutCellComponent>()->RequestBus("UiLayoutCellBus");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::Activate()
{
    UiLayoutCellBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::Deactivate()
{
    UiLayoutCellBus::Handler::BusDisconnect();

    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutCellComponent::InvalidateParentLayout()
{
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    EBUS_EVENT_ID(canvasEntityId, UiLayoutManagerBus, MarkToRecomputeLayoutsAffectedByLayoutCellChange, GetEntityId(), false);
}