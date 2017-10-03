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
            float m_nearClip = 0.01f;
            float m_farClip = 100.0f;
            bool m_valid = false;
            bool m_isOrthographic = false;
            AZ::Vector2 m_viewportSize = AZ::Vector2::CreateZero();
        };

        enum Modifiers
        {
            MI_NONE = 0,
            MI_LEFT_BUTTON = 0x01,
            MI_MIDDLE_BUTTON = 0x02,
            MI_RIGHT_BUTTON = 0x04,
            MI_ALT = 0x08,
            MI_SHIFT = 0x10,
            MI_CTRL = 0x20
        };

        struct MouseInteraction
        {
            AZ::EntityId m_cameraID = AZ::EntityId();
            int m_viewportID = 0;

            AZ::Vector3 m_rayOrigin = AZ::Vector3::CreateZero(); ///< in world space
            AZ::Vector3 m_rayDirection = AZ::Vector3::CreateZero(); ///< in world space, unit length

            AZ::Vector2 m_screenCoordinates = AZ::Vector2::CreateZero();
            int m_keyModifiers = 0;
            float m_interactionData = 0.0f;

            int IsLeftButton() const { return m_keyModifiers & MI_LEFT_BUTTON; }
            int IsMiddleButton() const { return m_keyModifiers & MI_MIDDLE_BUTTON; }
            int IsRightButton() const { return m_keyModifiers & MI_RIGHT_BUTTON; }

            bool IsAlt() const { return (m_keyModifiers & MI_ALT) != 0; }
            bool IsShift() const { return (m_keyModifiers & MI_SHIFT) != 0; }
            bool IsCtrl() const { return (m_keyModifiers & MI_CTRL) != 0; }
        };


        struct KeyboardInteraction
        {
            AZ::EntityId m_cameraID;
            AZ::u32 m_viewportID;
            int m_keyCode;
            int m_keyModifiers;
            float m_interactionData;

            int AsAscii() const { return m_keyCode; }

            bool IsAlt() const { return (m_keyModifiers & MI_ALT) != 0; }
            bool IsShift() const { return (m_keyModifiers & MI_SHIFT) != 0; }
            bool IsCtrl() const { return (m_keyModifiers & MI_CTRL) != 0; }
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
    };
    using ViewportInteractionRequestBus = AZ::EBus<ViewportInteractionRequests>;
}

