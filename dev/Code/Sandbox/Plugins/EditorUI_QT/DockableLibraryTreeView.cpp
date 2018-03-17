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
#include "DockableLibraryTreeView.h"

//Editor
#include <EditorDefs.h>
#include <BaseLibraryManager.h>
#include <ILogFile.h>

//Local
#include <DockableLibraryPanel.h>
#include <LibraryTreeView.h>
#include <LibraryTreeViewItem.h>
#include <DefaultViewWidget.h>
#include <ContextMenu.h>
#include "RenameableTitleBar.h"
#include "UIUtils.h"
#include "Utils.h"

//Qt
#include <QMessageBox>
#include <QVBoxLayout>
#include <QResizeEvent>

#define COLLAPSED_HEIGHT 1

DockableLibraryTreeView::DockableLibraryTreeView(QWidget* parent)
    : QDockWidget(parent)
    , m_treeView(nullptr)
    , m_titleBar(nullptr)
    , m_defaultView(nullptr)
    , m_library(nullptr)
    , m_layout(nullptr)
    , m_centralWidget(nullptr)
    , m_collapsedWidget(nullptr)
    , m_libPanel(nullptr)
{
    setObjectName("DockableLibraryTreeView");
    m_libPanel = static_cast<DockableLibraryPanel*>(parent);
    setMouseTracking(true);
}

DockableLibraryTreeView::~DockableLibraryTreeView()
{
    if (m_treeView)
    {
        delete m_treeView;
        m_treeView = nullptr;
    }
    if (m_titleBar)
    {
        delete m_titleBar;
        m_titleBar = nullptr;
    }
}

