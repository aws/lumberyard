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
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>
#include <AzToolsFramework/UI/SearchWidget/SearchWidgetTypes.hxx>
#include "OutlinerSearchWidget.h"

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

namespace EntityOutliner
{
    class DisplayOptionsMenu;
    enum class DisplaySortMode : unsigned char;
    enum class DisplayOption : unsigned char;
}

class OutlinerWidget
    : public QWidget
    , private AzToolsFramework::EditorMetricsEventsBus::Handler
    , private AzToolsFramework::EditorPickModeNotificationBus::Handler
    , private AzToolsFramework::EntityHighlightMessages::Bus::Handler
    , private OutlinerModelNotificationBus::Handler
    , private AzToolsFramework::ToolsApplicationEvents::Bus::Handler
    , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    , private AzToolsFramework::EditorEntityInfoNotificationBus::Handler
    , private AzToolsFramework::ComponentModeFramework::EditorComponentModeNotificationBus::Handler
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

    void OnSortModeChanged(EntityOutliner::DisplaySortMode sortMode);
    void OnDisplayOptionChanged(EntityOutliner::DisplayOption displayOption, bool enable);

private:

    QString FindCommonSliceAssetName(const AZStd::vector<AZ::EntityId>& entityList) const;

    AzFramework::EntityContextId GetPickModeEntityContextId();

    // EntityHighlightMessages
    virtual void EntityHighlightRequested(AZ::EntityId) override;
    virtual void EntityStrongHighlightRequested(AZ::EntityId) override;

    // EditorPickModeNotificationBus
    void OnEntityPickModeStarted() override;
    void OnEntityPickModeStopped() override;

    // EditorMetricsEventsBus
    void EntityCreated(const AZ::EntityId& entityId) override;

    // EditorEntityContextNotificationBus
    void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/, const AzFramework::SliceInstantiationTicket& /*ticket*/) override;
    void OnStartPlayInEditor() override;
    void OnStopPlayInEditor() override;
    void OnFocusInEntityOutliner(const AzToolsFramework::EntityIdList& entityIdList) override;

    /// AzToolsFramework::EditorEntityInfoNotificationBus implementation
    void OnEntityInfoUpdatedAddChildEnd(AZ::EntityId /*parentId*/, AZ::EntityId /*childId*/) override;
    void OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& /*name*/) override;
    

    // EditorMetricsEventsBus
    using EditorMetricsEventsBusTraits::EnteredComponentMode;
    using EditorMetricsEventsBusTraits::LeftComponentMode;

    // EditorComponentModeNotificationBus
    void EnteredComponentMode(const AZStd::vector<AZ::Uuid>& componentModeTypes) override;
    void LeftComponentMode(const AZStd::vector<AZ::Uuid>& componentModeTypes) override;

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
    void DoMoveEntityUp();
    void DoMoveEntityDown();
    void GoToEntitiesInViewport();
    void SetupActions();
    void SelectSliceRoot();

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
    void OnDropEvent();
    bool m_inObjectPickMode;

    void InvalidateFilter();
    void ClearFilter();

    AZ::EntityId GetEntityIdFromIndex(const QModelIndex& index) const;
    QModelIndex GetIndexFromEntityId(const AZ::EntityId& entityId) const;
    void ExtractEntityIdsFromSelection(const QItemSelection& selection, AzToolsFramework::EntityIdList& entityIdList) const;

    // AzToolsFramework::OutlinerModelNotificationBus::Handler
    // Receive notification from the outliner model that we should scroll
    // to a given entity
    void QueueScrollToNewContent(const AZ::EntityId& entityId) override;

    void ScrollToNewContent();
    bool m_scrollToNewContentQueued;
    bool m_scrollToSelectedEntity;
    bool m_dropOperationInProgress;
    bool m_expandSelectedEntity;
    bool m_focusInEntityOutliner;
    AZ::EntityId m_scrollToEntityId;

    void QueueUpdateSelection();
    void UpdateSelection();
    AzToolsFramework::EntityIdSet m_entitiesToSelect;
    AzToolsFramework::EntityIdSet m_entitiesToDeselect;
    AzToolsFramework::EntityIdSet m_entitiesSelectedByOutliner;
    AzToolsFramework::EntityIdSet m_entitiesDeselectedByOutliner;
    bool m_selectionChangeQueued;
    bool m_selectionChangeInProgress;
    bool m_enableSelectionUpdates;

    QIcon m_emptyIcon;
    QIcon m_clearIcon;

    void QueueContentUpdateSort(const AZ::EntityId& entityId);
    void SortContent();

    EntityOutliner::DisplayOptionsMenu* m_displayOptionsMenu;
    AzToolsFramework::EntityIdSet m_entitiesToSort;
    EntityOutliner::DisplaySortMode m_sortMode;
    bool m_sortContentQueued;
};

#endif
