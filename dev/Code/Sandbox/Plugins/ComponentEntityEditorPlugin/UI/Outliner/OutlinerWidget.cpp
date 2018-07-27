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
#include "stdafx.h"

#include "OutlinerListModel.hxx"
#include "OutlinerSortFilterProxyModel.hxx"
#include "OutlinerWidget.hxx"

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteUtil.hxx>

#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QTimer>

#include <UI/Outliner/ui_OutlinerWidget.h>

Q_DECLARE_METATYPE(AZ::Uuid);

OutlinerWidget::OutlinerWidget(QWidget* pParent, Qt::WindowFlags flags)
    : QWidget(pParent, flags)
    , m_gui(nullptr)
    , m_listModel(nullptr)
    , m_proxyModel(nullptr)
    , m_selectionContextId(0)
    , m_selectedEntityIds()
    , m_inObjectPickMode(false)
    , m_initiatedObjectPickMode(false)
    , m_scrollToNewContentQueued(false)
    , m_scrollToEntityId()
    , m_entitiesToSelect()
    , m_entitiesToDeselect()
    , m_selectionChangeQueued(false)
    , m_selectionChangeInProgress(false)
    , m_enableSelectionUpdates(true)
{
    m_gui = new Ui::OutlinerWidgetUI();
    m_gui->setupUi(this);

    m_listModel = aznew OutlinerListModel(this);

    const int autoExpandDelayMilliseconds = 500;
    m_gui->m_objectTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gui->m_objectTree->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_gui->m_objectTree->setAutoExpandDelay(autoExpandDelayMilliseconds);
    m_gui->m_objectTree->setDragEnabled(true);
    m_gui->m_objectTree->setDropIndicatorShown(true);
    m_gui->m_objectTree->setAcceptDrops(true);
    m_gui->m_objectTree->setDragDropOverwriteMode(false);
    m_gui->m_objectTree->setDragDropMode(QAbstractItemView::DragDrop);
    m_gui->m_objectTree->setDefaultDropAction(Qt::CopyAction);
    m_gui->m_objectTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gui->m_objectTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gui->m_objectTree->setAutoScrollMargin(20);
    connect(m_gui->m_objectTree, &QTreeView::customContextMenuRequested, this, &OutlinerWidget::OnOpenTreeContextMenu);

    // custom item delegate
    m_gui->m_objectTree->setItemDelegate(aznew OutlinerItemDelegate(m_gui->m_objectTree));

    m_proxyModel = aznew OutlinerSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_listModel);
    m_gui->m_objectTree->setModel(m_proxyModel);

    // Link up signals for informing the model of tree changes using the proxy as an intermediary
    connect(m_gui->m_objectTree, &QTreeView::clicked, this, &OutlinerWidget::OnTreeItemClicked);
    connect(m_gui->m_objectTree, &QTreeView::expanded, this, &OutlinerWidget::OnTreeItemExpanded);
    connect(m_gui->m_objectTree, &QTreeView::collapsed, this, &OutlinerWidget::OnTreeItemCollapsed);
    connect(m_listModel, &OutlinerListModel::ExpandEntity, this, &OutlinerWidget::OnExpandEntity);
    connect(m_listModel, &OutlinerListModel::SelectEntity, this, &OutlinerWidget::OnSelectEntity);
    connect(m_listModel, &OutlinerListModel::EnableSelectionUpdates, this, &OutlinerWidget::OnEnableSelectionUpdates);
    connect(m_listModel, &OutlinerListModel::ResetFilter, this, &OutlinerWidget::ClearFilter);
    connect(m_listModel, &OutlinerListModel::ReapplyFilter, this, &OutlinerWidget::InvalidateFilter);

    // Sorting is performed in a very specific way by the proxy model.
    // Disable sort indicator so user isn't confused by the fact
    // that they can't actually change how sorting works.
    m_gui->m_objectTree->hideColumn(OutlinerListModel::ColumnSortIndex);
    m_gui->m_objectTree->setSortingEnabled(true);
    m_gui->m_objectTree->header()->setSortIndicatorShown(false);

    // resize the icon columns so that the Visibility and Lock toggle icon columns stay right-justified
    m_gui->m_objectTree->header()->resizeSection(OutlinerListModel::ColumnName, width() - 70);
    m_gui->m_objectTree->header()->resizeSection(OutlinerListModel::ColumnVisibilityToggle, 24);
    m_gui->m_objectTree->header()->resizeSection(OutlinerListModel::ColumnLockToggle, 50);

    connect(m_gui->m_objectTree->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        &OutlinerWidget::OnSelectionChanged);

    connect(m_gui->m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &OutlinerWidget::OnSearchTextChanged);
    connect(m_gui->m_searchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, this, &OutlinerWidget::OnFilterChanged);

    AZ::SerializeContext* serializeContext = nullptr;
    EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);

    if (serializeContext)
    {
        AzToolsFramework::ComponentPaletteUtil::ComponentDataTable componentDataTable;
        AzToolsFramework::ComponentPaletteUtil::ComponentIconTable componentIconTable;
        AZStd::vector<AZ::ComponentServiceType> serviceFilter;
            
        AzToolsFramework::ComponentPaletteUtil::BuildComponentTables(serializeContext, AzToolsFramework::AppearsInGameComponentMenu, serviceFilter, componentDataTable, componentIconTable);

        for (const auto& categoryPair : componentDataTable)
        {
            const auto& componentMap = categoryPair.second;
            for (const auto& componentPair : componentMap)
            {
                m_gui->m_searchWidget->AddTypeFilter(categoryPair.first, componentPair.first, QVariant::fromValue<AZ::Uuid>(componentPair.second->m_typeId));
            }
        }
    }

    SetupActions();

    m_emptyIcon = QIcon();
    m_clearIcon = QIcon(":/AssetBrowser/Resources/close.png");

    m_listModel->Initialize();

    AzToolsFramework::EditorMetricsEventsBus::Handler::BusConnect();
    EditorPickModeRequests::Bus::Handler::BusConnect();
    EntityHighlightMessages::Bus::Handler::BusConnect();
    OutlinerModelNotificationBus::Handler::BusConnect();
    ToolsApplicationEvents::Bus::Handler::BusConnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();

}

