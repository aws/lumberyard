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

#include <QtCore/QMimeData>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QStyle>
#include <QGuiApplication>
#include <QTimer>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/std/sort.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/SelectionComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

////////////////////////////////////////////////////////////////////////////
// OutlinerListModel
////////////////////////////////////////////////////////////////////////////

OutlinerListModel::OutlinerListModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_entitySelectQueue()
    , m_entityExpandQueue()
    , m_entityChangeQueue()
    , m_entityChangeQueued(false)
    , m_entityLayoutQueued(false)
    , m_entityExpansionState()
    , m_entityFilteredState()
{
}

OutlinerListModel::~OutlinerListModel()
{
    AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusDisconnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusDisconnect();
    AzToolsFramework::EntityCompositionNotificationBus::Handler::BusDisconnect();
    AZ::EntitySystemBus::Handler::BusDisconnect();
}

void OutlinerListModel::Initialize()
{
    AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusConnect();
    AzToolsFramework::EntityCompositionNotificationBus::Handler::BusConnect();
    AZ::EntitySystemBus::Handler::BusConnect();
}

int OutlinerListModel::rowCount(const QModelIndex& parent) const
{
    auto parentId = GetEntityFromIndex(parent);

    AZStd::size_t childCount = 0;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(childCount, parentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildCount);
    return (int)childCount;
}

int OutlinerListModel::columnCount(const QModelIndex& /*parent*/) const
{
    return ColumnCount;
}

QModelIndex OutlinerListModel::index(int row, int column, const QModelIndex& parent) const
{
    // sanity check
    if (!hasIndex(row, column, parent) || (parent.isValid() && parent.column() != 0) || (row < 0 || row >= rowCount(parent)))
    {
        return QModelIndex();
    }

    auto parentId = GetEntityFromIndex(parent);

    AZ::EntityId childId;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(childId, parentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChild, row);
    return GetIndexFromEntity(childId, column);
}

QVariant OutlinerListModel::data(const QModelIndex& index, int role) const
{
    auto id = GetEntityFromIndex(index);
    if (id.IsValid())
    {
        switch (index.column())
        {
        case ColumnName:
            return dataForName(index, role);

        case ColumnVisibilityToggle:
            return dataForVisibility(index, role);

        case ColumnLockToggle:
            return dataForLock(index, role);

        case ColumnSortIndex:
            return dataForSortIndex(index, role);
        }
    }

    return QVariant();
}

QMap<int, QVariant> OutlinerListModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> roles = QAbstractItemModel::itemData(index);
    for (int i = Qt::UserRole; i < RoleCount; ++i)
    {
        QVariant variantData = data(index, i);
        if (variantData.isValid())
        {
            roles.insert(i, variantData);
        }
    }

    return roles;
}

QVariant OutlinerListModel::dataForAll(const QModelIndex& index, int role) const
{
    auto id = GetEntityFromIndex(index);

    switch (role)
    {
    case EntityIdRole:
        return (AZ::u64)id;

    case SliceBackgroundRole:
    {
        bool isSliceRoot = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceRoot, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceRoot);
        bool isSliceEntity = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceEntity, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceEntity);
        return isSliceRoot && isSliceEntity;
    }

    case SelectedRole:
    {
        bool isSelected = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSelected, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSelected);
        return isSelected;
    }

    case ChildSelectedRole:
        return HasSelectedDescendant(id);

    case PartiallyVisibleRole:
        return !AreAllDescendantsSameVisibleState(id);

    case PartiallyLockedRole:
        return !AreAllDescendantsSameLockState(id);

    case ChildCountRole:
    {
        AZStd::size_t childCount = 0;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(childCount, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildCount);
        return (int)childCount;
    }

    case ExpandedRole:
        return IsExpanded(id);

    case VisibilityRole:
        return !IsFiltered(id);
    }

    return QVariant();
}

QVariant OutlinerListModel::dataForName(const QModelIndex& index, int role) const
{
    auto id = GetEntityFromIndex(index);

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        AZStd::string name;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(name, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetName);
        return QString(name.data());
    }

    case Qt::ToolTipRole:
    {
        AZStd::string sliceAssetName;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(sliceAssetName, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetSliceAssetName);
        return !sliceAssetName.empty() ? QString("Slice asset: %1").arg(sliceAssetName.data()) : QString("Slice asset: This entity is not part of a slice.");
    }

    case EntityTypeRole:
    {
        bool isSliceRoot = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceRoot, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceRoot);
        bool isSliceEntity = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceEntity, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceEntity);

        if (isSliceEntity)
        {
            return isSliceRoot ? SliceHandleType : SliceEntityType;
        }

        // Guaranteed to return a valid type.
        return EntityType;
    }

    case Qt::ForegroundRole:
    {
        bool isSliceEntity = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceEntity, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceEntity);

        // We use the parent's palette because the GUI Application palette is returning the wrong colors
        auto parentWidgetPtr = static_cast<QWidget*>(QObject::parent());
        return QBrush(parentWidgetPtr->palette().color(isSliceEntity ? QPalette::Link : QPalette::Text));
    }

    case Qt::DecorationRole:
    {
        bool isSliceEntity = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceEntity, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceEntity);
        return QIcon(isSliceEntity ? ":/Icons/Slice_Entity_Icon.tif" : ":/Icons/Entity_Icon.tif");
    }
    }

    return dataForAll(index, role);
}

QVariant OutlinerListModel::dataForVisibility(const QModelIndex& index, int role) const
{
    auto id = GetEntityFromIndex(index);

    switch (role)
    {
    case Qt::CheckStateRole:
    {
        bool isVisible = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isVisible, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);
        return isVisible ? Qt::Checked : Qt::Unchecked;
    }

    case Qt::ToolTipRole:
        return QString("Show/Hide Entity");
    }

    return dataForAll(index, role);
}

