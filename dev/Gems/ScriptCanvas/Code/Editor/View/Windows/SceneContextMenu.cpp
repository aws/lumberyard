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

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include <AzCore/Component/Entity.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>

#include "SceneContextMenu.h"

namespace ScriptCanvasEditor
{
    namespace
    {
        QAction* GetCopyAction(const QPointF scenePos, const AZ::EntityId& sceneId, QObject* parent)
        {
            auto* action = new QAction({ QObject::tr("Copy Selected Items") }, parent);
            action->setToolTip(QObject::tr("Copy the selected items to the clipboard"));
            action->setStatusTip(QObject::tr("Copy the selected items to the clipboard"));

            bool hasSelection;
            GraphCanvas::SceneRequestBus::EventResult(hasSelection, sceneId, &GraphCanvas::SceneRequests::HasSelectedItems);
            action->setEnabled(hasSelection);

            QObject::connect(action,
                &QAction::triggered,
                [sceneId](bool)
            {
                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::CopySelection);
            });

            return action;
        }

        QAction* GetCutAction(const QPointF scenePos, const AZ::EntityId& sceneId, QObject* parent)
        {
            auto* action = new QAction({ QObject::tr("Cut Selected Items") }, parent);
            action->setToolTip(QObject::tr("Cut this selection to the clipboard"));
            action->setStatusTip(QObject::tr("Cut this selection to the clipboard"));

            bool hasSelection;
            GraphCanvas::SceneRequestBus::EventResult(hasSelection, sceneId, &GraphCanvas::SceneRequests::HasSelectedItems);
            action->setEnabled(hasSelection);

            QObject::connect(action,
                &QAction::triggered,
                [sceneId](bool)
            {
                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::CutSelection);
            });

            return action;
        }

        QAction* GetPasteAction(const QPointF scenePos, const AZ::EntityId& sceneId, QObject* parent)
        {
            auto* action = new QAction({ QObject::tr("Paste") }, parent);
            action->setToolTip(QObject::tr("Paste this node from the clipboard"));
            action->setStatusTip(QObject::tr("Paste this node from the clipboard"));
            QClipboard* clipboard = QApplication::clipboard();
            const QMimeData* mimeData = clipboard->mimeData();
            if (mimeData)
            {
                action->setEnabled(mimeData->hasFormat("application/x-lumberyard-graphcanvas"));
            }

            QObject::connect(action,
                &QAction::triggered,
                [scenePos, sceneId](bool)
            {
                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::PasteAt, scenePos);
            });

            return action;
        }


        QAction* GetSelectedItemsDeleteAction(const AZ::EntityId& sceneId, QObject* parent)
        {
            auto* action = new QAction({ QObject::tr("Delete Selected Items") }, parent);
            action->setToolTip(QObject::tr("Deletes this node and any other selected nodes"));
            action->setStatusTip(QObject::tr("Deletes this node and any other selected nodes"));

            AZStd::vector<AZ::EntityId> selectedNodes;
            GraphCanvas::SceneRequestBus::EventResult(selectedNodes, sceneId, &GraphCanvas::SceneRequests::GetSelectedNodes);
            AZStd::vector<AZ::EntityId> selectedConnections;
            GraphCanvas::SceneRequestBus::EventResult(selectedConnections, sceneId, &GraphCanvas::SceneRequests::GetSelectedConnections);

            action->setEnabled(!selectedNodes.empty() || !selectedConnections.empty());

            QObject::connect(action,
                &QAction::triggered,
                [sceneId, selectedNodes, selectedConnections](bool)
            {
                ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::DeleteNodes, sceneId, selectedNodes);
                ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::DeleteConnections, sceneId, selectedConnections);
            });

            return action;
        }
    } // inner anonymous namespace
} // anonymous namespace.

namespace ScriptCanvasEditor
{
    SceneContextMenu::SceneContextMenu(const QPointF& scenePos, const AZ::EntityId& sceneId)
        : QMenu()
    {
        addAction(GetCutAction(scenePos, sceneId, this));
        addAction(GetCopyAction(scenePos, sceneId, this));
        addAction(GetPasteAction(scenePos, sceneId, this));        
        addAction(GetSelectedItemsDeleteAction(sceneId, this));
        addSeparator();
    }
}
