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
#include <Editor/View/Widgets/NodePalette.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>

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

    bool AddSelectedEntitiesAction::TriggerAction(const AZ::EntityId& sceneId, const AZ::Vector2& scenePos)
    {
        AzToolsFramework::EntityIdList selectedEntities;

        EBUS_EVENT_RESULT(selectedEntities,
            AzToolsFramework::ToolsApplicationRequests::Bus,
            GetSelectedEntities);

        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::ClearSelection);

        AZ::EntityId graphId;
        GeneralRequestBus::BroadcastResult(graphId, &GeneralRequests::GetGraphId, sceneId);

        AZ::Vector2 addPosition = scenePos;

        for (const AZ::EntityId& id : selectedEntities)
        {
            NodeIdPair nodePair = Nodes::CreateEntityNode(id, graphId);
            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, addPosition);
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

    void CutGraphSelectionAction::RefreshAction(const AZ::EntityId& sceneId)
    {
        bool hasSelectedItems = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelectedItems, sceneId, &GraphCanvas::SceneRequests::HasSelectedItems);

        setEnabled(hasSelectedItems);
    }

    bool CutGraphSelectionAction::TriggerAction(const AZ::EntityId& sceneId, const AZ::Vector2& scenePos)
    {
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::CutSelection);
        return true;
    }

    /////////////////////////////
    // CopyGraphSelectionAction
    /////////////////////////////

    CopyGraphSelectionAction::CopyGraphSelectionAction(QObject* parent)
        : CreateNodeAction("Copy", parent)
    {
    }

    void CopyGraphSelectionAction::RefreshAction(const AZ::EntityId& sceneId)
    {
        bool hasSelectedItems = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelectedItems, sceneId, &GraphCanvas::SceneRequests::HasSelectedItems);

        setEnabled(hasSelectedItems);
    }

    bool CopyGraphSelectionAction::TriggerAction(const AZ::EntityId& sceneId, const AZ::Vector2& scenePos)
    {
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::CopySelection);
        return false;
    }

    //////////////////////////////
    // PasteGraphSelectionAction
    //////////////////////////////

    PasteGraphSelectionAction::PasteGraphSelectionAction(QObject* parent)
        : CreateNodeAction("Paste", parent)
    {
    }

    void PasteGraphSelectionAction::RefreshAction(const AZ::EntityId& sceneId)
    {
        AZStd::string mimeType;
        GraphCanvas::SceneRequestBus::EventResult(mimeType, sceneId, &GraphCanvas::SceneRequests::GetCopyMimeType);

        bool isPasteable = QApplication::clipboard()->mimeData()->hasFormat(mimeType.c_str());
        setEnabled(isPasteable);
    }

    bool PasteGraphSelectionAction::TriggerAction(const AZ::EntityId& sceneId, const AZ::Vector2& scenePos)
    {
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::PasteAt, QPoint(scenePos.GetX(), scenePos.GetY()));
        return true;
    }

    //////////////////////////////////
    // DuplicateGraphSelectionAction
    //////////////////////////////////

    DuplicateGraphSelectionAction::DuplicateGraphSelectionAction(QObject* parent)
        : CreateNodeAction("Duplicate", parent)
    {
    }

    void DuplicateGraphSelectionAction::RefreshAction(const AZ::EntityId& sceneId)
    {
        bool hasSelectedItems = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelectedItems, sceneId, &GraphCanvas::SceneRequests::HasSelectedItems);

        setEnabled(hasSelectedItems);
    }

    bool DuplicateGraphSelectionAction::TriggerAction(const AZ::EntityId& sceneId, const AZ::Vector2& scenePos)
    {
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::DuplicateSelectionAt, QPoint(scenePos.GetX(), scenePos.GetY()));
        return true;
    }

    ///////////////////////////////
    // DeleteGraphSelectionAction
    ///////////////////////////////

    DeleteGraphSelectionAction::DeleteGraphSelectionAction(QObject* parent)
        : CreateNodeAction("Delete", parent)
    {
    }

    void DeleteGraphSelectionAction::RefreshAction(const AZ::EntityId& sceneId)
    {
        bool hasSelectedItems = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelectedItems, sceneId, &GraphCanvas::SceneRequests::HasSelectedItems);

        setEnabled(hasSelectedItems);
    }

    bool DeleteGraphSelectionAction::TriggerAction(const AZ::EntityId& sceneId, const AZ::Vector2& scenePos)
    {
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::DeleteSelection);
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

    bool AddCommentAction::TriggerAction(const AZ::EntityId& sceneId, const AZ::Vector2& scenePos)
    {
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::ClearSelection);

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateCommentNodeAndActivate);
        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Bus Node");

        if (graphCanvasEntity)
        {
            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePos);
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
        m_palette = new Widget::NodePalette(tr("Node Palette"), this, inContextMenu);

        actionWidget->setDefaultWidget(m_palette);

        // Ordering
        addAction(new CutGraphSelectionAction(this));
        addAction(new CopyGraphSelectionAction(this));
        addAction(new PasteGraphSelectionAction(this));
        addAction(new DuplicateGraphSelectionAction(this));
        addAction(new DeleteGraphSelectionAction(this));
        addSeparator();
        addAction(new AddCommentAction(this));
        addSeparator();
        addAction(new AddSelectedEntitiesAction(this));
        addSeparator();
        addAction(actionWidget);

        connect(this, &QMenu::aboutToShow, this, &CreateNodeContextMenu::SetupDisplay);
        connect(m_palette, &Widget::NodePalette::OnContextMenuSelection, this, &CreateNodeContextMenu::HandleContextMenuSelection);
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

    void CreateNodeContextMenu::RefreshActions(const AZ::EntityId& sceneId)
    {
        for (QAction* action : actions())
        {
            CreateNodeAction* createNodeAction = qobject_cast<CreateNodeAction*>(action);

            if (createNodeAction)
            {
                // Not sure if the action has been disabled or not, so enable everything.
                // Then let the refresh handle disabling it.
                createNodeAction->setEnabled(true);
                createNodeAction->RefreshAction(sceneId);
            }
        }
    }

    void CreateNodeContextMenu::FilterForSourceSlot(const AZ::EntityId& sceneId, const AZ::EntityId& sourceSlotId)
    {
        (void)sceneId;
        (void)sourceSlotId;
    }

    const Widget::NodePalette* CreateNodeContextMenu::GetNodePalette() const
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