QVariant OutlinerListModel::dataForLock(const QModelIndex& index, int role) const
{
    auto id = GetEntityFromIndex(index);

    switch (role)
    {
    case Qt::CheckStateRole:
    {
        bool isLocked = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isLocked, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsLocked);
        return isLocked ? Qt::Checked : Qt::Unchecked;
    }

    case Qt::ToolTipRole:
        return QString("Lock/Unlock Entity (Locked means the entity is not selectable)");
    }

    return dataForAll(index, role);
}

QVariant OutlinerListModel::dataForSortIndex(const QModelIndex& index, int role) const
{
    auto id = GetEntityFromIndex(index);

    switch (role)
    {
    case Qt::DisplayRole:
        AZ::u64 sortIndex = 0;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(sortIndex, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetIndexForSorting);
        return sortIndex;
    }

    return dataForAll(index, role);
}

bool OutlinerListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    auto id = GetEntityFromIndex(index);

    switch (role)
    {
    case Qt::CheckStateRole:
    {
        if (value.canConvert<Qt::CheckState>())
        {
            switch (index.column())
            {
            case ColumnVisibilityToggle:
            {
                ToggleEditorVisibility(id);
            }
            break;

            case ColumnLockToggle:
            {
                ToggleEditorLockState(id);
            }
            break;
            }
        }
    }
    break;

    case Qt::EditRole:
    {
        if (index.column() == ColumnName && !value.toString().isEmpty())
        {
            auto id = GetEntityFromIndex(index);
            if (id.IsValid())
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, id);

                if (entity)
                {
                    AzToolsFramework::ScopedUndoBatch undo("Rename Entity");

                    entity->SetName(value.toString().toStdString().c_str());
                    undo.MarkEntityDirty(entity->GetId());

                    EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
                }
                else
                {
                    AZStd::string name;
                    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(name, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetName);
                    AZ_Assert(entity, "Outliner entry cannot locate entity with name: %s", name);
                }
            }
        }
    }
    break;
    }

    return QAbstractItemModel::setData(index, value, role);
}

QModelIndex OutlinerListModel::parent(const QModelIndex& index) const
{
    auto id = GetEntityFromIndex(index);
    if (id.IsValid())
    {
        AZ::EntityId parentId;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);
        return GetIndexFromEntity(parentId, index.column());
    }
    return QModelIndex();
}

bool OutlinerListModel::IsSelected(const AZ::EntityId& entityId) const
{
    bool isSelected = false;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSelected, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSelected);
    return isSelected;
}

Qt::ItemFlags OutlinerListModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags itemFlags = QAbstractItemModel::flags(index);
    switch (index.column())
    {
    case ColumnVisibilityToggle:
    case ColumnLockToggle:
        itemFlags |= Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        break;

    case ColumnName:
        itemFlags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
        break;

    default:
        itemFlags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
        break;
    }

    return itemFlags;
}

Qt::DropActions OutlinerListModel::supportedDropActions() const
{
    return Qt::CopyAction;
}

Qt::DropActions OutlinerListModel::supportedDragActions() const
{
    return Qt::CopyAction;
}


bool OutlinerListModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    if (data)
    {
        if (parent.isValid() && row == -1 && column == -1)
        {
            if (data->hasFormat(AzToolsFramework::ComponentTypeMimeData::GetMimeType()) ||
                data->hasFormat(AzToolsFramework::ComponentAssetMimeDataContainer::GetMimeType()))
            {
                return true;
            }
        }

        if (canDropMimeDataForEntityIds(data, action, row, column, parent))
        {
            return true;
        }
    }

    return false;
}

bool OutlinerListModel::canDropMimeDataForEntityIds(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    (void)action;
    (void)column;
    (void)row;

    if (!data || !data->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()) || column > 0)
    {
        return false;
    }

    QByteArray arrayData = data->data(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

    AzToolsFramework::EditorEntityIdContainer entityIdListContainer;
    if (!entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
    {
        return false;
    }

    AZ::EntityId newParentId = GetEntityFromIndex(parent);
    if (!CanReparentEntities(newParentId, entityIdListContainer.m_entityIds))
    {
        return false;
    }

    return true;
}

// Expected behavior when dropping an entity in the outliner view model:
//  DragEntity onto DropEntity, neither in each other's hierarchy:
//      DropEntity becomes parent of DragEntity
//  DragEntity onto DropEntity, DropEntity is DragEntity's parent:
//      DragEntity sets its parent to DropEntity's parent.
//  DragEntity onto DropEntity, DragEntity is DropEntity's parent:
//      No change occurs.
//  DragEntity onto DropEntity, DropEntity is DragEntity's grandfather (or deeper):
//      DragEntity's parent becomes DropEntity.
//  DragEntity onto DropEntity, DragEntity is DropEntity's grandfather (or deeper):
//      No change occurs.
//  DragEntity onto DragEntity
//      No change occurs.
//  DragEntity and DragEntityChild onto DropEntity:
//      DragEntity behaves as define previously.
//      DragEntityChild behaves as a second DragEntity, following normal drag rules.
//      Example: DragEntity and DragEntityChild dropped onto DragEntity:
//          DragEntity has no change occur.
//          DragEntityChild follows the rule "DragEntity onto DropEntity, DropEntity is DragEntity's parent"
bool OutlinerListModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (action == Qt::IgnoreAction)
    {
        return true;
    }

    // Navigation triggered - Drag+Drop from Component Palette to Entity Outliner / Drag+Drop from File Browser to Entity Outliner
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::DragAndDrop);

    if (parent.isValid() && row == -1 && column == -1)
    {
        // Handle drop from the component palette window.
        if (data->hasFormat(AzToolsFramework::ComponentTypeMimeData::GetMimeType()))
        {
            return dropMimeDataComponentPalette(data, action, row, column, parent);
        }

        if (data->hasFormat(AzToolsFramework::ComponentAssetMimeDataContainer::GetMimeType()))
        {
            return dropMimeDataComponentAssets(data, action, row, column, parent);
        }
    }

    if (data->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()))
    {
        return dropMimeDataEntities(data, action, row, column, parent);
    }

    return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
}

