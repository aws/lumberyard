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

#include "ViewportScreen.h"

#include <AzCore/Math/VectorConversions.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector4.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ScreenGeometry.h>

namespace AzFramework
{
    static AZ::Matrix4x4 ConvertFromZup()
    {
        // note: the below matrix is the result of these combined transformations
        //      pitch = AZ::Matrix4x4::CreateRotationX(AZ::DegToRad(-90.0f));
        //      yaw = AZ::Matrix4x4::CreateRotationZ(AZ::DegToRad(180.0f));
        //      conversion = pitchAdjust * yawAdjust

        return AZ::Matrix4x4::CreateFromColumns(
            AZ::Vector4(-1.0f, 0.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 0.0f, 1.0f, .0f),
            AZ::Vector4(0.0f, 1.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    AZ::Matrix4x4 CameraTransform(const CameraState& cameraState)
    {
        return AZ::Matrix4x4::CreateFromColumns(
            AZ::Vector3ToVector4(cameraState.m_side), AZ::Vector3ToVector4(cameraState.m_forward),
            AZ::Vector3ToVector4(cameraState.m_up), AZ::Vector3ToVector4(cameraState.m_position, 1.0f));
    }

    static float Aspect(const AZ::Vector2& viewportSize)
    {
        return viewportSize.GetX() / viewportSize.GetY();
    }

    AZ::Matrix4x4 CameraView(const CameraState& cameraState)
    {
        // ensure the camera is looking down positive z with the x axis pointing left
        return ConvertFromZup() * CameraTransform(cameraState).GetInverseFull();
    }

    AZ::Matrix4x4 InverseCameraView(const CameraState& cameraState)
    {
        // ensure the camera is looking down positive z with the x axis pointing left
        return CameraView(cameraState).GetInverseFull();
    }

    AZ::Matrix4x4 CameraProjection(const CameraState& cameraState)
    {
        return AZ::Matrix4x4::CreateProjection(
            cameraState.VerticalFovRadian(), Aspect(cameraState.m_viewportSize), cameraState.m_nearClip,
            cameraState.m_farClip);
    }

    AZ::Matrix4x4 InverseCameraProjection(const CameraState& cameraState)
    {
        return CameraProjection(cameraState).GetInverseFull();
    }

    ScreenPoint WorldToScreen(
        const AZ::Vector3& worldPosition, const AZ::Matrix4x4& cameraView, const AZ::Matrix4x4& cameraProjection,
        const AZ::Vector2& viewportSize)
    {
        // transform the world space position to clip space
        const auto clipSpacePosition =
            cameraProjection * cameraView * AZ::Vector3ToVector4(worldPosition, 1.0f);
        // transform the clip space position to ndc space (perspective divide)
        const auto ndcPosition = clipSpacePosition / clipSpacePosition.GetW();
        // transform ndc space from <-1,1> to <0, 1> range
        const auto ndcNormalizedPosition = (AZ::Vector4ToVector2(ndcPosition) + AZ::Vector2::CreateOne()) * 0.5f;
        // scale ndc position by screen dimensions to return screen position
        return ScreenPoint(
            aznumeric_caster(std::round(ndcNormalizedPosition.GetX() * viewportSize.GetX())),
            aznumeric_caster(std::round(viewportSize.GetY() - (ndcNormalizedPosition.GetY() * viewportSize.GetY()))));
    }

    ScreenPoint WorldToScreen(const AZ::Vector3& worldPosition, const CameraState& cameraState)
    {
        return WorldToScreen(
            worldPosition, CameraView(cameraState), CameraProjection(cameraState), cameraState.m_viewportSize);
    }

    AZ::Vector3 ScreenToWorld(
        const ScreenPoint& screenPosition, const AZ::Matrix4x4& inverseCameraView,
        const AZ::Matrix4x4& inverseCameraProjection, const AZ::Vector2& viewportSize)
    {
        const auto screenHeight = viewportSize.GetY();

        // convert screen space coordinates to <-1,1> range
        const auto ndcPosition =
            (AZ::Vector2(aznumeric_caster(screenPosition.m_x), aznumeric_caster(screenHeight - screenPosition.m_y)) /
                viewportSize) * 2.0f - AZ::Vector2::CreateOne();

        // transform ndc space position to clip space
        const auto clipSpacePosition = inverseCameraProjection * Vector2ToVector4(ndcPosition, -1.0f, 1.0f);
        // transform clip space to camera space
        const auto cameraSpacePosition = AZ::Vector4ToVector3(clipSpacePosition) / clipSpacePosition.GetW();
        // transform camera space to world space
        const auto worldPosition = inverseCameraView * cameraSpacePosition;

        return worldPosition;
    }

    AZ::Vector3 ScreenToWorld(
        const ScreenPoint& screenPosition, const CameraState& cameraState)
    {
        return ScreenToWorld(
            screenPosition, InverseCameraView(cameraState), InverseCameraProjection(cameraState),
            cameraState.m_viewportSize);
    }
} // namespace AzFramework