bool DockableLibraryTreeView::Init(IDataBaseLibrary* lib)
{
    CRY_ASSERT(lib);
    //create the contents
    m_library = lib;
    m_collapsedWidget = new QWidget(this);
    m_collapsedWidget->setFixedHeight(1);
    m_collapsedWidget->hide();
    m_titleBar = new RenameableTitleBar(this);
    m_centralWidget = new QWidget(this);
    m_layout = new QVBoxLayout(m_centralWidget);
    m_treeView = new CLibraryTreeView(m_centralWidget, lib);
    m_defaultView = new DefaultViewWidget(m_centralWidget);

    setTitleBarWidget(m_titleBar);
    m_titleBar->SetEditorsPlaceholderText(tr("Enter name for library"));
    m_titleBar->SetTitle(m_treeView->GetLibName());
    setContentsMargins(0, 0, 0, 0);

    
    m_centralWidget->setLayout(m_layout);
    m_layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    m_layout->addWidget(m_defaultView);
    m_layout->addWidget(m_treeView);
    m_layout->setContentsMargins(0, 0, 0, 0);
    DecorateDefaultView();
    
    //You must add the layout of the widget before you call this function; if not, the widget will not be visible.
    setWidget(m_centralWidget);

    connect(m_treeView, &CLibraryTreeView::SignalTreeViewEmpty, this, &DockableLibraryTreeView::ShowDefaultView);
    connect(m_treeView->model(), &QAbstractItemModel::rowsInserted, this, &DockableLibraryTreeView::ShowTreeView);
    connect(m_treeView, &CLibraryTreeView::SignalItemSelected, this, &DockableLibraryTreeView::PassThroughItemSelection);
    connect(m_treeView, &CLibraryTreeView::SignalItemAboutToBeRenamed, this, &DockableLibraryTreeView::PassThroughItemAboutToBeRenamed);
    connect(m_treeView, &CLibraryTreeView::SignalItemRenamed, this, &DockableLibraryTreeView::PassThroughItemRenamed);
    connect(m_treeView, &CLibraryTreeView::SignalCheckLod, this, &DockableLibraryTreeView::PassThroughItemCheckLod);
    connect(m_treeView, &CLibraryTreeView::SignalPopulateItemContextMenu, this, &DockableLibraryTreeView::PassThroughPopulateItemContextMenu);
    connect(m_treeView, &CLibraryTreeView::SignalTreeFilledFromLibrary, this, &DockableLibraryTreeView::PassThroughTreeFilledFromLibrary);
    connect(m_treeView, &CLibraryTreeView::SignalItemAboutToBeDragged, this, &DockableLibraryTreeView::PassThroughItemAboutToBeDragged);
    connect(m_treeView, &CLibraryTreeView::SignalDragOperationFinished, this, &DockableLibraryTreeView::PassThroughDragOperationFinished);
    connect(m_treeView, &CLibraryTreeView::SignalItemEnableStateChanged, this, &DockableLibraryTreeView::PassThroughItemEnableStateChanged);
    connect(m_treeView, &CLibraryTreeView::SignalPopulateSelectionList, this, &DockableLibraryTreeView::PassThroughPopulateSelectionList);
    connect(m_treeView, &CLibraryTreeView::SignalStartDrag, this, &DockableLibraryTreeView::PassThroughStartDrag);
    connect(m_treeView, &CLibraryTreeView::SignalMoveItem, this, &DockableLibraryTreeView::SignalMoveItem);
    connect(m_treeView, &CLibraryTreeView::SignalCopyItem, this, &DockableLibraryTreeView::SignalCopyItem);
    connect(m_treeView, &CLibraryTreeView::SignalPasteSelected, this, &DockableLibraryTreeView::SignalPasteItem);

    connect(m_treeView, &CLibraryTreeView::SignalUpdateTabName, this, &DockableLibraryTreeView::SignalUpdateTabName);

    connect(m_treeView, &CLibraryTreeView::itemExpanded, this, &DockableLibraryTreeView::OnContentSizeChanged);
    connect(m_treeView, &CLibraryTreeView::itemCollapsed, this, &DockableLibraryTreeView::OnContentSizeChanged);
    connect(this, &QDockWidget::windowTitleChanged, m_titleBar, &RenameableTitleBar::SetTitle);
    connect(this, &DockableLibraryTreeView::SignalShowEvent, this, &DockableLibraryTreeView::OnFirstShowing, Qt::ConnectionType::QueuedConnection); //triggers after the show event is finished
    connect(m_titleBar, &RenameableTitleBar::SignalLeftClicked, this, &DockableLibraryTreeView::ToggleCollapseState);
    connect(m_titleBar, &RenameableTitleBar::SignalRightClicked, this, &DockableLibraryTreeView::OnMenuRequested);
    connect(m_titleBar, &RenameableTitleBar::SignalTitleValidationCheck, this, &DockableLibraryTreeView::OnLibraryNameValidation);
    connect(m_titleBar, &RenameableTitleBar::SignalTitleChanged, this, &DockableLibraryTreeView::OnLibraryRename, Qt::ConnectionType::QueuedConnection);
    connect(m_defaultView, &DefaultViewWidget::SignalRightClicked, this, &DockableLibraryTreeView::OnMenuRequested);

    m_treeView->SetLibrary(lib);
    m_treeView->enableReordering();
    //uses the CLibraryTreeView's drop style
    setStyle(new dropIndicatorStyle(style()));
    setAcceptDrops(true);
    m_titleBar->setAcceptDrops(true);
    m_defaultView->setAcceptDrops(true);
    m_centralWidget->setAcceptDrops(true);

    if (m_treeView && m_titleBar && m_defaultView)
    {
        if (m_treeView->topLevelItemCount() > 0)
        {
            ShowTreeView();
        }
        else
        {
            ShowDefaultView();
        }
        return true;
    }
    else
    {
        return false;
    }

    emit SignalFocused(this);
}

void DockableLibraryTreeView::ShowDefaultView()
{
    m_treeView->hide();
    m_defaultView->show();
    OnContentSizeChanged();
}

void DockableLibraryTreeView::ShowTreeView()
{
    m_defaultView->hide();
    m_treeView->show();
    OnContentSizeChanged();
}

bool DockableLibraryTreeView::GetCollapseState()
{
    return widget()->height() == COLLAPSED_HEIGHT;
}

void DockableLibraryTreeView::ToggleCollapseState()
{
    CRY_ASSERT(GetLibrary());
    if (widget()->height() == COLLAPSED_HEIGHT)
    {
        Expand();
    }
    else
    {
        Collapse();
    }
    emit SignalFocused(this);
}