bool OutlinerListModel::dropMimeDataComponentPalette(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    (void)action;
    (void)row;
    (void)column;

    AZ::EntityId dropTargetId = GetEntityFromIndex(parent);
    AzToolsFramework::EntityIdList entityIds{ dropTargetId };

    AzToolsFramework::ComponentTypeMimeData::ClassDataContainer classDataContainer;
    AzToolsFramework::ComponentTypeMimeData::Get(data, classDataContainer);

    AZ::ComponentTypeList componentsToAdd;
    for (auto classData : classDataContainer)
    {
        if (classData)
        {
            componentsToAdd.push_back(classData->m_typeId);
        }
    }

    AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome addedComponentsResult = AZ::Failure(AZStd::string("Failed to call AddComponentsToEntities on EntityCompositionRequestBus"));
    AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(addedComponentsResult, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, entityIds, componentsToAdd);

    if (addedComponentsResult.IsSuccess())
    {
        for (auto componentsAddedToEntity : addedComponentsResult.GetValue())
        {
            auto entityId = componentsAddedToEntity.first;
            AZ_Assert(entityId == dropTargetId, "Only asked to add components to one entity, the id returned is not the right one");
            for (auto componentAddedToEntity : componentsAddedToEntity.second.m_componentsAdded)
            {
                AzToolsFramework::EditorMetricsEventsBus::Broadcast(&AzToolsFramework::EditorMetricsEventsBusTraits::ComponentAdded, entityId, AzToolsFramework::GetComponentTypeId(componentAddedToEntity));
            }
        }

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree_NewContent);
    }

    return false;
}

bool OutlinerListModel::dropMimeDataComponentAssets(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    (void)action;
    (void)row;
    (void)column;

    AZ::EntityId dropTargetId = GetEntityFromIndex(parent);
    AzToolsFramework::EntityIdList entityIds{ dropTargetId };

    AzToolsFramework::ComponentAssetMimeDataContainer componentAssetContainer;
    if (componentAssetContainer.FromMimeData(data))    //  This returns false if the mime data doesn't contain the right type.
    {
        // Build up all the components we want to add
        AZ::ComponentTypeList componentsToAdd;
        for (auto asset : componentAssetContainer.m_assets)
        {
            AZ::Uuid componentType = asset.m_classId;

            // add type for asset with known component class
            if (!componentType.IsNull())
            {
                componentsToAdd.push_back(componentType);
            }
        }

        // Add them all at once
        AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome = AZ::Failure(AZStd::string(""));
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(addComponentsOutcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, entityIds, componentsToAdd);

        if (!addComponentsOutcome.IsSuccess())
        {
            return false;
        }

        // Patch up the assets for each component added
        auto& componentsAdded = addComponentsOutcome.GetValue()[dropTargetId].m_componentsAdded;
        size_t componentAddedIndex = 0;
        for (auto asset : componentAssetContainer.m_assets)
        {
            auto componentAdded = componentsAdded[componentAddedIndex];
            auto componentType = asset.m_classId;
            auto assetId = asset.m_assetId;

            AZ_Assert(componentType == AzToolsFramework::GetComponentTypeId(componentAdded), "Type of component added does not match the type required by asset");

            // Add Component Metrics Event (Drag+Drop from File Browser to Entity Outliner)
            EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, ComponentAdded, dropTargetId, componentType);

            auto editorComponent = AzToolsFramework::GetEditorComponent(componentAdded);
            if (editorComponent)
            {
                editorComponent->SetPrimaryAsset(assetId);
            }

            ++componentAddedIndex;
        }

        if (IsSelected(dropTargetId))
        {
            EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree_NewContent);
        }
        return true;
    }

    return false;
}

bool OutlinerListModel::dropMimeDataEntities(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    (void)action;
    (void)column;

    QByteArray arrayData = data->data(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

    AzToolsFramework::EditorEntityIdContainer entityIdListContainer;
    if (!entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
    {
        return false;
    }

    AZ::EntityId newParentId = GetEntityFromIndex(parent);
    AZ::EntityId beforeEntityId = GetEntityFromIndex(index(row, 0, parent));
    AzToolsFramework::EntityIdList topLevelEntityIds;
    topLevelEntityIds.reserve(entityIdListContainer.m_entityIds.size());
    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::FindTopLevelEntityIdsInactive, entityIdListContainer.m_entityIds, topLevelEntityIds);
    if (!ReparentEntities(newParentId, topLevelEntityIds, beforeEntityId))
    {
        return false;
    }

    return true;
}

bool OutlinerListModel::CanReparentEntities(const AZ::EntityId& newParentId, const AzToolsFramework::EntityIdList &selectedEntityIds) const
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    if (selectedEntityIds.empty())
    {
        return false;
    }

    // Ignore entities not owned by the editor context. It is assumed that all entities belong
    // to the same context since multiple selection doesn't span across views.
    for (const AZ::EntityId& entityId : selectedEntityIds)
    {
        bool isEditorEntity = false;
        EBUS_EVENT_RESULT(isEditorEntity, AzToolsFramework::EditorEntityContextRequestBus, IsEditorEntity, entityId);
        if (!isEditorEntity)
        {
            return false;
        }

        bool isEntityEditable = true;
        EBUS_EVENT_RESULT(isEntityEditable, AzToolsFramework::ToolsApplicationRequests::Bus, IsEntityEditable, entityId);
        if (!isEntityEditable)
        {
            return false;
        }
    }

    //Only check the entity pointer if the entity id is valid because
    //we want to allow dragging items to unoccupied parts of the tree to un-parent them
    if (newParentId.IsValid())
    {
        AZ::Entity* newParentEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newParentEntity, &AZ::ComponentApplicationRequests::FindEntity, newParentId);
        if (!newParentEntity)
        {
            return false;
        }

        //move disallowing drop on locked entities check to allow feedback
        bool isLocked = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isLocked, newParentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsLocked);
        if (isLocked)
        {
            return false;
        }
    }

    //reject dragging on to yourself or your children
    AZ::EntityId currParentId = newParentId;
    while (currParentId.IsValid())
    {
        if (AZStd::find(selectedEntityIds.begin(), selectedEntityIds.end(), currParentId) != selectedEntityIds.end())
        {
            return false;
        }

        AZ::EntityId tempParentId;
        AZ::TransformBus::EventResult(tempParentId, currParentId, &AZ::TransformBus::Events::GetParentId);
        currParentId = tempParentId;
    }

    return true;
}

