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
#pragma once

#include <LyShine/Bus/UiCanvasUpdateNotificationBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiTooltipDataPopulatorBus.h>
#include <LyShine/Bus/UiTooltipBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/Component.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTooltipComponent
    : public AZ::Component
    , public UiCanvasUpdateNotificationBus::Handler
    , public UiInteractableNotificationBus::Handler
    , public UiTooltipDataPopulatorBus::Handler
    , public UiTooltipBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiTooltipComponent, LyShine::UiTooltipComponentUuid, AZ::Component);

    UiTooltipComponent();
    ~UiTooltipComponent() override;

    // UiCanvasUpdateNotification
    void Update(float deltaTime) override;
    // ~UiCanvasUpdateNotification

    // UiInteractableNotifications
    void OnHoverStart() override;
    void OnHoverEnd() override;
    void OnPressed() override;
    void OnReleased() override;
    // ~UiInteractableNotifications

    // UiTooltipDataPopulatorInterface
    virtual void PushDataToDisplayElement(AZ::EntityId displayEntityId) override;
    // ~UiTooltipDataPopulatorInterface

    // UiTooltipInterface
    virtual AZStd::string GetText() override;
    virtual void SetText(const AZStd::string& text) override;
    // ~UiTooltipInterface

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiTooltipService", 0x78437544));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiTooltipService", 0x78437544));
        incompatible.push_back(AZ_CRC("UiTooltipDisplayService", 0xb2f093fd));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
        required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
        required.push_back(AZ_CRC("UiInteractableService", 0x1d474c98));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiTooltipComponent);

    void HideDisplayElement();

protected: // data

    //! The tooltip text
    AZStd::string m_text;

    AZ::EntityId m_displayElementId;
};
