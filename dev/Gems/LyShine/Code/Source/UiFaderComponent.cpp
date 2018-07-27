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
#include "UiFaderComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/IUiRenderer.h>

#include <ITimer.h>

#include "UiSerialize.h"

// BehaviorContext UiFaderNotificationBus forwarder
class BehaviorUiFaderNotificationBusHandler
    : public UiFaderNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiFaderNotificationBusHandler, "{CAD44770-3D5E-4E67-8F05-D2A89E8C501A}", AZ::SystemAllocator,
        OnFadeComplete, OnFadeInterrupted, OnFaderDestroyed);

    void OnFadeComplete() override
    {
        Call(FN_OnFadeComplete);
    }

    void OnFadeInterrupted() override
    {
        Call(FN_OnFadeInterrupted);
    }

    void OnFaderDestroyed() override
    {
        Call(FN_OnFaderDestroyed);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiFaderComponent::UiFaderComponent()
    : m_fade(1.0f)
    , m_isFading(false)
    , m_fadeTarget(1.0f)
    , m_fadeSpeedInSeconds(1.0f)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiFaderComponent::~UiFaderComponent()
{
    if (m_isFading && m_entity)
    {
        EBUS_EVENT_ID(GetEntityId(), UiFaderNotificationBus, OnFaderDestroyed);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::Update(float deltaTime)
{
    if (!m_isFading)
    {
        return;
    }

    // Update fade
    m_fade += m_fadeSpeedInSeconds * deltaTime;

    // Check for completion
    if (m_fadeSpeedInSeconds == 0 ||
        m_fadeSpeedInSeconds > 0 && m_fade >= m_fadeTarget ||
        m_fadeSpeedInSeconds < 0 && m_fade <= m_fadeTarget)
    {
        CompleteFade();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::SetupBeforeRenderingComponents(Pass pass)
{
    if (pass == Pass::First)
    {
        IUiRenderer::Get()->PushAlphaFade(m_fade);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::SetupAfterRenderingComponents(Pass /* pass */)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::SetupAfterRenderingChildren(bool& /* isSecondComponentsPassRequired */)
{
    IUiRenderer::Get()->PopAlphaFade();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiFaderComponent::GetFadeValue()
{
    return m_fade;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::SetFadeValue(float fade)
{
    if (m_isFading)
    {
        EBUS_EVENT_ID(GetEntityId(), UiFaderNotificationBus, OnFadeInterrupted);
        m_isFading = false;
    }

    m_fade = fade;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::Fade(float targetValue, float speed)
{
    if (m_isFading)
    {
        EBUS_EVENT_ID(GetEntityId(), UiFaderNotificationBus, OnFadeInterrupted);
    }

    m_isFading = true;
    m_fadeTarget = clamp_tpl(targetValue, 0.0f, 1.0f);

    // Give speed a direction
    float fadeChange = m_fadeTarget - m_fade;
    float fadeDirection = fadeChange >= 0.0f ? 1.0f : -1.0f;
    m_fadeSpeedInSeconds = fadeDirection * speed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiFaderComponent::IsFading()
{
    return m_isFading;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiFaderComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiFaderComponent, AZ::Component>()
            ->Version(1)
            ->Field("Fade", &UiFaderComponent::m_fade);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiFaderComponent>("Fader", "A component that can fade its element and all its child elements");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiFader.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiFader.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiFaderComponent::m_fade, "Fade", "The initial fade value")
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiFaderBus>("UiFaderBus")
            ->Event("GetFadeValue", &UiFaderBus::Events::GetFadeValue)
            ->Event("SetFadeValue", &UiFaderBus::Events::SetFadeValue)
            ->Event("Fade", &UiFaderBus::Events::Fade)
            ->Event("IsFading", &UiFaderBus::Events::IsFading)
            ->VirtualProperty("Fade", "GetFadeValue", "SetFadeValue");

        behaviorContext->Class<UiFaderComponent>()->RequestBus("UiFaderBus");

        behaviorContext->EBus<UiFaderNotificationBus>("UiFaderNotificationBus")
            ->Handler<BehaviorUiFaderNotificationBusHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiFaderComponent::ComputeElementFadeValue(AZ::Entity* element)
{
    // NOTE: This is a short-term solution. In the longer term I want to use eBus.
    // We would probably cache the accumulate fade value in the visual components and visual
    // components could listen for update events from their closest ancestor element that can
    // generate those events (i.e. has a fader component)
    float fade = 1.0f;
    auto curElem = element;
    while (curElem)
    {
        float fadeValue = 1.0f;
        EBUS_EVENT_ID_RESULT(fadeValue, curElem->GetId(), UiFaderBus, GetFadeValue);
        fade *= fadeValue;

        EBUS_EVENT_ID_RESULT(curElem, curElem->GetId(), UiElementBus, GetParent);
    }

    return fade;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::Activate()
{
    UiUpdateBus::Handler::BusConnect(m_entity->GetId());
    UiRenderControlBus::Handler::BusConnect(m_entity->GetId());
    UiFaderBus::Handler::BusConnect(m_entity->GetId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::Deactivate()
{
    UiUpdateBus::Handler::BusDisconnect();
    UiRenderControlBus::Handler::BusDisconnect();
    UiFaderBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::CompleteFade()
{
    m_fade = m_fadeTarget;
    // Queue the OnFadeComplete event to prevent deletions during the canvas update
    EBUS_QUEUE_EVENT_ID(GetEntityId(), UiFaderNotificationBus, OnFadeComplete);
    m_isFading = false;
}