void DockableLibraryTreeView::Expand()
{
    setWidget(m_centralWidget);
    m_collapsedWidget->hide();
    m_centralWidget->show();
    update(); //force the size to take
    OnContentSizeChanged();
    m_titleBar->UpdateExpandedLibStyle(true);
}

void DockableLibraryTreeView::Collapse()
{
    setWidget(m_collapsedWidget);
    m_collapsedWidget->show();
    m_centralWidget->hide();
    update(); //force the size to take
    OnContentSizeChanged();
    m_titleBar->UpdateExpandedLibStyle(false);
}

void DockableLibraryTreeView::DecorateDefaultView()
{
    //sets up a generic default view
    m_defaultView->SetSpaceAtTop(0);
    emit SignalDecorateDefaultView(QString(m_library->GetName()), m_defaultView);
}

QString DockableLibraryTreeView::NameItemDialog(const QString& dialogTitle, const QString& prompt, const QString& preexistingPath /*= ""*/)
{
    UIUtils::FieldContainer fields;
    const UIUtils::TypeString* pParticleName = fields.AddField(new UIUtils::TypeString(prompt, preexistingPath));
    if (!UIUtils::GetMultiFieldDialog(this, dialogTitle, fields) || pParticleName->GetValue().isEmpty())
    {
        return "";
    }
    return pParticleName->GetValue();
}

void DockableLibraryTreeView::ResetSelection()
{
    m_treeView->setCurrentItem(0);
    m_treeView->SelectItem("", true);
}

void DockableLibraryTreeView::UpdateColors(const QColor& enabledColor, const QColor& disabledColor)
{
    m_treeView->UpdateColors(enabledColor, disabledColor);
    m_treeView->refreshActiveState();
}

void DockableLibraryTreeView::UpdateIconStyle(CLibraryTreeViewItem* item)
{
    m_treeView->UpdateItemStyle(item);
}

void DockableLibraryTreeView::SyncFromEngine()
{
    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

    CRY_ASSERT(m_treeView);
    QString collapseData = m_treeView->storeExpandState();
    StoreSelectionData();
    m_treeView->fillFromLibrary();
    m_treeView->restoreExpandState(collapseData);
    RestoreSelectionData();
    update();
}

//fix to make the treeview the correct size to prevent the need for scrolling when
//multiple libraries are included.
void DockableLibraryTreeView::OnContentSizeChanged()
{
    CRY_ASSERT(m_treeView);
    CRY_ASSERT(m_defaultView);
    CRY_ASSERT(m_titleBar);
    CRY_ASSERT(widget());
    CRY_ASSERT(m_collapsedWidget);
    QSize newSize = widget()->size();
    if (!m_collapsedWidget->isHidden())
    {
        newSize = m_collapsedWidget->size();
    }
    else if (!m_defaultView->isHidden())
    {
        newSize = m_defaultView->size();
    }
    else
    {
        newSize = m_treeView->GetSizeOfContents();
        newSize += QSize(0, m_treeView->contentsMargins().top() + m_treeView->contentsMargins().bottom());
        m_treeView->setFixedHeight(newSize.height()); //force the tree view to take up the available room
    }
    widget()->setFixedHeight(newSize.height());
    if (titleBarWidget())
    {
        setFixedHeight(newSize.height() + titleBarWidget()->height());
    }
    else
    {
        setFixedHeight(newSize.height());
    }
    update(); //force the size to take
    emit SignalContentResized(newSize);
}

CLibraryTreeViewItem* DockableLibraryTreeView::GetItemFromName(const QString& nameWithoutLibrary)
{
    CRY_ASSERT(m_treeView);
    return m_treeView->getItemFromName(nameWithoutLibrary);
}

void DockableLibraryTreeView::PassThroughItemSelection(CBaseLibraryItem* item)
{
    emit SignalItemSelected(item);
}

void DockableLibraryTreeView::PassThroughItemAboutToBeRenamed(CBaseLibraryItem* item, const QString& currentName, const QString& nextName, bool& proceed)
{
    emit SignalItemAboutToBeRenamed(item, currentName, nextName, proceed);
}

