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

#include <PhysX_precompiled.h>
#include <Editor/DebugDraw.h>
#include <Editor/ConfigurationWindowBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <LyViewPaneNames.h>
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>

namespace PhysX
{
    namespace DebugDraw
    {
        const AZ::u32 MaxTriangles = 16384 * 3;
        const AZ::u32 MaxTrianglesRange = 800;
        const AZ::Color WarningColor(1.0f, 0.0f, 0.0f, 1.0f);
        const float WarningFrequency = 1.0f; // the number of times per second to flash

        const AZ::Color WireframeColor(0.0f, 0.0f, 0.0f, 0.7f);
        const float ColliderLineWidth = 2.0f;

        void OpenPhysXSettingsWindow()
        {
            // Open configuration window
            AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::OpenViewPane,
                LyViewPane::PhysXConfigurationEditor);

            // Set to Global Settings configuration tab
            Editor::ConfigurationWindowRequestBus::Broadcast(&Editor::ConfigurationWindowRequests::ShowGlobalSettingsTab);
        }

        bool IsGlobalColliderDebugCheck(PhysX::EditorConfiguration::GlobalCollisionDebugState requiredState)
        {
            PhysX::Configuration configuration{};
            PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);
            return configuration.m_editorConfiguration.m_globalCollisionDebugDraw == requiredState;
        }

        // Collider
        void Collider::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Collider>()
                    ->Version(1)
                    ->Field("LocallyEnabled", &Collider::m_locallyEnabled)
                    ->Field("GlobalButtonState", &Collider::m_globalButtonState)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    using GlobalCollisionDebugState = EditorConfiguration::GlobalCollisionDebugState;
                    using VisibilityFunc = bool(*)();

                    editContext->Class<Collider>(
                        "PhysX Collider Debug Draw", "Manages global and per-collider debug draw settings and logic")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Collider::m_locallyEnabled, "Draw collider",
                            "Shows the geometry for the collider in the viewport")
                        ->Attribute(AZ::Edit::Attributes::CheckboxTooltip,
                            "If set, the geometry of this collider is visible in the viewport")
                        ->Attribute(AZ::Edit::Attributes::Visibility,
                            VisibilityFunc{ []() { return IsGlobalColliderDebugCheck(GlobalCollisionDebugState::Manual); } })
                        ->DataElement(AZ::Edit::UIHandlers::Button, &Collider::m_globalButtonState, "Draw collider",
                            "Shows the geometry for the collider in the viewport")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Global override")
                        ->Attribute(AZ::Edit::Attributes::ButtonTooltip,
                            "A global setting is overriding this property (to disable the override, "
                            "set the Global Collision Debug setting to \"Set manually\" in the PhysX Configuration)")
                        ->Attribute(AZ::Edit::Attributes::Visibility,
                            VisibilityFunc{ []() { return !IsGlobalColliderDebugCheck(GlobalCollisionDebugState::Manual); } })
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OpenPhysXSettingsWindow)
                        ;
                }
            }
        }

        void Collider::Connect(AZ::EntityId entityId)
        {
            m_entityId = entityId;
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entityId);
        }

        void Collider::SetDisplayCallback(const DisplayCallback* callback)
        {
            m_displayCallback = callback;
        }

        void Collider::Disconnect()
        {
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            m_displayCallback = nullptr;
            m_entityId = AZ::EntityId();
            m_triangleIndexesByMaterialSlot.clear();
            m_verts.clear();
            m_points.clear();
            m_indices.clear();
        }

        void Collider::SetMeshDirty()
        {
            m_meshDirty = true;
        }

        void Collider::BuildMeshes(const Physics::ShapeConfiguration& shapeConfig,
            const AZ::Data::Asset<Pipeline::MeshAsset>& meshColliderAsset) const
        {
            if (m_meshDirty)
            {
                m_verts.clear();
                m_indices.clear();
                m_points.clear();

                switch (shapeConfig.GetShapeType())
                {
                case Physics::ShapeType::Sphere:
                {
                    const auto& sphereConfig = static_cast<const Physics::SphereShapeConfiguration&>(shapeConfig);
                    AZ::Vector3 boxMax = AZ::Vector3(sphereConfig.m_scale * sphereConfig.m_radius);
                    BuildAABBVerts(-boxMax, boxMax);
                    m_meshDirty = false;
                }
                break;
                case Physics::ShapeType::Box:
                {
                    const auto& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(shapeConfig);
                    AZ::Vector3 boxMax = boxConfig.m_scale * 0.5f * boxConfig.m_dimensions;
                    BuildAABBVerts(-boxMax, boxMax);
                    m_meshDirty = false;
                }
                break;
                case Physics::ShapeType::Capsule:
                {
                    const auto& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfig);
                    LmbrCentral::CapsuleGeometrySystemRequestBus::Broadcast(
                        &LmbrCentral::CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
                        capsuleConfig.m_radius * capsuleConfig.m_scale.GetX(),
                        capsuleConfig.m_height * capsuleConfig.m_scale.GetZ(),
                        16, 8,
                        m_verts,
                        m_indices,
                        m_points
                    );
                    m_meshDirty = false;
                }
                break;
                case Physics::ShapeType::PhysicsAsset:
                    if (meshColliderAsset && meshColliderAsset.IsReady())
                    {
                        physx::PxBase* meshData = meshColliderAsset.Get()->GetMeshData();

                        if (meshData)
                        {
                            if (meshData->is<physx::PxTriangleMesh>())
                            {
                                BuildTriangleMesh(meshData);
                            }
                            else
                            {
                                BuildConvexMesh(meshData);
                            }
                        }

                        m_meshDirty = false;
                    }
                    break;
                }
                if ((m_indices.size() / 3) >= MaxTriangles)
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, m_entityId);
                    if (entity)
                    {
                        AZ_Warning("PhysX Debug Draw", false, "Mesh has too many triangles! Entity: '%s', id: %s",
                            entity->GetName().c_str(), entity->GetId().ToString().c_str());
                    }
                }
            }
        }

        void Collider::BuildAABBVerts(const AZ::Vector3& boxMin, const AZ::Vector3& boxMax) const
        {
            struct Triangle
            {
                AZ::Vector3 m_points[3];

                Triangle(AZ::Vector3 point0, AZ::Vector3 point1, AZ::Vector3 point2)
                {
                    m_points[0] = point0;
                    m_points[1] = point1;
                    m_points[2] = point2;
                }
            };

            float x[2] = { boxMin.GetX(), boxMax.GetX() };
            float y[2] = { boxMin.GetY(), boxMax.GetY() };
            float z[2] = { boxMin.GetZ(), boxMax.GetZ() };

            // There are 12 triangles in an AABB
            AZStd::vector<Triangle> triangles;
            // Bottom
            triangles.push_back(Triangle(AZ::Vector3(x[0], y[0], z[0]), AZ::Vector3(x[1], y[1], z[0]), AZ::Vector3(x[1], y[0], z[0])));
            triangles.push_back(Triangle(AZ::Vector3(x[0], y[0], z[0]), AZ::Vector3(x[0], y[1], z[0]), AZ::Vector3(x[1], y[1], z[0])));
            // Top
            triangles.push_back(Triangle(AZ::Vector3(x[0], y[0], z[1]), AZ::Vector3(x[1], y[0], z[1]), AZ::Vector3(x[1], y[1], z[1])));
            triangles.push_back(Triangle(AZ::Vector3(x[0], y[0], z[1]), AZ::Vector3(x[1], y[1], z[1]), AZ::Vector3(x[0], y[1], z[1])));
            // Near
            triangles.push_back(Triangle(AZ::Vector3(x[0], y[0], z[0]), AZ::Vector3(x[1], y[0], z[1]), AZ::Vector3(x[1], y[0], z[1])));
            triangles.push_back(Triangle(AZ::Vector3(x[0], y[0], z[0]), AZ::Vector3(x[1], y[0], z[1]), AZ::Vector3(x[0], y[0], z[1])));
            // Far
            triangles.push_back(Triangle(AZ::Vector3(x[0], y[1], z[0]), AZ::Vector3(x[1], y[1], z[1]), AZ::Vector3(x[0], y[1], z[1])));
            triangles.push_back(Triangle(AZ::Vector3(x[0], y[1], z[0]), AZ::Vector3(x[1], y[1], z[0]), AZ::Vector3(x[1], y[1], z[1])));
            // Left
            triangles.push_back(Triangle(AZ::Vector3(x[0], y[1], z[0]), AZ::Vector3(x[0], y[0], z[1]), AZ::Vector3(x[0], y[1], z[1])));
            triangles.push_back(Triangle(AZ::Vector3(x[0], y[1], z[0]), AZ::Vector3(x[0], y[0], z[0]), AZ::Vector3(x[0], y[0], z[1])));
            // Right
            triangles.push_back(Triangle(AZ::Vector3(x[1], y[0], z[0]), AZ::Vector3(x[1], y[1], z[0]), AZ::Vector3(x[1], y[1], z[1])));
            triangles.push_back(Triangle(AZ::Vector3(x[1], y[0], z[0]), AZ::Vector3(x[1], y[1], z[1]), AZ::Vector3(x[1], y[0], z[1])));

            int index = 0;

            for (const auto& triangle : triangles)
            {
                m_verts.push_back(triangle.m_points[0]);
                m_verts.push_back(triangle.m_points[1]);
                m_verts.push_back(triangle.m_points[2]);

                m_indices.push_back(index++);
                m_indices.push_back(index++);
                m_indices.push_back(index++);

                m_points.push_back(triangle.m_points[0]);
                m_points.push_back(triangle.m_points[1]);
                m_points.push_back(triangle.m_points[1]);
                m_points.push_back(triangle.m_points[2]);
                m_points.push_back(triangle.m_points[2]);
                m_points.push_back(triangle.m_points[0]);
            }
        }

        void Collider::BuildTriangleMesh(physx::PxBase* meshData) const
        {
            physx::PxTriangleMeshGeometry mesh = physx::PxTriangleMeshGeometry(reinterpret_cast<physx::PxTriangleMesh*>(meshData));

            const physx::PxTriangleMesh* triangleMesh = mesh.triangleMesh;
            const physx::PxVec3* vertices = triangleMesh->getVertices();
            const AZ::u32 vertCount = triangleMesh->getNbVertices();
            const AZ::u32 triangleCount = triangleMesh->getNbTriangles();
            const void* triangles = triangleMesh->getTriangles();

            m_verts.reserve(vertCount);
            m_indices.reserve(triangleCount * 3);
            m_points.reserve(triangleCount * 3 * 2);
            m_triangleIndexesByMaterialSlot.clear();

            physx::PxTriangleMeshFlags triangleMeshFlags = triangleMesh->getTriangleMeshFlags();
            const bool mesh16BitVertexIndices = triangleMeshFlags.isSet(physx::PxTriangleMeshFlag::Enum::e16_BIT_INDICES);

            auto GetVertIndex = [=](AZ::u32 index) -> AZ::u32
            {
                if (mesh16BitVertexIndices)
                {
                    return reinterpret_cast<const physx::PxU16*>(triangles)[index];
                }
                else
                {
                    return reinterpret_cast<const physx::PxU32*>(triangles)[index];
                }
            };

            for (AZ::u32 vertIndex = 0; vertIndex < vertCount; ++vertIndex)
            {
                AZ::Vector3 vert = PxMathConvert(vertices[vertIndex]);
                m_verts.push_back(vert);
            }

            for (AZ::u32 triangleIndex = 0; triangleIndex < triangleCount * 3; triangleIndex += 3)
            {
                AZ::u32 index1 = GetVertIndex(triangleIndex);
                AZ::u32 index2 = GetVertIndex(triangleIndex + 1);
                AZ::u32 index3 = GetVertIndex(triangleIndex + 2);

                AZ::Vector3 a = m_verts[index1];
                AZ::Vector3 b = m_verts[index2];
                AZ::Vector3 c = m_verts[index3];
                m_indices.push_back(index1);
                m_indices.push_back(index2);
                m_indices.push_back(index3);

                m_points.push_back(a);
                m_points.push_back(b);
                m_points.push_back(b);
                m_points.push_back(c);
                m_points.push_back(c);
                m_points.push_back(a);

                const physx::PxMaterialTableIndex materialIndex = triangleMesh->getTriangleMaterialIndex(triangleIndex / 3);
                const int slotIndex = static_cast<const int>(materialIndex);
                m_triangleIndexesByMaterialSlot[slotIndex].push_back(index1);
                m_triangleIndexesByMaterialSlot[slotIndex].push_back(index2);
                m_triangleIndexesByMaterialSlot[slotIndex].push_back(index3);
            }
        }

        void Collider::BuildConvexMesh(physx::PxBase* meshData) const
        {
            physx::PxConvexMeshGeometry mesh = physx::PxConvexMeshGeometry(reinterpret_cast<physx::PxConvexMesh*>(meshData));
            const physx::PxConvexMesh* convexMesh = mesh.convexMesh;
            const physx::PxU8* indices = convexMesh->getIndexBuffer();
            const physx::PxVec3* vertices = convexMesh->getVertices();
            const AZ::u32 numPolys = convexMesh->getNbPolygons();

            m_verts.clear();
            m_points.clear();

            for (AZ::u32 polygonIndex = 0; polygonIndex < numPolys; ++polygonIndex)
            {
                physx::PxHullPolygon poly;
                convexMesh->getPolygonData(polygonIndex, poly);

                AZ::u32 index1 = 0;
                AZ::u32 index2 = 1;
                AZ::u32 index3 = 2;

                const AZ::Vector3 a = PxMathConvert(vertices[indices[poly.mIndexBase + index1]]);
                const AZ::u32 triangleCount = poly.mNbVerts - 2;

                for (AZ::u32 triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
                {
                    AZ_Assert(index3 < poly.mNbVerts, "Implementation error: attempted to index outside range of polygon vertices.");

                    const AZ::Vector3 b = PxMathConvert(vertices[indices[poly.mIndexBase + index2]]);
                    const AZ::Vector3 c = PxMathConvert(vertices[indices[poly.mIndexBase + index3]]);

                    m_verts.push_back(a);
                    m_verts.push_back(b);
                    m_verts.push_back(c);

                    m_points.push_back(a);
                    m_points.push_back(b);
                    m_points.push_back(b);
                    m_points.push_back(c);
                    m_points.push_back(c);
                    m_points.push_back(a);

                    index2 = index3++;
                }
            }
        }

        AZ::Color Collider::CalcDebugColor(const Physics::ColliderConfiguration& colliderConfig
            , const ElementDebugInfo& elementDebugInfo) const
        {
            using GlobalCollisionDebugColorMode = PhysX::EditorConfiguration::GlobalCollisionDebugColorMode;

            AZ::Color debugColor = AZ::Colors::White;

            PhysX::Configuration globalConfiguration;
            PhysX::ConfigurationRequestBus::BroadcastResult(globalConfiguration, &ConfigurationRequests::GetConfiguration);
            GlobalCollisionDebugColorMode debugDrawColorMode = globalConfiguration.m_editorConfiguration.m_globalCollisionDebugDrawColorMode;

            switch (debugDrawColorMode)
            {
            case GlobalCollisionDebugColorMode::MaterialColor:
            {
                Physics::MaterialFromAssetConfiguration materialConfiguration;
                const Physics::MaterialId materialId = colliderConfig.m_materialSelection.GetMaterialId(elementDebugInfo.m_materialSlotIndex);
                if (colliderConfig.m_materialSelection.GetMaterialConfiguration(materialConfiguration, materialId))
                {
                    debugColor = materialConfiguration.m_configuration.m_debugColor;
                }
                break;
            }
            case GlobalCollisionDebugColorMode::ErrorColor:
            {
                debugColor = CalcDebugColorWarning(debugColor, elementDebugInfo.m_numTriangles);
                break;
            }
            // Don't add default case, compilation warning C4062 will happen if user adds a new ColorMode to the enum and not handles the case
            }
            debugColor.SetA(0.5f);
            return debugColor;
        }

        AZ::Color Collider::CalcDebugColorWarning(const AZ::Color& currentColor, AZ::u32 triangleCount) const
        {
            // Show glowing warning color when the triangle count exceeds the maximum allowed triangles
            if (triangleCount > MaxTriangles)
            {
                AZ::ScriptTimePoint currentTimePoint;
                AZ::TickRequestBus::BroadcastResult(currentTimePoint, &AZ::TickRequests::GetTimeAtCurrentTick);
                float currentTime = static_cast<float>(currentTimePoint.GetSeconds());
                float alpha = fabsf(sinf(currentTime * AZ::Constants::HalfPi * WarningFrequency));
                alpha *= static_cast<float>(AZStd::GetMin(MaxTrianglesRange, triangleCount - MaxTriangles)) /
                    static_cast<float>(MaxTriangles);
                return currentColor * (1.0f - alpha) + WarningColor * alpha;
            }
            return currentColor;
        }

        void Collider::DrawSphere(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig,
            const Physics::SphereShapeConfiguration& sphereShapeConfig,
            const AZ::Vector3& transformScale) const
        {
            const AZ::Transform scaleMatrix = AZ::Transform::CreateScale(transformScale);
            debugDisplay.PushMatrix(GetColliderLocalTransform(colliderConfig) * scaleMatrix);
            debugDisplay.SetColor(CalcDebugColor(colliderConfig));
            debugDisplay.DrawBall(AZ::Vector3::CreateZero(), sphereShapeConfig.m_radius);
            debugDisplay.SetColor(WireframeColor);
            debugDisplay.DrawWireSphere(AZ::Vector3::CreateZero(), sphereShapeConfig.m_radius);
            debugDisplay.PopMatrix();
        }

        void Collider::DrawBox(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig,
            const Physics::BoxShapeConfiguration& boxShapeConfig,
            const AZ::Vector3& transformScale) const
        {
            const AZ::Transform scaleMatrix = AZ::Transform::CreateScale(transformScale);
            const AZ::Color& faceColor = CalcDebugColor(colliderConfig);
            debugDisplay.PushMatrix(GetColliderLocalTransform(colliderConfig) * scaleMatrix);
            debugDisplay.SetColor(faceColor);
            debugDisplay.DrawSolidBox(-boxShapeConfig.m_dimensions * 0.5f, boxShapeConfig.m_dimensions * 0.5f);
            debugDisplay.SetColor(WireframeColor);
            debugDisplay.DrawWireBox(-boxShapeConfig.m_dimensions * 0.5f, boxShapeConfig.m_dimensions * 0.5f);
            debugDisplay.PopMatrix();
        }

        void Collider::DrawCapsule(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig,
            const Physics::CapsuleShapeConfiguration& capsuleShapeConfig,
            const AZ::Vector3& transformScale) const
        {
            debugDisplay.PushMatrix(GetColliderLocalTransform(colliderConfig));

            LmbrCentral::CapsuleGeometrySystemRequestBus::Broadcast(
                &LmbrCentral::CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
                capsuleShapeConfig.m_radius * transformScale.GetX(),
                capsuleShapeConfig.m_height * transformScale.GetZ(),
                16, 8,
                m_verts,
                m_indices,
                m_points
            );

            const AZ::Color& faceColor = CalcDebugColor(colliderConfig);
            debugDisplay.DrawTrianglesIndexed(m_verts, m_indices, faceColor);
            debugDisplay.DrawLines(m_points, WireframeColor);
            debugDisplay.SetLineWidth(ColliderLineWidth);
            debugDisplay.PopMatrix();
        }

        void Collider::DrawMesh(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig,
            const Physics::PhysicsAssetShapeConfiguration& assetConfig,
            const AZ::Data::Asset<Pipeline::MeshAsset>& meshColliderAsset) const
        {
            if (meshColliderAsset && meshColliderAsset.IsReady())
            {
                const AZ::Transform scaleMatrix = AZ::Transform::CreateScale(GetNonUniformScale() * assetConfig.m_assetScale);
                debugDisplay.PushMatrix(GetColliderLocalTransform(colliderConfig) * scaleMatrix);

                physx::PxBase* meshData = meshColliderAsset.Get()->GetMeshData();

                if (meshData)
                {
                    if (meshData->is<physx::PxTriangleMesh>())
                    {
                        DrawTriangleMesh(debugDisplay, colliderConfig, meshData);
                    }
                    else
                    {
                        DrawConvexMesh(debugDisplay, colliderConfig, meshData);
                    }
                }

                debugDisplay.PopMatrix();
            }
        }

        void Collider::DrawTriangleMesh(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig, physx::PxBase* meshData) const
        {
            if (!m_verts.empty())
            {
                for (const auto& element : m_triangleIndexesByMaterialSlot)
                {
                    const int materialSlot = element.first;
                    const AZStd::vector<AZ::u32>& triangleIndexes = element.second;
                    const AZ::u32 triangleCount = static_cast<AZ::u32>(triangleIndexes.size() / 3);

                    ElementDebugInfo triangleMeshInfo;
                    triangleMeshInfo.m_numTriangles = triangleCount;
                    triangleMeshInfo.m_materialSlotIndex = materialSlot;

                    debugDisplay.DrawTrianglesIndexed(m_verts, triangleIndexes
                        , CalcDebugColor(colliderConfig, triangleMeshInfo));
                }
                debugDisplay.DrawLines(m_points, WireframeColor);
            }

        }

        void Collider::DrawConvexMesh(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig, physx::PxBase* meshData) const
        {
            if (!m_verts.empty())
            {
                const AZ::u32 triangleCount = static_cast<AZ::u32>(m_verts.size() / 3);
                ElementDebugInfo convexMeshInfo;
                convexMeshInfo.m_numTriangles = triangleCount;

                debugDisplay.DrawTriangles(m_verts, CalcDebugColor(colliderConfig, convexMeshInfo));
                debugDisplay.DrawLines(m_points, WireframeColor);
            }
        }

        AZ::Transform Collider::GetColliderLocalTransform(const Physics::ColliderConfiguration& colliderConfig) const
        {
            return AZ::Transform::CreateFromQuaternionAndTranslation(
                colliderConfig.m_rotation, colliderConfig.m_position * GetNonUniformScale());
        }

        float Collider::GetUniformScale() const
        {
            return GetNonUniformScale().GetMaxElement();
        }

        AZ::Vector3 Collider::GetNonUniformScale() const
        {
            AZ::Vector3 scale = AZ::Vector3::CreateOne();
            AZ::TransformBus::EventResult(scale, m_entityId, &AZ::TransformInterface::GetLocalScale);
            return scale;
        }

        const AZStd::vector<AZ::Vector3>& Collider::GetVerts() const
        {
            return m_verts;
        }

        const AZStd::vector<AZ::Vector3>& Collider::GetPoints() const
        {
            return m_points;
        }

        const AZStd::vector<AZ::u32>& Collider::GetIndices() const
        {
            return m_indices;
        }

        // AzFramework::EntityDebugDisplayEventBus
        void Collider::DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay)
        {
            if (!m_displayCallback)
            {
                return;
            }

            // Let each collider decide how to scale itself, so extract the scale here.
            AZ::Transform entityWorldTransformWithoutScale = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(entityWorldTransformWithoutScale, m_entityId, &AZ::TransformInterface::GetWorldTM);
            entityWorldTransformWithoutScale.ExtractScale();

            PhysX::Configuration globalConfiguration;
            PhysX::ConfigurationRequestBus::BroadcastResult(globalConfiguration, &ConfigurationRequests::GetConfiguration);
            PhysX::Settings::ColliderProximityVisualization& proximityVisualization =
                globalConfiguration.m_settings.m_colliderProximityVisualization;
            const bool colliderIsInRange =
                proximityVisualization.m_cameraPosition.GetDistanceSq(entityWorldTransformWithoutScale.GetPosition()) <
                proximityVisualization.m_radius * proximityVisualization.m_radius;

            const PhysX::EditorConfiguration::GlobalCollisionDebugState& globalCollisionDebugDraw =
                globalConfiguration.m_editorConfiguration.m_globalCollisionDebugDraw;
            if (globalCollisionDebugDraw != PhysX::EditorConfiguration::GlobalCollisionDebugState::AlwaysOff)
            {
                if (globalCollisionDebugDraw == PhysX::EditorConfiguration::GlobalCollisionDebugState::AlwaysOn
                    || m_locallyEnabled
                    || (proximityVisualization.m_enabled && colliderIsInRange))
                {
                    debugDisplay.PushMatrix(entityWorldTransformWithoutScale);
                    m_displayCallback->Display(debugDisplay);
                    debugDisplay.PopMatrix();
                }
            }
        }
    } // namespace DebugDraw
} // namespace PhysX
