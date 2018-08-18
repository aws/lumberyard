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

#include <QPainter>

#include "LibraryTreeView.h"
#include "LibraryTreeViewItem.h"

#include <DockableLibraryTreeView.h>
#include <DockableLibraryPanel.h>
#include <BaseLibrary.h>
#include <BaseLibraryItem.h>
#include <QIcon>
#include <QEvent>
#include <Include/ILogFile.h>
#include "EditorCoreAPI.h"
#include "IEditor.h"
#include "Utils.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QScrollBar>
#include <QDrag>
#include <QDebug>
#include <QSize>
#include <QApplication>
#include "ContextMenu.h"
//Editor
#include <EditorDefs.h>
#include <BaseLibraryManager.h>
#include <IEditorParticleManager.h>
#include <EditorUI_QTDLLBus.h>

#define QTUI_INVALIAD_ITEMNAME "@"
#define QTUI_UNPARENT_ITEMNAME "@UnparentItem"

//////////////////////////////////////////////////////////////////////////////////////////////////
QTreeWidgetItem* findTreeViewItem(const char* name, QTreeWidgetItem* root)
{
    QString nme = name;
    QTreeWidgetItem* found = nullptr;

    int ccount = root->childCount();
    for (int c = 0; c < ccount; c++)
    {
        QTreeWidgetItem* group = (QTreeWidgetItem*)root->child(c);
        if (nme == group->text(LIBRARY_TREEVIEW_NAME_COLUMN))
        {
            found = group;
            break;
        }
    }

    return found;
}

void sortTreeRecurse(QTreeWidgetItem* root)
{
    if (!root)
    {
        return;
    }

    root->sortChildren(LIBRARY_TREEVIEW_NAME_COLUMN, Qt::SortOrder::AscendingOrder);

    int ccount = root->childCount();
    for (int c = 0; c < ccount; c++)
    {
        sortTreeRecurse(root->child(c));
    }
}