void DockableLibraryTreeView::PassThroughItemRenamed(CBaseLibraryItem* item, const QString& oldName, const QString& currentName, const QString newlib)
{
    emit SignalItemRenamed(item, oldName, currentName, newlib);
}

void DockableLibraryTreeView::PassThroughPopulateItemContextMenu(CLibraryTreeViewItem* focusedItem, ContextMenu* toAddTo)
{
    emit SignalPopulateItemContextMenu(focusedItem, toAddTo);
}

bool DockableLibraryTreeView::IsModified()
{
    return m_treeView->IsLibraryModified();
}

void DockableLibraryTreeView::Reload()
{
    CRY_ASSERT(m_library);
    QString file = m_library->GetFilename();
    IDataBaseManager* manager = m_library->GetManager();
    CRY_ASSERT(manager);
    IEditor* editor = GetIEditor();
    CRY_ASSERT(editor);

    QStringList selectedItemPaths;

    QVector<CLibraryTreeViewItem*> selectedItems = m_treeView->GetSelectedItems();

    for (auto iter = selectedItems.begin(); iter != selectedItems.end(); iter++)
    {
        selectedItemPaths.append((*iter)->GetPath());
    }
    // Clear selection in case the data get corrupted.
    ResetSelection();

    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);
    manager->DeleteLibrary(m_library->GetName(), true);
    m_library = nullptr;
    m_library = manager->LoadLibrary(file);
    CRY_ASSERT(m_library);
    m_treeView->SetLibrary(m_library);

    blockSignals(true);
    while (selectedItemPaths.count() > 0)
    {
        m_treeView->SelectItem(selectedItemPaths.takeFirst());
    }
    blockSignals(false);
}

void DockableLibraryTreeView::SelectItemFromName(const QString& nameWithoutLibrary, bool forceSelection)
{
    m_treeView->SelectItem(nameWithoutLibrary, forceSelection);
}

CBaseLibraryItem* DockableLibraryTreeView::AddItem(const QString& nameWithoutLibrary)
{
    QString fullName = m_library->GetName() + "." + nameWithoutLibrary;
    QStringList splitName = nameWithoutLibrary.split('.');
    QString childName = splitName.takeLast();
    QString groupName = splitName.join('.');
    IEditor* editor = GetIEditor();
    IDataBaseManager* manager = m_library->GetManager();

    CRY_ASSERT(editor);
    CRY_ASSERT(manager);

    ////////////////////////////////////////////////////////////////////////////////////////
    //Validate name.
    if (manager->FindItemByName(fullName))
    {
        //NOTE: have to create a local buffer and evaluate the full message here due to calling into another address space to evaluate va_args
        // this prevents passing of random data to the log system ... any plugin that does not do it this way is rolling the dice each time.
        QString warning = QString("Item with name %1 already exists").arg(fullName);
        LogWarning(warning);
        return nullptr;
    }
    if (!m_treeView->ValidateItemName(childName, nameWithoutLibrary, nullptr))
    {
        return nullptr;
    }
    ////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////
    //Create the new item
    OnContentSizeChanged();
    CBaseLibraryItem* newitem = reinterpret_cast<CBaseLibraryItem*>(manager->CreateItem(m_library));
    return newitem;
    ////////////////////////////////////////////////////////////////////////////////////////
}

QString DockableLibraryTreeView::AddLibItem(QString path, const bool overrideSafety /*= false*/)
{
    QString newItemName = path;
    QString defaultName = "emitter";
    ////////////////////////////////////////////////////////////////////////////////////////
    // Get Item name
    if (!overrideSafety)
    {
        newItemName = newItemName.isEmpty() ? defaultName : newItemName + "." + defaultName;
        QString libName = m_library->GetName();
        newItemName = m_library->GetManager()->MakeUniqueItemName(newItemName, libName);
    }
    if (newItemName.isEmpty())
    {
        return "";
    }
    ////////////////////////////////////////////////////////////////////////////////////////

    //Remove Lib Name
    CBaseLibraryItem* item = AddItem(newItemName);
    if (!item)
    {
        return "";
    }
    emit SignalItemAdded(item, newItemName);
    item->SetName(newItemName.toUtf8().data());
    //sync from the engine to ensure the library tree view is accurate
    SyncFromEngine();
    CLibraryTreeViewItem* treeItem = GetItemFromName(newItemName);
    if (treeItem)
    {
        treeItem->SetVirtual(false);
        RefreshView();
    }
    return treeItem->GetFullPath();
}

