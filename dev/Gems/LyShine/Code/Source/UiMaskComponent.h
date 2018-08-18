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

#include <LyShine/Bus/UiRenderControlBus.h>
#include <LyShine/Bus/UiMaskBus.h>
#include <LyShine/Bus/UiInteractionMaskBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/Component.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiMaskComponent
    : public AZ::Component
    , public UiRenderControlBus::Handler
    , public UiMaskBus::Handler
    , public UiInteractionMaskBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiMaskComponent, LyShine::UiMaskComponentUuid, AZ::Component);

    UiMaskComponent();
    ~UiMaskComponent() override;

    // UiRenderControlInterface
    void SetupBeforeRenderingComponents(Pass pass) override;
    void SetupAfterRenderingComponents(Pass pass) override;
    void SetupAfterRenderingChildren(bool& isSecondComponentsPassRequired) override;
    // ~UiRenderControlInterface

    // UiMaskInterface
    bool GetIsMaskingEnabled() override;
    void SetIsMaskingEnabled(bool enableMasking) override;
    bool GetIsInteractionMaskingEnabled() override;
    void SetIsInteractionMaskingEnabled(bool enableMasking) override;
    bool GetDrawBehind() override;
    void SetDrawBehind(bool drawMaskVisualBehindChildren) override;
    bool GetDrawInFront() override;
    void SetDrawInFront(bool drawMaskVisualInFrontOfChildren) override;
    bool GetUseAlphaTest() override;
    void SetUseAlphaTest(bool useAlphaTest) override;
    // ~UiMaskInterface

    // UiInteractionMaskInterface
    bool IsPointMasked(AZ::Vector2 point) override;
    // ~UiInteractionMaskInterface

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiMaskService", 0x806f6dac));
        provided.push_back(AZ_CRC("UiRenderControlService", 0x4e302454));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiRenderControlService", 0x4e302454));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // Note that the UiVisualService is not required because a child mask element can be used instead.
        required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
        required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiMaskComponent);

private: // member functions

    //! Method used to populate the drop down for the m_childMaskElement property field
    using EntityComboBoxVec = AZStd::vector< AZStd::pair< AZ::EntityId, AZStd::string > >;
    EntityComboBoxVec PopulateChildEntityList();

private: // data

    //! flag allows for easy debugging, also can be used to turn mask on/off from C++
    //! or in an animation.
    bool m_enableMasking;

    //! flags to control whether the mask is drawn to color buffer as well as to the
    //! stencil in the first and second passes
    bool m_drawMaskVisualBehindChildren;
    bool m_drawMaskVisualInFrontOfChildren;

    //! Whether to enable alphatest when drawing mask visual.
    bool m_useAlphaTest;

    //! Whether to mask interaction
    bool m_maskInteraction;

    //! An optional child element that defines additional mask visuals
    AZ::EntityId m_childMaskElement;

    //! Saved prior base state from before rendering (non-persistent variable, only used during rendering)
    int m_priorBaseState;

    //! Saved enabled state of the child mask element
    bool m_priorChildMaskElementIsEnabled = true;
};
