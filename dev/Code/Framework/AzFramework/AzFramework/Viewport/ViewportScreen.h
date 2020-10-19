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

namespace AZ
{
    class Matrix4x4;
    class Vector2;
    class Vector3;
} // namespace AZ

namespace AzFramework
{
    struct CameraState;
    struct ScreenPoint;
    struct ViewportInfo;

    //! Project a position in world space to screen space for the given camera.
    ScreenPoint WorldToScreen(const AZ::Vector3& worldPosition, const CameraState& cameraState);

    //! Overload of WorldToScreen that accepts camera values that can be precomputed if this function
    //! is called for many points in a loop.
    ScreenPoint WorldToScreen(
        const AZ::Vector3& worldPosition, const AZ::Matrix4x4& cameraView, const AZ::Matrix4x4& cameraProjection,
        const AZ::Vector2& viewportSize);

    //! Unproject a position in screen space to world space.
    //! Note: The position returned will be on the near clip plane of the camera in world space.
    AZ::Vector3 ScreenToWorld(const ScreenPoint& screenPosition, const CameraState& cameraState);

    //! Overload of ScreenToWorld that accepts camera values that can be precomputed if this function
    //! is called for many points in a loop.
    AZ::Vector3 ScreenToWorld(
        const ScreenPoint& screenPosition, const AZ::Matrix4x4& inverseCameraView,
        const AZ::Matrix4x4& inverseCameraProjection, const AZ::Vector2& viewportSize);

    //! Return the camera projection for the current camera state.
    AZ::Matrix4x4 CameraProjection(const CameraState& cameraState);

    //! Return the inverse of the camera projection for the current camera state.
    AZ::Matrix4x4 InverseCameraProjection(const CameraState& cameraState);

    //! Return the camera view for the current camera state.
    AZ::Matrix4x4 CameraView(const CameraState& cameraState);

    //! Return the inverse of the camera view for the current camera state.
    //! @note This is the same as the CameraTransform but corrected for Z up.
    AZ::Matrix4x4 InverseCameraView(const CameraState& cameraState);

    //! Return the camera transform for the current camera state.
    AZ::Matrix4x4 CameraTransform(const CameraState& cameraState);
} // namespace AzFramework
