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

#include <AzCore/Interface/Interface.h>

#include <Components/ClothComponentMesh/ClothConstraints.h>

#include <NvCloth/ITangentSpaceHelper.h>

namespace NvCloth
{
    AZStd::unique_ptr<ClothConstraints> ClothConstraints::Create(
        const AZStd::vector<float>& motionConstraintsData,
        const float motionConstraintsMaxDistance,
        const AZStd::vector<AZ::Vector2>& backstopData,
        const float backstopMaxRadius,
        const float backstopMaxBackOffset,
        const float backstopMaxFrontOffset,
        const AZStd::vector<SimParticleFormat>& simParticles,
        const AZStd::vector<SimIndexType>& simIndices,
        const AZStd::vector<int>& meshRemappedVertices)
    {
        AZStd::unique_ptr<ClothConstraints> clothConstraints = AZStd::make_unique<ClothConstraints>();

        clothConstraints->m_motionConstraintsData.resize(simParticles.size(), AZ::g_fltMax);
        clothConstraints->m_motionConstraintsMaxDistance = motionConstraintsMaxDistance;
        clothConstraints->m_motionConstraints.resize(simParticles.size());

        for (size_t i = 0; i < motionConstraintsData.size(); ++i)
        {
            const int remappedIndex = meshRemappedVertices[i];
            if (remappedIndex < 0)
            {
                // Removed particle
                continue;
            }

            // Keep the minimum distance for remapped duplicated vertices
            if (motionConstraintsData[i] < clothConstraints->m_motionConstraintsData[remappedIndex])
            {
                clothConstraints->m_motionConstraintsData[remappedIndex] = motionConstraintsData[i];
            }
        }

        const bool hasBackstopData = AZStd::any_of(
            backstopData.cbegin(),
            backstopData.cend(),
            [](const AZ::Vector2& backstop)
            {
                const float radius = backstop.GetY();
                return radius > 0.0f;
            });
        if (hasBackstopData)
        {
            clothConstraints->m_backstopData.resize(simParticles.size(), AZ::Vector2(0.0f, AZ::g_fltMax));
            clothConstraints->m_backstopMaxRadius = backstopMaxRadius;
            clothConstraints->m_backstopMaxBackOffset = backstopMaxBackOffset;
            clothConstraints->m_backstopMaxFrontOffset = backstopMaxFrontOffset;
            clothConstraints->m_separationConstraints.resize(simParticles.size());

            for (size_t i = 0; i < backstopData.size(); ++i)
            {
                const int remappedIndex = meshRemappedVertices[i];
                if (remappedIndex < 0)
                {
                    // Removed particle
                    continue;
                }

                // Keep the minimum radius for remapped duplicated vertices
                if (backstopData[i].GetY() < clothConstraints->m_backstopData[remappedIndex].GetY())
                {
                    clothConstraints->m_backstopData[remappedIndex] = backstopData[i];
                }
            }
        }

        // Calculates the current constraints and fills the data as nvcloth needs them,
        // ready to be queried by the cloth component.
        clothConstraints->CalculateConstraints(simParticles, simIndices);

        return clothConstraints;
    }

    void ClothConstraints::CalculateConstraints(
        const AZStd::vector<SimParticleFormat>& simParticles,
        const AZStd::vector<SimIndexType>& simIndices)
    {
        for (size_t i = 0; i < m_motionConstraints.size(); ++i)
        {
            const AZ::Vector3 position = simParticles[i].GetAsVector3();
            const float maxDistance = (simParticles[i].GetW() > 0.0f)
                ? m_motionConstraintsData[i] * m_motionConstraintsMaxDistance
                : 0.0f;

            m_motionConstraints[i].Set(position, maxDistance);
        }

        if (!m_separationConstraints.empty())
        {
            bool normalsCalculated = AZ::Interface<ITangentSpaceHelper>::Get()->CalculateNormals(simParticles, simIndices, m_normals);
            AZ_Assert(normalsCalculated, "Cloth constraints failed to calculate normals.");

            for (size_t i = 0; i < m_separationConstraints.size(); ++i)
            {
                const AZ::Vector3 position = CalculateBackstopSpherePosition(i);

                const float radiusScale = m_backstopData[i].GetY();
                const float radius = radiusScale * m_backstopMaxRadius;

                m_separationConstraints[i].Set(position, radius);
            }
        }
    }

    const AZStd::vector<AZ::Vector4>& ClothConstraints::GetMotionConstraints() const
    {
        return m_motionConstraints;
    }

    const AZStd::vector<AZ::Vector4>& ClothConstraints::GetSeparationConstraints() const
    {
        return m_separationConstraints;
    }

    void ClothConstraints::SetMotionConstraintMaxDistance(float distance)
    {
        m_motionConstraintsMaxDistance = distance;
        for (size_t i = 0; i < m_motionConstraints.size(); ++i)
        {
            const float maxDistanceScale = m_motionConstraintsData[i];
            m_motionConstraints[i].SetW(maxDistanceScale * m_motionConstraintsMaxDistance);
        }
    }

    void ClothConstraints::SetBackstopMaxRadius(float radius)
    {
        m_backstopMaxRadius = radius;

        for (size_t i = 0; i < m_separationConstraints.size(); ++i)
        {
            const float radiusScale = m_backstopData[i].GetY();
            m_separationConstraints[i].SetW(radiusScale * m_backstopMaxRadius);
        }
    }

    void ClothConstraints::SetBackstopMaxOffsets(float backOffset, float frontOffset)
    {
        m_backstopMaxBackOffset = backOffset;
        m_backstopMaxFrontOffset = frontOffset;

        for (size_t i = 0; i < m_separationConstraints.size(); ++i)
        {
            const AZ::Vector3 position = CalculateBackstopSpherePosition(i);

            m_separationConstraints[i].Set(
                position,
                m_separationConstraints[i].GetW());
        }
    }

    AZ::Vector3 ClothConstraints::CalculateBackstopSpherePosition(size_t index) const
    {
        const float offsetScale = m_backstopData[index].GetX();
        const float offset = offsetScale * ((offsetScale >= 0.0f) ? m_backstopMaxBackOffset : m_backstopMaxFrontOffset);

        const float radiusScale = m_backstopData[index].GetY();
        const float radius = radiusScale * m_backstopMaxRadius;

        AZ::Vector3 position = m_motionConstraints[index].GetAsVector3();
        if (offset >= 0.0f)
        {
            position -= m_normals[index] * (radius + offset); // Place sphere behind the particle
        }
        else
        {
            position += m_normals[index] * (radius - offset); // Place sphere in front of the particle
        }
        return position;
    }
} // namespace NvCloth
