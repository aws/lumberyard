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

#ifndef NETWORKFEATURETESTS_PLAYERCONTROLLER_H
#define NETWORKFEATURETESTS_PLAYERCONTROLLER_H

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <ActionMap.h>


namespace NetworkFeatureTest
{
    class PlayerControllerComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public IActionListener
    {
    public:
        AZ_COMPONENT(PlayerControllerComponent, "{F5AA3A59-EF8B-4CC8-A37E-1CB7E6669B62}");

        PlayerControllerComponent();
        ~PlayerControllerComponent();

        ///////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        ///////////////////

        ///////////////////
        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ///////////////////

        ///////////////////
        // IActionListener
        void OnAction(const ActionId& action, int activationMode, float value) override;
        ///////////////////

    private:

        void OnActionPressed(const ActionId& action, float value);
        void OnActionHeld(const ActionId& action, float value);
        void OnActionReleased(const ActionId& action, float value);
        void OnActionAlways(const ActionId& action, float value);

        float m_forwardScalar;
        float m_horizontalScalar;
        float m_translationSpeed;

        float m_rotationSpeed;

        bool m_isRotationScalar;
        float m_yawRotation;
        float m_pitchRotation;

        bool m_allowJump;
        bool m_shouldJump;
    };
}

#endif