typedef QStringList TStringVec;
void splitPath(const QString& name, TStringVec& path)
{
    QString n = name.toLower();
    int pos = n.lastIndexOf('.');
    while (pos >= 0)
    {
        QString part = n.mid(pos + 1);
        n = n.left(pos);
        if (!part.isEmpty()) 
        {
             path.push_back(part);
        }
        pos = n.lastIndexOf('.');
    }
    if (!n.isEmpty())
    {
        path.push_back(n);
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////

CLibraryTreeView::CLibraryTreeView(QWidget* parent, IDataBaseLibrary* library)
    : QTreeWidget(parent)
    , m_baseLibrary(library)
    , m_allowSelection(true)
    , m_isInDrop(false)
    , m_iconFolderClosed(nullptr)
    , m_iconFolderOpen(nullptr)
    , m_amSettingCheckboxStates(false)
    , m_filter("")
{
    //select a single object for now ...
    setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setHeaderHidden(true);

    // connect events
    connect(this, &QTreeWidget::currentItemChanged, this, &CLibraryTreeView::OnCurrentItemChanged);
    connect(this, &QTreeWidget::itemSelectionChanged, this, &CLibraryTreeView::OnSelectionChanged);
    connect(model(), &QAbstractItemModel::rowsInserted, this, &CLibraryTreeView::OnRowInserted);
    connect(model(), &QAbstractItemModel::rowsRemoved, this, &CLibraryTreeView::refreshActiveState);
    connect(this, &QTreeWidget::itemExpanded, this, &CLibraryTreeView::refreshActiveState);
    connect(this, &QTreeWidget::itemCollapsed, this, &CLibraryTreeView::refreshActiveState);
    connect(itemDelegate(), &QAbstractItemDelegate::closeEditor, this, &CLibraryTreeView::EndRename);
    connect(this, &QTreeWidget::itemChanged, this, &CLibraryTreeView::OnItemChanged);
    setStyle(new dropIndicatorStyle(style()));
    m_iconFolderClosed = new QIcon(":/particleQT/icons/folderclosed.png");
    m_iconFolderOpen = new QIcon(":/particleQT/icons/openfolder.png");
    m_iconShowLOD = new QIcon(":/particleQT/icons/lod_icon.png");
    m_iconIsGroup = new QIcon(":/particleQT/icons/group_icon.png");
    m_iconGroupWithLOD = new QIcon(":/particleQT/icons/group_with_lod_icon.png");
    m_iconEmpty = new QIcon(":/particleQT/icons/empty_icon.png");

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMouseTracking(true);
}

CLibraryTreeView::~CLibraryTreeView()
{
    m_baseLibrary = nullptr;
}

void CLibraryTreeView::enableReordering()
{
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDragDropOverwriteMode(false);
}

void CLibraryTreeView::fillFromLibrary(bool alphaSort /*= false*/)
{
    setReloading(true);
    CRY_ASSERT(m_baseLibrary);
    QString libraryName = QString(m_baseLibrary->GetName());
    QString expandData = storeExpandState();
    QTreeWidget::clear();
    m_activeItems.clear();

    m_nameToNode.clear();
    m_nameToNode[""] = invisibleRootItem(); // groupless connects to invisibleRootItem()
    this->setColumnCount(2);
    this->setAllColumnsShowFocus(false);
    //Build the nodes first
    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(m_baseLibrary->GetManager());
    int count = m_baseLibrary->GetItemCount();
    for (int i = 0; i < count; i++)
    {
        CBaseLibraryItem* item = (CBaseLibraryItem*)m_baseLibrary->GetItem(i);

        QString qname = item->GetName();
        if (m_nameToNode[qname] == nullptr)
        {
            //The order of the groups is parent.child.child so sub 0 will always be the highest parent.
            QStringList splitGroups = qname.split(".");
            int gnum = splitGroups.length();
            QString accumulatedGroup = "";
            for (int g = 0; g < gnum; g++)
            {
                QString parent = accumulatedGroup;
                QString splitName = splitGroups[g];
                if (g == 0)
                {
                    accumulatedGroup = splitName;
                }
                else
                {
                    accumulatedGroup += "." + splitName;
                }
                if (m_nameToNode[accumulatedGroup] == nullptr)
                {
                    CRY_ASSERT(m_nameToNode[parent]);

                    //if the item does not exist add it
                    CBaseLibraryItem* item = static_cast<CBaseLibraryItem*>(mngr->FindItemByName(QString(libraryName + "." + accumulatedGroup)));
                    if (!item)
                    {
                        item = static_cast<CBaseLibraryItem*>(mngr->CreateItem(m_baseLibrary));
                        item->SetName(accumulatedGroup.toUtf8().data());
                        item->IsParticleItem = false; //items that exist in path but not in reality are folders
                    }
                    else if (item->IsParticleItem == -1)
                    {
                        item->IsParticleItem = true;
                    }

                    CLibraryTreeViewItem* child = new CLibraryTreeViewItem(m_nameToNode[parent], mngr, item->GetFullName(), LIBRARY_TREEVIEW_NAME_COLUMN, LIBRARY_TREEVIEW_INDICATOR_COLUMN);
                    child->SetItem(item);
                    child->SetName(accumulatedGroup, false);
                    child->SetLibraryName(libraryName);
                    m_nameToNode[accumulatedGroup] = child; //add as possible parent in the future.
                }
            }
        }
    }

    //Create empty item. This is to add empty space in the bottom of library tree. 
    CLibraryTreeViewItem* child = new CLibraryTreeViewItem(m_nameToNode[""], mngr, QTUI_UNPARENT_ITEMNAME, LIBRARY_TREEVIEW_NAME_COLUMN, LIBRARY_TREEVIEW_INDICATOR_COLUMN);
    child->SetEnabled(false); 
    //make this item unselecteable
    child->setFlags(Qt::NoItemFlags);
    child->SetItem(nullptr);
    child->SetName("", false); // set display name
    child->FromString(QTUI_UNPARENT_ITEMNAME); //set lookup name
    child->SetLibraryName(libraryName);
    child->SetVirtual(true);
    m_nameToNode[libraryName + QTUI_UNPARENT_ITEMNAME] = child;

    if (alphaSort)
    {
        sortTreeRecurse(invisibleRootItem());
    }

    //signal that the tree has been filled so any library specific styling can be done
    emit SignalTreeFilledFromLibrary(m_baseLibrary, this);

    //moves the name column to the left of the indicator column
    if (header()->visualIndex(LIBRARY_TREEVIEW_INDICATOR_COLUMN) > header()->visualIndex(LIBRARY_TREEVIEW_NAME_COLUMN))
    {
        header()->moveSection(LIBRARY_TREEVIEW_INDICATOR_COLUMN, LIBRARY_TREEVIEW_NAME_COLUMN);
    }
    header()->setStretchLastSection(true);
    ApplyFilter(m_filter);
    setReloading(false);
    //set up indicators on non-virtual items
    for (auto iter = m_nameToNode.begin(); iter != m_nameToNode.end(); iter++)
    {
        if ((*iter) == invisibleRootItem())
        {
            continue;
        }
        CLibraryTreeViewItem* child = static_cast<CLibraryTreeViewItem*>(*iter);
        if (child)
        {
            if (!child->IsVirtualItem())
            {
                child->setCheckState(LIBRARY_TREEVIEW_INDICATOR_COLUMN, child->GetItem()->GetIsEnabled() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
                emit SignalItemEnableStateChanged(child->GetItem(), child->checkState(LIBRARY_TREEVIEW_INDICATOR_COLUMN));
            }
        }
    }
    refreshActiveState();
    restoreExpandState(expandData);
    clearSelection();
}

void CLibraryTreeView::refreshActiveState()
{
    //first element is "invisibleRootItem"
    if (invisibleRootItem()->childCount() == 0)
    {
        return; //nothing to style
    }
    //selection is not allowed while library is in flux, best not to proceed
    if (!m_allowSelection)
    {
        return;
    }
    //expand all to ensure column width for checkboxes exists when loading libraries
    //block signals
    {
        blockSignals(true);
        QString restoreState = storeExpandState();
        //children will always be longer then parent
        int lastStringComp = 0;
        //expand to first leaf
        for (auto iter = m_nameToNode.begin(); iter != m_nameToNode.end(); iter++)
        {
            if ((*iter) == invisibleRootItem() || !(*iter))
            {
                continue;
            }
            (*iter)->setExpanded(true);
            int nextStringLength = iter.key().length();
            if (nextStringLength < lastStringComp)
            {
                break; //we're no longer walking down a path, bail
            }
            lastStringComp = nextStringLength;
        }
        resizeColumnToContents(LIBRARY_TREEVIEW_INDICATOR_COLUMN);
        restoreExpandState(restoreState);
        blockSignals(false);
    }
    if (invisibleRootItem()->childCount() <= 1) //One empty tree item for each library tree
    {
        emit SignalTreeViewEmpty();
    }

    QtRecurseAll(invisibleRootItem(), [&](QTreeWidgetItem* item)
        {
            if (item == invisibleRootItem())
            {
                return; // skip root
            }
            auto node = (CLibraryTreeViewItem*)item;

            //to use item delegates we need an index
            UpdateItemStyle(node);
        });
    if (parentWidget())
    {
        parentWidget()->resize(parentWidget()->size()); //fix sizing for dockablelibrarytreeviews without breaking most other possible parents
    }
}

// Helper functions to deal with widget state restoration
static QString getItemPath(QTreeWidgetItem* item)
{
    QString ret;
    while (item)
    {
        ret = ret + "." + item->text(LIBRARY_TREEVIEW_NAME_COLUMN);
        item = item->parent();
    }
    return ret;
}

static QString storeExpandState(QTreeWidget* tw)
{
    QString ret;
    QTreeWidgetItemIterator it(tw);
    while (*it)
    {
        QTreeWidgetItem* item = (QTreeWidgetItem*)*it;
        if (item->isExpanded())
        {
            ret += getItemPath(item) + QTUI_UTILS_NAME_SEPARATOR;
        }
        ++it;
    }
    if (ret.size() > 0)
    {
        ret.chop(2);
    }
    return ret;
}

static void restoreExpandState(QTreeWidget* tw, const QString& data)
{
    QMap<QString, bool> expanded;
    QStringList lst = data.split(QTUI_UTILS_NAME_SEPARATOR);
    for (auto it : lst)
    {
        expanded[it] = true;
    }

    QString ret;
    QTreeWidgetItemIterator it(tw);
    while (*it)
    {
        QTreeWidgetItem* item = (QTreeWidgetItem*)*it;
        item->setExpanded(expanded.contains(getItemPath(item)));
        ++it;
    }
}

QString CLibraryTreeView::storeExpandState()
{
    return ::storeExpandState(this);
}

void CLibraryTreeView::restoreExpandState(const QString& data)
{
    return ::restoreExpandState(this, data);
}

void CLibraryTreeView::OnCurrentItemChanged(QTreeWidgetItem* item, QTreeWidgetItem* previous)
{
    if (!m_allowSelection)
    {
        return;
    }
    if (item)
    {
        CLibraryTreeViewItem* treeItem = static_cast<CLibraryTreeViewItem*>(item);
        m_activeItems.clear();
        m_activeItems.push_back(treeItem);
    }
}

void CLibraryTreeView::StartRename(CLibraryTreeViewItem* item, const QString& forceNameSelected /*= ""*/, const bool overrideSafety /*= false*/)
{
    // enable editable flag
    Qt::ItemFlags currentFlags = item->flags();
    item->setFlags(currentFlags | Qt::ItemIsEditable);
    if (forceNameSelected.isEmpty())
    {
        if (!GetSelectedItems().contains(item))
        {
            SelectItem(item->GetPath());
        }
        editItem(item, LIBRARY_TREEVIEW_NAME_COLUMN);
    }
    else
    {
        bool proceed = false;
        emit SignalItemAboutToBeRenamed(item->GetItem(), item->GetPath(), forceNameSelected, proceed);
        if (overrideSafety)
        {
            RenameItem(item->GetPreviousPath(), forceNameSelected);
            return;
        }
        if (proceed)
        {
            // Note this section of code is only triggered when un-grouping emitters that were previously grouped, so the user is not providing a new item name in this case. 
            // We could just pass "" to ValidateItemName() for basically the same end result, but passing in the final token seems more appropriate.
            const QString itemName = forceNameSelected.split('.').last();

            if (ValidateItemName(itemName, forceNameSelected, item->GetItem()))
            {
                RenameItem(item->GetPreviousPath(), forceNameSelected);
                return;
            }
        }
    }
    //Disable all items, but the one we are attempting to edit.
    QtRecurseAll(invisibleRootItem(), [&](QTreeWidgetItem* item)
        {
            if (item == invisibleRootItem())
            {
                return; // skip root
            }

            if (item->flags() & Qt::ItemIsEditable)
            {
                return; // skip one we want to edit.
            }
            item->setDisabled(true);
        });
}

// End rename is a special case where the undo scope start outside the function.
void CLibraryTreeView::EndRename()
{
    //this will be called when exit this function
    FunctionCallExit<AZStd::function<void()>> exitCall([&]()
        {
            EBUS_EVENT(EditorLibraryUndoRequestsBus, EndUndoBatch);
        }
    );
    
    CRY_ASSERT(m_baseLibrary);
    DockableLibraryPanel* panel = static_cast <DockableLibraryTreeView*>(parentWidget()->parentWidget())->GetLibraryPanel();
    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(GetIEditor()->GetParticleManager());

    CLibraryTreeViewItem* treeItem = GetFirstSelectedItem();
    if (treeItem == nullptr)
    {
        return;
    }
    QString nextName = treeItem->GetPath();
    QString oldName = treeItem->GetPreviousPath();
    //enable all items
    QtRecurseAll(invisibleRootItem(), [&](QTreeWidgetItem* item)
        {
            //skip the root and empty item
            if (item == invisibleRootItem() || item->flags() == Qt::NoItemFlags)
            {
                return; // skip root
            }
            item->setDisabled(false);
        });


    bool proceed = true;
    emit SignalItemAboutToBeRenamed(treeItem->GetItem(), oldName, nextName, proceed);
    if (proceed)
    {
        if (ValidateItemName(QString(treeItem->text(LIBRARY_TREEVIEW_NAME_COLUMN)), nextName, treeItem->GetItem()))
        {
            EditorUIPlugin::ScopedLibraryModifyUndoPtr modifyUndo;
            AZStd::string libName = treeItem->GetItem()->GetLibrary()->GetName().toUtf8().data();
            EBUS_EVENT_RESULT(modifyUndo, EditorLibraryUndoRequestsBus, AddScopedLibraryModifyUndo, libName);
            EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
            EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

            RenameItem(oldName, nextName);
            m_allowSelection = true;

            //Enable all items for editing
            fillFromLibrary();
            return;
        }
    }

    treeItem->SetName(oldName, false);
    m_allowSelection = true;

    //Enable all items for editing
    refreshActiveState();
}

QString CLibraryTreeView::getItemPath(QTreeWidgetItem* item, bool includeLibrary /*= false*/)
{
    CRY_ASSERT(m_baseLibrary);
    QString ret = "";
    while (item)
    {
        ret = item->text(LIBRARY_TREEVIEW_NAME_COLUMN) + "." + ret;
        item = item->parent();
    }
    if (includeLibrary && m_baseLibrary != nullptr)
    {
        ret = m_baseLibrary->GetName() + "." + ret;
    }
    ret.chop(1);
    return ret;
}

// Drag and drop support
bool CLibraryTreeView::event(QEvent* e)
{
    return QTreeWidget::event(e);
}

void CLibraryTreeView::startDrag(Qt::DropActions supportedActions)
{
    if (m_amSettingCheckboxStates || !m_allowSelection)
    {
        //only drag checkbox results
        return;
    }

    emit SignalItemAboutToBeDragged(GetFirstSelectedItem());

    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    mimeData->setData(MIMEType, "");
    drag->setMimeData(mimeData);
    emit SignalStartDrag(drag, supportedActions);

    //inform any dependent systems that the drag has ended
    emit SignalDragOperationFinished();
}

void CLibraryTreeView::dropEvent(QDropEvent* event)
{
    return DropMetaData(event);
}

void CLibraryTreeView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        //Ignore right click
        m_allowSelection = false;
        OnMenuRequested(event->pos());
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        int columnSelected = columnAt(event->pos().x());
        if (columnSelected == LIBRARY_TREEVIEW_INDICATOR_COLUMN)
        {
            CLibraryTreeViewItem* item = static_cast<CLibraryTreeViewItem*>(itemAt(event->pos()));
            if (item && !item->IsVirtualItem())
            {
                Qt::CheckState checked = item->checkState(LIBRARY_TREEVIEW_INDICATOR_COLUMN) == Qt::Checked ? Qt::Unchecked : Qt::Checked;
                item->setCheckState(LIBRARY_TREEVIEW_INDICATOR_COLUMN, checked); //triggers item changed
                m_amSettingCheckboxStates = true;
                m_lastCheckboxState = checked;
                return;
            }
        }
        else if (columnSelected == LIBRARY_TREEVIEW_NAME_COLUMN)
        {
            CLibraryTreeViewItem* item = static_cast<CLibraryTreeViewItem*>(itemAt(event->pos()));
            if (item && !item->IsVirtualItem())
            {
                if (currentItem() == item)
                {
                    emit SignalItemReselected(item->GetItem());
                }
            }
        }
        else // If select empty place, deselect all
        {
             clearSelection();
        }
    }
    QTreeWidget::mousePressEvent(event);
}

void CLibraryTreeView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_amSettingCheckboxStates)
    {
        int columnSelected = columnAt(event->pos().x());
        if (columnSelected == LIBRARY_TREEVIEW_INDICATOR_COLUMN)
        {
            CLibraryTreeViewItem* item = static_cast<CLibraryTreeViewItem*>(itemAt(event->pos()));
            if (item && !item->IsVirtualItem() && item->checkState(LIBRARY_TREEVIEW_INDICATOR_COLUMN) != m_lastCheckboxState)
            {
                item->setCheckState(LIBRARY_TREEVIEW_INDICATOR_COLUMN, m_lastCheckboxState);
            }
        }
    }
    QTreeWidget::mouseMoveEvent(event);
}