bool DockableLibraryTreeView::CanItemHaveFolderAsChild(const QString& nameWithoutLibrary)
{
    if (nameWithoutLibrary == "")
    {
        return true;
    }
    CLibraryTreeViewItem* item = GetItemFromName(nameWithoutLibrary);
    if (!item)
    {
        //if an item needs created we will create it appropriately
        return true;
    }
    return item->IsVirtualItem();
}

void DockableLibraryTreeView::RenameItem(CLibraryTreeViewItem* item, const QString forceNameOverride /*= ""*/, const bool overrideSafety /*= false*/)
{
    m_treeView->StartRename(item, forceNameOverride, overrideSafety);
}

const QString DockableLibraryTreeView::StoreExpandItems()
{
    return m_treeView->storeExpandState();
}

void DockableLibraryTreeView::RestoreExpandItems(const QString expandData)
{
    if (!expandData.isEmpty())
    {
        m_treeView->restoreExpandState(expandData);
    }
}


void DockableLibraryTreeView::StoreSelectionData()
{
    m_treeView->StoreSelectionData();
}

void DockableLibraryTreeView::RestoreSelectionData()
{
    m_treeView->RestoreSelectionData();
}

void DockableLibraryTreeView::ExpandItem(CLibraryTreeViewItem* item)
{
    m_treeView->ExpandItem(item);
}

void DockableLibraryTreeView::CollapseItem(CLibraryTreeViewItem* item)
{
    m_treeView->CollapseItem(item);
}

void DockableLibraryTreeView::ExpandAll()
{
    Expand();
    //reloading prevents refreshing the active view,
    //the stored expansion state can become corrupt if refreshing while expanding
    m_treeView->blockSignals(true);
    m_treeView->expandAll();
    m_treeView->blockSignals(false);
    RefreshView();
}

void DockableLibraryTreeView::CollapseAll()
{
    Collapse();
    //reloading prevents refreshing the active view,
    //the stored expansion state can become corrupt if refreshing while collapsing
    m_treeView->blockSignals(true);
    m_treeView->collapseAll();
    m_treeView->blockSignals(false);
    RefreshView();
}

QVector<CLibraryTreeViewItem*> DockableLibraryTreeView::GetSelectedItems()
{
    return m_treeView->GetSelectedItems();
}

QVector<CBaseLibraryItem*> DockableLibraryTreeView::GetSelectedItemsWithChildren()
{
    return m_treeView->GetSelectedItemsWithChildren();
}

QVector<CLibraryTreeViewItem*> DockableLibraryTreeView::GetSelectedTreeItemsWithChildren()
{
    return m_treeView->GetSelectedTreeItemsWithChildren();
}

QVector<CBaseLibraryItem*> DockableLibraryTreeView::GetChildrenOfItem(const QString& itemPath)
{
    return m_treeView->GetChildrenOfItem(itemPath);
}

QVector<CLibraryTreeViewItem*> DockableLibraryTreeView::GetChildrenOfTreeItem(const QString& itemPath)
{
    return m_treeView->GetChildrenOfTreeItem(itemPath);
}

void DockableLibraryTreeView::PassThroughTreeFilledFromLibrary(IDataBaseLibrary* lib, CLibraryTreeView* view)
{
    emit SignalTreeFilledFromLibrary(lib, view);
}

void DockableLibraryTreeView::PassThroughItemAboutToBeDragged(CLibraryTreeViewItem* item)
{
    emit SignalItemAboutToBeDragged(item);
}

void DockableLibraryTreeView::PassThroughDragOperationFinished()
{
    emit SignalDragOperationFinished();
}

void DockableLibraryTreeView::ShowOnlyStringsMatching(const QString& filter)
{
    if (m_treeView->ApplyFilter(filter))
    {
        show();
    }
    else
    {
        hide();
    }
    OnContentSizeChanged(); //update the content size to match the new filter
}

