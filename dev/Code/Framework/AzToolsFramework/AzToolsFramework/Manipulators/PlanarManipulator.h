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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Picking/ContextBoundAPI.h>

#include "BaseManipulator.h"

namespace AzToolsFramework
{
    /**
    * PlanarManipulator serves as a visual tool for users to modify values in two dimension in a plane defined two non-collinear axes in 3D space.
    */
    class PlanarManipulator
        : public BaseManipulator
    {
    public:
        AZ_RTTI(PlanarManipulator, "{2B1C2140-F3B1-4DB2-B066-156B67B57B97}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(PlanarManipulator, AZ::SystemAllocator, 0);

        explicit PlanarManipulator(AZ::EntityId entityId);
        ~PlanarManipulator() = default;

        /**
        * This is the function signature of callbacks that will be invoked whenever a manipulator
        * is being clicked on or dragged.
        */
        using MouseActionCallback = AZStd::function<void(const PlanarManipulationData&)>;

        void InstallMouseDownCallback(MouseActionCallback onMouseDownCallback);
        void InstallMouseMoveCallback(MouseActionCallback onMouseDownCallback);
        void InstallMouseUpCallback(MouseActionCallback onMouseDownCallback);

        void SetBoundsDirty() override;

        void OnMouseDown(const ViewportInteraction::MouseInteraction& interaction, float t) override;
        void OnMouseMove(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseUp(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseOver(ManipulatorId manipulatorId)  override;

        void Draw(AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState) override;

        void SetPosition(const AZ::Vector3& pos) { m_origin = pos; }

        /**
        * Make sure \ref axis1 and \ref axis2 are not collinear.
        */
        void SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2);
        void SetAxesLength(float length1, float length2);

        const AZ::Vector3& GetAxis1() const { return m_axis1; }
        const AZ::Vector3& GetAxis2() const { return m_axis2; }
        const AZ::Vector3& GetPosition() const { return m_origin; }

        void SetColor(const AZ::Color& color) { m_color = color; }
        void SetMouseOverColor(const AZ::Color& color) { m_mouseOverColor = color; }

    protected:

        void Invalidate() override;

    private:

        PlanarManipulationData GetManipulationData(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection);

        /* Shape and Color. All positions and direction should be specified in entity's local space. */

        AZ::Vector3 m_origin = AZ::Vector3::CreateZero();
        AZ::Vector3 m_axis1 = AZ::Vector3::CreateAxisX(); ///< m_direction1 and m_direction2 have to be orthogonal, they together define a plane in 3d space
        AZ::Vector3 m_axis2 = AZ::Vector3::CreateAxisY();
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 0.3f);
        AZ::Color m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;
        bool m_isSizeFixedInScreen = true;
        float m_lengthAxis1 = 40.0f; ///< lengths along m_direction1 and m_direction2 respectively
        float m_lengthAxis2 = 30.0f;
        float m_screenToWorldMultiplier = 1.0f; ///< The closer the manipulator is shown in the camera, the smaller this number is.

        /* Hit Detection */

        Picking::RegisteredBoundId m_quadBoundId = Picking::InvalidBoundId;

        /* Mouse Actions */

        float m_cameraFarClip = 1.0f;
        AZ::Vector3 m_originWorld = AZ::Vector3::CreateZero();
        AZ::Vector3 m_axis1World = AZ::Vector3::CreateZero();
        AZ::Vector3 m_axis2World = AZ::Vector3::CreateZero();
        AZ::Vector3 m_manipulatorPlaneNormalWorld = AZ::Vector3::CreateZero();

        AZ::Vector3 m_startHitWorldPosition = AZ::Vector3::CreateZero();
        AZ::Vector3 m_currentHitWorldPosition = AZ::Vector3::CreateZero();

        MouseActionCallback m_onMouseDownCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;
        MouseActionCallback m_onMouseUpCallback = nullptr;
    };

    /**
     * Return local space translation of manipulator.
     * @param manipulationData Current state of linear manipulator.
     * @param inverseTransform Inverse transform of entity manipulator exists on.
     */
    AZ_FORCE_INLINE AZ::Vector3 CalculateLocalOffsetFromOrigin(const PlanarManipulationData& manipulationData, const AZ::Transform& inverseTransform)
    {
        return Vector4ToVector3(inverseTransform
                * Vector3ToVector4(manipulationData.m_axesDelta[0] * manipulationData.m_manipulationWorldDirection1
                                 + manipulationData.m_axesDelta[1] * manipulationData.m_manipulationWorldDirection2));
    }
}