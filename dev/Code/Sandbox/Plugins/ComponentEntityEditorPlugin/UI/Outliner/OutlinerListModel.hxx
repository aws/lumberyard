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

#ifndef OUTLINER_VIEW_MODEL_H
#define OUTLINER_VIEW_MODEL_H

#include <AzCore/base.h>
#include <QtWidgets/QWidget>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QCheckBox>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/EntityBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/SearchWidget/SearchCriteriaWidget.hxx>

#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>

#pragma once

//! Model for items in the OutlinerTreeView.
//! Each item represents an Entity.
//! Items are parented in the tree according to their transform hierarchy.
class OutlinerListModel
    : public QAbstractItemModel
    , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    , private AzToolsFramework::EditorEntityInfoNotificationBus::Handler
    , private AzToolsFramework::ToolsApplicationEvents::Bus::Handler
    , private AzToolsFramework::EntityCompositionNotificationBus::Handler
    , private AZ::EntitySystemBus::Handler
{
    Q_OBJECT;

public:
    AZ_CLASS_ALLOCATOR(OutlinerListModel, AZ::SystemAllocator, 0);

    //! Columns of data to display about each Entity.
    enum Column
    {
        ColumnName,                 //!< Entity name
        ColumnVisibilityToggle,     //!< Visibility Icons
        ColumnLockToggle,           //!< Lock Icons
        ColumnSortIndex,            //!< Index of sort order
        ColumnCount                 //!< Total number of columns
    };

    enum EntryType
    {
        EntityType,
        SliceEntityType,
        SliceHandleType
    };

    enum Roles
    {
        VisibilityRole = Qt::UserRole + 1,
        SliceBackgroundRole,
        EntityIdRole,
        EntityTypeRole,
        SelectedRole,
        ChildSelectedRole,
        PartiallyVisibleRole,
        PartiallyLockedRole,
        ChildCountRole,
        ExpandedRole,
        RoleCount
    };

    enum EntityIcon
    {
        SliceHandleIcon,            // Icon used to decorate slice handles
        BrokenSliceHandleIcon,      // Icon used to decorate broken slice handles
        SliceEntityIcon,            // Icon used to decorate entities that are part of a slice instantiation
        StandardEntityIcon          // Icon used to decorate entities that are not part of a slice instantiation
    };

    // 8 Pixels of spacing is appropriate and matches the outliner concept work from the UI team.
    static const int s_OutlinerSpacing = 8;

    OutlinerListModel(QObject* parent = nullptr);
    ~OutlinerListModel();

    void Initialize();

    // Qt overrides.
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex&) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    Qt::DropActions supportedDropActions() const override;
    Qt::DropActions supportedDragActions() const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;

    bool IsSelected(const AZ::EntityId& entityId) const;

    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    QStringList mimeTypes() const override;

    QString GetSliceAssetName(const AZ::EntityId& entityId) const;

    QModelIndex GetIndexFromEntity(const AZ::EntityId& entityId, int column = 0) const;
    AZ::EntityId GetEntityFromIndex(const QModelIndex& index) const;

    bool FilterEntity(const AZ::EntityId& entityId);

Q_SIGNALS:
    void ExpandEntity(const AZ::EntityId& entityId, bool expand);
    void SelectEntity(const AZ::EntityId& entityId, bool select);
    void EnableSelectionUpdates(bool enable);
    void ResetFilter();
    void ReapplyFilter();

    public Q_SLOTS:
    void SearchStringChanged(const AZStd::string& filter);
    void SearchFilterChanged(const AZStd::vector<AZ::Uuid>& componentFilters);
    void OnEntityExpanded(const AZ::EntityId& entityId);
    void OnEntityCollapsed(const AZ::EntityId& entityId);

    // Buffer Processing Slots - These are called using single-shot events when the buffers begin to fill.
    bool CanReparentEntities(const AZ::EntityId& newParentId, const AzToolsFramework::EntityIdList& selectedEntityIds) const;
    bool ReparentEntities(const AZ::EntityId& newParentId, const AzToolsFramework::EntityIdList& selectedEntityIds, const AZ::EntityId& beforeEntityId = AZ::EntityId());

    //! Use the current filter setting and re-evaluate the filter.
    void InvalidateFilter();

