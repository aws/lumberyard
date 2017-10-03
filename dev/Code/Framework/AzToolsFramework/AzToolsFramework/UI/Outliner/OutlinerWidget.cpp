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
#include "ToolsComponents/EditorSelectionAccentingBus.h"

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>

#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QtCore/QPropertyAnimation>
#include <QtCore/qtimer>

#include <UI/Outliner/ui_OutlinerWidget.h>

namespace AzToolsFramework
{
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
        m_gui = aznew Ui::OutlinerWidgetUI();
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
        connect(m_gui->m_objectTree, &QTreeView::customContextMenuRequested, this, &OutlinerWidget::OnOpenTreeContextMenu);

        // custom item delegate
        m_gui->m_objectTree->setItemDelegate(new OutlinerItemDelegate(m_gui->m_objectTree));

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

        QStringList tags;
        tags << tr("name");

        m_gui->m_filterWidget->SetAcceptedTags(tags, tags[0]);

        connect(m_gui->m_objectTree->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &OutlinerWidget::OnSelectionChanged);

        connect(m_gui->m_filterWidget,
            &AzToolsFramework::SearchCriteriaWidget::SearchCriteriaChanged,
            this,
            &OutlinerWidget::OnSearchCriteriaChanged);

        SetupActions();

        m_listModel->Initialize();

