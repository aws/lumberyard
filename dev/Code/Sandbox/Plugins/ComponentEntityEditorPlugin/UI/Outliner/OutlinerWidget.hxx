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
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>
#include <AzToolsFramework/UI/SearchWidget/SearchWidgetTypes.hxx>

#include <QWidget>
#include <QtGui/QIcon>

#pragma once

class QAction;

namespace Ui
{
    class OutlinerWidgetUI;
}

class QItemSelection;

class OutlinerListModel;
class OutlinerSortFilterProxyModel;

class OutlinerWidget
    : public QWidget
    , private AzToolsFramework::EditorMetricsEventsBus::Handler
    , private AzToolsFramework::EditorPickModeRequests::Bus::Handler
    , private AzToolsFramework::EntityHighlightMessages::Bus::Handler
    , private OutlinerModelNotificationBus::Handler
    , private AzToolsFramework::ToolsApplicationEvents::Bus::Handler
    , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
{
    Q_OBJECT;
public:
    AZ_CLASS_ALLOCATOR(OutlinerWidget, AZ::SystemAllocator, 0)

        OutlinerWidget(QWidget* pParent = NULL, Qt::WindowFlags flags = 0);
    virtual ~OutlinerWidget();

    private Q_SLOTS:
    void OnSelectionChanged(const QItemSelection&, const QItemSelection&);
    void OnOpenTreeContextMenu(const QPoint& pos);
        
    void OnSearchTextChanged(const QString& activeTextFilter);
    void OnFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters);
    
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
    /// AzToolsFramework::EditorEntityContextNotificationBus implementation
    void OnStartPlayInEditor() override;
    void OnStopPlayInEditor() override;
    //////////////////////////////////////////////////////////////////////////

    // Build a selection object from the given entities. Entities already in the Widget's selection buffers are ignored.
    template <class EntityIdCollection>
    QItemSelection BuildSelectionFromEntities(const EntityIdCollection& entityIds);

    Ui::OutlinerWidgetUI* m_gui;
    OutlinerListModel* m_listModel;
    OutlinerSortFilterProxyModel* m_proxyModel;
    AZ::u64 m_selectionContextId;
    AZStd::vector<AZ::EntityId> m_selectedEntityIds;

    void PrepareSelection();
    void DoCreateEntity();
    void DoCreateEntityWithParent(const AZ::EntityId& parentId);
    void DoShowSlice();
    void DoDuplicateSelection();
    void DoDeleteSelection();
    void DoDeleteSelectionAndDescendants();
    void DoRenameSelection();
    void DoReparentSelection();
    void DoMoveEntityUp();
    void DoMoveEntityDown();
    void GoToEntitiesInViewport();
    void SetupActions();

    QAction* m_actionToShowSlice;
    QAction* m_actionToCreateEntity;
    QAction* m_actionToDeleteSelection;
    QAction* m_actionToDeleteSelectionAndDescendants;
    QAction* m_actionToRenameSelection;
    QAction* m_actionToReparentSelection;
    QAction* m_actionToMoveEntityUp;
    QAction* m_actionToMoveEntityDown;
    QAction* m_actionGoToEntitiesInViewport;

    void OnTreeItemClicked(const QModelIndex &index);
    void OnTreeItemExpanded(const QModelIndex &index);
    void OnTreeItemCollapsed(const QModelIndex &index);
    void OnExpandEntity(const AZ::EntityId& entityId, bool expand);
    void OnSelectEntity(const AZ::EntityId& entityId, bool selected);
    void OnEnableSelectionUpdates(bool enable);
    bool m_inObjectPickMode;
    bool m_initiatedObjectPickMode;

    void InvalidateFilter();
    void ClearFilter();

    AZ::EntityId GetEntityIdFromIndex(const QModelIndex& index) const;
    QModelIndex GetIndexFromEntityId(const AZ::EntityId& entityId) const;

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::OutlinerModelNotificationBus::Handler
    // Receive notification from the outliner model that we should scroll
    // to a given entity
    //////////////////////////////////////////////////////////////////////////
    void QueueScrollToNewContent(const AZ::EntityId& entityId) override;

    void ScrollToNewContent();
    bool m_scrollToNewContentQueued;
    AZ::EntityId m_scrollToEntityId;

    void QueueUpdateSelection();
    void UpdateSelection();
    AZStd::unordered_set<AZ::EntityId> m_entitiesToSelect;
    AZStd::unordered_set<AZ::EntityId> m_entitiesToDeselect;
    AZStd::unordered_set<AZ::EntityId> m_entitiesSelectedByOutliner;
    AZStd::unordered_set<AZ::EntityId> m_entitiesDeselectedByOutliner;
    bool m_selectionChangeQueued;
    bool m_selectionChangeInProgress;
    bool m_enableSelectionUpdates;

    QIcon m_emptyIcon;
    QIcon m_clearIcon;
};

#endif
