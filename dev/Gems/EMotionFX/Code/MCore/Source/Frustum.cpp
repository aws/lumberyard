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

// include required headers
#include "Frustum.h"
#include "AABB.h"
#include "LogManager.h"
#include "Matrix4.h"
#include "Algorithms.h"


namespace MCore
{
    // initialize the frustum from some given settings
    /*template <class T>
    void ViewFrustum::Init(const T& fov, const T& aspect, const T& zNear, const T& zFar, const Matrix &viewMatrix, const TVector3<T> &viewPos)
    {
        T w = 2.0 * zNear * Math::Tan(fov*0.5); // viewplane width
        T h = w * aspect;                       // viewplane height

        Matrix  invMatrix(viewMatrix); invMatrix.Transpose();
        TVector3<T> forward     = invMatrix.GetForward();
        TVector3<T> right       = invMatrix.GetRight();
        TVector3<T> up          = invMatrix.GetUp();
        TVector3<T> normal;

        // top
        normal = (-up + (w / zNear)*forward).Normalize();
        this->mPlanes[FRUSTUMPLANE_TOP] = PlaneEq(normal, -normal.Dot(viewPos));

        // bottom
        normal = (up + (w / zNear)*forward).Normalize();
        this->mPlanes[FRUSTUMPLANE_BOTTOM] = PlaneEq(normal, -normal.Dot(viewPos));

        // left
        normal = (right + (h / zNear)*forward).Normalize();
        this->mPlanes[FRUSTUMPLANE_LEFT] = PlaneEq(normal, -normal.Dot(viewPos));

        // right
        normal = (-right + (h / zNear)*forward).Normalize();
        this->mPlanes[FRUSTUMPLANE_RIGHT] = PlaneEq(normal, -normal.Dot(viewPos));

        // near plane
        this->mPlanes[FRUSTUMPLANE_NEAR] = PlaneEq(forward, -forward.Dot(viewPos));

        // far plane
        this->mPlanes[FRUSTUMPLANE_FAR] = PlaneEq(-forward, forward.Dot(viewPos+(forward*zFar)));
    }*/



    // checks if a given axis aligned boundingbox is partially (intersects) with this frustum
    bool Frustum::PartiallyContains(const AABB& box) const
    {
        const uint32 numPlanes = mPlanes.GetLength();
        for (uint32 i = 0; i < numPlanes; ++i)
        {
            if (!mPlanes[i].PartiallyAbove(box))
            {
                return false;
            }
        }

        return true;
    }


    // checks if a given axis aligned boundingbox is completely within this frustum
    bool Frustum::CompletelyContains(const AABB& box) const
    {
        const uint32 numPlanes = mPlanes.GetLength();
        for (uint32 i = 0; i < numPlanes; ++i)
        {
            if (!mPlanes[i].CompletelyAbove(box))
            {
                return false;
            }
        }

        return true;
    }


    // log some info
    void ViewFrustum::Log()
    {
        AZ::Vector3 normal;
        float distance;

        LogDetailedInfo("View Frustum:");

        normal  = this->mPlanes[FRUSTUMPLANE_LEFT].GetNormal();
        distance = this->mPlanes[FRUSTUMPLANE_LEFT].GetDist();
        LogDetailedInfo("Left Plane: Normal=(%.6f, %.6f, %.6f) Distance=%.6f", 
            static_cast<float>(normal.GetX()), 
            static_cast<float>(normal.GetY()), 
            static_cast<float>(normal.GetZ()), 
            distance);

        normal  = this->mPlanes[FRUSTUMPLANE_RIGHT].GetNormal();
        distance = this->mPlanes[FRUSTUMPLANE_RIGHT].GetDist();
        LogDetailedInfo("Right Plane: Normal=(%.6f, %.6f, %.6f) Distance=%.6f", 
            static_cast<float>(normal.GetX()), 
            static_cast<float>(normal.GetY()), 
            static_cast<float>(normal.GetZ()), 
            distance);

        normal  = this->mPlanes[FRUSTUMPLANE_TOP].GetNormal();
        distance = this->mPlanes[FRUSTUMPLANE_TOP].GetDist();
        LogDetailedInfo("Top Plane: Normal=(%.6f, %.6f, %.6f) Distance=%.6f", 
            static_cast<float>(normal.GetX()), 
            static_cast<float>(normal.GetY()), 
            static_cast<float>(normal.GetZ()), 
            distance);

        normal  = this->mPlanes[FRUSTUMPLANE_BOTTOM].GetNormal();
        distance = this->mPlanes[FRUSTUMPLANE_BOTTOM].GetDist();
        LogDetailedInfo("Bottom Plane: Normal=(%.6f, %.6f, %.6f) Distance=%.6f", 
            static_cast<float>(normal.GetX()), 
            static_cast<float>(normal.GetY()), 
            static_cast<float>(normal.GetZ()), 
            distance);

        normal  = this->mPlanes[FRUSTUMPLANE_NEAR].GetNormal();
        distance = this->mPlanes[FRUSTUMPLANE_NEAR].GetDist();
        LogDetailedInfo("Near Plane: Normal=(%.6f, %.6f, %.6f) Distance=%.6f", 
            static_cast<float>(normal.GetX()), 
            static_cast<float>(normal.GetY()), 
            static_cast<float>(normal.GetZ()), 
            distance);

        normal  = this->mPlanes[FRUSTUMPLANE_FAR].GetNormal();
        distance = this->mPlanes[FRUSTUMPLANE_FAR].GetDist();
        LogDetailedInfo("Far Plane: Normal=(%.6f, %.6f, %.6f) Distance=%.6f", 
            static_cast<float>(normal.GetX()), 
            static_cast<float>(normal.GetY()), 
            static_cast<float>(normal.GetZ()), 
            distance);
    }