void CLibraryTreeView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        //Ignore right click
    }
    else if (m_amSettingCheckboxStates)
    {
        //ignore signal to prevent recalling item changed
        m_amSettingCheckboxStates = false;
    }
    else
    {
        QTreeWidget::mouseReleaseEvent(event);
    }
}

void CLibraryTreeView::leaveEvent(QEvent* event)
{
    return QTreeWidget::leaveEvent(event);
}

QModelIndex CLibraryTreeView::getIndexFromItem(QTreeWidgetItem* item)
{
    return indexFromItem(item);
}

CLibraryTreeViewItem* CLibraryTreeView::getItemFromIndex(const QModelIndex& item)
{
    return static_cast<CLibraryTreeViewItem*>(itemFromIndex(item));
}

CLibraryTreeViewItem* CLibraryTreeView::getItemFromName(const QString& name)
{
    if (name.isEmpty())
    {
        return nullptr;
    }
    if (m_nameToNode.contains(name))
    {
        return static_cast<CLibraryTreeViewItem*>(m_nameToNode[name]);
    }
    else if (m_nameToNode.contains(GetLibName() + "." + name))
    {
        return static_cast<CLibraryTreeViewItem*>(m_nameToNode[GetLibName() + "." + name]);
    }
    else
    {
        return nullptr;
    }
}

void CLibraryTreeView::UpdateColors(const QColor& enabledColor, const QColor& disabledColor)
{
    m_enabledItemTextColor = enabledColor;
    m_disabledItemTextColor = disabledColor;
}

void CLibraryTreeView::ExpandItem(CLibraryTreeViewItem* item)
{
    for (unsigned int i = 0; i < item->childCount(); i++)
    {
        CLibraryTreeViewItem* child = static_cast<CLibraryTreeViewItem*>(item->child(i));
        expand(getIndexFromItem(child));
    }
    expand(getIndexFromItem(item));
}

void CLibraryTreeView::CollapseItem(CLibraryTreeViewItem* item)
{
    for (unsigned int i = 0; i < item->childCount(); i++)
    {
        CLibraryTreeViewItem* child = static_cast<CLibraryTreeViewItem*>(item->child(i));
        collapse(getIndexFromItem(child));
    }
    collapse(getIndexFromItem(item));
}

bool CLibraryTreeView::RenameItem(const QString& oldName, const QString& nextName)
{
    CLibraryTreeViewItem* libItem = static_cast<CLibraryTreeViewItem*>(getItemFromName(oldName));
    CRY_ASSERT(libItem);
    CBaseLibraryItem* renamedItem = libItem->GetItem();
    bool proceed = true;

    EBUS_EVENT(LibraryItemCacheRequestsBus, PurgeItemCache, renamedItem->GetFullName().toUtf8().data());

    if (renamedItem)
    {
        //we signal first so the parents get updated before the children
        //in cases of inheritance this prevent children from becoming orphans
        emit SignalItemRenamed(renamedItem, oldName, nextName);
    }
    libItem->SetName(nextName, true);
    renamedItem->SetName(nextName.toUtf8().data());
    renamedItem->SetModified(true);

    EBUS_EVENT(LibraryItemCacheRequestsBus, UpdateItemCache, renamedItem->GetFullName().toUtf8().data());

    m_nameToNode.insert(nextName, libItem);
    m_nameToNode.remove(oldName);
    bool childrenSucceeded = true;
    //update children
    for (unsigned int i = 0; i < libItem->childCount(); i++)
    {
        CLibraryTreeViewItem* child = static_cast<CLibraryTreeViewItem*>(libItem->child(i));
        QString childName = child->text(LIBRARY_TREEVIEW_NAME_COLUMN);
        if (!RenameItem(oldName + "." + childName, nextName + "." + childName))
        {
            childrenSucceeded = false;
        }
    }

    if (!childrenSucceeded)
    {
        //revert to old name and return false
        RenameItem(nextName, oldName);
        return false;
    }
    return true;
}

CLibraryTreeViewItem* CLibraryTreeView::GetFirstSelectedItem()
{
    if (m_activeItems.count() > 0)
    {
        return m_activeItems.first();
    }
    return nullptr;
}

QVector<CLibraryTreeViewItem*> CLibraryTreeView::GetSelectedItems()
{
    return m_activeItems;
}

void CLibraryTreeView::OnSelectionChanged()
{
    if (m_amSettingCheckboxStates)
    {
        //clear the selection
        clearSelection();
    }
    if (!m_allowSelection)
    {
        return;
    }
    m_activeItems.clear();

    QList<QTreeWidgetItem*> _items = selectedItems();
    QList<QTreeWidgetItem*>::iterator _itr = _items.begin();
    while (_itr != _items.end())
    {
        if (*_itr != invisibleRootItem())
        {
            m_activeItems.push_back(static_cast<CLibraryTreeViewItem*>(*_itr));
        }
        _itr++;
    }
    if (m_activeItems.count() > 0)
    {
        QVector<CLibraryTreeViewItem*> itemSelection;
        emit SignalPopulateSelectionList(itemSelection);
        //if neither ctrl nor shift is pressed or this is the first item selected
        if ((!(QApplication::keyboardModifiers() & Qt::ControlModifier) && !(QApplication::keyboardModifiers() & Qt::ShiftModifier)) || itemSelection.count() == 1)
        {
            if (!m_activeItems.isEmpty())
            {
                emit SignalItemSelected(m_activeItems.first()->GetItem());
            }
        }
    }
}

void CLibraryTreeView::setReloading(bool val)
{
    m_allowSelection = !val;
}

void CLibraryTreeView::SelectItem(QString pathWithoutLibrary, bool forceSelection)
{
    if (pathWithoutLibrary.isEmpty())
    {
        clearSelection();
        m_activeItems.clear();
        return;
    }
    else //Handle the primary selection
    {
        clearSelection();
        if (m_nameToNode.contains(pathWithoutLibrary))
        {
            CLibraryTreeViewItem* item = static_cast<CLibraryTreeViewItem*>(m_nameToNode[pathWithoutLibrary]);
            if (forceSelection || m_allowSelection)
            {
                item->ExpandToParent();
                item->setSelected(true);
            }
            m_activeItems.clear();
            m_activeItems.push_back(item);
            emit SignalItemSelected(item->GetItem());
        }
        if (m_activeItems.count() > 0)
        {
            if (m_activeItems.first() == invisibleRootItem())
            {
                clearSelection();
                m_activeItems.clear();
            }
        }
    }
}

QString CLibraryTreeView::GetLibName()
{
    return QString(m_baseLibrary->GetName());
}

void CLibraryTreeView::SetLibrary(IDataBaseLibrary* lib, bool alphaSort /*= false*/)
{
    IDataBaseLibrary* oldLib = m_baseLibrary;
    CRY_ASSERT(lib);
    m_baseLibrary = lib;
    fillFromLibrary(false);
}

QSize CLibraryTreeView::GetSizeOfContents()
{
    if (invisibleRootItem()->childCount() == 0)
    {
        return QSize(width(), qMax(height(), invisibleRootItem()->treeWidget()->height()));
    }
    QSize totalSize = QSize(width(), 0);
    QtRecurseAll(invisibleRootItem(), [&](QTreeWidgetItem* item)
        {
            if (invisibleRootItem() == item)
            {
                return;
            }
            if (!item->isHidden())
            {
                totalSize += QSize(0, rowHeight(indexFromItem(item)));
            }
        });
    return totalSize;
}

void CLibraryTreeView::OnMenuRequested(const QPoint& pos)
{
    CLibraryTreeViewItem* item = static_cast<CLibraryTreeViewItem*>(itemAt(pos));
    if (item && !item->ToString().contains(QTUI_UNPARENT_ITEMNAME)) // Unparent Item should not pop out context menu
    {
        ContextMenu* menu = new ContextMenu(this);
        //ignore selection
        emit SignalPopulateItemContextMenu(item, menu);
        if (menu)
        {
            connect(menu, &QMenu::aboutToHide, this, [=]() {menu->deleteLater(); });
            menu->popup(mapToGlobal(pos));
        }
    }
    m_allowSelection = true;
}

bool CLibraryTreeView::IsLibraryModified()
{
    return m_baseLibrary->IsModified();
}

bool CLibraryTreeView::ValidateItemName(const QString& itemName, const QString& itemPath, const CBaseLibraryItem* item)
{
    // Note some of the following checks may be redundant, since this function originally only checked
    // itemPath; itemName was added later without modifying any of the original checks.

    if (itemPath.contains(".."))
    {
        return false; // empty groups detected in group name
    }

    if (itemPath.length() > 0 && itemPath.at(itemPath.length() - 1) == '.')
    {
        return false; // name ends in a dot, this would create an empty group
    }

    bool itemNameContainsInvalidChars = itemName.contains('.'); // '.' is used as the delimiter between library folders in the saved xml.
    bool itemPathContainsInvalidChars = (static_cast<string>(itemPath.toUtf8().data())).
        find_first_not_of(VALID_LIBRARY_ITEM_CHARACTERS) != std::string::npos;

    if (itemNameContainsInvalidChars || itemPathContainsInvalidChars)
    {
        QMessageBox dlg(tr("Warning"), tr("Invalid Item Name. Symbols/glyphs are not allowed in the name.\n"),
            QMessageBox::Icon::Warning, QMessageBox::Button::Close, 0, 0, this);
        int answer = dlg.exec();
        return false;
    }

    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(m_baseLibrary->GetManager());
    if (!mngr)
    {
        return false;
    }
    QString fullname = m_baseLibrary->GetName() + "." + itemPath;
    IDataBaseItem* foundItem = mngr->FindItemByName(fullname);
    if (foundItem && foundItem != item) // "foundItem != item" allows for case-only renames
    {
        QString warning = "Invalid item name. Item \"" + itemPath + "\" already exists in the library.";
        QMessageBox* dlg = new QMessageBox(tr("Warning"), warning, QMessageBox::Icon::Warning, QMessageBox::Button::Close, 0, 0, this);
        connect(dlg, &QMessageBox::finished, this, [dlg]()
            {
                dlg->deleteLater();
            });
        dlg->open();
        return false;
    }

    return true;
}

QVector<CBaseLibraryItem*> CLibraryTreeView::GetChildrenOfItem(const QString& path)
{
    QVector<CBaseLibraryItem*> result;
    QtRecurseAll(getItemFromName(path), [&](QTreeWidgetItem* _it)
        {
            CLibraryTreeViewItem* it = static_cast<CLibraryTreeViewItem*>(_it);
            if (it->GetItem() != nullptr)
            {
                result.push_back(it->GetItem());
            }
        });
    return result;
}

QVector<CBaseLibraryItem*> CLibraryTreeView::GetSelectedItemsWithChildren()
{
    QVector<CLibraryTreeViewItem*> selectedParents = GetSelectedItems();
    QVector<CBaseLibraryItem*> resultingList;
    while (selectedParents.count() > 0)
    {
        CLibraryTreeViewItem* temp = selectedParents.takeFirst();
        QVector<CBaseLibraryItem*> children = GetChildrenOfItem(temp->GetPath());
        while (children.count() > 0)
        {
            if (!resultingList.contains(children.first()))
            {
                resultingList += children.takeFirst();
            }
            else
            {
                children.takeFirst(); //skip any items we already have
            }
        }
    }
    return resultingList;
}

QVector<CLibraryTreeViewItem*> CLibraryTreeView::GetSelectedTreeItemsWithChildren()
{
    QList<QTreeWidgetItem*> selectedParents = selectedItems();
    QMap<QString, CLibraryTreeViewItem*> resultMap;
    while (selectedParents.count() > 0)
    {
        CLibraryTreeViewItem* temp = static_cast<CLibraryTreeViewItem*>(selectedParents.takeFirst());
        QVector<CLibraryTreeViewItem*> children = GetChildrenOfTreeItem(temp->GetPath());
        while (children.count() > 0)
        {
            CLibraryTreeViewItem* child = children.takeFirst();
            if (!resultMap.contains(child->GetFullPath()))
            {
                resultMap.insert(child->GetFullPath(), child);
            }
        }
    }
    QVector<CLibraryTreeViewItem*> resultingVector;

    for (auto pair = resultMap.begin(); pair != resultMap.end(); pair++)
    {
        resultingVector.append((*pair));
    }

    return resultingVector;
}

QVector<CLibraryTreeViewItem*> CLibraryTreeView::GetChildrenOfTreeItem(const QString& path)
{
    QVector<CLibraryTreeViewItem*> result;
    QtRecurseAll(getItemFromName(path), [&](QTreeWidgetItem* _it)
        {
            if (_it == invisibleRootItem())
            {
                return;
            }
            CLibraryTreeViewItem* it = static_cast<CLibraryTreeViewItem*>(_it);
            if (it->GetItem() != nullptr)
            {
                result.push_back(it);
            }
        });
    return result;
}

void CLibraryTreeView::OnRowInserted(const QModelIndex& parentIndex, int first, int last)
{
    //if we are not in a drop this equates to a simple refreshActiveState
    if (!m_isInDrop)
    {
        refreshActiveState();
        return;
    }

    //handle the update for all selected items
    for (unsigned int i = first; i <= last; i++)
    {
        QModelIndex index;
        if (parentIndex.row() == -1) // if model index parent is root, ask tree view directly for the correct subindex
        {
            index = model()->index(i, 0);
        }
        else                 // we are in a valid child
        {
            index = parentIndex.child(i, 0);
        }

        // get the QTreeWidgetItem of the index of the root element that has been moved
        QTreeWidgetItem* item = getItemFromIndex(index);
        CLibraryTreeViewItem* libItem = static_cast<CLibraryTreeViewItem*>(item);

        // do the rename
        RenameItem(libItem->GetPreviousPath(), libItem->GetPath());
    }
}