bool OutlinerListModel::ReparentEntities(const AZ::EntityId& newParentId, const AzToolsFramework::EntityIdList &selectedEntityIds, const AZ::EntityId& beforeEntityId)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    if (!CanReparentEntities(newParentId, selectedEntityIds))
    {
        return false;
    }

    AzToolsFramework::ScopedUndoBatch undo("Reparent Entities");
    //capture child entity order before re-parent operation, which will automatically add order info if not present
    AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(newParentId);

    // The new parent is dirty due to sort change(s)
    undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(newParentId));

    {
        AzToolsFramework::ScopedUndoBatch undo("Reparent Entities");

        for (const AZ::EntityId& entityId : selectedEntityIds)
        {
            AZ::EntityId oldParentId;
            EBUS_EVENT_ID_RESULT(oldParentId, entityId, AZ::TransformBus, GetParentId);

            //  Guarding this to prevent the entity from being marked dirty when the parent doesn't change.
            EBUS_EVENT_ID(entityId, AZ::TransformBus, SetParent, newParentId);

            // Allow for metrics collection
            EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, UpdateTransformParentEntity, entityId, newParentId, oldParentId);

            // The old parent is dirty due to sort change
            undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(oldParentId));

            // The reparented entity is dirty due to parent change
            undo.MarkEntityDirty(entityId);
        }
    }

    //search for the insertion entity in the order array
    auto beforeEntityItr = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), beforeEntityId);

    //replace order info matching selection with bad values rather than remove to preserve layout
    for (auto& id : entityOrderArray)
    {
        if (AZStd::find(selectedEntityIds.begin(), selectedEntityIds.end(), id) != selectedEntityIds.end())
        {
            id = AZ::EntityId();
        }
    }

    if (newParentId.IsValid())
    {
        //if adding to a valid parent entity, insert at the found entity location or at the head of the container
        auto insertItr = beforeEntityItr != entityOrderArray.end() ? beforeEntityItr : entityOrderArray.begin();
        entityOrderArray.insert(insertItr, selectedEntityIds.begin(), selectedEntityIds.end());
    }
    else
    {
        //if adding to an invalid parent entity (the root), insert at the found entity location or at the tail of the container
        auto insertItr = beforeEntityItr != entityOrderArray.end() ? beforeEntityItr : entityOrderArray.end();
        entityOrderArray.insert(insertItr, selectedEntityIds.begin(), selectedEntityIds.end());
    }

    //remove placeholder entity ids
    entityOrderArray.erase(AZStd::remove(entityOrderArray.begin(), entityOrderArray.end(), AZ::EntityId()), entityOrderArray.end());

    //update order array
    AzToolsFramework::SetEntityChildOrder(newParentId, entityOrderArray);

    EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
    return true;
}

QMimeData* OutlinerListModel::mimeData(const QModelIndexList& indexes) const
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    AZ::Uuid uuid1 = AZ::AzTypeInfo<AZ::Entity>::Uuid();
    AZ::Uuid uuid2 = AZ::AzTypeInfo<AzToolsFramework::EditorEntityIdContainer>::Uuid();

    AzToolsFramework::EditorEntityIdContainer entityIdList;
    for (const QModelIndex& index : indexes)
    {
        if (index.column() == 0) // ignore all but one cell in row
        {
            AZ::EntityId entityId = GetEntityFromIndex(index);
            if (entityId.IsValid())
            {
                entityIdList.m_entityIds.push_back(entityId);
            }
        }
    }
    if (entityIdList.m_entityIds.empty())
    {
        return nullptr;
    }

    AZStd::vector<char> encoded;
    if (!entityIdList.ToBuffer(encoded))
    {
        return nullptr;
    }

    QMimeData* mimeDataPtr = new QMimeData();
    QByteArray encodedData;
    encodedData.resize((int)encoded.size());
    memcpy(encodedData.data(), encoded.data(), encoded.size());

    mimeDataPtr->setData(AzToolsFramework::EditorEntityIdContainer::GetMimeType(), encodedData);
    return mimeDataPtr;
}

QStringList OutlinerListModel::mimeTypes() const
{
    QStringList list = QAbstractItemModel::mimeTypes();
    list.append(AzToolsFramework::EditorEntityIdContainer::GetMimeType());
    list.append(AzToolsFramework::ComponentTypeMimeData::GetMimeType());
    list.append(AzToolsFramework::ComponentAssetMimeDataContainer::GetMimeType());
    return list;
}

void OutlinerListModel::QueueEntityUpdate(AZ::EntityId entityId)
{
    if (!m_entityChangeQueued)
    {
        m_entityChangeQueued = true;
        QTimer::singleShot(0, this, &OutlinerListModel::ProcessEntityUpdates);
    }
    m_entityChangeQueue.insert(entityId);
}

void OutlinerListModel::QueueAncestorUpdate(AZ::EntityId entityId)
{
    //primarily needed for ancestors that reflect child state (selected, locked, hidden)
    AZ::EntityId parentId;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);
    for (AZ::EntityId currentId = parentId; currentId.IsValid(); currentId = parentId)
    {
        QueueEntityUpdate(currentId);
        parentId.SetInvalid();
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, currentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);
    }
}