OutlinerWidget::~OutlinerWidget()
{
    AzToolsFramework::EditorMetricsEventsBus::Handler::BusDisconnect();
    EditorPickModeRequests::Bus::Handler::BusDisconnect();
    EntityHighlightMessages::Bus::Handler::BusDisconnect();
    OutlinerModelNotificationBus::Handler::BusDisconnect();
    ToolsApplicationEvents::Bus::Handler::BusDisconnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();

    delete m_listModel;
    delete m_gui;
}

void OutlinerWidget::resizeEvent(QResizeEvent* event)
{
    // Every time we resize the widget, we need to grow or shring the name column to keep the two
    // icon columns right-justified in the pane.
    m_gui->m_objectTree->header()->resizeSection(OutlinerListModel::ColumnName, event->size().width() - 70);
}

// Users should be able to drag an entity in the outliner without selecting it.
// This is useful to allow for dragging entities into AZ::EntityId fields in the component editor.
// There are two obvious ways to allow for this:
//  1-Update selection behavior in the outliner's tree view to not occur until mouse release.
//  2-Revert outliner's selection to the Editor's selection on drag actions and focus loss.
//      Also, copy outliner's tree view selection to the Editor's selection on mouse release.
//  Currently, the first behavior is implemented.
void OutlinerWidget::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    if (m_selectionChangeInProgress || !m_enableSelectionUpdates)
    {
        return;
    }

    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    // Add the newly selected and deselected entities from the outliner to the appropriate selection buffer.

    for (const auto& selectedIndex : selected.indexes())
    {
        // Skip any column except the main name column...
        if (selectedIndex.column() == OutlinerListModel::ColumnName)
        {
            const AZ::EntityId entityId = GetEntityIdFromIndex(selectedIndex);
            if (entityId.IsValid())
            {
                m_entitiesSelectedByOutliner.insert(entityId);
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::MarkEntitySelected, entityId);
            }
        }
    }

    for (const auto& deselectedIndex : deselected.indexes())
    {
        // Skip any column except the main name column...
        if (deselectedIndex.column() == OutlinerListModel::ColumnName)
        {
            const AZ::EntityId entityId = GetEntityIdFromIndex(deselectedIndex);
            if (entityId.IsValid())
            {
                m_entitiesDeselectedByOutliner.insert(entityId);
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::MarkEntityDeselected, entityId);
            }
        }
    }

    m_entitiesDeselectedByOutliner.clear();
    m_entitiesSelectedByOutliner.clear();
}

