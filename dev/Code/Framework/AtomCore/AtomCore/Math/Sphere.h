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
namespace AZ
{
    class Sphere
    {
    public:
        AZ_TYPE_INFO(Sphere, "{34BB6527-81AE-4854-99ED-D1A319DCD0A9}")
        Sphere() {}
        explicit Sphere(const Vector3& center, const VectorFloat& radius)
        {
            m_center = center;
            m_radius = radius;
        }

        static const Sphere CreateUnitSphere()
        {
            return Sphere(AZ::Vector3::CreateZero(), AZ::VectorFloat::CreateOne());
        }

        AZ::Vector3 GetCenter() const                   { return m_center; }
        AZ::VectorFloat GetRadius() const               { return m_radius; }
        void SetCenter(AZ::Vector3 center)       { m_center = center; }
        void SetRadius(AZ::VectorFloat radius)   { m_radius = radius; }

        void Set(const Sphere sphere)
        {
            m_center = sphere.m_center;
            m_radius = sphere.m_radius;
        }

        Sphere& operator=(const Sphere& rhs)
        {
            Set(rhs);
            return *this;
        }

        bool operator==(const Sphere& rhs) const
        {
            return  (m_center == rhs.m_center) && 
                    (m_radius == rhs.m_radius);
        }
        bool operator!=(const Sphere& rhs) const
        {
            return !(*this == rhs);
        }

    private:
        AZ::Vector3 m_center;
        AZ::VectorFloat m_radius;
    };
}//AZ