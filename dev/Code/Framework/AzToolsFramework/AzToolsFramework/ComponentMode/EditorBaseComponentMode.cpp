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

#include "EditorBaseComponentMode.h"

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        AZ_CLASS_ALLOCATOR_IMPL(EditorBaseComponentMode, AZ::SystemAllocator, 0)

        EditorBaseComponentMode::EditorBaseComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType)
            : m_entityComponentIdPair(entityComponentIdPair), m_componentType(componentType)
        {
            const AZ::EntityId entityId = entityComponentIdPair.GetEntityId();
            AZ_Assert(entityId.IsValid(), "Attempting to create a Component Mode with an invalid EntityId");

            if (const AZ::Entity* entity = AzToolsFramework::GetEntity(entityId))
            {
                AZ_Assert(entity->GetState() == AZ::Entity::ES_ACTIVE,
                    "Attempting to create a Component Mode for an Entity which is not currently active. "
                    "Its current state is %u", entity->GetState());
            }

            ComponentModeRequestBus::Handler::BusConnect(m_entityComponentIdPair.GetEntityId());
            ToolsApplicationNotificationBus::Handler::BusConnect();
        }

        EditorBaseComponentMode::~EditorBaseComponentMode()
        {
            ToolsApplicationNotificationBus::Handler::BusDisconnect();
            ComponentModeRequestBus::Handler::BusDisconnect();
        }

        void EditorBaseComponentMode::AfterUndoRedo()
        {
            Refresh();
        }

        AZStd::vector<ActionOverride> EditorBaseComponentMode::PopulateActionsImpl()
        {
            return AZStd::vector<ActionOverride>{};
        }

        AZStd::vector<ActionOverride> EditorBaseComponentMode::PopulateActions()
        {
            return PopulateActionsImpl();
        }
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework