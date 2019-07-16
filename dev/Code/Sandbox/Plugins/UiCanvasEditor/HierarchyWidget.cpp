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

#include "EditorCommon.h"
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>

HierarchyWidget::HierarchyWidget(EditorWindow* editorWindow)
    : QTreeWidget()
    , m_isDeleting(false)
    , m_editorWindow(editorWindow)
    , m_entityItemMap()
    , m_itemBeingHovered(nullptr)
    , m_inDragStartState(false)
    , m_selectionChangedBeforeDrag(false)
    , m_signalSelectionChange(true)
    , m_inObjectPickMode(false)
    , m_inited(false)
{
    setMouseTracking(true);

    // Style.
    {
        setAcceptDrops(true);
        setDropIndicatorShown(true);
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragDrop);
        setSelectionMode(QAbstractItemView::ExtendedSelection);

        setColumnCount(kHierarchyColumnCount);
        setHeader(new HierarchyHeader(this));

        // IMPORTANT: This MUST be done here.
        // This CAN'T be done inside HierarchyHeader.
        header()->setSectionsClickable(true);

        header()->setSectionResizeMode(kHierarchyColumnName, QHeaderView::Stretch);
        header()->setSectionResizeMode(kHierarchyColumnIsVisible, QHeaderView::Fixed);
        header()->setSectionResizeMode(kHierarchyColumnIsSelectable, QHeaderView::Fixed);

        // This controls the width of the last 2 columns; both in the header and in the body of the HierarchyWidget.
        header()->resizeSection(kHierarchyColumnIsVisible, UICANVASEDITOR_HIERARCHY_HEADER_ICON_SIZE);
        header()->resizeSection(kHierarchyColumnIsSelectable, UICANVASEDITOR_HIERARCHY_HEADER_ICON_SIZE);
    }

    // Connect signals.
    {
        // Selection change notification.
        QObject::connect(selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
            SLOT(CurrentSelectionHasChanged(const QItemSelection &, const QItemSelection &)));

        QObject::connect(model(),
            SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)),
            SLOT(DataHasChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)));
    }

    QObject::connect(this,
        &QTreeWidget::itemClicked,
        this,
        [this](QTreeWidgetItem* item, int column)
        {
            HierarchyItem* i = dynamic_cast<HierarchyItem*>(item);

            if (column == kHierarchyColumnIsVisible)
            {
                ToggleVisibility(i);
            }
            else if (column == kHierarchyColumnIsSelectable)
            {
                CommandHierarchyItemToggleIsSelectable::Push(m_editorWindow->GetActiveStack(),
                    this,
                    HierarchyItemRawPtrList({i}));
            }
            else if (m_inObjectPickMode)
            {
                PickItem(i);
            }
        });

    QObject::connect(this,
        &QTreeWidget::itemExpanded,
        this,
        [this](QTreeWidgetItem* item)
        {
            CommandHierarchyItemToggleIsExpanded::Push(m_editorWindow->GetActiveStack(),
                this,
                dynamic_cast<HierarchyItem*>(item));
        });

    QObject::connect(this,
        &QTreeWidget::itemCollapsed,
        this,
        [this](QTreeWidgetItem* item)
        {
            CommandHierarchyItemToggleIsExpanded::Push(m_editorWindow->GetActiveStack(),
                this,
                dynamic_cast<HierarchyItem*>(item));
        });

    EntityHighlightMessages::Bus::Handler::BusConnect();
}

HierarchyWidget::~HierarchyWidget()
{
    AzToolsFramework::EditorPickModeNotificationBus::Handler::BusDisconnect();
    EntityHighlightMessages::Bus::Handler::BusDisconnect();
}

void HierarchyWidget::SetIsDeleting(bool b)
{
    m_isDeleting = b;
}

EntityHelpers::EntityToHierarchyItemMap& HierarchyWidget::GetEntityItemMap()
{
    return m_entityItemMap;
}

EditorWindow* HierarchyWidget::GetEditorWindow()
{
    return m_editorWindow;
}

void HierarchyWidget::ActiveCanvasChanged()
{
    EntityContextChanged();
}

