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
#include <QFontMetrics>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QStyle>
#include <QtWidgets/QMessageBox>
#include <QGuiApplication>
#include <QTimer>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Slice/SliceAsset.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/ToolsComponents/EditorLayerComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/SelectionComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

#include "OutlinerDisplayOptionsMenu.h"
#include "OutlinerSortFilterProxyModel.hxx"
#include "OutlinerTreeView.hxx"

#include <Editor/CryEditDoc.h>
#include <AzCore/Outcome/Outcome.h>

////////////////////////////////////////////////////////////////////////////
// OutlinerListModel
////////////////////////////////////////////////////////////////////////////

bool OutlinerListModel::s_paintingName = false;

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
        bool isSubsliceRoot = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSubsliceRoot, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSubsliceRoot);
        return (isSliceRoot || isSubsliceRoot) && isSliceEntity;
    }

    case SliceEntityOverrideRole:
    {
        bool entityHasOverrides = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(entityHasOverrides, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::HasSliceAnyOverrides);
        return entityHasOverrides;
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

    case InLockedLayerRole:
        return IsInLayerWithProperty(id, LayerProperty::Locked);

    case InInvisibleLayerRole:
        return IsInLayerWithProperty(id, LayerProperty::Invisible);

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
        QString label{ name.data() };
        if (s_paintingName && !m_filterString.empty())
        {
            // highlight characters in filter
            int highlightTextIndex = 0;
            do
            {
                highlightTextIndex = label.lastIndexOf(QString(m_filterString.c_str()), highlightTextIndex - 1, Qt::CaseInsensitive);
                if (highlightTextIndex >= 0)
                {
                    const QString BACKGROUND_COLOR{ "#707070" };
                    label.insert(highlightTextIndex + m_filterString.length(), "</span>");
                    label.insert(highlightTextIndex, "<span style=\"background-color: " + BACKGROUND_COLOR + "\">");
                }
            } while(highlightTextIndex > 0);
        }
        return label;
    }

    case Qt::ToolTipRole:
    {
        return GetEntityTooltip(id);
    }

    case EntityTypeRole:
    {
        bool isSliceRoot = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceRoot, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceRoot);
        bool isSliceEntity = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceEntity, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceEntity);
        bool isSubsliceRoot = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSubsliceRoot, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSubsliceRoot);

        bool isLayerEntity = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isLayerEntity,
            id,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

        if (isLayerEntity)
        {
            return LayerType;
        }
        else if (isSliceEntity)
        {
            return (isSliceRoot | isSubsliceRoot) ? SliceHandleType : SliceEntityType;
        }

        // Guaranteed to return a valid type.
        return EntityType;
    }

    case LayerColorRole:
    {
        QColor layerColor;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            layerColor,
            id,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerColor);
        return layerColor;
    }

    case Qt::ForegroundRole:
    {
        // We use the parent's palette because the GUI Application palette is returning the wrong colors
        auto parentWidgetPtr = static_cast<QWidget*>(QObject::parent());
        return QBrush(parentWidgetPtr->palette().color(QPalette::Text));
    }

    case Qt::DecorationRole:
    {
        return GetEntityIcon(id);
    }
    }

    return dataForAll(index, role);
}

QVariant OutlinerListModel::GetEntityIcon(const AZ::EntityId& id) const
{
    AZ::Entity* entity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);

    // build the icon name based on the entity type and state
    QString iconName;

    bool isSliceEntity = false;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceEntity, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceEntity);

    bool isLayerEntity = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerEntity,
        id,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);


    if (isLayerEntity)
    {
        // There is only one layer icon, it does not have multiple states like entities.
        return QIcon(QPixmap(QString(":/Icons/layer_icon.svg")));
    }
    else if (isSliceEntity)
    {
        iconName = "Slice_Entity";

        bool hasSliceEntityOverrides = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(hasSliceEntityOverrides, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::HasSliceEntityOverrides);

        if (hasSliceEntityOverrides)
        {
            iconName += "_Modified";
        }
    }
    else
    {
        iconName = "Entity";
    }

    bool isEditorOnly = false;
    AzToolsFramework::EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, id, &AzToolsFramework::EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);

    const bool isInitiallyActive = entity ? entity->IsRuntimeActiveByDefault() : true;

    if (isEditorOnly)
    {
        iconName += "_Editor_Only";
    }
    else if (!isInitiallyActive)
    {
        iconName += "_Not_Active";
    }

    QPixmap entityIcon = QPixmap(QString(":/Icons/%1.svg").arg(iconName));

    // Draw a circle at the bottom right corner of the icon when the entity's children have overrides.
    bool hasSliceChildrenOverrides = false;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(hasSliceChildrenOverrides, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::HasSliceChildrenOverrides);

    if (hasSliceChildrenOverrides)
    {
        QBitmap mask = entityIcon.mask();
        QPainter maskPainter(&mask);
        maskPainter.drawEllipse(entityIcon.width() - maskDiameter, entityIcon.height() - maskDiameter, maskDiameter, maskDiameter);
        entityIcon.setMask(mask);

        QPainter iconPainter(&entityIcon);
        iconPainter.setPen(Qt::transparent);
        iconPainter.setBrush(QBrush(circleIconColor));
        iconPainter.drawEllipse(entityIcon.width() - circleIconDiameter, entityIcon.height() - circleIconDiameter, circleIconDiameter, circleIconDiameter);
    }

    return QIcon(entityIcon);
}

QVariant OutlinerListModel::GetEntityTooltip(const AZ::EntityId& id) const
{
    bool isLayerEntity = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerEntity,
        id,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

    if (isLayerEntity)
    {
        bool isLayerNameValid = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isLayerNameValid,
            id,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::IsLayerNameValid);
        if (!isLayerNameValid)
        {
            return "Layer names at the same level of the hierarchy must be unique before you can save them.";
        }

        AZ::Outcome<AZStd::string, AZStd::string> layerFullNameResult(
            AZ::Failure(AZStd::string("Unable to get the layer's file name with an extension.")));
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            layerFullNameResult,
            id,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerFullFileName);
        if (!layerFullNameResult.IsSuccess())
        {
            return layerFullNameResult.GetError().c_str();
        }

        QString layerTooltip = layerFullNameResult.GetValue().c_str();

        bool hasUnsavedChanges = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            hasUnsavedChanges,
            id,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasUnsavedChanges);
        if (hasUnsavedChanges)
        {
            layerTooltip = QObject::tr("%1(Unsaved)").arg(layerTooltip);
        }

        QString levelAbsoluteFolder = Path::GetPath(GetIEditor()->GetDocument()->GetActivePathName());
        bool doesLayerFileExistOnDisk = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            doesLayerFileExistOnDisk,
            id,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::DoesLayerFileExistOnDisk,
            levelAbsoluteFolder);
        if (doesLayerFileExistOnDisk)
        {
            layerTooltip = QObject::tr("Unsaved layer");
        }

        return layerTooltip;
    }


    AZStd::string sliceAssetName;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(sliceAssetName, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetSliceAssetName);

    bool isEditorOnly = false;
    AzToolsFramework::EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, id, &AzToolsFramework::EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);

    QString tooltip = !sliceAssetName.empty() ? QString("Slice asset: %1").arg(sliceAssetName.data()) : QString("Slice asset: This entity is not part of a slice.");


    bool hasCyclicDependency = false;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(hasCyclicDependency, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::HasCyclicDependency);
    if (hasCyclicDependency)
    {
        tooltip = QObject::tr("Circular dependency detected. Cannot save instances of a slice inside of itself. Other valid overrides can be saved as normal.");
    }
    else
    if (isEditorOnly)
    {
        tooltip = QString("(%1) %2").arg(tr("Editor-only"), tooltip);
    }
    else
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
        if (!entity->IsRuntimeActiveByDefault())
        {
            tooltip = QString("(%1) %2").arg(tr("Not Active on Start"), tooltip);
        }
    }

    return tooltip;
}

QVariant OutlinerListModel::dataForVisibility(const QModelIndex& index, int role) const
{
    auto id = GetEntityFromIndex(index);

    switch (role)
    {
    case Qt::CheckStateRole:
    {
        return IsEntityVisible(id) ? Qt::Checked : Qt::Unchecked;
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
        // Lock state is tracked in 3 places:
        // EditorLockComponent, EditorEntityModel, and ComponentEntityObject.
        // In addition to that, entities that are in layers can have the layer's lock state override their own.
        // Retrieving the lock state from the lock component is ideal for drawing the lock icon in the outliner because
        // the outliner needs to show that specific entity's lock state, and not the actual final lock state including the layer behavior.
        // The EditorLockComponent only knows about the specific entity's lock state and not the hierarchy.
        AzToolsFramework::EditorLockComponentRequestBus::EventResult(isLocked, id, &AzToolsFramework::EditorLockComponentRequests::GetLocked);
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
        if (canDropMimeDataForEntityIds(data, action, row, column, parent))
        {
            return true;
        }

        if (CanDropMimeDataAssets(data, action, row, column, parent))
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
    }

    if (data->hasFormat(AzToolsFramework::AssetBrowser::AssetBrowserEntry::GetMimeType()))
    {
        return DropMimeDataAssets(data, action, row, column, parent);
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
    for (const auto& classData : classDataContainer)
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
        for (const auto& componentsAddedToEntity : addedComponentsResult.GetValue())
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


void OutlinerListModel::DecodeAssetMimeData(const QMimeData* data, AZStd::vector<ComponentAssetPair>& componentAssetPairs, SliceAssetList& sliceAssets) const
{
    using namespace AzToolsFramework;

    AZStd::vector<AssetBrowser::AssetBrowserEntry*> entries;
    AssetBrowser::AssetBrowserEntry::FromMimeData(data, entries);

    AZStd::vector<const AssetBrowser::ProductAssetBrowserEntry*> products;
    products.reserve(entries.size());

    // Look at all products and determine if they have an associated component.
    // If so, store the componentType->assetId pair.
    for (AssetBrowser::AssetBrowserEntry* entry : entries)
    {
        products.clear();
        const AssetBrowser::ProductAssetBrowserEntry* browserEntry = azrtti_cast<const AssetBrowser::ProductAssetBrowserEntry*>(entry);
        if (browserEntry)
        {
            products.push_back(browserEntry);
        }
        else
        {
            entry->GetChildren<AssetBrowser::ProductAssetBrowserEntry>(products);
        }
        for (const auto* product : products)
        {
            if (product->GetAssetType() == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
            {
                sliceAssets.push_back(product->GetAssetId());
            }
            else
            {
                bool canCreateComponent = false;
                AZ::AssetTypeInfoBus::EventResult(canCreateComponent, product->GetAssetType(), &AZ::AssetTypeInfo::CanCreateComponent, product->GetAssetId());

                AZ::TypeId componentType;
                AZ::AssetTypeInfoBus::EventResult(componentType, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);

                if (canCreateComponent && !componentType.IsNull())
                {
                    componentAssetPairs.push_back(AZStd::make_pair(componentType, product->GetAssetId()));
                }
            }
        }
    }
}

bool OutlinerListModel::CanDropMimeDataAssets(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    (void)action;
    (void)column;
    (void)row;
    (void)parent;

    using namespace AzToolsFramework;

    if (data->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
    {
        ComponentAssetPairs componentAssetPairs;
        SliceAssetList sliceAssets;
        DecodeAssetMimeData(data, componentAssetPairs, sliceAssets);

        return (!componentAssetPairs.empty() || !sliceAssets.empty());
    }

    return false;
}

bool OutlinerListModel::DropMimeDataAssets(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    using namespace AzToolsFramework;

    ComponentAssetPairs componentAssetPairs;
    SliceAssetList sliceAssets;
    DecodeAssetMimeData(data, componentAssetPairs, sliceAssets);

    ScopedUndoBatch undo("Create/Modify Entities for Asset Drop");

    AZ::Vector3 viewportCenter = AZ::Vector3::CreateZero();
    EditorRequestBus::BroadcastResult(viewportCenter, &EditorRequestBus::Events::GetWorldPositionAtViewportCenter);

    for (const AZ::Data::AssetId& sliceAssetId : sliceAssets)
    {
        // Queue a slice instantiation for each dragged slice.
        AZ::Data::Asset<AZ::SliceAsset> sliceAsset =
            AZ::Data::AssetManager::Instance().GetAsset<AZ::SliceAsset>(sliceAssetId, false);
        if (sliceAsset)
        {
            EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(EditorMetricsEventsBusTraits::NavigationTrigger::DragAndDrop);
            AZStd::string idString;
            sliceAsset.GetId().ToString(idString);
            EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBusTraits::SliceInstantiated, AZ::Crc32(idString.c_str()));

            EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequestBus::Events::InstantiateEditorSlice, sliceAsset, AZ::Transform::CreateTranslation(viewportCenter));
        }
    }

    AZ::EntityId assignParentId = GetEntityFromIndex(parent);
    ToolsApplicationNotificationBus::Broadcast(&ToolsApplicationNotificationBus::Events::SetEntityInstantiationPosition, assignParentId, GetEntityFromIndex(index(row, 0, parent)));

    if (componentAssetPairs.empty())
    {
        return false;
    }

    QWidget* mainWindow = nullptr;
    EditorRequests::Bus::BroadcastResult(mainWindow, &EditorRequests::GetMainWindow);

    // Only resolve an existing if the drop was directly on it. Otherwise we'll create a new entity.
    AZ::EntityId targetEntityId = (row < 0) ? GetEntityFromIndex(parent) : AZ::EntityId();
    bool createdNewEntity = false;

    // Shift modifier enables creating a child entity from the asset.
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        assignParentId = targetEntityId;
        targetEntityId.SetInvalid();
    }

    if (!targetEntityId.IsValid())
    {
        EditorRequests::Bus::BroadcastResult(targetEntityId, &EditorRequests::CreateNewEntity, assignParentId);
        if (!targetEntityId.IsValid())
        {
            QMessageBox::warning(mainWindow, tr("Asset Drop Failed"), tr("A new entity could not be created for the specified asset."));
            return false;
        }

        createdNewEntity = true;
    }

    AZ::Entity* targetEntity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(targetEntity, &AZ::ComponentApplicationBus::Events::FindEntity, targetEntityId);
    if (!targetEntity)
    {
        QMessageBox::warning(mainWindow, tr("Asset Drop Failed"), tr("Failed to locate target entity."));
        return false;
    }

    // Batch-add all the components.
    AZ::ComponentTypeList componentsToAdd;
    componentsToAdd.reserve(componentAssetPairs.size());
    for (const ComponentAssetPair& pair : componentAssetPairs)
    {
        const AZ::TypeId& componentType = pair.first;
        const AZ::Data::AssetId& assetId = pair.second;

        componentsToAdd.push_back(componentType);
    }

    AZStd::vector<AZ::EntityId> entityIds = { targetEntityId };
    EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome = AZ::Failure(AZStd::string());
    EntityCompositionRequestBus::BroadcastResult(addComponentsOutcome, &EntityCompositionRequests::AddComponentsToEntities, entityIds, componentsToAdd);

    if (!addComponentsOutcome.IsSuccess())
    {
        QMessageBox::warning(mainWindow, tr("Asset Drop Failed"), 
            QStringLiteral("Components could not be added to the target entity \"%1\".\n\nDetails:\n%2.")
            .arg(targetEntity->GetName().c_str())
            .arg(addComponentsOutcome.GetError().c_str()));

        if (createdNewEntity)
        {
            EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::DestroyEditorEntity, targetEntityId);
        }

        return false;
    }

    // Assign asset associated with each created component.
    const AZ::Entity::ComponentArrayType& componentsAdded = addComponentsOutcome.GetValue()[targetEntityId].m_componentsAdded;
    for (const ComponentAssetPair& pair : componentAssetPairs)
    {
        const AZ::TypeId& componentType = pair.first;
        const AZ::Data::AssetId& assetId = pair.second;

        // Name the entity after the first asset.
        if (createdNewEntity && &pair == &componentAssetPairs.front())
        {
            AZStd::string assetPath;
            EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, assetId);
            if (!assetPath.empty())
            {
                AZStd::string entityName;
                AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), entityName);
                targetEntity->SetName(entityName);
            }
        }

        AZ::Component* componentAdded = targetEntity->FindComponent(componentType);
        if (componentAdded)
        {
            // Add Component Metrics Event (Drag+Drop from Asset Browser to Entity Outliner)
            EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBus::Events::ComponentAdded, targetEntityId, componentType);

            Components::EditorComponentBase* editorComponent = GetEditorComponent(componentAdded);
            if (editorComponent)
            {
                editorComponent->SetPrimaryAsset(assetId);
            }
        }
    }

    if (createdNewEntity)
    {
        const AzToolsFramework::EntityIdList selection = { targetEntityId };
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selection);
    }

    if (IsSelected(targetEntityId))
    {
        ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::InvalidatePropertyDisplay, Refresh_EntireTree_NewContent);
    }

    return true;
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

        // when in a forced sort mode, reject reordering under the same parent
        if (m_sortMode != EntityOutliner::DisplaySortMode::Manually)
        {
            AZ::EntityId currentParentId;
            AZ::TransformBus::EventResult(currentParentId, entityId, &AZ::TransformBus::Events::GetParentId);
            if (currentParentId == newParentId)
            {
                return false;
            }
        }

        if (newParentId.IsValid())
        {
            bool isLayerEntity = false;
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                isLayerEntity,
                entityId,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
            // Layers can only have other layers as parents, or have no parent.
            if (isLayerEntity)
            {
                bool newParentIsLayer = false;
                AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                    newParentIsLayer,
                    newParentId,
                    &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
                if (!newParentIsLayer)
                {
                    return false;
                }
            }
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

    m_isFilterDirty = true;

    AzToolsFramework::ScopedUndoBatch undo("Reparent Entities");
    //capture child entity order before re-parent operation, which will automatically add order info if not present
    AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(newParentId);

    // The new parent is dirty due to sort change(s)
    undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(newParentId));

    AzToolsFramework::EntityIdList processedEntityIds;
    {
        AzToolsFramework::ScopedUndoBatch undo("Reparent Entities");

        for (AZ::EntityId entityId : selectedEntityIds)
        {
            AZ::EntityId oldParentId;
            EBUS_EVENT_ID_RESULT(oldParentId, entityId, AZ::TransformBus, GetParentId);

            if (oldParentId != newParentId
                && AzToolsFramework::SliceUtilities::IsReparentNonTrivial(entityId, newParentId))
            {
                entityId = AzToolsFramework::SliceUtilities::ReparentNonTrivialEntityHierarchy(entityId, newParentId);
            }

            //  Guarding this to prevent the entity from being marked dirty when the parent doesn't change.
            EBUS_EVENT_ID(entityId, AZ::TransformBus, SetParent, newParentId);

            // Allow for metrics collection
            EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, UpdateTransformParentEntity, entityId, newParentId, oldParentId);

            // The old parent is dirty due to sort change
            undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(oldParentId));

            // The reparented entity is dirty due to parent change
            undo.MarkEntityDirty(entityId);

            processedEntityIds.push_back(entityId);
        }
    }

    //search for the insertion entity in the order array
    auto beforeEntityItr = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), beforeEntityId);

    //replace order info matching selection with bad values rather than remove to preserve layout
    for (auto& id : entityOrderArray)
    {
        if (AZStd::find(processedEntityIds.begin(), processedEntityIds.end(), id) != processedEntityIds.end())
        {
            id = AZ::EntityId();
        }
    }

    if (newParentId.IsValid())
    {
        //if adding to a valid parent entity, insert at the found entity location or at the head of the container
        auto insertItr = beforeEntityItr != entityOrderArray.end() ? beforeEntityItr : entityOrderArray.begin();
        entityOrderArray.insert(insertItr, processedEntityIds.begin(), processedEntityIds.end());
    }
    else
    {
        //if adding to an invalid parent entity (the root), insert at the found entity location or at the tail of the container
        auto insertItr = beforeEntityItr != entityOrderArray.end() ? beforeEntityItr : entityOrderArray.end();
        entityOrderArray.insert(insertItr, processedEntityIds.begin(), processedEntityIds.end());
    }

    //remove placeholder entity ids
    entityOrderArray.erase(AZStd::remove(entityOrderArray.begin(), entityOrderArray.end(), AZ::EntityId()), entityOrderArray.end());

    //update order array
    AzToolsFramework::SetEntityChildOrder(newParentId, entityOrderArray);

    // update the selection
    {
        // first select the full hierarchy to expand the clones
        AzToolsFramework::EntityIdSet entityHierarchiesSet;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(entityHierarchiesSet, &AzToolsFramework::ToolsApplicationRequestBus::Events::GatherEntitiesAndAllDescendents, processedEntityIds);

        AzToolsFramework::EntityIdList entityHierarchiesList(entityHierarchiesSet.begin(), entityHierarchiesSet.end());
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityHierarchiesList);

        // then reselect the entities that were originally selected
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, processedEntityIds);
    }

    EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
    return true;
}