void OutlinerListModel::QueueEntityToExpand(AZ::EntityId entityId, bool expand)
{
    m_entityExpansionState[entityId] = expand;
    m_entityExpandQueue.insert(entityId);
    QueueEntityUpdate(entityId);
}


void OutlinerListModel::ProcessEntityUpdates()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    m_entityChangeQueued = false;

    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "OutlinerListModel::ProcessEntityUpdates:ExpandQueue");
        for (auto entityId : m_entityExpandQueue)
        {
            emit ExpandEntity(entityId, IsExpanded(entityId));
        };
        m_entityExpandQueue.clear();
    }

    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "OutlinerListModel::ProcessEntityUpdates:SelectQueue");
        for (auto entityId : m_entitySelectQueue)
        {
            emit SelectEntity(entityId, IsSelected(entityId));
        };
        m_entitySelectQueue.clear();
    }

    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "OutlinerListModel::ProcessEntityUpdates:ChangeQueue");
        for (auto entityId : m_entityChangeQueue)
        {
            auto startIndex = GetIndexFromEntity(entityId, ColumnName);
            auto endIndex = GetIndexFromEntity(entityId, ColumnCount - 1);
            if (startIndex.isValid() && endIndex.isValid())
            {
                //refresh entire row...can optimize to queue update for specific columns and roles later if needed    
                emit dataChanged(startIndex, endIndex);
            }
        };
        m_entityChangeQueue.clear();
    }

    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "OutlinerListModel::ProcessEntityUpdates:LayoutChanged");
        if (m_entityLayoutQueued)
        {
            emit layoutAboutToBeChanged();
            emit layoutChanged();
        }
        m_entityLayoutQueued = false;
    }

    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "OutlinerListModel::ProcessEntityUpdates:InvalidateFilter");
        InvalidateFilter();
    }
}

void OutlinerListModel::OnEntityInfoResetBegin()
{
    emit EnableSelectionUpdates(false);
    beginResetModel();
}

void OutlinerListModel::OnEntityInfoResetEnd()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    endResetModel();
    m_entityChangeQueued = false;
    m_entitySelectQueue.clear();
    m_entityExpandQueue.clear();
    m_entityChangeQueue.clear();
    //m_entityExpansionState.clear();
    //m_entityFilteredState.clear();
    QueueEntityUpdate(AZ::EntityId());
    emit EnableSelectionUpdates(true);
}

void OutlinerListModel::OnEntityInfoUpdatedAddChildBegin(AZ::EntityId parentId, AZ::EntityId childId)
{
    //add/remove operations trigger selection change signals which assert and break undo/redo operations in progress in inspector etc.
    //so disallow selection updates until change is complete
    emit EnableSelectionUpdates(false);
    auto parentIndex = GetIndexFromEntity(parentId);
    auto childIndex = GetIndexFromEntity(childId);
    beginInsertRows(parentIndex, childIndex.row(), childIndex.row());
}

void OutlinerListModel::OnEntityInfoUpdatedAddChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
{
    (void)parentId;
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    endInsertRows();

    //expand ancestors if a new descendant is already selected
    if (IsSelected(childId) || HasSelectedDescendant(childId))
    {
        ExpandAncestors(childId);
    }

    //restore selection and expansion state for previously registered entity ids (for the view/model only)
    RestoreDescendantSelection(childId);
    RestoreDescendantExpansion(childId);

    //must refresh partial lock/visibility of parents
    QueueAncestorUpdate(childId);
    emit EnableSelectionUpdates(true);
}

void OutlinerListModel::OnEntityInfoUpdatedRemoveChildBegin(AZ::EntityId parentId, AZ::EntityId childId)
{
    //add/remove operations trigger selection change signals which assert and break undo/redo operations in progress in inspector etc.
    //so disallow selection updates until change is complete
    emit EnableSelectionUpdates(false);
    auto parentIndex = GetIndexFromEntity(parentId);
    auto childIndex = GetIndexFromEntity(childId);
    beginRemoveRows(parentIndex, childIndex.row(), childIndex.row());
}

void OutlinerListModel::OnEntityInfoUpdatedRemoveChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
{
    (void)childId;
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    endRemoveRows();

    //must refresh partial lock/visibility of parents
    QueueAncestorUpdate(parentId);
    emit EnableSelectionUpdates(true);
}

void OutlinerListModel::OnEntityInfoUpdatedOrderBegin(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index)
{
    (void)parentId;
    (void)childId;
    (void)index;
}

void OutlinerListModel::OnEntityInfoUpdatedOrderEnd(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    (void)index;
    m_entityLayoutQueued = true;
    QueueEntityUpdate(parentId);
    QueueEntityUpdate(childId);
}

void OutlinerListModel::OnEntityInfoUpdatedSelection(AZ::EntityId entityId, bool selected)
{
    //update all ancestors because they will show highlight if ancestor is selected
    QueueAncestorUpdate(entityId);

    //expand ancestors upon new selection
    if (selected)
    {
        ExpandAncestors(entityId);
    }

    //notify observers
    emit SelectEntity(entityId, selected);
}

void OutlinerListModel::OnEntityInfoUpdatedLocked(AZ::EntityId entityId, bool /*locked*/)
{
    //update all ancestors because they will show partial state for descendants
    QueueEntityUpdate(entityId);
    QueueAncestorUpdate(entityId);
}

void OutlinerListModel::OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool /*visible*/)
{
    //update all ancestors because they will show partial state for descendants
    QueueEntityUpdate(entityId);
    QueueAncestorUpdate(entityId);
}

void OutlinerListModel::OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& name)
{
    (void)name;
    QueueEntityUpdate(entityId);
}

QString OutlinerListModel::GetSliceAssetName(const AZ::EntityId& entityId) const
{
    AZStd::string sliceAssetName;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(sliceAssetName, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetSliceAssetName);
    return QString(sliceAssetName.data());
}

