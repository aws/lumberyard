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

#include <QVBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Editor/Nodes/NodeUtils.h>
#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/GraphCanvasBus.h>

#include "CreateNodeContextMenu.h"

namespace ScriptCanvasEditor
{
    /////////////////////
    // CreateNodeAction
    /////////////////////
    CreateNodeAction::CreateNodeAction(const QString& text, QObject* parent)
        : QAction(text, parent)
    {
    }

    //////////////////////////////
    // AddSelectedEntitiesAction
    //////////////////////////////

    AddSelectedEntitiesAction::AddSelectedEntitiesAction(QObject* parent)
        : CreateNodeAction("", parent)
    {
    }

    void AddSelectedEntitiesAction::RefreshAction(const AZ::EntityId&)
    {
        AzToolsFramework::EntityIdList selectedEntities;

        EBUS_EVENT_RESULT(selectedEntities,
            AzToolsFramework::ToolsApplicationRequests::Bus,
            GetSelectedEntities);

        setEnabled(!selectedEntities.empty());

        if (selectedEntities.size() <= 1)
        {
            setText("Reference Selected Entity");
        }
        else
        {
            setText("Reference Selected Entities");
        }
    }

    bool AddSelectedEntitiesAction::TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos)
    {
        AzToolsFramework::EntityIdList selectedEntities;

        EBUS_EVENT_RESULT(selectedEntities,
            AzToolsFramework::ToolsApplicationRequests::Bus,
            GetSelectedEntities);

        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);

        AZ::Vector2 addPosition = scenePos;

        for (const AZ::EntityId& id : selectedEntities)
        {
            NodeIdPair nodePair = Nodes::CreateEntityNode(id, scriptCanvasSceneId);
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, addPosition);
            addPosition += AZ::Vector2(20, 20);
        }

        return true;
    }

    ////////////////////////////
    // CutGraphSelectionAction
    ////////////////////////////

    CutGraphSelectionAction::CutGraphSelectionAction(QObject* parent)
        : CreateNodeAction("Cut", parent)
    {
    }

    void CutGraphSelectionAction::RefreshAction(const AZ::EntityId& scriptCanvasSceneId)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        bool hasSelectedItems = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelectedItems, graphCanvasGraphId, &GraphCanvas::SceneRequests::HasSelectedItems);

        setEnabled(hasSelectedItems);
    }

    bool CutGraphSelectionAction::TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::CutSelection);
        return true;
    }

    /////////////////////////////
    // CopyGraphSelectionAction
    /////////////////////////////

    CopyGraphSelectionAction::CopyGraphSelectionAction(QObject* parent)
        : CreateNodeAction("Copy", parent)
    {
    }

    void CopyGraphSelectionAction::RefreshAction(const AZ::EntityId& scriptCanvasSceneId)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        bool hasSelectedItems = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelectedItems, graphCanvasGraphId, &GraphCanvas::SceneRequests::HasSelectedItems);

        setEnabled(hasSelectedItems);
    }

    bool CopyGraphSelectionAction::TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::CopySelection);
        return false;
    }

    //////////////////////////////
    // PasteGraphSelectionAction
    //////////////////////////////

    PasteGraphSelectionAction::PasteGraphSelectionAction(QObject* parent)
        : CreateNodeAction("Paste", parent)
    {
    }

    void PasteGraphSelectionAction::RefreshAction(const AZ::EntityId& scriptCanvasSceneId)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        AZStd::string mimeType;
        GraphCanvas::SceneRequestBus::EventResult(mimeType, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetCopyMimeType);

        bool isPasteable = QApplication::clipboard()->mimeData()->hasFormat(mimeType.c_str());
        setEnabled(isPasteable);
    }

    bool PasteGraphSelectionAction::TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::PasteAt, QPoint(scenePos.GetX(), scenePos.GetY()));
        return true;
    }

    //////////////////////////////////
    // DuplicateGraphSelectionAction
    //////////////////////////////////

    DuplicateGraphSelectionAction::DuplicateGraphSelectionAction(QObject* parent)
        : CreateNodeAction("Duplicate", parent)
    {
    }

    void DuplicateGraphSelectionAction::RefreshAction(const AZ::EntityId& scriptCanvasSceneId)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        bool hasSelectedItems = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelectedItems, graphCanvasGraphId, &GraphCanvas::SceneRequests::HasSelectedItems);

        setEnabled(hasSelectedItems);
    }

    bool DuplicateGraphSelectionAction::TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::DuplicateSelectionAt, QPoint(scenePos.GetX(), scenePos.GetY()));
        return true;
    }

    ///////////////////////////////
    // DeleteGraphSelectionAction
    ///////////////////////////////

    DeleteGraphSelectionAction::DeleteGraphSelectionAction(QObject* parent)
        : CreateNodeAction("Delete", parent)
    {
    }

    void DeleteGraphSelectionAction::RefreshAction(const AZ::EntityId& scriptCanvasSceneId)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        bool hasSelectedItems = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelectedItems, graphCanvasGraphId, &GraphCanvas::SceneRequests::HasSelectedItems);

        setEnabled(hasSelectedItems);
    }

    bool DeleteGraphSelectionAction::TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::DeleteSelection);
        return true;
    }

    /////////////////////
    // AddCommentAction
    /////////////////////

    AddCommentAction::AddCommentAction(QObject* parent)
        : CreateNodeAction("Add Comment", parent)
    {
    }

    void AddCommentAction::RefreshAction(const AZ::EntityId&)
    {
    }

    bool AddCommentAction::TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateCommentNodeAndActivate);
        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Bus Node");

        if (graphCanvasEntity)
        {
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePos);
        }

        return graphCanvasEntity != nullptr;
    }

    ////////////////////////////
    // EndpointSelectionAction
    ////////////////////////////

    EndpointSelectionAction::EndpointSelectionAction(const GraphCanvas::Endpoint& proposedEndpoint)
        : QAction(nullptr)
        , m_endpoint(proposedEndpoint)
    {
        AZStd::string name;
        GraphCanvas::SlotRequestBus::EventResult(name, proposedEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetName);

        AZStd::string tooltip;
        GraphCanvas::SlotRequestBus::EventResult(tooltip, proposedEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetTooltip);

        setText(name.c_str());
        setToolTip(tooltip.c_str());
    }

    const GraphCanvas::Endpoint& EndpointSelectionAction::GetEndpoint() const
    {
        return m_endpoint;
    }

    /////////////////////////////
    // CreateBlockCommentAction
    /////////////////////////////

    CreateBlockCommentAction::CreateBlockCommentAction(QObject* parent)
        : CreateNodeAction("Create Block Comment", parent)
    {
    }

    void CreateBlockCommentAction::RefreshAction(const AZ::EntityId& scriptCanvasSceneId)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        bool hasSelection = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelection, graphCanvasGraphId, &GraphCanvas::SceneRequests::HasSelectedItems);

        if (hasSelection)
        {
            setText("Create Block Comment For Selection");
            setToolTip("Will create a block comment around the selected nodes.");
        }
        else
        {
            setText("Create Block Comment");
            setToolTip("Will create a block comment at the specified point.");
        }
    }

    bool CreateBlockCommentAction::TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        bool hasSelection = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelection, graphCanvasGraphId, &GraphCanvas::SceneRequests::HasSelectedItems);
        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateBlockCommentNodeAndActivate);

        if (graphCanvasEntity)
        {
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePos);

            if (hasSelection)
            {
                AZStd::vector< AZ::EntityId > selectedNodes;
                GraphCanvas::SceneRequestBus::EventResult(selectedNodes, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetSelectedNodes);
                
                QGraphicsItem* rootItem = nullptr;
                QRectF boundingArea;

                for (const AZ::EntityId& selectedNode : selectedNodes)
                {
                    GraphCanvas::SceneMemberUIRequestBus::EventResult(rootItem, selectedNode, &GraphCanvas::SceneMemberUIRequests::GetRootGraphicsItem);

                    if (rootItem)
                    {
                        if (boundingArea.isEmpty())
                        {
                            boundingArea = rootItem->sceneBoundingRect();
                        }
                        else
                        {
                            boundingArea = boundingArea.united(rootItem->sceneBoundingRect());
                        }
                    }
                }

                AZ::Vector2 gridStep;
                AZ::EntityId grid;
                GraphCanvas::SceneRequestBus::EventResult(grid, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

                GraphCanvas::GridRequestBus::EventResult(gridStep, grid, &GraphCanvas::GridRequests::GetMinorPitch);

                boundingArea.adjust(-gridStep.GetX(), -gridStep.GetY(), gridStep.GetX(), gridStep.GetY());
                
                GraphCanvas::BlockCommentRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::BlockCommentRequests::SetBlock, boundingArea);
            }

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);
            GraphCanvas::SceneMemberUIRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        }

        return graphCanvasEntity != nullptr;
    }

    //////////////////////
    // AddBookmarkAction
    //////////////////////

    AddBookmarkAction::AddBookmarkAction(QObject* parent)
        : CreateNodeAction("Add Bookmark", parent)
    {
    }

    void AddBookmarkAction::RefreshAction(const AZ::EntityId&)
    {
    }

    bool AddBookmarkAction::TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasSceneId);

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateBookmarkAnchorAndActivate);
        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Bus Node");

        if (graphCanvasEntity)
        {
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddBookmarkAnchor, graphCanvasEntity->GetId(), scenePos);
        }

        return graphCanvasEntity != nullptr;
    }
    
    //////////////////////////
    // CreateNodeContextMenu
    //////////////////////////

    CreateNodeContextMenu::CreateNodeContextMenu()
    {
        QWidgetAction* actionWidget = new QWidgetAction(this);

        const bool inContextMenu = true;
        m_palette = aznew Widget::NodePaletteDockWidget(tr("Node Palette"), this, inContextMenu);

        actionWidget->setDefaultWidget(m_palette);

        // Ordering
        addAction(new CutGraphSelectionAction(this));
        addAction(new CopyGraphSelectionAction(this));
        addAction(new PasteGraphSelectionAction(this));
        addAction(new DuplicateGraphSelectionAction(this));
        addAction(new DeleteGraphSelectionAction(this));
        addSeparator();
        addAction(new AddBookmarkAction(this));
        addAction(new AddCommentAction(this));
        addAction(new CreateBlockCommentAction(this));
        addSeparator();
        addAction(new AddSelectedEntitiesAction(this));
        addSeparator();
        addAction(actionWidget);

        connect(this, &QMenu::aboutToShow, this, &CreateNodeContextMenu::SetupDisplay);
        connect(m_palette, &Widget::NodePaletteDockWidget::OnContextMenuSelection, this, &CreateNodeContextMenu::HandleContextMenuSelection);
    }

    void CreateNodeContextMenu::DisableCreateActions()
    {
        for (QAction* action : actions())
        {
            CreateNodeAction* createNodeAction = qobject_cast<CreateNodeAction*>(action);

            if (createNodeAction)
            {
                createNodeAction->setEnabled(false);
            }
        }
    }

    void CreateNodeContextMenu::RefreshActions(const AZ::EntityId& scriptCanvasGraphId)
    {
        for (QAction* action : actions())
        {
            CreateNodeAction* createNodeAction = qobject_cast<CreateNodeAction*>(action);

            if (createNodeAction)
            {
                // Not sure if the action has been disabled or not, so enable everything.
                // Then let the refresh handle disabling it.
                createNodeAction->setEnabled(true);
                createNodeAction->RefreshAction(scriptCanvasGraphId);
            }
        }
    }

    void CreateNodeContextMenu::ResetSourceSlotFilter()
    {
        m_palette->ResetSourceSlotFilter();
    }

    void CreateNodeContextMenu::FilterForSourceSlot(const AZ::EntityId& scriptCanvasGraphId, const AZ::EntityId& sourceSlotId)
    {
        m_palette->FilterForSourceSlot(scriptCanvasGraphId, sourceSlotId);
    }

    const Widget::NodePaletteDockWidget* CreateNodeContextMenu::GetNodePalette() const
    {
        return m_palette;
    }

    void CreateNodeContextMenu::HandleContextMenuSelection()
    {
        close();
    }

    void CreateNodeContextMenu::SetupDisplay()
    {
        m_palette->ResetDisplay();
        m_palette->FocusOnSearchFilter();
    }

    void CreateNodeContextMenu::keyPressEvent(QKeyEvent* keyEvent)
    {
        if (!m_palette->hasFocus())
        {
            QMenu::keyPressEvent(keyEvent);
        }
    }

    #include "Editor/View/Windows/CreateNodeContextMenu.moc"
}

