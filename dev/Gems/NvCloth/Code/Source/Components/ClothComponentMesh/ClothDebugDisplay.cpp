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

#include <NvCloth_precompiled.h>

#include <AzCore/Console/IConsole.h>

#include <AzFramework/Viewport/ViewportColors.h>
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>

#include <Components/ClothComponentMesh/ClothDebugDisplay.h>
#include <Components/ClothComponentMesh/ActorClothColliders.h>
#include <Components/ClothComponentMesh/ClothComponentMesh.h>

#include <System/Cloth.h>

namespace NvCloth
{
    AZ_CVAR(int32_t, cloth_DebugDraw, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Draw cloth wireframe mesh:\n"
        " 0 - Disabled\n"
        " 1 - Cloth wireframe and particle weights");

    AZ_CVAR(int32_t, cloth_DebugDrawNormals, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Draw cloth normals:\n"
        " 0 - Disabled\n"
        " 1 - Cloth normals\n"
        " 2 - Cloth normals, tangents and bitangents");

    AZ_CVAR(int32_t, cloth_DebugDrawColliders, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Draw cloth colliders:\n"
        " 0 - Disabled\n"
        " 1 - Cloth colliders");

    ClothDebugDisplay::ClothDebugDisplay(ClothComponentMesh* clothComponentMesh)
        : m_clothComponentMesh(clothComponentMesh)
    {
        AZ_Assert(m_clothComponentMesh, "Invalid cloth component mesh");
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_clothComponentMesh->m_entityId);
    }

    ClothDebugDisplay::~ClothDebugDisplay()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    }

    bool ClothDebugDisplay::IsDebugDrawEnabled() const
    {
        return cloth_DebugDraw > 0
            || cloth_DebugDrawNormals > 0
            || cloth_DebugDrawColliders > 0;
    }

    void ClothDebugDisplay::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_UNUSED(viewportInfo);

        if (!IsDebugDrawEnabled() || !m_clothComponentMesh->m_clothSimulation)
        {
            return;
        }

        AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(entityTransform, m_clothComponentMesh->m_entityId, &AZ::TransformInterface::GetWorldTM);
        debugDisplay.PushMatrix(entityTransform);

        if (cloth_DebugDraw > 0)
        {
            DisplayParticles(debugDisplay);
            DisplayWireCloth(debugDisplay);
        }

        if (cloth_DebugDrawNormals > 0)
        {
            bool showTangents = (cloth_DebugDrawNormals > 1);
            DisplayNormals(debugDisplay, showTangents);
        }

        if (cloth_DebugDrawColliders > 0)
        {
            DisplayColliders(debugDisplay);
        }

        debugDisplay.PopMatrix();
    }

    void ClothDebugDisplay::DisplayParticles(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const float particleAlpha = 1.0f;
        const float particleRadius = 0.007f;

        const auto& clothRenderParticles = m_clothComponentMesh->GetRenderParticles();

        for(const auto& particle : clothRenderParticles)
        {
            const AZ::Vector3 position(particle.x, particle.y, particle.z);
            const AZ::Vector4 color = AZ::Vector4::CreateFromVector3AndFloat(AZ::Vector3(particle.w), particleAlpha);
            debugDisplay.SetColor(color);
            debugDisplay.DrawBall(position, particleRadius, false/*drawShaded*/);
        }
    }

    void ClothDebugDisplay::DisplayWireCloth(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const auto& clothIndices = m_clothComponentMesh->m_clothSimulation->GetInitialIndices();
        const auto& clothRenderParticles = m_clothComponentMesh->GetRenderParticles();

        const size_t numIndices = clothIndices.size();
        if (numIndices % 3 != 0)
        {
            AZ_Warning("ClothDebugDisplay", false, 
                "Cloth indices contains a list of triangles but its count (%d) is not a multiple of 3.", numIndices);
            return;
        }

        for (size_t index = 0; index < numIndices; index += 3)
        {
            const SimIndexType& vertexIndex0 = clothIndices[index + 0];
            const SimIndexType& vertexIndex1 = clothIndices[index + 1];
            const SimIndexType& vertexIndex2 = clothIndices[index + 2];
            const SimParticleType& particle0 = clothRenderParticles[vertexIndex0];
            const SimParticleType& particle1 = clothRenderParticles[vertexIndex1];
            const SimParticleType& particle2 = clothRenderParticles[vertexIndex2];
            const AZ::Vector3 position0(particle0.x, particle0.y, particle0.z);
            const AZ::Vector3 position1(particle1.x, particle1.y, particle1.z);
            const AZ::Vector3 position2(particle2.x, particle2.y, particle2.z);
            const AZ::Vector4 color0 = AZ::Vector4::CreateFromVector3(AZ::Vector3(particle0.w));
            const AZ::Vector4 color1 = AZ::Vector4::CreateFromVector3(AZ::Vector3(particle1.w));
            const AZ::Vector4 color2 = AZ::Vector4::CreateFromVector3(AZ::Vector3(particle2.w));
            debugDisplay.DrawLine(position0, position1, color0, color1);
            debugDisplay.DrawLine(position1, position2, color1, color2);
            debugDisplay.DrawLine(position2, position0, color2, color0);
        }
    }

    void ClothDebugDisplay::DisplayNormals(AzFramework::DebugDisplayRequests& debugDisplay, bool showTangents)
    {
        const auto& clothIndices = m_clothComponentMesh->m_clothSimulation->GetInitialIndices();
        const auto& clothRenderParticles = m_clothComponentMesh->GetRenderParticles();
        const auto& clothRenderTangentSpaces = m_clothComponentMesh->GetRenderTangentSpaces();

        const size_t numIndices = clothIndices.size();
        if (numIndices % 3 != 0)
        {
            AZ_Warning("ClothDebugDisplay", false,
                "Cloth indices contains a list of triangles but its count (%d) is not a multiple of 3.", numIndices);
            return;
        }
        if (clothRenderParticles.size() != clothRenderTangentSpaces.GetBaseCount())
        {
            AZ_Warning("ClothDebugDisplay", false,
                "Number of cloth particles (%d) doesn't match with the number of tangent spaces (%d).", 
                clothRenderParticles.size(), clothRenderTangentSpaces.GetBaseCount());
            return;
        }

        const float normalLength = 0.05f;
        const float tangentLength = 0.05f;
        const float bitangentLength = 0.05f;
        const AZ::Vector4 colorNormal = AZ::Colors::Blue.GetAsVector4();
        const AZ::Vector4 colorTangent = AZ::Colors::Red.GetAsVector4();
        const AZ::Vector4 colorBitangent = AZ::Colors::Green.GetAsVector4();

        for (size_t index = 0; index < numIndices; ++index)
        {
            const SimIndexType& vertexIndex = clothIndices[index];

            const SimParticleType& particle = clothRenderParticles[vertexIndex];
            const AZ::Vector3 position(particle.x, particle.y, particle.z);

            const Vec3 normalVec3 = clothRenderTangentSpaces.GetNormal(vertexIndex);
            const AZ::Vector3 normal(normalVec3.x, normalVec3.y, normalVec3.z);

            debugDisplay.DrawLine(position, position + normalLength * normal, colorNormal, colorNormal);

            if (showTangents)
            {
                const Vec3 tangentVec3 = clothRenderTangentSpaces.GetTangent(vertexIndex);
                const Vec3 bitangentVec3 = clothRenderTangentSpaces.GetBitangent(vertexIndex);
                const AZ::Vector3 tangent(tangentVec3.x, tangentVec3.y, tangentVec3.z);
                const AZ::Vector3 bitangent(bitangentVec3.x, bitangentVec3.y, bitangentVec3.z);

                debugDisplay.DrawLine(position, position + tangentLength * tangent, colorTangent, colorTangent);
                debugDisplay.DrawLine(position, position + bitangentLength * bitangent, colorBitangent, colorBitangent);
            }
        }
    }

    void ClothDebugDisplay::DisplayColliders(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (!m_clothComponentMesh->m_actorClothColliders)
        {
            return;
        }

        if (m_clothComponentMesh->IsClothFullyAnimated())
        {
            return;
        }

        for (const SphereCollider& collider : m_clothComponentMesh->m_actorClothColliders->GetSphereColliders())
        {
            DrawSphereCollider(debugDisplay, collider.m_radius, collider.m_currentModelSpaceTransform);
        }

        for (const CapsuleCollider& collider : m_clothComponentMesh->m_actorClothColliders->GetCapsuleColliders())
        {
            DrawCapsuleCollider(debugDisplay, collider.m_radius, collider.m_height, collider.m_currentModelSpaceTransform);
        }
    }

    void ClothDebugDisplay::DrawSphereCollider(AzFramework::DebugDisplayRequests& debugDisplay, float radius, const AZ::Transform& transform)
    {
        debugDisplay.PushMatrix(transform);
        debugDisplay.SetColor(AzFramework::ViewportColors::DeselectedColor);
        debugDisplay.DrawBall(AZ::Vector3::CreateZero(), radius, false/*drawShaded*/);
        debugDisplay.SetColor(AzFramework::ViewportColors::WireColor);
        debugDisplay.DrawWireSphere(AZ::Vector3::CreateZero(), radius);
        debugDisplay.PopMatrix();
    }

    void ClothDebugDisplay::DrawCapsuleCollider(AzFramework::DebugDisplayRequests& debugDisplay, float radius, float height, const AZ::Transform& transform)
    {
        debugDisplay.PushMatrix(transform);

        AZStd::vector<AZ::Vector3> capsuleVertexBuffer;
        AZStd::vector<AZ::u32> capsuleIndexBuffer;
        AZStd::vector<AZ::Vector3> capsuleLineBuffer;
        const AZ::u32 sides = 16;
        const AZ::u32 capSegments = 8;

        LmbrCentral::CapsuleGeometrySystemRequestBus::Broadcast(
            &LmbrCentral::CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
            radius,
            height,
            sides, capSegments,
            capsuleVertexBuffer,
            capsuleIndexBuffer,
            capsuleLineBuffer
        );

        debugDisplay.DrawTrianglesIndexed(capsuleVertexBuffer, capsuleIndexBuffer, AzFramework::ViewportColors::DeselectedColor);
        debugDisplay.DrawLines(capsuleLineBuffer, AzFramework::ViewportColors::WireColor);

        debugDisplay.PopMatrix();
    }
} // namespace NvCloth