QModelIndex OutlinerListModel::GetIndexFromEntity(const AZ::EntityId& entityId, int column) const
{
    if (entityId.IsValid())
    {
        AZ::EntityId parentId;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

        AZStd::size_t row = 0;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(row, parentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildIndex, entityId);
        return createIndex(static_cast<int>(row), column, static_cast<AZ::u64>(entityId));
    }
    return QModelIndex();
}

AZ::EntityId OutlinerListModel::GetEntityFromIndex(const QModelIndex& index) const
{
    return index.isValid() ? AZ::EntityId(static_cast<AZ::u64>(index.internalId())) : AZ::EntityId();
}

void OutlinerListModel::SearchStringChanged(const AZStd::string& filter)
{
    if (filter.size() > 0)
    {
        CacheSelectionIfAppropriate();
    }

    m_filterString = filter;
    InvalidateFilter();

    RestoreSelectionIfAppropriate();
}

void OutlinerListModel::SearchFilterChanged(const AZStd::vector<AZ::Uuid>& componentFilters)
{
    if (componentFilters.size() > 0)
    {
        CacheSelectionIfAppropriate();
    }

    m_componentFilters = AZStd::move(componentFilters);
    InvalidateFilter();

    RestoreSelectionIfAppropriate();
}

bool OutlinerListModel::ShouldOverrideUnfilteredSelection()
{
    AzToolsFramework::EntityIdList currentSelection;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(currentSelection, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    // If new selection is greater, then we need to override
    if (currentSelection.size() > m_unfilteredSelectionEntityIds.size())
    {
        return true;
    }

    for (auto& entityId : currentSelection)
    {
        // If one element in the current selection doesn't exist in our unfiltered selection, we need to override with the new selection
        auto it = AZStd::find(m_unfilteredSelectionEntityIds.begin(), m_unfilteredSelectionEntityIds.end(), entityId);
        if (it == m_unfilteredSelectionEntityIds.end())
        {
            return true;
        }
    }

    if (currentSelection.empty())
    {
        for (auto& entityId : m_unfilteredSelectionEntityIds)
        {
            // If any of the entities are visible, then the user must have forcibly cleared selection
            if (!IsFiltered(entityId))
            {
                return true;
            }
        }
    }

    return false;
}

void OutlinerListModel::CacheSelectionIfAppropriate()
{
    if (ShouldOverrideUnfilteredSelection())
    {
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(m_unfilteredSelectionEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
    }
}

void OutlinerListModel::RestoreSelectionIfAppropriate()
{
    if (m_unfilteredSelectionEntityIds.size() > 0)
    {
        // Store these in a temp list so it doesn't get cleared mid-loop by external logic
        AzToolsFramework::EntityIdList tempList = m_unfilteredSelectionEntityIds;

        for (auto& entityId : tempList)
        {
            if (!IsFiltered(entityId))
            {
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::MarkEntitySelected, entityId);
            }
        }

        m_unfilteredSelectionEntityIds = tempList;
    }

    if (m_unfilteredSelectionEntityIds.size() > 0 && m_componentFilters.size() == 0 && m_filterString.size() == 0)
    {
        m_unfilteredSelectionEntityIds.clear();
    }
}

void OutlinerListModel::AfterEntitySelectionChanged()
{
    if (m_unfilteredSelectionEntityIds.size() > 0)
    {
        if (ShouldOverrideUnfilteredSelection())
        {
            m_unfilteredSelectionEntityIds.clear();
        }
    }
}

void OutlinerListModel::OnEntityExpanded(const AZ::EntityId& entityId)
{
    m_entityExpansionState[entityId] = true;
    QueueEntityUpdate(entityId);
}

void OutlinerListModel::OnEntityCollapsed(const AZ::EntityId& entityId)
{
    m_entityExpansionState[entityId] = false;
    QueueEntityUpdate(entityId);
}

void OutlinerListModel::InvalidateFilter()
{
    FilterEntity(AZ::EntityId());

    // Emit data changed directly as it is immediately valid
    auto modelIndex = GetIndexFromEntity(AZ::EntityId());
    if (modelIndex.isValid())
    {
        emit dataChanged(modelIndex, modelIndex, { VisibilityRole });
    }
}

void OutlinerListModel::OnEditorEntitiesReplacedBySlicedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& replacedEntitiesMap)
{
    //the original entity was destroyed by now but the replacement may need to be refreshed
    for (const auto& replacedPair : replacedEntitiesMap)
    {
        auto expansionIter = m_entityExpansionState.find(replacedPair.first);
        QueueEntityToExpand(replacedPair.second, expansionIter != m_entityExpansionState.end() && expansionIter->second);
    }
}


//! Editor lock component interface to enable/disable selection capabilities in the viewport.
void OutlinerListModel::ToggleEditorLockState(const AZ::EntityId& entityId)
{
    if (entityId.IsValid())
    {
        bool isLocked = false;
        AzToolsFramework::EditorLockComponentRequestBus::EventResult(isLocked, entityId, &AzToolsFramework::EditorLockComponentRequests::GetLocked);

        if (IsSelected(entityId))
        {
            AzToolsFramework::EntityIdList selectedEntityIds;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

            for (auto selectedId : selectedEntityIds)
            {
                SetEditorLockState(selectedId, !isLocked);
            }
        }
        else
        {
            SetEditorLockState(entityId, !isLocked);
        }
    }
}

void OutlinerListModel::SetEditorLockState(const AZ::EntityId& entityId, bool isLocked)
{
    if (entityId.IsValid())
    {
        AzToolsFramework::EditorLockComponentRequestBus::Event(entityId, &AzToolsFramework::EditorLockComponentRequests::SetLocked, isLocked);

        AzToolsFramework::EntityIdList children;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);

        for (auto childId : children)
        {
            SetEditorLockState(childId, isLocked);
        }
    }
}

