﻿/*
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

#include <AzCore/Script/ScriptTimePoint.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/ColliderComponentBus.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ComponentModes/BoxComponentMode.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Editor/EditorClassConverters.h>
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <PhysX/ConfigurationBus.h>
#include <Source/BoxColliderComponent.h>
#include <Source/CapsuleColliderComponent.h>
#include <Source/EditorColliderComponent.h>
#include <Source/EditorRigidBodyComponent.h>
#include <Source/EditorSystemComponent.h>
#include <Source/MeshColliderComponent.h>
#include <Source/SphereColliderComponent.h>
#include <Source/Utils.h>
#include <Source/World.h>

#include <LyViewPaneNames.h>
#include <Editor/ConfigurationWindowBus.h>
#include <Editor/ColliderComponentMode.h>

namespace PhysX
{
    
    void EditorProxyAssetShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorProxyAssetShapeConfig>()
                ->Version(1)
                ->Field("Asset", &EditorProxyAssetShapeConfig::m_pxAsset)
                ->Field("Configuration", &EditorProxyAssetShapeConfig::m_configuration)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorProxyAssetShapeConfig>("EditorProxyShapeConfig", "PhysX Base shape collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyAssetShapeConfig::m_pxAsset, "PhysX Mesh", "PhysX mesh collider asset")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorProxyAssetShapeConfig::m_configuration, "Configuration", "Configuration of asset shape")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void EditorProxyShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        EditorProxyAssetShapeConfig::Reflect(context);

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
                            // note: we do not want the user to be able to change shape types while in ComponentMode (there will
                            // potentially be different ComponentModes for different shape types)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &AzToolsFramework::ComponentModeFramework::InComponentMode)

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
        DebugDraw::Collider::Reflect(context);

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
                ->Version(9, &PhysX::ClassConverters::UpgradeEditorColliderComponent)
                ->Field("ColliderConfiguration", &EditorColliderComponent::m_configuration)
                ->Field("ShapeConfiguration", &EditorColliderComponent::m_shapeConfiguration)
                ->Field("DebugDrawSettings", &EditorColliderComponent::m_colliderDebugDraw)
                ->Field("ComponentMode", &EditorColliderComponent::m_componentModeDelegate)
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
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/component/physx/collider")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorColliderComponent::m_configuration, "Collider Configuration", "Configuration of the collider")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorColliderComponent::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorColliderComponent::m_shapeConfiguration, "Shape Configuration", "Configuration of the shape")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorColliderComponent::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorColliderComponent::m_componentModeDelegate, "Component Mode", "Collider Component Mode")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorColliderComponent::m_colliderDebugDraw,
                        "Debug draw settings", "Debug draw settings")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    EditorColliderComponent::EditorColliderComponent(
        const Physics::ColliderConfiguration& colliderConfiguration,
        const Physics::ShapeConfiguration& shapeConfiguration)
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

    Physics::ColliderConfiguration EditorColliderComponent::GetColliderConfigurationScaled() const
    {
        // Scale the collider offset
        Physics::ColliderConfiguration colliderConfiguration = m_configuration;
        colliderConfiguration.m_position *= Utils::GetNonUniformScale(GetEntityId());
        return colliderConfiguration;
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
            m_physicsAsset.m_configuration = static_cast<const Physics::PhysicsAssetShapeConfiguration&>(shapeConfiguration);
            break;
        case Physics::ShapeType::CookedMesh:
            m_cookedMesh = static_cast<const Physics::CookedMeshShapeConfiguration&>(shapeConfiguration);
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
        return const_cast<Physics::ShapeConfiguration&>(static_cast<const EditorProxyShapeConfig&>(*this).GetCurrent());
    }

    const Physics::ShapeConfiguration& EditorProxyShapeConfig::GetCurrent() const
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
            return m_physicsAsset.m_configuration;
        case Physics::ShapeType::CookedMesh:
            return m_cookedMesh;
        default:
            AZ_Warning("EditorProxyShapeConfig", false, "Unsupported shape type");
            return m_box;
        }
    }

    void EditorColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        PhysX::MeshColliderComponentRequestsBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::BoxManipulatorRequestBus::Handler::BusConnect(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));
        ColliderShapeRequestBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
        EditorColliderComponentRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(GetEntityId(), GetId()));

        AZ::TransformBus::EventResult(m_cachedWorldTransform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        // Debug drawing
        m_colliderDebugDraw.Connect(GetEntityId());
        m_colliderDebugDraw.SetDisplayCallback(this);

        // ComponentMode
        m_componentModeDelegate.ConnectWithSingleComponentMode<
            EditorColliderComponent, ColliderComponentMode>(
                AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);

        bool usingMaterialsFromAsset = IsAssetConfig() ? m_shapeConfiguration.m_physicsAsset.m_configuration.m_useMaterialsFromAsset : false;
        m_configuration.m_materialSelection.SetSlotsReadOnly(usingMaterialsFromAsset);

        if (ShouldUpdateCollisionMeshFromRender())
        {
            SetCollisionMeshFromRender();
        }

        UpdateMeshAsset();
        UpdateShapeConfigurationScale();
        CreateStaticEditorCollider();

        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }

    void EditorColliderComponent::Deactivate()
    {
        Physics::WorldBodyRequestBus::Handler::BusDisconnect();
        m_colliderDebugDraw.Disconnect();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        LmbrCentral::MeshComponentNotificationBus::Handler::BusDisconnect();
        ColliderShapeRequestBus::Handler::BusDisconnect();
        AzToolsFramework::BoxManipulatorRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        PhysX::MeshColliderComponentRequestsBus::Handler::BusDisconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);

        m_componentModeDelegate.Disconnect();

        m_editorBody = nullptr;
    }

    AZ::u32 EditorColliderComponent::OnConfigurationChanged()
    {
        if (m_shapeConfiguration.IsAssetConfig())
        {
            UpdateMeshAsset();
            m_configuration.m_materialSelection.SetSlotsReadOnly(m_shapeConfiguration.m_physicsAsset.m_configuration.m_useMaterialsFromAsset);
        }
        else
        {
            m_configuration.m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
            m_configuration.m_materialSelection.SetSlotsReadOnly(false);
        }

        // ensure we refresh the ComponentMode (and Manipulators) when the configuration
        // changes to keep the ComponentMode in sync with the shape (otherwise the manipulators
        // will move out of alignment with the shape)
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::Refresh,
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));

        UpdateShapeConfigurationScale();
        CreateStaticEditorCollider();

        m_colliderDebugDraw.ClearCachedGeometry();

        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
        Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorColliderComponent::OnSelected()
    {
        PhysX::ConfigurationNotificationBus::Handler::BusConnect();
    }

    void EditorColliderComponent::OnDeselected()
    {
        PhysX::ConfigurationNotificationBus::Handler::BusDisconnect();
    }

    void EditorColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto sharedColliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>(m_configuration);
        BaseColliderComponent* colliderComponent = nullptr;

        switch (m_shapeConfiguration.m_shapeType)
        {
        case Physics::ShapeType::Sphere:
            colliderComponent = gameEntity->CreateComponent<SphereColliderComponent>();
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(sharedColliderConfig,
                AZStd::make_shared<Physics::SphereShapeConfiguration>(m_shapeConfiguration.m_sphere)) });
            break;
        case Physics::ShapeType::Box:
            colliderComponent = gameEntity->CreateComponent<BoxColliderComponent>();
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(sharedColliderConfig,
                AZStd::make_shared<Physics::BoxShapeConfiguration>(m_shapeConfiguration.m_box)) });
            break;
        case Physics::ShapeType::Capsule:
            colliderComponent = gameEntity->CreateComponent<CapsuleColliderComponent>();
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(sharedColliderConfig,
                AZStd::make_shared<Physics::CapsuleShapeConfiguration>(m_shapeConfiguration.m_capsule)) });
            break;
        case Physics::ShapeType::PhysicsAsset:
            colliderComponent = gameEntity->CreateComponent<MeshColliderComponent>();
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(sharedColliderConfig,
                AZStd::make_shared<Physics::PhysicsAssetShapeConfiguration>(m_shapeConfiguration.m_physicsAsset.m_configuration)) });

            AZ_Warning("PhysX", m_shapeConfiguration.m_physicsAsset.m_pxAsset.GetId().IsValid(),
                "EditorColliderComponent::BuildGameEntity. No asset assigned to Collider Component. Entity: %s",
                GetEntity()->GetName().c_str());
            break;
        case Physics::ShapeType::CookedMesh:
            colliderComponent = gameEntity->CreateComponent<BaseColliderComponent>();
            colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(sharedColliderConfig,
                AZStd::make_shared<Physics::CookedMeshShapeConfiguration>(m_shapeConfiguration.m_cookedMesh)) });
            break;
        default:
            AZ_Warning("EditorColliderComponent", false, "Unsupported shape type for building game entity!")
        }

        StaticRigidBodyUtils::TryCreateRuntimeComponent(*GetEntity(), *gameEntity);
    }

    AZ::Transform EditorColliderComponent::GetColliderLocalTransform() const
    {
        const AZ::Vector3 nonUniformScale = Utils::GetNonUniformScale(GetEntityId());
        return AZ::Transform::CreateFromQuaternionAndTranslation(
            m_configuration.m_rotation, m_configuration.m_position * nonUniformScale);
    }

    void EditorColliderComponent::UpdateMeshAsset()
    {
        if (m_shapeConfiguration.m_physicsAsset.m_pxAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_shapeConfiguration.m_physicsAsset.m_pxAsset.GetId());
            m_shapeConfiguration.m_physicsAsset.m_pxAsset.QueueLoad();
            m_shapeConfiguration.m_physicsAsset.m_configuration.m_asset = m_shapeConfiguration.m_physicsAsset.m_pxAsset;
            m_colliderDebugDraw.ClearCachedGeometry();
        }

        UpdateMaterialSlotsFromMeshAsset();
    }

    void EditorColliderComponent::CreateStaticEditorCollider()
    {
        // Don't create static rigid body in the editor if current entity components
        // don't allow creation of runtime static rigid body component
        if (!StaticRigidBodyUtils::CanCreateRuntimeComponent(*GetEntity()))
        {
            return;
        }

        if (m_shapeConfiguration.IsAssetConfig() && m_shapeConfiguration.m_physicsAsset.m_pxAsset.GetStatus() != AZ::Data::AssetData::AssetStatus::Ready)
        {
            // Mesh asset has not been loaded, wait for OnAssetReady to be invoked.
            // We specifically check Ready state here rather than ReadyPreNotify to ensure OnAssetReady has been invoked
            m_editorBody.reset();
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

            m_editorBody = AZ::Interface<Physics::System>::Get()->CreateStaticRigidBody(configuration);

            if (m_shapeConfiguration.IsAssetConfig())
            {
                AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
                Utils::GetShapesFromAsset(m_shapeConfiguration.m_physicsAsset.m_configuration, m_configuration, shapes);
                
                for (const auto& shape : shapes)
                {
                    m_editorBody->AddShape(shape);
                }
            }
            else
            {
                const Physics::ColliderConfiguration colliderConfiguration = GetColliderConfigurationScaled();

                const auto& shapeConfiguration = m_shapeConfiguration.GetCurrent();
                AZStd::shared_ptr<Physics::Shape> shape = AZ::Interface<Physics::System>::Get()->CreateShape(
                    colliderConfiguration, shapeConfiguration);

                m_editorBody->AddShape(shape);
            }

            editorWorld->AddBody(*m_editorBody);
        }

        m_colliderDebugDraw.ClearCachedGeometry();

        Physics::WorldBodyRequestBus::Handler::BusConnect(GetEntityId());
    }

    AZ::Data::Asset<Pipeline::MeshAsset> EditorColliderComponent::GetMeshAsset() const
    {
        return m_shapeConfiguration.m_physicsAsset.m_pxAsset;
    }

    void EditorColliderComponent::GetStaticWorldSpaceMeshTriangles(AZStd::vector<AZ::Vector3>& verts, AZStd::vector<AZ::u32>& indices) const
    {
        bool isStatic = false;
        AZ::TransformBus::EventResult(isStatic, m_entity->GetId(), &AZ::TransformBus::Events::IsStaticTransform);

        if (isStatic)
        {
            AZ_Warning("PhysX", verts.empty() && indices.empty(), 
                "Existing vertex and index data will be lost. Is this a duplicate model instance?");

            BuildDebugDrawMesh();

            AZ::u32 numShapes = m_colliderDebugDraw.GetNumShapes();
            AZ_Warning("PhysX", numShapes > 0, 
                "GetStaticWorldSpaceMeshTriangles: No debug draw geometry found. "
                "Please make sure the collision geometry is loaded. Entity: %s",
                GetEntity()->GetName().c_str());

            indices.clear();
            verts.clear();

            AZ::Transform overallTM = GetWorldTM() * GetColliderLocalTransform();

            // Vertices can be added directly but indices need to be incremented
            // by the amount of vertices already added in the last iteration
            for (AZ::u32 shapeIndex = 0; shapeIndex < numShapes; shapeIndex++)
            {
                const AZStd::vector<AZ::u32>& shapeIndices = m_colliderDebugDraw.GetIndices(shapeIndex);
                const AZStd::vector<AZ::Vector3>& shapeVerts = m_colliderDebugDraw.GetVerts(shapeIndex);

                AZ::u32 startingIndex = static_cast<AZ::u32>(verts.size());
                
                indices.reserve(indices.size() + shapeIndices.size());

                AZStd::transform(shapeIndices.begin(), shapeIndices.end(), AZStd::back_inserter(indices), 
                    [startingIndex](AZ::u32 index)
                {
                    return index + startingIndex;
                });

                verts.reserve(verts.size() + shapeVerts.size());

                AZStd::transform(shapeVerts.begin(), shapeVerts.end(), AZStd::back_inserter(verts), 
                    [&overallTM](const AZ::Vector3& vertex)
                {
                    return overallTM * vertex;
                });
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
            m_shapeConfiguration.m_physicsAsset.m_pxAsset.Create(id);
            UpdateMeshAsset();
            m_colliderDebugDraw.ClearCachedGeometry();
        }
    }

    void EditorColliderComponent::SetMaterialAsset(const AZ::Data::AssetId& id)
    {
        m_configuration.m_materialSelection.SetMaterialLibrary(id);
    }

    void EditorColliderComponent::SetMaterialId(const Physics::MaterialId& id)
    {
        m_configuration.m_materialSelection.SetMaterialId(id);
    }

    void EditorColliderComponent::UpdateMaterialSlotsFromMeshAsset()
    {
        Physics::SystemRequestBus::Broadcast(&Physics::SystemRequests::UpdateMaterialSelection,
            m_shapeConfiguration.GetCurrent(), m_configuration);

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);

        ValidateMaterialSurfaces();
    }

    void EditorColliderComponent::ValidateMaterialSurfaces()
    {
        const AZ::Data::Asset<Pipeline::MeshAsset>& physicsAsset = m_shapeConfiguration.m_physicsAsset.m_pxAsset;

        if (!IsAssetConfig() || !physicsAsset.IsReady())
        {
            return;
        }

        // Here we check the material indices assigned to every shape and validate that every index is used at least once.
        // It's not an error if the validation fails here but something we want to let the designers know about.
        size_t surfacesNum = physicsAsset->m_assetData.m_surfaceNames.size();
        const AZStd::vector<AZ::u16>& indexPerShape = physicsAsset->m_assetData.m_materialIndexPerShape;

        AZStd::unordered_set<AZ::u16> usedIndices;

        for (AZ::u16 index : indexPerShape)
        {
            if (index == Pipeline::MeshAssetData::TriangleMeshMaterialIndex)
            {
                // Triangle mesh indices are cooked into binary data, pass the validation in this case.
                return;
            }

            usedIndices.insert(index);
        }

        AZ_Warning("PhysX", usedIndices.size() == surfacesNum,
            "EditorColliderComponent::ValidateMaterialSurfaces. Entity: %s. Number of surfaces used by the shape (%d) does not match the "
            "total number of surfaces in the asset (%d). Please check that there are no convex meshes with per-face materials. Asset: %s",
            GetEntity()->GetName().c_str(), usedIndices.size(), surfacesNum, physicsAsset.GetHint().c_str())
    }

    void EditorColliderComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_shapeConfiguration.m_physicsAsset.m_pxAsset)
        {
            m_shapeConfiguration.m_physicsAsset.m_pxAsset = asset;
            m_shapeConfiguration.m_physicsAsset.m_configuration.m_asset = m_shapeConfiguration.m_physicsAsset.m_pxAsset;

            UpdateMaterialSlotsFromMeshAsset();
            CreateStaticEditorCollider();

            // Invalidate debug draw cached data
            m_colliderDebugDraw.ClearCachedGeometry();

            // Notify about the data update of the collider
            Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
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

    void EditorColliderComponent::BuildDebugDrawMesh() const
    {
        if (m_shapeConfiguration.IsAssetConfig())
        {
            const AZ::Data::Asset<Pipeline::MeshAsset>& physicsAsset = m_shapeConfiguration.m_physicsAsset.m_pxAsset;
            const Physics::PhysicsAssetShapeConfiguration& physicsAssetConfiguration = m_shapeConfiguration.m_physicsAsset.m_configuration;

            if (!physicsAsset.IsReady())
            {
                // Skip processing if the asset isn't ready
                return;
            }

            Physics::ShapeConfigurationList shapeConfigList;
            Utils::GetColliderShapeConfigsFromAsset(physicsAssetConfiguration, m_configuration, shapeConfigList);

            for (size_t shapeIndex = 0; shapeIndex < shapeConfigList.size(); shapeIndex++)
            {
                const Physics::ShapeConfiguration* shapeConfiguration = shapeConfigList[shapeIndex].second.get();
                AZ_Assert(shapeConfiguration, "BuildDebugDrawMesh: Invalid shape configuration");

                if (shapeConfiguration)
                {
                    m_colliderDebugDraw.BuildMeshes(*shapeConfiguration, shapeIndex);
                }
            }
        }
        else
        {
            const AZ::u32 shapeIndex = 0; // There's only one mesh gets built from the primitive collider, hence use geomIndex 0.
            m_colliderDebugDraw.BuildMeshes(m_shapeConfiguration.GetCurrent(), shapeIndex);
        }
    }


    void EditorColliderComponent::DisplayPrimitiveCollider(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        switch (m_shapeConfiguration.m_shapeType)
        {
        case Physics::ShapeType::Sphere:
            m_colliderDebugDraw.DrawSphere(debugDisplay, m_configuration, m_shapeConfiguration.m_sphere);
            break;
        case Physics::ShapeType::Box:
            m_colliderDebugDraw.DrawBox(debugDisplay, m_configuration, m_shapeConfiguration.m_box);
            break;
        case Physics::ShapeType::Capsule:
            m_colliderDebugDraw.DrawCapsule(debugDisplay, m_configuration, m_shapeConfiguration.m_capsule);
            break;
        }
    }

    void EditorColliderComponent::DisplayMeshCollider(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        if (!m_colliderDebugDraw.HasCachedGeometry())
        {
            return;
        }

        const AZ::Data::Asset<Pipeline::MeshAsset>& physicsAsset = m_shapeConfiguration.m_physicsAsset.m_pxAsset;
        const Physics::PhysicsAssetShapeConfiguration& physicsAssetConfiguration = m_shapeConfiguration.m_physicsAsset.m_configuration;

        Physics::ShapeConfigurationList shapeConfigList;
        Utils::GetColliderShapeConfigsFromAsset(physicsAssetConfiguration, m_configuration, shapeConfigList);

        const AZ::Vector3& assetScale = physicsAssetConfiguration.m_assetScale;

        for (size_t shapeIndex = 0; shapeIndex < shapeConfigList.size(); shapeIndex++)
        {
            const Physics::ColliderConfiguration* colliderConfiguration = shapeConfigList[shapeIndex].first.get();
            const Physics::ShapeConfiguration* shapeConfiguration = shapeConfigList[shapeIndex].second.get();

            AZ_Assert(shapeConfiguration && colliderConfiguration, "DisplayMeshCollider: Invalid shape-collider configuration pair");

            switch (shapeConfiguration->GetShapeType())
            {
            case Physics::ShapeType::CookedMesh:
            {
                const Physics::CookedMeshShapeConfiguration* cookedMeshShapeConfiguration =
                    static_cast<const Physics::CookedMeshShapeConfiguration*>(shapeConfiguration);

                const AZ::Vector3 nonUniformScale = Utils::GetNonUniformScale(GetEntityId());

                m_colliderDebugDraw.DrawMesh(debugDisplay, *colliderConfiguration, *cookedMeshShapeConfiguration,
                    assetScale * nonUniformScale, shapeIndex);
                break;
            }
            case Physics::ShapeType::Sphere:
            {
                const Physics::SphereShapeConfiguration* sphereShapeConfiguration =
                    static_cast<const Physics::SphereShapeConfiguration*>(shapeConfiguration);

                m_colliderDebugDraw.DrawSphere(debugDisplay, *colliderConfiguration, *sphereShapeConfiguration, assetScale);
                break;
            }
            case Physics::ShapeType::Box:
            {
                const Physics::BoxShapeConfiguration* boxShapeConfiguration =
                    static_cast<const Physics::BoxShapeConfiguration*>(shapeConfiguration);

                m_colliderDebugDraw.DrawBox(debugDisplay, *colliderConfiguration, *boxShapeConfiguration, assetScale);
                break;
            }
            case Physics::ShapeType::Capsule:
            {
                const Physics::CapsuleShapeConfiguration* capsuleShapeConfiguration =
                    static_cast<const Physics::CapsuleShapeConfiguration*>(shapeConfiguration);

                m_colliderDebugDraw.DrawCapsule(debugDisplay, *colliderConfiguration, *capsuleShapeConfiguration, assetScale);
                break;
            }
            default:
            {
                AZ_Error("EditorColliderComponent", false, "DisplayMeshCollider: Unsupported ShapeType %d. Entity %s, ID: %llu",
                    static_cast<AZ::u32>(shapeConfiguration->GetShapeType()), GetEntity()->GetName().c_str(), GetEntityId());
                break;
            }
            }
        }
    }

    void EditorColliderComponent::Display(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        if (!m_colliderDebugDraw.HasCachedGeometry())
        {
            BuildDebugDrawMesh();
        }

        if (m_colliderDebugDraw.HasCachedGeometry())
        {
            if (m_shapeConfiguration.IsAssetConfig())
            {
                DisplayMeshCollider(debugDisplay);
            }
            else
            {
                DisplayPrimitiveCollider(debugDisplay);
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
        return GetColliderWorldTransform();
    }

    AZ::Vector3 EditorColliderComponent::GetBoxScale()
    {
        return GetWorldTM().RetrieveScale();
    }

    void EditorColliderComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (world.IsClose(m_cachedWorldTransform))
        {
            return;
        }
        m_cachedWorldTransform = world;

        UpdateShapeConfigurationScale();
        CreateStaticEditorCollider();
        Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
    }

    // PhysX::ConfigurationNotificationBus
    void EditorColliderComponent::OnPhysXConfigurationRefreshed(const PhysXConfiguration& /*configuration*/)
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }

    void EditorColliderComponent::OnDefaultMaterialLibraryChanged(const AZ::Data::AssetId& defaultMaterialLibrary)
    {
        m_configuration.m_materialSelection.OnDefaultMaterialLibraryChanged(defaultMaterialLibrary);
    }

    // PhysX::ColliderShapeBus
    AZ::Aabb EditorColliderComponent::GetColliderShapeAabb()
    {
        return PhysX::Utils::GetColliderAabb(GetWorldTM()
            , m_shapeConfiguration.GetCurrent()
            , m_configuration);
    }

    void EditorColliderComponent::UpdateShapeConfigurationScale()
    {
        auto& shapeConfiguration = m_shapeConfiguration.GetCurrent();
        shapeConfiguration.m_scale = GetWorldTM().ExtractScale();
        m_colliderDebugDraw.ClearCachedGeometry();
    }

    void EditorColliderComponent::EnablePhysics()
    {
        if (!IsPhysicsEnabled())
        {
            CreateStaticEditorCollider();
        }
    }

    void EditorColliderComponent::DisablePhysics()
    {
        m_editorBody.reset();
    }

    bool EditorColliderComponent::IsPhysicsEnabled() const
    {
        return m_editorBody && m_editorBody->GetWorld();
    }

    AZ::Aabb EditorColliderComponent::GetAabb() const
    {
        if (m_editorBody)
        {
            return m_editorBody->GetAabb();
        }
        return AZ::Aabb::CreateNull();
    }

    Physics::WorldBody* EditorColliderComponent::GetWorldBody()
    {
        return m_editorBody.get();
    }

    Physics::RayCastHit EditorColliderComponent::RayCast(const Physics::RayCastRequest& request)
    {
        if (m_editorBody)
        {
            return m_editorBody->RayCast(request);
        }
        return Physics::RayCastHit();
    }

    bool EditorColliderComponent::IsTrigger()
    {
        return m_configuration.m_isTrigger;
    }

    void EditorColliderComponent::SetColliderOffset(const AZ::Vector3& offset)
    {
        m_configuration.m_position = offset;
        CreateStaticEditorCollider();
    }

    AZ::Vector3 EditorColliderComponent::GetColliderOffset()
    {
        return m_configuration.m_position;
    }

    void EditorColliderComponent::SetColliderRotation(const AZ::Quaternion& rotation)
    {
        m_configuration.m_rotation = rotation;
        CreateStaticEditorCollider();
    }

    AZ::Quaternion EditorColliderComponent::GetColliderRotation()
    {
        return m_configuration.m_rotation;
    }

    AZ::Transform EditorColliderComponent::GetColliderWorldTransform()
    {
        return AzToolsFramework::TransformNormalizedScale(GetWorldTM()) * GetColliderLocalTransform();
    }

    bool EditorColliderComponent::ShouldUpdateCollisionMeshFromRender() const
    {
        if (!m_shapeConfiguration.IsAssetConfig())
        {
            return false;
        }

        bool collisionMeshNotSet = !m_shapeConfiguration.m_physicsAsset.m_pxAsset.GetId().IsValid();
        return collisionMeshNotSet;
    }

    AZ::Data::AssetId EditorColliderComponent::FindMatchingPhysicsAsset(
        const AZ::Data::Asset<AZ::Data::AssetData>& renderMeshAsset,
        const AZStd::vector<AZ::Data::AssetId>& physicsAssets)
    {
        AZ::Data::AssetId foundAssetId;

        // Extract the file name from the path to the asset
        AZStd::string renderMeshFileName;
        AzFramework::StringFunc::Path::Split(renderMeshAsset.GetHint().c_str(),
            nullptr, nullptr, &renderMeshFileName);

        // Find the collision mesh asset matching the render mesh
        for (const AZ::Data::AssetId& assetId : physicsAssets)
        {
            AZStd::string assetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath,
                &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetId);

            AZStd::string physicsAssetFileName;
            AzFramework::StringFunc::Path::Split(assetPath.c_str(), nullptr, nullptr, &physicsAssetFileName);

            if (physicsAssetFileName == renderMeshFileName)
            {
                foundAssetId = assetId;
                break;
            }
        }

        return foundAssetId;
    };

    void EditorColliderComponent::SetCollisionMeshFromRender()
    {
        AZ::Data::Asset<AZ::Data::AssetData> renderMeshAsset;
        LmbrCentral::MeshComponentRequestBus::EventResult(renderMeshAsset, 
            GetEntityId(), &LmbrCentral::MeshComponentRequests::GetMeshAsset);

        if (!renderMeshAsset.GetId().IsValid())
        {
            return;
        }

        bool productsQueryResult = false;
        AZStd::vector<AZ::Data::AssetInfo> productsInfo;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(productsQueryResult,
            &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetsProducedBySourceUUID, 
            renderMeshAsset.GetId().m_guid, productsInfo);

        if (productsQueryResult)
        {
            AZStd::vector<AZ::Data::AssetId> physicsAssets;
            physicsAssets.reserve(productsInfo.size());

            for (const AZ::Data::AssetInfo& info : productsInfo)
            {
                if (info.m_assetType == AZ::AzTypeInfo<Pipeline::MeshAsset>::Uuid())
                {
                    physicsAssets.push_back(info.m_assetId);
                }
            }

            // If there's only one physics asset, we set it regardless of the name
            if (physicsAssets.size() == 1)
            {
                SetMeshAsset(physicsAssets[0]);
            }
            // For multiple assets we pick the one matching the name of the render mesh asset
            else if (physicsAssets.size() > 1)
            {
                AZ::Data::AssetId matchingPhysicsAsset = FindMatchingPhysicsAsset(renderMeshAsset, physicsAssets);
                    
                if (matchingPhysicsAsset.IsValid())
                {
                    SetMeshAsset(matchingPhysicsAsset);
                }
                else
                {
                    AZ_Warning("EditorColliderComponent", false,
                        "SetCollisionMeshFromRender on entity %s: Unable to find a matching physics asset "
                        "for the render mesh asset GUID: %s, hint: %s",
                        GetEntity()->GetName().c_str(),
                        renderMeshAsset.GetId().m_guid.ToString<AZStd::string>().c_str(),
                        renderMeshAsset.GetHint().c_str());
                }
            }
            // This is not necessarily an incorrect case but it's worth reporting 
            // in case if we forgot to configure the source asset to produce the collision mesh
            else if (physicsAssets.empty())
            {
                AZ_TracePrintf("EditorColliderComponent",
                    "SetCollisionMeshFromRender on entity %s: The source asset for %s did not produce any physics assets",
                    GetEntity()->GetName().c_str(),
                    renderMeshAsset.GetHint().c_str());
            }
        }
        else
        {
            AZ_Warning("EditorColliderComponent", false, 
                "SetCollisionMeshFromRender on entity %s: Unable to get the assets produced by the render mesh asset GUID: %s, hint: %s",
                GetEntity()->GetName().c_str(), 
                renderMeshAsset.GetId().m_guid.ToString<AZStd::string>().c_str(),
                renderMeshAsset.GetHint().c_str());
        }
    }
    
    void EditorColliderComponent::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        AZ_UNUSED(asset);

        if (ShouldUpdateCollisionMeshFromRender())
        {
            SetCollisionMeshFromRender();
        }
    }

    void EditorColliderComponent::SetShapeType(Physics::ShapeType shapeType)
    {
        m_shapeConfiguration.m_shapeType = shapeType;
        CreateStaticEditorCollider();
    }

    Physics::ShapeType EditorColliderComponent::GetShapeType()
    {
        return m_shapeConfiguration.GetCurrent().GetShapeType();
    }

    void EditorColliderComponent::SetSphereRadius(float radius)
    {
        m_shapeConfiguration.m_sphere.m_radius = radius;
        CreateStaticEditorCollider();
    }

    float EditorColliderComponent::GetSphereRadius()
    {
        return m_shapeConfiguration.m_sphere.m_radius;
    }

    void EditorColliderComponent::SetCapsuleRadius(float radius)
    {
        m_shapeConfiguration.m_capsule.m_radius = radius;
        CreateStaticEditorCollider();
    }

    float EditorColliderComponent::GetCapsuleRadius()
    {
        return m_shapeConfiguration.m_capsule.m_radius;
    }

    void EditorColliderComponent::SetCapsuleHeight(float height)
    {
        m_shapeConfiguration.m_capsule.m_height = height;
        CreateStaticEditorCollider();
    }

    float EditorColliderComponent::GetCapsuleHeight()
    {
        return m_shapeConfiguration.m_capsule.m_height;
    }

    void EditorColliderComponent::SetAssetScale(const AZ::Vector3& scale)
    {
        m_shapeConfiguration.m_physicsAsset.m_configuration.m_assetScale = scale;
        CreateStaticEditorCollider();
    }

    AZ::Vector3 EditorColliderComponent::GetAssetScale()
    {
        return m_shapeConfiguration.m_physicsAsset.m_configuration.m_assetScale;
    }
} // namespace PhysX