void DockableLibraryTreeView::RemoveFilter()
{
    m_treeView->ApplyFilter("");
    show();
    OnContentSizeChanged(); //update the content size to match the new filter
}

void DockableLibraryTreeView::SetAllItemsEnabled(const bool& enabled /*= true*/)
{
    m_treeView->SetAllItemsEnabled(enabled);
}

void DockableLibraryTreeView::PassThroughItemEnableStateChanged(CBaseLibraryItem* item, const bool& state)
{
    emit SignalItemEnableStateChanged(item, state);
}

void DockableLibraryTreeView::RefreshView()
{
    OnContentSizeChanged(); //update the content size for good measure
    m_treeView->refreshActiveState();
}

void DockableLibraryTreeView::showEvent(QShowEvent* e)
{
    QDockWidget::showEvent(e);
    emit SignalShowEvent();
}

void DockableLibraryTreeView::OnFirstShowing()
{
    //this function forces the sizing to correct itself when the widget is displayed.
    //otherwise when Level is first loaded it will not show the default view
    update();
    if (m_treeView->topLevelItemCount() > 1) //Item count always > 1 since there is an empty "unparent" item
    {
        ShowDefaultView();
        ShowTreeView();
    }
    else
    {
        ShowTreeView();
        ShowDefaultView();
    }
}

void DockableLibraryTreeView::OnMenuRequested(const QPoint& pos)
{
    ContextMenu* menu = new ContextMenu(this);
    emit SignalFocused(this); //make sure this library is selected
    emit SignalPopulateLibraryContextMenu(menu, QString(GetLibrary()->GetName()));
    /*connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
    menu->popup(m_titleBar->mapToGlobal(pos));*/
    menu->exec(pos);
    //the menu will be deleted on deletion of this, as this is set to be the menu's parent
}

QString DockableLibraryTreeView::AddLibFolder(QString path, const bool overrideSafety /*= false*/)
{
    QString newItemName = path;
    QString defaultName = "folder";
    ////////////////////////////////////////////////////////////////////////////////////////
    // Get Item name
    if (!overrideSafety)
    {
        newItemName = newItemName.isEmpty() ? defaultName : newItemName + "." + defaultName;
        newItemName = m_library->GetManager()->MakeUniqueItemName(newItemName, m_library->GetName());
    }
    if (newItemName.isEmpty())
    {
        return "";
    }
    ////////////////////////////////////////////////////////////////////////////////////////

    QStringList nextSplitName = newItemName.split('.');
    nextSplitName.pop_back();
    QString nextPath = "";
    do
    {
        if (!nextPath.isEmpty())
        {
            nextPath += ".";
        }
        if (!nextSplitName.isEmpty())
        {
            nextPath += nextSplitName.takeFirst();
        }
        if (!CanItemHaveFolderAsChild(nextPath))
        {
            //NOTE: have to create a local buffer and evaluate the full message here due to calling into another address space to evaluate va_args
            // this prevents passing of random data to the log system ... any plugin that does not do it this way is rolling the dice each time.
            QString warning = QString("Folder \'%1\' is not allowed to add under an item \'%2\'.").arg(newItemName).arg(nextPath);
            LogWarning(warning);
            return "";
        }
    } while (nextSplitName.count() > 0);

    //add items as we go down if  an item can't have a folder cancel and return
    CBaseLibraryItem* item = AddItem(newItemName);
    if (!item)
    {
        return "";
    }
    emit SignalItemAdded(item, newItemName);
    item->SetName(newItemName.toUtf8().data());
    //sync from the engine to ensure the library tree view is accurate
    SyncFromEngine();
    CLibraryTreeViewItem* treeItem = GetItemFromName(newItemName);
    if (treeItem)
    {
        treeItem->SetVirtual(true);
        // SyncFromEngine will rebuild the libraryTreeView, therefore, the treeItem will be corrupted.
        SyncFromEngine(); 
        treeItem = GetItemFromName(newItemName);
        return treeItem->GetFullPath();
    }
    return "";
}