//! Editor Visibility interface to enable/disable rendering in the viewport.
void OutlinerListModel::ToggleEditorVisibility(const AZ::EntityId& entityId)
{
    if (entityId.IsValid())
    {
        bool isVisible = false;
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(isVisible, entityId, &AzToolsFramework::EditorVisibilityRequests::GetVisibilityFlag);

        if (IsSelected(entityId))
        {
            AzToolsFramework::EntityIdList selectedEntityIds;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

            for (AZ::EntityId selectedId : selectedEntityIds)
            {
                SetEditorVisibility(selectedId, !isVisible);
            }
        }
        else
        {
            SetEditorVisibility(entityId, !isVisible);
        }
    }
}

void OutlinerListModel::SetEditorVisibility(const AZ::EntityId& entityId, bool isVisible)
{
    if (entityId.IsValid())
    {
        AzToolsFramework::EditorVisibilityRequestBus::Event(entityId, &AzToolsFramework::EditorVisibilityRequests::SetVisibilityFlag, isVisible);

        AzToolsFramework::EntityIdList children;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
        for (auto childId : children)
        {
            SetEditorVisibility(childId, isVisible);
        }
    }
}

void OutlinerListModel::ExpandAncestors(const AZ::EntityId& entityId)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    //typically to reveal selected entities, expand all parent entities
    if (entityId.IsValid())
    {
        AZ::EntityId parentId;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);
        QueueEntityToExpand(parentId, true);
        ExpandAncestors(parentId);
    }
}

bool OutlinerListModel::IsExpanded(const AZ::EntityId& entityId) const
{
    auto expandedItr = m_entityExpansionState.find(entityId);
    return expandedItr != m_entityExpansionState.end() && expandedItr->second;
}

void OutlinerListModel::RestoreDescendantExpansion(const AZ::EntityId& entityId)
{
    //re-expand/collapse entities in the model that may have been previously removed or rearranged, resulting in new model indices
    if (entityId.IsValid())
    {
        QueueEntityToExpand(entityId, IsExpanded(entityId));

        AzToolsFramework::EntityIdList children;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
        for (auto childId : children)
        {
            RestoreDescendantExpansion(childId);
        }
    }
}

void OutlinerListModel::RestoreDescendantSelection(const AZ::EntityId& entityId)
{
    //re-select entities in the model that may have been previously removed or rearranged, resulting in new model indices
    if (entityId.IsValid())
    {
        m_entitySelectQueue.insert(entityId);
        QueueEntityUpdate(entityId);

        AzToolsFramework::EntityIdList children;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
        for (auto childId : children)
        {
            RestoreDescendantSelection(childId);
        }
    }
}

bool OutlinerListModel::FilterEntity(const AZ::EntityId& entityId)
{
    bool isFilterMatch = true;

    if (m_filterString.size() > 0)
    {
        AZStd::string name;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(name, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetName);

        if (AzFramework::StringFunc::Find(name.c_str(), m_filterString.c_str()) == AZStd::string::npos)
        {
            isFilterMatch = false;
        }
    }

    // If we matched the filter string and have any component filters, make sure we match those too
    if (isFilterMatch && m_componentFilters.size() > 0)
    {
        bool hasMatchingComponent = false;

        for (AZ::Uuid& componentType : m_componentFilters)
        {
            if (AzToolsFramework::EntityHasComponentOfType(entityId, componentType))
            {
                hasMatchingComponent = true;
                break;
            }
        }

        isFilterMatch &= hasMatchingComponent;
    }

    AzToolsFramework::EntityIdList children;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
    for (auto childId : children)
    {
        isFilterMatch |= FilterEntity(childId);
    }

    // If we now match the filter and we previously didn't and we're set in an expanded state
    // we need to queue an expand again so that the treeview state matches our internal saved state
    if (isFilterMatch && m_entityFilteredState[entityId] && IsExpanded(entityId))
    {
        QueueEntityToExpand(entityId, true);
    }

    m_entityFilteredState[entityId] = !isFilterMatch;

    return isFilterMatch;
}

bool OutlinerListModel::IsFiltered(const AZ::EntityId& entityId) const
{
    auto hiddenItr = m_entityFilteredState.find(entityId);
    return hiddenItr != m_entityFilteredState.end() && hiddenItr->second;
}

bool OutlinerListModel::HasSelectedDescendant(const AZ::EntityId& entityId) const
{
    //TODO result can be cached in mutable map and cleared when any descendant changes to avoid recursion in deep hierarchies
    AzToolsFramework::EntityIdList children;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
    for (auto childId : children)
    {
        bool isSelected = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSelected, childId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSelected);
        if (isSelected || HasSelectedDescendant(childId))
        {
            return true;
        }
    }
    return false;
}

bool OutlinerListModel::AreAllDescendantsSameLockState(const AZ::EntityId& entityId) const
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    //TODO result can be cached in mutable map and cleared when any descendant changes to avoid recursion in deep hierarchies
    bool isLocked = false;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isLocked, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsLocked);

    AzToolsFramework::EntityIdList children;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
    for (auto childId : children)
    {
        bool isLockedChild = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isLockedChild, childId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsLocked);
        if (isLocked != isLockedChild || !AreAllDescendantsSameLockState(childId))
        {
            return false;
        }
    }
    return true;
}

bool OutlinerListModel::AreAllDescendantsSameVisibleState(const AZ::EntityId& entityId) const
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    //TODO result can be cached in mutable map and cleared when any descendant changes to avoid recursion in deep hierarchies
    bool isVisible = false;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isVisible, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

    AzToolsFramework::EntityIdList children;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
    for (auto childId : children)
    {
        bool isVisibleChild = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isVisibleChild, childId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);
        if (isVisible != isVisibleChild || !AreAllDescendantsSameLockState(childId))
        {
            return false;
        }
    }
    return true;
}

void OutlinerListModel::OnEntityInitialized(const AZ::EntityId& /*entityId*/)
{
    if (m_filterString.size() > 0 || m_componentFilters.size() > 0)
    {
        emit ReapplyFilter();
    }
}

