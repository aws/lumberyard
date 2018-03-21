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


#include "EMotionFX_precompiled.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <LmbrCentral/Physics/CryCharacterPhysicsBus.h>

#include <Integration/Editor/Components/EditorActorComponent.h>
#include <Integration/AnimGraphComponentBus.h>

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/NodeSelectionWindow.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>

#include <MathConversion.h>
#include <IRenderAuxGeom.h>

//#pragma optimize("",off)

namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorActorComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(1)
                    ->Field("ActorAsset", &EditorActorComponent::m_actorAsset)
                    ->Field("MaterialPerLOD", &EditorActorComponent::m_materialPerLOD)
                    ->Field("AttachmentType", &EditorActorComponent::m_attachmentType)
                    ->Field("AttachmentTarget", &EditorActorComponent::m_attachmentTarget)
                    ->Field("AttachmentTargetJoint", &EditorActorComponent::m_attachmentJointName)
                    ->Field("AttachmentJointIndex", &EditorActorComponent::m_attachmentJointIndex)
                    ->Field("RenderSkeleton", &EditorActorComponent::m_renderSkeleton)
                    ->Field("RenderCharacter", &EditorActorComponent::m_renderCharacter)
                    ->Field("SkinningMethod", &EditorActorComponent::m_skinningMethod)
                ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorActorComponent>("Actor", "The Actor component manages an instance of an Actor")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Animation")
                        ->Attribute(AZ::Edit::Attributes::Icon, ":/EMotionFX/ActorComponent.png")
                        ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<ActorAsset>::Uuid())
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, ":/EMotionFX/ActorComponent.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(0, &EditorActorComponent::m_actorAsset,
                        "Actor asset", "Assigned actor asset")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnAssetSelected)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute("EditButton", "Gems/EMotionFX/Assets/Editor/Images/Icons/EMFX_icon_32x32")
                        ->Attribute("EditDescription", "Open in Animation Editor")
                        ->Attribute("EditCallback", &EditorActorComponent::LaunchAnimationEditor)
                        ->DataElement(0, &EditorActorComponent::m_materialPerLOD,
                        "LOD Materials", "Material assignment for each LOD level")
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnMaterialChanged)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Options")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(0, &EditorActorComponent::m_renderCharacter,
                        "Draw character", "Toggles rendering of character mesh.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnDebugDrawFlagChanged)
                        ->DataElement(0, &EditorActorComponent::m_renderSkeleton,
                        "Draw skeleton", "Toggles rendering of skeleton.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnDebugDrawFlagChanged)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorActorComponent::m_skinningMethod,
                        "Skinning method", "Choose the skinning method this actor is using")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnSkinningMethodChanged)
                        ->EnumAttribute(SkinningMethod::DualQuat, "Dual quat skinning")
                        ->EnumAttribute(SkinningMethod::Linear, "Linear skinning")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Attach To")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorActorComponent::m_attachmentType,
                        "Attachment type", "Type of attachment to use when attaching to the target entity.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnAttachmentTypeChanged)
                        ->EnumAttribute(AttachmentType::None, "None")
                    // ActorAttachment is disabled in 1.11 because it doesn't work as intended. We are encouraging user to use AttachmentComponent instead.
                    // In the future, ActorAttachment will be replaced with AttachmentComponent when AttachmentComponent also handles actor instance's attachment update.
                    //->EnumAttribute(AttachmentType::ActorAttachment, "Actor attachment")
                        ->EnumAttribute(AttachmentType::SkinAttachment, "Skin attachment")
                        ->DataElement(0, &EditorActorComponent::m_attachmentTarget,
                        "Target entity", "Entity Id whose actor instance we should attach to.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("EMotionFXActorService", 0xd6e8f48d))
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorActorComponent::AttachmentTargetVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnAttachmentTargetChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Button, &EditorActorComponent::m_attachmentJointName,
                        "Target joint", "Joint on target entity to which to attach.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorActorComponent::AttachmentTargetJointVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnAttachmentTargetJointSelect)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, &EditorActorComponent::AttachmentJointButtonText)
                    ;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        EditorActorComponent::EditorActorComponent()
            : m_renderCharacter(true)
            , m_renderSkeleton(false)
            , m_skinningMethod(SkinningMethod::DualQuat)
            , m_attachmentType(AttachmentType::None)
            , m_attachmentJointIndex(0)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        EditorActorComponent::~EditorActorComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::Init()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::Activate()
        {
            CreateActorInstance();

            ActorComponentRequestBus::Handler::BusConnect(GetEntityId());
            EditorActorComponentRequestBus::Handler::BusConnect(GetEntityId());
            LmbrCentral::MeshComponentRequestBus::Handler::BusConnect(GetEntityId());
            LmbrCentral::RenderNodeRequestBus::Handler::BusConnect(GetEntityId());
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::Deactivate()
        {
            LmbrCentral::RenderNodeRequestBus::Handler::BusDisconnect();
            LmbrCentral::MeshComponentRequestBus::Handler::BusDisconnect();
            EditorActorComponentRequestBus::Handler::BusDisconnect();
            ActorComponentRequestBus::Handler::BusDisconnect();

            AZ::TransformNotificationBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusDisconnect();

            DestroyActorInstance();
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::CreateActorInstance()
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();

            // Queue actor asset load. Instantiation occurs in OnAssetReady.
            if (m_actorAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::Handler::BusConnect(m_actorAsset.GetId());
                m_actorAsset.QueueLoad();
            }
            else
            {
                DestroyActorInstance();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::DestroyActorInstance()
        {
            if (m_actorInstance)
            {
                ActorComponentNotificationBus::Event(
                    GetEntityId(),
                    &ActorComponentNotificationBus::Events::OnActorInstanceDestroyed,
                    m_actorInstance.get());
            }

            m_actorInstance = nullptr;
            m_renderNode = nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        // ActorComponentRequestBus::Handler
        //////////////////////////////////////////////////////////////////////////
        const AZ::Data::AssetId& EditorActorComponent::GetActorAssetId()
        {
            return m_actorAsset.GetId();
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        AZ::Crc32 EditorActorComponent::OnAssetSelected()
        {
            CreateActorInstance();

            if (!m_actorAsset.GetId().IsValid())
            {
                m_materialPerLOD.clear();

                return AZ::Edit::PropertyRefreshLevels::EntireTree;
            }

            return AZ::Edit::PropertyRefreshLevels::None;
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnMaterialChanged()
        {
            if (m_renderNode)
            {
                m_renderNode->SetMaterials(m_materialPerLOD);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnDebugDrawFlagChanged()
        {
            if (m_renderSkeleton)
            {
                AZ::TickBus::Handler::BusConnect();
            }
            else
            {
                AZ::TickBus::Handler::BusDisconnect();
            }

            if (m_renderNode)
            {
                m_renderNode->Hide(!m_renderCharacter);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnSkinningMethodChanged()
        {
            if (m_renderNode)
            {
                m_renderNode->SetSkinningMethod(m_skinningMethod);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        bool EditorActorComponent::AttachmentTargetVisibility()
        {
            return (m_attachmentType != AttachmentType::None);
        }

        //////////////////////////////////////////////////////////////////////////
        bool EditorActorComponent::AttachmentTargetJointVisibility()
        {
            return (m_attachmentType == AttachmentType::ActorAttachment);
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string EditorActorComponent::AttachmentJointButtonText()
        {
            return m_attachmentJointName.empty() ?
                   AZStd::string("(No joint selected)") : m_attachmentJointName;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Crc32 EditorActorComponent::OnAttachmentTypeChanged()
        {
            if (m_attachmentType == AttachmentType::None)
            {
                m_attachmentTarget.SetInvalid();
                m_attachmentJointName.clear();
            }
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Crc32 EditorActorComponent::OnAttachmentTargetChanged()
        {
            if (!IsValidAttachment(GetEntityId(), m_attachmentTarget))
            {
                m_attachmentTarget.SetInvalid();
                AZ_Error("EMotionFX", false, "You cannot attach to yourself or create circular dependencies! Attachment cannot be performed.");
            }
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Crc32 EditorActorComponent::OnAttachmentTargetJointSelect()
        {
            // Grab actor instance and invoke UI for joint selection.
            EMotionFXPtr<EMotionFX::ActorInstance> actorInstance;
            ActorComponentRequestBus::EventResult(
                actorInstance,
                m_attachmentTarget,
                &ActorComponentRequestBus::Events::GetActorInstance);

            AZ::Crc32 refreshLevel = AZ::Edit::PropertyRefreshLevels::None;

            if (actorInstance)
            {
                EMStudio::NodeSelectionWindow* nodeSelectWindow = new EMStudio::NodeSelectionWindow(nullptr, true);
                nodeSelectWindow->setWindowTitle(nodeSelectWindow->tr("Select Target Joint"));

                CommandSystem::SelectionList selection;

                // If a joint was previously selected, ensure it's pre-selected in the UI.
                if (!m_attachmentJointName.empty())
                {
                    EMotionFX::Node* node = actorInstance->GetActor()->GetSkeleton()->FindNodeByName(m_attachmentJointName.c_str());
                    if (node)
                    {
                        selection.AddNode(node);
                    }
                }

                QObject::connect(nodeSelectWindow, &EMStudio::NodeSelectionWindow::accepted,
                    [this, nodeSelectWindow, &refreshLevel, &actorInstance]()
                    {
                        auto& selectedItems = nodeSelectWindow->GetNodeHierarchyWidget()->GetSelectedItems();
                        if (!selectedItems.GetIsEmpty())
                        {
                            const char* jointName = selectedItems[0].GetNodeName();
                            EMotionFX::Node* node = actorInstance->GetActor()->GetSkeleton()->FindNodeByName(jointName);
                            if (node)
                            {
                                m_attachmentJointName = jointName;
                                m_attachmentJointIndex = node->GetNodeIndex();

                                refreshLevel = AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
                            }
                        }
                    });

                nodeSelectWindow->Update(actorInstance->GetID(), &selection);
                nodeSelectWindow->exec();
                delete nodeSelectWindow;
            }

            return refreshLevel;
        }

        void EditorActorComponent::LaunchAnimationEditor(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType&)
        {
            if (assetId.IsValid())
            {
                AZ::Data::AssetId animgraphAssetId;
                animgraphAssetId.SetInvalid();
                EditorAnimGraphComponentRequestBus::EventResult(animgraphAssetId, GetEntityId(), &EditorAnimGraphComponentRequestBus::Events::GetAnimGraphAssetId);
                AZ::Data::AssetId motionSetAssetId;
                motionSetAssetId.SetInvalid();
                EditorAnimGraphComponentRequestBus::EventResult(motionSetAssetId, GetEntityId(), &EditorAnimGraphComponentRequestBus::Events::GetMotionSetAssetId);

                // call to open must be done before LoadCharacter
                const char* panelName = EMStudio::MainWindow::GetEMotionFXPaneName();
                EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, OpenViewPane, panelName);

                EMStudio::MainWindow* mainWindow = EMStudio::GetMainWindow();
                if (mainWindow)
                {
                    mainWindow->LoadCharacter(assetId, animgraphAssetId, motionSetAssetId);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            if (asset == m_actorAsset)
            {
                OnAssetReady(asset);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_actorAsset = asset;

            // Enable/disable debug drawing.
            OnDebugDrawFlagChanged();

            // Create actor instance.
            auto* actorAsset = m_actorAsset.GetAs<ActorAsset>();
            AZ_Error("EMotionFX", actorAsset, "Actor asset is not valid.");
            if (!actorAsset)
            {
                return;
            }

            if (m_actorInstance)
            {
                ActorComponentNotificationBus::Event(
                    GetEntityId(),
                    &ActorComponentNotificationBus::Events::OnActorInstanceDestroyed,
                    m_actorInstance.get());
            }

            m_actorInstance = actorAsset->CreateInstance(GetEntityId());
            if (!m_actorInstance)
            {
                AZ_Error("EMotionFX", actorAsset, "Failed to create actor instance.");
                return;
            }

            // If we are loading the actor for the first time, automatically add the material
            // per lod information. If the amount of lods between different actors that are assigned
            // to this component differ, then reinit the materials.
            if (m_materialPerLOD.size() != actorAsset->GetActor()->GetNumLODLevels())
            {
                InitializeMaterialSlots(*actorAsset);
            }

            // Assign entity Id to user data field, so we can extract owning entity from an EMFX actor pointer.
            m_actorInstance->SetCustomData(reinterpret_cast<void*>(static_cast<AZ::u64>(GetEntityId())));

            // Notify listeners that an actor instance has been created.
            ActorComponentNotificationBus::Event(
                GetEntityId(),
                &ActorComponentNotificationBus::Events::OnActorInstanceCreated,
                m_actorInstance.get());

            // Setup initial transform and listen for transform changes.
            AZ::Transform transform;
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            OnTransformChanged(transform, transform);
            AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

            // Force an update of node transforms so we can get an accurate bounding box.
            m_actorInstance->UpdateTransformations(0.f, true, false);

            m_renderNode = AZStd::make_unique<ActorRenderNode>(GetEntityId(), m_actorInstance, m_actorAsset, transform);
            m_renderNode->SetMaterials(m_materialPerLOD);
            m_renderNode->RegisterWithRenderer(true);
            m_renderNode->Hide(!m_renderCharacter);
            m_renderNode->SetSkinningMethod(m_skinningMethod);

            // Send general mesh creation notification to interested parties.
            LmbrCentral::MeshComponentNotificationBus::Event(GetEntityId(), &LmbrCentral::MeshComponentNotifications::OnMeshCreated, m_actorAsset);
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::InitializeMaterialSlots(ActorAsset& actorAsset)
        {
            // Set appropriate LOD material length.
            m_materialPerLOD.resize(actorAsset.GetActor()->GetNumLODLevels());

            // If a material exists next to the actor, pre-initialize LOD material slots with that material.
            // This is merely an accelerator for the user, and is isolated to tools-only code (the editor actor component).
            AZStd::string materialAssetPath;
            EBUS_EVENT_RESULT(materialAssetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, actorAsset.GetId());
            if (!materialAssetPath.empty())
            {
                // Query the catalog for a material of the same name as the actor.
                AzFramework::StringFunc::Path::ReplaceExtension(materialAssetPath, "mtl");
                AZ::Data::AssetId materialAssetId;
                EBUS_EVENT_RESULT(materialAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, materialAssetPath.c_str(), AZ::Data::s_invalidAssetType, false);

                // If found, initialize all empty material slots with the material.
                if (materialAssetId.IsValid())
                {
                    for (auto& materialPath : m_materialPerLOD)
                    {
                        if (materialPath.GetAssetPath().empty())
                        {
                            materialPath.SetAssetPath(materialAssetPath.c_str());
                        }
                    }
                }
            }

            using namespace AzToolsFramework;
            ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::InvalidatePropertyDisplay, Refresh_EntireTree);
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
        {
            (void)local;

            if (m_actorInstance)
            {
                // We're currently not using EMotionFX's internal Actor attachments, so SetLocalTransform expects world-space.
                const AZ::Quaternion entityOrientation = AZ::Quaternion::CreateFromTransform(world);
                const AZ::Vector3 entityPosition = world.GetTranslation();
                const AZ::Transform worldTransformNoScale = AZ::Transform::CreateFromQuaternionAndTranslation(entityOrientation, entityPosition);
                m_actorInstance->SetLocalTransform(MCore::AzTransformToEmfxTransform(worldTransformNoScale));
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::Asset<ActorAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ActorAsset>(assetId, false);
            if (asset)
            {
                m_actorAsset = asset;
                OnAssetSelected();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            if (m_actorInstance && m_renderSkeleton)
            {
                ActorComponent::DrawSkeleton(m_actorInstance);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Aabb EditorActorComponent::GetWorldBounds()
        {
            if (m_actorInstance)
            {
                MCore::AABB emfxAabb;
                m_actorInstance->CalcNodeBasedAABB(&emfxAabb);
                return AZ::Aabb::CreateFromMinMax(emfxAabb.GetMin(), emfxAabb.GetMax());
            }

            return AZ::Aabb::CreateNull();
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Aabb EditorActorComponent::GetLocalBounds()
        {
            if (m_actorInstance)
            {
                AZ::Aabb localAabb = GetWorldBounds();
                if (localAabb.IsValid())
                {
                    localAabb.ApplyTransform(m_renderNode->m_worldTransform.GetInverseFast());
                    return localAabb;
                }
            }

            return AZ::Aabb::CreateNull();
        }

        //////////////////////////////////////////////////////////////////////////
        bool EditorActorComponent::GetVisibility()
        {
            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::SetVisibility(bool isVisible)
        {
            (void)isVisible;
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            ActorComponent::Configuration cfg;
            cfg.m_actorAsset = m_actorAsset;
            cfg.m_materialPerLOD = m_materialPerLOD;
            cfg.m_renderSkeleton = m_renderSkeleton;
            cfg.m_attachmentType = m_attachmentType;
            cfg.m_attachmentTarget = m_attachmentTarget;
            cfg.m_attachmentJointIndex = m_attachmentJointIndex;
            cfg.m_skinningMethod = m_skinningMethod;

            gameEntity->AddComponent(aznew ActorComponent(&cfg));
        }

        //////////////////////////////////////////////////////////////////////////
        IRenderNode* EditorActorComponent::GetRenderNode()
        {
            return m_renderNode.get();
        }

        //////////////////////////////////////////////////////////////////////////
        const float EditorActorComponent::s_renderNodeRequestBusOrder = 100.f;
        float EditorActorComponent::GetRenderNodeRequestBusOrder() const
        {
            return s_renderNodeRequestBusOrder;
        }

        AZ::EntityId EditorActorComponent::GetAttachedToEntityId() const
        {
            return m_attachmentTarget;
        }

        //////////////////////////////////////////////////////////////////////////
        // Check if the given attachment is valid.
        bool EditorActorComponent::IsValidAttachment(const AZ::EntityId& attachment, const AZ::EntityId& attachTo) const
        {
            // Cannot attach to yourself.
            if (attachment == attachTo)
            {
                return false;
            }

            // Walk our way up to the root.
            AZ::EntityId resultId;
            EditorActorComponentRequestBus::EventResult(resultId, attachTo, &EditorActorComponentRequestBus::Events::GetAttachedToEntityId);
            while (resultId.IsValid())
            {
                AZ::EntityId localResult;
                EditorActorComponentRequestBus::EventResult(localResult, resultId, &EditorActorComponentRequestBus::Events::GetAttachedToEntityId);

                // We detected a loop.
                if (localResult == attachment)
                {
                    return false;
                }

                resultId = localResult;
            }

            return true;
        }

    } //namespace Integration
} // namespace EMotionFX