void DockableLibraryTreeView::RenameLibrary()
{
    m_defaultView->setDisabled(true);
    m_titleBar->BeginRename();
}

void DockableLibraryTreeView::focusInEvent(QFocusEvent* e)
{
    emit SignalFocused(this);
    return QDockWidget::focusInEvent(e);
}

void DockableLibraryTreeView::OnLibraryRename(const QString& str)
{
    m_defaultView->setDisabled(false);
    emit SignalLibraryRenamed(m_library->GetName(), str);
}

void DockableLibraryTreeView::OnLibraryNameValidation(const QString& str, bool& isValid)
{
    isValid = false;
    if (str.isEmpty())
    {
        return;
    }
    if (str.toStdString().find_first_not_of(VALID_LIBRARY_CHARACTERS) != std::string::npos)
    {
        QMessageBox::warning(this, tr("Warning"), tr("Invalid library name. Symbols/glyphs is not allowed in the name.\n"), QMessageBox::Close);
        return;
    }
    isValid = true;
    if (str.compare(m_treeView->GetLibName(), Qt::CaseInsensitive) == 0)
    {
        return; //we were not renamed so we don't allow further checking
    }
    emit SignalTitleValidationCheck(str, isValid);
}

QString DockableLibraryTreeView::StoreExpansionData()
{
    QString result = m_collapsedWidget->isHidden() ? "1&&" : "0&&";
    result += m_treeView->storeExpandState();
    return result;
}

void DockableLibraryTreeView::RestoreExpansionData(const QString& value)
{
    if (value.isEmpty())
    {
        return;
    }
    QStringList splitValue = value.split(QTUI_UTILS_NAME_SEPARATOR);
    if (splitValue.takeFirst() == "1")
    {
        Expand();
    }
    else
    {
        Collapse();
    }
    QString resultingValue = splitValue.join(QTUI_UTILS_NAME_SEPARATOR);
    m_treeView->restoreExpandState(value);
}

void DockableLibraryTreeView::PassThroughItemCheckLod(CBaseLibraryItem* item, bool& checkLod)
{
    emit SignalItemCheckLod(item, checkLod);
}

void DockableLibraryTreeView::PassThroughPopulateSelectionList(QVector<CLibraryTreeViewItem*>& listOut)
{
    emit SignalPopulateSelectionList(listOut);
}

void DockableLibraryTreeView::PassThroughStartDrag(QDrag* drag, Qt::DropActions supportedActions)
{
    emit SignalStartDrag(drag, supportedActions);
}


void DockableLibraryTreeView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("LibraryTreeData"))
    {
        return event->accept();
    }
    QDockWidget::dragEnterEvent(event);
}

void DockableLibraryTreeView::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("LibraryTreeData"))
    {
        return event->accept();
    }
    event->ignore();
}

void DockableLibraryTreeView::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasFormat("LibraryTreeData"))
    {
        event->accept();
        return m_treeView->DropMetaData(event, false);
    }
    event->ignore();
}

void DockableLibraryTreeView::ProcessDelayedWarning(QString warning)
{
    //NOTE: have to create a local buffer and evaluate the full message here due to calling into another address space to evaluate va_args
    // this prevents passing of random data to the log system ... any plugin that does not do it this way is rolling the dice each time.
    const size_t BUFFER_SIZE = 2046;
    char buffer[BUFFER_SIZE];
    if (warning.toUtf8().size() < BUFFER_SIZE)
    {
        strcpy(buffer, warning.toUtf8().data());
    }
    else
    {
        strncpy(buffer, warning.toUtf8().data(), BUFFER_SIZE - 1);
        // ensure the string is null-terminated (strncpy does not guarantee that
        buffer[BUFFER_SIZE - 1] = 0;
    }
    IEditor* editor = GetIEditor();
    editor->GetLogFile()->Warning(buffer);
}

void DockableLibraryTreeView::LogWarning(QString warning)
{
    // Have to do this queued, otherwise there's a crash in Qt because of event loop processing
    // and how the log warning is handled.
    QMetaObject::invokeMethod(this, "ProcessDelayedWarning", Qt::QueuedConnection, Q_ARG(QString, warning));
}

#include <DockableLibraryTreeView.moc>