QMimeData* OutlinerListModel::mimeData(const QModelIndexList& indexes) const
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    AZ::TypeId uuid1 = AZ::AzTypeInfo<AZ::Entity>::Uuid();
    AZ::TypeId uuid2 = AZ::AzTypeInfo<AzToolsFramework::EditorEntityIdContainer>::Uuid();

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
    if (m_layoutResetQueued)
    {
        return;
    }
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

// Helper struct for tracking a range of indices.
struct ModelIndexRange
{
public:
    QModelIndex m_start;
    QModelIndex m_end;
};


void OutlinerListModel::ProcessEntityUpdates()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    m_entityChangeQueued = false;
    if (m_layoutResetQueued)
    {
        return;
    }

    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Editor, "OutlinerListModel::ProcessEntityUpdates:ExpandQueue");
        for (auto entityId : m_entityExpandQueue)
        {
            emit ExpandEntity(entityId, IsExpanded(entityId));
        };
        m_entityExpandQueue.clear();
    }

    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Editor, "OutlinerListModel::ProcessEntityUpdates:SelectQueue");
        for (auto entityId : m_entitySelectQueue)
        {
            emit SelectEntity(entityId, IsSelected(entityId));
        };
        m_entitySelectQueue.clear();
    }

    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Editor, "OutlinerListModel::ProcessEntityUpdates:ChangeQueue");

        QMap<QModelIndex, ModelIndexRange> parentsToChangedIndices;

        for (auto entityId : m_entityChangeQueue)
        {
            auto myStartIndex = GetIndexFromEntity(entityId, ColumnName);
            ModelIndexRange& changedRange = parentsToChangedIndices[myStartIndex.parent()];

            // Expand the range around each changed index, even if a sibling between didn't change.
            // This is faster than emitting a data change for every modified index.
            if (!changedRange.m_start.isValid() ||
                myStartIndex.row() < changedRange.m_start.row())
            {
                changedRange.m_start = myStartIndex;
            }
            auto myEndIndex = GetIndexFromEntity(entityId, ColumnCount - 1);
            if (!changedRange.m_end.isValid() ||
                myEndIndex.row() > changedRange.m_end.row())
            {
                changedRange.m_end = myEndIndex;
            }
        };
        for (const auto& changedGroups : parentsToChangedIndices)
        {
            // Not checking for isValid for performance reasons, the scope marker above (ChangeQueue) was showing up
            // as a high cost in cloning large entity stacks. This data is all good because it was just generated.
            emit dataChanged(changedGroups.m_start, changedGroups.m_end);
        }
        m_entityChangeQueue.clear();
    }

    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Editor, "OutlinerListModel::ProcessEntityUpdates:LayoutChanged");
        if (m_entityLayoutQueued)
        {
            emit layoutAboutToBeChanged();
            emit layoutChanged();
        }
        m_entityLayoutQueued = false;
    }

    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Editor, "OutlinerListModel::ProcessEntityUpdates:InvalidateFilter");
        if (m_isFilterDirty)
        {
            InvalidateFilter();
        }
    }
}

