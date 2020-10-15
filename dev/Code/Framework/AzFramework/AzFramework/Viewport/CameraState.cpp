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

#include "CameraState.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Transform.h>

namespace AzFramework
{
    void SetCameraClippingVolume(
        AzFramework::CameraState& cameraState, const float nearPlane, const float farPlane, const float fovRad)
    {
        cameraState.m_nearClip = nearPlane;
        cameraState.m_farClip = farPlane;
        cameraState.m_fovOrZoom = fovRad;
    }

    static void SetDefaultCameraClippingVolume(AzFramework::CameraState& cameraState)
    {
        SetCameraClippingVolume(cameraState, 0.1f, 1000.0f, AZ::DegToRad(60.0f));
    }

    AzFramework::CameraState CreateDefaultCamera(
         const AZ::Transform& transform, const AZ::Vector2& viewportSize)
    {
        AzFramework::CameraState cameraState;

        SetDefaultCameraClippingVolume(cameraState);

        cameraState.m_side = transform.GetBasisX();
        cameraState.m_forward = transform.GetBasisY();
        cameraState.m_up = transform.GetBasisZ();
        cameraState.m_position = transform.GetTranslation();
        cameraState.m_viewportSize = viewportSize;

        return cameraState;
    }

    AzFramework::CameraState CreateIdentityDefaultCamera(
        const AZ::Vector3& position, const AZ::Vector2& viewportSize)
    {
        return CreateDefaultCamera(AZ::Transform::CreateTranslation(position), viewportSize);
    }

    void CameraState::Reflect(AZ::SerializeContext& serializeContext)
    {
        serializeContext.Class<CameraState>()->
            Field("Position", &CameraState::m_position)->
            Field("Forward", &CameraState::m_forward)->
            Field("Side", &CameraState::m_side)->
            Field("Up", &CameraState::m_up)->
            Field("ViewportSize", &CameraState::m_viewportSize)->
            Field("NearClip", &CameraState::m_nearClip)->
            Field("FarClip", &CameraState::m_farClip)->
            Field("FovZoom", &CameraState::m_fovOrZoom)->
            Field("Ortho", &CameraState::m_orthographic);
    }
} // namespace AzFramework