    // initialize the frustum planes from the given matrix
    void ViewFrustum::InitFromMatrix(const Matrix& viewProjMatrix)
    {
        // extract the right plane
        AZ::Vector4 rightPlane;
        rightPlane.SetX(MMAT(viewProjMatrix, 0, 3) - MMAT(viewProjMatrix, 0, 0));
        rightPlane.SetY(MMAT(viewProjMatrix, 1, 3) - MMAT(viewProjMatrix, 1, 0));
        rightPlane.SetZ(MMAT(viewProjMatrix, 2, 3) - MMAT(viewProjMatrix, 2, 0));
        rightPlane.SetW(MMAT(viewProjMatrix, 3, 3) - MMAT(viewProjMatrix, 3, 0));

        // normalize the result and construct the plane equation
        rightPlane.Normalize();
        this->mPlanes[FRUSTUMPLANE_RIGHT] = PlaneEq(AZ::Vector3(rightPlane.GetX(), rightPlane.GetY(), rightPlane.GetZ()), rightPlane.GetW());

        // extract the left plane
        AZ::Vector4 leftPlane;
        leftPlane.SetX(MMAT(viewProjMatrix, 0, 3) + MMAT(viewProjMatrix, 0, 0));
        leftPlane.SetY(MMAT(viewProjMatrix, 1, 3) + MMAT(viewProjMatrix, 1, 0));
        leftPlane.SetZ(MMAT(viewProjMatrix, 2, 3) + MMAT(viewProjMatrix, 2, 0));
        leftPlane.SetW(MMAT(viewProjMatrix, 3, 3) + MMAT(viewProjMatrix, 3, 0));

        // normalize the result and construct the plane equation
        leftPlane.Normalize();
        this->mPlanes[FRUSTUMPLANE_LEFT] = PlaneEq(AZ::Vector3(leftPlane.GetX(), leftPlane.GetY(), leftPlane.GetZ()), leftPlane.GetW());

        // extract the bottom plane
        AZ::Vector4 bottomPlane;
        bottomPlane.SetX(MMAT(viewProjMatrix, 0, 3) + MMAT(viewProjMatrix, 0, 1));
        bottomPlane.SetY(MMAT(viewProjMatrix, 1, 3) + MMAT(viewProjMatrix, 1, 1));
        bottomPlane.SetZ(MMAT(viewProjMatrix, 2, 3) + MMAT(viewProjMatrix, 2, 1));
        bottomPlane.SetW(MMAT(viewProjMatrix, 3, 3) + MMAT(viewProjMatrix, 3, 1));

        // normalize the result and construct the plane equation
        bottomPlane.Normalize();
        this->mPlanes[FRUSTUMPLANE_BOTTOM] = PlaneEq(AZ::Vector3(bottomPlane.GetX(), bottomPlane.GetY(), bottomPlane.GetZ()), bottomPlane.GetW());

        // extract the top plane
        AZ::Vector4 topPlane;
        topPlane.SetX(MMAT(viewProjMatrix, 0, 3) - MMAT(viewProjMatrix, 0, 1));
        topPlane.SetY(MMAT(viewProjMatrix, 1, 3) - MMAT(viewProjMatrix, 1, 1));
        topPlane.SetZ(MMAT(viewProjMatrix, 2, 3) - MMAT(viewProjMatrix, 2, 1));
        topPlane.SetW(MMAT(viewProjMatrix, 3, 3) - MMAT(viewProjMatrix, 3, 1));

        // normalize the result and construct the plane equation
        topPlane.Normalize();
        this->mPlanes[FRUSTUMPLANE_TOP] = PlaneEq(AZ::Vector3(topPlane.GetX(), topPlane.GetY(), topPlane.GetZ()), topPlane.GetW());

        // extract the far plane
        AZ::Vector4 farPlane;
        farPlane.SetX(MMAT(viewProjMatrix, 0, 3) - MMAT(viewProjMatrix, 0, 2));
        farPlane.SetY(MMAT(viewProjMatrix, 1, 3) - MMAT(viewProjMatrix, 1, 2));
        farPlane.SetZ(MMAT(viewProjMatrix, 2, 3) - MMAT(viewProjMatrix, 2, 2));
        farPlane.SetW(MMAT(viewProjMatrix, 3, 3) - MMAT(viewProjMatrix, 3, 2));

        // normalize the result and construct the plane equation
        farPlane.Normalize();
        this->mPlanes[FRUSTUMPLANE_FAR] = PlaneEq(AZ::Vector3(farPlane.GetX(), farPlane.GetY(), farPlane.GetZ()), farPlane.GetW());

        // extract the near plane
        AZ::Vector4 nearPlane;
        nearPlane.SetX(MMAT(viewProjMatrix, 0, 3) + MMAT(viewProjMatrix, 0, 2));
        nearPlane.SetY(MMAT(viewProjMatrix, 1, 3) + MMAT(viewProjMatrix, 1, 2));
        nearPlane.SetZ(MMAT(viewProjMatrix, 2, 3) + MMAT(viewProjMatrix, 2, 2));
        nearPlane.SetW(MMAT(viewProjMatrix, 3, 3) + MMAT(viewProjMatrix, 3, 3));

        // normalize the result and construct the plane equation
        nearPlane.Normalize();
        this->mPlanes[FRUSTUMPLANE_NEAR] = PlaneEq(AZ::Vector3(nearPlane.GetX(), nearPlane.GetY(), nearPlane.GetZ()), nearPlane.GetW());
    }
}   // namespace MCore