void HierarchyWidget::EntityContextChanged()
{
    if (m_inObjectPickMode)
    {
        OnEntityPickModeStopped();
    }

    // Disconnect from the PickModeRequests bus and reconnect with the new entity context
    AzToolsFramework::EditorPickModeNotificationBus::Handler::BusDisconnect();
    UiEditorEntityContext* context = m_editorWindow->GetEntityContext();
    if (context)
    {
        AzToolsFramework::EditorPickModeNotificationBus::Handler::BusConnect(context->GetContextId());
    }
}

void HierarchyWidget::CreateItems(const LyShine::EntityArray& elements)
{
    std::list<AZ::Entity*> elementList(elements.begin(), elements.end());

    // Build the rest of the list.
    // Note: This is a breadth-first traversal through all child elements.
    for (auto& e : elementList)
    {
        LyShine::EntityArray childElements;
        EBUS_EVENT_ID_RESULT(childElements, e->GetId(), UiElementBus, GetChildElements);
        elementList.insert(elementList.end(), childElements.begin(), childElements.end());
    }

    // Create the items.
    for (auto& e : elementList)
    {
        QTreeWidgetItem* parent = HierarchyHelpers::ElementToItem(this, EntityHelpers::GetParentElement(e), true);

        HierarchyItem* child = new HierarchyItem(m_editorWindow,
                parent,
                e->GetName().c_str(),
                e);

        // Reorder.
        {
            int index = -1;
            EBUS_EVENT_ID_RESULT(index, EntityHelpers::GetParentElement(e)->GetId(), UiElementBus, GetIndexOfChild, e);

            parent->removeChild(child);
            parent->insertChild(index, child);
        }
    }

    // restore the expanded state of all items
    ApplyElementIsExpanded();

    m_inited = true;
}

void HierarchyWidget::RecreateItems(const LyShine::EntityArray& elements)
{
    // remember the currently selected items so we can restore them
    EntityHelpers::EntityIdList selectedEntityIds = SelectionHelpers::GetSelectedElementIds(this,
        selectedItems(), false);

    ClearItems();

    CreateItems(elements);

    HierarchyHelpers::SetSelectedItems(this, &selectedEntityIds);
}

void HierarchyWidget::ClearItems()
{
    ClearAllHierarchyItemEntityIds();

    // Remove all the items from the list (doesn't delete Entities since we cleared the EntityIds)
    clear();

    // The map needs to be cleared here since HandleItemRemove won't remove the map entry due to the entity Ids being cleared above
    m_entityItemMap.clear();

    m_inited = false;
}

AZ::Entity* HierarchyWidget::CurrentSelectedElement() const
{
    auto currentItem = dynamic_cast<HierarchyItem*>(QTreeWidget::currentItem());
    AZ::Entity* currentElement = (currentItem && currentItem->isSelected()) ? currentItem->GetElement() : nullptr;
    return currentElement;
}

void HierarchyWidget::contextMenuEvent(QContextMenuEvent* ev)
{
    // The context menu.
    if (m_inited)
    {
        HierarchyMenu contextMenu(this,
            (HierarchyMenu::Show::kCutCopyPaste |
             HierarchyMenu::Show::kSavePrefab |
             HierarchyMenu::Show::kNew_EmptyElement |
             HierarchyMenu::Show::kNew_ElementFromPrefabs |
             HierarchyMenu::Show::kDeleteElement |
             HierarchyMenu::Show::kNewSlice |
             HierarchyMenu::Show::kNew_InstantiateSlice |
             HierarchyMenu::Show::kPushToSlice |
             HierarchyMenu::Show::kFindElements |
             HierarchyMenu::Show::kEditorOnly),
            true);

        contextMenu.exec(ev->globalPos());
    }

    QTreeWidget::contextMenuEvent(ev);
}

void HierarchyWidget::SignalUserSelectionHasChanged(const QTreeWidgetItemRawPtrQList& selectedItems)
{
    HierarchyItemRawPtrList items = SelectionHelpers::GetSelectedHierarchyItems(this,
            selectedItems);
    SetUserSelection(items.empty() ? nullptr : &items);
}

void HierarchyWidget::CurrentSelectionHasChanged(const QItemSelection& selected,
    const QItemSelection& deselected)
{
    m_selectionChangedBeforeDrag = true;

    // IMPORTANT: This signal is triggered at the right time, but
    // "selected.indexes()" DOESN'T contain ALL the items currently
    // selected. It ONLY contains the newly selected items. To avoid
    // having to track what's added and removed to the selection,
    // we'll use selectedItems().

    if (m_signalSelectionChange && !m_isDeleting)
    {
        SignalUserSelectionHasChanged(selectedItems());
    }
}

