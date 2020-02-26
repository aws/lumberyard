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
    }

    void EditorShapeColliderComponent::CreateStaticEditorCollider()
    {
        // We're ignoring dynamic bodies in the editor for now
        auto rigidBody = GetEntity()->FindComponent<PhysX::EditorRigidBodyComponent>();
        if (rigidBody)
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

        AZ::Crc32 shapeCrc;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(shapeCrc, GetEntityId(),
            &LmbrCentral::ShapeComponentRequests::GetShapeType);

        // using if blocks because switch statements aren't supported for values generated by the crc macro
        if (shapeCrc == ShapeConstants::Box)
        {
            m_shapeType = ShapeType::Box;
            UpdateBoxConfig(GetUniformScale());
        }

        else if (shapeCrc == ShapeConstants::Capsule)
        {
            m_shapeType = ShapeType::Capsule;
            UpdateCapsuleConfig(GetUniformScale());
        }

        else if (shapeCrc == ShapeConstants::Sphere)
        {
            m_shapeType = ShapeType::Sphere;
            UpdateSphereConfig(GetUniformScale());
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

    AZ::Vector3 EditorShapeColliderComponent::GetUniformScale()
    {
        // all currently supported shape types scale uniformly based on the largest element of the non-uniform scale
        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::TransformBus::EventResult(nonUniformScale, GetEntityId(), &AZ::TransformBus::Events::GetLocalScale);
        return AZ::Vector3(nonUniformScale.GetMaxElement());
    }

    void EditorShapeColliderComponent::UpdateBoxConfig(const AZ::Vector3& scale)
    {
        AZ::Vector3 boxDimensions = AZ::Vector3::CreateOne();
        LmbrCentral::BoxShapeComponentRequestsBus::EventResult(boxDimensions, GetEntityId(),
            &LmbrCentral::BoxShapeComponentRequests::GetBoxDimensions);
        m_shapeConfigs.emplace_back(AZStd::make_shared<Physics::BoxShapeConfiguration>(boxDimensions));
        m_shapeConfigs.back()->m_scale = scale;
    }

    void EditorShapeColliderComponent::UpdateCapsuleConfig(const AZ::Vector3& scale)
    {
        LmbrCentral::CapsuleShapeConfig lmbrCentralCapsuleShapeConfig;
        LmbrCentral::CapsuleShapeComponentRequestsBus::EventResult(lmbrCentralCapsuleShapeConfig, GetEntityId(),
            &LmbrCentral::CapsuleShapeComponentRequests::GetCapsuleConfiguration);
        m_shapeConfigs.emplace_back(AZStd::make_shared<Physics::CapsuleShapeConfiguration>(
            Utils::ConvertFromLmbrCentralCapsuleConfig(lmbrCentralCapsuleShapeConfig)));
        m_shapeConfigs.back()->m_scale = scale;
    }

    void EditorShapeColliderComponent::UpdateSphereConfig(const AZ::Vector3& scale)
    {
        float radius = 0.0f;
        LmbrCentral::SphereShapeComponentRequestsBus::EventResult(radius, GetEntityId(),
            &LmbrCentral::SphereShapeComponentRequests::GetRadius);
        m_shapeConfigs.emplace_back(AZStd::make_shared<Physics::SphereShapeConfiguration>(radius));
        m_shapeConfigs.back()->m_scale = scale;
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
        const float height = polygonPrismPtr->GetHeight();

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
                points.emplace_back(AZ::Vector3(currentEdge->m_origin.GetX(), currentEdge->m_origin.GetY(), height));
                currentEdge = currentEdge->m_next;
            }

            AZStd::vector<AZ::u8> cookedData;
            bool cookingResult = false;
            PhysX::SystemRequestsBus::BroadcastResult(cookingResult, &PhysX::SystemRequests::CookConvexMeshToMemory,
                points.data(), static_cast<AZ::u32>(points.size()), cookedData);
            Physics::CookedMeshShapeConfiguration shapeConfig;
            shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(),
                Physics::CookedMeshShapeConfiguration::MeshType::Convex);
            shapeConfig.m_scale = m_scale;

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

        m_scale = AZ::Vector3(GetTransform()->GetLocalScale().GetMaxElement());
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
        m_scale = AZ::Vector3(GetTransform()->GetLocalScale().GetMaxElement());
        UpdatePolygonPrismDecomposition();
        CreateStaticEditorCollider();
        ColliderComponentEventBus::Event(GetEntityId(), &ColliderComponentEvents::OnColliderChanged);
    }

    // PhysX::ConfigurationNotificationBus
    void EditorShapeColliderComponent::OnConfigurationRefreshed(const Configuration& /*configuration*/)
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
                const float height = polygonPrismPtr->GetHeight();
                m_colliderDebugDraw.DrawPolygonPrism(debugDisplay, m_colliderConfig, m_mesh.GetDebugDrawPoints(height,
                    m_scale.GetMaxElement()));
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
                    m_colliderDebugDraw.DrawBox(debugDisplay, m_colliderConfig, boxConfig, boxConfig.m_scale);
                    break;
                }
                case Physics::ShapeType::Capsule:
                {
                    const auto& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(*shapeConfig);
                    m_colliderDebugDraw.DrawCapsule(debugDisplay, m_colliderConfig, capsuleConfig, capsuleConfig.m_scale);
                    break;
                }
                case Physics::ShapeType::Sphere:
                {
                    const auto& sphereConfig = static_cast<const Physics::SphereShapeConfiguration&>(*shapeConfig);
                    m_colliderDebugDraw.DrawSphere(debugDisplay, m_colliderConfig, sphereConfig, sphereConfig.m_scale);
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