bool CLibraryTreeView::ApplyFilter(const QString& filter)
{
    //hide everything unless filter is empty
    bool startHidden = !filter.isEmpty();
    QtRecurseAll(invisibleRootItem(), [=](QTreeWidgetItem* item)
        {
            if (item == invisibleRootItem())
            {
                return;
            }
            item->setHidden(startHidden);
        });
    //block signals to avoid excessive calculations
    blockSignals(true);
    if (!startHidden)
    {
        //collapse everything, then expand to the selected items
        collapseAll();
        if (m_allowSelection)
        {
            QVector<CLibraryTreeViewItem*> selection = GetSelectedItems();
            while (selection.count() > 0)
            {
                selection.takeFirst()->ExpandToParent();
            }
        }
        blockSignals(false);
        refreshActiveState(); //ensure our state is fresh
        return true;
    }

    //then unhide everything depending on leaf being shown
    QtRecurseLeavesOnly(invisibleRootItem(), [=](QTreeWidgetItem* item)
        {
            if (item == invisibleRootItem())
            {
                return;
            }
            CLibraryTreeViewItem* node = static_cast<CLibraryTreeViewItem*>(item);
            if (node->ToString().contains(filter, Qt::CaseInsensitive))
            {
                node->ExpandToParent(true);
            }
        });

    blockSignals(false);
    refreshActiveState(); //ensure our state is fresh
    m_filter = filter;

    return QtRecurseAnyMatch<QTreeWidgetItem*>(invisibleRootItem(), [=](QTreeWidgetItem* item) -> bool
        {
            if (item == invisibleRootItem())
            {
                return false;
            }
            if (!item->isHidden())
            {
                return true;
            }
            return false;
        }) || GetLibName().contains(m_filter, Qt::CaseInsensitive);
}

