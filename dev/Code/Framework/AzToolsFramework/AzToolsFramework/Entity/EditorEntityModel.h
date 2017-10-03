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
#pragma once

#include "EditorEntityInfoBus.h"
#include "EditorEntitySortBus.h"

#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TickBus.h>

#include <AzFramework/Entity/EntityContextBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntitySortBus.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>

namespace AzToolsFramework
{
    class EditorEntityModel
        : public AzFramework::EntityContextEventBus::Handler
        , public AzToolsFramework::EditorMetricsEventsBus::Handler
        , public EditorEntityContextNotificationBus::Handler
        , public EditorEntitySortNotificationBus::MultiHandler
        , public ToolsApplicationEvents::Bus::Handler
        , public AZ::EntityBus::MultiHandler
        , public AZ::TickBus::Handler
    {
    public:
        EditorEntityModel();
        ~EditorEntityModel();
        EditorEntityModel(const EditorEntityModel&) = delete;

        ////////////////////////////////////////////////////////////////////////
        // ToolsApplicationEvents::Bus
        ////////////////////////////////////////////////////////////////////////
        void EntityRegistered(AZ::EntityId entityId) override;
        void EntityDeregistered(AZ::EntityId entityId) override;
        void EntityParentChanged(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId) override;

        ////////////////////////////////////////////////////////////////////////
        // EditorMetricsEventsBus
        ////////////////////////////////////////////////////////////////////////
        void EntitiesAboutToBeCloned() override;
        void EntitiesCloned() override;

        ///////////////////////////////////
        // EditorEntitySortNotificationBus
        ///////////////////////////////////
        void ChildEntityOrderArrayUpdated() override;

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEntityContextNotificationBus::Handler
        //////////////////////////////////////////////////////////////////////////
        void OnEditorEntitiesReplacedBySlicedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& replacedEntitiesMap) override;
        void OnEditorEntitiesSliceOwnershipChanged(const AzToolsFramework::EntityIdList& entityIdList) override;
        void OnContextReset() override;
        void OnEntityStreamLoadBegin() override;
        void OnEntityStreamLoadSuccess() override;
        void OnEntityStreamLoadFailed() override;

        ////////////////////////////////////////////////
        // AzFramework::EntityContextEventBus::Handler
        ////////////////////////////////////////////////
        void OnEntityContextReset() override;
        void OnEntityContextLoadedFromStream(const AZ::SliceComponent::EntityList&) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::EntityBus
        ////////////////////////////////////////////////////////////////////////
        void OnEntityActivated(const AZ::EntityId& entityId) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        ////////////////////////////////////////////////////////////////////////
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        void Reset();
        void ProcessQueuedEntityAdds();
        void ClearQueuedEntityAdds();
        void AddEntity(AZ::EntityId entityId);
        void RemoveEntity(AZ::EntityId entityId);

        void AddChildToParent(AZ::EntityId parentId, AZ::EntityId childId);
        void RemoveChildFromParent(AZ::EntityId parentId, AZ::EntityId childId);
        void ReparentChild(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId);

        void UpdateSliceInfoHierarchy(AZ::EntityId entityId);

        class EditorEntityModelEntry
            : private AZ::EntityBus::Handler
            , private EditorLockComponentNotificationBus::Handler
            , private EditorVisibilityNotificationBus::Handler
            , private EntitySelectionEvents::Bus::Handler
            , public EditorEntityInfoRequestBus::Handler
        {
        public:
            EditorEntityModelEntry();
            ~EditorEntityModelEntry();

            void Connect();
            void Disconnect();

            // Slices
            void UpdateSliceInfo();

            // Sibling sort index
            void UpdateOrderInfo(bool notify);

            void UpdateChildOrderInfo(bool forceAddToBack);

            void SetId(const AZ::EntityId& entityId);
            void SetParent(AZ::EntityId parentId);
            void AddChild(AZ::EntityId childId);
            void RemoveChild(AZ::EntityId childId);
            bool HasChild(AZ::EntityId childId) const;

            /////////////////////////////
            // EditorEntityInfoRequests
            /////////////////////////////
            AZ::EntityId GetId() const;
            AZ::EntityId GetParent() const override;
            EntityIdList GetChildren() const override;
            AZ::EntityId GetChild(AZStd::size_t index) const override;
            AZStd::size_t GetChildCount() const override;
            AZ::u64 GetChildIndex(AZ::EntityId childId) const override;
            AZStd::string GetName() const override;
            AZStd::string GetSliceAssetName() const override;
            bool IsSliceEntity() const override;
            bool IsSliceRoot() const override;
            AZ::u64 GetIndexForSorting() const override;
            bool IsSelected() const override;
            bool IsVisible() const override;
            bool IsHidden() const override;
            bool IsLocked() const override;
            bool IsConnected() const;

            ////////////////////////////////////////////////////////////////////////
            // EditorLockComponentNotificationBus::Handler
            ////////////////////////////////////////////////////////////////////////
            void OnEntityLockChanged(bool locked) override;

            ////////////////////////////////////////////////////////////////////////
            // EditorVisibilityNotificationBus::Handler
            ////////////////////////////////////////////////////////////////////////
            void OnEntityVisibilityFlagChanged(bool visibility) override;

            //////////////////////////////////////////////////////////////////////////
            // AzToolsFramework::EntitySelectionEvents::Handler
            // Used to handle selection changes on a per-entity basis.
            //////////////////////////////////////////////////////////////////////////
            void OnSelected() override;
            void OnDeselected() override;

            ////////////////////////////////////////////////////////////////////////
            // AZ::EntityBus
            ////////////////////////////////////////////////////////////////////////
            void OnEntityNameChanged(const AZStd::string& name) override;

        private:
            EntityOrderArray GetChildOrderArray() const;
            void SetChildOrderArray(const EntityOrderArray& order);
            void AddChildOrder(AZ::EntityId childId);
            void RemoveChildOrder(AZ::EntityId childId);

            AZ::EntityId m_entityId;
            AZ::EntityId m_parentId;
            EntityIdList m_children;
            bool m_isSliceEntity = false;
            bool m_isSliceRoot = false;
            AZ::u64 m_indexForSorting = 0;
            bool m_selected = false;
            bool m_visible = true;
            bool m_locked = false;
            bool m_connected = false;
            AZStd::string m_name;
            AZStd::string m_sliceAssetName;
            AZStd::unordered_map<AZ::EntityId, AZ::u64> m_childIndexCache;
        };

        EditorEntityModelEntry& GetInfo(const AZ::EntityId& entityId);
        AZStd::unordered_map<AZ::EntityId, EditorEntityModelEntry> m_entityInfoTable;
        AZStd::unordered_map<AZ::EntityId, AZStd::unordered_set<AZ::EntityId>> m_entityOrphanTable;
        AZStd::unordered_map<AZ::EntityId, AZ::u64> m_savedOrderInfo; ///< Sort order of recently deleted entities. Used to restore order if entity is later replaced by sliced entity.
        bool m_enableChildReorderHandler = true;
        bool m_forceAddToBack = false;

        // Don't add new entities to the model until they're all ready to go.
        // This lets us add the entities in a more efficient manner.
        AZStd::unordered_set<AZ::EntityId> m_queuedEntityAdds;
        AZStd::unordered_set<AZ::EntityId> m_queuedEntityAddsNotYetActivated;
    };
}