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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/array.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>

namespace AzFramework
{
    class EntityDebugDisplayRequests;
}

namespace AzToolsFramework
{
    namespace ViewportInteraction
    {
        struct CameraState;
        struct MouseInteraction;
    }

    struct LinearManipulationData
    {
        AZ::Vector3 m_localManipulatorDirection;
        AZ::Vector3 m_manipulationWorldDirection; ///< The manipulator direction in viewport world space
        float m_totalTranslationWorldDelta; ///< The total translation delta along the manipulator direction since mouse down event.
        float m_screenToWorldMultiplier; ///< This is a scaling number that converts values in screen space to world space based on the viewing camera status and it's distance to the entity being manipulated.
        ManipulatorId m_manipulatorId;
    };

    struct PlanarManipulationData
    {
        AZ::Vector3 m_startHitWorldPosition; ///< The intersection point in world space between the ray and the manipulator when the mouse down event happens.
        AZ::Vector3 m_currentHitWorldPosition; ///< The hit intersection point in world space between the ray and the manipulator when mouse move event 
        AZ::Vector3 m_manipulationWorldDirection1; ///< The manipulator direction in viewport world space
        AZ::Vector3 m_manipulationWorldDirection2; ///< The manipulator direction in viewport world space
        AZStd::array<float, 2> m_axesDelta; ///< total translation delta along the two manipulator axes since mouse down event
        ManipulatorId m_manipulatorId;
    };

    struct AngularManipulationData
    {
        AZ::Vector3 m_localRotationAxis;
        AZ::Vector2 m_startMouseScreenPosition;
        AZ::Vector2 m_lastMouseScreenPosition;
        AZ::Vector2 m_currentMouseScreenPosition;
        float m_lastTotalScreenRotationDelta; ///< The angle is in radian.
        float m_totalScreenRotationDelta; ///< Shows how much the mouse has rotated around a center computed when MouseDown event happened. This value increases if the rotation goes counter clock-wise.
        float m_lastTotalWorldRotationDelta; ///< The angle is in radian.
        float m_totalWorldRotationDelta; ///< This value is the same as \ref m_totalScreenRotationDelta, except when the rotation axis is shooting away from the viewing camera, in which case this value is 2Pi - \ref m_totalScreenRotationDelta.
        ManipulatorId m_manipulatorId;
    };

    /**
     * The base class for manipulators, providing interfaces for users of manipulators to talk to.
     */
    class BaseManipulator
    {
    public:
        AZ_RTTI(BaseManipulator, "{3D1CD58D-C589-464C-BC9A-480D59341AB4}");

        explicit BaseManipulator(AZ::EntityId entityId);
        virtual ~BaseManipulator();

        /**
         * Callback for the event when the mouse pointer is over this manipulator and a mouse button is pressed . 
         * Specific manipulators should provide their own overrides to handle this event.
         * @param interaction It contains various mouse states when the event happens, as well as a ray shooting from the viewing camera through the mouse pointer.
         * @param t The parameter value in the ray's explicit equation that represents the intersecting point on the target manipulator in world space.
         */
        virtual void OnMouseDown(const ViewportInteraction::MouseInteraction& /*interaction*/, float /*t*/) {};

        /**
         * Callback for the event when this manipulator is active and the mouse is moved.
         * Specific manipulators should provide their own overrides to handle this event.
         * @param interaction It contains various mouse states when the event happens, as well as a ray shooting from the viewing camera through the mouse pointer.
         */
        virtual void OnMouseMove(const ViewportInteraction::MouseInteraction& /*interaction*/) {};

        /**
         * Callback for the event when this manipulator is active and the mouse wheel is scrolled.
         * Specific manipulators should provide their own overrides to handle this event.
         * @param interaction It contains various mouse states when the event happens, as well as a ray shooting from the viewing camera through the mouse pointer.
         */
        virtual void OnMouseWheel(const ViewportInteraction::MouseInteraction& /*interaction*/) {};

