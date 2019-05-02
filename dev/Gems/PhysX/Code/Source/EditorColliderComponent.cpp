
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
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Script/ScriptTimePoint.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <PhysX/ConfigurationBus.h>
#include <Source/EditorSystemComponent.h>
#include <Source/EditorColliderComponent.h>
#include <Source/SphereColliderComponent.h>
#include <Source/BoxColliderComponent.h>
#include <Source/CapsuleColliderComponent.h>
#include <Source/MeshColliderComponent.h>
#include <Source/EditorRigidBodyComponent.h>
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>
#include <Editor/EditorClassConverters.h>
#include <Source/Utils.h>
#include <Source/World.h>
#include <AzFramework/Physics/RigidBody.h>
#include <PhysX/EditorRigidBodyRequestBus.h>

namespace PhysX
{
    const AZ::u32 MaxTriangles = 16384 * 3;
    const AZ::u32 MaxTrianglesRange = 800;

    void EditorProxyShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorProxyShapeConfig>()
                ->Version(2, &PhysX::ClassConverters::EditorProxyShapeConfigVersionConverter)
                ->Field("ShapeType", &EditorProxyShapeConfig::m_shapeType)
                ->Field("Sphere", &EditorProxyShapeConfig::m_sphere)
                ->Field("Box", &EditorProxyShapeConfig::m_box)
                ->Field("Capsule", &EditorProxyShapeConfig::m_capsule)
                ->Field("PhysicsAsset", &EditorProxyShapeConfig::m_physicsAsset)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorProxyShapeConfig>(
                    "EditorProxyShapeConfig", "PhysX Base shape collider")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorProxyShapeConfig::m_shapeType, "Shape", "The shape of the collider")
                    ->EnumAttribute(Physics::ShapeType::Sphere, "Sphere")
                    ->EnumAttribute(Physics::ShapeType::Box, "Box")
                    ->EnumAttribute(Physics::ShapeType::Capsule, "Capsule")
                    ->EnumAttribute(Physics::ShapeType::PhysicsAsset, "PhysicsAsset")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_sphere, "Sphere", "Configuration of sphere shape")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::IsSphereConfig)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyShapeConfig::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_box, "Box", "Configuration of box shape")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::IsBoxConfig)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyShapeConfig::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_capsule, "Capsule", "Configuration of capsule shape")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::IsCapsuleConfig)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyShapeConfig::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyShapeConfig::m_physicsAsset, "Asset", "Configuration of asset shape")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorProxyShapeConfig::IsAssetConfig)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorProxyShapeConfig::OnConfigurationChanged)
                ;
            }
        }
    }

    AZ::u32 EditorProxyShapeConfig::OnConfigurationChanged()
    {
        Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    void EditorColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorProxyShapeConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate old separate components
            serializeContext->ClassDeprecate(
                "EditorCapsuleColliderComponent",
                "{0BD5AF3A-35C0-4386-9930-54A2A3E97432}",
                &ClassConverters::DeprecateEditorCapsuleColliderComponent)
            ;

            serializeContext->ClassDeprecate(
                "EditorBoxColliderComponent",
                "{FAECF2BE-625B-469D-BBFF-E345BBB12D66}",
                &ClassConverters::DeprecateEditorBoxColliderComponent)
            ;

            serializeContext->ClassDeprecate(
                "EditorSphereColliderComponent",
                "{D11C1624-4AE9-4B66-A6F6-40EDB9CDCE99}",
                &ClassConverters::DeprecateEditorSphereColliderComponent)
            ;

            serializeContext->ClassDeprecate(
                "EditorMeshColliderComponent",
                "{214185DA-ABD9-4410-9819-7C177801CF7A}",
                &ClassConverters::DeprecateEditorMeshColliderComponent)
            ;

            serializeContext->Class<EditorColliderComponent, EditorComponentBase>()
                ->Version(4, &PhysX::ClassConverters::UpgradeEditorColliderComponent)
                ->Field("ColliderConfiguration", &EditorColliderComponent::m_configuration)
                ->Field("ShapeConfiguration", &EditorColliderComponent::m_shapeConfiguration)
                ->Field("MeshAsset", &EditorColliderComponent::m_meshColliderAsset)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorColliderComponent>(
                    "PhysX Collider", "PhysX shape collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PhysXCollider.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/PhysXCollider.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/physx-base-collider")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorColliderComponent::m_configuration, "Collider Configuration", "Configuration of the collider")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorColliderComponent::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorColliderComponent::m_shapeConfiguration, "Shape Configuration", "Configuration of the shape")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorColliderComponent::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorColliderComponent::m_meshColliderAsset, "PxMesh", "PhysX mesh collider asset")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorColliderComponent::IsAssetConfig)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorColliderComponent::OnConfigurationChanged)
                ;
            }
        }
    }

    EditorColliderComponent::EditorColliderComponent(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration)
        : m_configuration(colliderConfiguration)
        , m_shapeConfiguration(shapeConfiguration)
    {
    }

    const EditorProxyShapeConfig& EditorColliderComponent::GetShapeConfiguration() const
    {
        return m_shapeConfiguration;
    }

    const Physics::ColliderConfiguration& EditorColliderComponent::GetColliderConfiguration() const
    {
        return m_configuration;
    }

    EditorProxyShapeConfig::EditorProxyShapeConfig(const Physics::ShapeConfiguration& shapeConfiguration)
    {
        m_shapeType = shapeConfiguration.GetShapeType();
        switch (m_shapeType)
        {
        case Physics::ShapeType::Sphere:
            m_sphere = static_cast<const Physics::SphereShapeConfiguration&>(shapeConfiguration);
            break;
        case Physics::ShapeType::Box:
            m_box = static_cast<const Physics::BoxShapeConfiguration&>(shapeConfiguration);
            break;
        case Physics::ShapeType::Capsule:
            m_capsule = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfiguration);
            break;
        case Physics::ShapeType::PhysicsAsset:
            m_physicsAsset = static_cast<const Physics::PhysicsAssetShapeConfiguration&>(shapeConfiguration);
            break;
        default:
            AZ_Warning("EditorProxyShapeConfig", false, "Invalid shape type!");
        }
    }

    bool EditorProxyShapeConfig::IsSphereConfig() const
    {
        return m_shapeType == Physics::ShapeType::Sphere;
    }

    bool EditorProxyShapeConfig::IsBoxConfig() const
    {
        return m_shapeType == Physics::ShapeType::Box;
    }

    bool EditorProxyShapeConfig::IsCapsuleConfig() const
    {
        return m_shapeType == Physics::ShapeType::Capsule;
    }

    bool EditorProxyShapeConfig::IsAssetConfig() const
    {
        return m_shapeType == Physics::ShapeType::PhysicsAsset;
    }

    Physics::ShapeConfiguration& EditorProxyShapeConfig::GetCurrent()
    {
        switch (m_shapeType)
        {
        case Physics::ShapeType::Sphere:
            return m_sphere;
        case Physics::ShapeType::Box:
            return m_box;
        case Physics::ShapeType::Capsule:
            return m_capsule;
        case Physics::ShapeType::PhysicsAsset:
            return m_physicsAsset;
        default:
            AZ_Warning("EditorProxyShapeConfig", false, "Unsupported shape type");
            return m_box;
        }
    }

    void EditorColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        PhysX::MeshColliderComponentRequestsBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

        m_boxManipulator.SetHandler(this);
        UpdateMeshAsset();

        m_warningColor = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);

        m_triangleCollisionMeshColor = AZ::Color(0.5f, 0.5f, 0.5f, 0.7f);
        m_convexCollisionMeshColor = AZ::Color(0.5f, 0.5f, 0.5f, 0.7f);
        m_wireFrameColor = AZ::Color(0.0f, 0.0f, 0.0f, 0.7f);

        UpdateShapeConfigurationScale();
        CreateStaticEditorCollider();

        PhysX::EditorRigidBodyRequestBus::Event(GetEntityId(), &PhysX::EditorRigidBodyRequests::RefreshEditorRigidBody);
    }

    void EditorColliderComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        m_boxManipulator.SetHandler(nullptr);
        m_boxManipulator.OnDeselect();

        m_editorBody = nullptr;
    }

    AZ::u32 EditorColliderComponent::OnConfigurationChanged()
    {
        if (m_shapeConfiguration.IsBoxConfig())
        {
            m_boxManipulator.OnSelect(GetEntityId());
            m_configuration.m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
        }
        else if (m_shapeConfiguration.IsAssetConfig())
        {
            UpdateMeshAsset();
            m_boxManipulator.OnDeselect();
        }
        else
        {
            m_boxManipulator.OnDeselect();
            m_configuration.m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
        }

        UpdateShapeConfigurationScale();
        CreateStaticEditorCollider();

        m_meshDirty = true;
        PhysX::EditorRigidBodyRequestBus::Event(GetEntityId(), &PhysX::EditorRigidBodyRequests::RefreshEditorRigidBody);

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    void EditorColliderComponent::OnSelected()
    {
        if (m_shapeConfiguration.IsBoxConfig())
        {
            m_boxManipulator.OnSelect(GetEntityId());
        }

        AZ::TickBus::Handler::BusConnect();
        PhysX::ConfigurationNotificationBus::Handler::BusConnect();
    }

    void EditorColliderComponent::OnDeselected()
    {
        m_boxManipulator.OnDeselect();
        PhysX::ConfigurationNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void EditorColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        switch (m_shapeConfiguration.m_shapeType)
        {
        case Physics::ShapeType::Sphere:
            gameEntity->CreateComponent<SphereColliderComponent>(m_configuration, m_shapeConfiguration.m_sphere);
            break;
        case Physics::ShapeType::Box:
            gameEntity->CreateComponent<BoxColliderComponent>(m_configuration, m_shapeConfiguration.m_box);
            break;
        case Physics::ShapeType::Capsule:
            gameEntity->CreateComponent<CapsuleColliderComponent>(m_configuration, m_shapeConfiguration.m_capsule);
            break;
        case Physics::ShapeType::PhysicsAsset:
            gameEntity->CreateComponent<MeshColliderComponent>(m_configuration, m_shapeConfiguration.m_physicsAsset);
            break;
        }
    }

    AZ::Transform EditorColliderComponent::GetColliderTransform() const
    {
        return AZ::Transform::CreateFromQuaternionAndTranslation(m_configuration.m_rotation, m_configuration.m_position * GetNonUniformScale());
    }

    float EditorColliderComponent::GetUniformScale() const
    {
        return GetNonUniformScale().GetMaxElement();
    }

    AZ::Vector3 EditorColliderComponent::GetNonUniformScale() const
    {
        return GetWorldTM().RetrieveScale();
    }

    void EditorColliderComponent::UpdateMeshAsset()
    {
        if (m_meshColliderAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_meshColliderAsset.GetId());
            m_meshColliderAsset.QueueLoad();
            m_shapeConfiguration.m_physicsAsset.m_asset = m_meshColliderAsset;
            m_meshDirty = true;
        }

        UpdateMaterialSlotsFromMeshAsset();
    }

    void EditorColliderComponent::CreateStaticEditorCollider()
    {
        // We're ignoring dynamic bodies in the editor for now
        auto rigidBody = GetEntity()->FindComponent<PhysX::EditorRigidBodyComponent>();
        if (rigidBody)
        {
            return;
        }

        if (m_shapeConfiguration.IsAssetConfig() && m_meshColliderAsset.GetStatus() != AZ::Data::AssetData::AssetStatus::Ready)
        {
            // Mesh asset has not been loaded, wait for OnAssetReady to be invoked.
            // We specifically check Ready state here rather than ReadyPreNotify to ensure OnAssetReady has been invoked
            return;
        }

        AZStd::shared_ptr<Physics::World> editorWorld;
        Physics::EditorWorldBus::BroadcastResult(editorWorld, &Physics::EditorWorldRequests::GetEditorWorld);
        if (editorWorld != nullptr)
        {
            AZ::Transform colliderTransform = GetWorldTM();
            AZ::Vector3 scale = colliderTransform.ExtractScale();

            Physics::WorldBodyConfiguration configuration;
            configuration.m_orientation = AZ::Quaternion::CreateFromTransform(colliderTransform);
            configuration.m_position = colliderTransform.GetPosition();
            configuration.m_entityId = GetEntityId();
            configuration.m_debugName = GetEntity()->GetName();

            Physics::SystemRequestBus::BroadcastResult(m_editorBody, &Physics::SystemRequests::CreateStaticRigidBody, configuration);

            AZStd::shared_ptr<Physics::Shape> shape;
            auto& shapeConfiguration = m_shapeConfiguration.GetCurrent();
            Physics::SystemRequestBus::BroadcastResult(shape, &Physics::SystemRequests::CreateShape, m_configuration, shapeConfiguration);

            m_editorBody->AddShape(shape);
            editorWorld->AddBody(*m_editorBody);
        }

        m_meshDirty = true;
    }

    AZ::Data::Asset<Pipeline::MeshAsset> EditorColliderComponent::GetMeshAsset() const
    {
        return m_meshColliderAsset;
    }


    void EditorColliderComponent::GetStaticWorldSpaceMeshTriangles(AZStd::vector<AZ::Vector3>& verts, AZStd::vector<AZ::u32>& indices) const
    {
        bool isStatic = false;
        AZ::TransformBus::EventResult(isStatic, m_entity->GetId(), &AZ::TransformBus::Events::IsStaticTransform);

        if (isStatic)
        {
            AZ_Warning("PhysX", verts.empty() && indices.empty(), "Existing vertex and index data will be lost. Is this a duplicate model instance?");

            // We have to rebuild the mesh in case the world matrix changes.
            BuildMeshes();

            verts.assign(m_verts.begin(), m_verts.end());
            indices.assign(m_indices.begin(), m_indices.end());

            for (auto& vert : verts)
            {
                vert = GetColliderTransform() * vert;
                vert = GetWorldTM() * vert;
            }
        }
    }

    Physics::MaterialId EditorColliderComponent::GetMaterialId() const
    {
        return m_configuration.m_materialSelection.GetMaterialId();
    }


    void EditorColliderComponent::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        if (id.IsValid())
        {
            m_shapeConfiguration.m_shapeType = Physics::ShapeType::PhysicsAsset;
            m_meshColliderAsset.Create(id);
            UpdateMeshAsset();
            m_meshDirty = true;
        }
    }

    void EditorColliderComponent::SetMaterialAsset(const AZ::Data::AssetId& id)
    {
        m_configuration.m_materialSelection.CreateMaterialLibrary(id);
    }

    void EditorColliderComponent::SetMaterialId(const Physics::MaterialId& id)
    {
        m_configuration.m_materialSelection.SetMaterialId(id);
    }

    void EditorColliderComponent::UpdateMaterialSlotsFromMeshAsset()
    {
        Physics::MaterialSelection::SlotsArray materialSlots;
        if (m_meshColliderAsset.GetId().IsValid())
        {
            if (m_meshColliderAsset.IsReady())
            {
                if (auto meshAsset = m_meshColliderAsset.Get())
                {
                    m_configuration.m_materialSelection.SetMaterialSlots(meshAsset->GetMaterialSlots());
                }
                else
                {
                    m_configuration.m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
                    AZ_Warning("PhysX", false, "EditorColliderComponent: MeshAsset is invalid");
                }
            }
        }
        else
        {
            m_configuration.m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
        }
    }

    void EditorColliderComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_meshColliderAsset)
        {
            m_meshColliderAsset = asset;
            m_shapeConfiguration.m_physicsAsset.m_asset = m_meshColliderAsset;

            UpdateMaterialSlotsFromMeshAsset();
            CreateStaticEditorCollider();
        }
        else
        {
            m_configuration.m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
        }
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
    }

    void EditorColliderComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void EditorColliderComponent::DisplayEntity(bool& handled)
    {
        // Let each collider decide how to scale itself, so extract the scale here.
        AZ::Transform entityWorldTransformWithoutScale = GetWorldTM();
        entityWorldTransformWithoutScale.ExtractScale();

        PhysX::Configuration globalConfiguration;
        PhysX::ConfigurationRequestBus::BroadcastResult(globalConfiguration, &ConfigurationRequests::GetConfiguration);
        PhysX::Settings::ColliderProximityVisualization& colliderProximityVisualization = globalConfiguration.m_settings.m_colliderProximityVisualization;
        float distance = colliderProximityVisualization.m_cameraPosition.GetDistance(entityWorldTransformWithoutScale.GetPosition());
        bool colliderIsInRange = distance < colliderProximityVisualization.m_radius;

        if (m_configuration.m_visible || (colliderProximityVisualization.m_enabled && colliderIsInRange))
        {
            AzFramework::EntityDebugDisplayRequests* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
            AZ_Assert(displayContext, "Invalid display context.");

            displayContext->PushMatrix(entityWorldTransformWithoutScale);
            Display(*displayContext);
            displayContext->PopMatrix();
            handled = true;
        }
    }

    void EditorColliderComponent::Display(AzFramework::EntityDebugDisplayRequests& displayContext)
    {
        BuildMeshes();

        switch (m_shapeConfiguration.m_shapeType)
        {
        case Physics::ShapeType::Sphere:
            DrawSphere(displayContext, m_shapeConfiguration.m_sphere);
            break;
        case Physics::ShapeType::Box:
            DrawBox(displayContext, m_shapeConfiguration.m_box);
            break;
        case Physics::ShapeType::Capsule:
            DrawCapsule(displayContext, m_shapeConfiguration.m_capsule);
            break;
        case Physics::ShapeType::PhysicsAsset:
            DrawMesh(displayContext);
            break;
        }
    }

    void EditorColliderComponent::DrawSphere(AzFramework::EntityDebugDisplayRequests& displayContext, const Physics::SphereShapeConfiguration& config)
    {
        AZ::Transform scaleMatrix = AZ::Transform::CreateScale(AZ::Vector3(GetUniformScale()));
        displayContext.PushMatrix(GetColliderTransform() * scaleMatrix);
        displayContext.SetColor(AzFramework::ViewportColors::DeselectedColor);
        displayContext.DrawBall(AZ::Vector3::CreateZero(), config.m_radius);
        displayContext.SetColor(AzFramework::ViewportColors::WireColor);
        displayContext.DrawWireSphere(AZ::Vector3::CreateZero(), config.m_radius);
        displayContext.PopMatrix();
    }

    void EditorColliderComponent::DrawBox(AzFramework::EntityDebugDisplayRequests& displayContext, const Physics::BoxShapeConfiguration& config)
    {
        AZ::Transform scaleMatrix = AZ::Transform::CreateScale(GetNonUniformScale());
        displayContext.PushMatrix(GetColliderTransform() * scaleMatrix);
        displayContext.SetColor(AzFramework::ViewportColors::DeselectedColor);
        displayContext.DrawSolidBox(-config.m_dimensions * 0.5f, config.m_dimensions * 0.5f);
        displayContext.SetColor(AzFramework::ViewportColors::WireColor);
        displayContext.DrawWireBox(-config.m_dimensions * 0.5f, config.m_dimensions * 0.5f);
        displayContext.PopMatrix();
    }

    void EditorColliderComponent::DrawCapsule(AzFramework::EntityDebugDisplayRequests& displayContext, const Physics::CapsuleShapeConfiguration& config)
    {
        displayContext.PushMatrix(GetColliderTransform());
        AZ::Vector3 scale = GetCapsuleScale();

        LmbrCentral::CapsuleGeometrySystemRequestBus::Broadcast(
            &LmbrCentral::CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
            config.m_radius * scale.GetX(),
            config.m_height * scale.GetZ(),
            16, 8,
            m_verts,
            m_indices,
            m_points
            );

        displayContext.DrawTrianglesIndexed(m_verts, m_indices, AzFramework::ViewportColors::DeselectedColor);
        displayContext.DrawLines(m_points, AzFramework::ViewportColors::WireColor);
        displayContext.SetLineWidth(m_colliderVisualizationLineWidth);
        displayContext.PopMatrix();
    }

    void EditorColliderComponent::UpdateColliderMeshColor(AZ::Color& baseColor, AZ::u32 triangleCount) const
    {
        if (triangleCount > MaxTriangles)
        {
            float curTime = m_time;
            int icurTime = (int)curTime;
            float alpha = sinf(((icurTime & 1) + (curTime - icurTime) * (1 - (icurTime & 1) * 2)) * AZ::Constants::Pi * 0.5f);
            alpha *= AZStd::GetMin((float)MaxTrianglesRange, AZStd::GetMax(0.0f, (float)triangleCount - MaxTriangles)) / (float)MaxTriangles;
            baseColor = m_triangleCollisionMeshColor * (1.0f - alpha) + AZ::Color((float)m_warningColor.GetR(), 0.0f, 0.0f, (float)m_triangleCollisionMeshColor.GetR()) * alpha;
        }
    }

    void EditorColliderComponent::DrawTriangleMesh(AzFramework::EntityDebugDisplayRequests& displayContext, physx::PxBase* meshData) const
    {
        if (!m_verts.empty())
        {
            AZ::u64 triangleCount = m_indices.size() / 3;
            UpdateColliderMeshColor(m_convexCollisionMeshColor, triangleCount);
            displayContext.DrawTrianglesIndexed(m_verts, m_indices, m_convexCollisionMeshColor);
            displayContext.DrawLines(m_points, m_wireFrameColor);
        }
    }

    void EditorColliderComponent::DrawConvexMesh(AzFramework::EntityDebugDisplayRequests& displayContext, physx::PxBase* meshData) const
    {
        if (!m_verts.empty())
        {
            AZ::u64 triangleCount = m_verts.size() / 3;
            UpdateColliderMeshColor(m_triangleCollisionMeshColor, triangleCount);
            displayContext.DrawTriangles(m_verts, m_triangleCollisionMeshColor);
            displayContext.DrawLines(m_points, m_wireFrameColor);
        }
    }

    void EditorColliderComponent::DrawMesh(AzFramework::EntityDebugDisplayRequests& displayContext)
    {
        if (m_meshColliderAsset && m_meshColliderAsset.IsReady())
        {
            AZ::Transform scaleMatrix = AZ::Transform::CreateScale(GetNonUniformScale() * m_shapeConfiguration.m_physicsAsset.m_assetScale);
            displayContext.PushMatrix(GetColliderTransform() * scaleMatrix);

            physx::PxBase* meshData = m_meshColliderAsset.Get()->GetMeshData();

            if (meshData)
            {
                if (meshData->is<physx::PxTriangleMesh>())
                {
                    DrawTriangleMesh(displayContext, meshData);
                }
                else
                {
                    DrawConvexMesh(displayContext, meshData);
                }
            }

            displayContext.PopMatrix();
        }
    }

    void EditorColliderComponent::BuildMeshes() const
    {
        if (m_meshDirty)
        {
            m_verts.clear();
            m_indices.clear();
            m_points.clear();

            switch (m_shapeConfiguration.m_shapeType)
            {
            case Physics::ShapeType::Sphere:
            {
                // TODO: Have a geometry function like GenerateCapsuleMesh. Currently, AABB geometry will be close enough for exporting editor collision purposes
                AZ::Vector3 boxMax(m_shapeConfiguration.m_sphere.m_radius,
                    m_shapeConfiguration.m_sphere.m_radius,
                    m_shapeConfiguration.m_sphere.m_radius);
                boxMax *= m_shapeConfiguration.m_sphere.m_scale;
                AZ::Vector3 boxMin = boxMax;
                boxMin *= -1.f;
                BuildAABBVerts(boxMin, boxMax);
                m_meshDirty = false;
            }
            break;
            case Physics::ShapeType::Box:
                m_shapeConfiguration.m_box.m_dimensions;
                BuildAABBVerts(-m_shapeConfiguration.m_box.m_dimensions * (m_shapeConfiguration.m_box.m_scale * 0.5f),
                    m_shapeConfiguration.m_box.m_dimensions * (m_shapeConfiguration.m_box.m_scale * 0.5f));
                m_meshDirty = false;
                break;
            case Physics::ShapeType::Capsule:
                LmbrCentral::CapsuleGeometrySystemRequestBus::Broadcast(
                    &LmbrCentral::CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
                    m_shapeConfiguration.m_capsule.m_radius * m_shapeConfiguration.m_capsule.m_scale.GetX(),
                    m_shapeConfiguration.m_capsule.m_height * m_shapeConfiguration.m_capsule.m_scale.GetZ(),
                    16, 8,
                    m_verts,
                    m_indices,
                    m_points
                    );
                m_meshDirty = false;
                break;
            case Physics::ShapeType::PhysicsAsset:
                if (m_meshColliderAsset && m_meshColliderAsset.IsReady())
                {
                    physx::PxBase* meshData = m_meshColliderAsset.Get()->GetMeshData();

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
        }
    }

    void EditorColliderComponent::BuildAABBVerts(const AZ::Vector3& boxMin, const AZ::Vector3& boxMax) const
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

    void EditorColliderComponent::BuildTriangleMesh(physx::PxBase* meshData) const
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
        }
    }

    void EditorColliderComponent::BuildConvexMesh(physx::PxBase* meshData) const
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


    bool EditorColliderComponent::IsAssetConfig() const
    {
        return m_shapeConfiguration.IsAssetConfig();
    }

    AZ::Vector3 EditorColliderComponent::GetDimensions()
    {
        return m_shapeConfiguration.m_box.m_dimensions;
    }

    void EditorColliderComponent::SetDimensions(const AZ::Vector3& dimensions)
    {
        m_shapeConfiguration.m_box.m_dimensions = dimensions;
        CreateStaticEditorCollider();
    }

    AZ::Transform EditorColliderComponent::GetCurrentTransform()
    {
        return GetWorldTM() * GetColliderTransform();
    }

    AZ::Vector3 EditorColliderComponent::GetCapsuleScale()
    {
        AZ_Warning("EditorColliderComponent", m_shapeConfiguration.IsCapsuleConfig(), "Calling GetCapsuleScale for non capsule config");
        AZ::Vector3 nonUniformScale = GetNonUniformScale();
        // Capsule radius scales with max of X and Y
        // Capsule height scales with Z
        float scaleX = nonUniformScale.GetX().GetMax(nonUniformScale.GetY());
        float scaleY = scaleX;
        float scaleZ = nonUniformScale.GetZ();
        return AZ::Vector3(scaleX, scaleY, scaleZ);
    }

    // TickBus::Handler
    void EditorColliderComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        m_time = time.GetSeconds();
    }

    void EditorColliderComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        UpdateShapeConfigurationScale();
        m_boxManipulator.RefreshManipulators();
        CreateStaticEditorCollider();
        Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
    }

    // PhysX::ConfigurationNotificationBus
    void EditorColliderComponent::OnConfigurationRefreshed(const Configuration& configuration)
    {
        AZ_UNUSED(configuration);
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }

    void EditorColliderComponent::UpdateShapeConfigurationScale()
    {
        auto& shapeConfiguration = m_shapeConfiguration.GetCurrent();
        shapeConfiguration.m_scale = GetWorldTM().ExtractScale();
        m_meshDirty = true;
    }
}