void HierarchyWidget::DataHasChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if (topLeft == bottomRight)
    {
        // We only care about text changes, which can ONLY be done one at a
        // time. This implies that topLeft must be the same as bottomRight.

        HierarchyItem* hierarchyItem = dynamic_cast<HierarchyItem*>(itemFromIndex(topLeft));
        AZ::Entity* element = hierarchyItem->GetElement();
        AZ_Assert(element, "No entity found for hierarchy item");
        AZ::EntityId entityId = element->GetId();
        QTreeWidgetItem* item = HierarchyHelpers::ElementToItem(this, element, false);
        QString toName(item ? item->text(0) : "");

        CommandHierarchyItemRename::Push(m_editorWindow->GetActiveStack(),
            this,
            entityId,
            element->GetName().c_str(),
            toName);
    }
}

void HierarchyWidget::HandleItemAdd(HierarchyItem* item)
{
    m_entityItemMap[ item->GetEntityId() ] = item;
}

void HierarchyWidget::HandleItemRemove(HierarchyItem* item)
{
    if (item == m_itemBeingHovered)
    {
        m_itemBeingHovered = nullptr;
    }

    m_entityItemMap.erase(item->GetEntityId());
}

void HierarchyWidget::ReparentItems(bool onCreationOfElement,
    const QTreeWidgetItemRawPtrList& baseParentItems,
    const HierarchyItemRawPtrList& childItems)
{
    CommandHierarchyItemReparent::Push(onCreationOfElement,
        m_editorWindow->GetActiveStack(),
        this,
        childItems,
        baseParentItems);
}

void HierarchyWidget::ClearAllHierarchyItemEntityIds()
{
    // as a simple way of going through all the HierarchyItem's we use the
    // EntityHelpers::EntityToHierarchyItemMap
    for (auto mapItem : m_entityItemMap)
    {
        mapItem.second->ClearEntityId();
    }
}

void HierarchyWidget::ApplyElementIsExpanded()
{
    // Seed the list.
    HierarchyItemRawPtrList allItems;
    HierarchyHelpers::AppendAllChildrenToEndOfList(m_editorWindow->GetHierarchy()->invisibleRootItem(), allItems);

    // Traverse the list.
    blockSignals(true);
    {
        HierarchyHelpers::TraverseListAndAllChildren(allItems,
            [](HierarchyItem* childItem)
            {
                childItem->ApplyElementIsExpanded();
            });
    }
    blockSignals(false);
}

void HierarchyWidget::mousePressEvent(QMouseEvent* ev)
{
    m_selectionChangedBeforeDrag = false;

    HierarchyItem* item = dynamic_cast<HierarchyItem*>(itemAt(ev->pos()));
    if (!item)
    {
        // This allows the user to UNSELECT an item
        // by clicking in an empty area of the widget.
        SetUniqueSelectionHighlight((QTreeWidgetItem*)nullptr);
    }

    // Remember the selected items before the selection change in case a drag is started.
    // When dragging outside the hierarchy, the selection is reverted back to this selection
    m_beforeDragSelection = selectedItems();

    m_signalSelectionChange = false;

    QTreeWidget::mousePressEvent(ev);

    m_signalSelectionChange = true;
}

void HierarchyWidget::mouseDoubleClickEvent(QMouseEvent* ev)
{
    HierarchyItem* item = dynamic_cast<HierarchyItem*>(itemAt(ev->pos()));
    if (item)
    {
        // Double-clicking to edit text is only allowed in the FIRST column.
        for (int col = kHierarchyColumnIsVisible; col < kHierarchyColumnCount; ++col)
        {
            QRect r = visualRect(indexFromItem(item, col));
            if (r.contains(ev->pos()))
            {
                // Ignore the event.
                return;
            }
        }
    }

    QTreeWidget::mouseDoubleClickEvent(ev);
}

void HierarchyWidget::startDrag(Qt::DropActions supportedActions)
{
    // This flag is used to determine whether to perform an action on leaveEvent.
    // If an item is dragged really fast outside the hierarchy, this startDrag event is called,
    // but the dragEnterEvent and dragLeaveEvent are replaced with the leaveEvent
    m_inDragStartState = true;

    // Remember the current selection so that we can revert back to it when the items are dragged back into the hierarchy
    m_dragSelection = selectedItems();

    QTreeView::startDrag(supportedActions);
}