void OutlinerListModel::OnEntityCompositionChanged(const AzToolsFramework::EntityIdList& /*entityIds*/)
{
    if (m_componentFilters.size() > 0)
    {
        emit ReapplyFilter();
    }
}

void OutlinerListModel::OnContextReset()
{
    if (m_filterString.size() > 0 || m_componentFilters.size() > 0)
    {
        emit ResetFilter();
    }
}


////////////////////////////////////////////////////////////////////////////
// OutlinerItemDelegate
////////////////////////////////////////////////////////////////////////////

OutlinerItemDelegate::OutlinerItemDelegate(QWidget* parent)
    : QStyledItemDelegate(parent)
{
    m_visibilityCheckBoxWithBorder.setObjectName("bordered");
    m_lockCheckBoxWithBorder.setObjectName("bordered");
}

void OutlinerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // We're only using these check boxes as renderers so their actual state doesn't matter.
    // We can set it right before we draw using information from the model data.
    if (index.column() == OutlinerListModel::ColumnVisibilityToggle)
    {
        painter->save();
        painter->translate(option.rect.topLeft());

        bool checked = index.data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked;
        m_visibilityCheckBoxWithBorder.setChecked(checked);
        m_visibilityCheckBox.setChecked(checked);

        if (index.data(OutlinerListModel::PartiallyVisibleRole).value<bool>())
        {
            m_visibilityCheckBoxWithBorder.render(painter);
        }
        else
        {
            m_visibilityCheckBox.render(painter);
        }

        painter->restore();
        return;
    }

    if (index.column() == OutlinerListModel::ColumnLockToggle)
    {
        painter->save();
        painter->translate(option.rect.topLeft());

        bool checked = index.data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked;
        m_lockCheckBoxWithBorder.setChecked(checked);
        m_lockCheckBox.setChecked(checked);

        if (index.data(OutlinerListModel::PartiallyLockedRole).value<bool>())
        {
            m_lockCheckBoxWithBorder.render(painter);
        }
        else
        {
            m_lockCheckBox.render(painter);
        }

        painter->restore();
        return;
    }

    QStyleOptionViewItem customOptions{ option };
    if (customOptions.state & QStyle::State_HasFocus)
    {
        //  Do not draw the focus rectangle in this column.
        customOptions.state ^= QStyle::State_HasFocus;
    }

    // Draw this Slice Handle Accent if the item is not selected before the
    // entry is drawn.
    if (!(option.state & QStyle::State_Selected))
    {
        if (index.data(OutlinerListModel::SliceBackgroundRole).value<bool>())
        {
            auto boxRect = option.rect;

            boxRect.setX(boxRect.x() + 0.5);
            boxRect.setY(boxRect.y() + 3.5);
            boxRect.setWidth(boxRect.width() - 1.0);
            boxRect.setHeight(boxRect.height() - 4.0);

            // These colors need to be moved into a style sheet in the future.
            // The new editor design will likely include style sheet colors for
            // slices and relevant selection of such.
            QColor background(30, 37, 47);
            QColor line(30, 50, 80);

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(boxRect, 6, 6);
            QPen pen(line, 1);
            painter->setPen(pen);
            painter->fillPath(path, background);
            painter->drawPath(path);
            painter->restore();
        }

        // Draw a dashed line around any visible, collapsed entry in the outliner that has
        // children underneath it currently selected.
        if (!index.data(OutlinerListModel::ExpandedRole).template value<bool>())
        {
            if (index.data(OutlinerListModel::ChildSelectedRole).template value<bool>())
            {
                auto boxRect = option.rect;

                boxRect.setX(boxRect.x() + 0.5);
                boxRect.setY(boxRect.y() + 3.5);
                boxRect.setWidth(boxRect.width() - 1.0);
                boxRect.setHeight(boxRect.height() - 4.0);

                QPainterPath path;
                path.addRoundedRect(boxRect, 6, 6);

                // Get the foreground color of the current object to draw our sub-object-selected box
                auto targetColorBrush = index.data(Qt::ForegroundRole).value<QBrush>();
                QPen pen(targetColorBrush.color(), 2);

                // Alter the dash pattern available for better visual appeal
                QVector<qreal> dashes;
                dashes << 3 << 5;
                pen.setStyle(Qt::PenStyle::DashLine);
                pen.setDashPattern(dashes);

                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(pen);
                painter->drawPath(path);
                painter->restore();
            }
        }
    }

    QStyledItemDelegate::paint(painter, customOptions, index);

    // Draw this Slice Handle Accent if the item is selected after the entry is drawn.
    if (option.state & QStyle::State_Selected)
    {
        if (index.data(OutlinerListModel::SliceBackgroundRole).value<bool>())
        {
            auto boxRect = option.rect;

            boxRect.setX(boxRect.x() + 0.5);
            boxRect.setY(boxRect.y() + 3.5);
            boxRect.setWidth(boxRect.width() - 1.0);
            boxRect.setHeight(boxRect.height() - 4.0);

            // These colors need to be moved into a style sheet in the future.
            // The new editor design will likely include style sheet colors for
            // slices and relevant selection of such.
            // ----------------------------------------------------------------------
            // The accent is drawn a very transparent blue color. to allow the entry text and icon to show through while
            // providing a subtle accent that is distinctly different from the standard slice handle accent.
            QColor background(0, 0, 255, 50);
            // The accent outline is a dark blue to keep the contrast down.
            QColor line(0, 0, 120);

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(boxRect, 6, 6);
            QPen pen(line, 1);
            painter->setPen(pen);
            painter->fillPath(path, background);
            painter->drawPath(path);
            painter->restore();
        }
    }
}

QSize OutlinerItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const
{
    // Get the height of a tall character...
    auto textRect = option.fontMetrics.boundingRect("|");
    // And add 8 to it gives the outliner roughly the visible spacing we're looking for.
    return QSize(0, textRect.height() + OutlinerListModel::s_OutlinerSpacing);
}

#include <UI/Outliner/OutlinerListModel.moc>
