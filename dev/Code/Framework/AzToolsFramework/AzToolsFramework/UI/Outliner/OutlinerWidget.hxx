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

#ifndef OUTLINER_VIEW_H
#define OUTLINER_VIEW_H

#include "OutlinerCacheBus.h"

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/base.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>
#include <AzToolsFramework/UI/SearchWidget/SearchWidgetTypes.hxx>

#include <QtWidgets/QWidget>

#pragma once

class QAction;

namespace Ui
{
    class OutlinerWidgetUI;
}

class QItemSelection;

namespace AzToolsFramework
{
    class OutlinerListModel;
    class OutlinerSortFilterProxyModel;

    class OutlinerWidget
        : public QWidget
        , private EditorMetricsEventsBus::Handler
        , private EditorPickModeRequests::Bus::Handler
        , private EntityHighlightMessages::Bus::Handler
        , private OutlinerModelNotificationBus::Handler
        , private ToolsApplicationEvents::Bus::Handler
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(OutlinerWidget, AZ::SystemAllocator, 0)

            OutlinerWidget(QWidget* pParent = NULL, Qt::WindowFlags flags = 0);
        virtual ~OutlinerWidget();

    private Q_SLOTS:
        void OnSelectionChanged(const QItemSelection&, const QItemSelection&);
        void OnSearchCriteriaChanged(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator);
        void OnOpenTreeContextMenu(const QPoint& pos);

    private:
        void RevertMouseSelection();
        void resizeEvent(QResizeEvent* event) override;

        QString FindCommonSliceAssetName(const AZStd::vector<AZ::EntityId>& entityList) const;

        // EntityHighlightMessages
        virtual void EntityHighlightRequested(AZ::EntityId) override;
        virtual void EntityStrongHighlightRequested(AZ::EntityId) override;

        //////////////////////////////////////////////////////////////////////////
        // EditorPickModeRequests::Bus::Handler
        void StartObjectPickMode() override;
        void StopObjectPickMode() override;
        void OnPickModeSelect(AZ::EntityId /*id*/) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorMetricsEventsBus::Handler
        void EntityCreated(const AZ::EntityId& entityId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::OutlinerModelNotificationBus::Handler
        // Receive notification from the outliner model that a list of entities
        // have been selected
        //////////////////////////////////////////////////////////////////////////
        void ModelEntitySelectionChanged(const AZStd::unordered_set<AZ::EntityId>& selectedEntityIdList, const AZStd::unordered_set<AZ::EntityId>& deselectedEntityIdList) override;

        // Build a selection object from the given entities. Entities already in the Widget's selection buffers are ignored.
        QItemSelection BuildSelectionFromEntities(const AZStd::unordered_set<AZ::EntityId>& entityIds);

        Ui::OutlinerWidgetUI* m_gui;
        OutlinerListModel* m_listModel;
        OutlinerSortFilterProxyModel* m_proxyModel;
        AZ::u64 m_selectionContextId;
        AZStd::vector<AZ::EntityId> m_selectedEntityIds;

        void PrepareSelection();
        void PrepareAction();
        void DoShowSearchBox();
        void DoCreateEntity();
        void DoCreateEntityWithParent(const AZ::EntityId& parentId);
        void DoShowSlice();
        void DoDuplicateSelection();
        void DoDeleteSelection();
        void DoDeleteSelectionAndDescendants();
        void DoRenameSelection();
        void DoReparentSelection();
        void SetupActions();

        QAction* m_actionToShowSlice;
        QAction* m_actionToShowSearchBox;
        QAction* m_actionToCreateEntity;
        QAction* m_actionToDeleteSelection;
        QAction* m_actionToDeleteSelectionAndDescendants;
        QAction* m_actionToRenameSelection;
        QAction* m_actionToReparentSelection;

        void OnTreeItemClicked(const QModelIndex &index);
        bool m_inObjectPickMode;
        bool m_initiatedObjectPickMode;

        AZ::EntityId GetEntityIdFromIndex(const QModelIndex& index) const;
        QModelIndex GetIndexFromEntityId(const AZ::EntityId& entityId) const;

        void ScrollToNewContent();
        void QueueScrollToNewContent(const AZ::EntityId& entityId);
        bool m_scrollToNewContentQueued;
        AZ::EntityId m_scrollToEntityId;
    };

} // WorldEditor

#endif