void OutlinerWidget::EntityHighlightRequested(AZ::EntityId entityId)
{
    if (!entityId.IsValid())
    {
        return;
    }

    // Pulse opacity and duration are hardcoded. It'd be great to sample this from designer-provided settings,
    // such as the globally-applied stylesheet.
    const float pulseOpacityStart = 0.5f;
    const int pulseLengthMilliseconds = 500;

    // Highlighting an entity in the outliner is accomplished by:
    //  1. Make sure the entity's row is actually visible.
    //      Accomplished with the handy "scrollTo" command.
    //  2. Create a single colored widget
    //      accomplished through setting the style sheet with a new background color.
    //  3. Apply an graphics opacity effect to this widget, so it can be made transparent.
    //  4. Creating a property animation to modify the opacity value over time, deleting the widget at 0 opacity.
    //  5. Setting this QWidget as an "index widget" to overlay over the entity row to be highlighted.

    const QModelIndex proxyIndex = GetIndexFromEntityId(entityId);

    // Scrolling to the entity will make sure that it is visible.
    // This will automatically open parents.
    m_gui->m_objectTree->scrollTo(proxyIndex);

    QWidget* testWidget = new QWidget(m_gui->m_objectTree);

    testWidget->setProperty("PulseHighlight", true);

    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(testWidget);
    effect->setOpacity(pulseOpacityStart);
    testWidget->setGraphicsEffect(effect);

    QPropertyAnimation* anim = new QPropertyAnimation(testWidget);
    anim->setTargetObject(effect);
    anim->setPropertyName("opacity");
    anim->setDuration(pulseLengthMilliseconds);
    anim->setStartValue(effect->opacity());
    anim->setEndValue(0);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    m_gui->m_objectTree->setIndexWidget(proxyIndex, testWidget);
}

// Currently a "strong highlight" request is handled by replacing the current selection with the requested entity.
void OutlinerWidget::EntityStrongHighlightRequested(AZ::EntityId entityId)
{
    // This can be sent from the Entity Inspector, and changing the selection can affect the state of that control
    // so use singleShot with 0 to queue the response until any other events sent to the Entity Inspector have been processed
    QTimer::singleShot(0, this, [entityId, this]() {
        const QModelIndex proxyIndex = GetIndexFromEntityId(entityId);
        if (!proxyIndex.isValid())
        {
            return;
        }

        // Scrolling to the entity will make sure that it is visible.
        // This will automatically open parents.
        m_gui->m_objectTree->scrollTo(proxyIndex);

        // Not setting "ignoreSignal" because this will set both outliner and world selections.
        m_gui->m_objectTree->selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect);
    });
}

void OutlinerWidget::QueueUpdateSelection()
{
    if (!m_selectionChangeQueued)
    {
        QTimer::singleShot(0, this, &OutlinerWidget::UpdateSelection);
        m_selectionChangeQueued = true;
    }
}

void OutlinerWidget::UpdateSelection()
{
    if (m_selectionChangeQueued)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        m_selectionChangeInProgress = true;

        if (m_entitiesToDeselect.size() >= 1000)
        {
            // Calling Deselect for a large number of items is very slow,
            // use a single ClearAndSelect call instead.
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "OutlinerWidget::ModelEntitySelectionChanged:ClearAndSelect");

            AzToolsFramework::EntityIdList selectedEntities;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

            m_gui->m_objectTree->selectionModel()->select(
                BuildSelectionFromEntities(selectedEntities), QItemSelectionModel::ClearAndSelect);
        }
        else
        {
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "OutlinerWidget::ModelEntitySelectionChanged:Deselect");
                m_gui->m_objectTree->selectionModel()->select(
                    BuildSelectionFromEntities(m_entitiesToDeselect), QItemSelectionModel::Deselect);
            }
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "OutlinerWidget::ModelEntitySelectionChanged:Select");
                m_gui->m_objectTree->selectionModel()->select(
                    BuildSelectionFromEntities(m_entitiesToSelect), QItemSelectionModel::Select);
            }
        }

        m_gui->m_objectTree->update();
        m_entitiesToSelect.clear();
        m_entitiesToDeselect.clear();
        m_selectionChangeQueued = false;
        m_selectionChangeInProgress = false;
    }
}

    template <class EntityIdCollection>
    QItemSelection OutlinerWidget::BuildSelectionFromEntities(const EntityIdCollection& entityIds)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    QItemSelection selection;

    for (const auto& entityId : entityIds)
    {
        const QModelIndex proxyIndex = GetIndexFromEntityId(entityId);
        if (proxyIndex.isValid())
        {
            selection.select(proxyIndex, proxyIndex);
        }
    }

    return selection;
}

