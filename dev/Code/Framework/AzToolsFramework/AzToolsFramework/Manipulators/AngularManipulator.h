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

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/Picking/ContextBoundAPI.h>

#include "BaseManipulator.h"

namespace AzToolsFramework
{
    /**
    * AngularManipulator serves as a visual tool for users to change a component's property based on rotation
    * around an axis. The rotation angle increases if the rotation goes counter clock-wise when looking
    * in the opposite direction the rotation axis points to.
    */
    class AngularManipulator
        : public BaseManipulator
    {
    public:
        AZ_RTTI(AngularManipulator, "{01CB40F9-4537-4187-A8A6-1A12356D3FD1}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(AngularManipulator, AZ::SystemAllocator, 0);

        explicit AngularManipulator(AZ::EntityId entityId);
        ~AngularManipulator();

        /**
        * This is the function signature of callbacks that will be invoked whenever a manipulator
        * is being clicked on or dragged.
        */
        using MouseActionCallback = AZStd::function<void(const AngularManipulationData&)>;

        void InstallMouseDownCallback(MouseActionCallback onMouseDownCallback);
        void InstallMouseMoveCallback(MouseActionCallback onMouseDownCallback);
        void InstallMouseUpCallback(MouseActionCallback onMouseDownCallback);

        void SetBoundsDirty() override;

        void OnMouseDown(const ViewportInteraction::MouseInteraction& interaction, float t) override;
        void OnMouseMove(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseUp(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseOver(ManipulatorId manipulatorId)  override;

        void Draw(AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState) override;

        void SetPosition(const AZ::Vector3& pos) { m_centerPosition = pos; }
        void SetRotationAxis(const AZ::Vector3& axis);
        void SetRadius(float radius) { m_radius = radius; }
        void SetColor(const AZ::Color& color) { m_color = color; }
        void SetMouseOverColor(const AZ::Color& color) { m_mouseOverColor = color; }

    protected:

        void Invalidate() override;

    private:

        void CalcManipulationData(const AZ::Vector2& mouseScreenPosition);
        AngularManipulationData GetManipulationData();

    private:

        /* Shape and Color */

        // in entity's local space
        AZ::Vector3 m_centerPosition = AZ::Vector3::CreateZero();
        AZ::Vector3 m_rotationAxis = AZ::Vector3(1.0f, 0.0f, 0.0f);
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        AZ::Color m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;
        bool m_isSizeFixedInScreen = true;
        float m_radius = 80.0f;
        float m_screenToWorldMultiplier = 1.0f; ///< The closer the manipulator is shown in the camera, the smaller this number is.

        /* Hit Detection */

        Picking::RegisteredBoundId m_torusBoundId = Picking::InvalidBoundId;

        /* Mouse Actions */

        bool m_isRotationAxisWorldTowardsCamera = true;
        float m_lastTotalScreenRotationDelta = 0.0f;
        float m_totalScreenRotationDelta = 0.0f;
        float m_lastTotalWorldRotationDelta = 0.0f;
        float m_totalWorldRotationDelta = 0.0f;
        AZ::Vector2 m_rotationCenterScreenPosition = AZ::Vector2::CreateZero();
        AZ::Vector2 m_startMouseScreenPosition = AZ::Vector2::CreateZero();
        AZ::Vector2 m_lastMouseScreenPosition = AZ::Vector2::CreateZero();
        AZ::Vector2 m_currentMouseScreenPosition = AZ::Vector2::CreateZero();
        AZ::Vector2 m_startRadiusVector = AZ::Vector2::CreateOne(); ///< in screen space, from rotation center to current mouse pointer

        AZ::Vector3 m_centerPositionWorld = AZ::Vector3::CreateZero();
        AZ::Vector3 m_rotationAxisWorld = AZ::Vector3::CreateZero();
        AZ::Vector3 m_startRadiusVectorWorld = AZ::Vector3::CreateZero();
        AZ::Vector3 m_startRadiusSideVectorWorld = AZ::Vector3::CreateZero();

        MouseActionCallback m_onMouseDownCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;
        MouseActionCallback m_onMouseUpCallback = nullptr;
    };
}