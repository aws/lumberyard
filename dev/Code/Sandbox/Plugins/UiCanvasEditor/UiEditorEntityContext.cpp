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

#include "stdafx.h"

#include "EditorCommon.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/UiComponentTypes.h>

#include "UiEditorEntityContext.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
UiEditorEntityContext::UiEditorEntityContext(EditorWindow* editorWindow)
    : m_editorWindow(editorWindow)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiEditorEntityContext::~UiEditorEntityContext()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::HandleLoadedRootSliceEntity(AZ::Entity* rootEntity, bool remapIds, AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable)
{
    AZ_Assert(m_rootAsset, "The context has not been initialized.");

    if (!AzFramework::EntityContext::HandleLoadedRootSliceEntity(rootEntity, remapIds, idRemapTable))
    {
        return false;
    }

    AZ::SliceComponent::EntityList entities;
    GetRootSlice()->GetEntities(entities);

    GetRootSlice()->SetIsDynamic(true);

    InitializeEntities(entities);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::InitUiContext()
{
    InitContext();

    GetRootSlice()->Instantiate();

    UiEntityContextRequestBus::Handler::BusConnect(GetContextId());

    UiEditorEntityContextRequestBus::Handler::BusConnect(GetContextId());

    AzToolsFramework::EditorEntityContextPickingRequestBus::Handler::BusConnect(GetContextId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::DestroyUiContext()
{
    UiEditorEntityContextRequestBus::Handler::BusDisconnect();

    UiEntityContextRequestBus::Handler::BusDisconnect();

    AzToolsFramework::EditorEntityContextPickingRequestBus::Handler::BusDisconnect();

    DestroyContext();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType)
{
    AZ::Entity targetSliceEntity;
    AZ::SliceComponent* targetSlice = targetSliceEntity.CreateComponent<AZ::SliceComponent>();

    // Prepare entities for export. This involves invoking BuildGameEntity on source
    // entity's components, targeting a separate entity for export.
    AZ::SliceComponent::EntityList sourceEntities;
    GetRootSlice()->GetEntities(sourceEntities);
    for (AZ::Entity* entity : sourceEntities)
    {
        AZ::Entity* exportEntity = aznew AZ::Entity(entity->GetName().c_str());
        exportEntity->SetId(entity->GetId());
        AZ_Assert(exportEntity, "Failed to create target entity \"%s\" for export.",
            entity->GetName().c_str());

        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, PreExportEntity,
            *entity,
            *exportEntity);

        targetSlice->AddEntity(exportEntity);
    }

    AZ::SliceComponent::EntityList targetEntities;
    targetSlice->GetEntities(targetEntities);

    // Export runtime slice representing the entities in this canvas, which is a completely flat list of entities.
    AZ::Utils::SaveObjectToStream<AZ::Entity>(stream, streamType, &targetSliceEntity);

    AZ_Assert(targetEntities.size() == sourceEntities.size(),
        "Entity export list size must match that of the import list.");

    // Finalize entities for export. This will remove any export components temporarily
    // assigned by the source entity's components.
    auto sourceIter = sourceEntities.begin();
    auto targetIter = targetEntities.begin();
    for (; sourceIter != sourceEntities.end(); ++sourceIter, ++targetIter)
    {
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, PostExportEntity,
            *(*sourceIter),
            *(*targetIter));
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::SaveCanvasEntityToStreamForGame(AZ::Entity* canvasEntity, AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType)
{
    AZ::Entity* sourceCanvasEntity = canvasEntity;
    AZ::Entity* exportCanvasEntity = aznew AZ::Entity(sourceCanvasEntity->GetName().c_str());
    exportCanvasEntity->SetId(sourceCanvasEntity->GetId());
    AZ_Assert(exportCanvasEntity, "Failed to create target entity \"%s\" for export.",
        sourceCanvasEntity->GetName().c_str());

    EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, PreExportEntity,
        *sourceCanvasEntity,
        *exportCanvasEntity);

    // Export entity representing the canvas, which has only runtime components.
    AZ::Utils::SaveObjectToStream<AZ::Entity>(stream, streamType, exportCanvasEntity);

    EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, PostExportEntity,
        *sourceCanvasEntity,
        *exportCanvasEntity);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiEditorEntityContext::CreateUiEntity(const char* name)
{
    AZ::Entity* entity = CreateEntity(name);

    if (entity)
    {
        // we don't currently do anything extra here, UI entities are not automatically
        // Init'ed and Activate'd when they are created. We wait until the required components
        // are added before Init and Activate
    }

    return entity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::AddUiEntity(AZ::Entity* entity)
{
    AZ_Assert(entity, "Supplied entity is invalid.");

    AddEntity(entity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::AddUiEntities(const AzFramework::EntityContext::EntityList& entities)
{
    AZ::SliceAsset* rootSlice = m_rootAsset.Get();

    for (AZ::Entity* entity : entities)
    {
        AZ_Assert(!AzFramework::EntityIdContextQueryBus::MultiHandler::BusIsConnectedId(entity->GetId()), "Entity already in context.");
        rootSlice->GetComponent()->AddEntity(entity);
    }

    HandleEntitiesAdded(entities);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::CloneUiEntities(const AZStd::vector<AZ::EntityId>& sourceEntities, AzFramework::EntityContext::EntityList& resultEntities)
{
    resultEntities.clear();

    AZ::SliceComponent::InstantiatedContainer sourceObjects(false);
    for (const AZ::EntityId& id : sourceEntities)
    {
        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, id);
        if (entity)
        {
            sourceObjects.m_entities.push_back(entity);
        }
    }

    AZ::SliceComponent::EntityIdToEntityIdMap idMap;
    AZ::SliceComponent::InstantiatedContainer* clonedObjects =
        AZ::EntityUtils::CloneObjectAndFixEntities(&sourceObjects, idMap);
    if (!clonedObjects)
    {
        AZ_Error("UiEntityContext", false, "Failed to clone source entities.");
        return false;
    }

    resultEntities = clonedObjects->m_entities;

    AddUiEntities(resultEntities);

    clonedObjects->m_deleteEntitiesOnDestruction = false;
    delete clonedObjects;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::DestroyUiEntity(AZ::EntityId entityId)
{
    return DestroyEntityById(entityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::SupportsViewportEntityIdPicking()
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::SliceComponent::SliceInstanceAddress UiEditorEntityContext::CloneEditorSliceInstance(
    AZ::SliceComponent::SliceInstanceAddress sourceInstance)
{
    return AZ::SliceComponent::SliceInstanceAddress();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiEditorEntityContext::InstantiateEditorSlice(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, AZ::Vector2 viewportPosition)
{
    if (sliceAsset.GetId().IsValid())
    {
        m_instantiatingSlices.push_back(AZStd::make_pair(sliceAsset, viewportPosition));

        const AzFramework::SliceInstantiationTicket ticket = InstantiateSlice(sliceAsset);
        if (ticket)
        {
            AzFramework::SliceInstantiationResultBus::MultiHandler::BusConnect(ticket);
        }

        return ticket;
    }

    return AzFramework::SliceInstantiationTicket();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::RestoreSliceEntity(AZ::Entity* entity, const AZ::SliceComponent::EntityRestoreInfo& info)
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
    
////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::QueueSliceReplacement(const char* targetPath,
                                                  const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& selectedToAssetMap,
                                                  const AZStd::unordered_set<AZ::EntityId>& entitiesInSelection,
                                                  AZ::Entity* commonParent, AZ::Entity* insertBefore)
{
    AZ_Error("EditorEntityContext", m_queuedSliceReplacement.m_path.empty(), "A slice replacement is already on the queue.");

    m_queuedSliceReplacement.Setup(targetPath, selectedToAssetMap, entitiesInSelection, commonParent, insertBefore);

    AzFramework::AssetCatalogEventBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::DeleteElements(AzToolsFramework::EntityIdList elements)
{
    // Deletes the specified elements using an undoable command
    if (elements.size() > 0)
    {
        HierarchyWidget* hierarchy = m_editorWindow->GetHierarchy();

        // Get the list of currently selected entities so that we can attempt to restore that
        // after the delete (the undoable command currently only works on selected entities)
        QTreeWidgetItemRawPtrQList selection = hierarchy->selectedItems();
        EntityHelpers::EntityIdList selectedEntities = SelectionHelpers::GetSelectedElementIds(hierarchy, selection, false);

        // Use an undoable command to delete the entities
        // The way the command is implemented depends upon selecting the items first
        HierarchyHelpers::SetSelectedItems(hierarchy, &elements);
        CommandHierarchyItemDelete::Push(m_editorWindow->GetActiveStack(),
            hierarchy,
            hierarchy->selectedItems());

        // Attempt to set the selection back to what it was but first remove any items from the selected
        // list that no longer exist
        selectedEntities.erase(
            std::remove_if(
                selectedEntities.begin(), selectedEntities.end(),
                [](AZ::EntityId entityId)
                {
                    AZ::Entity* entity = nullptr;
                    EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);
                    return !entity;
                }),
            selectedEntities.end());

        HierarchyHelpers::SetSelectedItems(hierarchy, &selectedEntities);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::HasPendingRequests()
{
    if (!m_queuedSliceEntityRestores.empty())
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::IsInstantiatingSlices()
{    
    if (!m_instantiatingSlices.empty())
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::DetachSliceEntities(const AzToolsFramework::EntityIdList& entities)
{
    if (entities.empty())
    {
        return;
    }

    for (const AZ::EntityId& entityId : entities)
    {
        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
        EBUS_EVENT_ID_RESULT(sliceAddress, entityId, AzFramework::EntityIdContextQueryBus, GetOwningSlice);

        if (sliceAddress.IsValid())
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            AZ_Error("EditorEntityContext", entity, "Unable to find entity for EntityID %llu", entityId);

            if (entity)
            {
                if (sliceAddress.GetReference()->GetSliceComponent()->RemoveEntity(entityId, false)) // Remove from current slice instance without deleting
                {
                    GetRootSlice()->AddEntity(entity); // Add back as loose entity
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
{
    if (m_queuedSliceReplacement.IsValid())
    {
        AZStd::string relativePath;
        EBUS_EVENT_RESULT(relativePath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, assetId);

        if (AZStd::string::npos != AzFramework::StringFunc::Find(m_queuedSliceReplacement.m_path.c_str(), relativePath.c_str()))
        {
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();

            AZStd::unordered_set<AZ::EntityId> topLevelEntities;
            GetTopLevelEntities(m_queuedSliceReplacement.m_entitiesInSelection, topLevelEntities);

            // Request the slice instantiation.
            AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::SliceAsset>(assetId, false);
            AZ::Vector2 viewportPosition(-1.0f, -1.0f);
            m_queuedSliceReplacement.m_ticket = InstantiateEditorSlice(asset, viewportPosition);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::ResetContext()
{
    // First deactivate all the entities, before calling the base class ResetContext which will
    // delete them all.
    // This helps us know that we do not need to maintain the cached pointers between the UiElementComponents
    // as individual elements are destroyed.
    AZ::SliceComponent::EntityList entities;
    bool result = GetRootSlice()->GetEntities(entities);
    if (result)
    {
        for (AZ::Entity* entity : entities)
        {
            if (entity->GetState() == AZ::Entity::ES_ACTIVE)
            {
                entity->Deactivate();
            }
        }
    }

    // Now reset the context which will destroy all the entities
    EntityContext::ResetContext();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
{
    // For UI slices we don't need to do anything here. The main EditorEntityContextComponent
    // changes the transforms here. But we need the entities to be initialized and activated
    // before recalculating offsets so we do it in OnSliceInstantiated.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
{
    const AzFramework::SliceInstantiationTicket& ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

    // If we got here by creating a new slice then we have extra work to do (deleting the old entities etc)
    AZ::Entity* insertBefore = nullptr;
    if (ticket == m_queuedSliceReplacement.m_ticket)
    {
        m_queuedSliceReplacement.Finalize(sliceAddress, m_editorWindow);

        // Select the common parent (the call to Finalize will have deleted the elements that were selected)
        m_editorWindow->GetHierarchy()->SetUniqueSelectionHighlight(m_queuedSliceReplacement.m_commonParent);
        insertBefore = m_queuedSliceReplacement.m_insertBefore;
    }

    AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

    // Close out the next ticket corresponding to this asset.
    for (auto instantiatingIter = m_instantiatingSlices.begin(); instantiatingIter != m_instantiatingSlices.end(); ++instantiatingIter)
    {
        if (instantiatingIter->first.GetId() == sliceAssetId)
        {
            const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;

            if (entities.size() == 0)
            {
                // if there are no entities there was an error with the instantiation
                UiEditorEntityContextNotificationBus::Broadcast(&UiEditorEntityContextNotificationBus::Events::OnSliceInstantiationFailed, sliceAssetId, ticket);
                m_instantiatingSlices.erase(instantiatingIter);
                break;
            }

            // Initialize the new entities and create a set of all the top-level entities.
            AZStd::unordered_set<AZ::Entity*> topLevelEntities;
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

                topLevelEntities.insert(entity);
            }

            // remove anything from the topLevelEntities set that is referenced as the child of another element in the list
            for (AZ::Entity* entity : entities)
            {
                LyShine::EntityArray children;
                EBUS_EVENT_ID_RESULT(children, entity->GetId(), UiElementBus, GetChildElements);
                
                for (auto child : children)
                {
                    topLevelEntities.erase(child);
                }
            }

            // This can be null if nothing is selected. That is OK, the usage of it below treats that as meaning
            // add as a child of the root element.
            AZ::Entity* parent = m_editorWindow->GetHierarchy()->CurrentSelectedElement();

            // Now topLevelElements contains all of the top-level elements in the set of newly instantiated entities
            // Copy the topLevelEntities set into a list
            LyShine::EntityArray entitiesToInit;
            for (auto entity : topLevelEntities)
            {
                entitiesToInit.push_back(entity);
            }

            // There must be at least one element
            AZ_Assert(entitiesToInit.size() >= 1, "There must be at least one top-level entity in a UI slice.");

            // Initialize the internal parent pointers and the canvas pointer in the elements
            // We do this before adding the elements, otherwise the GetUniqueChildName code in FixupCreatedEntities will
            // already see the new elements and think the names are not unique
            EBUS_EVENT_ID(m_editorWindow->GetCanvas(), UiCanvasBus, FixupCreatedEntities, entitiesToInit, true, parent);

            // Add all of the top-level entities as children of the parent
            for (auto entity : topLevelEntities)
            {
                EBUS_EVENT_ID(m_editorWindow->GetCanvas(), UiCanvasBus, AddElement, entity, parent, insertBefore);
            }

            // Here we adjust the position of the instantiated entities so that if the slice was instantiated from the
            // viewport menu we instantiate it at the mouse position
            AZ::Vector2 desiredViewportPosition = instantiatingIter->second;
            if (desiredViewportPosition != AZ::Vector2(-1.0f, -1.0f))
            {
                // This is the same behavior as the old "Add elements from prefab" had.

                AZ::Entity* rootElement = entitiesToInit[0];

                // Transform pivot position to canvas space
                AZ::Vector2 pivotPos;
                EBUS_EVENT_ID_RESULT(pivotPos, rootElement->GetId(), UiTransformBus, GetCanvasSpacePivotNoScaleRotate);

                // Transform destination position to canvas space
                AZ::Matrix4x4 transformFromViewport;
                EBUS_EVENT_ID(rootElement->GetId(), UiTransformBus, GetTransformFromViewport, transformFromViewport);
                AZ::Vector3 destPos3 = transformFromViewport * AZ::Vector3(desiredViewportPosition.GetX(), desiredViewportPosition.GetY(), 0.0f);
                AZ::Vector2 destPos(destPos3.GetX(), destPos3.GetY());

                AZ::Vector2 offsetDelta = destPos - pivotPos;

                // Adjust offsets on all top level elements
                for (auto entity : entitiesToInit)
                {
                    UiTransform2dInterface::Offsets offsets;
                    EBUS_EVENT_ID_RESULT(offsets, entity->GetId(), UiTransform2dBus, GetOffsets);
                    EBUS_EVENT_ID(entity->GetId(), UiTransform2dBus, SetOffsets, offsets + offsetDelta);
                }
            }

            // the entities have already been created but we need to make an undo command that can undo/redo that action
            HierarchyWidget* hierarchyWidget = m_editorWindow->GetHierarchy();

            QTreeWidgetItemRawPtrQList selectedItems = hierarchyWidget->selectedItems();

            // use an undoable command to create the elements from the slice
            CommandHierarchyItemCreateFromData::Push(m_editorWindow->GetActiveStack(),
                hierarchyWidget,
                selectedItems,
                true,
                [ topLevelEntities ](HierarchyItem* parent, LyShine::EntityArray& listOfNewlyCreatedTopLevelElements)
                {
                    for (AZ::Entity* entity : topLevelEntities)
                    {
                        listOfNewlyCreatedTopLevelElements.push_back(entity);
                    }
                },
                "Instantiate Slice");

            m_instantiatingSlices.erase(instantiatingIter);

            EBUS_EVENT(UiEditorEntityContextNotificationBus, OnSliceInstantiated, sliceAssetId, sliceAddress, ticket);

            break;
        }
    }
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
{
    const AzFramework::SliceInstantiationTicket& ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

    AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

    for (auto instantiatingIter = m_instantiatingSlices.begin(); instantiatingIter != m_instantiatingSlices.end(); ++instantiatingIter)
    {
        if (instantiatingIter->first.GetId() == sliceAssetId)
        {
            EBUS_EVENT(UiEditorEntityContextNotificationBus, OnSliceInstantiationFailed, sliceAssetId, ticket);

            m_instantiatingSlices.erase(instantiatingIter);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
{
    AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

    for (auto iter = m_queuedSliceEntityRestores.begin(); iter != m_queuedSliceEntityRestores.end(); )
    {
        SliceEntityRestoreRequest& request = *iter;
        if (asset.GetId() == request.m_asset.GetId())
        {
            AZ::SliceComponent::SliceInstanceAddress address = 
                GetRootSlice()->RestoreEntity(request.m_entity, request.m_restoreInfo);

            // Note that we do not add the entity to the context/rootSlice using AddEntity here.
            // This is because it has already been added to the root slice as a prefab instance.
            // Instead we call HandleEntitiesAdded which just adds it to the context
            if (address.IsValid())
            {
                HandleEntitiesAdded({request.m_entity});
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

    // Pass on to base EntityContext.
    EntityContext::OnAssetReady(asset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Root slice (or its dependents) has been reloaded.
void UiEditorEntityContext::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
{
    bool isActive = false;
    if (m_editorWindow->GetEntityContext() && m_editorWindow->GetEntityContext()->GetContextId() == GetContextId())
    {
        isActive = true;
    }

    HierarchyWidget* hierarchy = nullptr;
    EntityHelpers::EntityIdList selectedEntities;
    if (isActive)
    {
        hierarchy = m_editorWindow->GetHierarchy();
        const QTreeWidgetItemRawPtrQList& selection = hierarchy->selectedItems();
        selectedEntities = SelectionHelpers::GetSelectedElementIds(hierarchy, selection, false);

        // This ensures there's no "current item".
        hierarchy->SetUniqueSelectionHighlight((QTreeWidgetItem*)nullptr);

        // IMPORTANT: This is necessary to indirectly trigger detach()
        // in the PropertiesWidget.
        hierarchy->SetUserSelection(nullptr);
    }

    EntityContext::OnAssetReloaded(asset);

    EBUS_EVENT_ID(m_editorWindow->GetCanvasForEntityContext(GetContextId()), UiCanvasBus, ReinitializeElements);

    if (isActive)
    {
        // Ensure selection set is preserved after applying the new level slice.
        // But make sure we don't add any EntityId to selection that no longer exists as that cause a crash later
        selectedEntities.erase(
            std::remove_if(
                selectedEntities.begin(), selectedEntities.end(),
                [](AZ::EntityId entityId)
                {
                    AZ::Entity* entity = nullptr;
                    EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);
                    return !entity;
                }),
            selectedEntities.end());
    
        // Refresh the Hierarchy pane
        LyShine::EntityArray childElements;
        EBUS_EVENT_ID_RESULT(childElements, m_editorWindow->GetCanvas(), UiCanvasBus, GetChildElements);
        hierarchy->RecreateItems(childElements);

        HierarchyHelpers::SetSelectedItems(hierarchy, &selectedEntities);
    }

    // We want to update the status for any tabs being used to edit slices.
    // If that tab has just done a push, we want to check at this point whether there are any differences between the
    // reloaded asset and the instance.
    m_editorWindow->UpdateChangedStatusOnAssetChange(GetContextId(), asset);
}

//////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnContextEntitiesAdded(const EntityList& entities)
{
    EntityContext::OnContextEntitiesAdded(entities);

    InitializeEntities(entities);
}

//////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::ValidateEntitiesAreValidForContext(const EntityList& entities)
{
    // All entities in a slice being instantiated in the UI editor should
    // have the UiElementComponent on them.
    for (AZ::Entity* entity : entities)
    {
        if (!entity->FindComponent(LyShine::UiElementComponentUuid))
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::SetupUiEntity(AZ::Entity* entity)
{
    InitializeEntities({ entity });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::InitializeEntities(const AzFramework::EntityContext::EntityList& entities)
{
    // UI entities are now automatically activated on creation

    for (AZ::Entity* entity : entities)
    {
        if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
        {
            entity->Init();
        }
    }

    for (AZ::Entity* entity : entities)
    {
        if (entity->GetState() == AZ::Entity::ES_INIT)
        {
            entity->Activate();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::GetTopLevelEntities(const AZStd::unordered_set<AZ::EntityId>& entities, AZStd::unordered_set<AZ::EntityId>& topLevelEntities)
{
    for (auto entityId : entities)
    {
        // if this entities parent is not in the set then it is a top-level
        AZ::Entity* parentElement = nullptr;
        EBUS_EVENT_ID_RESULT(parentElement, entityId, UiElementBus, GetParent);

        if (!parentElement || entities.count(parentElement->GetId()) == 0)
        {
            topLevelEntities.insert(entityId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::QueuedSliceReplacement::IsValid() const
{
    return !m_path.empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::QueuedSliceReplacement::Reset()
{
    m_path.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::QueuedSliceReplacement::Finalize(
    const AZ::SliceComponent::SliceInstanceAddress& instanceAddress,
    EditorWindow* editorWindow)
{
    AZ::SliceComponent::EntityAncestorList ancestors;
    AZStd::unordered_map<AZ::EntityId, AZ::EntityId> remapIds;

    const auto& newEntities = instanceAddress.GetInstance()->GetInstantiated()->m_entities;

    // Store mapping between live Ids we're out to remove, and the ones now provided by
    // the slice instance, so we can fix up references on any still-external entities.
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
    }

    AZ::SerializeContext* serializeContext = nullptr;
    EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        
    // Remap references on any entities left out of the slice, to any entities in the slice instance.
    for (const AZ::EntityId& selectedId : m_entitiesInSelection)
    {
        if (m_selectedToAssetMap.find(selectedId) != m_selectedToAssetMap.end())
        {
            // Entity is included in the slice; no need to patch.
            continue;
        }

        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, selectedId);

        AZ_Error("EditorEntityContext", entity, "Failed to locate live entity during slice replacement.");

        if (entity)
        {
            entity->Deactivate();

            AZ::EntityUtils::ReplaceEntityRefs(entity,
                [&remapIds](const AZ::EntityId& originalId, bool /*isEntityId*/) -> AZ::EntityId
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
                }, serializeContext);

            entity->Activate();
        }
    }

    // Delete the entities from the world that were used to create the slice, since the slice
    // will be instantiated to replace them.

    AZStd::vector<AZ::EntityId> deleteEntityIds;
    deleteEntityIds.reserve(m_selectedToAssetMap.size());
    for (const auto& pair : m_selectedToAssetMap)
    {
        deleteEntityIds.push_back(pair.first);
    }

    // Use an undoable command to delete the entities
    HierarchyWidget* hierarchy = editorWindow->GetHierarchy();

    CommandHierarchyItemDelete::Push(editorWindow->GetActiveStack(),
        hierarchy,
        hierarchy->selectedItems());

    // This ensures there's no "current item".
    hierarchy->SetUniqueSelectionHighlight((QTreeWidgetItem*)nullptr);

    // IMPORTANT: This is necessary to indirectly trigger detach()
    // in the PropertiesWidget.
    hierarchy->SetUserSelection(nullptr);

    Reset();
}