void OutlinerWidget::OnOpenTreeContextMenu(const QPoint& pos)
{
    bool isDocumentOpen = false;
    EBUS_EVENT_RESULT(isDocumentOpen, AzToolsFramework::EditorRequests::Bus, IsLevelDocumentOpen);
    if (!isDocumentOpen)
    {
        return;
    }

    QMenu* contextMenu = new QMenu(this);

    // Populate global context menu.
    EBUS_EVENT(AzToolsFramework::EditorEvents::Bus,
        PopulateEditorGlobalContextMenu,
        contextMenu,
        AZ::Vector2::CreateZero(),
        AzToolsFramework::EditorEvents::eECMF_HIDE_ENTITY_CREATION | AzToolsFramework::EditorEvents::eECMF_USE_VIEWPORT_CENTER);

    PrepareSelection();

    //register rename menu action
    if (!m_selectedEntityIds.empty())
    {
        contextMenu->addSeparator();

        if (m_selectedEntityIds.size() == 1)
        {
            contextMenu->addAction(m_actionToRenameSelection);
        }

        if (m_selectedEntityIds.size() == 1)
        {
            AZ::EntityId entityId = m_selectedEntityIds[0];

            AZ::EntityId parentId;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

            AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(parentId);

            if (entityOrderArray.size() > 1)
            {
                if (AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), entityId) != entityOrderArray.end())
                {
                    if (entityOrderArray.front() != entityId)
                    {
                        contextMenu->addAction(m_actionToMoveEntityUp);
                    }

                    if (entityOrderArray.back() != entityId)
                    {
                        contextMenu->addAction(m_actionToMoveEntityDown);
                    }
                }
            }
        }

        contextMenu->addSeparator();

        contextMenu->addAction(m_actionGoToEntitiesInViewport);
    }

    contextMenu->exec(m_gui->m_objectTree->mapToGlobal(pos));
    delete contextMenu;
}

QString OutlinerWidget::FindCommonSliceAssetName(const AZStd::vector<AZ::EntityId>& entityList) const
{
    if (entityList.size() > 0)
    {
        //  Check if all selected items are in the same slice
        QString commonSliceName = m_listModel->GetSliceAssetName(entityList.front());
        if (!commonSliceName.isEmpty())
        {
            bool hasCommonSliceName = true;
            for (const AZ::EntityId& id : entityList)
            {
                QString sliceName = m_listModel->GetSliceAssetName(id);

                if (sliceName != commonSliceName)
                {
                    hasCommonSliceName = false;
                    break;
                }
            }

            if (hasCommonSliceName)
            {
                return commonSliceName;
            }
        }
    }

    return QString();
}

void OutlinerWidget::PrepareSelection()
{
    m_selectedEntityIds.clear();
    EBUS_EVENT_RESULT(m_selectedEntityIds, AzToolsFramework::ToolsApplicationRequests::Bus, GetSelectedEntities);
}