void OutlinerListModel::OnEntityInfoResetBegin()
{
    emit EnableSelectionUpdates(false);
    beginResetModel();
}

void OutlinerListModel::OnEntityInfoResetEnd()
{
    m_layoutResetQueued = true;
    endResetModel();
    QTimer::singleShot(0, this, &OutlinerListModel::ProcessEntityInfoResetEnd);
}

void OutlinerListModel::ProcessEntityInfoResetEnd()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    m_layoutResetQueued = false;
    m_entityChangeQueued = false;
    m_entityChangeQueue.clear();
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
    if ((IsSelected(childId) || HasSelectedDescendant(childId)) && !m_dropOperationInProgress)
    {
        ExpandAncestors(childId);
    }

    //restore selection and expansion state for previously registered entity ids (for the view/model only)
    RestoreDescendantSelection(childId);
    RestoreDescendantExpansion(childId);

    //must refresh partial lock/visibility of parents
    m_isFilterDirty = true;
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
    m_isFilterDirty = true;
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
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
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
    if (selected && m_autoExpandEnabled)
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

void OutlinerListModel::OnEntityInfoUpdatedUnsavedChanges(AZ::EntityId entityId)
{
    QueueEntityUpdate(entityId);
}

void OutlinerListModel::OnEntityInfoUpdateSliceOwnership(AZ::EntityId entityId)
{
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
    m_isFilterDirty = true;
    if (filter.size() > 0)
    {
        CacheSelectionIfAppropriate();
    }

    m_filterString = filter;
    InvalidateFilter();

    RestoreSelectionIfAppropriate();
}

void OutlinerListModel::SearchFilterChanged(const AZStd::vector<ComponentTypeValue>& componentFilters)
{
    m_isFilterDirty = true;
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

void OutlinerListModel::AfterEntitySelectionChanged(const AzToolsFramework::EntityIdList&, const AzToolsFramework::EntityIdList&)
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
    m_isFilterDirty = true;
    m_entityExpansionState[entityId] = true;
    QueueEntityUpdate(entityId);
}

void OutlinerListModel::OnEntityCollapsed(const AZ::EntityId& entityId)
{
    m_isFilterDirty = true;
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
    m_isFilterDirty = false;
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

        AzToolsFramework::ScopedUndoBatch undo("Toggle Entity Lock State");

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
    // When an entity is unlocked, if it was in a locked layer(s), unlock those layers.
    // Layers are checked for recursively because lock status for a layer is currently saved in the level and not the layer itself.
    // When lock status can be saved in the layer, then this should only unlock the most immediate layer.
    if (!isLocked)
    {
        AZ::EntityId parentLayerCheck = entityId;
        while (parentLayerCheck.IsValid())
        {
            AZ::EntityId parentId;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, parentLayerCheck, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

            bool isParentLayer = false;
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                isParentLayer,
                parentId,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

            if (isParentLayer)
            {
                bool isParentLayerLocked = false;
                AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isParentLayerLocked, parentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
                if (isParentLayerLocked)
                {
                    // If a child of a layer has its lock state changed to false, change that layer to no longer be locked.
                    // Do this for all layers in the hierarchy.
                    AzToolsFramework::EditorLockComponentRequestBus::Event(parentId, &AzToolsFramework::EditorLockComponentRequests::SetLocked, false);
                }
            }

            parentLayerCheck = parentId;
        }
    }

    bool isLayer = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayer,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
    SetEditorLockStateRecursively(entityId, isLocked, entityId, isLayer);
}

void OutlinerListModel::SetEditorLockStateRecursively(const AZ::EntityId& entityId, bool isLocked, const AZ::EntityId& toggledEntityId, bool toggledEntityWasLayer)
{
    if (!entityId.IsValid())
    {
        return;
    }
    if (!toggledEntityWasLayer || toggledEntityId == entityId)
    {
        AzToolsFramework::EditorLockComponentRequestBus::Event(entityId, &AzToolsFramework::EditorLockComponentRequests::SetLocked, isLocked);
    }
    else
    {
        bool oldLockState = false;
        AzToolsFramework::EditorLockComponentRequestBus::EventResult(oldLockState, entityId, &AzToolsFramework::EditorLockComponentRequests::GetLocked);

        bool newLockState = isLocked ? true : oldLockState;
        AzToolsFramework::EditorLockComponentNotificationBus::Event(
            entityId,
            &AzToolsFramework::EditorLockComponentNotificationBus::Events::OnEntityLockChanged,
            newLockState);
    }

    AzToolsFramework::EntityIdList children;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);

    for (auto childId : children)
    {
        SetEditorLockStateRecursively(childId, isLocked, toggledEntityId, toggledEntityWasLayer);
    }
}

