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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Component/EntityId.h>

namespace AzToolsFramework
{
    namespace ViewportInteraction
    {
        /**
         * The camera details should come from the runtime camera component itself rather
         * than from the specialized cameras. In other words, the details should represent the 
         * state of the actual camera.
         */
        struct CameraState
        {
            AZ::Vector3 m_location = AZ::Vector3::CreateZero();
            AZ::Vector3 m_forward = AZ::Vector3(0.0f, 0.0f, 1.0f);
            AZ::Vector3 m_up = AZ::Vector3(0.0f, 1.0f, 0.0f);
            AZ::Vector3 m_side = AZ::Vector3(1.0f, 0.0f, 0.0f);

            union
            {
                float m_verticalFovRadian = 0.0f; ///< when the camera is in perspective
                float m_zoom; ///< when the camera is in orthographic
            };
            
            AZ::Vector2 m_viewportSize = AZ::Vector2::CreateZero();
            float m_nearClip = 0.01f;
            float m_farClip = 100.0f;
            bool m_valid = false;
            bool m_isOrthographic = false;
        };

        enum class KeyboardModifier : AZ::u32
        {
            None = 0,
            Alt = 0x01,
            Shift = 0x02,
            Ctrl = 0x04
        };

        struct KeyboardModifiers
        {
            KeyboardModifiers() = default;
            explicit KeyboardModifiers(AZ::u32 keyModifiers) { m_keyModifiers = keyModifiers; }

            bool Alt() const { return (m_keyModifiers & static_cast<AZ::u32>(KeyboardModifier::Alt)) != 0; }
            bool Shift() const { return (m_keyModifiers & static_cast<AZ::u32>(KeyboardModifier::Shift)) != 0; }
            bool Ctrl() const { return (m_keyModifiers & static_cast<AZ::u32>(KeyboardModifier::Ctrl)) != 0; }
            bool None() const { return m_keyModifiers == static_cast<AZ::u32>(KeyboardModifier::None); }

            bool operator==(const KeyboardModifiers& keyboardModifiers) const 
            { 
                return m_keyModifiers == keyboardModifiers.m_keyModifiers; 
            }

            bool operator!=(const KeyboardModifiers& keyboardModifiers) const
            {
                return m_keyModifiers != keyboardModifiers.m_keyModifiers;
            }
            
            AZ::u32 m_keyModifiers = 0;
        };

        enum class MouseButton : AZ::u32
        {
            None = 0,
            Left = 0x01,
            Middle = 0x02,
            Right = 0x04
        };

        struct MouseButtons
        {
            MouseButtons() = default;
            explicit MouseButtons(AZ::u32 mouseButtons) { m_mouseButtons = mouseButtons; }

            bool Left() const { return (m_mouseButtons & static_cast<AZ::u32>(MouseButton::Left)) != 0; }
            bool Middle() const { return (m_mouseButtons & static_cast<AZ::u32>(MouseButton::Middle)) != 0; }
            bool Right() const { return (m_mouseButtons & static_cast<AZ::u32>(MouseButton::Right)) != 0; }
            bool None() const { return m_mouseButtons == static_cast<AZ::u32>(MouseButton::None); }
            bool Any() const { return m_mouseButtons != static_cast<AZ::u32>(MouseButton::None); }

            AZ::u32 m_mouseButtons = 0;
        };

        struct InteractionId
        {
            InteractionId() {}
            InteractionId(AZ::EntityId cameraId, int viewportId)
                : m_cameraId(cameraId), m_viewportId(viewportId) {}

            AZ::EntityId m_cameraId;
            int m_viewportId = 0;
        };

        struct MousePick
        {
            AZ::Vector3 m_rayOrigin = AZ::Vector3::CreateZero(); ///< world space
            AZ::Vector3 m_rayDirection = AZ::Vector3::CreateZero(); ///< world space, unit length
            AZ::Vector2 m_screenCoordinates = AZ::Vector2::CreateZero();
        };

        struct MouseInteraction
        {
            MousePick m_mousePick;
            MouseButtons m_mouseButtons;
            InteractionId m_interactionId;
            KeyboardModifiers m_keyboardModifiers;
        };

        struct KeyboardInteraction
        {
            int m_keyCode = 0;
            int AsAscii() const { return m_keyCode; }

            InteractionId m_interactionId;
            KeyboardModifiers m_keyboardModifiers;
        };
    }

    class ViewportInteractionRequests : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = int;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual ~ViewportInteractionRequests() = default;

        virtual ViewportInteraction::CameraState GetCameraState() = 0;
        virtual bool GridSnappingEnabled() = 0;
        virtual float GridSize() = 0;
        virtual AZ::Vector3 PickSurface(const AZ::Vector2& point) = 0;
        virtual float TerrainHeight(const AZ::Vector2& position) = 0;
    };
    using ViewportInteractionRequestBus = AZ::EBus<ViewportInteractionRequests>;
}
