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
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <IInput.h>
#include "FlyCameraInputBus.h"

namespace LYGame
{
    class FlyCameraInputComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public IInputEventListener
        , public FlyCameraInputBus::Handler
    {
    public:
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void Reflect(AZ::ReflectContext* reflection);

        AZ_COMPONENT(FlyCameraInputComponent, "{EB588B1E-AC2E-44AA-A1E6-E5960942E950}");
        virtual ~FlyCameraInputComponent();

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // IInputEventListener
        bool OnInputEvent(const SInputEvent& inputEvent) override;

        // FlyCameraInputInterface
        void SetIsEnabled(bool isEnabled) override;
        bool GetIsEnabled() override;

    private:
        void OnMouseEvent(const SInputEvent& inputEvent);
        void OnKeyboardEvent(const SInputEvent& inputEvent);
        void OnGamepadEvent(const SInputEvent& inputEvent);
        void OnTouchEvent(const SInputEvent& inputEvent);
        void OnVirtualLeftThumbstickEvent(const SInputEvent& inputEvent);
        void OnVirtualRightThumbstickEvent(const SInputEvent& inputEvent);

    private:
        // Editable Properties
        float m_moveSpeed = 20.0f;
        float m_rotationSpeed = 5.0f;

        float m_mouseSensitivity = 0.025f;
        float m_virtualThumbstickRadiusAsPercentageOfScreenWidth = 0.1f;

        bool m_InvertRotationInputAxisX = false;
        bool m_InvertRotationInputAxisY = false;

        bool m_isEnabled = true;

        // Run-time Properties
        Vec2 m_movement = ZERO;
        Vec2 m_rotation = ZERO;

        Vec2 m_leftDownPosition = ZERO;
        EKeyId m_leftFingerId = eKI_Unknown;

        Vec2 m_rightDownPosition = ZERO;
        EKeyId m_rightFingerId = eKI_Unknown;

        int m_thumbstickTextureId = 0;
    };
} // LYGame
