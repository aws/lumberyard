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

#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    /// 3D Frustum class. 
    /// Plane normals are assumed to point to the interior of the frustum.
    class Frustum
    {
    public:
        
        AZ_TYPE_INFO(Frustum, "{B197DA61-19CC-4A5E-9D13-9C644CBAF6E2}")
        Frustum() {}
        explicit Frustum(const Plane& nearPlane, const Plane& farPlane , const Plane& leftPlane,
            const Plane& rightPlane, const Plane& topPlane, const Plane& bottomPlane)
            : m_near(nearPlane)
            , m_far(farPlane)
            , m_left(leftPlane)
            , m_right(rightPlane)
            , m_top(topPlane)
            , m_bottom(bottomPlane)
        {
        }

        /**
         * Extract frustum from matrix. Matrix Usage and Z conventions
         * form the matrix of use cases.
         * SymmetricZ implies -w <= z <= w, non symmetric 0<=z<=w
         * RowMajor implies x*M convention
         * ColumnMajor implies M*x convention;
         */
        static const Frustum CreateFromMatrixRowMajor(Matrix4x4 matrix)
        {
            Frustum frustum;
           
            frustum.SetNearPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(2)));

            frustum.SetFarPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(2)));

            frustum.SetLeftPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) + matrix.GetColumn(0)));

            frustum.SetRightPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(0)));

            frustum.SetTopPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(1)));

            frustum.SetBottomPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) + matrix.GetColumn(1)));

            return frustum;
        }

        static const Frustum CreateFromMatrixColumnMajor(Matrix4x4 matrix)
        {
            Frustum frustum;
            
            frustum.SetNearPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(2)));
            
            frustum.SetFarPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(2)));
            
            frustum.SetLeftPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(3) + matrix.GetRow(0)));

            frustum.SetRightPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(0)));

            frustum.SetTopPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(1)));

            frustum.SetBottomPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(3) + matrix.GetRow(1)));

            return frustum;
        }

        static const Frustum CreateFromMatrixRowMajorSymmetricZ(Matrix4x4 matrix)
        {
            Frustum frustum;
            
            frustum.SetNearPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) + matrix.GetColumn(2)));
            
            frustum.SetFarPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(2)));

            frustum.SetLeftPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) + matrix.GetColumn(0)));
            
            frustum.SetRightPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(0)));

            frustum.SetTopPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(1)));

            frustum.SetBottomPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) + matrix.GetColumn(1)));

            return frustum;
        }

        static const Frustum CreateFromMatrixColumnMajorSymmetricZ(Matrix4x4 matrix)
        {
            Frustum frustum;
            frustum.SetNearPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(3) + matrix.GetRow(2)));

            frustum.SetFarPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(2)));
           
            frustum.SetLeftPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(3) + matrix.GetRow(0)));
            
            frustum.SetRightPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(0)));
           
            frustum.SetTopPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(1)));

            frustum.SetBottomPlane(AZ::Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(1)));

            return frustum;
        }

        Plane GetNearPlane() const { return m_near; }
        Plane GetFarPlane() const { return m_far; }
        Plane GetTopPlane() const { return m_top; }
        Plane GetBottomPlane() const { return m_bottom; }
        Plane GetLeftPlane() const { return m_left; }
        Plane GetRightPlane() const { return m_right; }

        void SetNearPlane(Plane plane) { m_near = plane; }
        void SetFarPlane(Plane plane) { m_far = plane; }
        void SetLeftPlane(Plane plane) { m_left = plane; }
        void SetRightPlane(Plane plane) { m_right = plane; }
        void SetBottomPlane(Plane plane) { m_bottom = plane; }
        void SetTopPlane(Plane plane) { m_top = plane; }

        void Set(Frustum frustum)
        {
            m_near = frustum.m_near;
            m_far = frustum.m_far;
            m_left = frustum.m_left;
            m_right = frustum.m_right;
            m_top = frustum.m_top;
            m_bottom = frustum.m_bottom;
        }

        Frustum& operator=(const Frustum& rhs)
        {
            Set(rhs);
            return *this;
        }

        bool operator==(const Frustum& rhs) const
        {
            return  (m_near == rhs.m_near) &&
                (m_far == rhs.m_far) &&
                (m_left == rhs.m_left) &&
                (m_right == rhs.m_right) &&
                (m_top == rhs.m_top) &&
                (m_bottom == rhs.m_bottom);
        }

        bool operator!=(const Frustum& rhs) const
        {
            return  !(*this == rhs);
        }

    private:
        Plane m_near;
        Plane m_far;
        Plane m_left;
        Plane m_right;
        Plane m_top;
        Plane m_bottom;
    };

}//AZ