//! Editor Visibility interface to enable/disable rendering in the viewport.
void OutlinerListModel::ToggleEditorVisibility(const AZ::EntityId& entityId)
{
    if (entityId.IsValid())
    {
        bool isVisible = IsEntityVisible(entityId);

        AzToolsFramework::ScopedUndoBatch undo("Toggle Entity Visibility");

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
    // When an entity is set to visible, if it was in an invisible layer(s), make that layer visible.
    if (isVisible)
    {
        AZ::EntityId parentLayerCheck = entityId;
        while (parentLayerCheck.IsValid())
        {
            AZ::EntityId parentId;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, parentLayerCheck, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

            bool isParentLayer = false;
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                isParentLayer,
                parentId,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

            if (isParentLayer)
            {
                if (!IsEntityVisible(parentId))
                {
                    // If a child of a layer has its visibility state changed to true, change that layer to be visible.
                    // Do this for all layers in the hierarchy.
                    SetEntityVisibility(parentId, true);
                    // Even though layer visibility is saved to each layer individually, parents still need to be checked recursively so
                    // that the entity that was toggled can become visible.
                }
            }

            parentLayerCheck = parentId;
        }
    }
    bool isLayer = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayer,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
    SetEditorVisibilityStateRecursively(entityId, isVisible, entityId, isLayer);
}

void OutlinerListModel::SetEditorVisibilityStateRecursively(const AZ::EntityId& entityId, bool isVisible, const AZ::EntityId& toggledEntityId, bool toggledEntityWasLayer)
{
    if (!entityId.IsValid())
    {
        return;
    }
    if (!toggledEntityWasLayer || toggledEntityId == entityId)
    {
        SetEntityVisibility(entityId, isVisible);
    }
    else
    {
        bool oldVisibilityState = IsEntityVisible(entityId);

        bool newVisibilityState = isVisible ? oldVisibilityState : false;
        AzToolsFramework::EditorVisibilityNotificationBus::Event(
            entityId,
            &AzToolsFramework::EditorVisibilityNotificationBus::Events::OnEntityVisibilityChanged,
            newVisibilityState);
    }

    AzToolsFramework::EntityIdList children;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);

    for (auto childId : children)
    {
        SetEditorVisibilityStateRecursively(childId, isVisible, toggledEntityId, toggledEntityWasLayer);
    }
}

bool OutlinerListModel::IsEntityVisible(const AZ::EntityId& entityId) const
{
    // Visibility state is tracked in 5 places, see dataForLock for info on 3 of these ways.
    // Visibility's fourth state over lock is the EditorVisibilityRequestBus has two sets of
    // setting and getting functions for visibility. Get/SetVisibilityFlag is what should be used in most cases.
    // The fifth state is tracked on layers. Layers are always invisible to other systems, so the visibility flag
    // is set false there. However, layers need to be able to toggle visibility to hide/show their children, so
    // layers have a unique flag.
    bool isVisible = true;
    bool isLayerEntity = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerEntity,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
    if (isLayerEntity)
    {
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isVisible,
            entityId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);
    }
    else
    {
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(
            isVisible,
            entityId,
            &AzToolsFramework::EditorVisibilityRequestBus::Events::GetVisibilityFlag);
    }
    return isVisible;
}