void HierarchyWidget::dragEnterEvent(QDragEnterEvent * event)
{
    if (!AcceptsMimeData(event->mimeData()))
    {
        return;
    }

    m_inDragStartState = false;

    if (m_selectionChangedBeforeDrag)
    {
        m_signalSelectionChange = false;

        // Set the current selection to the items being dragged
        clearSelection();
        for (auto i : m_dragSelection)
        {
            i->setSelected(true);
        }

        m_signalSelectionChange = true;
    }

    QTreeView::dragEnterEvent(event);
}

void HierarchyWidget::dragLeaveEvent(QDragLeaveEvent * event)
{
    // This is called when dragging outside the hierarchy, or when a drag is released inside the hierarchy
    // but a dropEvent isn't called (ex. drop item onto itself or press Esc to cancel a drag)

    // Check if mouse position is inside or outside the hierarchy
    QRect widgetRect = geometry();
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    if (widgetRect.contains(mousePos))
    {
        if (m_selectionChangedBeforeDrag)
        {
            // Treat this event as a mouse release (mouseReleaseEvent is not called in this case)
            SignalUserSelectionHasChanged(selectedItems());
        }
    }
    else
    {
        if (m_selectionChangedBeforeDrag)
        {
            m_signalSelectionChange = false;

            // Set the current selection to the items that were selected before the drag
            clearSelection();
            for (auto i : m_beforeDragSelection)
            {
                i->setSelected(true);
            }

            m_signalSelectionChange = true;
        }
    }

    QTreeView::dragLeaveEvent(event);
}

void HierarchyWidget::dropEvent(QDropEvent* ev)
{
    m_inDragStartState = false;
    
    m_signalSelectionChange = false;

    // Get a list of selected items
    QTreeWidgetItemRawPtrQList selection = selectedItems();

    // Change current selection to only contain top level items. This avoids
    // the default drop behavior from changing the internal hierarchy of 
    // the dragged elements
    QTreeWidgetItemRawPtrQList topLevelSelection;
    SelectionHelpers::GetListOfTopLevelSelectedItems(this, selection, topLevelSelection);
    clearSelection();
    for (auto i : topLevelSelection)
    {
        i->setSelected(true);
    }

    // Set current parent and child index of each selected item
    for (auto i : selection)
    {
        HierarchyItem* item = dynamic_cast<HierarchyItem*>(i);
        if (item)
        {
            QModelIndex itemIndex = indexFromItem(item);

            QTreeWidgetItem* baseParentItem = itemFromIndex(itemIndex.parent());
            if (!baseParentItem)
            {
                baseParentItem = invisibleRootItem();
            }
            HierarchyItem* parentItem = dynamic_cast<HierarchyItem*>(baseParentItem);
            AZ::EntityId parentId = (parentItem ? parentItem->GetEntityId() : AZ::EntityId());

            item->SetPreMove(parentId, itemIndex.row());
        }
    }

    // Do the drop event
    ev->setDropAction(Qt::MoveAction);
    QTreeWidget::dropEvent(ev);

    // Make a list of selected items and their parents
    HierarchyItemRawPtrList childItems;
    QTreeWidgetItemRawPtrList baseParentItems;

    bool itemMoved = false;

    for (auto i : selection)
    {
        HierarchyItem* item = dynamic_cast<HierarchyItem*>(i);
        if (item)
        {
            QModelIndex index = indexFromItem(item);

            QTreeWidgetItem* baseParentItem = itemFromIndex(index.parent()); 
            if (!baseParentItem)
            {
                baseParentItem = invisibleRootItem();
            }
            HierarchyItem* parentItem = dynamic_cast<HierarchyItem*>(baseParentItem);
            AZ::EntityId parentId = parentItem ? parentItem->GetEntityId() : AZ::EntityId();

            if ((item->GetPreMoveChildRow() != index.row()) || (item->GetPreMoveParentId() != parentId))
            {
                // Item has moved
                itemMoved = true;
            }

            childItems.push_back(item);
            baseParentItems.push_back(baseParentItem);
        }
    }

    if (itemMoved)
    {
        ReparentItems(false, baseParentItems, childItems);
    }
    else
    {
        // Items didn't move, but they became unselected so they need to be reselected
        for (auto i : childItems)
        {
            i->setSelected(true);
        }
    }

    m_signalSelectionChange = true;

    if (m_selectionChangedBeforeDrag)
    {
        // Signal a selection change on the mouse release
        SignalUserSelectionHasChanged(selectedItems());
    }
}

