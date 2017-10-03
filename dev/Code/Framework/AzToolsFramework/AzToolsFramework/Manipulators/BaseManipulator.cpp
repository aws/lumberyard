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

#include "BaseManipulator.h"
#include "ManipulatorManager.h"

#include <AzCore/Math/IntersectSegment.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    const AZ::Color BaseManipulator::s_defaultMouseOverColor = AZ::Color(0.0f, 1.0f, 1.0f, 1.0f);

    BaseManipulator::BaseManipulator(AZ::EntityId entityId)
        : m_manipulatorId(InvalidManipulatorId)
        , m_manipulatorManagerId(InvalidManipulatorManagerId)
        , m_entityId(entityId)
        , m_isPerformingOperation(false)
        , m_isBoundsDirty(true)
        , m_isMouseOver(false)
    {
    }

    BaseManipulator::~BaseManipulator()
    {
        if (IsRegistered())
        {
            Unregister();
        }
    }

    void BaseManipulator::OnMouseOver(ManipulatorId manipulatorId)
    {
        if (m_manipulatorId == manipulatorId)
        {
            m_isMouseOver = true;
        }   
        else
        {
            m_isMouseOver = false;
        }   
    }

    void BaseManipulator::Register(ManipulatorManagerId managerId)
    {
        if (IsRegistered())
        {
            Unregister();
        }

        AzToolsFramework::ManipulatorManagerRequestBus::Event(managerId,
            &AzToolsFramework::ManipulatorManagerRequestBus::Events::RegisterManipulator, *this);
    }

    void BaseManipulator::Unregister()
    {
        // if the manipulator has already been unregistered, the m_manipulatorManagerId should be invalid, which makes the call below a no-op.
        AzToolsFramework::ManipulatorManagerRequestBus::Event(m_manipulatorManagerId,
            &AzToolsFramework::ManipulatorManagerRequestBus::Events::UnregisterManipulator, *this);
    }

    void BaseManipulator::Invalidate()
    {
        m_manipulatorId = InvalidManipulatorId;
        m_manipulatorManagerId = InvalidManipulatorManagerId;
        SetBoundsDirty();
    }

    namespace Internal
    {
        float CalcScreenToWorldMultiplier(float distanceToCamera, const ViewportInteraction::CameraState& cameraState)
        {
            float result = 1.0f;

            float viewportHeight = cameraState.m_viewportSize.GetY();
            float nearPlaneHalfHeight = cameraState.m_nearClip * tanf(cameraState.m_verticalFovRadian * 0.5f);
            float nearPlaneOverViewportHeight = nearPlaneHalfHeight / (viewportHeight * 0.5f);

            if (cameraState.m_isOrthographic)
            {
                result = nearPlaneOverViewportHeight / cameraState.m_zoom;
            }
            else
            {
                float cameraDistOverNearClip = distanceToCamera / cameraState.m_nearClip;
                result = nearPlaneOverViewportHeight * cameraDistOverNearClip;
            }

            return result;
        }

        void CalcRayPlaneIntersectingPoint(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float maxRayLength,
            const AZ::Vector3& pointOnPlane, const AZ::Vector3& planeNormal, AZ::Vector3& resultIntersectingPoint)
        {
            float t = 0.0f;
            int hit = AZ::Intersect::IntersectRayPlane(rayOrigin, rayDirection, pointOnPlane, planeNormal, t);
            if (hit > 0)
            {
                t = AZ::GetMin(t, maxRayLength);
                resultIntersectingPoint = rayOrigin + t * rayDirection;
            }
        }

        void TransformDirectionBasedOnReferenceSpace(ManipulatorReferenceSpace referenceSpace, const AZ::Transform& entityWorldTM, const AZ::Vector3& direction, AZ::Vector3& resultDirection)
        {
            switch (referenceSpace)
            {
                case AzToolsFramework::ManipulatorReferenceSpace::Local:
                {
                    resultDirection = entityWorldTM * direction - entityWorldTM.GetPosition();
                    resultDirection.NormalizeExact();
                } break;

                case AzToolsFramework::ManipulatorReferenceSpace::World:
                {
                    resultDirection = direction;
                    // resultDirection.NormalizeSafeApprox();
                } break;

                default:
                {
                    // default to local space
                    resultDirection = entityWorldTM * direction - entityWorldTM.GetPosition();
                    resultDirection.NormalizeExact();
                } break;
            }
        }
    }
}