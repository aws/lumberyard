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

#include "EditorEntityModel.h"
#include "EditorEntitySortBus.h"

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceMetadataInfoBus.h>

#include <AzCore/std/sort.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>

namespace AzToolsFramework
{
    EditorEntityModel::EditorEntityModel()
    {
        ToolsApplicationEvents::Bus::Handler::BusConnect();
        EditorEntityContextNotificationBus::Handler::BusConnect();
        EditorMetricsEventsBus::Handler::BusConnect();

        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequestBus::Events::GetEditorEntityContextId);
        if (!editorEntityContextId.IsNull())
        {
            AzFramework::EntityContextEventBus::Handler::BusConnect(editorEntityContextId);
        }

        Reset();
    }

    EditorEntityModel::~EditorEntityModel()
    {
        AzFramework::EntityContextEventBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        EditorMetricsEventsBus::Handler::BusDisconnect();
    }

    void EditorEntityModel::Reset()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        //disconnect all entity ids
        EditorEntitySortNotificationBus::MultiHandler::BusDisconnect();

        //resetting model content and state, notifying any observers
        EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoResetBegin);

        m_enableChildReorderHandler = false;

        m_entityInfoTable.clear();
        m_entityOrphanTable.clear();

        ClearQueuedEntityAdds();

        //AZ::EntityId::InvalidEntityId is the value of the parent entity id for any non-parented entity
        //registering AZ::EntityId::InvalidEntityId as a ghost root entity so requests can be made without special cases
        AddEntity(AZ::EntityId());

        m_enableChildReorderHandler = true;

        EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoResetEnd);
    }

    namespace Internal
    {
        // BasicEntityData is used by ProcessQueuedEntityAdds() to sort entities
        // before their EditorEntityModelEntry is created.
        struct BasicEntityData
        {
            AZ::EntityId m_entityId;
            AZ::EntityId m_parentId;
            AZ::u64      m_indexForSorting = std::numeric_limits<AZ::u64>::max();

            bool operator< (const BasicEntityData& rhs) const
            {
                if (m_indexForSorting < rhs.m_indexForSorting)
                {
                    return true;
                }
                if (m_indexForSorting > rhs.m_indexForSorting)
                {
                    return false;
                }
                return m_entityId < rhs.m_entityId;
            }
        };
    } // namespace Internal

    void EditorEntityModel::ProcessQueuedEntityAdds()
    {
        using Internal::BasicEntityData;

        // Clear the queued entity data
        AZStd::unordered_set<AZ::EntityId> unsortedEntitiesToAdd = AZStd::move(m_queuedEntityAdds);
        ClearQueuedEntityAdds();

        // Add pending entities in breadth-first order.
        // This prevents temporary orphans and the need to re-sort children,
        // which can have a serious performance impact in large levels.
        AZStd::vector<AZ::EntityId> sortedEntitiesToAdd;
        sortedEntitiesToAdd.reserve(unsortedEntitiesToAdd.size());

        { // Sort pending entities
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityModel::AddEntityBatch:Sort");

            // Gather basic sorting data for each pending entity and
            // create map from parent ID to child entries.
            AZStd::unordered_map<AZ::EntityId, AZStd::vector<BasicEntityData>> parentIdToChildData;
            for (AZ::EntityId entityId : unsortedEntitiesToAdd)
            {
                BasicEntityData entityData;
                entityData.m_entityId = entityId;
                AZ::TransformBus::EventResult(entityData.m_parentId, entityId, &AZ::TransformBus::Events::GetParentId);

                // Can the parent be found?
                if (GetInfo(entityData.m_parentId).IsConnected() ||
                    unsortedEntitiesToAdd.find(entityData.m_parentId) != unsortedEntitiesToAdd.end())
                {
                    EditorEntitySortRequestBus::EventResult(entityData.m_indexForSorting, GetEntityIdForSortInfo(entityData.m_parentId), &EditorEntitySortRequestBus::Events::GetChildEntityIndex, entityId);
                }
                else
                {
                    // For the sake of sorting, treat orphaned entities as children of the root node.
                    // Note that the sort-index remains very high, so they'll be added after valid children.
                    entityData.m_parentId.SetInvalid();
                }

                parentIdToChildData[entityData.m_parentId].emplace_back(entityData);
            }

            // Sort siblings
            for (auto parentChildrenPair : parentIdToChildData)
            {
                AZStd::vector<BasicEntityData>& children = parentChildrenPair.second;

                AZStd::sort(children.begin(), children.end());
            }

            // General algorithm for breadth-first search is:
            // - create a list, populated with any root entities
            // - in a loop:
            // -- grab next entity in list
            // -- pushing its children to the back of the list,

            // Find root entities. Note that the pending entities might not all share a common root.
            AZStd::vector<AZ::EntityId> parentsToInspect;
            parentsToInspect.reserve(unsortedEntitiesToAdd.size() + 1); // Reserve space for pending + roots. Guess that there's one root.

            for (auto& parentChildrenPair : parentIdToChildData)
            {
                AZ::EntityId parentId = parentChildrenPair.first;

                if (unsortedEntitiesToAdd.find(parentId) == unsortedEntitiesToAdd.end())
                {
                    parentsToInspect.emplace_back(parentId);
                }
            }

            // Sort roots. This isn't necessary for correctness, but ensures a stable sort.
            AZStd::sort(parentsToInspect.begin(), parentsToInspect.end());

            parentsToInspect.reserve(parentsToInspect.size() + unsortedEntitiesToAdd.size());

            // Inspect entities in a loop...
            for (size_t i = 0; i < parentsToInspect.size(); ++i)
            {
                AZ::EntityId parentId = parentsToInspect[i];

                auto foundChildren = parentIdToChildData.find(parentId);
                if (foundChildren != parentIdToChildData.end())
                {
                    AZStd::vector<BasicEntityData>& children = foundChildren->second;

                    for (const BasicEntityData& childData : children)
                    {
                        parentsToInspect.emplace_back(childData.m_entityId);
                        sortedEntitiesToAdd.emplace_back(childData.m_entityId);
                    }

                    parentIdToChildData.erase(foundChildren);
                }
            }

            // Grab any pending entities that failed to sort.
            // This should only happen in edge-cases (ex: circular parent linkage)
            if (!parentIdToChildData.empty())
            {
                AZ_Assert(0, "Couldn't pre-sort all new entities.");
                for (auto& parentChildrenPair : parentIdToChildData)
                {
                    AZStd::vector<BasicEntityData>& children = parentChildrenPair.second;

                    for (BasicEntityData& child : children)
                    {
                        sortedEntitiesToAdd.emplace_back(child.m_entityId);
                    }
                }
            }
        }

        { // Add sorted entities
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityModel::AddEntityBatch:Add");
            for (AZ::EntityId entityId : sortedEntitiesToAdd)
            {
                AddEntity(entityId);
            }
        }
    }

    void EditorEntityModel::ClearQueuedEntityAdds()
    {
        m_queuedEntityAdds.clear();
        m_queuedEntityAddsNotYetActivated.clear();
        AZ::EntityBus::MultiHandler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void EditorEntityModel::AddEntity(AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        auto& entityInfo = GetInfo(entityId);

        //initialize and connect this entry to the entity id
        entityInfo.Connect();

        if (entityId.IsValid())
        {
            AddChildToParent(entityInfo.GetParent(), entityId);
        }

        //re-parent any orphaned entities that may have either
        //1) been orphaned by a parent that was destroyed without destroying children
        //2) been temporarily attached to the "root" because parent didn't yet exist (during load/undo/redo)
        AZStd::unordered_set<AZ::EntityId> children;
        AZStd::swap(children, m_entityOrphanTable[entityId]);
        for (auto childId : children)
        {
            // When an orphan child entity is added before its transform parent, the orphan child entity 
            // will be added to the root entity (root entity has an invalid EntityId) first and then 
            // reparented to its real transform parent later after the parent is added. Therefore if the 
            // root entity doesn't contain the orphan child entity, it means the orphan child entity has 
            // not been added, so we skip reparenting.
            AZ::EntityId rootEntityId = AZ::EntityId();
            auto& parentInfo = GetInfo(rootEntityId);
            if (parentInfo.HasChild(childId))
            {
                ReparentChild(childId, entityId, AZ::EntityId());
            }
        }

        EditorEntitySortNotificationBus::MultiHandler::BusConnect(entityId);
    }

    void EditorEntityModel::RemoveEntity(AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        auto& entityInfo = GetInfo(entityId);
        if (!entityInfo.IsConnected())
        {
            return;
        }

        //orphan any children that remain attached to the entity
        auto children = entityInfo.GetChildren();
        for (auto childId : children)
        {
            ReparentChild(childId, AZ::EntityId(), entityId);
            m_entityOrphanTable[entityId].insert(childId);
        }

        m_savedOrderInfo[entityId] = AZStd::make_pair(entityInfo.GetParent(), entityInfo.GetIndexForSorting());

        if (entityId.IsValid())
        {
            RemoveChildFromParent(entityInfo.GetParent(), entityId);
        }

        //disconnect entry from any buses
        entityInfo.Disconnect();

        EditorEntitySortNotificationBus::MultiHandler::BusDisconnect(entityId);
    }

    void EditorEntityModel::AddChildToParent(AZ::EntityId parentId, AZ::EntityId childId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        AZ_Assert(childId != parentId, "AddChildToParent called with same child and parent");
        if (childId == parentId || !childId.IsValid())
        {
            return;
        }

        auto& childInfo = GetInfo(childId);
        auto& parentInfo = GetInfo(parentId);
        if (!parentInfo.IsConnected())
        {
            AddChildToParent(AZ::EntityId(), childId);
            m_entityOrphanTable[parentId].insert(childId);
            return;
        }

        childInfo.SetParent(parentId);

        //creating/pushing slices doesn't always destroy/de-register the original entity before adding the replacement
        if (!parentInfo.HasChild(childId))
        {
            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedAddChildBegin, parentId, childId);

            parentInfo.AddChild(childId);

            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedAddChildEnd, parentId, childId);
        }
        
        AZStd::unordered_map<AZ::EntityId, AZStd::pair<AZ::EntityId, AZ::u64>>::const_iterator orderItr = m_savedOrderInfo.find(childId);
        if (orderItr != m_savedOrderInfo.end() && orderItr->second.first == parentId)
        {
            AzToolsFramework::RecoverEntitySortInfo(parentId, childId, orderItr->second.second);
            m_savedOrderInfo.erase(childId);
        }
        else
        {
            AzToolsFramework::AddEntityIdToSortInfo(parentId, childId, m_forceAddToBack);
        }
        childInfo.UpdateSliceInfo();
        childInfo.UpdateOrderInfo(true);
    }

    void EditorEntityModel::RemoveChildFromParent(AZ::EntityId parentId, AZ::EntityId childId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        AZ_Assert(childId != parentId, "RemoveChildFromparent called with same child and parent");
        AZ_Assert(childId.IsValid(), "RemoveChildFromparent called with an invalid child entity id");
        if (childId == parentId || !childId.IsValid())
        {
            return;
        }

        auto& childInfo = GetInfo(childId);

        //always override the parent to match internal state
        m_entityOrphanTable[parentId].erase(childId);
        parentId = childInfo.GetParent();
        m_entityOrphanTable[parentId].erase(childId);

        auto& parentInfo = GetInfo(parentId);
        if (parentInfo.IsConnected())
        {
            //creating/pushing slices doesn't always destroy/de-register the original entity before adding the replacement
            if (parentInfo.HasChild(childId))
            {
                EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedRemoveChildBegin, parentId, childId);

                parentInfo.RemoveChild(childId);

                EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedRemoveChildEnd, parentId, childId);
            }
        }
        AzToolsFramework::RemoveEntityIdFromSortInfo(parentId, childId);

        childInfo.SetParent(AZ::EntityId());
        childInfo.UpdateSliceInfo();
        childInfo.UpdateOrderInfo(false);
    }

    void EditorEntityModel::ReparentChild(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        AZ_Assert(oldParentId != entityId, "ReparentChild gave us an oldParentId that is the same as the entityId. An entity cannot be a parent of itself, ignoring old parent");
        AZ_Assert(newParentId != entityId, "ReparentChild gave us an newParentId that is the same as the entityId. An entity cannot be a parent of itself, ignoring old parent");
        if (oldParentId != entityId && newParentId != entityId)
        {
            RemoveChildFromParent(oldParentId, entityId);
            AddChildToParent(newParentId, entityId);
        }
    }

    EditorEntityModel::EditorEntityModelEntry& EditorEntityModel::GetInfo(const AZ::EntityId& entityId)
    {
        //retrieve or add an entity entry to the table
        //the entry must exist, even if not connected, so children and other data can be assigned
        auto& entityInfo = m_entityInfoTable[entityId];

        //the entity id defaults to invalid and must be set to match the requested id
        //disconnect and reassign if there's a mismatch
        if (entityInfo.GetId() != entityId)
        {
            entityInfo.Disconnect();
            entityInfo.SetId(entityId);
        }
        return entityInfo;
    }

    void EditorEntityModel::EntityRegistered(AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        //when an editor entity is created and registered, add it to a pending list.
        //once all entities in the pending list are activated, add them to model.
        bool isEditorEntity = false;
        EBUS_EVENT_RESULT(isEditorEntity, EditorEntityContextRequestBus, IsEditorEntity, entityId);
        if (isEditorEntity)
        {
            // As an optimization, new entities are queued.
            // They're added to the model once they've all activated, or the next tick happens.
            m_queuedEntityAdds.insert(entityId);
            m_queuedEntityAddsNotYetActivated.insert(entityId);
            AZ::EntityBus::MultiHandler::BusConnect(entityId);
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void EditorEntityModel::EntityDeregistered(AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        //when an editor entity is de-registered, stop tracking it
        if (m_entityInfoTable.find(entityId) != m_entityInfoTable.end())
        {
            RemoveEntity(entityId);
        }

        if (m_queuedEntityAdds.find(entityId) != m_queuedEntityAdds.end())
        {
            m_queuedEntityAdds.erase(entityId);
            m_queuedEntityAddsNotYetActivated.erase(entityId);
            AZ::EntityBus::MultiHandler::BusDisconnect(entityId);

            // If deregistered entity was the last one being waited on, the queued entities can now be processed.
            if (m_queuedEntityAddsNotYetActivated.empty() && !m_queuedEntityAdds.empty())
            {
                ProcessQueuedEntityAdds();
            }
        }
    }

    void EditorEntityModel::EntityParentChanged(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        if (GetInfo(entityId).IsConnected())
        {
            ReparentChild(entityId, newParentId, oldParentId);
        }
    }

    void EditorEntityModel::EntitiesAboutToBeCloned()
    {
        m_forceAddToBack = true;
    }

    void EditorEntityModel::EntitiesCloned()
    {
        m_forceAddToBack = false;
    }

    void EditorEntityModel::ChildEntityOrderArrayUpdated()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        //when notified that a parent has reordered its children, they must be updated
        if (m_enableChildReorderHandler)
        {
            auto orderEntityId = *EditorEntitySortNotificationBus::GetCurrentBusId();
            auto parentEntityId = orderEntityId;
            // If we are being called for the invalid entity sort info id, then use it as the parent for updating purposes
            if (orderEntityId == GetEntityIdForSortInfo(AZ::EntityId()))
            {
                parentEntityId = AZ::EntityId();
            }
            auto& entityInfo = GetInfo(parentEntityId);
            for (auto childId : entityInfo.GetChildren())
            {
                auto& childInfo = GetInfo(childId);
                childInfo.UpdateOrderInfo(true);
            }
        }
    }

    void EditorEntityModel::OnEditorEntitiesReplacedBySlicedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& replacedEntitiesMap)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        //the original entity was destroyed by now but the replacement may need to be refreshed
        for (const auto& replacedPair : replacedEntitiesMap)
        {
            AZ::EntityId oldEntityId = replacedPair.first;
            AZ::EntityId newEntityId = replacedPair.second;

            // Verify that the old sort order spot was saved and save it for later use.
            auto savedSortInfoIter = m_savedOrderInfo.find(oldEntityId);
            if (savedSortInfoIter != m_savedOrderInfo.end())
            {
                AZStd::pair<AZ::EntityId, AZ::u64> parentIdAndSortIndexPair = savedSortInfoIter->second;
                AZ::u64 savedSortIndex = parentIdAndSortIndexPair.second;

                // Get the appropriate parent ID
                auto entityInfo = GetInfo(newEntityId);
                AZ::EntityId parentId = GetEntityIdForSortInfo(entityInfo.GetParent());
                EntityOrderArray entityOrderArray;
                EditorEntitySortRequestBus::EventResult(entityOrderArray, parentId, &EditorEntitySortRequestBus::Events::GetChildEntityOrderArray);

                // Find our entity ID and remove it.
                auto sortIter = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), replacedPair.second);
                if (sortIter != entityOrderArray.end())
                {
                    entityOrderArray.erase(sortIter);

                    // Make sure we don't overwrite the bounds of our vector.
                    if (savedSortIndex > entityOrderArray.size())
                    {
                        savedSortIndex = entityOrderArray.size();
                    }

                    //re-insert the new entity into the sort array.
                    entityOrderArray.insert(entityOrderArray.begin() + savedSortIndex, newEntityId);

                    // Push the final array back to the sort component
                    AzToolsFramework::SetEntityChildOrder(parentId, entityOrderArray);
                }
            }

            EBUS_EVENT(ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, Refresh_Values);

            UpdateSliceInfoHierarchy(replacedPair.second);
        }

        // Destroy all saved ordering info.
        m_savedOrderInfo.clear();
    }

    void EditorEntityModel::OnEditorEntitiesSliceOwnershipChanged(const AzToolsFramework::EntityIdList& entityIdList)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        for (const auto& entityId : entityIdList)
        {
            UpdateSliceInfoHierarchy(entityId);
        }
    }

    void EditorEntityModel::OnContextReset()
    {
        Reset();
    }

    void EditorEntityModel::OnEntityStreamLoadBegin()
    {
        Reset();
    }

    void EditorEntityModel::OnEntityStreamLoadSuccess()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        //block internal reorder event handling to avoid recursion since we're manually updating everything
        m_enableChildReorderHandler = false;

        //prepare to completely reset any observers to the current model state
        EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoResetBegin);

        //refresh all order info while blocking related events (keeps UI observers from updating until refresh is complete)
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityModel::OnEntityStreamLoadSuccess:UpdateChildOrderInfo");
            for (auto& entityInfoPair : m_entityInfoTable)
            {
                if (entityInfoPair.second.IsConnected())
                {
                    //add order info if missing
                    entityInfoPair.second.UpdateChildOrderInfo(m_forceAddToBack);
                }
            }
        }
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorEntityModel::OnEntityStreamLoadSuccess:UpdateOrderInfo");
            for (auto& entityInfoPair : m_entityInfoTable)
            {
                if (entityInfoPair.second.IsConnected())
                {
                    entityInfoPair.second.UpdateOrderInfo(false);
                }
            }
        }

        EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoResetEnd);

        m_enableChildReorderHandler = true;
    }

    void EditorEntityModel::OnEntityStreamLoadFailed()
    {
        Reset();
    }

    void EditorEntityModel::OnEntityContextReset()
    {
        Reset();
    }

    void EditorEntityModel::OnEntityContextLoadedFromStream(const AZ::SliceComponent::EntityList&)
    {
        // Register as handler for sort updates to the root slice metadata entity as it handles all unparented entity sorting
        EditorEntitySortNotificationBus::MultiHandler::BusConnect(GetEntityIdForSortInfo(AZ::EntityId()));
    }


    void EditorEntityModel::OnEntityActivated(const AZ::EntityId& activatedEntityId)
    {
        // Stop listening for this entity's activation.
        AZ::EntityBus::MultiHandler::BusDisconnect(activatedEntityId);
        m_queuedEntityAddsNotYetActivated.erase(activatedEntityId);

        // Once all queued entities have activated, add them all to the model.
        if (m_queuedEntityAddsNotYetActivated.empty())
        {
            ProcessQueuedEntityAdds();
        }
    }
    
    void EditorEntityModel::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        // If a tick has elapsed, and entities are still queued, add them all to the model.
        if (!m_queuedEntityAdds.empty())
        {
            ProcessQueuedEntityAdds();
        }

        AZ::TickBus::Handler::BusDisconnect();
    }

    void EditorEntityModel::UpdateSliceInfoHierarchy(AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        auto& entityInfo = GetInfo(entityId);
        entityInfo.UpdateOrderInfo(false);
        entityInfo.UpdateSliceInfo();

        for (auto childId : entityInfo.GetChildren())
        {
            UpdateSliceInfoHierarchy(childId);
        }
    }

    EditorEntityModel::EditorEntityModelEntry::EditorEntityModelEntry()
    {
    }

    EditorEntityModel::EditorEntityModelEntry::~EditorEntityModelEntry()
    {
        Disconnect();
    }

    void EditorEntityModel::EditorEntityModelEntry::Connect()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        Disconnect();

        AZ::TransformBus::EventResult(m_parentId, m_entityId, &AZ::TransformBus::Events::GetParentId);
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(m_selected, &AzToolsFramework::ToolsApplicationRequests::IsSelected, m_entityId);
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(m_visible, m_entityId, &AzToolsFramework::EditorVisibilityRequests::GetVisibilityFlag);
        AzToolsFramework::EditorLockComponentRequestBus::EventResult(m_locked, m_entityId, &AzToolsFramework::EditorLockComponentRequests::GetLocked);

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, m_entityId);
        m_name = entity ? entity->GetName() : "";

        // Slice status is required before we attempt to determine sibling sort index or slice root status
        UpdateSliceInfo();

        UpdateOrderInfo(false);

        AZ::EntityBus::Handler::BusConnect(m_entityId);
        EditorEntityInfoRequestBus::Handler::BusConnect(m_entityId);
        EditorLockComponentNotificationBus::Handler::BusConnect(m_entityId);
        EditorVisibilityNotificationBus::Handler::BusConnect(m_entityId);
        EntitySelectionEvents::Bus::Handler::BusConnect(m_entityId);
        m_connected = true;
    }

    void EditorEntityModel::EditorEntityModelEntry::Disconnect()
    {
        m_connected = false;
        AZ::EntityBus::Handler::BusDisconnect();
        EditorEntityInfoRequestBus::Handler::BusDisconnect();
        EditorLockComponentNotificationBus::Handler::BusDisconnect();
        EditorVisibilityNotificationBus::Handler::BusDisconnect();
        EntitySelectionEvents::Bus::Handler::BusDisconnect();
    }

    void EditorEntityModel::EditorEntityModelEntry::UpdateSliceInfo()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        //reset slice info
        m_isSliceEntity = false;
        m_isSliceRoot = false;
        m_sliceAssetName.clear();

        //determine if entity belongs to a slice
        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
        AzFramework::EntityIdContextQueryBus::EventResult(sliceAddress, m_entityId, &AzFramework::EntityIdContextQueries::GetOwningSlice);

        auto sliceReference = sliceAddress.first;
        auto sliceInstance = sliceAddress.second;
        m_isSliceEntity = sliceReference && sliceInstance;

        if (m_isSliceEntity)
        {
            //determine slice asset name
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(m_sliceAssetName, &AZ::Data::AssetCatalogRequests::GetAssetPathById, sliceReference->GetSliceAsset().GetId());

            //determine if entity parent belongs to a slice
            AZ::SliceComponent::SliceInstanceAddress parentSliceAddress;
            AzFramework::EntityIdContextQueryBus::EventResult(parentSliceAddress, m_parentId, &AzFramework::EntityIdContextQueries::GetOwningSlice);

            //we're a slice root if we don't have a slice reference or instance or our parent slice reference or instances don't match
            auto parentSliceReference = parentSliceAddress.first;
            auto parentSliceInstance = parentSliceAddress.second;
            m_isSliceRoot = !parentSliceReference || !parentSliceInstance || (sliceReference != parentSliceReference) || (sliceInstance->GetId() != parentSliceInstance->GetId());
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::UpdateOrderInfo(bool notify)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        AZ::u64 oldIndex = m_indexForSorting;
        AZ::u64 newIndex = 0;

        //because sort info is stored on ether a parent or a meta data entity, need to determine which before querying
        EditorEntitySortRequestBus::EventResult(newIndex, GetEntityIdForSortInfo(m_parentId), &EditorEntitySortRequestBus::Events::GetChildEntityIndex, m_entityId);

        if (notify && oldIndex != newIndex)
        {
            //notifications still use actual parent
            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedOrderBegin, m_parentId, m_entityId, m_indexForSorting);
        }

        m_indexForSorting = newIndex;

        if (notify && oldIndex != newIndex)
        {
            //notifications still use actual parent
            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedOrderEnd, m_parentId, m_entityId, m_indexForSorting);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::UpdateChildOrderInfo(bool forceAddToBack)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        //add order info if missing
        for (auto childId : m_children)
        {
            AzToolsFramework::AddEntityIdToSortInfo(m_entityId, childId, forceAddToBack);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::SetId(const AZ::EntityId& entityId)
    {
        m_entityId = entityId;
    }

    void EditorEntityModel::EditorEntityModelEntry::SetParent(AZ::EntityId parentId)
    {
        m_parentId = parentId;
    }

    void EditorEntityModel::EditorEntityModelEntry::AddChild(AZ::EntityId childId)
    {
        auto childItr = m_childIndexCache.find(childId);
        if (childItr == m_childIndexCache.end())
        {
            //cache indices for faster lookup
            m_childIndexCache[childId] = static_cast<AZ::u64>(m_children.size());
            m_children.push_back(childId);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::RemoveChild(AZ::EntityId childId)
    {
        auto childItr = m_childIndexCache.find(childId);
        if (childItr != m_childIndexCache.end())
        {
            m_children.erase(m_children.begin() + childItr->second);

            //rebuild index cache for faster lookup
            m_childIndexCache.clear();
            for (auto idx : m_children)
            {
                m_childIndexCache[idx] = static_cast<AZ::u64>(m_childIndexCache.size());
            }
        }
    }

    bool EditorEntityModel::EditorEntityModelEntry::HasChild(AZ::EntityId childId) const
    {
        auto childItr = m_childIndexCache.find(childId);
        return childItr != m_childIndexCache.end();
    }

    AZ::EntityId EditorEntityModel::EditorEntityModelEntry::GetId() const
    {
        return m_entityId;
    }

    AZ::EntityId EditorEntityModel::EditorEntityModelEntry::GetParent() const
    {
        return m_parentId;
    }

    EntityIdList EditorEntityModel::EditorEntityModelEntry::GetChildren() const
    {
        return m_children;
    }

    AZ::EntityId EditorEntityModel::EditorEntityModelEntry::GetChild(AZStd::size_t index) const
    {
        return index < m_children.size() ? m_children[index] : AZ::EntityId();
    }

    AZStd::size_t EditorEntityModel::EditorEntityModelEntry::GetChildCount() const
    {
        return m_children.size();
    }

    AZ::u64 EditorEntityModel::EditorEntityModelEntry::GetChildIndex(AZ::EntityId childId) const
    {
        auto childItr = m_childIndexCache.find(childId);
        return childItr != m_childIndexCache.end() ? childItr->second : static_cast<AZ::u64>(m_children.size());
    }

    AZStd::string EditorEntityModel::EditorEntityModelEntry::GetName() const
    {
        return m_name;
    }

    AZStd::string EditorEntityModel::EditorEntityModelEntry::GetSliceAssetName() const
    {
        return m_sliceAssetName;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsSliceEntity() const
    {
        return m_isSliceEntity;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsSliceRoot() const
    {
        return m_isSliceRoot;
    }

    AZ::u64 EditorEntityModel::EditorEntityModelEntry::GetIndexForSorting() const
    {
        AZ::u64 sortIndex = 0;
        EditorEntitySortRequestBus::EventResult(sortIndex, GetEntityIdForSortInfo(m_parentId), &EditorEntitySortRequestBus::Events::GetChildEntityIndex, m_entityId);
        return sortIndex;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsSelected() const
    {
        return m_selected;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsVisible() const
    {
        return m_visible;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsHidden() const
    {
        return !m_visible;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsLocked() const
    {
        return m_locked;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsConnected() const
    {
        return m_connected;
    }

    void EditorEntityModel::EditorEntityModelEntry::OnEntityLockChanged(bool locked)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        if (m_locked != locked)
        {
            m_locked = locked;
            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedLocked, m_entityId, m_locked);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnEntityVisibilityFlagChanged(bool visibility)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        if (m_visible != visibility)
        {
            m_visible = visibility;
            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedVisibility, m_entityId, m_visible);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnSelected()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        if (!m_selected)
        {
            m_selected = true;
            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedSelection, m_entityId, m_selected);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnDeselected()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        if (m_selected)
        {
            m_selected = false;
            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedSelection, m_entityId, m_selected);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnEntityNameChanged(const AZStd::string& name)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        if (m_name != name)
        {
            m_name = name;
            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedName, m_entityId, m_name);
        }
    }
}