QStringList HierarchyWidget::mimeTypes() const
{
    QStringList list = QTreeWidget::mimeTypes();
    list.append(AzToolsFramework::EditorEntityIdContainer::GetMimeType());
    return list;
}

QMimeData* HierarchyWidget::mimeData(const QList<QTreeWidgetItem*> items) const
{
    AzToolsFramework::EditorEntityIdContainer entityIdList;
    for (auto i : items)
    {
        HierarchyItem* item = dynamic_cast<HierarchyItem*>(i);
        AZ::EntityId entityId = item->GetEntityId();
        if (entityId.IsValid())
        {
            entityIdList.m_entityIds.push_back(entityId);
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

bool HierarchyWidget::AcceptsMimeData(const QMimeData *mimeData)
{
    if (!mimeData || !mimeData->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()))
    {
        return false;
    }

    QByteArray arrayData = mimeData->data(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

    AzToolsFramework::EditorEntityIdContainer entityIdListContainer;
    if (!entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
    {
        return false;
    }

    if (entityIdListContainer.m_entityIds.empty())
    {
        return false;
    }

    // Get the entity context that the first dragged entity is attached to
    AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
    EBUS_EVENT_ID_RESULT(contextId, entityIdListContainer.m_entityIds[0], AzFramework::EntityIdContextQueryBus, GetOwningContextId);
    if (contextId.IsNull())
    {
        return false;
    }

    // Check that the entity context is the UI editor entity context
    UiEditorEntityContext* editorEntityContext = m_editorWindow->GetEntityContext();
    if (!editorEntityContext || (editorEntityContext->GetContextId() != contextId))
    {
        return false;
    }

    return true;
}

void HierarchyWidget::mouseMoveEvent(QMouseEvent* ev)
{
    HierarchyItem* itemBeingHovered = dynamic_cast<HierarchyItem*>(itemAt(ev->pos()));
    if (itemBeingHovered)
    {
        // Hovering.

        if (m_itemBeingHovered)
        {
            if (itemBeingHovered == m_itemBeingHovered)
            {
                // Still hovering over the same item.
                // Nothing to do.
            }
            else
            {
                // Hover start over a different item.

                // Hover ends over the previous item.
                m_itemBeingHovered->SetMouseIsHovering(false);

                // Hover starts over the current item.
                m_itemBeingHovered = itemBeingHovered;
                m_itemBeingHovered->SetMouseIsHovering(true);
            }
        }
        else
        {
            // Hover start.
            m_itemBeingHovered = itemBeingHovered;
            m_itemBeingHovered->SetMouseIsHovering(true);
        }
    }
    else
    {
        // Not hovering.

        if (m_itemBeingHovered)
        {
            // Hover end.
            m_itemBeingHovered->SetMouseIsHovering(false);
            m_itemBeingHovered = nullptr;
        }
        else
        {
            // Still not hovering.
            // Nothing to do.
        }
    }

    QTreeWidget::mouseMoveEvent(ev);
}

void HierarchyWidget::mouseReleaseEvent(QMouseEvent* ev)
{
    if (m_selectionChangedBeforeDrag)
    {
        // Signal a selection change on the mouse release
        SignalUserSelectionHasChanged(selectedItems());
    }

    QTreeWidget::mouseReleaseEvent(ev);

    // In pick mode, the user can click on an item and drag the mouse to change the current item.
    // In this case, a click event is not sent on a mouse release, so set the current item as the
    // picked item here
    if (m_inObjectPickMode)
    {
        // If there is a current item, set that as picked
        if (currentIndex() != QModelIndex()) // check for a valid index
        {
            QTreeWidgetItem* item = itemFromIndex(currentIndex());
            if (item)
            {
                PickItem(dynamic_cast<HierarchyItem*>(item));
            }
        }
    }
}

void HierarchyWidget::leaveEvent(QEvent* ev)
{
    ClearItemBeingHovered();

    // If an item is dragged really fast outside the hierarchy, the startDrag event is called,
    // but the dragEnterEvent and dragLeaveEvent are replaced with the leaveEvent.
    // In this case, perform the dragLeaveEvent here
    if (m_inDragStartState)
    {
        if (m_selectionChangedBeforeDrag)
        {
            m_signalSelectionChange = false;

            // Set the current selection to the items that were selected before the drag
            clearSelection();
            for (auto i : m_beforeDragSelection)
            {
                i->setSelected(true);
            }

            m_signalSelectionChange = true;
        }

        m_inDragStartState = false;
    }

    QTreeWidget::leaveEvent(ev);
}

void HierarchyWidget::ClearItemBeingHovered()
{
    if (!m_itemBeingHovered)
    {
        // Nothing to do.
        return;
    }

    m_itemBeingHovered->SetMouseIsHovering(false);
    m_itemBeingHovered = nullptr;
}

void HierarchyWidget::UpdateSliceInfo()
{
    // Update the slice information (color, font, tooltip) for all elements.
    // As a simple way of going through all the HierarchyItem's we use the
    // EntityHelpers::EntityToHierarchyItemMap
    for (auto mapItem : m_entityItemMap)
    {
        mapItem.second->UpdateSliceInfo();
    }
}

void HierarchyWidget::DeleteSelectedItems()
{
    DeleteSelectedItems(selectedItems());
}

void HierarchyWidget::OnEntityPickModeStarted()
{
    setDragEnabled(false);
    m_currentItemBeforePickMode = currentIndex();
    m_selectionModeBeforePickMode = selectionMode();
    setSelectionMode(QAbstractItemView::NoSelection);
    m_editTriggersBeforePickMode = editTriggers();
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setCursor(m_editorWindow->GetEntityPickerCursor());
    m_inObjectPickMode = true;
}

void HierarchyWidget::OnEntityPickModeStopped()
{
    if (m_inObjectPickMode)
    {
        setCurrentIndex(m_currentItemBeforePickMode);
        setDragEnabled(true);
        setSelectionMode(m_selectionModeBeforePickMode);
        setEditTriggers(m_editTriggersBeforePickMode);
        setCursor(Qt::ArrowCursor);
        m_inObjectPickMode = false;
    }
}

void HierarchyWidget::EntityHighlightRequested(AZ::EntityId entityId)
{
}

void HierarchyWidget::EntityStrongHighlightRequested(AZ::EntityId entityId)
{
    // Check if this entity is in the same entity context
    if (!IsEntityInEntityContext(entityId))
    {
        return;
    }

    QTreeWidgetItem* item = HierarchyHelpers::ElementToItem(this, entityId, false);
    if (!item)
    {
        return;
    }

    // Scrolling to the entity will make sure that it is visible.
    // This will automatically open parents
    scrollToItem(item);

    // Select the entity
    SetUniqueSelectionHighlight(item);
}

void HierarchyWidget::PickItem(HierarchyItem* item)
{
    const AZ::EntityId entityId = item->GetEntityId();
    if (entityId.IsValid())
    {
        AzToolsFramework::EditorPickModeRequestBus::Broadcast(
            &AzToolsFramework::EditorPickModeRequests::PickModeSelectEntity, entityId);

        AzToolsFramework::EditorPickModeRequestBus::Broadcast(
            &AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);
    }
}

bool HierarchyWidget::IsEntityInEntityContext(AZ::EntityId entityId)
{
    AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
    EBUS_EVENT_ID_RESULT(contextId, entityId, AzFramework::EntityIdContextQueryBus, GetOwningContextId);

    if (!contextId.IsNull())
    {
        UiEditorEntityContext* editorEntityContext = m_editorWindow->GetEntityContext();
        if (editorEntityContext && editorEntityContext->GetContextId() == contextId)
        {
            return true;
        }
    }

    return false;
}

void HierarchyWidget::ToggleVisibility(HierarchyItem* hierarchyItem)
{
    bool isItemVisible = true;
    AZ::EntityId itemEntityId = hierarchyItem->GetEntityId();
    EBUS_EVENT_ID_RESULT(isItemVisible, itemEntityId, UiEditorBus, GetIsVisible);

    // There is one exception to toggling the visibility. If the clicked item has invisible ancestors,
    // then we make that item and all its ancestors visible regardless of the item's visibility

    // Make a list of items to modify
    HierarchyItemRawPtrList items;

    // Look for invisible ancestors
    AZ::EntityId parent;
    EBUS_EVENT_ID_RESULT(parent, itemEntityId, UiElementBus, GetParentEntityId);
    while (parent.IsValid())
    {
        bool isParentVisible = true;
        EBUS_EVENT_ID_RESULT(isParentVisible, parent, UiEditorBus, GetIsVisible);

        if (!isParentVisible)
        {
            items.push_back(m_entityItemMap[parent]);
        }

        AZ::EntityId newParent = parent;
        parent.SetInvalid();
        EBUS_EVENT_ID_RESULT(parent, newParent, UiElementBus, GetParentEntityId);
    }

    bool makeVisible = items.size() > 0 ? true : !isItemVisible;

    // Add the item that was clicked
    if (makeVisible != isItemVisible)
    {
        items.push_back(m_entityItemMap[itemEntityId]);
    }

    CommandHierarchyItemToggleIsVisible::Push(m_editorWindow->GetActiveStack(),
        this,
        items);
}

void HierarchyWidget::DeleteSelectedItems(const QTreeWidgetItemRawPtrQList& selectedItems)
{
    CommandHierarchyItemDelete::Push(m_editorWindow->GetActiveStack(),
        this,
        selectedItems);

    // This ensures there's no "current item".
    SetUniqueSelectionHighlight((QTreeWidgetItem*)nullptr);

    // IMPORTANT: This is necessary to indirectly trigger detach()
    // in the PropertiesWidget.
    SetUserSelection(nullptr);
}

void HierarchyWidget::Cut()
{
    QTreeWidgetItemRawPtrQList selection = selectedItems();

    HierarchyClipboard::CopySelectedItemsToClipboard(this,
        selection);
    DeleteSelectedItems(selection);
}

void HierarchyWidget::Copy()
{
    HierarchyClipboard::CopySelectedItemsToClipboard(this,
        selectedItems());
}

void HierarchyWidget::PasteAsSibling()
{
    HierarchyClipboard::CreateElementsFromClipboard(this,
        selectedItems(),
        false);
}

void HierarchyWidget::PasteAsChild()
{
    HierarchyClipboard::CreateElementsFromClipboard(this,
        selectedItems(),
        true);
}

void HierarchyWidget::SetEditorOnlyForSelectedItems(bool editorOnly)
{
    QTreeWidgetItemRawPtrQList selection = selectedItems();
    if (!selection.empty())
    {
        SerializeHelpers::SerializedEntryList preChangeState;
        HierarchyClipboard::BeginUndoableEntitiesChange(m_editorWindow, preChangeState);

        for (auto item : selection)
        {
            HierarchyItem* i = dynamic_cast<HierarchyItem*>(item);

            AzToolsFramework::EditorOnlyEntityComponentRequestBus::Event(i->GetEntityId(), &AzToolsFramework::EditorOnlyEntityComponentRequests::SetIsEditorOnlyEntity, editorOnly);

            i->UpdateEditorOnlyInfo();
        }

        HierarchyClipboard::EndUndoableEntitiesChange(m_editorWindow, "editor only selection", preChangeState);

        emit editorOnlyStateChangedOnSelectedElements();
    }
}

void HierarchyWidget::AddElement(const QTreeWidgetItemRawPtrQList& selectedItems, const QPoint* optionalPos)
{
    CommandHierarchyItemCreate::Push(m_editorWindow->GetActiveStack(),
        this,
        selectedItems,
        [optionalPos](AZ::Entity* element)
        {
            if (optionalPos)
            {
                EntityHelpers::MoveElementToGlobalPosition(element, *optionalPos);
            }
        });
}

void HierarchyWidget::SetUniqueSelectionHighlight(QTreeWidgetItem* item)
{
    // Stop object pick mode when an action explicitly wants to set the hierarchy's selected items
    AzToolsFramework::EditorPickModeRequestBus::Broadcast(
        &AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);

    clearSelection();

    setCurrentIndex(indexFromItem(item));
}

void HierarchyWidget::SetUniqueSelectionHighlight(AZ::Entity* element)
{
    SetUniqueSelectionHighlight(HierarchyHelpers::ElementToItem(this, element, false));
}

#include <HierarchyWidget.moc>
