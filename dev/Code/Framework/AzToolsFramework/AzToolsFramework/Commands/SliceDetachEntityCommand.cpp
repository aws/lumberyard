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
#include "SliceDetachEntityCommand.h"
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>

namespace AzToolsFramework
{
    SliceDetachEntityCommand::SliceDetachEntityCommand(const AzToolsFramework::EntityIdList& entityIds, const AZStd::string& friendlyName)
        : UndoSystem::URSequencePoint(friendlyName)
        , m_entityIds(entityIds)
    {}

    void SliceDetachEntityCommand::Undo()
    {
        // Get the root slice
        AZ::SliceComponent* editorRootSlice = nullptr;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(editorRootSlice, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorRootSlice);

        for (const auto& pair : m_entityRestoreInfoArray)
        {
            const AZ::EntityId& entityId = pair.first;
            const AZ::SliceComponent::EntityRestoreInfo& restoreInfo = pair.second;

            AZ::Entity* entity = editorRootSlice->FindEntity(entityId);
            if (entity)
            {
                EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequestBus::Events::RestoreSliceEntity, entity, restoreInfo, SliceEntityRestoreType::Detached);
            }
            else
            {
                AZ_Error("SliceDetachEntityCommand", entity, "Unable to find previous detached entity of Id %s. Cannot undo \"Detach\" action.", entityId.ToString().c_str());
            }
        }

        m_entityRestoreInfoArray.clear();
    }

    void SliceDetachEntityCommand::Redo()
    {
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            m_changed, &AzToolsFramework::ToolsApplicationRequestBus::Events::DetachEntities, m_entityIds, m_entityRestoreInfoArray);
    }
}