void OutlinerListModel::SetEntityVisibility(const AZ::EntityId& entityId, bool visibility) const
{
    bool isLayerEntity = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerEntity,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

    if (isLayerEntity)
    {
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            entityId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility,
            visibility);
    }
    else
    {
        AzToolsFramework::EditorVisibilityRequestBus::Event(
            entityId,
            &AzToolsFramework::EditorVisibilityRequestBus::Events::SetVisibilityFlag,
            visibility);
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
        else
        {
            QueueEntityUpdate(entityId);
        }
    }

    int numberOfGlobalFlags = 0;
    // First check if any of the global flags are valid
    if (isFilterMatch && m_componentFilters.size() > 0)
    {
        int globalFlags = 0;

        for (ComponentTypeValue& componentType : m_componentFilters)
        {
            if (componentType.m_globalVal >= 0)
            {
                switch (componentType.m_globalVal)
                {
                case 0:
                    globalFlags |= static_cast<int>(GlobalSearchCriteriaFlags::Unlocked);
                    break;
                case 1:
                    globalFlags |= static_cast<int>(GlobalSearchCriteriaFlags::Locked);
                    break;
                case 2:
                    globalFlags |= static_cast<int>(GlobalSearchCriteriaFlags::Visible);
                    break;
                case 3:
                    globalFlags |= static_cast<int>(GlobalSearchCriteriaFlags::Hidden);
                    break;
                }
                ++numberOfGlobalFlags;
            }
        }
        if ((globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Unlocked)) && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Locked)))
        {
            globalFlags &= (static_cast<int>(GlobalSearchCriteriaFlags::Visible) | static_cast<int>(GlobalSearchCriteriaFlags::Hidden));
        }
        if ((globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Visible)) && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Hidden)))
        {
            globalFlags &= (static_cast<int>(GlobalSearchCriteriaFlags::Unlocked) | static_cast<int>(GlobalSearchCriteriaFlags::Locked));
        }
        if (globalFlags)
        {
            bool visibleFlag = false;
            AzToolsFramework::EditorVisibilityRequestBus::EventResult(visibleFlag, entityId, &AzToolsFramework::EditorVisibilityRequests::GetVisibilityFlag);
            if (visibleFlag && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Hidden)))
            {
                isFilterMatch = false;
            }
            if (!visibleFlag && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Visible)))
            {
                isFilterMatch = false;
            }
            bool isLocked = false;
            AzToolsFramework::EditorLockComponentRequestBus::EventResult(isLocked, entityId, &AzToolsFramework::EditorLockComponentRequests::GetLocked);
            if (isLocked && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Unlocked)))
            {
                isFilterMatch = false;
            }
            if (!isLocked && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Locked)))
            {
                isFilterMatch = false;
            }
        }
    }

    // If we matched the filter string and have any component filters, make sure we match those too
    if (isFilterMatch && m_componentFilters.size() > numberOfGlobalFlags)
    {
        bool hasMatchingComponent = false;

        for (ComponentTypeValue& componentType : m_componentFilters)
        {
            if (componentType.m_globalVal < 0)
            {
            if (AzToolsFramework::EntityHasComponentOfType(entityId, componentType.m_uuid, true, true))
                {
                    hasMatchingComponent = true;
                    break;
                }
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

void OutlinerListModel::EnableAutoExpand(bool enable)
{
    m_autoExpandEnabled = enable;
}

void OutlinerListModel::SetDropOperationInProgress(bool inProgress)
{
    m_dropOperationInProgress = inProgress;
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
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isLocked, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);

    AzToolsFramework::EntityIdList children;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
    for (auto childId : children)
    {
        bool isLockedChild = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isLockedChild, childId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
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

    bool isVisible = IsEntityVisible(entityId);

    AzToolsFramework::EntityIdList children;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
    for (auto childId : children)
    {
        bool isVisibleChild = IsEntityVisible(childId);
        if (isVisible != isVisibleChild || !AreAllDescendantsSameVisibleState(childId))
        {
            return false;
        }
    }
    return true;
}

bool OutlinerListModel::IsInLayerWithProperty(AZ::EntityId entityId, const LayerProperty& layerProperty) const
{
    while (entityId.IsValid())
    {
        AZ::EntityId parentId;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

        bool isParentLayer = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isParentLayer,
            parentId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

        if (isParentLayer)
        {
            if (layerProperty == LayerProperty::Locked)
            {
                bool isParentLayerLocked = false;
                AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isParentLayerLocked, parentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
                if (isParentLayerLocked)
                {
                    return true;
                }
                // If this layer wasn't locked, keep checking the hierarchy, a layer above this one may be locked.
            }
            else if (layerProperty == LayerProperty::Invisible)
            {
                bool isParentVisible = IsEntityVisible(parentId);
                if (!isParentVisible)
                {
                    return true;
                }
                // If this layer was visible, keep checking the hierarchy, a layer above this one may be invisible.
            }
        }

        entityId = parentId;
    }
    return false;
}

void OutlinerListModel::OnEntityInitialized(const AZ::EntityId& /*entityId*/)
{
    if (m_filterString.size() > 0 || m_componentFilters.size() > 0)
    {
        m_isFilterDirty = true;
        emit ReapplyFilter();
    }
}

void OutlinerListModel::OnEntityCompositionChanged(const AzToolsFramework::EntityIdList& /*entityIds*/)
{
    if (m_componentFilters.size() > 0)
    {
        m_isFilterDirty = true;
        emit ReapplyFilter();
    }
}

void OutlinerListModel::OnContextReset()
{
    if (m_filterString.size() > 0 || m_componentFilters.size() > 0)
    {
        m_isFilterDirty = true;
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
    m_visibilityCheckBoxLayerOverride.setObjectName("visibilityLayerOverride");
    m_lockCheckBoxLayerOverride.setObjectName("lockLayerOverride");
}

void OutlinerItemDelegate::DrawLayerStripeAndBorder(QPainter* painter, int stripeX, int top, int bottom, QColor layerBorderColor, QColor layerColor) const
{
    if (!painter)
    {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    QPen layerPen;
    layerPen.setWidthF(OutlinerListModel::GetLayerStripeWidth());
    layerPen.setColor(layerBorderColor);
    painter->setPen(layerPen);

    QLine layerStripeLine(stripeX, top, stripeX, bottom);
    painter->drawLine(layerStripeLine);

    layerPen.setColor(layerColor);
    // Draw the line a little taller so it connects to the top of the layer
    layerStripeLine.translate(OutlinerListModel::GetLayerStripeWidth(), 0);
    painter->setPen(layerPen);
    painter->drawLine(layerStripeLine);

    painter->restore();
}

void OutlinerItemDelegate::DrawLayerUI(
    QPainter* painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index,
    const AZ::EntityId& entityId,
    bool isSelected) const
{
    if (!painter)
    {
        return;
    }

    static const QColor layerBGColor(GetIEditor()->GetColorByName("LayerBackgroundColor"));
    static const QColor layerChildBGColor(GetIEditor()->GetColorByName("LayerChildBackgroundColor"));

    static const QColor layerBorderTopColor(GetIEditor()->GetColorByName("LayerBorderTop"));
    static const QColor layerBorderBottomColor(GetIEditor()->GetColorByName("LayerBorderBottom"));

    static const QColor selectedLayerBGColor(GetIEditor()->GetColorByName("LayerBGSelectionColor"));
    static const QColor selectedLayerChildBGColor(GetIEditor()->GetColorByName("LayerChildBGSelectionColor"));

    bool isLayerEntity = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerEntity,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
    QColor layerColor;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        layerColor,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerColor);

    const QTreeView* outlinerTreeView(qobject_cast<const QTreeView*>(option.widget));
    int indentation = outlinerTreeView->indentation();

    bool isFirstColumn = index.column() == OutlinerListModel::ColumnName;
    bool hasVisibleChildren = index.data(OutlinerListModel::ExpandedRole).value<bool>() && index.model()->hasChildren(index);
    if (isLayerEntity &&
        isFirstColumn)
    {
        DrawLayerStripeAndBorder(
            painter,
            option.rect.left(),
            option.rect.top(),
            option.rect.bottom(),
            layerBorderBottomColor,
            layerColor);
    }

    bool isInLayer = false;

    // Start by assuming this row is the last in a layer, because it's easier to check if it's not the last in the layer.
    bool continueCheckingLastInLayer = true;
    QModelIndex lastInLayerIndex(index);
    int indentationIfLastInLayer = 0;

    QModelIndex sibling = index.sibling(index.row() + 1, index.column());
    // This row can't be the last in a layer if it has children and is expanded.
    if (index.data(OutlinerListModel::ExpandedRole).value<bool>() &&
        index.model()->hasChildren(index))
    {
        continueCheckingLastInLayer = false;
        lastInLayerIndex = QModelIndex();
    }
    else if (sibling.isValid())
    {
        // If the entity is a layer and has a sibling, but did not have children it's the last entity in the layer.
        continueCheckingLastInLayer = false;
        if (!isLayerEntity)
        {
            // If this entity has a sibling in the next row and isn't a layer itself, then it can't be the last entity in a layer.
            lastInLayerIndex = QModelIndex();
        }
    }

    int ancestorIndentation = 0;
    QModelIndex previousAncestor = index;

    for (QModelIndex ancestorIndex = index.parent(); ancestorIndex.isValid(); ancestorIndex = ancestorIndex.parent())
    {
        ancestorIndentation -= indentation;
        OutlinerListModel::EntryType ancestorEntryType =
            OutlinerListModel::EntryType(ancestorIndex.data(OutlinerListModel::EntityTypeRole).value<int>());
        if (ancestorEntryType == OutlinerListModel::LayerType && !isInLayer)
        {
            isInLayer = true;
        }

        if (continueCheckingLastInLayer && ancestorEntryType == OutlinerListModel::LayerType)
        {
            // If this ancestor is a layer, and our previous ancestor does not have a sibling, then the index is the last in the layer.
            indentationIfLastInLayer = ancestorIndentation + indentation;
            QModelIndex previousAncestorSibling = previousAncestor.sibling(previousAncestor.row() + 1, previousAncestor.column());
            if (previousAncestorSibling.isValid())
            {
                continueCheckingLastInLayer = false;
            }
        }

        if (ancestorEntryType == OutlinerListModel::LayerType && isFirstColumn)
        {
            QColor ancestorLayerColor = ancestorIndex.data(OutlinerListModel::LayerColorRole).value<QColor>();
            int stripeX = option.rect.left() + ancestorIndentation;
            DrawLayerStripeAndBorder(
                painter,
                stripeX,
                option.rect.top() - m_layerDividerLineHeight,
                option.rect.bottom(),
                layerBorderBottomColor,
                ancestorLayerColor);
        }

        previousAncestor = ancestorIndex;
    }
    // If this entity is the last entity in a layer at the root of the outliner, but is not a root entity itself,
    // then adjust the indentation one more time for that.
    if (continueCheckingLastInLayer && index.parent().isValid())
    {
        indentationIfLastInLayer -= indentation;
    }

    const bool isLayerOrInLayer = isLayerEntity || isInLayer;

    if (isLayerOrInLayer)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);

        QPainterPath layerBGPath;
        QRect layerBGRect(option.rect);
        if (isFirstColumn && isLayerEntity)
        {
            layerBGRect.setLeft(layerBGRect.left() + indentation + OutlinerListModel::GetLayerStripeWidth());
        }
        layerBGPath.addRect(layerBGRect);
        QColor layerBG = isLayerEntity ? layerBGColor : layerChildBGColor;
        if (isSelected)
        {
            layerBG = isLayerEntity ? selectedLayerBGColor : selectedLayerChildBGColor;
        }
        painter->fillPath(layerBGPath, layerBG);

        QPoint lineBottomLeft(option.rect.bottomLeft());
        if (isLayerEntity)
        {
            QPoint lineTopLeft(option.rect.topLeft());

            if (isFirstColumn)
            {
                lineTopLeft.setX(lineTopLeft.x() + OutlinerListModel::GetLayerStripeWidth());
            }
            QPen topLinePen(layerBorderTopColor, m_layerDividerLineHeight);
            painter->setPen(topLinePen);
            painter->drawLine(lineTopLeft, option.rect.topRight());
        }
        // Each layer entity has a line drawn on the bottom, and the last entity in the layer has the same line drawn.
        if (isLayerEntity || lastInLayerIndex.isValid())
        {
            if (isFirstColumn)
            {
                lineBottomLeft.setX(lineBottomLeft.x() + indentationIfLastInLayer);
                // Leave room for the 2 layer stripes on the first child to connect to the layer box.
                if (!isLayerEntity || hasVisibleChildren)
                {
                    lineBottomLeft.setX(lineBottomLeft.x() + OutlinerListModel::GetLayerStripeWidth());
                }
            }
            int dividerLineHeight = m_layerDividerLineHeight;
            QPen bottomLinePen(layerBorderBottomColor, dividerLineHeight);
            painter->setPen(bottomLinePen);
            painter->drawLine(lineBottomLeft, option.rect.bottomRight());
        }

        painter->restore();
    }
}

QString OutlinerItemDelegate::GetLayerInfoString(const AZ::EntityId& entityId) const
{
    QString result;
    bool isLayerEntity = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerEntity,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
    if (!isLayerEntity)
    {
        return result;
    }

    bool hasUnsavedLayerChanges = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        hasUnsavedLayerChanges,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasUnsavedChanges);

    if (hasUnsavedLayerChanges)
    {
        result = QObject::tr("*");
    }

    bool isLayerNameValid = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerNameValid,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::IsLayerNameValid);

    if (!isLayerNameValid)
    {
        result = QObject::tr("<font color=\"red\">(!)</font> %1").arg(result);
    }

    return result;
}

