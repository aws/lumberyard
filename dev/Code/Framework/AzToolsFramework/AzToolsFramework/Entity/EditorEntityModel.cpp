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

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzFramework/Entity/EntityContextBus.h>

namespace AzToolsFramework
{
    EditorEntityModel::~EditorEntityModel()
    {
        m_entityModelEntries.clear();
    }

    void EditorEntityModel::AddEntity(AZ::EntityId entityId)
    {
        auto existingModelEntry = m_entityModelEntries.find(entityId);
        AZ_Assert(existingModelEntry == m_entityModelEntries.end(), "AddEntity called for an entity we already are tracking");
        if (existingModelEntry != m_entityModelEntries.end())
        {
            return;
        }

        auto newModelEntry = AZStd::make_unique<EditorEntityModelEntry>(entityId);
        newModelEntry->Initialize();
        m_entityModelEntries[entityId] = AZStd::move(newModelEntry);
    }

    void EditorEntityModel::RemoveEntity(AZ::EntityId entityId)
    {
        auto entityModelEntry = m_entityModelEntries.find(entityId);
        AZ_Assert(entityModelEntry != m_entityModelEntries.end(), "RemoveEntity called for an entity we were not tracking");

        if (entityModelEntry != m_entityModelEntries.end())
        {
            m_entityModelEntries.erase(entityModelEntry);
        }
    }

    void EditorEntityModel::ChangeEntityParent(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId)
    {
        (void)oldParentId;

        auto entityModelEntryIterator = m_entityModelEntries.find(entityId);
        AZ_Assert(entityModelEntryIterator != m_entityModelEntries.end(), "ChangeEntityParent called for an entity we were not tracking");
        if (entityModelEntryIterator == m_entityModelEntries.end())
        {
            return;
        }

        auto& entityModelEntry = entityModelEntryIterator->second;

        AZ_Assert(entityModelEntry->GetParent() == oldParentId, "EntityParentChanged gave us an old parent id that was not what we had previously");

        entityModelEntry->SetParent(newParentId);
    }

    bool EditorEntityModel::IsTrackingEntity(AZ::EntityId entityId)
    {
        return m_entityModelEntries.find(entityId) != m_entityModelEntries.end();
    }

    EditorEntityModel::EditorEntityModelEntry::EditorEntityModelEntry(const AZ::EntityId& entityId)
        : m_entityId(entityId)
    {
    }

    EditorEntityModel::EditorEntityModelEntry::~EditorEntityModelEntry()
    {
        EditorEntityInfoRequestBus::Handler::BusDisconnect();
    }

    void EditorEntityModel::EditorEntityModelEntry::Initialize()
    {
        m_parentId = DetermineParentEntityId();

        // Slice status is required before we attempt to determine sibling sort index or slice root status
        DetermineSliceStatus();

        m_siblingSortIndex = DetermineSiblingSortIndex();

        EditorEntityInfoRequestBus::Handler::BusConnect(m_entityId);
    }

    void EditorEntityModel::EditorEntityModelEntry::SetParent(AZ::EntityId parentId)
    {
        m_parentId = parentId;
    }

    AZ::EntityId EditorEntityModel::EditorEntityModelEntry::GetParent()
    {
        return m_parentId;
    }

    AZStd::vector<AZ::EntityId> EditorEntityModel::EditorEntityModelEntry::GetChildren()
    {
        return m_children;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsSliceEntity()
    {
        return m_isSliceEntity;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsSliceRoot()
    {
        return m_isSliceRoot;
    }

    AZ::u64 EditorEntityModel::EditorEntityModelEntry::DepthInHierarchy()
    {
        return m_depthInHierarchy;
    }

    AZ::u64 EditorEntityModel::EditorEntityModelEntry::SiblingSortIndex()
    {
        return m_siblingSortIndex;
    }

    void EditorEntityModel::EditorEntityModelEntry::DetermineSliceStatus()
    {
        // Assume all slice info is false
        m_isSliceEntity = false;
        m_isSliceRoot = false;

        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
        AzFramework::EntityIdContextQueryBus::EventResult(sliceAddress, m_entityId, &AzFramework::EntityIdContextQueries::GetOwningSlice);
        auto sliceReference = sliceAddress.first;
        auto sliceInstance = sliceAddress.second;
        if (!sliceReference || !sliceInstance)
        {
            return;
        }

        m_isSliceEntity = sliceReference && sliceInstance;

        AZ::SliceComponent::SliceInstanceAddress parentSliceAddress;
        AzFramework::EntityIdContextQueryBus::EventResult(parentSliceAddress, m_parentId, &AzFramework::EntityIdContextQueries::GetOwningSlice);
        auto parentSliceReference = sliceAddress.first;
        auto parentSliceInstance = sliceAddress.second;
        // If we don't have a slice reference or instance, we're a slice root
        // If our parent slice reference or instances don't match, we're also a root
        if (!parentSliceReference 
            || !parentSliceInstance
            || (sliceReference != parentSliceReference) 
            || (sliceInstance->GetId() != parentSliceInstance->GetId()))
        {
            m_isSliceRoot = true;
        }
    }

    AZ::EntityId EditorEntityModel::EditorEntityModelEntry::DetermineParentEntityId()
    {
        AZ::EntityId parentId;
        AZ::TransformBus::EventResult(parentId, m_entityId, &AZ::TransformBus::Events::GetParentId);
        return parentId;
    }

    void EditorEntityModel::EditorEntityModelEntry::UpdateSiblingSortIndex()
    {
        m_siblingSortIndex = DetermineSiblingSortIndex();
    }

    AZ::u64 EditorEntityModel::EditorEntityModelEntry::DetermineSiblingSortIndex()
    {
        // Get our parent's sort order
        if (!m_parentId.IsValid())
        {
            if (IsSliceEntity())
            {
                // Todo: Query slice metadata for index
                return 0;
            }
            // Todo: Query level slice metadata for index
            return 0;
        }

        EntityOrderArray siblingOrderArray;
        EditorEntitySortRequestBus::EventResult(siblingOrderArray, m_parentId, &EditorEntitySortRequestBus::Events::GetChildEntityOrderArray);
        
        // Find us in the array
        for (size_t orderIndex = 0; orderIndex < siblingOrderArray.size(); ++orderIndex)
        {
            if (siblingOrderArray[orderIndex] == m_entityId)
            {
                return orderIndex;
            }
        }

        // Couldn't find us in the array, let's patch ourselves to the bottom
        siblingOrderArray.push_back(m_entityId);
        EditorEntitySortRequestBus::Event(m_parentId, &EditorEntitySortRequestBus::Events::SetChildEntityOrderArray, siblingOrderArray);

        return siblingOrderArray.size() - 1;
    }
}