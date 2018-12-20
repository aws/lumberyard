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

#include "StdAfx.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceMetadataInfoBus.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Commands/PreemptiveUndoCache.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/Slice/SliceDataFlagsCommand.h>
#include <AzToolsFramework/Slice/SliceCompilation.h>

#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/SelectionComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>

#include "EditorEntityContextComponent.h"

namespace AzToolsFramework
{
    //=========================================================================
    // Reflect
    //=========================================================================
    void EditorEntityContextComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorEntityContextComponent, AZ::Component>()
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorEntityContextComponent>(
                    "Editor Entity Context", "System component responsible for owning the edit-time entity context")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }
    }

    //=========================================================================
    // EditorEntityContextComponent ctor
    //=========================================================================
    EditorEntityContextComponent::EditorEntityContextComponent()
        : AzFramework::EntityContext(AzFramework::EntityContextId::CreateRandom())
        , m_isRunningGame(false)
        , m_requiredEditorComponentTypes
        // These are the components that will be force added to every entity in the editor
        ({
            azrtti_typeid<AzToolsFramework::Components::EditorDisabledCompositionComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorOnlyEntityComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorEntityIconComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorEntitySortComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorInspectorComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorLockComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorPendingCompositionComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorVisibilityComponent>(),
            azrtti_typeid<AzToolsFramework::Components::SelectionComponent>(),
            azrtti_typeid<AzToolsFramework::Components::TransformComponent>()
        })
    {
    }

    //=========================================================================
    // EditorEntityContextComponent dtor
    //=========================================================================
    EditorEntityContextComponent::~EditorEntityContextComponent()
    {
    }

    //=========================================================================
    // Init
    //=========================================================================
    void EditorEntityContextComponent::Init()
    {
    }

    //=========================================================================
    // Activate
    //=========================================================================
    void EditorEntityContextComponent::Activate()
    {
        InitContext();

        GetRootSlice()->Instantiate();

        EditorEntityContextRequestBus::Handler::BusConnect();

        EditorEntityContextPickingRequestBus::Handler::BusConnect(GetContextId());
    }

    //=========================================================================
    // Deactivate
    //=========================================================================
    void EditorEntityContextComponent::Deactivate()
    {
        EditorEntityContextRequestBus::Handler::BusDisconnect();

        EditorEntityContextPickingRequestBus::Handler::BusDisconnect();

        DestroyContext();
    }

    //=========================================================================
    // EditorEntityContextRequestBus::ResetEditorContext
    //=========================================================================
    void EditorEntityContextComponent::ResetEditorContext()
    {
        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnContextReset);

        if (m_isRunningGame)
        {
            // Ensure we exit play-in-editor when the context is reset (switching levels).
            StopPlayInEditor();
        }

        ResetContext();
    }

    //=========================================================================
    // EditorEntityContextRequestBus::CreateEditorEntity
    //=========================================================================
    AZ::Entity* EditorEntityContextComponent::CreateEditorEntity(const char* name)
    {
        AZ::Entity* entity = CreateEntity(name);

        if (entity)
        {
            SetupEditorEntity(entity);

            // Store creation undo command.
            {
                ScopedUndoBatch undoBatch("Create Entity");

                EntityCreateCommand* command = aznew EntityCreateCommand(static_cast<AZ::u64>(entity->GetId()));
                command->Capture(entity);
                command->SetParent(undoBatch.GetUndoBatch());
            }
        }

        return entity;
    }

    //=========================================================================
    // EditorEntityContextRequestBus::AddEditorEntity
    //=========================================================================
    void EditorEntityContextComponent::AddEditorEntity(AZ::Entity* entity)
    {
        AZ_Assert(entity, "Supplied entity is invalid.");

        AddEntity(entity);
    }

    //=========================================================================
    // EditorEntityContextRequestBus::AddEditorEntities
    //=========================================================================
    void EditorEntityContextComponent::AddEditorEntities(const AzToolsFramework::EntityList& entities)
    {
        AZ::SliceAsset* rootSlice = m_rootAsset.Get();

        for (AZ::Entity* entity : entities)
        {
            AZ_Assert(!AzFramework::EntityIdContextQueryBus::MultiHandler::BusIsConnectedId(entity->GetId()), "Entity already in context.");
            rootSlice->GetComponent()->AddEntity(entity);
        }

        HandleEntitiesAdded(entities);
    }

    void EditorEntityContextComponent::AddEditorSliceEntities(const AzToolsFramework::EntityList& entities)
    {
        HandleEntitiesAdded(entities);
    }

    //=========================================================================
    // EditorEntityContextRequestBus::CloneEditorEntities
    //=========================================================================
    bool EditorEntityContextComponent::CloneEditorEntities(const AzToolsFramework::EntityIdList& sourceEntities,
                                                           AzToolsFramework::EntityList& resultEntities,
                                                           AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        resultEntities.clear();

        AZ::EntityUtils::SerializableEntityContainer sourceObjects;
        for (const AZ::EntityId& id : sourceEntities)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, id);
            if (entity)
            {
                sourceObjects.m_entities.push_back(entity);
            }
        }

        AZ::EntityUtils::SerializableEntityContainer* clonedObjects =
            AZ::IdUtils::Remapper<AZ::EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(&sourceObjects, sourceToCloneEntityIdMap);
        if (!clonedObjects)
        {
            AZ_Error("EditorEntityContext", false, "Failed to clone source entities.");
            sourceToCloneEntityIdMap.clear();
            return false;
        }

        resultEntities = clonedObjects->m_entities;

        delete clonedObjects;

        return true;
    }

    //=========================================================================
    // EditorEntityContextRequestBus::DestroyEditorEntity
    //=========================================================================
    bool EditorEntityContextComponent::DestroyEditorEntity(AZ::EntityId entityId)
    {
        if (DestroyEntityById(entityId))
        {
            EditorRequests::Bus::Broadcast(&EditorRequests::DestroyEditorRepresentation, entityId, false);
            return true;
        }

        return false;
    }

    //=========================================================================
    // EditorEntityContextRequestBus::DetachSliceEntities
    //=========================================================================
    void EditorEntityContextComponent::DetachSliceEntities(const AzToolsFramework::EntityIdList& entities)
    {
        if (entities.empty())
        {
            return;
        }

        AzToolsFramework::EntityIdList changedEntities;

        for (const AZ::EntityId& entityId : entities)
        {
            AZ::SliceComponent::SliceInstanceAddress sliceAddress;
            AzFramework::EntityIdContextQueryBus::EventResult(sliceAddress, entityId, &EntityIdContextQueries::GetOwningSlice);
            if (sliceAddress.IsValid())
            {
                AZ::SliceComponent::SliceReference* sliceReference = sliceAddress.GetReference();

                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
                AZ_Error("EditorEntityContext", entity, "Unable to find entity for EntityID %llu", entityId);

                if (entity)
                {
                    changedEntities.push_back(entityId);

                    if (sliceReference->GetSliceComponent()->RemoveEntity(entityId, false)) // Remove from current slice instance without deleting
                    {
                        GetRootSlice()->AddEntity(entity); // Add back as loose entity
                    }
                }
            }
        }

        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnEditorEntitiesSliceOwnershipChanged, changedEntities);
    }

    //=========================================================================
    // EditorEntityContextRequestBus::ResetEntitiesToSliceDefaults
    //=========================================================================
    void EditorEntityContextComponent::ResetEntitiesToSliceDefaults(AzToolsFramework::EntityIdList entities)
    {
        ScopedUndoBatch undoBatch("Resetting entities to slice defaults.");

        PreemptiveUndoCache* preemptiveUndoCache = nullptr;
        ToolsApplicationRequests::Bus::BroadcastResult(preemptiveUndoCache, &ToolsApplicationRequests::GetUndoCache);

        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        AzToolsFramework::SelectionCommand* selCommand = aznew AzToolsFramework::SelectionCommand(
            selectedEntities, "Reset Entity to Slice Defaults");
        selCommand->SetParent(undoBatch.GetUndoBatch());

        AzToolsFramework::SelectionCommand* newSelCommand = aznew AzToolsFramework::SelectionCommand(
            selectedEntities, "Reset Entity to Slice Defaults");

        for (AZ::EntityId id : entities)
        {
            AZ::SliceComponent::SliceInstanceAddress sliceAddress;
            AzFramework::EntityIdContextQueryBus::EventResult(sliceAddress, id, &AzFramework::EntityIdContextQueries::GetOwningSlice);

            AZ::SliceComponent::SliceReference* sliceReference = sliceAddress.GetReference();
            AZ::SliceComponent::SliceInstance* sliceInstance = sliceAddress.GetInstance();

            AZ::EntityId sliceRootEntityId;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(sliceRootEntityId,
                &AzToolsFramework::ToolsApplicationRequestBus::Events::GetRootEntityIdOfSliceInstance, sliceAddress);

            bool isSliceRootEntity = (id == sliceRootEntityId);

            if (sliceReference)
            {
                // Clear any data flags for entity
                auto clearDataFlagsCommand = aznew ClearSliceDataFlagsBelowAddressCommand(id, AZ::DataPatch::AddressType(), "Clear data flags");
                clearDataFlagsCommand->SetParent(undoBatch.GetUndoBatch());

                AZ::Entity* oldEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(oldEntity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
                AZ_Assert(oldEntity, "Couldn't find the entity we were looking for!");
                if (!oldEntity)
                {
                    continue;
                }

                // Clone the entity from the slice source (clean)
                auto sourceEntityIterator = sliceInstance->GetEntityIdToBaseMap().find(id);
                AZ_Assert(sourceEntityIterator != sliceInstance->GetEntityIdToBaseMap().end(), "Attempting to clone an invalid instance entity id for this slice instance!");
                if (sourceEntityIterator != sliceInstance->GetEntityIdToBaseMap().end())
                {
                    AZ::SliceComponent* dependentSlice = sliceReference->GetSliceAsset().Get()->GetComponent();
                    AZ::Entity* sourceEntity = dependentSlice->FindEntity(sourceEntityIterator->second);

                    AZ_Assert(sourceEntity, "Couldn't find source entity from sourceEntityId in slice reference!");
                    if (sourceEntity)
                    {
                        AZ::Entity* entityClone = dependentSlice->GetSerializeContext()->CloneObject(sourceEntity);
                        if (entityClone)
                        {
                            AZ::IdUtils::Remapper<AZ::EntityId>::ReplaceIdsAndIdRefs(
                                entityClone,
                                [sliceInstance](const AZ::EntityId& originalId, bool /*isEntityId*/, const AZStd::function<AZ::EntityId()>& /*idGenerator*/) -> AZ::EntityId
                                {
                                    auto findIt = sliceInstance->GetEntityIdMap().find(originalId);
                                    if (findIt == sliceInstance->GetEntityIdMap().end())
                                    {
                                        return originalId; // entityId is not being remapped
                                    }
                                    else
                                    {
                                        return findIt->second; // return the remapped id
                                    }
                                }, dependentSlice->GetSerializeContext());

                            // Only restore world position and rotation for the slice root entity.
                            if (isSliceRootEntity)
                            {
                                // Get the transform component on the cloned entity.  We cannot use the bus since it isn't activated.
                                AzToolsFramework::Components::TransformComponent* transformComponent = entityClone->FindComponent<AzToolsFramework::Components::TransformComponent>();
                                AZ_Assert(transformComponent, "Entity doesn't have a transform component!");
                                if (transformComponent)
                                {
                                    AZ::Vector3 oldEntityWorldPosition;
                                    AZ::TransformBus::EventResult(oldEntityWorldPosition, id, &AZ::TransformBus::Events::GetWorldTranslation);
                                    transformComponent->SetWorldTranslation(oldEntityWorldPosition);

                                    AZ::Quaternion oldEntityRotation;
                                    AZ::TransformBus::EventResult(oldEntityRotation, id, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
                                    transformComponent->SetRotationQuaternion(oldEntityRotation);
                                }
                            }

                            // Create a state command and capture both the undo and redo data
                            AzToolsFramework::EntityStateCommand* stateCommand = aznew AzToolsFramework::EntityStateCommand(
                                static_cast<AzToolsFramework::UndoSystem::URCommandID>(id));
                            stateCommand->Capture(oldEntity, true);
                            stateCommand->Capture(entityClone, false);
                            stateCommand->SetParent(undoBatch.GetUndoBatch());

                            // Delete our temporary entity clone
                            delete entityClone;
                        }
                    }
                }
            }
        }

        newSelCommand->SetParent(undoBatch.GetUndoBatch());

        // Run the redo in order to do the initial swap of entity data
        undoBatch.GetUndoBatch()->RunRedo();

        // Make sure to set selection to newly cloned entities
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::Bus::Events::SetSelectedEntities, selectedEntities);
    }

    AZ::SliceComponent::SliceInstanceAddress EditorEntityContextComponent::CloneSubSliceInstance(
        const AZ::SliceComponent::SliceInstanceAddress& sourceSliceInstanceAddress,
        const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sourceSubSliceInstanceAncestry,
        const AZ::SliceComponent::SliceInstanceAddress& sourceSubSliceInstanceAddress,
        AZ::SliceComponent::EntityIdToEntityIdMap* out_sourceToCloneEntityIdMap)
    {
        AZ::SliceComponent::SliceInstanceAddress subSliceInstanceCloneAddress = 
            GetRootSlice()->CloneAndAddSubSliceInstance(sourceSliceInstanceAddress.GetInstance(), sourceSubSliceInstanceAncestry, sourceSubSliceInstanceAddress, out_sourceToCloneEntityIdMap);

        if (!subSliceInstanceCloneAddress.IsValid())
        {
            AZ_Assert(false, "Clone sub-slice instance failed.");
            return AZ::SliceComponent::SliceInstanceAddress();
        }

        return subSliceInstanceCloneAddress;
    }

    //=========================================================================
    // EditorEntityContextRequestBus::CloneEditorSliceInstance
    //=========================================================================
    AZ::SliceComponent::SliceInstanceAddress EditorEntityContextComponent::CloneEditorSliceInstance(
        AZ::SliceComponent::SliceInstanceAddress sourceInstance, AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (sourceInstance.IsValid())
        {
            AZ::SliceComponent::SliceInstanceAddress address = CloneSliceInstance(sourceInstance, sourceToCloneEntityIdMap);

            return address;
        }
        else
        {
            AZ_Error("EditorEntityContext", false, "Invalid slice source instance. Unable to clone.");
        }

        return AZ::SliceComponent::SliceInstanceAddress();
    }

    //=========================================================================
    // EditorEntityContextRequestBus::SaveToStreamForEditor
    //=========================================================================
    bool EditorEntityContextComponent::SaveToStreamForEditor(AZ::IO::GenericStream& stream)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ_Assert(stream.IsOpen(), "Invalid target stream.");
        AZ_Assert(m_rootAsset.Get() && m_rootAsset.Get()->GetComponent(), "The context is not initialized.");

        return AZ::Utils::SaveObjectToStream<AZ::Entity>(stream, AZ::DataStream::ST_XML, m_rootAsset.Get()->GetEntity());
    }

    //=========================================================================
    // EditorEntityContextRequestBus::SaveToStreamForGame
    //=========================================================================
    bool EditorEntityContextComponent::SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ::SliceComponent::EntityList sourceEntities;
        GetRootSlice()->GetEntities(sourceEntities);

        // Create a source slice from our editor components.
        AZ::Entity* sourceSliceEntity = aznew AZ::Entity();
        AZ::SliceComponent* sourceSliceData = sourceSliceEntity->CreateComponent<AZ::SliceComponent>();
        AZ::Data::Asset<AZ::SliceAsset> sourceSliceAsset(aznew AZ::SliceAsset());
        sourceSliceAsset.Get()->SetData(sourceSliceEntity, sourceSliceData);

        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            // This is a sanity check that the editor source entity was created and validated before starting export to game target entity
            // Note - this assert should *not* live directly in PreExportEntity implementation,
            //   because of the case where we are modifying system entities we do not want system components active, ticking tick buses, running simulations, etc.
            AZ_Assert(sourceEntity->GetState() == AZ::Entity::ES_ACTIVE, "Source entity must be active.");

            sourceSliceData->AddEntity(sourceEntity);
        }

        // Emulate client flags.
        AZ::PlatformTagSet platformTags = {AZ_CRC("renderer", 0xf199a19c)};

        // Compile the source slice into the runtime slice (with runtime components).
        WorldEditorOnlyEntityHandler editorOnlyEntityHandler;
        SliceCompilationResult sliceCompilationResult = CompileEditorSlice(sourceSliceAsset, platformTags, *m_serializeContext, &editorOnlyEntityHandler);

        // Reclaim entities from the temporary source asset.
        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            // This is a sanity check that the editor source entity was created and validated and didn't suddenly get out of an active state during export
            // Note - this assert should *not* live directly in PostExportEntity implementation,
            //   because of the case where we are modifying system entities we do not want system components active, ticking tick buses, running simulations, etc.
            AZ_Assert(sourceEntity->GetState() == AZ::Entity::ES_ACTIVE, "Source entity must be active.");

            sourceSliceData->RemoveEntity(sourceEntity, false);
        }

        if (!sliceCompilationResult)
        {
            AZ_Error("Save Runtime Stream", false, "Failed to export entities for runtime:\n%s", sliceCompilationResult.GetError().c_str());
            return false;
        }

        // Export runtime slice representing the level, which is a completely flat list of entities.
        AZ::Data::Asset<AZ::SliceAsset> exportSliceAsset = sliceCompilationResult.GetValue();
        return AZ::Utils::SaveObjectToStream<AZ::Entity>(stream, streamType, exportSliceAsset.Get()->GetEntity());
    }

    //=========================================================================
    // EditorEntityContextRequestBus::LoadFromStream
    //=========================================================================
    bool EditorEntityContextComponent::LoadFromStream(AZ::IO::GenericStream& stream)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ_Assert(stream.IsOpen(), "Invalid source stream.");
        AZ_Assert(m_rootAsset, "The context has not been initialized.");

        EditorEntityContextNotificationBus::Broadcast(
            &EditorEntityContextNotification::OnEntityStreamLoadBegin);

        const bool loadedSuccessfully = AzFramework::EntityContext::LoadFromStream(stream, false, nullptr, AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterSourceSlicesOnly));

        if (loadedSuccessfully)
        {
            AZ::SliceComponent::EntityList entities;
            GetRootSlice()->GetEntities(entities);

            GetRootSlice()->SetIsDynamic(true);

            SetupEditorEntities(entities);

            EditorEntityContextNotificationBus::Broadcast(
                &EditorEntityContextNotification::OnEntityStreamLoadSuccess);
        }
        else
        {
            EditorEntityContextNotificationBus::Broadcast(
                &EditorEntityContextNotification::OnEntityStreamLoadFailed);
        }

        return loadedSuccessfully;
    }

    //=========================================================================
    // EditorEntityContextRequestBus::InstantiateEditorSlice
    //=========================================================================
    AzFramework::SliceInstantiationTicket EditorEntityContextComponent::InstantiateEditorSlice(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, const AZ::Transform& worldTransform)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (sliceAsset.GetId().IsValid())
        {
            m_instantiatingSlices.push_back(AZStd::make_pair(sliceAsset, worldTransform));

            const AzFramework::SliceInstantiationTicket ticket = InstantiateSlice(sliceAsset);
            if (ticket)
            {
                AzFramework::SliceInstantiationResultBus::MultiHandler::BusConnect(ticket);
            }

            return ticket;
        }

        return AzFramework::SliceInstantiationTicket();
    }

    //=========================================================================
    // EntityContextRequestBus::StartPlayInEditor
    //=========================================================================
    void EditorEntityContextComponent::StartPlayInEditor()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnStartPlayInEditorBegin);

        // Save the editor context to a memory stream.
        AZStd::vector<char> entityBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > stream(&entityBuffer);
        const bool isRuntimeStreamValid = SaveToStreamForGame(stream, AZ::DataStream::ST_BINARY);

        if (!isRuntimeStreamValid)
        {
            AZ_Error("EditorEntityContext", false, "Failed to create runtime entity context for play-in-editor mode. Entities will not be created.");
        }

        // Deactivate the editor context.
        AZ::SliceComponent* rootSlice = GetRootSlice();
        if (rootSlice)
        {
            AZ::SliceComponent::EntityList entities;
            rootSlice->GetEntities(entities);

            for (AZ::Entity* entity : entities)
            {
                if (entity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    entity->Deactivate();
                }
            }
        }

        m_runtimeToEditorIdMap.clear();

        if (isRuntimeStreamValid)
        {
            // Load the exported stream into the game context.
            stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            AzFramework::GameEntityContextRequestBus::Broadcast(&AzFramework::GameEntityContextRequests::LoadFromStream, stream, true);

            // Retrieve Id map from game entity context (editor->runtime).
            AzFramework::EntityContextId gameContextId = AzFramework::EntityContextId::CreateNull();
            AzFramework::GameEntityContextRequestBus::BroadcastResult(
                gameContextId, &AzFramework::GameEntityContextRequests::GetGameEntityContextId);
            AzFramework::EntityContextRequestBus::EventResult(
                m_editorToRuntimeIdMap, gameContextId, &AzFramework::EntityContextRequests::GetLoadedEntityIdMap);

            // Generate reverse lookup (runtime->editor).
            for (const auto& idMapping : m_editorToRuntimeIdMap)
            {
                m_runtimeToEditorIdMap.insert(AZStd::make_pair(idMapping.second, idMapping.first));
            }
        }

        m_isRunningGame = true;

        ToolsApplicationRequests::Bus::BroadcastResult(m_selectedBeforeStartingGame, &ToolsApplicationRequests::GetSelectedEntities);

        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnStartPlayInEditor);
    }

    //=========================================================================
    // EntityContextRequestBus::StopPlayInEditor
    //=========================================================================
    void EditorEntityContextComponent::StopPlayInEditor()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        m_isRunningGame = false;

        m_editorToRuntimeIdMap.clear();
        m_runtimeToEditorIdMap.clear();

        // Reset the runtime context.
        AzFramework::GameEntityContextRequestBus::Broadcast(
            &AzFramework::GameEntityContextRequests::ResetGameContext);

        // Do a full lua GC.
        AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::GarbageCollect);

        // Re-activate the editor context.
        AZ::SliceComponent* rootSlice = GetRootSlice();
        if (rootSlice)
        {
            AZ::SliceComponent::EntityList entities;
            rootSlice->GetEntities(entities);

            for (AZ::Entity* entity : entities)
            {
                if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    entity->Init();
                }

                if (entity->GetState() == AZ::Entity::ES_INIT)
                {
                    entity->Activate();
                }
            }
        }

        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, m_selectedBeforeStartingGame);
        m_selectedBeforeStartingGame.clear();

        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnStopPlayInEditor);
    }

    //=========================================================================
    // EntityContextRequestBus::IsEditorRunningGame
    //=========================================================================
    bool EditorEntityContextComponent::IsEditorRunningGame()
    {
        return m_isRunningGame;
    }

    //=========================================================================
    // EntityContextRequestBus::IsEditorEntity
    //=========================================================================
    bool EditorEntityContextComponent::IsEditorEntity(AZ::EntityId id)
    {
        AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityIdContextQueryBus::EventResult(contextId, id, &EntityIdContextQueries::GetOwningContextId);

        if (contextId == GetContextId())
        {
            return true;
        }

        return false;
    }

    //=========================================================================
    // EntityContextRequestBus::AddRequiredComponents
    //=========================================================================
    void EditorEntityContextComponent::AddRequiredComponents(AZ::Entity& entity)
    {
        for (const auto& componentType : m_requiredEditorComponentTypes)
        {
            if (!entity.FindComponent(componentType))
            {
                entity.CreateComponent(componentType);
            }
        }
    }

    //=========================================================================
    // EntityContextRequestBus::GetRequiredComponentTypes
    //=========================================================================
    const AZ::ComponentTypeList& EditorEntityContextComponent::GetRequiredComponentTypes()
    {
        return m_requiredEditorComponentTypes;
    }

    //=========================================================================
    // EntityContextRequestBus::RestoreSliceEntity
    //=========================================================================
    void EditorEntityContextComponent::RestoreSliceEntity(AZ::Entity* entity, const AZ::SliceComponent::EntityRestoreInfo& info)
    {
        AZ_Error("EditorEntityContext", info.m_assetId.IsValid(), "Invalid asset Id for entity restore.");

        // If asset isn't loaded when this request is made, we need to queue the load and process the request
        // when the asset is ready. Otherwise we'll immediately process the request when OnAssetReady is invoked
        // by the AssetBus connection policy.

        AZ::Data::Asset<AZ::Data::AssetData> asset =
            AZ::Data::AssetManager::Instance().GetAsset<AZ::SliceAsset>(info.m_assetId, true);

        SliceEntityRestoreRequest request = {entity, info, asset};
        m_queuedSliceEntityRestores.emplace_back(request);

        AZ::Data::AssetBus::MultiHandler::BusConnect(asset.GetId());
    }

    //=========================================================================
    // EntityContextEventBus::QueueSliceReplacement
    //=========================================================================
    void EditorEntityContextComponent::QueueSliceReplacement(const char* targetPath,
        const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& selectedToAssetMap,
        const AZStd::unordered_set<AZ::EntityId>& entitiesInSelection,
        const AZ::EntityId& parentAfterReplacement,
        const AZ::Vector3& offsetAfterReplacement,
        const AZ::Quaternion& rotationAfterReplacement,
        bool rootAutoCreated)
    {
        AZ_Error("EditorEntityContext", m_queuedSliceReplacement.m_path.empty(), "A slice replacement is already on the queue.");
        m_queuedSliceReplacement.Setup(targetPath, selectedToAssetMap, entitiesInSelection, parentAfterReplacement, offsetAfterReplacement, rotationAfterReplacement, rootAutoCreated);
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }

    //=========================================================================
    // EntityContextEventBus::MapEditorIdToRuntimeId
    //=========================================================================
    bool EditorEntityContextComponent::MapEditorIdToRuntimeId(const AZ::EntityId& editorId, AZ::EntityId& runtimeId)
    {
        auto iter = m_editorToRuntimeIdMap.find(editorId);
        if (iter != m_editorToRuntimeIdMap.end())
        {
            runtimeId = iter->second;
            return true;
        }

        return false;
    }

    //=========================================================================
    // EntityContextEventBus::MapRuntimeIdToEditorId
    //=========================================================================
    bool EditorEntityContextComponent::MapRuntimeIdToEditorId(const AZ::EntityId& runtimeId, AZ::EntityId& editorId)
    {
        auto iter = m_runtimeToEditorIdMap.find(runtimeId);
        if (iter != m_runtimeToEditorIdMap.end())
        {
            editorId = iter->second;
            return true;
        }

        return false;
    }

    //=========================================================================
    // EditorEntityContextPickingRequestBus::SupportsViewportEntityIdPicking
    //=========================================================================
    bool EditorEntityContextComponent::SupportsViewportEntityIdPicking()
    {
        return true;
    }

    //=========================================================================
    // EntityContextEventBus::OnCatalogAssetAdded
    //=========================================================================
    void EditorEntityContextComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        if (m_queuedSliceReplacement.IsValid())
        {
            if (m_queuedSliceReplacement.OnCatalogAssetAdded(assetId))
            {
                AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            }
        }
    }

    //=========================================================================
    // EntityContextEventBus::OnSlicePreInstantiate
    //=========================================================================
    void EditorEntityContextComponent::OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

        // Start an undo that will wrap the entire slice instantiation event (unable to do this at a higher level since this is queued up by AzFramework and there's no undo concept at that level)
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::BeginUndoBatch, "Slice Instantiation");

        if (ticket == m_queuedSliceReplacement.m_ticket)
        {
            m_queuedSliceReplacement.OnSlicePreInstantiate();
        }

        // Find the next ticket corresponding to this asset.
        // Given the desired world root, position entities in the instance.
        for (auto instantiatingIter = m_instantiatingSlices.begin(); instantiatingIter != m_instantiatingSlices.end(); ++instantiatingIter)
        {
            if (instantiatingIter->first.GetId() == sliceAssetId)
            {
                const AZ::Transform& worldTransform = instantiatingIter->second;

                const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;

                for (AZ::Entity* entity : entities)
                {
                    auto* transformComponent = entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
                    if (transformComponent)
                    {
                        // Non-root entities will be positioned relative to their parents.
                        // NOTE: The second expression (parentId == entity->Id) is needed only due to backward data compatibility.
                        if (!transformComponent->GetParentId().IsValid() || transformComponent->GetParentId() == entity->GetId())
                        {
                            // Note: Root slice entity always has translation at origin, so this maintains scale & rotation.
                            transformComponent->SetWorldTM(worldTransform * transformComponent->GetWorldTM());
                        }
                    }
                }

                break;
            }
        }
    }

    //=========================================================================
    // EntityContextEventBus::OnSliceInstantiated
    //=========================================================================
    void EditorEntityContextComponent::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

        if (ticket == m_queuedSliceReplacement.m_ticket)
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityContextComponent::OnSliceInstantiated:FinalizeQueuedSlice");
            m_queuedSliceReplacement.Finalize(sliceAddress);
        }

        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        // Close out the next ticket corresponding to this asset.
        for (auto instantiatingIter = m_instantiatingSlices.begin(); instantiatingIter != m_instantiatingSlices.end(); ++instantiatingIter)
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityContextComponent::OnSliceInstantiated:CloseTicket");
            if (instantiatingIter->first.GetId() == sliceAssetId)
            {
                const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;

                // Select slice roots when found, otherwise default to selecting all entities in slice
                AzToolsFramework::EntityIdSet setOfEntityIds;
                for (const AZ::Entity* const entity : entities)
                {
                    setOfEntityIds.insert(entity->GetId());
                }

                AzToolsFramework::EntityIdList selectEntities;
                AZ::EntityId commonRoot;
                bool wasCommonRootFound = false;
                ToolsApplicationRequests::Bus::BroadcastResult(
                    wasCommonRootFound, &ToolsApplicationRequests::FindCommonRoot, setOfEntityIds, commonRoot, &selectEntities);
                if (!wasCommonRootFound || selectEntities.empty())
                {
                    for (AZ::Entity* entity : entities)
                    {
                        selectEntities.push_back(entity->GetId());
                    }
                }

                ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selectEntities);

                // Create a slice instantiation undo command.
                {
                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityContextComponent::OnSliceInstantiated:CloseTicket:CreateInstantiateUndo");
                    ScopedUndoBatch undoBatch("Instantiate Slice");
                    for (AZ::Entity* entity : entities)
                    {
                        // Don't mark entities as dirty for PropertyChange undo action if they are just instantiated.
                        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::RemoveDirtyEntity, entity->GetId());

                        EntityCreateCommand* command = aznew EntityCreateCommand(
                            reinterpret_cast<UndoSystem::URCommandID>(sliceAddress.GetInstance()));
                        command->Capture(entity);
                        command->SetParent(undoBatch.GetUndoBatch());
                    }
                }

                EditorEntityContextNotificationBus::Broadcast(
                    &EditorEntityContextNotification::OnSliceInstantiated, sliceAssetId, sliceAddress, ticket);

                m_instantiatingSlices.erase(instantiatingIter);

                break;
            }
        }

        // End the slice instantiation undo batch started in OnSlicePreInstantiate
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::EndUndoBatch);
    }

    //=========================================================================
    // EntityContextEventBus::OnSliceInstantiationFailed
    //=========================================================================
    void EditorEntityContextComponent::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        for (auto instantiatingIter = m_instantiatingSlices.begin(); instantiatingIter != m_instantiatingSlices.end(); ++instantiatingIter)
        {
            if (instantiatingIter->first.GetId() == sliceAssetId)
            {
                EditorEntityContextNotificationBus::Broadcast(
                    &EditorEntityContextNotification::OnSliceInstantiationFailed, sliceAssetId, ticket);

                m_instantiatingSlices.erase(instantiatingIter);
                break;
            }
        }
    }

    //=========================================================================
    // PrepareForContextReset
    //=========================================================================
    void EditorEntityContextComponent::PrepareForContextReset()
    {
        EntityContext::PrepareForContextReset();
        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::PrepareForContextReset);
    }

    //=========================================================================
    // OnContextEntitiesAdded
    //=========================================================================
    void EditorEntityContextComponent::OnContextEntitiesAdded(const EntityList& entities)
    {
        EntityContext::OnContextEntitiesAdded(entities);

        // Any entities being added to the context that don't belong to another slice
        // need to be associated with the root slice metadata info component.
        for (const auto* entity : entities)
        {
            auto sliceAddress = m_rootAsset.Get()->GetComponent()->FindSlice(entity->GetId());
            if (!sliceAddress.IsValid())
            {
                AZ::SliceMetadataInfoManipulationBus::Event(m_rootAsset.Get()->GetComponent()->GetMetadataEntity()->GetId(), &AZ::SliceMetadataInfoManipulationBus::Events::AddAssociatedEntity, entity->GetId());
            }
        }

        SetupEditorEntities(entities);
    }

    //=========================================================================
    // OnContextEntityRemoved
    //=========================================================================
    void EditorEntityContextComponent::OnContextEntityRemoved(const AZ::EntityId& entityId)
    {
        EditorRequests::Bus::Broadcast(&EditorRequests::DestroyEditorRepresentation, entityId, false);
    }

    //=========================================================================
    // HandleNewMetadataEntities
    //=========================================================================
    void EditorEntityContextComponent::HandleNewMetadataEntitiesCreated(AZ::SliceComponent& slice)
    {
        AZ::SliceComponent::EntityList metadataEntityIds;
        slice.GetAllMetadataEntities(metadataEntityIds);

        const auto& slices = slice.GetSlices();

        // Add the metadata entity from the root slice to the appropriate context
        // For now, the instance address of the root slice is <nullptr,nullptr>
        SliceMetadataEntityContextRequestBus::Broadcast(&SliceMetadataEntityContextRequestBus::Events::AddMetadataEntityToContext, AZ::SliceComponent::SliceInstanceAddress(), *slice.GetMetadataEntity());

        // Add the metadata entities from every slice instance streamed in as well. We need to grab them directly
        // from their slices so we have the appropriate instance address for each one.
        for (auto& subSlice : slices)
        {
            for (const auto& instance : subSlice.GetInstances())
            {
                if (auto instantiated = instance.GetInstantiated())
                {
                    for (auto* metadataEntity : instantiated->m_metadataEntities)
                    {
                        SliceMetadataEntityContextRequestBus::Broadcast(&SliceMetadataEntityContextRequestBus::Events::AddMetadataEntityToContext, AZ::SliceComponent::SliceInstanceAddress(const_cast<AZ::SliceComponent::SliceReference*>(&subSlice), const_cast<AZ::SliceComponent::SliceInstance*>(&instance)), *metadataEntity);
                    }
                }
            }
        }
    }

    //=========================================================================
    // SetupEditorEntity
    //=========================================================================
    void EditorEntityContextComponent::SetupEditorEntity(AZ::Entity* entity)
    {
        SetupEditorEntities({ entity });
    }

    //=========================================================================
    // SetupEditorEntities
    //=========================================================================
    void EditorEntityContextComponent::SetupEditorEntities(const AZStd::vector<AZ::Entity*>& entities)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // All editor entities are automatically activated.

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityContextComponent::SetupEditorEntities:ScrubEntities");

            // Scrub entities before initialization.
            // Anything could go wrong with entities loaded from disk.
            // Ex: There might be duplicates of components that do not tolerate
            // duplication and would crash during their Init().
            EntityCompositionRequestBus::Broadcast(&EntityCompositionRequestBus::Events::ScrubEntities, entities);
        }

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityContextComponent::SetupEditorEntities:InitEntities");
            for (AZ::Entity* entity : entities)
            {
                if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    entity->Init();
                }
            }
        }

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityContextComponent::SetupEditorEntities:CreateEditorRepresentations");
            for (AZ::Entity* entity : entities)
            {
                EditorRequests::Bus::Broadcast(&EditorRequests::CreateEditorRepresentation, entity);
            }
        }

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityContextComponent::SetupEditorEntities:ActivateEntities");
            for (AZ::Entity* entity : entities)
            {
                if (entity->GetState() == AZ::Entity::ES_INIT)
                {
                    // Always invalidate the entity dependencies when loading in the editor
                    // (we don't know what code has changed since the last time the editor was run and the services provided/required
                    // by entities might have changed)
                    entity->InvalidateDependencies();
                    entity->Activate();
                }

                PreemptiveUndoCache::Get()->UpdateCache(entity->GetId());
            }
        }
    }

    //=========================================================================
    // OnAssetReady
    //=========================================================================
    void EditorEntityContextComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

        AzFramework::EntityContext::EntityList entitiesAdded;

        for (auto iter = m_queuedSliceEntityRestores.begin(); iter != m_queuedSliceEntityRestores.end(); )
        {
            SliceEntityRestoreRequest& request = *iter;
            if (asset.GetId() == request.m_asset.GetId())
            {
                AZ::SliceComponent::SliceInstanceAddress address =
                    GetRootSlice()->RestoreEntity(request.m_entity, request.m_restoreInfo);

                if (address.IsValid())
                {
                    entitiesAdded.push_back(request.m_entity);
                }
                else
                {
                    AZ_Error("EditorEntityContext", false, "Failed to restore entity \"%s\" [%llu]",
                        request.m_entity->GetName().c_str(), request.m_entity->GetId());
                    delete request.m_entity;
                }

                iter = m_queuedSliceEntityRestores.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        HandleEntitiesAdded(entitiesAdded);

        // Pass on to base EntityContext.
        EntityContext::OnAssetReady(asset);
    }

    //=========================================================================
    // OnAssetReloaded - Root slice (or its dependents) has been reloaded.
    //=========================================================================
    void EditorEntityContextComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AzToolsFramework::EntityIdList selectedEntities;
        ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);

        EntityContext::OnAssetReloaded(asset);

        // Ensure selection set is preserved after applying the new level slice.
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selectedEntities);
    }
    //=========================================================================
    // QueuedSliceReplacement::IsValid
    //=========================================================================
    bool EditorEntityContextComponent::QueuedSliceReplacement::IsValid() const
    {
        return !m_path.empty();
    }

    //=========================================================================
    // QueuedSliceReplacement::Reset
    //=========================================================================
    void EditorEntityContextComponent::QueuedSliceReplacement::Reset()
    {
        m_path.clear();
    }

    //=========================================================================
    // QueuedSliceReplacement::OnCatalogAssetAdded
    //=========================================================================
    bool EditorEntityContextComponent::QueuedSliceReplacement::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AZStd::string relativePath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            relativePath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetId);

        if (AZStd::string::npos != AzFramework::StringFunc::Find(m_path.c_str(), relativePath.c_str()))
        {
            // Find the root entity within the supplied list and that was used to create the slice.
            // This entity and its descendants will be replaced by a new instance of the slice.
            AZ::Transform spawnSliceTransform = AZ::Transform::CreateIdentity();
            for (const auto& pair : m_selectedToAssetMap)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(
                    entity, &AZ::ComponentApplicationBus::Events::FindEntity, pair.first);

                if (entity)
                {
                    Components::TransformComponent* transformComponent =
                        entity->FindComponent<Components::TransformComponent>();
                    if (transformComponent)
                    {
                        if (transformComponent->IsRootEntity())
                        {
                            // We need to inherit translation and rotation. Scale are adopted from the slice.
                            spawnSliceTransform = AZ::Transform::CreateTranslation(transformComponent->GetWorldTM().GetTranslation());
                            spawnSliceTransform.SetRotationPartFromQuaternion(transformComponent->GetWorldRotationQuaternion());
                            break;
                        }
                    }
                }
            }

            // Request the slice instantiation.
            AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::SliceAsset>(assetId, false);
            EditorEntityContextRequestBus::BroadcastResult(m_ticket, &EditorEntityContextRequests::InstantiateEditorSlice, asset, spawnSliceTransform);

            return true;
        }

        // Not the asset we're queued to instantiate.
        return false;
    }

    void EditorEntityContextComponent::QueuedSliceReplacement::OnSlicePreInstantiate()
    {
        // Delete the entities from the world that were used to create the slice, since the slice
        // will be instantiated to replace them.
        AZStd::vector<AZ::EntityId> deleteEntityIds;
        deleteEntityIds.reserve(m_entitiesInSelection.size());
        for (const auto& entityToDelete : m_entitiesInSelection)
        {
            deleteEntityIds.push_back(entityToDelete);
        }

        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::DeleteEntities, deleteEntityIds);

        //remove deleted items from dirty list to prevent undo warnings
        for (const auto& entityToDelete : m_entitiesInSelection)
        {
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::RemoveDirtyEntity, entityToDelete);
        }
    }

    //=========================================================================
    // QueuedSliceReplacement::Finalize
    //=========================================================================
    void EditorEntityContextComponent::QueuedSliceReplacement::Finalize(const AZ::SliceComponent::SliceInstanceAddress& instanceAddress)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ::SliceComponent::EntityAncestorList ancestors;
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> remapIds;

        const auto& newEntities = instanceAddress.GetInstance()->GetInstantiated()->m_entities;

        const AZ::Entity* rootEntity = nullptr;

        // Store mapping between live Ids we're out to remove, and the ones now provided by
        // the slice instance, so we can fix up references on any still-external entities.
        // Also finds the root entity of this slice
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityContextComponent::QueuedSliceReplacement::Finalize:CalculateRemapAndFindRoot");
            for (const AZ::Entity* newEntity : newEntities)
            {
                ancestors.clear();
                instanceAddress.GetReference()->GetInstanceEntityAncestry(newEntity->GetId(), ancestors, 1);

                AZ_Error("EditorEntityContext", !ancestors.empty(), "Failed to locate ancestor for newly created slice entity.");
                if (!ancestors.empty())
                {
                    for (const auto& pair : m_selectedToAssetMap)
                    {
                        const AZ::EntityId& ancestorId = ancestors.front().m_entity->GetId();
                        if (pair.second == ancestorId)
                        {
                            remapIds[pair.first] = newEntity->GetId();
                            break;
                        }
                    }
                }

                Components::TransformComponent* transformComponent =
                    newEntity->FindComponent<Components::TransformComponent>();

                if (transformComponent)
                {
                    if (transformComponent->IsRootEntity())
                    {
                        // There should always be ONLY ONE root for any slice, if this assert was hit then the slice is invalid
                        AZ_Assert(!rootEntity, "There cannot be more than one root for any Slice")
                        rootEntity = newEntity;
                    }
                }
            }
        }

        // Set the slice root as a child of the parent after slice replacement and
        // position the slice root at the correct offset and rotation from this new parent
        if (rootEntity)
        {
            Components::TransformComponent* transformComponent =
                rootEntity->FindComponent<Components::TransformComponent>();

            if (transformComponent)
            {
                if (m_parentAfterReplacement.IsValid())
                {
                    transformComponent->SetParentRelative(m_parentAfterReplacement);
                }

                if (m_rootAutoCreated)
                {
                    AZ::Transform sliceRootTM(AZ::Transform::Identity());
                    sliceRootTM.SetTranslation(m_offsetAfterReplacement);
                    sliceRootTM.SetRotationPartFromQuaternion(m_rotationAfterReplacement);
                    transformComponent->SetLocalTM(sliceRootTM);
                }
                else
                {
                    transformComponent->SetWorldTranslation(m_offsetAfterReplacement);
                    transformComponent->SetRotationQuaternion(m_rotationAfterReplacement);
                }
            }
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        // Remap references from outside the slice to the new slice entities (so that other entities don't get into a broken state due to slice creation)
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityContextComponent::QueuedSliceReplacement::Finalize:RemapExternalReferences");
            AZ::SliceComponent* editorRootSlice;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(editorRootSlice, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorRootSlice);
            AZ_Assert(editorRootSlice != nullptr, "Editor root slice not found!");

            AZ::SliceComponent::EntityList editorRootSliceEntities;
            editorRootSlice->GetEntities(editorRootSliceEntities);

            // gather metadata entities
            AZ::SliceComponent::EntityIdSet metadataEntityIds;
            editorRootSlice->GetMetadataEntityIds(metadataEntityIds);
            for (AZ::EntityId metadataEntityId : metadataEntityIds)
            {
                AZ::Entity* metadataEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(metadataEntity, &AZ::ComponentApplicationBus::Events::FindEntity, metadataEntityId);
                if (metadataEntity)
                {
                    editorRootSliceEntities.push_back(metadataEntity);
                }
            }

            for (AZ::Entity* entity : editorRootSliceEntities)
            {
                bool needsReferenceUpdates = false;

                // Deactivating and re-activating an entity is expensive - much more expensive than this check, so we first make sure we need to do
                // any remapping before going through the act of remapping
                AZ::EntityUtils::EnumerateEntityIds(entity,
                    [&needsReferenceUpdates, &remapIds](const AZ::EntityId& id, bool isEntityId, const AZ::SerializeContext::ClassElement* /*elementData*/) -> void
                {
                    if (!isEntityId && id.IsValid())
                    {
                        auto iter = remapIds.find(id);
                        if (iter != remapIds.end())
                        {
                            needsReferenceUpdates = true;
                        }
                    }
                }, serializeContext);

                if (needsReferenceUpdates)
                {
                    entity->Deactivate();

                    AZ::IdUtils::Remapper<AZ::EntityId>::RemapIds(entity,
                        [&remapIds](const AZ::EntityId& originalId, bool /*isEntityId*/, const AZStd::function<AZ::EntityId()>& /*idGenerator*/) -> AZ::EntityId
                        {
                            auto iter = remapIds.find(originalId);
                            if (iter == remapIds.end())
                            {
                                return originalId;
                            }
                            else
                            {
                                return iter->second;
                            }
                        }, serializeContext, false);

                    entity->Activate();
                }
            }
        }


        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnEditorEntitiesReplacedBySlicedEntities, remapIds);

        Reset();
    }

} // namespace AzToolsFramework
