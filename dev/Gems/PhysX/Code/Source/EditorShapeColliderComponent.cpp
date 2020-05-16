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
#include <EditorShapeColliderComponent.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/SystemBus.h>
#include <LyViewPaneNames.h>
#include <ShapeColliderComponent.h>
#include <EditorRigidBodyComponent.h>
#include <PhysX/ColliderComponentBus.h>
#include <Editor/ConfigurationWindowBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <PhysX/SystemComponentBus.h>
#include <AzCore/Math/Geometry2DUtils.h>
#include <Source/Utils.h>

namespace PhysX
{
    EditorShapeColliderComponent::EditorShapeColliderComponent()
    {
        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::Offset, false);
    }

    void EditorShapeColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorShapeColliderComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("ColliderConfiguration", &EditorShapeColliderComponent::m_colliderConfig)
                ->Field("DebugDrawSettings", &EditorShapeColliderComponent::m_colliderDebugDraw)
                ->Field("ShapeConfigs", &EditorShapeColliderComponent::m_shapeConfigs)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorShapeColliderComponent>(
                    "PhysX Shape Collider", "Creates geometry in the PhysX simulation based on an attached shape component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PhysXCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/PhysXCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorShapeColliderComponent::m_colliderConfig,
                        "Collider configuration", "Configuration of the collider")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorShapeColliderComponent::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorShapeColliderComponent::m_colliderDebugDraw,
                        "Debug draw settings", "Debug draw settings")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void EditorShapeColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
        provided.push_back(AZ_CRC("PhysXTriggerService", 0x3a117d7b));
        provided.push_back(AZ_CRC("PhysXShapeColliderService", 0x98a7e779));
    }

    void EditorShapeColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        required.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
    }

    void EditorShapeColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Not compatible with Legacy Cry Physics services
        incompatible.push_back(AZ_CRC("ColliderService", 0x902d4e93));
        incompatible.push_back(AZ_CRC("LegacyCryPhysicsService", 0xbb370351));
        incompatible.push_back(AZ_CRC("PhysXShapeColliderService", 0x98a7e779));
    }

    const AZStd::vector<AZ::Vector3>& EditorShapeColliderComponent::GetSamplePoints() const
    {
        if (m_geometryCache.m_cachedSamplePointsDirty)
        {
            UpdateCachedSamplePoints();
            m_geometryCache.m_cachedSamplePointsDirty = false;
        }

        return m_geometryCache.m_cachedSamplePoints;
    }

    void EditorShapeColliderComponent::UpdateCachedSamplePoints() const
    {
        m_geometryCache.m_cachedSamplePoints.clear();

        switch (m_shapeType)
        {
        case ShapeType::Box:
        {
            const AZ::Vector3 boxMax = 0.5f * m_geometryCache.m_boxDimensions;
            const AZ::Vector3 boxMin = -boxMax;
            m_geometryCache.m_cachedSamplePoints = Utils::Geometry::GenerateBoxPoints(boxMin, boxMax);
            break;
        }
        case ShapeType::Sphere:
        {
            m_geometryCache.m_cachedSamplePoints = Utils::Geometry::GenerateSpherePoints(m_geometryCache.m_radius);
            break;
        }
        case ShapeType::Capsule:
        {
            const float cylinderHeight = m_geometryCache.m_height - 2 * m_geometryCache.m_radius;
            if (cylinderHeight > 0.0f)
            {
                m_geometryCache.m_cachedSamplePoints = Utils::Geometry::GenerateCylinderPoints(cylinderHeight,
                    m_geometryCache.m_radius);
            }
            break;
        }
        case ShapeType::PolygonPrism:
        {
            if (!m_shapeConfigs.empty())
            {
                AZ::PolygonPrismPtr polygonPrismPtr;
                LmbrCentral::PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(),
                    &LmbrCentral::PolygonPrismShapeComponentRequests::GetPolygonPrism);

                if (polygonPrismPtr)
                {
                    const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());

                    const AZStd::vector<AZ::Vector2>& vertices = polygonPrismPtr->m_vertexContainer.GetVertices();
                    for (const auto& vertex : vertices)
                    {
                        const float scalarScale = uniformScale.GetX();
                        const float scaledX = scalarScale * vertex.GetX();
                        const float scaledY = scalarScale * vertex.GetY();
                        m_geometryCache.m_cachedSamplePoints.emplace_back(AZ::Vector3(scaledX, scaledY, 0.0f));
                        m_geometryCache.m_cachedSamplePoints.emplace_back(AZ::Vector3(scaledX, scaledY, m_geometryCache.m_height));
                    }
                }
            }

            break;
        }
        default:
            AZ_WarningOnce("PhysX Shape Collider Component", false, "Unsupported shape type in UpdateCachedSamplePoints");
            break;
        }

        AZ::Transform transform = GetWorldTM();
        transform.ExtractScaleExact();
        const size_t numPoints = m_geometryCache.m_cachedSamplePoints.size();
        for (size_t pointIndex = 0; pointIndex < numPoints; ++pointIndex)
        {
            m_geometryCache.m_cachedSamplePoints[pointIndex] = transform * m_geometryCache.m_cachedSamplePoints[pointIndex];
        }
    }

    const Physics::ColliderConfiguration& EditorShapeColliderComponent::GetColliderConfiguration() const
    {
        return m_colliderConfig;
    }

    const AZStd::vector<AZStd::shared_ptr<Physics::ShapeConfiguration>>& EditorShapeColliderComponent::GetShapeConfigurations() const
    {
        return m_shapeConfigs;
    }

    void EditorShapeColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto* shapeColliderComponent = gameEntity->CreateComponent<ShapeColliderComponent>();
        Physics::ShapeConfigurationList shapeConfigurationList;
        shapeConfigurationList.reserve(m_shapeConfigs.size());
        for (const auto& shapeConfig : m_shapeConfigs)
        {
            shapeConfigurationList.emplace_back(
                AZStd::make_shared<Physics::ColliderConfiguration>(m_colliderConfig),
                shapeConfig);
        }

        shapeColliderComponent->SetShapeConfigurationList(shapeConfigurationList);

        StaticRigidBodyUtils::TryCreateRuntimeComponent(*GetEntity(), *gameEntity);
    }

    void EditorShapeColliderComponent::CreateStaticEditorCollider()
    {
        // Don't create static rigid body in the editor if current entity components
        // don't allow creation of runtime static rigid body component
        if (!StaticRigidBodyUtils::CanCreateRuntimeComponent(*GetEntity()))
        {
            return;
        }

        AZStd::shared_ptr<Physics::World> editorWorld;
        Physics::EditorWorldBus::BroadcastResult(editorWorld, &Physics::EditorWorldRequests::GetEditorWorld);
        if (editorWorld)
        {
            AZ::Transform colliderTransform = GetWorldTM();
            colliderTransform.ExtractScale();

            Physics::WorldBodyConfiguration configuration;
            configuration.m_orientation = AZ::Quaternion::CreateFromTransform(colliderTransform);
            configuration.m_position = colliderTransform.GetPosition();
            configuration.m_entityId = GetEntityId();
            configuration.m_debugName = GetEntity()->GetName();

            m_editorBody = AZ::Interface<Physics::System>::Get()->CreateStaticRigidBody(configuration);

            for (const auto& shapeConfig : m_shapeConfigs)
            {
                AZStd::shared_ptr<Physics::Shape> shape = AZ::Interface<Physics::System>::Get()->CreateShape(m_colliderConfig, *shapeConfig);
                m_editorBody->AddShape(shape);
            }

            if (!m_shapeConfigs.empty())
            {
                editorWorld->AddBody(*m_editorBody);
            }

            Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
        }
    }

    AZ::u32 EditorShapeColliderComponent::OnConfigurationChanged()
    {
        m_colliderConfig.m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
        CreateStaticEditorCollider();
        ColliderComponentEventBus::Event(GetEntityId(), &ColliderComponentEvents::OnColliderChanged);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorShapeColliderComponent::UpdateShapeConfigs()
    {
        m_shapeConfigs.clear();
        m_geometryCache.m_cachedSamplePointsDirty = true;

        AZ::Crc32 shapeCrc;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(shapeCrc, GetEntityId(),
            &LmbrCentral::ShapeComponentRequests::GetShapeType);

        const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());

        // using if blocks because switch statements aren't supported for values generated by the crc macro
        if (shapeCrc == ShapeConstants::Box)
        {
            m_shapeType = ShapeType::Box;
            UpdateBoxConfig(uniformScale);
        }

        else if (shapeCrc == ShapeConstants::Capsule)
        {
            m_shapeType = ShapeType::Capsule;
            UpdateCapsuleConfig(uniformScale);
        }

        else if (shapeCrc == ShapeConstants::Sphere)
        {
            m_shapeType = ShapeType::Sphere;
            UpdateSphereConfig(uniformScale);
        }

        else if (shapeCrc == ShapeConstants::PolygonPrism)
        {
            m_shapeType = ShapeType::PolygonPrism;
            UpdatePolygonPrismDecomposition();
        }

        else
        {
            m_shapeType = !shapeCrc ? ShapeType::None : ShapeType::Unsupported;
            AZ_Warning("PhysX Shape Collider Component", m_shapeTypeWarningIssued, "Unsupported shape type for "
                "entity \"%s\". The following shapes are currently supported - box, capsule, sphere, polygon prism.",
                GetEntity()->GetName().c_str());
            m_shapeTypeWarningIssued = true;
        }

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
    }

    void EditorShapeColliderComponent::UpdateBoxConfig(const AZ::Vector3& scale)
    {
        AZ::Vector3 boxDimensions = AZ::Vector3::CreateOne();
        LmbrCentral::BoxShapeComponentRequestsBus::EventResult(boxDimensions, GetEntityId(),
            &LmbrCentral::BoxShapeComponentRequests::GetBoxDimensions);
        m_shapeConfigs.emplace_back(AZStd::make_shared<Physics::BoxShapeConfiguration>(boxDimensions));
        m_shapeConfigs.back()->m_scale = scale;
        m_geometryCache.m_boxDimensions = scale * boxDimensions;
    }

    void EditorShapeColliderComponent::UpdateCapsuleConfig(const AZ::Vector3& scale)
    {
        LmbrCentral::CapsuleShapeConfig lmbrCentralCapsuleShapeConfig;
        LmbrCentral::CapsuleShapeComponentRequestsBus::EventResult(lmbrCentralCapsuleShapeConfig, GetEntityId(),
            &LmbrCentral::CapsuleShapeComponentRequests::GetCapsuleConfiguration);
        const Physics::CapsuleShapeConfiguration& capsuleShapeConfig =
            Utils::ConvertFromLmbrCentralCapsuleConfig(lmbrCentralCapsuleShapeConfig);
        m_shapeConfigs.emplace_back(AZStd::make_shared<Physics::CapsuleShapeConfiguration>(capsuleShapeConfig));
        m_shapeConfigs.back()->m_scale = scale;
        const float scalarScale = scale.GetMaxElement();
        m_geometryCache.m_radius = scalarScale * capsuleShapeConfig.m_radius;
        m_geometryCache.m_height = scalarScale * capsuleShapeConfig.m_height;
    }

    void EditorShapeColliderComponent::UpdateSphereConfig(const AZ::Vector3& scale)
    {
        float radius = 0.0f;
        LmbrCentral::SphereShapeComponentRequestsBus::EventResult(radius, GetEntityId(),
            &LmbrCentral::SphereShapeComponentRequests::GetRadius);
        m_shapeConfigs.emplace_back(AZStd::make_shared<Physics::SphereShapeConfiguration>(radius));
        m_shapeConfigs.back()->m_scale = scale;
        m_geometryCache.m_radius = scale.GetMaxElement() * radius;
    }

    void EditorShapeColliderComponent::UpdatePolygonPrismDecomposition()
    {
        m_mesh.Clear();

        AZ::PolygonPrismPtr polygonPrismPtr;
        LmbrCentral::PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(),
            &LmbrCentral::PolygonPrismShapeComponentRequests::GetPolygonPrism);

        if (polygonPrismPtr)
        {
            UpdatePolygonPrismDecomposition(polygonPrismPtr);
        }

        CreateStaticEditorCollider();
        m_mesh.SetDebugDrawDirty();
    }

    void EditorShapeColliderComponent::UpdatePolygonPrismDecomposition(const AZ::PolygonPrismPtr polygonPrismPtr)
    {
        const AZStd::vector<AZ::Vector2>& vertices = polygonPrismPtr->m_vertexContainer.GetVertices();

        // if the polygon prism vertices do not form a simple polygon, we cannot perform the decomposition
        if (!AZ::Geometry2DUtils::IsSimplePolygon(vertices))
        {
            if (!m_simplePolygonErrorIssued)
            {
                AZ_Error("PhysX Shape Collider Component", false, "Invalid polygon prism for entity \"%s\""
                    " - must be a simple polygon (no self intersection or duplicate vertices) to be represented in PhysX.",
                    GetEntity()->GetName().c_str());
                m_simplePolygonErrorIssued = true;
            }

            m_mesh.Clear();
            m_shapeConfigs.clear();
            return;
        }

        m_simplePolygonErrorIssued = false;
        size_t numFacesRemoved = 0;

        // If the polygon prism is already convex and meets the PhysX limit on convex mesh vertices/faces,
        // then we don't need to do any complicated decomposition
        if (vertices.size() <= PolygonPrismMeshUtils::MaxPolygonPrismEdges && AZ::Geometry2DUtils::IsConvex(vertices))
        {
            m_mesh.CreateFromSimpleConvexPolygon(vertices);
        }
        else
        {
            // Compute the constrained Delaunay triangulation using poly2tri
            AZStd::vector<p2t::Point> p2tVertices;
            std::vector<p2t::Point*> polyline;
            p2tVertices.reserve(vertices.size());
            polyline.reserve(vertices.size());

            int vertexIndex = 0;
            for (const AZ::Vector2& vert : vertices)
            {
                p2tVertices.push_back(p2t::Point(vert.GetX(), vert.GetY()));
                polyline.push_back(&(p2tVertices.data()[vertexIndex++]));
            }

            p2t::CDT constrainedDelaunayTriangulation(polyline);
            constrainedDelaunayTriangulation.Triangulate();
            const std::vector<p2t::Triangle*>& triangles = constrainedDelaunayTriangulation.GetTriangles();

            // Iteratively merge faces if it's possible to do so while maintaining convexity
            m_mesh.CreateFromPoly2Tri(triangles);
            numFacesRemoved = m_mesh.ConvexMerge();
        }

        // Create the cooked convex mesh configurations
        const AZStd::vector<PolygonPrismMeshUtils::Face>& faces = m_mesh.GetFaces();
        size_t numFacesTotal = faces.size();
        m_shapeConfigs.clear();
        if (numFacesRemoved <= numFacesTotal)
        {
            m_shapeConfigs.reserve(numFacesTotal - numFacesRemoved);
        }

        const float unscaledPrismHeight = polygonPrismPtr->GetHeight();

        const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());
        m_geometryCache.m_height = uniformScale.GetX() * unscaledPrismHeight;

        for (int faceIndex = 0; faceIndex < numFacesTotal; faceIndex++)
        {
            if (faces[faceIndex].m_removed)
            {
                continue;
            }

            AZStd::vector<AZ::Vector3> points;
            points.reserve(2 * faces[faceIndex].m_numEdges);
            PolygonPrismMeshUtils::HalfEdge* currentEdge = faces[faceIndex].m_edge;

            for (int edgeIndex = 0; edgeIndex < faces[faceIndex].m_numEdges; edgeIndex++)
            {
                points.emplace_back(AZ::Vector3(currentEdge->m_origin.GetX(), currentEdge->m_origin.GetY(), 0.0f));
                points.emplace_back(AZ::Vector3(currentEdge->m_origin.GetX(), currentEdge->m_origin.GetY(), unscaledPrismHeight));
                currentEdge = currentEdge->m_next;
            }

            AZStd::vector<AZ::u8> cookedData;
            bool cookingResult = false;
            PhysX::SystemRequestsBus::BroadcastResult(cookingResult, &PhysX::SystemRequests::CookConvexMeshToMemory,
                points.data(), static_cast<AZ::u32>(points.size()), cookedData);
            Physics::CookedMeshShapeConfiguration shapeConfig;
            shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(),
                Physics::CookedMeshShapeConfiguration::MeshType::Convex);
            shapeConfig.m_scale = uniformScale;

            m_shapeConfigs.push_back(AZStd::make_shared<Physics::CookedMeshShapeConfiguration>(shapeConfig));
        }
    }

    // AZ::Component
    void EditorShapeColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        PhysX::ColliderShapeRequestBus::Handler::BusConnect(GetEntityId());
        UpdateShapeConfigs();

        // Debug drawing
        m_colliderDebugDraw.Connect(GetEntityId());
        m_colliderDebugDraw.SetDisplayCallback(this);

        CreateStaticEditorCollider();

        ColliderComponentEventBus::Event(GetEntityId(), &ColliderComponentEvents::OnColliderChanged);
    }

    void EditorShapeColliderComponent::Deactivate()
    {
        m_colliderDebugDraw.Disconnect();

        PhysX::ColliderShapeRequestBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        m_editorBody = nullptr;
        Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorShapeColliderComponent::OnSelected()
    {
        PhysX::ConfigurationNotificationBus::Handler::BusConnect();
    }

    void EditorShapeColliderComponent::OnDeselected()
    {
        PhysX::ConfigurationNotificationBus::Handler::BusDisconnect();
    }

    // TransformBus
    void EditorShapeColliderComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        UpdatePolygonPrismDecomposition();
        CreateStaticEditorCollider();
        m_geometryCache.m_cachedSamplePointsDirty = true;
        ColliderComponentEventBus::Event(GetEntityId(), &ColliderComponentEvents::OnColliderChanged);
    }

    // PhysX::ConfigurationNotificationBus
    void EditorShapeColliderComponent::OnPhysXConfigurationRefreshed(const PhysXConfiguration& /*configuration*/)
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }

    void EditorShapeColliderComponent::OnDefaultMaterialLibraryChanged(const AZ::Data::AssetId& defaultMaterialLibrary)
    {
        m_colliderConfig.m_materialSelection.OnDefaultMaterialLibraryChanged(defaultMaterialLibrary);
        ColliderComponentEventBus::Event(GetEntityId(), &ColliderComponentEvents::OnColliderChanged);
    }

    // LmbrCentral::ShapeComponentNotificationBus
    void EditorShapeColliderComponent::OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        UpdateShapeConfigs();
        CreateStaticEditorCollider();
        ColliderComponentEventBus::Event(GetEntityId(), &ColliderComponentEvents::OnColliderChanged);
    }

    // DisplayCallback
    void EditorShapeColliderComponent::Display(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        // polygon prism is a special case
        if (m_shapeType == ShapeType::PolygonPrism)
        {
            AZ::PolygonPrismPtr polygonPrismPtr;
            LmbrCentral::PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(),
                &LmbrCentral::PolygonPrismShapeComponentRequests::GetPolygonPrism);
            if (polygonPrismPtr)
            {
                const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());
                const float height = polygonPrismPtr->GetHeight();
                m_colliderDebugDraw.DrawPolygonPrism(debugDisplay, m_colliderConfig, m_mesh.GetDebugDrawPoints(height,
                    uniformScale.GetX()));
            }
        }

        // for primitive shapes just display the shape configs
        else
        {
            for (const auto& shapeConfig : m_shapeConfigs)
            {
                switch (shapeConfig->GetShapeType())
                {
                case Physics::ShapeType::Box:
                {
                    const auto& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(*shapeConfig);
                    m_colliderDebugDraw.DrawBox(debugDisplay, m_colliderConfig, boxConfig, AZ::Vector3::CreateOne(), true);
                    break;
                }
                case Physics::ShapeType::Capsule:
                {
                    const auto& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(*shapeConfig);
                    m_colliderDebugDraw.DrawCapsule(debugDisplay, m_colliderConfig, capsuleConfig, AZ::Vector3::CreateOne(), true);
                    break;
                }
                case Physics::ShapeType::Sphere:
                {
                    const auto& sphereConfig = static_cast<const Physics::SphereShapeConfiguration&>(*shapeConfig);
                    m_colliderDebugDraw.DrawSphere(debugDisplay, m_colliderConfig, sphereConfig);
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    // ColliderShapeRequestBus
    AZ::Aabb EditorShapeColliderComponent::GetColliderShapeAabb()
    {
        AZ::Aabb aabb = AZ::Aabb::CreateFromPoint(GetWorldTM().GetPosition());
        LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, GetEntityId(),
            &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);
        return aabb;
    }

    bool EditorShapeColliderComponent::IsTrigger()
    {
        return m_colliderConfig.m_isTrigger;
    }
} // namespace PhysX