void OutlinerWidget::DoCreateEntity()
{
    // Navigation triggered
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(
        qApp->activePopupWidget() ?
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu :
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

    PrepareSelection();

    if (m_selectedEntityIds.empty())
    {
        DoCreateEntityWithParent(AZ::EntityId());
        return;
    }

    if (m_selectedEntityIds.size() == 1)
    {
        DoCreateEntityWithParent(m_selectedEntityIds.front());
        return;
    }
}

void OutlinerWidget::DoCreateEntityWithParent(const AZ::EntityId& parentId)
{
    // Navigation triggered
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(
        qApp->activePopupWidget() ?
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu :
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

    PrepareSelection();

    AZ::EntityId entityId;
    EBUS_EVENT_RESULT(entityId, AzToolsFramework::EditorRequests::Bus, CreateNewEntity, parentId);

    // Create Entity metrics event (Right click Entity Outliner)
    EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, EntityCreated, entityId);
}

void OutlinerWidget::DoShowSlice()
{
    // Navigation triggered
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(
        qApp->activePopupWidget() ?
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu :
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

    PrepareSelection();

    if (!m_selectedEntityIds.empty())
    {
        QString commonSliceName = FindCommonSliceAssetName(m_selectedEntityIds);
        if (!commonSliceName.isEmpty())
        {
            EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, ShowAssetInBrowser, commonSliceName.toStdString().c_str());
        }
    }
}

void OutlinerWidget::DoDuplicateSelection()
{
    // Navigation triggered
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(
        qApp->activePopupWidget() ?
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu :
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

    PrepareSelection();

    if (!m_selectedEntityIds.empty())
    {
        AzToolsFramework::ScopedUndoBatch undo("Duplicate Entity(s)");

        bool handled = false;
        EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, CloneSelection, handled);
    }
}

void OutlinerWidget::DoDeleteSelection()
{
    // Navigation triggered
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(
        qApp->activePopupWidget() ?
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu :
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

    PrepareSelection();

    EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, DeleteSelectedEntities, false);

    PrepareSelection();
}

void OutlinerWidget::DoDeleteSelectionAndDescendants()
{
    // Navigation triggered
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(
        qApp->activePopupWidget() ?
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu :
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

    PrepareSelection();

    EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, DeleteSelectedEntities, true);

    PrepareSelection();
}

void OutlinerWidget::DoRenameSelection()
{
    // Navigation triggered
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(
        qApp->activePopupWidget() ?
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu :
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

    PrepareSelection();

    if (m_selectedEntityIds.size() == 1)
    {
        const QModelIndex proxyIndex = GetIndexFromEntityId(m_selectedEntityIds.front());
        if (proxyIndex.isValid())
        {
            m_gui->m_objectTree->setCurrentIndex(proxyIndex);
            m_gui->m_objectTree->QTreeView::edit(proxyIndex);
        }
    }
}

void OutlinerWidget::DoReparentSelection()
{
    // Navigation triggered
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(
        qApp->activePopupWidget() ?
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu :
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

    PrepareSelection();

    if (!m_selectedEntityIds.empty())
    {
        EBUS_EVENT(AzToolsFramework::EditorPickModeRequests::Bus, StopObjectPickMode);
        EBUS_EVENT(AzToolsFramework::EditorPickModeRequests::Bus, StartObjectPickMode);
        m_initiatedObjectPickMode = true;
    }
}

void OutlinerWidget::DoMoveEntityUp()
{
    // Navigation triggered
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(
        qApp->activePopupWidget() ?
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu :
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

    PrepareSelection();

    if (m_selectedEntityIds.size() != 1)
    {
        return;
    }

    AZ::EntityId entityId = m_selectedEntityIds[0];
    AZ_Assert(entityId.IsValid(), "Invalid entity selected to move up");
    if (!entityId.IsValid())
    {
        return;
    }

    AZ::EntityId parentId;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

    AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(parentId);

    auto entityItr = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), entityId);
    if (entityOrderArray.empty() || entityOrderArray.front() == entityId || entityItr == entityOrderArray.end())
    {
        return;
    }

    AzToolsFramework::ScopedUndoBatch undo("Move Entity Up");

    // Parent is dirty due to sort change
    undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(parentId));

    AZStd::swap(*entityItr, *(entityItr - 1));
    AzToolsFramework::SetEntityChildOrder(parentId, entityOrderArray);
}

void OutlinerWidget::DoMoveEntityDown()
{
    // Navigation triggered
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(
        qApp->activePopupWidget() ?
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu :
        AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

    PrepareSelection();

    if (m_selectedEntityIds.size() != 1)
    {
        return;
    }

    AZ::EntityId entityId = m_selectedEntityIds[0];
    AZ_Assert(entityId.IsValid(), "Invalid entity selected to move down");
    if (!entityId.IsValid())
    {
        return;
    }

    AZ::EntityId parentId;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

    AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(parentId);

    auto entityItr = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), entityId);
    if (entityOrderArray.empty() || entityOrderArray.back() == entityId || entityItr == entityOrderArray.end())
    {
        return;
    }

    AzToolsFramework::ScopedUndoBatch undo("Move Entity Down");

    // Parent is dirty due to sort change
    undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(parentId));

    AZStd::swap(*entityItr, *(entityItr + 1));
    AzToolsFramework::SetEntityChildOrder(parentId, entityOrderArray);
}

void OutlinerWidget::GoToEntitiesInViewport()
{
    EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, GoToSelectedOrHighlightedEntitiesInViewports);
}