        /**
         * Callback for the event when this manipulator is active and a mouse button is released.
         * Specific manipulators should provide their own overrides to handle this event.
         * @param interaction It contains various mouse states when the event happens, as well as a ray shooting from the viewing camera through the mouse pointer.
         */
        virtual void OnMouseUp(const ViewportInteraction::MouseInteraction& /*interaction*/) {};

        /**
         * This function changes the state indicating whether the manipulator is under the mouse pointer.
         * It is called in the event of OnMouseMove and OnMouseWheel only when there is no manipulator currently performing actions.
         * Any override of this function has to call this inside so that the /ref m_isMouseOver state is properly managed.
         */
        virtual void OnMouseOver(ManipulatorId manipulatorId);

        virtual void Draw(AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState) = 0;

        
        virtual void SetBoundsDirty() { m_isBoundsDirty = true; }

        /**
         * Register itself to a manipulator manager so that it can receive various mouse events and perform manipulations.
         * @param managerId The id identifying a unique manipulator manager.
         */
        void Register(ManipulatorManagerId managerId);

        /**
         * Unregister itself from the manipulator manager it was registered with.
         */
        void Unregister();

        bool IsRegistered() const { return (m_manipulatorId != InvalidManipulatorId && m_manipulatorManagerId != InvalidManipulatorManagerId); }

        bool IsPerformingOperation() const { return m_isPerformingOperation; }

        ManipulatorId GetManipulatorId() const { return m_manipulatorId; }
        ManipulatorManagerId GetManipulatorManagerId() const { return m_manipulatorManagerId; }

    protected:

        // This is called only when unregistering. Users of the manipulators should not call it separately.
        virtual void Invalidate();

    private:
        friend class ManipulatorManager;

        void SetManipulatorId(ManipulatorId id) { m_manipulatorId = id; }
        void SetManipulatorManagerId(ManipulatorManagerId id) { m_manipulatorManagerId = id; }

    protected:

        AZ::EntityId m_entityId;
        ManipulatorId m_manipulatorId;
        ManipulatorManagerId m_manipulatorManagerId;
        
        bool m_isBoundsDirty;
        bool m_isPerformingOperation; // after mouse down and before mouse up
        bool m_isMouseOver;

        static const AZ::Color s_defaultMouseOverColor;
    };

    namespace Internal
    {
        /**
         * This is a helper function that calculates the multiplier that can be used to convert a manipulator's size in screen space to world space.
         * @param distanceToCamera  The distance from the manipulator to the camera along the camera's forward direction in world space.
         * @param cameraState       This contains various states of the camera.
         * @return                  The value of the multiplier.
         */
        float CalcScreenToWorldMultiplier(float distanceToCamera, const ViewportInteraction::CameraState& cameraState);

        /**
         * This helper function calculates the intersecting point between a ray and a plane.
         * @param rayOrigin                     The origin of the ray to test.
         * @param rayDirection                  The direction of the ray to test.
         * @param maxRayLength
         * @param pointOnPlane                  A point on the plane.
         * @param planeNormal                   The normal vector of the plane.
         * @param[out] resultIntersectingPoint  This stores the result intersecting point. It will be left unchanged if there 
         *                                      is no intersection between the ray and the plane.
         */
        void CalcRayPlaneIntersectingPoint(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float maxRayLength,
            const AZ::Vector3& pointOnPlane, const AZ::Vector3& planeNormal, AZ::Vector3& resultIntersectingPoint);

        /**
         * This helper function transforms a manipulator's direction values to world space based on manipulator's reference space to be properly displayed.
         */
        void TransformDirectionBasedOnReferenceSpace(ManipulatorReferenceSpace referenceSpace, const AZ::Transform& entityWorldTM, const AZ::Vector3& direction, AZ::Vector3& resultDirection);
    }
}