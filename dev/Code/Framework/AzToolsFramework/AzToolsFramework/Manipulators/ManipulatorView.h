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
#include <AzToolsFramework/Manipulators/BaseManipulator.h>
#include <AzToolsFramework/Picking/ContextBoundAPI.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    class PlanarManipulator;
    class LinearManipulator;
    class AngularManipulator;
    class LineSegmentSelectionManipulator;
    class SplineSelectionManipulator;

    using DecideColorFn = AZStd::function<AZ::Color(
        const ViewportInteraction::MouseInteraction&,
        bool mouseOver, const AZ::Color& defaultColor)>;

    extern const float g_defaultManipulatorSphereRadius;

    class ManipulatorView
    {
    public:
        ManipulatorView();
        virtual ~ManipulatorView();

        void SetBoundDirty(ManipulatorManagerId managerId);
        void RefreshBound(ManipulatorManagerId managerId,
            ManipulatorId manipulatorId, const Picking::BoundRequestShapeBase& shape);
        void Invalidate(ManipulatorManagerId managerId);

        virtual void Draw(
            ManipulatorManagerId managerId, const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId, const ManipulatorState& manipulatorState,
            AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace manipulatorSpace) = 0;

        AZ::Color m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;
        Picking::RegisteredBoundId m_boundId = Picking::InvalidBoundId; ///< Used for hit detection.
        bool m_screenSizeFixed = true; ///< Should manipulator size be adjusted based on camera distance.
        bool m_boundDirty = true; ///< Do the bounds need to be recalculated.
    };

    using ManipulatorViews = AZStd::vector<AZStd::unique_ptr<ManipulatorView>>;

    class ManipulatorViewQuad
        : public ManipulatorView
    {
    public:
        void Draw(
            ManipulatorManagerId managerId, const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId, const ManipulatorState& manipulatorState,
            AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace manipulatorSpace) override;

        AZ::Vector3 m_axis1 = AZ::Vector3(1.0f, 0.0f, 0.0f);
        AZ::Vector3 m_axis2 = AZ::Vector3(0.0f, 1.0f, 0.0f);
        AZ::Color m_axis1Color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        AZ::Color m_axis2Color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        float m_size = 0.06f; ///< size to render and do mouse ray intersection tests against.

    private:
        AZ::Vector3 m_cameraCorrectedAxis1;
        AZ::Vector3 m_cameraCorrectedAxis2;
    };

    class ManipulatorViewQuadBillboard
        : public ManipulatorView
    {
    public:
        void Draw(
            ManipulatorManagerId managerId, const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId, const ManipulatorState& manipulatorState,
            AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace manipulatorSpace) override;

        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        float m_size = 0.005f; ///< size to render and do mouse ray intersection tests against.
    };

    class ManipulatorViewLine
        : public ManipulatorView
    {
    public:
        void Draw(
            ManipulatorManagerId managerId, const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId, const ManipulatorState& manipulatorState,
            AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace manipulatorSpace) override;

        AZ::Vector3 m_axis;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        float m_length;
        float m_width;

    private:
        AZ::Vector3 m_cameraCorrectedAxis;
    };

    class ManipulatorViewLineSelect
        : public ManipulatorView
    {
    public:
        void Draw(
            ManipulatorManagerId managerId, const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId, const ManipulatorState& manipulatorState,
            AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace manipulatorSpace) override;

        AZ::Vector3 m_localStart;
        AZ::Vector3 m_localEnd;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        float m_width;
    };

    class ManipulatorViewCone
        : public ManipulatorView
    {
    public:
        void Draw(
            ManipulatorManagerId managerId, const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId, const ManipulatorState& manipulatorState,
            AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace manipulatorSpace) override;

        AZ::Vector3 m_offset;
        AZ::Vector3 m_axis;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        float m_length;
        float m_radius;

    private:
        AZ::Vector3 m_cameraCorrectedAxis;
        AZ::Vector3 m_cameraCorrectedOffset;
    };

    class ManipulatorViewBox
        : public ManipulatorView
    {
    public:
        void Draw(
            ManipulatorManagerId managerId, const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId, const ManipulatorState& manipulatorState,
            AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace manipulatorSpace) override;

        AZ::Vector3 m_offset;
        AZ::Quaternion m_orientation;
        AZ::Vector3 m_halfExtents;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);

    private:
        AZ::Vector3 m_cameraCorrectedOffset;
    };

    class ManipulatorViewCylinder
        : public ManipulatorView
    {
    public:
        void Draw(
            ManipulatorManagerId managerId, const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId, const ManipulatorState& manipulatorState,
            AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace manipulatorSpace) override;

        AZ::Vector3 m_axis;
        float m_length;
        float m_radius;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);

    private:
        AZ::Vector3 m_cameraCorrectedAxis;
    };

    class ManipulatorViewSphere
        : public ManipulatorView
    {
    public:
        void Draw(
            ManipulatorManagerId managerId, const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId, const ManipulatorState& manipulatorState,
            AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace manipulatorSpace) override;

        float m_radius;
        DecideColorFn m_decideColorFn;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
    };

    class ManipulatorViewCircle
        : public ManipulatorView
    {
    public:
        void Draw(
            ManipulatorManagerId managerId, const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId, const ManipulatorState& manipulatorState,
            AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace manipulatorSpace) override;

        AZ::Vector3 m_axis;
        float m_width;
        float m_radius;
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
    };

    class ManipulatorViewSplineSelect
        : public ManipulatorView
    {
    public:
        void Draw(
            ManipulatorManagerId managerId, const ManipulatorManagerState& managerState,
            ManipulatorId manipulatorId, const ManipulatorState& manipulatorState,
            AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace manipulatorSpace) override;

        AZStd::weak_ptr<const AZ::Spline> m_spline;
        float m_width;
        AZ::Color m_color = AZ::Color(0.0f, 1.0f, 0.0f, 1.0f);
    };

    /**
     * Calculate scale factor based on distance from camera
     */
    inline AZ::VectorFloat CalculateScreenToWorldMultiplier(
        bool screenSizeFixed, const AZ::Vector3& worldPosition, const ViewportInteraction::CameraState& cameraState)
    {
        // author sizes of manipulators as they would appear in perspective 10 meters from the camera.
        const AZ::VectorFloat tenRecip = AZ::VectorFloat(1.0f / 10.0f);
        return screenSizeFixed
            ? GetMax(
                cameraState.m_location.GetDistance(worldPosition),
                AZ::VectorFloat(cameraState.m_nearClip)) * tenRecip
            : AZ::VectorFloat::CreateOne();
    }

    /**
     * @brief Return the world transform of the entity with uniform scale - choose
     * the largest element.
     */
    AZ::Transform WorldFromLocalWithUniformScale(const AZ::EntityId entityId);
    /**
     * @brief Take a transform and return it with uniform scale - choose the largest element.
     */
    AZ::Transform TransformUniformScale(const AZ::Transform& transform);

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewQuad(
        const PlanarManipulator& planarManipulator, const AZ::Color& axis1Color,
        const AZ::Color& axis2Color, float size);

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewQuadBillboard(
        const AZ::Color& color, float size);

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewLine(
        const LinearManipulator& linearManipulator, const AZ::Color& color,
        float length, float width);

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewLineSelect(
        const LineSegmentSelectionManipulator& lineSegmentManipulator, const AZ::Color& color,
        float width);

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewCone(
        const LinearManipulator& linearManipulator, const AZ::Color& color,
        const AZ::Vector3& offset, float length, float radius);

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewBox(
        const AZ::Transform& transform, const AZ::Color& color,
        const AZ::Vector3& offset, const AZ::Vector3& halfExtents);

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewCylinder(
        const LinearManipulator& linearManipulator, const AZ::Color& color,
        float length, float radius);

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewSphere(
        const AZ::Color& color, float radius, DecideColorFn decideColor);

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewCircle(
        const AngularManipulator& angularManipulator, const AZ::Color& color,
        float radius, float width);

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewSplineSelect(
        const SplineSelectionManipulator& splineManipulator, const AZ::Color& color,
        float width);
}