void CLibraryTreeView::SetAllItemsEnabled(const bool& enabled /*= true*/)
{
    QtRecurseAll(invisibleRootItem(), [=](QTreeWidgetItem* item)
        {
            if (item == invisibleRootItem())
            {
                return;
            }
            CLibraryTreeViewItem* node = static_cast<CLibraryTreeViewItem*>(item);
            if (node && !node->IsVirtualItem())
            {
                node->SetEnabled(enabled);
                node->setCheckState(LIBRARY_TREEVIEW_INDICATOR_COLUMN, enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
            }
        });
    refreshActiveState();
}

void CLibraryTreeView::OnItemChanged(QTreeWidgetItem* item, int column)
{
    if (column == LIBRARY_TREEVIEW_INDICATOR_COLUMN)
    {
        CLibraryTreeViewItem* node = static_cast<CLibraryTreeViewItem*>(item);
        if (!node->IsVirtualItem())
        {
            node->SetEnabled(item->checkState(column) == Qt::Checked);
            emit SignalItemEnableStateChanged(node->GetItem(), node->IsEnabled());
            UpdateItemStyle(node);
        }
    }
}

void CLibraryTreeView::UpdateItemStyle(CLibraryTreeViewItem* item)
{
    if (!item->IsVirtualItem())
    {
        item->setForeground(LIBRARY_TREEVIEW_NAME_COLUMN, item->IsEnabled() ? m_enabledItemTextColor : m_disabledItemTextColor);
        bool hasLod = false;
        emit SignalCheckLod(item->GetItem(), hasLod);
    }
    else
    {
        if (!item->ToString().contains(QTUI_UNPARENT_ITEMNAME))
        {
            item->setIcon(LIBRARY_TREEVIEW_NAME_COLUMN, (item->childCount()> 0 && item->isExpanded()) ? *m_iconFolderOpen : *m_iconFolderClosed);
        }
    }
    // With adding new lod and group icon, double the icon width to 32 to hold two icons
    setIconSize(QSize(32, 16));
}

void CLibraryTreeView::StoreSelectionData()
{
    m_cachedSelectonData.clear();
    for (unsigned int i = 0; i < m_activeItems.count(); i++)
    {
        m_cachedSelectonData += m_activeItems[i]->GetPath();
        if (i < m_activeItems.count() - 1)
        {
            //don't add marker to end
            m_cachedSelectonData += "|";
        }
    }
}

void CLibraryTreeView::RestoreSelectionData()
{
    QStringList selection = m_cachedSelectonData.split("|");
    while (selection.count() > 0)
    {
        SelectItem(selection.takeFirst());
    }
}

QString CLibraryTreeView::GetPathFromItemFullName(const QString& name)
{
    QStringList nameList = name.split('.');
    if (nameList.first().compare(m_baseLibrary->GetName()) == 0)
    {
        nameList.takeFirst();
    }
    return nameList.join('.');
}

QString CLibraryTreeView::GetReverseMatchedStrings(const QString& a, const QString& b)
{
    QString ra = a;
    std::reverse(ra.begin(), ra.end());
    QString rb = b;
    std::reverse(rb.begin(), rb.end());
    QString result = "";
    int shortestLength = a.length() < b.length() ? a.length() : b.length();
    for (int i = 0; i < shortestLength; i++)
    {
        if (ra[i] != rb[i])
        {
            std::reverse(result.begin(), result.end());
            return result;
        }
        result += ra[i];
    }
    std::reverse(result.begin(), result.end());
    return result;
}

QStringList CLibraryTreeView::mimeTypes() const
{
    QStringList result = QTreeWidget::mimeTypes();
    result.append("LibraryTreeData");
    return result;
}

void CLibraryTreeView::DropMetaData(QDropEvent* event, bool dropToLocation /*= true*/)
{
    QModelIndex droppedIndex = indexAt(event->pos());
    QString previousParent = QTUI_INVALIAD_ITEMNAME;
    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(m_baseLibrary->GetManager());
    if (!mngr)
    {
        return;
    }
    //parse drop data to build a selectedItems list
    const QMimeData* mimeData = event->mimeData();
    QString data = mimeData->data("LibraryTreeData");
    QStringList perItemData = data.split(QTUI_UTILS_NAME_SEPARATOR);
    QVector<CBaseLibraryItem*> _items;
    QVector<CBaseLibraryItem*> _newItems;

    //This vector used to perform a rename to move the items to the correct location.
    //It holds the top level items of those about to be dropped.
    //ex: items about to be dropped: P1.child1 && P1 && P3, then the parents vector will hold P1,P3.
    QVector<QString> parents;
    QVector<QString> extraFolders; //used to remove extra folders left over from moving the emitters
    QString lastParent = QTUI_UTILS_NAME_SEPARATOR; //the removed portion of the data, should be safe to use

    for (unsigned int i = 0; i < perItemData.count(); i++)
    {
        _items.append(static_cast<CBaseLibraryItem*>(mngr->FindItemByName(perItemData[i])));
        if (perItemData[i].contains(lastParent + ".") == 0)
        {
            lastParent = perItemData[i];
            parents.push_back(lastParent);
        }
    }

    bool isReorder = false;
    CLibraryTreeViewItem* dropTreeViewItem = dropToLocation ? (CLibraryTreeViewItem*)getItemFromIndex(droppedIndex) : NULL;
    CBaseLibraryItem* dropLibraryItem = dropTreeViewItem ? dropTreeViewItem->GetItem() : NULL;
    QString dropLibraryItemName = dropLibraryItem ? dropLibraryItem->GetFullName() : m_baseLibrary->GetName();

    CBaseLibraryItem* neighbour = dropLibraryItem; // used to reorder
    DropIndicatorPosition dropPosition = dropIndicatorPosition();
    if (dropToLocation) //if drop to location
    {
        if (dropPosition != QAbstractItemView::OnItem)
        {
            isReorder = true;
            // Drop location move up one level if drop below/above selected item
            dropTreeViewItem = static_cast<CLibraryTreeViewItem*>(dropTreeViewItem->parent());
            dropLibraryItem = dropTreeViewItem ? dropTreeViewItem->GetItem() : NULL;
            dropLibraryItemName = dropLibraryItem ? dropLibraryItem->GetFullName() : m_baseLibrary->GetName();
        }
    }

    // Validate Drop
    QString errorMsg;
    QVector<CBaseLibraryItem*>::iterator _itr = _items.begin();
    QHash<CBaseLibraryItem*, QString> itemFinalNames;
    QVector<CBaseLibraryItem*> ignoredItems;
    QString ignoredParentName = QTUI_INVALIAD_ITEMNAME;

    while (_itr != _items.end())
    {
        if (!(*_itr))
        {
            _itr++;
            continue;
        }
        if (dropToLocation && dropLibraryItem) // if drop to tree item
        {
            // Check if a folder get dropped under a particle
            if (!dropTreeViewItem->IsVirtualItem() && (*_itr) && !(*_itr)->IsParticleItem)
            {
                errorMsg.push_back(QString("Folder is not allowed to drag under an emitter \'%2\'.\n").arg(dropLibraryItemName));
                _itr++;
                continue;
            }

            // Prevent user drag a parent item into a child if they are from the same library
            if ((*_itr)->GetLibrary() == m_baseLibrary)
            {
                CLibraryTreeViewItem* dragTreeItem = getItemFromName((*_itr)->GetName());
                if (dragTreeItem)
                {
                    if (dragTreeItem == dropTreeViewItem)
                    {
                        errorMsg.push_back(QString("Parent item \'%1\' is not allowed to drag under child item \'%2\'.\n").arg(dragTreeItem->GetFullPath(), dropLibraryItemName));
                        _itr++;
                        continue;
                    }

                    bool shouldContinue = false;
                    for (int index = 0; index < dragTreeItem->childCount(); index++)
                    {
                        CLibraryTreeViewItem* child = static_cast<CLibraryTreeViewItem*>(dragTreeItem->child(index));
                        if (child == dropTreeViewItem)
                        {
                            errorMsg.push_back(QString("Parent item \'%1\' is not allowed to drag under child item \'%2\'.\n").arg(dragTreeItem->GetFullPath(), dropLibraryItemName));
                            _itr++;
                            shouldContinue = true;
                            break;
                        }
                    }
                    if (shouldContinue)
                    {
                        continue;
                    }
                }
            }
        }

        // Check if the item is dropped into its original position
        // If it is a child of previous ignored item, ignored it.

        QString ItemGroupName = (*_itr)->GetLibrary()->GetName() + "." + (*_itr)->GetGroupName(); // Item Lib Name + Item Group Name
        if (ItemGroupName.contains(ignoredParentName))
        {
            ignoredItems.push_back(*_itr);
            if (isReorder) // Store this item for reording later.
            {
                _newItems.push_back(*_itr);
            }
            _itr++;
            continue;
        }
        if (dropLibraryItem)
        {
            if (ItemGroupName == QString(dropLibraryItem->GetFullName()))
            {
                ignoredItems.push_back(*_itr);
                ignoredParentName = (*_itr)->GetFullName();
                if (isReorder) // Store this item for reording later.
                {
                    _newItems.push_back(*_itr);
                }
                _itr++;
                continue;
            }
        }
        else
        {
            QStringList nameList = QString((*_itr)->GetFullName()).split(".");
            QString libName = nameList.takeFirst();
            // Drop a top level item to it's own library
            if ((*_itr)->GetGroupName().isEmpty() && libName == m_baseLibrary->GetName())
            {
                ignoredItems.push_back(*_itr);
                ignoredParentName = (*_itr)->GetFullName();
                if (isReorder) // Store this item for reording later.
                {
                    _newItems.push_back(*_itr);
                }
                _itr++;
                continue;
            }
        }

        // Valiad Drop location does not have duplicate name item

        QString finalItemName = "";
        GetFinalItemName(finalItemName, (*_itr), dropLibraryItem, parents);
        if (!finalItemName.isEmpty())
        {
            if (itemFinalNames.keys(finalItemName).size() == 0)
            {
                itemFinalNames.insert((*_itr), finalItemName);
            }
            else
            {
                CBaseLibraryItem* itemInList = itemFinalNames.key(finalItemName, nullptr);
                errorMsg.push_back(QString("Cannot drop item \'%1\' to the same location as item \'%2\'. Duplicates are not allowed\n").arg(QString((*_itr)->GetFullName()),
                        QString(itemInList->GetFullName())));
                _itr++;
                continue;
            }
        }
        QString finalItemNameWithLib = QString("%1.%2").arg(QString(m_baseLibrary->GetName()), finalItemName);

        if (mngr->FindItemByName(finalItemNameWithLib))
        {
            errorMsg.push_back(QString("Item \'%1\' already exists.\n").arg(finalItemNameWithLib));
        }

        _itr++;
    }
    if (!errorMsg.isEmpty())
    {
        GetIEditor()->GetLogFile()->Warning(errorMsg.toUtf8().data());
        return;
    }

    // Remove the item should be ignored from the item list
    for (CBaseLibraryItem* ignored : ignoredItems)
    {
        for (int index = 0; index < _items.size(); index++)
        {
            if (_items[index] == ignored)
            {
                _items.remove(index);
                break;
            }
        }
    }

    DockableLibraryPanel* panel = static_cast <DockableLibraryTreeView*>(parentWidget()->parentWidget())->GetLibraryPanel();

    EditorUIPlugin::ScopedBatchUndoPtr undoBatch;
    EBUS_EVENT_RESULT(undoBatch, EditorLibraryUndoRequestsBus, AddScopedBatchUndo, "Drag drop particles");
    
    //collect all modified libraries for save undo
    AZStd::unordered_map<AZStd::string, IDataBaseLibrary*> modifiedLibs;
    for (auto itr = _items.begin(); itr != _items.end(); itr++)
    {
        CBaseLibraryItem* item = (*itr);
        if (item) 
        {
            modifiedLibs[AZStd::string(item->GetLibrary()->GetName().toUtf8().data())] = item->GetLibrary();
        }
    }
    modifiedLibs[AZStd::string(m_baseLibrary->GetName().toUtf8().data())] = m_baseLibrary;

    //add undo scope for each library
    EditorUIPlugin::ScopedLibraryModifyUndoPtr *libUndoArray = new EditorUIPlugin::ScopedLibraryModifyUndoPtr[modifiedLibs.size()];
    int i = 0; 
    for (auto libItr = modifiedLibs.begin(); libItr != modifiedLibs.end(); libItr++, i++)
    {
        EBUS_EVENT_RESULT(libUndoArray[i], EditorLibraryUndoRequestsBus, AddScopedLibraryModifyUndo, libItr->first);
    }

    //disable other undos
    EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, true);

    //add items to the library
    for (auto itr = _items.begin(); itr != _items.end(); itr++)
    {
        CBaseLibraryItem* item = (*itr);
        if (!item)
        {
            continue;
        }

        QString oldItemsName = item->GetFullName();
        // keep track of previous parent. Skip unnecessary recursive calls because of the particle hierarchy
        if (item->IsParticleItem)
        {
            if (oldItemsName.contains(previousParent + "."))
            {
                continue;
            }
            else
            {
                previousParent = oldItemsName;
            }
        }


        QString finalItemName = itemFinalNames[item];
        if (finalItemName.isEmpty())
        {
            GetFinalItemName(finalItemName, item, dropLibraryItem, parents);
        }

        QStringList finalItemNameList = finalItemName.split(".");
        finalItemNameList.takeLast();
        QString finalParentName = m_baseLibrary->GetName();
        if (!finalItemNameList.join(".").isEmpty())
        {
            finalParentName.append(".");
            finalParentName.append(finalItemNameList.join("."));
        }

        CBaseLibraryItem* placeholderItem = nullptr;
        CParticleItem* pParticle = static_cast<CParticleItem*>(item);
        if (pParticle)
        {
            // continue if it is a folder
            if (pParticle->IsParticleItem == 0)
            {
                // move folder item
                emit SignalMoveItem(finalParentName, QString(pParticle->GetFullName()), placeholderItem);
                if (placeholderItem != nullptr)
                {
                    _newItems.push_back(placeholderItem);
                }
                continue;
            }
        }


        emit SignalCopyItem(item);

        placeholderItem = reinterpret_cast<CBaseLibraryItem*>(mngr->CreateItem(m_baseLibrary));


        // temporary name to allow renaming.
        QString oldItemsNameWithoutLib = item->GetName();
        placeholderItem->SetName(QString("%1_old").arg(finalItemName).toUtf8().data());
        // Call SignalItemRenamed to fix hierarchy.
        emit SignalItemRenamed(placeholderItem, QString("%1_old").arg(finalItemName), finalItemName);
        // Fix bad name given by SignalItemRenamed.
        placeholderItem->SetName(finalItemName.toUtf8().data());
        placeholderItem->SetModified(true);

        //Update tab titles
        emit SignalUpdateTabName(oldItemsName, QString("%1.%2").arg(QString(m_baseLibrary->GetName()), finalItemName));

        //if we are dragging to a new library we can miss the paste, so fill from library before pasting
        if (!dropLibraryItem)
        {
            fillFromLibrary(false);
        }
        emit SignalPasteSelected(placeholderItem, true);
        _newItems.push_back(placeholderItem);
        if (!pParticle)
        {
            continue;
        }
        if (!pParticle->IsParticleItem)
        {
            CParticleItem* pPlaceholderParticle = static_cast<CParticleItem*>(placeholderItem);
            pPlaceholderParticle->IsParticleItem = 0; // isparticleitem== 0 means it's a folder
        }
    }
    if (dropToLocation && isReorder)
    {
        for (CBaseLibraryItem* item : _newItems)
        {
            ReorderItem(neighbour, item, dropPosition);
        }
    }
    // cleanup after performing drag action.
    while (_items.count() > 0)
    {
        CBaseLibraryItem* item = _items.takeLast();
        if (!item)
        {
            continue;
        }
        //clean up the remains
        item->SetModified(true);
        item->GetLibrary()->GetManager()->DeleteItem(item);
    }
    
    //The order in which items are saved is based on the order they appear in IParticleEffect, not their order in the Library. 
    //So anytime the Item tree structure changes in the library, we have to make sure the IParticleEffect stays in sync.
    for (auto libItr = modifiedLibs.begin(); libItr != modifiedLibs.end(); libItr++, i++)
    {
        FixParticleEffectOrder(libItr->second);
    }

    EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, false);
    delete [] libUndoArray;

    emit SignalDragOperationFinished();
}