void OutlinerWidget::SetupActions()
{
    m_actionToShowSlice = new QAction(tr("Show Slice"), this);
    m_actionToShowSlice->setShortcut(tr("Ctrl+Shift+F"));
    m_actionToShowSlice->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToShowSlice, &QAction::triggered, this, &OutlinerWidget::DoShowSlice);
    addAction(m_actionToShowSlice);

    m_actionToCreateEntity = new QAction(tr("Create Entity"), this);
    m_actionToCreateEntity->setShortcut(tr("Ctrl+Alt+N"));
    m_actionToCreateEntity->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToCreateEntity, &QAction::triggered, this, &OutlinerWidget::DoCreateEntity);
    addAction(m_actionToCreateEntity);

    m_actionToDeleteSelection = new QAction(tr("Delete Selection"), this);
    m_actionToDeleteSelection->setShortcut(QKeySequence("Shift+Delete"));
    m_actionToDeleteSelection->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToDeleteSelection, &QAction::triggered, this, &OutlinerWidget::DoDeleteSelection);
    addAction(m_actionToDeleteSelection);

    m_actionToDeleteSelectionAndDescendants = new QAction(tr("Delete Selection And Descendants"), this);
    m_actionToDeleteSelectionAndDescendants->setShortcut(QKeySequence::Delete);
    m_actionToDeleteSelectionAndDescendants->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToDeleteSelectionAndDescendants, &QAction::triggered, this, &OutlinerWidget::DoDeleteSelectionAndDescendants);
    addAction(m_actionToDeleteSelectionAndDescendants);

    m_actionToRenameSelection = new QAction(tr("Rename Selection"), this);
#ifdef AZ_PLATFORM_APPLE_OSX
    // "Alt+Return" translates to Option+Return on macOS
    m_actionToRenameSelection->setShortcut(tr("Alt+Return"));
#endif
    m_actionToRenameSelection->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToRenameSelection, &QAction::triggered, this, &OutlinerWidget::DoRenameSelection);
    addAction(m_actionToRenameSelection);

    m_actionToReparentSelection = new QAction(tr("Reparent Selection"), this);
    m_actionToReparentSelection->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToReparentSelection, &QAction::triggered, this, &OutlinerWidget::DoReparentSelection);
    addAction(m_actionToReparentSelection);

    m_actionToMoveEntityUp = new QAction(tr("Move Up"), this);
    m_actionToMoveEntityUp->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToMoveEntityUp, &QAction::triggered, this, &OutlinerWidget::DoMoveEntityUp);
    addAction(m_actionToMoveEntityUp);

    m_actionToMoveEntityDown = new QAction(tr("Move Down"), this);
    m_actionToMoveEntityDown->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToMoveEntityDown, &QAction::triggered, this, &OutlinerWidget::DoMoveEntityDown);
    addAction(m_actionToMoveEntityDown);

    m_actionGoToEntitiesInViewport = new QAction(tr("Goto Entities in Viewport"), this);
    m_actionGoToEntitiesInViewport->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionGoToEntitiesInViewport, &QAction::triggered, this, &OutlinerWidget::GoToEntitiesInViewport);
    addAction(m_actionGoToEntitiesInViewport);
}

void OutlinerWidget::StartObjectPickMode()
{
    m_gui->m_objectTree->setDragEnabled(false);
    m_gui->m_objectTree->setSelectionMode(QAbstractItemView::NoSelection);
    m_gui->m_objectTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_inObjectPickMode = true;
}

void OutlinerWidget::StopObjectPickMode()
{
    m_gui->m_objectTree->setDragEnabled(true);
    m_gui->m_objectTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gui->m_objectTree->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_inObjectPickMode = false;
    m_initiatedObjectPickMode = false;
}

void OutlinerWidget::OnPickModeSelect(AZ::EntityId id)
{
    if (m_initiatedObjectPickMode)
    {
        if (id.IsValid() && !m_listModel->IsSelected(id))
        {
            PrepareSelection();
            m_listModel->ReparentEntities(id, m_selectedEntityIds);
        }
    }
}

void OutlinerWidget::OnTreeItemClicked(const QModelIndex &index)
{
    if (m_inObjectPickMode)
    {
        const AZ::EntityId entityId = GetEntityIdFromIndex(index);
        if (entityId.IsValid())
        {
            EBUS_EVENT(AzToolsFramework::EditorPickModeRequests::Bus, OnPickModeSelect, entityId);
            EBUS_EVENT(AzToolsFramework::EditorPickModeRequests::Bus, StopObjectPickMode);
        }
    }
}

