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

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Include/ScriptCanvas/Bus/NodeIdPair.h>

#include <ScriptCanvasDeveloperEditor/NodePaletteFullCreation.h>

namespace ScriptCanvasDeveloperEditor
{    
    namespace NodePaletteFullCreation
    {
        void NodePaletteFullCreationAction()
        {   
            AZ::EntityId activeGraphCanvasGraphId;
            ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(activeGraphCanvasGraphId, &ScriptCanvasEditor::GeneralRequests::GetActiveGraphCanvasGraphId);

            AZ::EntityId activeScriptCanvasGraphId;
            ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(activeScriptCanvasGraphId, &ScriptCanvasEditor::GeneralRequests::GetActiveScriptCanvasGraphId);
            
            if (activeGraphCanvasGraphId.IsValid())
            {
                AZ::Vector2 nodeCreationPos;                

                AZ::EntityId viewId;
                GraphCanvas::SceneRequestBus::EventResult(viewId, activeGraphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

                AZ::EntityId gridId;
                GraphCanvas::SceneRequestBus::EventResult(gridId, activeGraphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

                AZ::Vector2 minorPitch;
                GraphCanvas::GridRequestBus::EventResult(minorPitch, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

                GraphCanvas::ViewRequestBus::EventResult(nodeCreationPos, viewId, &GraphCanvas::ViewRequests::GetViewSceneCenter);

                GraphCanvas::GraphCanvasGraphicsView* graphicsView = nullptr;
                GraphCanvas::ViewRequestBus::EventResult(graphicsView, viewId, &GraphCanvas::ViewRequests::AsGraphicsView);

                QRectF viewportRectangle = graphicsView->mapToScene(graphicsView->viewport()->geometry()).boundingRect();

                int widthOffset = 0;
                int heightOffset = 0;

                int maxRowHeight = 0;

                ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PushPreventUndoStateUpdate);

                const GraphCanvas::GraphCanvasTreeItem* treeItem = nullptr;
                ScriptCanvasEditor::AutomationRequestBus::BroadcastResult(treeItem, &ScriptCanvasEditor::AutomationRequests::GetNodePaletteRoot);

                AZStd::list< const GraphCanvas::GraphCanvasTreeItem* > expandedItems;
                expandedItems.emplace_back(treeItem);

                while (!expandedItems.empty())
                {
                    const GraphCanvas::GraphCanvasTreeItem* treeItem = expandedItems.front();
                    expandedItems.pop_front();

                    for (int i = 0; i < treeItem->GetChildCount(); ++i)
                    {
                        expandedItems.emplace_back(treeItem->FindChildByRow(i));
                    }

                    GraphCanvas::GraphCanvasMimeEvent* mimeEvent = treeItem->CreateMimeEvent();

                    if (mimeEvent)
                    {
                        AZ::Vector2 position(viewportRectangle.x() + widthOffset, viewportRectangle.y() + heightOffset);

                        ScriptCanvasEditor::NodeIdPair createdPair;
                        ScriptCanvasEditor::AutomationRequestBus::BroadcastResult(createdPair, &ScriptCanvasEditor::AutomationRequests::ProcessCreateNodeMimeEvent, mimeEvent, activeGraphCanvasGraphId, position);

                        QGraphicsItem* graphicsWidget = nullptr;
                        GraphCanvas::SceneMemberUIRequestBus::EventResult(graphicsWidget, createdPair.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::GetRootGraphicsItem);

                        if (graphicsWidget)
                        {
                            int height = graphicsWidget->boundingRect().height();

                            if (height > maxRowHeight)
                            {
                                maxRowHeight = height;
                            }

                            widthOffset += graphicsWidget->boundingRect().width() + minorPitch.GetX();

                            if (widthOffset >= viewportRectangle.width())
                            {
                                widthOffset = 0;
                                heightOffset += maxRowHeight + minorPitch.GetY();
                            }
                        }

                        GraphCanvas::SceneNotificationBus::Event(activeGraphCanvasGraphId, &GraphCanvas::SceneNotifications::PostCreationEvent);
                    }
                }

                ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PopPreventUndoStateUpdate);
                ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PostUndoPoint, activeScriptCanvasGraphId);
            }
        }

        QAction* CreateNodePaletteFullCreationAction(QWidget* mainWindow)
        {
            QAction* createNodePaletteAction = nullptr;

            if (mainWindow)
            {
                createNodePaletteAction = new QAction(QAction::tr("Create Node Palette"), mainWindow);
                createNodePaletteAction->setAutoRepeat(false);
                createNodePaletteAction->setToolTip("Tries to create every node in the node palette. All of them. At once.");
                createNodePaletteAction->setShortcut(QKeySequence(QAction::tr("Ctrl+Shift+p", "Debug|Create Node Palette")));

                mainWindow->addAction(createNodePaletteAction);
                mainWindow->connect(createNodePaletteAction, &QAction::triggered, &NodePaletteFullCreationAction);
            }

            return createNodePaletteAction;
        }
    }
}