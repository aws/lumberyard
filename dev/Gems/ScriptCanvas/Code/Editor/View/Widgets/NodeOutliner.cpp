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
#include <precompiled.h>

#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QAction>
#include <QMenu>
#include <QScopedValueRollback>
#include <QLineEdit>
#include <QTimer>

#include "NodeOutliner.h"

#include <GraphCanvas/Components/VisualBus.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

Q_DECLARE_METATYPE(AZ::EntityId);

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        #include <Editor/View/Widgets/NodeOutliner.moc>

        NodeOutliner::NodeOutliner(QWidget* parent /*= nullptr*/) 
            : AzQtComponents::StyledDockWidget(parent)
            , m_manipulatingSelection(false)
            , m_model(nullptr)
        {
            setWindowTitle("Node Outliner");
            setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

            QWidget* host = new NodeOutlinerHost();
            QVBoxLayout* layout = new QVBoxLayout();
            host->setLayout(layout);

            m_nodeTableView = new QTableView();
            m_nodeTableView->verticalHeader()->hide();
            m_nodeTableView->horizontalHeader()->hide();
            m_nodeTableView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
            m_nodeTableView->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
            m_nodeTableView->setAutoScroll(true);
            m_nodeTableView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

            connect(m_nodeTableView, &QAbstractItemView::doubleClicked, this, &NodeOutliner::OnTableDoubleClicked);

            // Set up all of our actions
            m_deleteAction = new QAction("Delete", this);
            m_deleteAction->setShortcut(QKeySequence(QKeySequence::Delete));
            m_deleteAction->setAutoRepeat(false);
            connect(m_deleteAction, &QAction::triggered, this, &NodeOutliner::OnDelete);            
            
            m_copyAction = new QAction("Copy", this);
            m_copyAction->setShortcut(QKeySequence(QKeySequence::Copy));
            m_copyAction->setAutoRepeat(false);
            connect(m_copyAction, &QAction::triggered, this, &NodeOutliner::OnCopy);            

            m_cutAction = new QAction("Cut", this);
            m_cutAction->setShortcut(QKeySequence(QKeySequence::Cut));            
            m_cutAction->setAutoRepeat(false);
            connect(m_cutAction, &QAction::triggered, this, &NodeOutliner::OnCut);            

            m_pasteAction = new QAction("Paste", this);
            m_pasteAction->setShortcut(QKeySequence(QKeySequence::Paste));
            m_pasteAction->setAutoRepeat(false);
            connect(m_pasteAction, &QAction::triggered, this, &NodeOutliner::OnPaste);

            m_quickFilter = new QLineEdit();
            QObject::connect(m_quickFilter, &QLineEdit::textChanged, this, &NodeOutliner::OnQuickFilterChanged);
            QObject::connect(m_quickFilter, &QLineEdit::returnPressed, this, &NodeOutliner::UpdateFilter);

            QAction* clearAction = m_quickFilter->addAction(QIcon(":/ScriptCanvasEditorResources/Resources/lineedit_clear.png"), QLineEdit::TrailingPosition);
            QObject::connect(clearAction, &QAction::triggered, this, &NodeOutliner::ClearFilter);

            addAction(m_cutAction);
            addAction(m_copyAction);
            addAction(m_pasteAction);
            addAction(m_deleteAction);

            // Tell the widget to auto create our context menu, for now
            setContextMenuPolicy(Qt::ActionsContextMenu);

            layout->addWidget(m_quickFilter);
            layout->addWidget(m_nodeTableView);

            setWidget(host);

            m_filterTimer.setInterval(250);
            m_filterTimer.setSingleShot(true);
            m_filterTimer.stop();

            QObject::connect(&m_filterTimer, &QTimer::timeout, this, &NodeOutliner::UpdateFilter);

            GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);
        }

        NodeOutliner::~NodeOutliner()
        {
            GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();
        }

        void NodeOutliner::RefreshDisplay(const AZStd::vector<AZ::EntityId>& selectedEntities)
        {
            if (m_nodeTableView->model())
            {
                QScopedValueRollback<bool> manipulatingSelectionGuard(m_manipulatingSelection, true);

                auto* outlinerModel = qobject_cast<NodeOutlineModel*>(qobject_cast<NodeOutlineModelSortFilterProxyModel*>(m_nodeTableView->model())->sourceModel());

                outlinerModel->RefreshNodeIds();
                outlinerModel->layoutChanged();

                QItemSelectionModel* selectionModel = m_nodeTableView->selectionModel();
                QItemSelection itemSelection;

                if (!selectedEntities.empty())
                {
                    // TODO: select the nodes in the outliner, ensure they remain synchronized
                }

                SetHasSelection(!selectedEntities.empty());                
                selectionModel->select(itemSelection, QItemSelectionModel::ClearAndSelect);
            }
        }

        void NodeOutliner::OnActiveGraphChanged(const AZ::EntityId& sceneId)
        {
            m_sceneId = sceneId;

            GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

            if (sceneId.IsValid())
            {
                GraphCanvas::SceneNotificationBus::Handler::BusConnect(sceneId);

                NodeOutlineModel* model = new NodeOutlineModel(sceneId, this);

                m_model = new NodeOutlineModelSortFilterProxyModel(nullptr);
                m_model->setSourceModel(model);
                m_model->sort(0);

                m_nodeTableView->setModel(m_model);

                // Setup table view.
                m_nodeTableView->horizontalHeader()->setSectionResizeMode(NodeOutlineModel::ColumnIndex::Name, QHeaderView::Stretch);

                connect(m_nodeTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NodeOutliner::OnItemSelectionChanged);
            }
            else
            {
                m_nodeTableView->setModel(nullptr);
            }

            m_nodeTableView->update();
        }

        void NodeOutliner::OnItemSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
        {
            if (!m_manipulatingSelection)
            {
                bool validSelection = false;
                if (NodeOutlineModel* outlineModel = qobject_cast<NodeOutlineModel*>(qobject_cast<NodeOutlineModelSortFilterProxyModel*>(m_nodeTableView->model())->sourceModel()))
                {
                    AZ::EntityId sceneId = outlineModel->GetSceneId();
                    AZStd::vector<AZ::EntityId> nodeIds;

                    GraphCanvas::SceneRequestBus::EventResult(nodeIds, sceneId, &GraphCanvas::SceneRequests::GetNodes);

                    for (const QModelIndex& selectedIndex : selected.indexes())
                    {
                        if (selectedIndex.row() < nodeIds.size())
                        {
                            AZ::EntityId nodeId = selectedIndex.data().value<AZ::EntityId>();
                            GraphCanvas::SceneMemberUIRequestBus::Event(nodeId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);

                            validSelection = true;
                        }
                    }

                    for (const QModelIndex& selectedIndex : deselected.indexes())
                    {
                        if (selectedIndex.row() < nodeIds.size())
                        {
                            AZ::EntityId nodeId = selectedIndex.data().value<AZ::EntityId>();
                            GraphCanvas::SceneMemberUIRequestBus::Event(nodeId, &GraphCanvas::SceneMemberUIRequests::SetSelected, false);
                        }
                    }
                }

                SetHasSelection(validSelection);
            }
        }

        void NodeOutliner::OnTableDoubleClicked(QModelIndex index)
        {
            if (auto* outlineModel = qobject_cast<NodeOutlineModel*>(qobject_cast<NodeOutlineModelSortFilterProxyModel*>(m_nodeTableView->model())->sourceModel()))
            {
                QScopedValueRollback<bool> manipulatingSelectionGuard(m_manipulatingSelection, true);

                AZ::EntityId sceneId = outlineModel->GetSceneId();
                AZStd::vector<AZ::EntityId> nodeIds;
                GraphCanvas::SceneRequestBus::EventResult(nodeIds, sceneId, &GraphCanvas::SceneRequests::GetNodes);

                // Clear any selected nodes
                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::ClearSelection);

                if (index.row() < nodeIds.size())
                {
                    AZ::EntityId nodeId = index.data(NodeOutlineModel::NodeIdRole).value<AZ::EntityId>();

                    // Only select the desired one
                    AZ::EntityId selectedNodeId = index.data(NodeOutlineModel::NodeIdRole).value<AZ::EntityId>();
                    if (selectedNodeId.IsValid())
                    {
                        GraphCanvas::SceneMemberUIRequestBus::Event(selectedNodeId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
                    }

                    QGraphicsItem* nodeItem{};
                    GraphCanvas::VisualRequestBus::EventResult(nodeItem, nodeId, &GraphCanvas::VisualRequests::AsGraphicsItem);
                    QGraphicsScene* graphicsScene{};
                    GraphCanvas::SceneRequestBus::EventResult(graphicsScene, sceneId, &GraphCanvas::SceneRequests::AsQGraphicsScene);
                    if (graphicsScene && nodeItem)
                    {
                        for (auto* graphicsView : graphicsScene->views())
                        {
                            graphicsView->centerOn(nodeItem);
                        }
                    }
                }
            }
        }

        void NodeOutliner::SetHasSelection(bool hasSelection)
        {
            m_deleteAction->setEnabled(hasSelection);
            m_copyAction->setEnabled(hasSelection);
            m_cutAction->setEnabled(hasSelection);
        }

        void NodeOutliner::FocusOnSearchFilter()
        {
            m_quickFilter->setFocus(Qt::FocusReason::MouseFocusReason);
        }

        void NodeOutliner::ClearFilter()
        {
            {
                QSignalBlocker blocker(m_quickFilter);
                m_quickFilter->setText("");
                m_model->ClearFilter();
            }

            UpdateFilter();
        }

        void NodeOutliner::UpdateFilter()
        {
            if (m_model)
            {
                m_model->SetFilter(m_quickFilter->text());
                m_model->invalidate();
            }
        }

        void NodeOutliner::OnQuickFilterChanged()
        {
            m_filterTimer.stop();
            m_filterTimer.start();
        }

        void NodeOutliner::OnDelete()
        {
            GraphCanvas::SceneRequestBus::Event(m_sceneId, &GraphCanvas::SceneRequests::DeleteSelection);
        }

        void NodeOutliner::OnCopy()
        {
            GraphCanvas::SceneRequestBus::Event(m_sceneId, &GraphCanvas::SceneRequests::CopySelection);
        }

        void NodeOutliner::OnCut()
        {
            GraphCanvas::SceneRequestBus::Event(m_sceneId, &GraphCanvas::SceneRequests::CutSelection);
        }

        void NodeOutliner::OnPaste()
        {
            GraphCanvas::SceneRequestBus::Event(m_sceneId, &GraphCanvas::SceneRequests::Paste);
        }

        bool NodeOutlineModelSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
        {
            NodeOutlineModel* model = qobject_cast<NodeOutlineModel*>(sourceModel());
            if (!model)
            {
                return false;
            }

            QModelIndex index = model->index(sourceRow, 0, sourceParent);

            if (m_filter.isEmpty())
            {
                return true;
            }
            
            QString test = model->data(index, Qt::DisplayRole).toString();

            bool showRow = test.lastIndexOf(m_filterRegex) >= 0;

            return showRow;
        }

        void NodeOutlineModelSortFilterProxyModel::SetFilter(const QString& filter)
        {
            m_filter = filter;
            m_filterRegex = QRegExp(m_filter, Qt::CaseInsensitive);
        }

        void NodeOutlineModelSortFilterProxyModel::ClearFilter()
        {
            m_filter.clear();
            m_filterRegex = QRegExp(m_filter, Qt::CaseInsensitive);
        }

        void NodeOutliner::OnNodeAdded(const AZ::EntityId& /*nodeId*/)
        {
            RefreshModel();
        }

        void NodeOutliner::OnNodeRemoved(const AZ::EntityId& /*nodeId*/)
        {
            RefreshModel();
        }

        void NodeOutliner::RefreshModel()
        {
            if (!m_nodeTableView->model())
            {
                return;
            }

            QScopedValueRollback<bool> manipulatingSelectionGuard(m_manipulatingSelection, true);

            auto* outlinerModel = qobject_cast<NodeOutlineModel*>(qobject_cast<NodeOutlineModelSortFilterProxyModel*>(m_nodeTableView->model())->sourceModel());
            if (outlinerModel)
            {
                QItemSelectionModel* selectionModel = m_nodeTableView->selectionModel();
                selectionModel->clearSelection();

                outlinerModel->RefreshNodeIds();

                m_model->invalidate();
            }
        }

        QVariant NodeOutlineModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
        {
            AZ::EntityId nodeId;

            if (index.row() < m_nodeIds.size())
            {
                nodeId = m_nodeIds[index.row()];
            }

            switch (role)
            {
            case NodeIdRole:
                return QVariant::fromValue<AZ::EntityId>(nodeId);

            case Qt::DisplayRole:
            {
                if (index.column() == ColumnIndex::Name)
                {
                    AZStd::string title;

                    GraphCanvas::NodeTitleRequestBus::EventResult(title, nodeId, &GraphCanvas::NodeTitleRequests::GetTitle);
                    return QVariant(title.c_str());
                }
            }
            break;

            default:
                break;
            }

            return QVariant();
        }

    }

}