void CLibraryTreeView::FixParticleEffectOrder(IDataBaseLibrary* lib)
{
    int count = lib->GetItemCount();
    std::unordered_map<CParticleItem*, std::vector<IParticleEffect*>> effectChildrens;

    //build orderred children list from library for each particle item
    for (int index = 0; index < count; index++)
    {
        CParticleItem* pParticle = static_cast<CParticleItem*> (lib->GetItem(index));
        if (pParticle)
        {
            if (pParticle->GetParent())
            {
                effectChildrens[pParticle->GetParent()].push_back(pParticle->GetEffect());
            }
        }
    }   

    //for each particle item's particle effect, reorder their children
    for (int index = 0; index < count; index++)
    {
        CParticleItem* pParticle = static_cast<CParticleItem*> (lib->GetItem(index));
        if (pParticle && pParticle->IsParticleItem && pParticle->GetEffect())
        {
            pParticle->GetEffect()->ReorderChildren(effectChildrens[pParticle]);
        }
    }
}

// This function could only used to reorder item with the same parent. Hierachy issue must
// be taken care before this function get called.
void CLibraryTreeView::ReorderItem(CBaseLibraryItem* neighbour, CBaseLibraryItem* item, DropIndicatorPosition position)
{
    CRY_ASSERT(position != QAbstractItemView::OnItem); // if item is dropped on item, it should not be reordered.
    int count = m_baseLibrary->GetItemCount();
    int insertIndex = -1;
    for (int index = 0; index < count; index++) // Find neighbour's index
    {
        if (neighbour == m_baseLibrary->GetItem(index))
        {
            insertIndex = index;
        }
    }

    if (position == QAbstractItemView::BelowItem) //if below neighbour, index++
    {
        insertIndex++;
    }

    m_baseLibrary->ChangeItemOrder(item, insertIndex);
}
void CLibraryTreeView::GetFinalItemName(QString& finalItemName, CBaseLibraryItem* libItem, CBaseLibraryItem* dropLibItem, const QVector<QString>& parents)
{
    QString resultingName = "";
    QString oldItemsName = libItem->GetFullName();
    //final name = drop location + (currentitem - drag item's group name)
    for (QString parent : parents)
    {
        //if either the item's parent or the item itself is in the list get the correct name.
        if (oldItemsName.contains(parent + ".") || oldItemsName == parent)
        {
            QStringList splitParent = parent.split('.');
            splitParent.takeLast();
            QString parentsGroup = splitParent.join('.');
            resultingName = oldItemsName.remove(parentsGroup + ".");
        }
    }
    if (!dropLibItem)
    {
        finalItemName = resultingName;
    }
    else
    {
        finalItemName = QString("%1.%2").arg(dropLibItem->GetName()).arg(resultingName);
    }
}


void CLibraryTreeView::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasFormat(MIMEType))
    {
        // if item does not contain a valid item, return.. (group, or something else)
        CLibraryTreeViewItem* libraryItem = GetFirstSelectedItem();
        SignalItemAboutToBeDragged(libraryItem);
        QTreeWidget::dragEnterEvent(e);
    }
}

#include <LibraryTreeView.moc>