        EditorMetricsEventsBus::Handler::BusConnect();
        EditorPickModeRequests::Bus::Handler::BusConnect();
        EntityHighlightMessages::Bus::Handler::BusConnect();
        OutlinerModelNotificationBus::Handler::BusConnect();
        ToolsApplicationEvents::Bus::Handler::BusConnect();
    }

    OutlinerWidget::~OutlinerWidget()
    {
        EditorMetricsEventsBus::Handler::BusDisconnect();
        EditorPickModeRequests::Bus::Handler::BusDisconnect();
        EntityHighlightMessages::Bus::Handler::BusDisconnect();
        OutlinerModelNotificationBus::Handler::BusDisconnect();
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();

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

        for (const auto& deselectedIndex : deselected.indexes())
        {
            // Skip any column except the main name column...
            if (deselectedIndex.column() == OutlinerListModel::ColumnName)
            {
                const AZ::EntityId entityId = GetEntityIdFromIndex(deselectedIndex);
                if (entityId.IsValid())
                {
                    m_entitiesDeselectedByOutliner.insert(entityId);
                    ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::MarkEntityDeselected, entityId);
                }
            }
        }

        for (const auto& selectedIndex : selected.indexes())
        {
            // Skip any column except the main name column...
            if (selectedIndex.column() == OutlinerListModel::ColumnName)
            {
                const AZ::EntityId entityId = GetEntityIdFromIndex(selectedIndex);
                if (entityId.IsValid())
                {
                    m_entitiesSelectedByOutliner.insert(entityId);
                    ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::MarkEntitySelected, entityId);
                }
            }
        }

        m_entitiesDeselectedByOutliner.clear();
        m_entitiesSelectedByOutliner.clear();
    }

    void OutlinerWidget::OnSearchCriteriaChanged(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        m_listModel->SearchCriteriaChanged(criteriaList, filterOperator);
        m_proxyModel->UpdateFilter();
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

        QWidget* testWidget = aznew QWidget(m_gui->m_objectTree);

        testWidget->setProperty("PulseHighlight", true);

        QGraphicsOpacityEffect* effect = aznew QGraphicsOpacityEffect(testWidget);
        effect->setOpacity(pulseOpacityStart);
        testWidget->setGraphicsEffect(effect);

        QPropertyAnimation* anim = aznew QPropertyAnimation(testWidget);
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

                EntityIdList selectedEntities;
                ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

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
        EBUS_EVENT_RESULT(isDocumentOpen, EditorRequests::Bus, IsLevelDocumentOpen);
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
                EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);

                EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(parentId);

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
        EBUS_EVENT_RESULT(m_selectedEntityIds, ToolsApplicationRequests::Bus, GetSelectedEntities);
    }

    void OutlinerWidget::PrepareAction()
    {
        // Navigation triggered
        AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(
            qApp->activePopupWidget() ?
            AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu :
            AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::Shortcut);

        PrepareSelection();
    }

    void OutlinerWidget::DoShowSearchBox()
    {
        m_gui->m_filterWidget->SelectTextEntryBox();
    }

    void OutlinerWidget::DoCreateEntity()
    {
        PrepareAction();

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
        PrepareAction();

        AZ::EntityId entityId;
        EBUS_EVENT_RESULT(entityId, AzToolsFramework::EditorRequests::Bus, CreateNewEntity, parentId);

        // Create Entity metrics event (Right click Entity Outliner)
        EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, EntityCreated, entityId);
    }

    void OutlinerWidget::DoShowSlice()
    {
        PrepareAction();

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
        PrepareAction();

        if (!m_selectedEntityIds.empty())
        {
            ScopedUndoBatch undo("Duplicate Entity(s)");

            bool handled = false;
            EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, CloneSelection, handled);
        }
    }

    void OutlinerWidget::DoDeleteSelection()
    {
        PrepareAction();

        EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, DeleteSelectedEntities, false);

        PrepareSelection();
    }

    void OutlinerWidget::DoDeleteSelectionAndDescendants()
    {
        PrepareAction();

        EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, DeleteSelectedEntities, true);

        PrepareSelection();
    }

    void OutlinerWidget::DoRenameSelection()
    {
        PrepareAction();

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
        PrepareAction();

        if (!m_selectedEntityIds.empty())
        {
            EBUS_EVENT(AzToolsFramework::EditorPickModeRequests::Bus, StopObjectPickMode);
            EBUS_EVENT(AzToolsFramework::EditorPickModeRequests::Bus, StartObjectPickMode);
            m_initiatedObjectPickMode = true;
        }
    }

    void OutlinerWidget::DoMoveEntityUp()
    {
        PrepareAction();

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
        EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);

        EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(parentId);

        auto entityItr = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), entityId);
        if (entityOrderArray.empty() || entityOrderArray.front() == entityId || entityItr == entityOrderArray.end())
        {
            return;
        }

        ScopedUndoBatch undo("Move Entity Up");

        // Parent is dirty due to sort change
        undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(parentId));

        AZStd::swap(*entityItr, *(entityItr - 1));
        AzToolsFramework::SetEntityChildOrder(parentId, entityOrderArray);
    }

    void OutlinerWidget::DoMoveEntityDown()
    {
        PrepareAction();

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
        EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);

        EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(parentId);

        auto entityItr = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), entityId);
        if (entityOrderArray.empty() || entityOrderArray.back() == entityId || entityItr == entityOrderArray.end())
        {
            return;
        }

        ScopedUndoBatch undo("Move Entity Down");

        // Parent is dirty due to sort change
        undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(parentId));

        AZStd::swap(*entityItr, *(entityItr + 1));
        AzToolsFramework::SetEntityChildOrder(parentId, entityOrderArray);
    }

    void OutlinerWidget::SetupActions()
    {
        m_actionToShowSlice = new QAction(tr("Show Slice"), this);
        m_actionToShowSlice->setShortcut(tr("Ctrl+Shift+F"));
        m_actionToShowSlice->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToShowSlice, &QAction::triggered, this, &OutlinerWidget::DoShowSlice);
        addAction(m_actionToShowSlice);

        m_actionToShowSearchBox = new QAction(tr("Show Search Box"), this);
        m_actionToShowSearchBox->setShortcut(QKeySequence::Find);
        m_actionToShowSearchBox->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToShowSearchBox, &QAction::triggered, this, &OutlinerWidget::DoShowSearchBox);
        addAction(m_actionToShowSearchBox);

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

} // WorldEditor

#include <UI/Outliner/OutlinerWidget.moc>