protected:

    //! Editor entity context notification bus
    void OnEditorEntitiesReplacedBySlicedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& replacedEntitiesMap) override;
    void OnContextReset() override;

    //! Editor component lock interface to enable/disable selection of entity in the viewport.
    //! Setting the editor lock state on a parent will recursively set the flag on all descendants as well. (to match visibility)
    void ToggleEditorLockState(const AZ::EntityId& entityId);
    void SetEditorLockState(const AZ::EntityId& entityId, bool isLocked);

    //! Editor Visibility interface to enable/disable rendering in the viewport.
    //! Setting the editor visibility on a parent will recursively set the flag on all descendants as well.
    void ToggleEditorVisibility(const AZ::EntityId& entityId);
    void SetEditorVisibility(const AZ::EntityId& entityId, bool isVisible);

    void QueueEntityUpdate(AZ::EntityId entityId);
    void QueueAncestorUpdate(AZ::EntityId entityId);
    void QueueEntityToExpand(AZ::EntityId entityId, bool expand);
    void ProcessEntityUpdates();
    AZStd::unordered_set<AZ::EntityId> m_entitySelectQueue;
    AZStd::unordered_set<AZ::EntityId> m_entityExpandQueue;
    AZStd::unordered_set<AZ::EntityId> m_entityChangeQueue;
    bool m_entityChangeQueued;
    bool m_entityLayoutQueued;

    AZStd::string m_filterString;
    AZStd::vector<AZ::Uuid> m_componentFilters;

    void OnEntityCompositionChanged(const AzToolsFramework::EntityIdList& entityIds) override;

    void OnEntityInitialized(const AZ::EntityId& entityId) override;
    void AfterEntitySelectionChanged() override;

    //! AzToolsFramework::EditorEntityInfoNotificationBus::Handler
    //! Get notifications when the EditorEntityInfo changes so we can update our model
    void OnEntityInfoResetBegin() override;
    void OnEntityInfoResetEnd() override;
    void OnEntityInfoUpdatedAddChildBegin(AZ::EntityId parentId, AZ::EntityId childId) override;
    void OnEntityInfoUpdatedAddChildEnd(AZ::EntityId parentId, AZ::EntityId childId) override;
    void OnEntityInfoUpdatedRemoveChildBegin(AZ::EntityId parentId, AZ::EntityId childId) override;
    void OnEntityInfoUpdatedRemoveChildEnd(AZ::EntityId parentId, AZ::EntityId childId) override;
    void OnEntityInfoUpdatedOrderBegin(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index) override;
    void OnEntityInfoUpdatedOrderEnd(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index) override;
    void OnEntityInfoUpdatedSelection(AZ::EntityId entityId, bool selected) override;
    void OnEntityInfoUpdatedLocked(AZ::EntityId entityId, bool locked) override;
    void OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool visible) override;
    void OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& name) override;

    // Drag and Drop
    bool dropMimeDataComponentPalette(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
    bool dropMimeDataComponentAssets(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
    bool dropMimeDataEntities(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

    bool canDropMimeDataForEntityIds(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const;

    QMap<int, QVariant> itemData(const QModelIndex &index) const;
    QVariant dataForAll(const QModelIndex& index, int role) const;
    QVariant dataForName(const QModelIndex& index, int role) const;
    QVariant dataForVisibility(const QModelIndex& index, int role) const;
    QVariant dataForLock(const QModelIndex& index, int role) const;
    QVariant dataForSortIndex(const QModelIndex& index, int role) const;

    //! Request a hierarchy expansion
    void ExpandAncestors(const AZ::EntityId& entityId);
    bool IsExpanded(const AZ::EntityId& entityId) const;
    AZStd::unordered_map<AZ::EntityId, bool> m_entityExpansionState;

    void RestoreDescendantExpansion(const AZ::EntityId& entityId);
    void RestoreDescendantSelection(const AZ::EntityId& entityId);

    bool IsFiltered(const AZ::EntityId& entityId) const;
    AZStd::unordered_map<AZ::EntityId, bool> m_entityFilteredState;

    bool HasSelectedDescendant(const AZ::EntityId& entityId) const;

    bool AreAllDescendantsSameLockState(const AZ::EntityId& entityId) const;
    bool AreAllDescendantsSameVisibleState(const AZ::EntityId& entityId) const;

    // These are needed until we completely disassociated selection control from the outliner state to
    // keep track of selection state before/during/after filtering and searching
    AzToolsFramework::EntityIdList m_unfilteredSelectionEntityIds;
    void CacheSelectionIfAppropriate();
    void RestoreSelectionIfAppropriate();
    bool ShouldOverrideUnfilteredSelection();
};

// Class used to identify the visibility checkbox element for styling purposes
class OutlinerVisibilityCheckBox : public QCheckBox
{
    Q_OBJECT;
};

// Class used to identify the lock checkbox element for styling purposes
class OutlinerLockCheckBox : public QCheckBox
{
    Q_OBJECT;
};

/*!
* OutlinerItemDelegate exists to render custom item-types.
* Other item-types render in the default fashion.
*/
class OutlinerItemDelegate
    : public QStyledItemDelegate
{
public:
    AZ_CLASS_ALLOCATOR(OutlinerItemDelegate, AZ::SystemAllocator, 0);

    OutlinerItemDelegate(QWidget* parent = nullptr);

    // Qt overrides
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
private:
    // Mutability added because these are being used ONLY as renderers
    // for custom check boxes. The decision of whether or not to draw
    // them checked is tracked by the individual entities and items in
    // the hierarchy cache.
    mutable OutlinerVisibilityCheckBox m_visibilityCheckBox;
    mutable OutlinerVisibilityCheckBox m_visibilityCheckBoxWithBorder;
    mutable OutlinerLockCheckBox m_lockCheckBox;
    mutable OutlinerLockCheckBox m_lockCheckBoxWithBorder;
};

Q_DECLARE_METATYPE(AZ::ComponentTypeList); // allows type to be stored by QVariable

#endif
