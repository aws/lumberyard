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

#include "Camera.h"


namespace MCommon
{
    // constructor
    Camera::Camera()
    {
        Reset();
        mPosition       = MCore::Vector3(0.0f, 0.0f, 0.0f);
        mScreenWidth    = 0;
        mScreenHeight   = 0;
        mProjectionMode = PROJMODE_PERSPECTIVE;
    }


    // destructor
    Camera::~Camera()
    {
    }


    // update the camera
    void Camera::Update(float timeDelta)
    {
        MCORE_UNUSED(timeDelta);

        // setup projection matrix
        switch (mProjectionMode)
        {
        // initialize for perspective projection
        case PROJMODE_PERSPECTIVE:
        {
            mProjectionMatrix.PerspectiveRH(MCore::Math::DegreesToRadians(mFOV), mAspect, mNearClipDistance, mFarClipDistance);
            break;
        }

        // initialize for orthographic projection
        case PROJMODE_ORTHOGRAPHIC:
        {
            const float halfX = mOrthoClipDimensions.GetX() * 0.5f;
            const float halfY = mOrthoClipDimensions.GetY() * 0.5f;
            mProjectionMatrix.OrthoOffCenterRH(-halfX, halfX, halfY, -halfY, -mFarClipDistance, mFarClipDistance);
            break;
        }
        }

        // calculate the viewproj matrix
        mViewProjMatrix = mViewMatrix * mProjectionMatrix;

        // init the view frustum
        mFrustum.InitFromMatrix(mViewProjMatrix);
    }


    // reset the base camera attributes
    void Camera::Reset(float flightTime)
    {
        MCORE_UNUSED(flightTime);

        mFOV                = 55.0f;
        mNearClipDistance   = 0.1f;
        mFarClipDistance    = 200.0f;
        mAspect             = 16.0f / 9.0f;
        mRotationSpeed      = 0.5f;
        mTranslationSpeed   = 1.0f;
        mViewMatrix.Identity();
    }


    // unproject screen coordinates to a ray
    MCore::Ray Camera::Unproject(int32 screenX, int32 screenY)
    {
        MCore::Matrix invProj = mProjectionMatrix.Inversed();
        MCore::Matrix invView = mViewMatrix.Inversed();

        MCore::Vector3  start   = MCore::Unproject(static_cast<float>(screenX), static_cast<float>(screenY), static_cast<float>(mScreenWidth), static_cast<float>(mScreenHeight), mNearClipDistance, invProj, invView);
        MCore::Vector3  end     = MCore::Unproject(static_cast<float>(screenX), static_cast<float>(screenY), static_cast<float>(mScreenWidth), static_cast<float>(mScreenHeight), mFarClipDistance, invProj, invView);

        //LogInfo("start=(%.2f, %.2f, %.2f) end=(%.2f, %.2f, %.2f)", start.x, start.y, start.z, end.x, end.y, end.z);

        return MCore::Ray(start, end);
    }
} // namespace MCommon
