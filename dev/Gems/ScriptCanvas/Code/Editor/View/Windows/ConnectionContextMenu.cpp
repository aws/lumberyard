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

#include "precompiled.h"

#include <ScriptCanvas/Bus/RequestBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

#include "ConnectionContextMenu.h"

namespace ScriptCanvasEditor
{
    ConnectionContextMenu::ConnectionContextMenu(const AZ::EntityId& connectionId)
        : QMenu()
    {
        AZ::EntityId sceneId;
        GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, connectionId, &GraphCanvas::SceneMemberRequests::GetScene);

        AZStd::vector<AZ::EntityId> selectedConnectionIds;
        GraphCanvas::SceneRequestBus::EventResult(selectedConnectionIds, sceneId, &GraphCanvas::SceneRequests::GetSelectedConnections);

        // Delete.
        {
            QAction* action = new QAction(QObject::tr("Delete"), this);
            QObject::connect(action,
                &QAction::triggered,
                [sceneId, selectedConnectionIds](bool checked)
                {
                    ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::DeleteConnections, sceneId, selectedConnectionIds);
                });

            addAction(action);
        }
    }
}