int OutlinerItemDelegate::GetEntityNameVerticalOffset(const AZ::EntityId& entityId) const
{
    // Start with a slight offset so that the entity name better aligns to the middle of the row.
    // The default vertical alignment ends up looking a little low on the row.
    const int entityNameOffset = -1;
    bool isLayerEntity = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerEntity,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
    if (!isLayerEntity)
    {
        return entityNameOffset;
    }
    // Layers draw a divider over the bottom of the row, so adjust the text rect to account for that.
    return -m_layerDividerLineHeight + entityNameOffset;
}

QString OutlinerItemDelegate::GetColumnHighlightedStylesheet(int column, bool highlighted) const
{
    QString stylesheet;
    QString color;
    QString checkBoxName;

    static const QColor outlinerHighlightColor(GetIEditor()->GetColorByName("OutlinerSelectionColor"));

    if (column == OutlinerListModel::ColumnVisibilityToggle)
    {
        checkBoxName = "OutlinerVisibilityCheckBox";
    }
    else if (column == OutlinerListModel::ColumnLockToggle)
    {
        checkBoxName = "OutlinerLockCheckBox";
    }
    else
    {
        return "";
    }

    if (highlighted)
    {
        color = outlinerHighlightColor.name();
    }
    else
    {
        color = "transparent";
    }

    stylesheet = QString("%1{background-color: %2} %1::indicator{background-color: %2)").arg(checkBoxName).arg(color);

    return stylesheet;
}

void OutlinerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    static const QColor sliceRootBackgroundColor(GetIEditor()->GetColorByName("SliceRootBackgroundColor"));
    static const QColor sliceRootBorderColor(GetIEditor()->GetColorByName("SliceRootBorderColor"));

    static const QColor selectedSliceRootBackgroundColor(GetIEditor()->GetColorByName("SelectedSliceRootBackgroundColor"));
    static const QColor selectedSliceRootBorderColor(GetIEditor()->GetColorByName("SelectedSliceRootBorderColor"));

    static const QColor sliceEntityColor(GetIEditor()->GetColorByName("SliceEntityColor"));
    static const QColor sliceOverrideColor(GetIEditor()->GetColorByName("SliceOverrideColor"));
    
    bool entityHasOverrides = false, childrenHaveOverrides = false, isSliceEntity = false;

    AZ::EntityId entityId(index.data(OutlinerListModel::EntityIdRole).value<AZ::u64>());
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceEntity, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceEntity);
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(entityHasOverrides, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::HasSliceEntityOverrides);
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(childrenHaveOverrides, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::HasSliceChildrenOverrides);

    const bool sliceHasOverrides = (entityHasOverrides || childrenHaveOverrides);

    static const QColor outlinerHighlightColor(GetIEditor()->GetColorByName("OutlinerSelectionColor"));

    const bool isSelected = (option.state & QStyle::State_Selected);
    if (isSelected)
    {
        bool isLayerEntity = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isLayerEntity,
            entityId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

        QRect selectionRect = option.rect;
        if (isLayerEntity && index.column() == OutlinerListModel::ColumnName)
        {
            selectionRect.setLeft(selectionRect.left() + OutlinerTreeView::GetLayerSquareSize());
        }
        QPainterPath path;
        path.addRect(selectionRect);
        painter->fillPath(path, outlinerHighlightColor);
    }

    DrawLayerUI(painter, option, index, entityId, isSelected);

    QPalette checkboxPalette;
    QColor transparentColor(0, 0, 0, 0);
    checkboxPalette.setColor(QPalette::ColorRole::Background, transparentColor);

    const bool isSliceRoot = index.data(OutlinerListModel::SliceBackgroundRole).value<bool>();

    const int slicePillCornerRadius = 4;
    // We're only using these check boxes as renderers so their actual state doesn't matter.
    // We can set it right before we draw using information from the model data.
    if (index.column() == OutlinerListModel::ColumnVisibilityToggle)
    {
        painter->save();
        painter->translate(option.rect.topLeft());

        OutlinerVisibilityCheckBox* checkboxToRender = &m_visibilityCheckBox;

        if (index.data(OutlinerListModel::InInvisibleLayerRole).value<bool>())
        {
            m_visibilityCheckBoxLayerOverride.setStyleSheet(GetColumnHighlightedStylesheet(index.column(), isSelected));
            checkboxToRender = &m_visibilityCheckBoxLayerOverride;
        }
        else if (index.data(OutlinerListModel::PartiallyVisibleRole).value<bool>())
        {
            m_visibilityCheckBoxWithBorder.setStyleSheet(GetColumnHighlightedStylesheet(index.column(), isSelected));
            checkboxToRender = &m_visibilityCheckBoxWithBorder;
        }
        bool checked = index.data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked;
        checkboxToRender->setChecked(checked);
        checkboxToRender->setPalette(checkboxPalette);
        checkboxToRender->render(painter);

        painter->restore();

        return;
    }

    if (index.column() == OutlinerListModel::ColumnLockToggle)
    {
        painter->save();
        painter->translate(option.rect.topLeft());

        OutlinerLockCheckBox* checkboxToRender = &m_lockCheckBox;

        if (index.data(OutlinerListModel::InLockedLayerRole).value<bool>())
        {
            m_lockCheckBoxLayerOverride.setStyleSheet(GetColumnHighlightedStylesheet(index.column(), isSelected));
            checkboxToRender = &m_lockCheckBoxLayerOverride;
        }
        else if (index.data(OutlinerListModel::PartiallyLockedRole).value<bool>())
        {
            m_lockCheckBoxWithBorder.setStyleSheet(GetColumnHighlightedStylesheet(index.column(), isSelected));
            checkboxToRender = &m_lockCheckBoxWithBorder;
        }
        bool checked = index.data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked;
        checkboxToRender->setChecked(checked);
        checkboxToRender->setPalette(checkboxPalette);
        checkboxToRender->render(painter);

        painter->restore();
        return;
    }

    QStyleOptionViewItem customOptions{ option }; 
    if (customOptions.state & QStyle::State_HasFocus)
    {
        //  Do not draw the focus rectangle in this column.
        customOptions.state ^= QStyle::State_HasFocus;
    }

    auto backgroundBoxRect = option.rect;

    backgroundBoxRect.setX(backgroundBoxRect.x() + 0.5);
    backgroundBoxRect.setY(backgroundBoxRect.y() + 2.5);
    backgroundBoxRect.setWidth(backgroundBoxRect.width() - 1.0);
    backgroundBoxRect.setHeight(backgroundBoxRect.height() - 1.0);

    const int sliceBorderHeight = 1;

    // Draw this Slice Handle Accent if the item is not selected before the
    // entry is drawn.
    if (!isSelected)
    {
        if (isSliceRoot)
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(backgroundBoxRect, slicePillCornerRadius, slicePillCornerRadius);
            QPen pen(sliceRootBorderColor, 1);
            painter->setPen(pen);
            painter->fillPath(path, sliceRootBackgroundColor);
            painter->drawPath(path);
            painter->restore();
        }

        // Draw a dashed line around any visible, collapsed entry in the outliner that has
        // children underneath it currently selected.
        if (!index.data(OutlinerListModel::ExpandedRole).template value<bool>()
            && index.data(OutlinerListModel::ChildSelectedRole).template value<bool>())
        {
            QPainterPath path;
            path.addRoundedRect(backgroundBoxRect, slicePillCornerRadius, slicePillCornerRadius);

            // Get the foreground color of the current object to draw our sub-object-selected box
            auto targetColor = index.data(Qt::ForegroundRole).value<QBrush>().color();
            if (isSliceEntity)
            {
                targetColor = (sliceHasOverrides ? sliceOverrideColor : sliceEntityColor);
            }
            QPen pen(targetColor, 2);

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
    else
    {
        //draw slice root background
        if (isSliceRoot)
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(backgroundBoxRect, 4, 4);
            QPen pen(selectedSliceRootBorderColor, sliceBorderHeight);
            painter->setPen(pen);
            painter->fillPath(path, selectedSliceRootBackgroundColor);
            painter->drawPath(path);
            painter->restore();
        }
    }

    if (index.column() == OutlinerListModel::ColumnName)
    {
        OutlinerListModel::s_paintingName = true;
        // standard painter can't handle rich text so we have to handle it
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QStyleOptionViewItemV4 optionV4{ customOptions };
        initStyleOption(&optionV4, index);
        optionV4.state &= ~(QStyle::State_HasFocus | QStyle::State_Selected);
     
        // get the rich text and save for later

        QString layerInfoString = GetLayerInfoString(entityId);

        QRect textRect = optionV4.widget->style()->proxy()->subElementRect(QStyle::SE_ItemViewItemText, &optionV4);

        QRegularExpression htmlMarkupRegex("<[^>]*>");

        // Start with the raw rich text for the entity name.
        QString entityNameRichText = optionV4.text;

        // If there is any HTML markup in the entity name, don't elide.
        if (!htmlMarkupRegex.match(entityNameRichText).hasMatch())
        {
            QFontMetrics fontMetrics(optionV4.font);
            int textWidthAvailable = textRect.width();
            // Qt uses "..." for elide, but there doesn't seem to be a way to retrieve this exact string from Qt.
            // Subtract the elide string from the width available, so it can actually appear.
            textWidthAvailable -= fontMetrics.width(QObject::tr("..."));
            if (!layerInfoString.isEmpty())
            {
                // The layer info string includes HTML markup, which can cause an issue computing the width.
                // The markup on the layer text is light (just color for now, may include italic or bold later), so 
                // an approximate width is computed by taking the width of the non-HTML portion of the string and padding it a bit.
                QString htmlStripped = layerInfoString;
                htmlStripped.remove(htmlMarkupRegex);
                const float layerInfoPadding = 1.2;
                textWidthAvailable -= fontMetrics.width(htmlStripped) * layerInfoPadding;
            }

            entityNameRichText = fontMetrics.elidedText(optionV4.text, Qt::TextElideMode::ElideRight, textWidthAvailable);
        }

        if (!layerInfoString.isEmpty())
        {
            entityNameRichText = QObject::tr("%1%2").arg(entityNameRichText).arg(layerInfoString);
        }

        // delete the text from the item so we can use the standard painter to draw the icon
        optionV4.text.clear();
        optionV4.widget->style()->drawControl(QStyle::CE_ItemViewItem, &optionV4, painter);

        // Now we setup a Text Document so it can draw the rich text
        QTextDocument textDoc;
        textDoc.setDefaultFont(optionV4.font);
        textDoc.setDefaultStyleSheet("body {color: white}");
        textDoc.setHtml("<body>" + entityNameRichText + "</body>");
        int verticalOffset = GetEntityNameVerticalOffset(entityId);
        painter->translate(textRect.topLeft() + QPoint(0, verticalOffset));
        textDoc.setTextWidth(textRect.width());
        textDoc.drawContents(painter, QRectF(0, 0, textRect.width(), textRect.height()));

        painter->restore();
        OutlinerListModel::s_paintingName = false;
    }
    else
    {
        QStyledItemDelegate::paint(painter, customOptions, index);
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
