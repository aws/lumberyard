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

#include <LyShine/Bus/UiUpdateBus.h>
#include <LyShine/Bus/UiFaderBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/Component.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiFaderComponent
    : public AZ::Component
    , public UiUpdateBus::Handler
    , public UiFaderBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiFaderComponent, LyShine::UiFaderComponentUuid, AZ::Component);

    UiFaderComponent();
    ~UiFaderComponent() override;

    // UiUpdateInterface
    void Update() override;
    // ~UiUpdateInterface

    // UiFaderInterface
    float GetFadeValue() override;
    void SetFadeValue(float fade) override;
    void Fade(float targetValue, float speed) override;
    bool IsFading() override;
    // ~UiFaderInterface

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiFaderService", 0x3c5847e9));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiFaderService", 0x3c5847e9));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
        required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
    }

    static void Reflect(AZ::ReflectContext* context);

    //! Helper function for visual components to compute their fade value
    static float ComputeElementFadeValue(AZ::Entity* element);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    void CompleteFade();

    AZ_DISABLE_COPY_MOVE(UiFaderComponent);

private: // data

    float m_fade;

    // Used for fade animation
    bool m_isFading;
    float m_fadeTarget;
    float m_fadeSpeedInSeconds;
};
