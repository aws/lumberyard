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
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzToolsFramework
{
    class EditorEntityModel
    {
    public:
        EditorEntityModel() = default;
        ~EditorEntityModel();

        EditorEntityModel(const EditorEntityModel&) = delete;

        void AddEntity(AZ::EntityId entityId);
        void RemoveEntity(AZ::EntityId entityId);
        void ChangeEntityParent(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId);

        bool IsTrackingEntity(AZ::EntityId entityId);

    private:
        class EditorEntityModelEntry
            : public EditorEntityInfoRequestBus::Handler
        {
        public:
            EditorEntityModelEntry(const AZ::EntityId& entityId);
            ~EditorEntityModelEntry();

            void Initialize();

            void SetParent(AZ::EntityId parentId);

            /////////////////////////////
            // EditorEntityInfoRequests
            /////////////////////////////
            AZ::EntityId GetParent() override;
            AZStd::vector<AZ::EntityId> GetChildren() override;
            bool IsSliceEntity() override;
            bool IsSliceRoot() override;
            AZ::u64 DepthInHierarchy() override;
            AZ::u64 SiblingSortIndex() override;

        private:
            // Slices
            void DetermineSliceStatus();

            // Parenting
            AZ::EntityId DetermineParentEntityId();

            // Sibling sort index
            void UpdateSiblingSortIndex();
            AZ::u64 DetermineSiblingSortIndex();

            AZ::EntityId m_entityId;
            AZ::EntityId m_parentId;
            AZStd::vector<AZ::EntityId> m_children;
            bool m_isSliceEntity;
            bool m_isSliceRoot;
            AZ::u64 m_depthInHierarchy;
            AZ::u64 m_siblingSortIndex;
        };

        using EntityModelEntries = AZStd::unordered_map<AZ::EntityId, AZStd::unique_ptr<EditorEntityModelEntry>>;
        EntityModelEntries m_entityModelEntries;

    };
}