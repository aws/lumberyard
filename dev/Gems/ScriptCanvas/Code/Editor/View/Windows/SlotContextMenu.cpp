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
#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include "SlotContextMenu.h"

namespace ScriptCanvasEditor
{
    SlotContextMenu::SlotContextMenu(const AZ::EntityId& selectedSlotId)
        : QMenu()
    {
        // Delete all connections.
        {
            QAction* action = new QAction(QObject::tr("Delete all connections"), this);
            QObject::connect(action,
                &QAction::triggered,
                [&selectedSlotId](bool checked)
                {
                    AZ::EntityId sceneId;
                    GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, selectedSlotId, &GraphCanvas::SceneMemberRequests::GetScene);

                    AZ::EntityId nodeId;
                    GraphCanvas::SlotRequestBus::EventResult(nodeId, selectedSlotId, &GraphCanvas::SlotRequests::GetNode);
                    auto selectedEndpoint = GraphCanvas::Endpoint{ nodeId, selectedSlotId };
                    ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::DisconnectEndpoints, sceneId, AZStd::vector<GraphCanvas::Endpoint>{selectedEndpoint});
                });

            addAction(action);
        }
    }
}
