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
#include "UiTooltipComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTooltipDisplayBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTooltipComponent::UiTooltipComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTooltipComponent::~UiTooltipComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::Update(float deltaTime)
{
    if (m_displayElementId.IsValid())
    {
        EBUS_EVENT_ID(m_displayElementId, UiTooltipDisplayBus, Update);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::OnHoverStart()
{
    // Get display element
    AZ::EntityId canvasId;
    EBUS_EVENT_ID_RESULT(canvasId, GetEntityId(), UiElementBus, GetCanvasEntityId);

    EBUS_EVENT_ID_RESULT(m_displayElementId, canvasId, UiCanvasBus, GetTooltipDisplayElement);

    // Show display element
    if (m_displayElementId.IsValid())
    {
        EBUS_EVENT_ID(m_displayElementId, UiTooltipDisplayBus, PrepareToShow, GetEntityId());

        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
        if (canvasEntityId.IsValid())
        {
            UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::OnHoverEnd()
{
    HideDisplayElement();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::OnPressed()
{
    HideDisplayElement();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::OnReleased()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::PushDataToDisplayElement(AZ::EntityId displayEntityId)
{
    AZ::EntityId textEntityId;
    EBUS_EVENT_ID_RESULT(textEntityId, displayEntityId, UiTooltipDisplayBus, GetTextEntity);

    if (textEntityId.IsValid())
    {
        EBUS_EVENT_ID(textEntityId, UiTextBus, SetText, m_text);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiTooltipComponent::GetText()
{
    return m_text;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::SetText(const AZStd::string& text)
{
    m_text = text;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiTooltipComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiTooltipComponent, AZ::Component>()
            ->Version(1)
            ->Field("Text", &UiTooltipComponent::m_text);

        AZ::EditContext* ec = serializeContext->GetEditContext();

        if (ec)
        {
            auto editInfo = ec->Class<UiTooltipComponent>("Tooltip", "A component that provides the data needed to display a tooltip.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiTooltip.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiTooltip.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(0, &UiTooltipComponent::m_text, "Text", "The text string.");
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiTooltipBus>("UiTooltipBus")
            ->Event("GetText", &UiTooltipBus::Events::GetText)
            ->Event("SetText", &UiTooltipBus::Events::SetText);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::Activate()
{
    UiInteractableNotificationBus::Handler::BusConnect(GetEntityId());
    UiTooltipDataPopulatorBus::Handler::BusConnect(GetEntityId());
    UiTooltipBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::Deactivate()
{
    UiCanvasUpdateNotificationBus::Handler::BusDisconnect();
    UiInteractableNotificationBus::Handler::BusDisconnect();
    UiTooltipDataPopulatorBus::Handler::BusDisconnect();
    UiTooltipBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::HideDisplayElement()
{
    if (m_displayElementId.IsValid())
    {
        EBUS_EVENT_ID(m_displayElementId, UiTooltipDisplayBus, Hide);
        m_displayElementId.SetInvalid();
        UiCanvasUpdateNotificationBus::Handler::BusDisconnect();
    }
}
