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

#include <QAbstractListModel>
#include <QAbstractItemView>
#include <QListView>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTimer>
#include <QSortFilterProxyModel>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <Editor/View/Widgets/NodeTableView.h>

class QAction;
class QLineEdit;

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class NodeOutlineModel 
            : public QAbstractTableModel
            , public GraphCanvas::NodeTitleNotificationsBus::MultiHandler
        {
            Q_OBJECT
        public:

            enum ColumnIndex
            {
                Name,
                Count
            };

            enum CustomRole
            {
                NodeIdRole = Qt::UserRole
            };

            NodeOutlineModel(const AZ::EntityId& sceneId, QObject* parent = nullptr)
                : QAbstractTableModel(parent)
                , m_sceneId(sceneId)
            {
                RefreshNodeIds();
            }

            static const char* GetMimeType() { return "lumberyard/x-scriptcanvas-outline"; }

            int columnCount(const QModelIndex &parent = QModelIndex()) const override
            {
                return ColumnIndex::Count;
            }

            int rowCount(const QModelIndex &parent = QModelIndex()) const override
            {
                return aznumeric_cast<int>(m_nodeIds.size());
            }

            QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

            Qt::ItemFlags flags(const QModelIndex &index) const override
            {
                return Qt::ItemFlags(
                    Qt::ItemIsEnabled |
                    Qt::ItemIsDragEnabled |
                    Qt::ItemIsDropEnabled |
                    Qt::ItemIsSelectable);
            }

            AZ::EntityId GetSceneId() const
            {
                return m_sceneId;
            }

            QItemSelectionRange GetSelectionRangeForRow(int row)
            {                
                return QItemSelectionRange(createIndex(row, 0, nullptr), createIndex(row, columnCount(), nullptr));
            }

            void RefreshNodeIds()
            {
                GraphCanvas::NodeTitleNotificationsBus::MultiHandler::BusDisconnect();

                GraphCanvas::SceneRequestBus::EventResult(m_nodeIds, m_sceneId, &GraphCanvas::SceneRequests::GetNodes);

                m_nodeIds.erase(
                    AZStd::remove_if(m_nodeIds.begin(), m_nodeIds.end(), [](AZ::EntityId nodeId) -> bool {
                        bool show = false;
                        GraphCanvas::NodeRequestBus::EventResult(show, nodeId, &GraphCanvas::NodeRequestBus::Events::ShowInOutliner);
                        return !show;
                    }),
                    m_nodeIds.end()
                );

                for (const AZ::EntityId& nodeId : m_nodeIds)
                {
                    GraphCanvas::NodeTitleNotificationsBus::MultiHandler::BusConnect(nodeId);
                }
            }

            // NodeTitleNotificationsBus
            void OnTitleChanged()
            {
                const AZ::EntityId* nodeId = GraphCanvas::NodeTitleNotificationsBus::GetCurrentBusId();

                if (nodeId)
                {
                    int row = -1;

                    for (size_t i = 0; i < m_nodeIds.size(); ++i)
                    {
                        if (m_nodeIds[i] == (*nodeId))
                        {
                            row = static_cast<int>(i);
                            break;
                        }
                    }

                    if (row >= 0)
                    {
                        dataChanged(createIndex(row, 0), createIndex(row, columnCount() - 1));
                    }
                }
            }
            ////

            private:
                
                AZStd::vector< AZ::EntityId > m_nodeIds;
                AZ::EntityId m_sceneId;
        };

        class NodeOutlineModelSortFilterProxyModel
            : public QSortFilterProxyModel
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(NodeOutlineModelSortFilterProxyModel, AZ::SystemAllocator, 0);

            NodeOutlineModelSortFilterProxyModel(QObject* parent)
                : QSortFilterProxyModel(parent) {}

            bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

            void SetFilter(const QString& filter);
            void ClearFilter();

        private:
            QString m_filter;
            QRegExp m_filterRegex;
        };


        class NodeOutlinerHost : public QWidget
        {
            Q_OBJECT
        public:
            QSize sizeHint() const override
            {
                return QSize(400, 400);
            }
        };

        class NodeOutliner 
            : public AzQtComponents::StyledDockWidget
            , public GraphCanvas::AssetEditorNotificationBus::Handler
            , public GraphCanvas::SceneNotificationBus::Handler
        {
            Q_OBJECT

        public:

            NodeOutliner(QWidget* parent = nullptr);
            ~NodeOutliner();

            void RefreshDisplay(const AZStd::vector<AZ::EntityId>& selectedEntities);

            // GraphCanvas::AssetEditorNotificationBus::Handler
            void OnActiveGraphChanged(const GraphCanvas::GraphId& sceneId);
            ////

            void OnItemSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
            void OnTableDoubleClicked(QModelIndex index);

            // GraphCanvas::SceneNotificationBus
            void OnNodeAdded(const AZ::EntityId& /*nodeId*/) override;
            void OnNodeRemoved(const AZ::EntityId& /*nodeId*/) override;
            ////

        private:

            void SetHasSelection(bool hasSelection);

            void FocusOnSearchFilter();

            void ClearFilter();
            void UpdateFilter();
            void OnQuickFilterChanged();

            void RefreshModel();

            // Shortcut Handlers
            void OnDelete();
            void OnCopy();
            void OnCut();
            void OnPaste();

            bool m_manipulatingSelection;
            
            AZ::EntityId m_sceneId;
            QTableView* m_nodeTableView;
            QLineEdit* m_quickFilter;
            NodeOutlineModelSortFilterProxyModel* m_model;

            QTimer m_filterTimer;

            QAction* m_deleteAction;
            QAction* m_copyAction;
            QAction* m_cutAction;
            QAction* m_pasteAction;
        };
    }
}