void OutlinerWidget::OnTreeItemExpanded(const QModelIndex &index)
{
    m_listModel->OnEntityExpanded(GetEntityIdFromIndex(index));
}

void OutlinerWidget::OnTreeItemCollapsed(const QModelIndex &index)
{
    m_listModel->OnEntityCollapsed(GetEntityIdFromIndex(index));
}

void OutlinerWidget::OnExpandEntity(const AZ::EntityId& entityId, bool expand)
{
    m_gui->m_objectTree->setExpanded(GetIndexFromEntityId(entityId), expand);
}

void OutlinerWidget::OnSelectEntity(const AZ::EntityId& entityId, bool selected)
{
    if (selected)
    {
        if (m_entitiesSelectedByOutliner.find(entityId) == m_entitiesSelectedByOutliner.end())
        {
            m_entitiesToSelect.insert(entityId);
            m_entitiesToDeselect.erase(entityId);
        }
    }
    else
    {
        if (m_entitiesDeselectedByOutliner.find(entityId) == m_entitiesDeselectedByOutliner.end())
        {
            m_entitiesToSelect.erase(entityId);
            m_entitiesToDeselect.insert(entityId);
        }
    }
    QueueUpdateSelection();
}

void OutlinerWidget::OnEnableSelectionUpdates(bool enable)
{
    m_enableSelectionUpdates = enable;
}

void OutlinerWidget::EntityCreated(const AZ::EntityId& entityId)
{
    QueueScrollToNewContent(entityId);
    // When a new entity is created we need to make sure to apply the filter
    m_listModel->FilterEntity(entityId);
}

void OutlinerWidget::ScrollToNewContent()
{
    m_scrollToNewContentQueued = false;

    const QModelIndex proxyIndex = GetIndexFromEntityId(m_scrollToEntityId);
    if (!proxyIndex.isValid())
    {
        QueueScrollToNewContent(m_scrollToEntityId);
        return;
    }

    m_gui->m_objectTree->scrollTo(proxyIndex);
}

void OutlinerWidget::QueueScrollToNewContent(const AZ::EntityId& entityId)
{
    if (entityId.IsValid())
    {
        m_scrollToEntityId = entityId;
        if (!m_scrollToNewContentQueued)
        {
            m_scrollToNewContentQueued = true;
            QTimer::singleShot(16, this, &OutlinerWidget::ScrollToNewContent);
        }
    }
}

AZ::EntityId OutlinerWidget::GetEntityIdFromIndex(const QModelIndex& index) const
{
    if (index.isValid())
    {
        const QModelIndex modelIndex = m_proxyModel->mapToSource(index);
        if (modelIndex.isValid())
        {
            return m_listModel->GetEntityFromIndex(modelIndex);
        }
    }

    return AZ::EntityId();
}

QModelIndex OutlinerWidget::GetIndexFromEntityId(const AZ::EntityId& entityId) const
{
    if (entityId.IsValid())
    {
        QModelIndex modelIndex = m_listModel->GetIndexFromEntity(entityId);
        if (modelIndex.isValid())
        {
            return m_proxyModel->mapFromSource(modelIndex);
        }
    }

    return QModelIndex();
}

void OutlinerWidget::OnSearchTextChanged(const QString& activeTextFilter)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    AZStd::string filterString = activeTextFilter.toUtf8().data();

    m_listModel->SearchStringChanged(filterString);
    m_proxyModel->UpdateFilter();
}

void OutlinerWidget::OnFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
{
    AZStd::vector<AZ::Uuid> componentFilters;
    componentFilters.reserve(activeTypeFilters.count());

    for (auto filter : activeTypeFilters)
    {
        AZ::Uuid typeId = qvariant_cast<AZ::Uuid>(filter.metadata);
        componentFilters.push_back(typeId);
    }

    m_listModel->SearchFilterChanged(componentFilters);
    m_proxyModel->UpdateFilter();
}

void OutlinerWidget::InvalidateFilter()
{
    m_listModel->InvalidateFilter();
    m_proxyModel->UpdateFilter();
}

void OutlinerWidget::ClearFilter()
{
    m_gui->m_searchWidget->ClearTextFilter();
    m_gui->m_searchWidget->ClearTypeFilter();
}

void OutlinerWidget::OnStartPlayInEditor()
{
    setEnabled(false);
}

void OutlinerWidget::OnStopPlayInEditor()
{
    setEnabled(true);
}


#include <UI/Outliner/OutlinerWidget.moc>