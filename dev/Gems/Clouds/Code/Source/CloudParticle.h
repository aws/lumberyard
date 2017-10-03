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

#include <MathConversion.h>

namespace CloudsGem
{
    class CloudParticle
    {
    public:
        AZ_TYPE_INFO(CloudParticle, "{10EFB89A-0AD3-41D7-969D-D9124781174F}");

        //! Perform reflection
        static void Reflect(AZ::ReflectContext* context);

        //! ctor
        CloudParticle() = default;
        CloudParticle(const Vec3& position, float radius, float angle, int32 textureId, const Vec3& uv0, const Vec3& uv1);

        //! dtor
        ~CloudParticle() = default;

        //! Returns radius x component
        float GetRadius() const { return m_radius; }

        //! Returns transparency value
        float GetTransparency() const { return m_transparency; }

        //! Returns particle position
        const Vec3 GetPosition() const { return AZVec3ToLYVec3(m_position); }

        //! Returns particle base color
        const ColorF& GetBaseColor() const { return AZColorToLYVec3(m_baseColor); }

        //! Returns the number of list colors?
        uint32 GetNumLitColors() const { return m_colors.size(); }

        //! Returns the lit color
        const ColorF GetLitColor(uint32 i) const { return i < m_colors.size() ? m_colors[i] : Col_Black; }

        //! Gets the square sort distance
        float GetSquareSortDistance() const { return m_squareSortDistance; }

        //! Sets the x component of particle radius.
        void SetRadius(float rad) { m_radius = rad; }

        //! Sets the particle transparency.
        void SetTransparency(float trans) { m_transparency = trans; }

        //! Sets the particle position.
        void SetPosition(const Vec3& pos) { m_position = LYVec3ToAZVec3(pos); }

        //! Sets the particle base color.
        void SetBaseColor(const ColorF& col) { m_baseColor = AZ::Color(col.r, col.g, col.b, col.a); }

        //! Adds a color for rendering.
        void AddLitColor(const ColorF& col) { m_colors.push_back(col); }

        //! Clears colors
        void ClearLitColors() { m_colors.clear(); }

        //! Sets the squared distance value
        void SetSquareSortDistance(float fSquareDistance) { m_squareSortDistance = fSquareDistance; }

        //! Gets the UV coordinates at the index specified.
        const Vec3 GetUV(int index) { return index == 0 ? AZVec3ToLYVec3(m_uv0) : AZVec3ToLYVec3(m_uv1); }

        //! overloaded comparators for sorting
        bool operator<(const CloudParticle& p) const { return m_squareSortDistance < p.m_squareSortDistance; }
        bool operator>(const CloudParticle& p) const { return m_squareSortDistance > p.m_squareSortDistance; }

    protected:

        // Serialized members
        AZ::Vector3 m_position{0.f, 0.f, 0.f};      //! Particle position
        float m_radius{ 0 };                        //! Size of particle
        float m_angle{ 0 };                         //! Not currently used but in XML
        uint32 m_textureId{ 0 };                    //! Texture Id for particle
        AZ::Vector3 m_uv0;                          //! UV sets
        AZ::Vector3 m_uv1;                          //! UV sets
        float m_transparency{0};                    //! Particle transparency
        float m_squareSortDistance{ 0 };            //! Used while sorting particles during shading
        AZ::Color m_baseColor{ 0.f, 0.f, 0.f, 1.f}; //! Base color of particle
        AZStd::vector<ColorF> m_colors;                    //! Array of lit colors
    };
}
