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
#include "EditorUI_QT_Precompiled.h"
#include "DockableLibraryPanel.h"
#include "Utils.h"
#include "UIFactory.h"
#include "GameEngine.h"

//Editor
#include <EditorDefs.h>
#include <BaseLibraryManager.h>
#include <Clipboard.h>
#include <ILogFile.h>
#include <IEditorParticleUtils.h> //for shortcut support on menu actions
#include <Include/IEditorParticleManager.h>

//QT
#include <QMenu>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QResizeEvent>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDrag>
#include <qcheckbox.h>
#include <qdialogbuttonbox.h>
#include <qscrollbar.h>
#include "RenameableTitleBar.h"

//Local
#include <DockableLibraryTreeView.h>
#include <PanelWidget/PanelWidget.h>
#include <LibraryTreeView.h>
#include <LibraryTreeViewItem.h>
#include <DockWidgetTitleBar.h>
#include <ContextMenu.h>
#include <UIUtils.h>
#include <DefaultViewWidget.h>

#include <EditorUI_QTDLLBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#define QTUI_EDITOR_ENDRENAME_FUNCTIONNAME "EndRename"
#define QTUI_EDITOR_ACTIONRENAME_FUNCTIONNAME "ActionRename"
#define QTUI_EDITOR_ACTIONADDITEM_FUNCTIONNAME "ActionAddItem"
#define QTUI_EDITOR_ACTIONADDFOLDER_FUNCTIONNAME "ActionAddFolder"

DockableLibraryPanel::DockableLibraryPanel(QWidget* parent)
    : FloatableDockPanel("", parent)
    , m_libraryManager(nullptr)
    , m_libraryTitleBar(nullptr)
    , m_panelName("")
    , m_titleBarMenu(nullptr)
    , m_dockableArea(nullptr)
    , m_enabledItemTextColor(Qt::magenta)
    , m_disabledItemTextColor(Qt::magenta)
    , m_searchArea(nullptr)
    , m_searchField(nullptr)
    , m_AddNewLibraryButton(nullptr)
    , m_defaultView(nullptr)
    , m_selectedLibraryKey("")
    , m_lastAddedItem("")
    , m_loadingLibrary(false)
{
    RegisterActions();
    m_libraryTreeViews.clear();
    EditorUIPlugin::LibraryPanelRequests::Bus::Handler::BusConnect();
}

DockableLibraryPanel::~DockableLibraryPanel()
{
    EditorUIPlugin::LibraryPanelRequests::Bus::Handler::BusDisconnect();
    if (m_libraryTitleBar)
    {
        delete m_libraryTitleBar;
        m_libraryTitleBar = nullptr;
    }

    if (m_titleBarMenu)
    {
        delete m_titleBarMenu;
        m_titleBarMenu = nullptr;
    }
}

void DockableLibraryPanel::Init(const QString& panelName, CBaseLibraryManager* libraryManager)
{
    CRY_ASSERT(libraryManager);
    m_panelName = panelName;
    m_libraryManager = libraryManager;
    m_layoutWidget = new QWidget(this);
    m_layout = new QVBoxLayout(m_layoutWidget);
    m_libraryTitleBar = new DockWidgetTitleBar(this);
    m_dockableArea = new PanelWidget(this);
    m_searchArea = new QWidget(this);
    m_searchField = new QComboBox(this);
    m_searchLayout = new QHBoxLayout(m_searchArea);
    m_AddNewLibraryButton = new QPushButton(this);
    m_scrollArea = new QScrollArea(this);
    m_dockableArea->setDockOptions(QMainWindow::DockOption::AnimatedDocks);
    setStyle(new dropIndicatorStyle(style()));
    m_dockableArea->setAcceptDrops(true);

    m_searchField->setObjectName("LibrarySearchField");
    m_AddNewLibraryButton->setIcon(QIcon(":/particleQT/icons/libraries_icon.png"));
    m_AddNewLibraryButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    //search area set up
    m_searchArea->setLayout(m_searchLayout);
    m_searchField->setEditable(true);
    m_searchField->setInsertPolicy(QComboBox::InsertAtTop);
    m_searchField->setDuplicatesEnabled(false);
    m_searchField->lineEdit()->setPlaceholderText(tr("Search"));
    m_searchLayout->setContentsMargins(0, 0, 0, 0);
    connect(m_searchField, &QComboBox::currentTextChanged, this, &DockableLibraryPanel::OnSearchFilterChanged);
    m_searchLayout->addWidget(m_searchField);
    m_searchLayout->addWidget(m_AddNewLibraryButton);
    connect(m_AddNewLibraryButton, &QPushButton::clicked, this, &DockableLibraryPanel::OnLibraryToggleButtonPushed);

    //set up dock area
    m_scrollArea->setWidgetResizable(true);
    m_dockableArea->setFocusPolicy(Qt::StrongFocus);
    m_scrollArea->setWidget(m_dockableArea);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setMouseTracking(true);
    m_dockableArea->setObjectName("LibraryDockArea");
    m_dockableArea->setMouseTracking(true);
    setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    m_scrollArea->setContentsMargins(0, 0, 0, 0);

    //set up default widget
    m_defaultView = new DefaultViewWidget(this);
    m_defaultView->setObjectName("LibraryDefaultView");
    m_defaultView->setContentsMargins(0, 0, 0, 0);
    DecorateDefaultLibraryView();

    //main layout set up
    m_layoutWidget->setLayout(m_layout);
    m_layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    m_searchArea->setFocusPolicy(Qt::StrongFocus);
    m_layout->addWidget(m_searchArea);
    m_defaultView->setFocusPolicy(Qt::StrongFocus);
    m_layout->addWidget(m_defaultView);
    m_scrollArea->setFocusPolicy(Qt::StrongFocus);
    m_layout->addWidget(m_scrollArea);

    m_layoutWidget->setFocusPolicy(Qt::StrongFocus);
    setWidget(m_layoutWidget);

    //setup the Dock Widget
    setObjectName("dwLibraryTreeView");
    m_layoutWidget->setObjectName("dwLibraryTreeViewCentralWidget");
    setTitleBarWidget(m_libraryTitleBar);
    connect(this, &DockableLibraryPanel::windowTitleChanged, this, &DockableLibraryPanel::OnWindowTitleChanged);

    RebuildFromEngineData(true);
    setWindowTitle(panelName);
    m_libraryTitleBar->SetupLabel(panelName);
    m_libraryTitleBar->SetShowMenuContextMenuCallback([&] { return GetTitleBarMenu();
        });
    m_titleBarMenu = new QMenu;

    setAllowedAreas(Qt::AllDockWidgetAreas);
}

void DockableLibraryPanel::ResetSelection()
{
    for (auto itr : m_libraryTreeViews)
    {
        itr->ResetSelection();
    }
    SelectSingleLibrary("");
    emit SignalItemSelected(nullptr);
}

void DockableLibraryPanel::OnWindowTitleUpdate()
{
    m_libraryTitleBar->SetTitle(GetTitleNameWithCount());
}

QString DockableLibraryPanel::GetTitleNameWithCount()
{
    // For now we just have one library supported at a time
    // In future versions this will return the number of libraries loaded
    return QString("%1 (%2)").arg(m_panelName).arg(m_libraryTreeViews.count());
}

QMenu* DockableLibraryPanel::GetTitleBarMenu()
{
    CRY_ASSERT(m_titleBarMenu);
    m_titleBarMenu->clear();
    emit SignalPopulateTitleBarMenu(m_titleBarMenu);
    return m_titleBarMenu;
}

void DockableLibraryPanel::RebuildFromEngineData(bool ignoreExpansionStates)
{
    EBUS_EVENT(EditorUIPlugin::LibraryItemCacheRequests::Bus, ClearAllItemCache);

    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

    m_ItemExpandState.clear();
    m_libExpandData.clear();
    if (!ignoreExpansionStates) // There is no expand/collapse info on new Scene
    {
        StoreAllExpandItemInfo();
    }
    //load/reload all the libraries in the manager
    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
    {
        m_dockableArea->RemovePanel(*iter);
    }
    m_libraryTreeViews.clear();
    //we have to add libraries in the opposite order we want them to display
    for (int i = 0; i < m_libraryManager->GetLibraryCount(); i++)
    {
        QString libName = m_libraryManager->GetLibrary(i)->GetName();
        // Level library is not loaded by default
        if (m_libraryManager->GetLibrary(i)->IsLevelLibrary() || libName == QString("Level"))
        {
            continue;
        }
        LoadLibrary(libName.toUtf8().data());
    }
    OnWindowTitleUpdate();
    TriggerDefaultView();
    if (!ignoreExpansionStates)
    {
        RestoreAllExpandItemInfo();
    }
}

void DockableLibraryPanel::LoadLibrary(const QString& libName)
{
    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

    if (!m_libraryTreeViews.contains(libName))
    {
        IDataBaseLibrary* library = m_libraryManager->FindLibrary(libName);
        if (!library)
        {
            IEditor* editor = GetIEditor();
            CRY_ASSERT(editor);
            
            library = m_libraryManager->LoadLibrary(libName);
            if (!library)
            {
                return;
            }
        }
        m_loadingLibrary = true;

        //update library items caches
        for (int i = 0; i < library->GetItemCount(); i++)
        {
            IDataBaseItem *item = library->GetItem(i);
            EBUS_EVENT(LibraryItemCacheRequestsBus, UpdateItemCache, item->GetFullName().toUtf8().data());
        }

        m_libraryTreeViews.insert(QString(libName), new DockableLibraryTreeView(this));
        DockableLibraryTreeView* dock = m_libraryTreeViews[QString(libName)];
        m_dockableArea->AddCustomPanel(dock);
        //movable dockableLibraryTreeViews is not in the provided wireframes,
        //it also only partially works since there is no concept of "order" when dealing with cry engine libraries.
        dock->setFeatures(QDockWidget::DockWidgetMovable);
        dock->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        SetHotkeyHandler(dock, [&](QKeyEvent* e) { keyPressEvent(e); });

        connect(dock, &DockableLibraryTreeView::SignalItemCheckLod, this, &DockableLibraryPanel::PassThroughItemCheckLod);

        connect(dock, &DockableLibraryTreeView::SignalItemSelected, this, &DockableLibraryPanel::PassThroughItemSelection);
        connect(dock, &DockableLibraryTreeView::SignalItemAboutToBeRenamed, this, &DockableLibraryPanel::PassThroughItemAboutToBeRenamed);
        connect(dock, &DockableLibraryTreeView::SignalItemRenamed, this, &DockableLibraryPanel::PassThroughItemRenamed);
        connect(dock, &DockableLibraryTreeView::SignalPopulateItemContextMenu, this, &DockableLibraryPanel::PassThroughPopulateItemContextMenu);
        connect(dock, &DockableLibraryTreeView::SignalTreeFilledFromLibrary, this, &DockableLibraryPanel::PassThroughTreeFilledFromLibrary);
        connect(dock, &DockableLibraryTreeView::SignalItemAboutToBeDragged, this, &DockableLibraryPanel::PassThroughItemAboutToBeDragged);
        connect(dock, &DockableLibraryTreeView::SignalDragOperationFinished, this, &DockableLibraryPanel::PassThroughDragOperationFinished);
        connect(dock, &DockableLibraryTreeView::SignalItemEnableStateChanged, this, &DockableLibraryPanel::PassThroughItemEnableStateChanged);
        connect(dock, &DockableLibraryTreeView::SignalDecorateDefaultView, this, &DockableLibraryPanel::PassThroughDecorateDefaultView);
        connect(dock, &DockableLibraryTreeView::SignalPopulateLibraryContextMenu, this, &DockableLibraryPanel::SignalPopulateLibraryContextMenu);
        connect(dock, &DockableLibraryTreeView::SignalItemAdded, this, &DockableLibraryPanel::PassThroughItemAdded);

        connect(dock, &DockableLibraryTreeView::SignalCopyItem, this, &DockableLibraryPanel::SignalCopyItem);
        connect(dock, &DockableLibraryTreeView::SignalPasteItem, this, &DockableLibraryPanel::SignalItemPasted);

        connect(dock, &DockableLibraryTreeView::SignalMoveItem, this, &DockableLibraryPanel::ActionMoveOneFolder);
        connect(dock, &DockableLibraryTreeView::SignalUpdateTabName, this, &DockableLibraryPanel::SignalUpdateTabName);

        connect(dock, &DockableLibraryTreeView::SignalContentResized, m_dockableArea, [=]() { m_dockableArea->FixSizing(true); });
        connect(dock, &DockableLibraryTreeView::SignalFocused, this, [=](DockableLibraryTreeView* libDock) {SelectSingleLibrary(libDock->windowTitle().toUtf8().data()); });

        connect(dock, &DockableLibraryTreeView::SignalLibraryRenamed, this, &DockableLibraryPanel::LibraryRenamed);
        connect(dock, &DockableLibraryTreeView::SignalTitleValidationCheck, this, &DockableLibraryPanel::OnLibraryTitleValidationCheck);
        connect(dock, &DockableLibraryTreeView::SignalPopulateSelectionList, this, [=](QVector<CLibraryTreeViewItem*>& listOut) { listOut = GetSelectedTreeItems(); });
        connect(dock, &DockableLibraryTreeView::SignalStartDrag, this, &DockableLibraryPanel::OnStartDragRequested);
        connect(dock, &DockableLibraryTreeView::dockLocationChanged, this, [=]() { OnDockLocationChanged(dock); });
                
        OnLibraryAdded(library);

        dock->Expand();
        dock->UpdateColors(m_enabledItemTextColor, m_disabledItemTextColor);
        m_loadingLibrary = false;
    }
    SelectSingleLibrary(libName);

}

QAction* DockableLibraryPanel::GetMenuAction(LibraryActions action, QString displayAlias, bool overrideSafety /*false*/, QWidget* owner /*=nullptr*/, Qt::ConnectionType connection /* = Qt::AutoConnection*/)
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
#define SET_SHORTCUT(x) act->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut(x))
#define SET_ICON(x) act->setIcon(QIcon(x))
    QAction* act = nullptr;
    if (owner)
    {
        act = new QAction(displayAlias, owner);
    }
    else
    {
        act = new QAction(displayAlias, this);
    }
    QString currentLib = GetSelectedLibraryName();
    bool isLibSelected = !currentLib.isEmpty();
    switch (action)
    {
    case DockableLibraryPanel::LibraryActions::ADD:
    {
        connect(act, &QAction::triggered, this, [=]()
        {
            AddLibrary();
        }, connection);
        SET_ICON("Editor/UI/Icons/toolbar/libraryAdd.png");
        SET_SHORTCUT("File Menu.Create new library");
        break;
    }
    case DockableLibraryPanel::LibraryActions::IMPORT:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                ImportLibrary(PromptImportLibrary());
            }, connection);
        SET_ICON("Editor/UI/Icons/toolbar/libraryLoad.png");
        SET_SHORTCUT("File Menu.Import");
        break;
    }
    case DockableLibraryPanel::LibraryActions::IMPORT_LEVEL_LIBRARY:
    {
        connect(act, &QAction::triggered, this, [=]()
        {
            if (GetIEditor()->GetGameEngine()->GetLevelName().compare("Untitled") == 0)
            {
                QMessageBox::warning(this, "Level Library", "Requires the Level to be loaded.");
                return;
            }
            if (m_libraryManager->GetLevelLibrary() != nullptr)
            {
                if (m_libraryManager->GetLevelLibrary()->GetItemCount() != 0)
                {
                    AddLevelCopyLibrary();
                }
                else
                {
                    QMessageBox::warning(this, "Level Library", "Level contains no particle data.");
                }
            }
            else
            {
                QMessageBox::warning(this, "Level Library", "No level library present.");
            }
        }, connection);
        SET_ICON("Editor/UI/Icons/toolbar/libraryLoad.png");
        SET_SHORTCUT("File Menu.Import level library");
        break;
    }
    case DockableLibraryPanel::LibraryActions::SAVE_AS:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                ActionExportLib(currentLib);
            }, connection);
        act->setEnabled(isLibSelected);
        break;
    }
    case DockableLibraryPanel::LibraryActions::SAVE:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                QString libName = currentLib;
                IDataBaseLibrary *lib = m_libraryManager->FindLibrary(libName);
                if (lib->Save())
                {
                    lib->SetModified(false);
                }
            }, connection);
        act->setEnabled(isLibSelected);
        break;
    }
    case DockableLibraryPanel::LibraryActions::SAVE_ALL:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                m_libraryManager->SaveAllLibs();
                emit SignalSaveAllLibs();
            }, connection);
        SET_ICON("Editor/UI/Icons/toolbar/librarySave.png");
        SET_SHORTCUT("File Menu.Save");
        break;
    }
    case DockableLibraryPanel::LibraryActions::ENABLE_ALL:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                if (!currentLib.isEmpty())
                {
                    if (m_libraryTreeViews.contains(currentLib))
                    {
                        EditorUIPlugin::ScopedBatchUndoPtr batchUndo;
                        EBUS_EVENT_RESULT(batchUndo, EditorLibraryUndoRequestsBus, AddScopedBatchUndo, "Enable All");

                        m_libraryTreeViews[currentLib]->SetAllItemsEnabled(true);
                    }
                }
            }, connection);
        break;
    }
    case DockableLibraryPanel::LibraryActions::DISABLE_ALL:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                if (!currentLib.isEmpty())
                {
                    if (m_libraryTreeViews.contains(currentLib))
                    {
                        EditorUIPlugin::ScopedBatchUndoPtr batchUndo;
                        EBUS_EVENT_RESULT(batchUndo, EditorLibraryUndoRequestsBus, AddScopedBatchUndo, "Disable All");

                        m_libraryTreeViews[currentLib]->SetAllItemsEnabled(false);
                    }
                }
            }, connection);
        break;
    }
    case DockableLibraryPanel::LibraryActions::REMOVE:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                RemoveLibrary(currentLib);
            }, connection);
        SET_ICON("Editor/UI/Icons/toolbar/itemRemove.png");
        act->setEnabled(isLibSelected);
        break;
    }
    case DockableLibraryPanel::LibraryActions::RELOAD:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                ReloadLibrary(currentLib);
            }, connection);
        act->setEnabled(isLibSelected);
        break;
    }
    case DockableLibraryPanel::LibraryActions::DUPLICATE_LIB:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                DuplicateLibrary(currentLib);
            }, connection);
        act->setEnabled(isLibSelected);
        break;
    }
    default:
    {
        act->setParent(nullptr);
        SAFE_DELETE(act);
        act = nullptr;
        break;
    }
    }
#undef SET_SHORTCUT
#undef SET_ICON
    return act;
}

QAction* DockableLibraryPanel::GetMenuAction(ItemActions action, QString fullNameOfItem, QString displayAlias, bool overrideSafety /*false*/, QWidget* owner /*=nullptr*/, Qt::ConnectionType connection /*autoConnection*/)
{
    QStringList splitName = fullNameOfItem.split('.');
    QString libName = splitName.takeFirst();
    QString itemPath = splitName.join('.');
    CLibraryTreeViewItem* item = nullptr;
    if (!fullNameOfItem.isEmpty())
    {
        item = GetTreeItemFromPath(fullNameOfItem);
    }
    QAction* act = nullptr;
    if (owner)
    {
        act = new QAction(displayAlias, owner);
    }
    else
    {
        act = new QAction(displayAlias, this);
    }
#define SET_SHORTCUT(x) act->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut(x))
#define SET_ICON(x) act->setIcon(QIcon(x))
    switch (action)
    {
    case DockableLibraryPanel::ItemActions::OPEN_IN_NEW_TAB:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                ActionOpenInNewTab(fullNameOfItem);
            }, connection);
        break;
    }
    case DockableLibraryPanel::ItemActions::ADD_ITEM:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                ActionAddItem(fullNameOfItem);
            }, connection);
        break;
    }
    case DockableLibraryPanel::ItemActions::ADD_FOLDER:
    {
        if (!itemPath.isEmpty() && item && !item->IsVirtualItem())
        {
            SAFE_DELETE(act);
            break;
        }
        connect(act, &QAction::triggered, this, [=]()
            {
                ActionAddFolder(fullNameOfItem);
            }, connection);
        break;
    }
    case DockableLibraryPanel::ItemActions::SHOW_PATH:
    {
        if (fullNameOfItem.isEmpty())
        {
            QVector<CBaseLibraryItem*> items = GetSelectedItems();
            if (items.count() && items.first())
            {
                fullNameOfItem = QString(items.first()->GetFullName());
            }
        }
        connect(act, &QAction::triggered, this, [=]()
            {
                CLibraryTreeViewItem* item = nullptr;
                if (!fullNameOfItem.isEmpty())
                {
                    item = GetTreeItemFromPath(fullNameOfItem);
                }
                QMessageBox::warning(this, tr("Focus"), fullNameOfItem, QMessageBox::Button::Close);
            }, connection);
        SET_SHORTCUT("File Menu.Focus");
        act->setEnabled(!fullNameOfItem.isEmpty());
        break;
    }
    case DockableLibraryPanel::ItemActions::RENAME:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                ActionRename(fullNameOfItem, overrideSafety);
            }, connection);
        SET_SHORTCUT("Edit Menu.Rename");
        break;
    }
    case DockableLibraryPanel::ItemActions::DESTROY:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                ActionDelete(fullNameOfItem, overrideSafety);
            }, connection);
        SET_SHORTCUT("Edit Menu.delete");
        break;
    }
    case DockableLibraryPanel::ItemActions::COPY_PATH:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                CLibraryTreeViewItem* item = nullptr;
                if (!fullNameOfItem.isEmpty())
                {
                    item = GetTreeItemFromPath(fullNameOfItem);
                }
                if (item)
                {
                    CClipboard clipboard(this);
                    clipboard.PutString(item->GetFullPath().toUtf8().data());
                }
                else
                {
                    act->setEnabled(false);
                }
            }, connection);
        SET_SHORTCUT("Edit Menu.Copy Path");
        break;
    }
    case DockableLibraryPanel::ItemActions::EXPAND_ALL:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                if (item)
                {
                    DockableLibraryTreeView* libDock = nullptr;
                    libDock = m_libraryTreeViews[libName];
                    libDock->ExpandItem(item);
                }
            }, connection);
        break;
    }

    case DockableLibraryPanel::ItemActions::COLLAPSE_ALL:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                if (item)
                {
                    DockableLibraryTreeView* libDock = nullptr;
                    libDock = m_libraryTreeViews[libName];
                    libDock->CollapseItem(item);
                }
            }, connection);
        break;
    }
    default:
    {
        act->setParent(nullptr);
        SAFE_DELETE(act);
        act = nullptr;
        break;
    }
    }
#undef SET_SHORTCUT
#undef SET_ICON
    return act;
}

QAction* DockableLibraryPanel::GetMenuAction(TreeActions action, QString displayAlias, bool overrideSafety /*false*/, QWidget* owner /*nullptr*/, Qt::ConnectionType connection /* = Qt::AutoConnection*/)
{
    QAction* act = nullptr;
    if (owner)
    {
        act = new QAction(displayAlias, owner);
    }
    else
    {
        act = new QAction(displayAlias, this);
    }

#define SET_SHORTCUT(x) act->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut(x))
#define SET_ICON(x) act->setIcon(QIcon(x))
    switch (action)
    {
    case DockableLibraryPanel::TreeActions::EXPAND_ALL:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                DockableLibraryTreeView* libDock = nullptr;
                libDock = m_libraryTreeViews[GetSelectedLibraryName()];
                if (!libDock)
                {
                    //no lib selected return nullptr
                    return;
                }
                libDock->ExpandAll();
            }, connection);
        SET_SHORTCUT("View Menu.Expand All");
        break;
    }
    case DockableLibraryPanel::TreeActions::COLLAPSE_ALL:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                DockableLibraryTreeView* libDock = nullptr;
                libDock = m_libraryTreeViews[GetSelectedLibraryName()];
                if (!libDock)
                {
                    //no lib selected return nullptr
                    return;
                }
                libDock->CollapseAll();
            }, connection);
        SET_SHORTCUT("View Menu.Collapse All");
        break;
    }
    default:
    {
        act->setParent(nullptr);
        SAFE_DELETE(act);
        act = nullptr;
        break;
    }
    }
#undef SET_SHORTCUT
#undef SET_ICON
    return act;
}

void DockableLibraryPanel::UpdateColors(const QMap<QString, QColor>& colorMap)
{
    if (!colorMap.contains("CTreeViewText"))
    {
        m_enabledItemTextColor = Qt::magenta;
    }
    else
    {
        m_enabledItemTextColor = colorMap["CTreeViewText"];
    }
    if (!colorMap.contains("CTextEditDisabledText"))
    {
        m_disabledItemTextColor = Qt::magenta;
    }
    else
    {
        m_disabledItemTextColor = colorMap["CTextEditDisabledText"];
    }
    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
    {
        (*iter)->UpdateColors(m_enabledItemTextColor, m_disabledItemTextColor);
    }
}

void DockableLibraryPanel::OnLibraryAdded(IDataBaseLibrary* addLib)
{
    CRY_ASSERT(addLib);
    DockableLibraryTreeView* dock = m_libraryTreeViews[QString(addLib->GetName())];
    CRY_ASSERT(dock);
    dock->setWindowTitle(tr(addLib->GetName().toUtf8().data()));
    dock->Init(addLib);
    SelectSingleLibrary(addLib->GetName());
}

void DockableLibraryPanel::OnWindowTitleChanged(const QString& title)
{
    m_panelName = title;
    OnWindowTitleUpdate();
}

void DockableLibraryPanel::SelectLibraryAndItemByName(const QString& libraryName, const QString& itemName, bool forceSelection)
{
    if (m_libraryTreeViews.contains(libraryName))
    {
        SelectSingleLibrary(libraryName.toUtf8().data());
        m_libraryTreeViews[libraryName]->SelectItemFromName(itemName, forceSelection);
    }
    else
    {
        SelectSingleLibrary(libraryName.toUtf8().data());
        ResetSelection(); // ResetSelection will emit signal :SignalItemSelected(nullptr);
    }
}

void DockableLibraryPanel::PassThroughItemSelection(CBaseLibraryItem* item)
{
    //find the libraries that do not own this item and clear the selections
    QString libName = "";
    if (item && item->GetLibrary())
    {
        libName = item->GetLibrary()->GetName();
        SelectSingleLibrary(libName.toUtf8().data());
    }

    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); ++iter)
    {
        if (iter.key().compare(libName, Qt::CaseInsensitive) != 0)
        {
            (*iter)->ResetSelection();
        }
    }
    emit SignalItemSelected(item);
}

void DockableLibraryPanel::PassThroughItemAboutToBeRenamed(CBaseLibraryItem* item, const QString& currentName, const QString& nextName, bool& proceed)
{
    emit SignalItemAboutToBeRenamed(item, currentName, nextName, proceed);
}

void DockableLibraryPanel::PassThroughItemCheckLod(CBaseLibraryItem* item, bool& hasLod)
{
    emit SignalItemCheckLod(item, hasLod);
}

void DockableLibraryPanel::PassThroughItemRenamed(CBaseLibraryItem* item, const QString& oldName, const QString& currentName, const QString newlib)
{
    emit SignalItemRenamed(item, oldName, currentName, newlib);
}

void DockableLibraryPanel::PassThroughPopulateItemContextMenu(CLibraryTreeViewItem* focusedItem, ContextMenu* toAddTo)
{
    emit SignalPopulateItemContextMenu(focusedItem, toAddTo);
}

CLibraryTreeViewItem* DockableLibraryPanel::GetTreeItemFromPath(const QString& fullName)
{
    QStringList splitName = fullName.split('.');
    QString libName = splitName.takeAt(0);
    QString itemName = splitName.join('.');
    if (m_libraryTreeViews.contains(libName))
    {
        DockableLibraryTreeView* library = m_libraryTreeViews[libName];
        if (library)
        {
            return library->GetItemFromName(itemName);
        }
    }
    return nullptr;
}

void DockableLibraryPanel::UpdateIconStyle(QString libName, QString fullname)
{
    QStringList splitName = fullname.split('.');
    QString lib = splitName.takeAt(0);
    QString itemName = splitName.join('.');
    if (m_libraryTreeViews.contains(libName))
    {
        CLibraryTreeViewItem* item = m_libraryTreeViews[libName]->GetItemFromName(itemName);
        if (item)
        {
            m_libraryTreeViews[libName]->UpdateIconStyle(item);
        }
    }
}

void DockableLibraryPanel::UpdateLibSelectionStyle()
{
    for (DockableLibraryTreeView* tree : m_libraryTreeViews)
    {
        if (!tree->GetLibrary())
        {
            continue;
        }

        QString libName = tree->GetLibrary()->GetName();
        //update style
        RenameableTitleBar* titlebar = static_cast<RenameableTitleBar*>((tree->titleBarWidget()));
        titlebar->UpdateSelectedLibStyle(m_selectedLibraryKey == libName);
    }
}

void DockableLibraryPanel::SelectSingleLibrary(const QString& libName)
{
    if (m_libraryTreeViews.contains(QString(libName)))
    {
        m_selectedLibraryKey = libName;
    }
    else
    {
        m_selectedLibraryKey = "";
    }
    UpdateLibSelectionStyle();
}

void DockableLibraryPanel::DecorateDefaultLibraryView()
{
    //Set UI size according to the wireframe
    m_defaultView->SetSpaceAtTop(50);
    m_defaultView->SetSpaceBetweenLabelsAndButtons(30);
    QString labelText = tr("You have 0 loaded libraries.\n Please import an existing library \n or create a new one.");
    m_defaultView->SetLabel(labelText);
    m_defaultView->AddButton("Import a library", GetMenuAction(DockableLibraryPanel::LibraryActions::IMPORT, tr("Import a library"), false, m_defaultView, Qt::QueuedConnection));
    m_defaultView->AddButton("Create new library", GetMenuAction(DockableLibraryPanel::LibraryActions::ADD, tr("Create new library"), false, m_defaultView, Qt::QueuedConnection));
}

void DockableLibraryPanel::ShowDefaultView()
{
    m_defaultView->show();
    m_scrollArea->hide();
}

void DockableLibraryPanel::HideDefaultView()
{
    m_defaultView->hide();
    m_scrollArea->show();
}

void DockableLibraryPanel::TriggerDefaultView()
{
    if (m_libraryTreeViews.count() > 0)
    {
        HideDefaultView();
    }
    else
    {
        ShowDefaultView();
    }
}

void DockableLibraryPanel::AddLibrary()
{
    QString libraryName = QString("New_Library");
    int count = 0;
    while (m_libraryManager->FindLibrary(libraryName.toUtf8().data()) || 
        !m_libraryManager->IsUniqueFilename(libraryName.toUtf8().data()))
    {
        //if the name "New_Library" is taken we append 00-99 depending on how many
        //New_library's are in the manager
        libraryName = QString("New_Library%1").arg(++count, 2, 10, QChar('0'));
    }
    m_libraryManager->AddLibrary(libraryName.toUtf8().data(),false, false);
    LoadLibrary(libraryName.toUtf8().data());
    if (m_libraryTreeViews.contains(libraryName))
    {
        m_libraryManager->ChangeLibraryOrder(m_libraryTreeViews[libraryName]->GetLibrary(), m_libraryManager->GetLibraryCount() - 1);
        m_scrollArea->verticalScrollBar()->setSliderPosition(0); // New library will added to top . Scroll to top to sure the new library.
        m_libraryTreeViews[libraryName]->RenameLibrary();
    }

    OnWindowTitleUpdate();
    TriggerDefaultView();
    
}

void DockableLibraryPanel::DuplicateLibrary(const QString& currentLib)
{
    IDataBaseLibrary *originLib = nullptr;
    for (int libIdx = 0; libIdx < m_libraryManager->GetLibraryCount(); libIdx++)
    {
        QString libName = m_libraryManager->GetLibrary(libIdx)->GetName();
        if (libName == currentLib)
        {
            originLib = m_libraryManager->GetLibrary(libIdx);
            break;
        }
    }
    AZ_Assert(originLib, "lib shouldn't be null");

    if (originLib == nullptr)
    {
        return;
    }
    
    //get a valid name for the copied lib
    QString libraryName = currentLib + "_Copy";
    int count = 0;
    while (m_libraryManager->FindLibrary(libraryName) || 
        !m_libraryManager->IsUniqueFilename(libraryName))
    {
        libraryName = QString::asprintf("%s_Copy%.2d", currentLib.toUtf8().data(), ++count);
    }

    m_libraryManager->AddLibrary(libraryName,false, false);

    IDataBaseLibrary* copyLibrary = m_libraryManager->FindLibrary(libraryName);
    //use item serialize instead of library serialize to copy the library so it won't override the items' guid
    IDataBaseItem::SerializeContext ctx;
    ctx.bCopyPaste = true;
    ctx.bIgnoreChilds = false;
    for (int i = 0; i < originLib->GetItemCount(); i++)
    {
        ctx.bLoading = false;
        ctx.node = GetIEditor()->GetSystem()->CreateXmlNode("Copy");
        IDataBaseItem *originItem = originLib->GetItem(i);
        originItem->Serialize(ctx);
        ctx.bLoading = true;
        CParticleItem* pItem = new CParticleItem();
        copyLibrary->AddItem(pItem);
        pItem->SetName(originItem->GetName());
        pItem->Serialize(ctx);
    }

    //load the library so it will be added to the tree view
    LoadLibrary(libraryName);

    if (m_libraryTreeViews.contains(libraryName))
    {
        m_libraryTreeViews[libraryName]->SyncFromEngine();
        m_libraryManager->ChangeLibraryOrder(m_libraryTreeViews[libraryName]->GetLibrary(), m_libraryManager->GetLibraryCount() - 1);
        m_scrollArea->verticalScrollBar()->setSliderPosition(0);
        m_libraryTreeViews[libraryName]->RenameLibrary();
    }

    OnWindowTitleUpdate();
    TriggerDefaultView();
}

void DockableLibraryPanel::AddLevelCopyLibrary()
{
    QString levelName = GetIEditor()->GetGameEngine()->GetLevelName();

    if (levelName == nullptr)
    {
        return;
    }

    QString levelCopyLibraryName = QString("level_") + levelName;
    
    if (m_libraryManager->FindLibrary(levelCopyLibraryName.toUtf8().data()) != nullptr)
    {
        QMessageBox::warning(this, "Cannot create library", levelCopyLibraryName + " library already exists.");
        return;
    }

    m_libraryManager->AddLibrary(levelCopyLibraryName.toUtf8().data(), false, false);
    LoadLibrary(levelCopyLibraryName.toUtf8().data());

    IDataBaseLibrary* levelLibrary = m_libraryManager->GetLevelLibrary();
    IDataBaseLibrary* levelCopyLibrary = m_libraryManager->FindLibrary(levelCopyLibraryName.toUtf8().data());

    if (levelLibrary != nullptr && levelCopyLibrary != nullptr)
    {
        for (int i = 0; i < levelLibrary->GetItemCount(); i++)
        {
            levelCopyLibrary->AddItem(levelLibrary->GetItem(i));
        }
    }

    if (m_libraryTreeViews.contains(levelCopyLibraryName))
    {
        m_libraryManager->ChangeLibraryOrder(m_libraryTreeViews[levelCopyLibraryName]->GetLibrary(), m_libraryManager->GetLibraryCount() - 1);
        m_scrollArea->verticalScrollBar()->setSliderPosition(0); // New library will added to top . Scroll to top to sure the new library.
    }

    levelCopyLibrary->Save();

    QMessageBox::information(this, "Library created", levelCopyLibraryName + " created and saved.");

    RebuildFromEngineData();
    OnWindowTitleUpdate();
    TriggerDefaultView();

    //undo for add library
    EBUS_EVENT(EditorLibraryUndoRequestsBus, AddLibraryCreateUndo, levelCopyLibraryName.toUtf8().data());
}

QString DockableLibraryPanel::GetSelectedLibraryName()
{
    return m_selectedLibraryKey;
}

void DockableLibraryPanel::ExportLibrary(const QString& currentLib)
{
    if (currentLib.isEmpty())
    {
        return;
    }

    CBaseLibrary* library = static_cast<CBaseLibrary*>(m_libraryManager->FindLibrary(currentLib.toUtf8().data()));
    CRY_ASSERT(library);

    //NOTE: when saving a library as some other file we need to rename the library so that we don't get errors about duplicate loading of libraries.
    QString location = QFileDialog::getSaveFileName(this, tr("Select location to save Library..."), "", tr("Emitter Files (*.xml)"));
    if (!location.length()) //if the user hit cancel
    {
        return;
    }

    //Extract the filename that was used.
    QStringList libstrlist = location.split('.');
    QString libName = "";
    if (libstrlist.count() > 0)
    {
        libName = libstrlist[qMax(libstrlist.count() - 2, 0)]; //second to last item or last item if only one item
    }
    if (libName.length() > 0)
    {
        libName = libName.split('/').back();
    }

    if (libName.toStdString().find_first_not_of(VALID_LIBRARY_CHARACTERS) != std::string::npos)
    {
        QMessageBox::warning(this, tr("Warning"), tr("Invalid library Name. Symbols/glyphs are not allowed in the name.\n"), QMessageBox::Button::Close);
    }

    //Store the name we used to be.
    QString oldName = library->GetName();
    library->SetName(libName.toStdString().c_str());

    XmlNodeRef node = GetIEditor()->GetSystem()->CreateXmlNode("Library");
    library->Serialize(node, false);
    node->saveToFile(location.toStdString().c_str());

    //Set the name back to where it was.
    library->SetName(oldName);
}

void DockableLibraryPanel::RemoveLibrary(const QString& libName)
{
    if (libName.isEmpty())
    {
        return;
    }
    CRY_ASSERT(m_libraryTreeViews.contains(libName));

    SelectSingleLibrary("");
    SignalItemSelected(nullptr);

    CBaseLibrary* lib = static_cast<CBaseLibrary*>(m_libraryManager->FindLibrary(libName.toUtf8().data()));
    if (lib)
    {
        EBUS_EVENT(EditorLibraryUndoRequestsBus, AddLibraryDeleteUndo, libName.toUtf8().data());
        lib->RemoveAllItems();

        m_libraryManager->DeleteLibrary(libName.toUtf8().data());
        RebuildFromEngineData();
    }
}

void DockableLibraryPanel::AddLibraryListToMenu(QMenu* subMenu)
{
    QString selected = GetSelectedLibraryName();
    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); ++iter)
    {
        QAction* act = subMenu->addAction(tr(iter.key().toUtf8().data()));
        connect(act, &QAction::triggered, this, [=]()
            {
                SelectSingleLibrary(iter.key().toUtf8().data());
            });
        act->setCheckable(true);
        act->setChecked(iter.key().compare(selected) == 0);
    }
}

void DockableLibraryPanel::ReloadLibrary(const QString& libName)
{
    if (libName.isEmpty())
    {
        return;
    }
    IDataBaseLibrary *lib = m_libraryManager->FindLibrary(libName.toUtf8().data());
    if (!lib)
    {
        return;
    }
    QString filename = lib->GetFilename();

    EditorUIPlugin::ScopedLibraryModifyUndoPtr modifyUndo;
    EBUS_EVENT_RESULT(modifyUndo, EditorLibraryUndoRequestsBus, AddScopedLibraryModifyUndo, AZStd::string(libName.toUtf8().data()));

    if (lib->IsLevelLibrary())
    {
        RemoveLibrary(libName);
        m_libraryManager->AddLibrary(libName.toUtf8().data());
        return LoadLibrary(libName.toUtf8().data());
    }

    //if file not exist, tell user it can't be reloaded if the file has never been saved
    if (!GetIEditor()->GetFileUtil()->FileExists(filename))
    {
        //message 
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Can't reload"));
        msgBox.setText(tr("Can't reload the library since it hasn't been saved before."));
        msgBox.addButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    CRY_ASSERT(m_libraryTreeViews.contains(libName));
    DockableLibraryTreeView* dock = m_libraryTreeViews[libName];
    CRY_ASSERT(dock);

    if (dock->IsModified())
    {
        QString message = tr("Library %1 was modified.\nReloading library will discard all modifications to this library!").arg(libName);

        QMessageBox::StandardButton reply = QMessageBox::question(this,
                "Library is modified", message, QMessageBox::Yes | QMessageBox::Cancel);

        if (reply != QMessageBox::Yes)
        {
            return;
        }
    }

    if (!dock->Reload())
    {
        // Force to remove the library from the panel since
        // it was destroyed during the reload. Thanks to the Scoped
        // Modified Undo command the user will be able to recover it.

        SelectSingleLibrary("");
        SignalItemSelected(nullptr);

        m_libraryManager->DeleteLibrary(libName.toUtf8().data());
        RebuildFromEngineData();
    }
}

void DockableLibraryPanel::ImportLibrary(const QString& file)
{
    if (!file.isEmpty())
    {
        IDataBaseLibrary* library = m_libraryManager->FindLibrary(file);
        if (!library)
        {
            library = m_libraryManager->LoadLibrary(file);
        }
        if (library)
        {
            LoadLibrary(library->GetName());

            OnWindowTitleUpdate();
            TriggerDefaultView();

            AZStd::string name = library->GetName().toUtf8().data();
            EBUS_EVENT(EditorLibraryUndoRequestsBus, AddLibraryCreateUndo, name);
        }
    }
}

CLibraryTreeViewItem* DockableLibraryPanel::AddDuplicateTreeItem(const QString& itemPath, const bool isVirtual /*= false*/, QString pasteTo /*= ""*/)
{
    CRY_ASSERT(!itemPath.isEmpty()); //Assert when item path is empty
    QStringList pathList = itemPath.split('.');
    QString destLib = "";
    QString origLib = "";
    QString newItemName = "";
    QString origItemName = "";
    if (!pasteTo.isEmpty())
    {
        QStringList destinationList = pasteTo.split(".");
        destLib = destinationList.takeFirst();
        newItemName = destinationList.join(".");
        if (!newItemName.isEmpty())
        {
            newItemName.append(".");
        }
        origLib = pathList.takeFirst();
        origItemName = pathList.join(".");
        newItemName.append(pathList.takeLast());
    }
    else
    {
        origLib = pathList.takeFirst();
        destLib = origLib;
        origItemName = pathList.join(".");
        newItemName = origItemName;
    }
    if (destLib.isEmpty())
    {
        //lib is empty attempt to use the currently selected library
        destLib = GetSelectedLibraryName();
    }
    if (destLib.isEmpty() || newItemName.isEmpty())
    {
        return nullptr;
    }
    DockableLibraryTreeView* origLibDock = nullptr;
    DockableLibraryTreeView* destLibDock = nullptr;
    if (m_libraryTreeViews.contains(destLib))
    {
        destLibDock = m_libraryTreeViews[destLib];
    }
    if (destLibDock == nullptr)
    {
        return nullptr;
    }

    if (m_libraryTreeViews.contains(origLib))
    {
        origLibDock = m_libraryTreeViews[origLib];
    }
    if (origLibDock == nullptr)
    {
        return nullptr;
    }

    EditorUIPlugin::ScopedLibraryModifyUndoPtr modifyUndo;
    EBUS_EVENT_RESULT(modifyUndo, EditorLibraryUndoRequestsBus, AddScopedLibraryModifyUndo, AZStd::string(destLib.toUtf8().data()));

    // Validate Item Name
    QString newItemFullName = destLib + "." + newItemName;
    if (m_libraryManager->FindItemByName(newItemFullName) != nullptr)
    {
        newItemName = m_libraryManager->MakeUniqueItemName(newItemName, destLib);
    }
    
    CBaseLibraryItem* item = destLibDock->AddItem(newItemName);

    if (item == nullptr)
    {
        return nullptr;
    }

    if (!isVirtual)
    {
        emit SignalItemAdded(item, newItemName);
    }
    else
    {
        item->IsParticleItem = false;
    }
    item->SetName(newItemName.toUtf8().data());
    //sync from the engine to ensure the library tree view is accurate
    destLibDock->SyncFromEngine();
    // Library get modified after item added and synchronized. We will get treeitem agian
    // in case the data is corrupted
    CLibraryTreeViewItem* treeItem = destLibDock->GetItemFromName(newItemName);

    if (treeItem)
    {
        if (!isVirtual)
        {
            treeItem->SetVirtual(false);
        }
        else
        {
            treeItem->SetVirtual(true);
        }
        destLibDock->RefreshView();
        //reselect item via name as syncFromEngine breaks pointers
        treeItem = destLibDock->GetItemFromName(newItemName);
    }

    return treeItem;
}

//walk all libraries and store each one's selection data
void DockableLibraryPanel::StoreAllSelectionData()
{
    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
    {
        StoreSelectionData(iter.key());
    }
}

//walk all libraries and restore each one's selection data
void DockableLibraryPanel::RestoreAllSelectionData()
{
    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
    {
        RestoreSelectionData(iter.key());
    }
}

QString DockableLibraryPanel::GetLibExpandInfo(const QString libName)
{
    if (!m_libraryTreeViews.contains(libName))
    {
        return "";
    }
    return m_libraryTreeViews[libName]->StoreExpansionData();
}

QString DockableLibraryPanel::GetLibItemExpandInfo(const QString libName)
{
    if (!m_libraryTreeViews.contains(libName))
    {
        return "";
    }
    return m_libraryTreeViews[libName]->StoreExpandItems();
}

void DockableLibraryPanel::RestoreExpandItemInfoForLibrary(const QString libName, const QString data)
{
    if (m_libraryTreeViews.contains(libName))
    {
        m_libraryTreeViews[libName]->RestoreExpandItems(data);
    }
}

void DockableLibraryPanel::RestoreExpandLibInfoForLibrary(const QString libName, const QString data)
{
    if (m_libraryTreeViews.contains(libName))
    {
        m_libraryTreeViews[libName]->RestoreExpansionData(data);
    }
}

void DockableLibraryPanel::StoreSelectionData(const QString& libName)
{
    if (m_libraryTreeViews.contains(libName))
    {
        m_libraryTreeViews[libName]->StoreSelectionData();
    }
}

void DockableLibraryPanel::RestoreSelectionData(const QString& libName)
{
    if (m_libraryTreeViews.contains(libName))
    {
        m_libraryTreeViews[libName]->RestoreSelectionData();
    }
}

void DockableLibraryPanel::StoreAllExpandItemInfo()
{
    m_ItemExpandState.clear();
    m_libExpandData.clear();

    for (int i = 0; i < m_libraryManager->GetLibraryCount(); i++)
    {
        QString libName = m_libraryManager->GetLibrary(i)->GetName();
        DockableLibraryTreeView* treeview = nullptr;
        if (m_libraryTreeViews.contains(libName))
        {
            treeview = m_libraryTreeViews[libName];
        }
        if (treeview)
        {
            m_ItemExpandState.insert(libName, treeview->StoreExpandItems());
            m_libExpandData.insert(libName, treeview->GetCollapseState());
        }
    }   
}

void DockableLibraryPanel::RestoreAllExpandItemInfo()
{
    for (auto itemExpandData : m_ItemExpandState)
    {
        QString key = m_ItemExpandState.key(itemExpandData);
        if (m_libraryTreeViews[key])
        {
            m_libraryTreeViews[key]->RestoreExpandItems(itemExpandData);
        }
    }
    for (DockableLibraryTreeView* treeview : m_libraryTreeViews)
    {
        if (!treeview)
        {
            continue;
        }
        if (m_libExpandData[QString(treeview->GetLibrary()->GetName())])
        {
            treeview->Collapse();
        }
    }
}


//builds a list of all selected library items and returns it
QVector<CBaseLibraryItem*> DockableLibraryPanel::GetSelectedItems()
{
    QVector<CBaseLibraryItem*> results;
    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
    {
        QVector<CLibraryTreeViewItem*> tempList = (*iter)->GetSelectedItems();
        while (tempList.count() > 0)
        {
            results.push_back(tempList.takeFirst()->GetItem());
        }
    }
    return results;
}

bool DockableLibraryPanel::RemoveItemFromLibrary(QString overrideSelection /*= ""*/, bool overrideSafety /*= false*/)
{
    QVector<CLibraryTreeViewItem*> selection = GetActionItems(overrideSelection);
    QVector<CLibraryTreeViewItem*>  cpoSelectedLibraryItems = GetTreeItemsAndChildrenOf(selection);
    //don't prompt if there's nothing to delete
    if (cpoSelectedLibraryItems.isEmpty())
    {
        return false;
    }

    //QMessageBox::StandardButton reply;
    int reply;
    QString message;
    if (!overrideSafety && UIFactory::GetQTUISettings()->m_deletionPrompt)
    {
        // make message box that verifies deletion
        QString strMessageString(tr("Delete the following item(s)?\n"));
        for (auto iter = cpoSelectedLibraryItems.begin(); iter != cpoSelectedLibraryItems.end(); ++iter)
        {
            strMessageString.append((*iter)->GetPath());
            strMessageString.append("\n");
        }


        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Delete Confirmation"));
        msgBox.setText(QString(strMessageString));
        QCheckBox dontPrompt(QObject::tr("Do not prompt again"), &msgBox);

        msgBox.setCheckBox(&dontPrompt);

        msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        reply = msgBox.exec();

        UIFactory::GetQTUISettings()->m_deletionPrompt = !dontPrompt.checkState();
    }
    else
    {
        reply = QMessageBox::Yes;
    }
    if (reply == QMessageBox::Yes)
    {
        EditorUIPlugin::ScopedBatchUndoPtr batchUndo;
        EBUS_EVENT_RESULT(batchUndo, EditorLibraryUndoRequestsBus, AddScopedBatchUndo, "remove items");

        while (cpoSelectedLibraryItems.count() > 0)
        {
            //Delete items child first so that we don't try to double delete children.
            CLibraryTreeViewItem* item = cpoSelectedLibraryItems.takeLast();
            QString itemName = item->GetFullPath();

            EditorUIPlugin::ScopedLibraryModifyUndoPtr modifyUndo;
            AZStd::string libName = item->GetLibraryName().toUtf8().data();
            EBUS_EVENT_RESULT(modifyUndo, EditorLibraryUndoRequestsBus, AddScopedLibraryModifyUndo, libName);

            emit SignalItemDeleted(itemName);
            m_libraryManager->DeleteItem(item->GetItem());
        }
        GetIEditor()->SetModifiedFlag();
        cpoSelectedLibraryItems.clear();
        //Send a select nullptr signal, so that preview and attribute get updated after deletion
        emit SignalItemSelected(nullptr);
        return true;
    }
    cpoSelectedLibraryItems.clear();
    return false;
}

void DockableLibraryPanel::ForceLibrarySync(const QString& name)
{
    if (m_libraryTreeViews.contains(name))
    {
        m_libraryTreeViews[name]->SyncFromEngine();
    }
}

void DockableLibraryPanel::PassThroughTreeFilledFromLibrary(IDataBaseLibrary* lib, CLibraryTreeView* view)
{
    emit SignalTreeFilledFromLibrary(lib, view);
}

//we only show one library at a time now, so set all the library tree views to the new size
void DockableLibraryPanel::resizeEvent(QResizeEvent* e)
{
    CRY_ASSERT(m_dockableArea);
    QSize nextSize = e->size();
    if (titleBarWidget())
    {
        nextSize.setHeight(nextSize.height() - titleBarWidget()->height());
    }
    m_dockableArea->resize(nextSize);
    blockSignals(true);
    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
    {
        (*iter)->resize(nextSize);
    }
    blockSignals(false);
}

void DockableLibraryPanel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        //Click on empty place, clear selection
        for (DockableLibraryTreeView* tree : m_libraryTreeViews)
        {
            tree->ResetSelection();
        }
    }
    return QDockWidget::mousePressEvent(event);
}

void DockableLibraryPanel::SetItemEnabled(const QString& libName, const QString& itemName, bool val)
{
    if (m_libraryTreeViews.contains(libName))
    {
        CLibraryTreeViewItem* item = m_libraryTreeViews[itemName]->GetItemFromName(itemName);
        if (item)
        {
            item->SetEnabled(val);
        }
    }
}

void DockableLibraryPanel::PassThroughItemAboutToBeDragged(CLibraryTreeViewItem* item)
{
    emit SignalItemAboutToBeDragged(item);
}

void DockableLibraryPanel::PassThroughDragOperationFinished()
{
    emit SignalDragOperationFinished();
}

void DockableLibraryPanel::OnSearchFilterChanged(const QString& searchFilter)
{
    if (searchFilter.isEmpty())
    {
        //show all
        for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
        {
            iter.value()->RemoveFilter();
        }
        m_searchField->clearEditText();
    }

    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
    {
        (*iter)->ShowOnlyStringsMatching(searchFilter);
    }
}

QVector<DockableLibraryTreeView*> DockableLibraryPanel::GetVisibleLibraries()
{
    QVector<DockableLibraryTreeView*> result;

    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
    {
        if (iter.value() && !iter.value()->isHidden())
        {
            result.append(iter.value());
        }
    }

    return result;
}

void DockableLibraryPanel::OnLibraryToggleButtonPushed()
{
    AddLibrary();
}

void DockableLibraryPanel::PassThroughItemEnableStateChanged(CBaseLibraryItem* item, const bool& state)
{
    emit SignalItemEnableStateChanged(item, state);
}

void DockableLibraryPanel::PassThroughItemAdded(CBaseLibraryItem* item, const QString& name)
{
    emit SignalItemAdded(item, name);
}

void DockableLibraryPanel::LibraryRenamed(const QString& lib, const QString& title)
{
    if (title != lib) // Only Rename when new name is different from old one
    {
        CBaseLibrary* library = static_cast<CBaseLibrary*>(m_libraryManager->FindLibrary(lib));
        if (library)
        {
            bool renamed = m_libraryManager->SetLibraryName(library, title.toUtf8().data());
            RebuildFromEngineData();
            if (!renamed)
            {
                //let user rename again
                QMessageBox::warning(this, tr("Error!"), tr("The library '%1' already exists.").arg(title));
                m_libraryTreeViews[lib]->RenameLibrary();
                return;
            }
        }
        else
        {
            return;
        }
    }

    //undo for add/rename or duplicate/rename library, we don't support rename library function
    EBUS_EVENT(EditorLibraryUndoRequestsBus, AddLibraryCreateUndo, title.toUtf8().data());
}

void DockableLibraryPanel::OnLibraryTitleValidationCheck(const QString& str, bool& isValid)
{
    isValid = false;
    if (m_libraryManager->FindLibrary(str.toUtf8().data()) || 
        m_libraryTreeViews.contains(str)) 
    {
        QMessageBox::warning(this, tr("Error!"), tr("The library '%1' already exists.").arg(str));
        return;
    }
    isValid = true;
}

void DockableLibraryPanel::PassThroughDecorateDefaultView(const QString& lib, DefaultViewWidget* view)
{
    emit SignalDecorateDefaultView(lib, view);
}

QVector<CLibraryTreeViewItem*> DockableLibraryPanel::GetSelectedTreeItems()
{
    QVector<CLibraryTreeViewItem*> results;
    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
    {
        results += (*iter)->GetSelectedItems();
    }
    return results;
}

QVector<CLibraryTreeViewItem*> DockableLibraryPanel::GetSelectedTreeItemsWithChildren()
{
    QVector<CLibraryTreeViewItem*> results;
    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
    {
        results += (*iter)->GetSelectedTreeItemsWithChildren();
    }
    return results;
}

QVector<CBaseLibraryItem*> DockableLibraryPanel::GetSelectedItemsWithChildren()
{
    QVector<CBaseLibraryItem*> results;
    for (auto iter = m_libraryTreeViews.begin(); iter != m_libraryTreeViews.end(); iter++)
    {
        results += (*iter)->GetSelectedItemsWithChildren();
    }
    return results;
}

void DockableLibraryPanel::OnStartDragRequested(QDrag* drag, Qt::DropActions supportedActions)
{
    QString dragData = "";
    QVector<CLibraryTreeViewItem*> selection = GetSelectedTreeItemsWithChildren();
    QRect rect;
    for (unsigned int i = 0; i < selection.count(); i++)
    {
        dragData.append(selection[i]->ToString());
        if (selection.count() != 1 && i < selection.count() - 1)
        {
            dragData.append(QTUI_UTILS_NAME_SEPARATOR);
        }
    }
    QMimeData* mimeData = drag->mimeData();
    mimeData->setData("LibraryTreeData", dragData.toUtf8());
    supportedActions |= Qt::MoveAction;
    drag->exec(supportedActions);
    RebuildFromEngineData();
}

void DockableLibraryPanel::OnDockLocationChanged(DockableLibraryTreeView* dock)
{
    //don't modify library order if it's loading library
    if (m_loadingLibrary)
    {
        return;
    }

    if (!dock)
    {
        return;
    }
    QPoint p = dock->pos();
    unsigned int newLocation = m_libraryTreeViews.count() - 1;
    for (QMap<QString, DockableLibraryTreeView*>::iterator itr = m_libraryTreeViews.begin(); itr != m_libraryTreeViews.end(); itr++)
    {
        QPoint cp = (*itr)->pos();
        if (cp.y() < p.y())
        {
            newLocation--;
        }
    }

    //undo command
    EditorUIPlugin::ScopedLibraryMoveUndoPtr moveUndo;
    EBUS_EVENT_RESULT(moveUndo, EditorLibraryUndoRequestsBus, AddScopedLibraryMoveUndo, AZStd::string(dock->GetLibrary()->GetName().toUtf8().data()));

    m_libraryManager->ChangeLibraryOrder(dock->GetLibrary(), newLocation);
}

QString DockableLibraryPanel::GetLastAddedItemName()
{
    return m_lastAddedItem;
}

QVector<CBaseLibraryItem*> DockableLibraryPanel::GetChildrenOfItem(CBaseLibraryItem* item)
{
    if (item)
    {
        QString libName = item->GetLibrary()->GetName();
        if (m_libraryTreeViews.contains(libName))
        {
            return m_libraryTreeViews[libName]->GetChildrenOfItem(item->GetName());
        }
    }
    return QVector<CBaseLibraryItem*>();
}

void DockableLibraryPanel::RegisterActions()
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
    IEditorParticleUtils* utils = GetIEditor()->GetParticleUtils();

    // Rename Action
    QAction* action = new QAction(this);
    action->setShortcut(utils->HotKey_GetShortcut("Edit Menu.Rename"));
    connect(action, &QAction::triggered, this, [=]() { ActionRename(); });
    addAction(action);

    // Delete Action
    action = new QAction(this);
    action->setShortcut(utils->HotKey_GetShortcut("Edit Menu.delete"));
    connect(action, &QAction::triggered, this, [=]() { ActionDelete(); });
    addAction(action);

    ///////////////////////////////////////////
    // Library Action, Save All Libraries
    action = new QAction(this);
    action->setShortcut(utils->HotKey_GetShortcut("File Menu.Save"));
    connect(action, &QAction::triggered, this, [=]()
        {
            m_libraryManager->SaveAllLibs();
            emit SignalSaveAllLibs();
        });
    addAction(action);


    ///////////////////////////////////////////
    // Library Action, Import Library
    action = new QAction(this);
    action->setShortcut(utils->HotKey_GetShortcut("File Menu.Import"));
    connect(action, &QAction::triggered, this, [=]() { ImportLibrary(PromptImportLibrary()); });
    addAction(action);

    ///////////////////////////////////////////
    // Library Action, Add Library
    action = new QAction(this);
    action->setShortcut(utils->HotKey_GetShortcut("File Menu.Create new library"));
    connect(action, &QAction::triggered, this, [=]() {
        AddLibrary();
    });
    addAction(action);

    ///////////////////////////////////////////
    //open in new tab action
    action = new QAction(this);
    connect(action, &QAction::triggered, this, [=]() { ActionOpenInNewTab(); });
    addAction(action);
}

void DockableLibraryPanel::UnregisterActions()
{
    for (auto action : actions())
    {
        removeAction(action);
        action->setParent(nullptr);
        SAFE_DELETE(action);
    }
}

QVector<CLibraryTreeViewItem*> DockableLibraryPanel::GetTreeItemsAndChildrenOf(const QVector<CLibraryTreeViewItem*>& selection)
{
    //we get the children similar to how they're gotten in CLibraryTreeView::GetSelectedTreeItemsWithChildren()
    //this ensures we have one copy of each item and that they're alphabetical which will group parents with children, parent first.
    QMap<QString, CLibraryTreeViewItem*> resultMap;
    QVector<CLibraryTreeViewItem*> resultingVector;
    //get ordered list of selection with children
    for (auto selectedParent : selection)
    {
        if (!m_libraryTreeViews.contains(selectedParent->GetLibraryName()))
        {
            continue;
        }
        DockableLibraryTreeView* libDock = m_libraryTreeViews[selectedParent->GetLibraryName()];
        QVector<CLibraryTreeViewItem*> children = libDock->GetChildrenOfTreeItem(selectedParent->GetPath());
        while (children.count() > 0)
        {
            CLibraryTreeViewItem* child = children.takeFirst();
            if (!resultMap.contains(child->GetFullPath()))
            {
                resultMap.insert(child->GetFullPath(), child);
            }
        }
    }
    //build and return resulting vector.
    for (auto pair = resultMap.begin(); pair != resultMap.end(); pair++)
    {
        resultingVector.append((*pair));
    }

    return resultingVector;
}

void DockableLibraryPanel::ActionDelete(const QString& overrideSelection /*= ""*/, const bool overrideSafety /*= false*/)
{
    if (RemoveItemFromLibrary(overrideSelection, overrideSafety))
    {
        RebuildFromEngineData();
    }
}

void DockableLibraryPanel::ActionRename(const QString& overrideSelection /* = ""*/, const bool overrideSafety /*= false*/)
{
    QVector<CLibraryTreeViewItem*> selection = GetActionItems(overrideSelection);

    if (selection.count() == 0 || selection.count() > 1)
    {
        return;
    }

    CLibraryTreeViewItem* item = selection.first();
    QString libName = item->GetLibraryName();
    DockableLibraryTreeView* libDock = nullptr;
    if (!m_libraryTreeViews.contains(libName) || libName.isEmpty())
    {
        libDock = m_libraryTreeViews[GetSelectedLibraryName()];
        if (!libDock)
        {
            //no lib selected return nullptr
            return;
        }
    }
    else
    {
        libDock = m_libraryTreeViews[libName];
    }
    if (item)
    {
        libDock->RenameItem(item);
    }
}

void DockableLibraryPanel::ActionExportLib(const QString& librarySelection /* = ""*/)
{
    QString lib = librarySelection;
    if (librarySelection.isEmpty())
    {
        lib = GetSelectedLibraryName();
    }
    ExportLibrary(lib);
}

QVector<CLibraryTreeViewItem*> DockableLibraryPanel::GetActionItems(const QString& overrideSelection)
{
    //if overrideSelection is part of selecteditems then use all selected items
    if (!overrideSelection.contains(QTUI_UTILS_NAME_SEPARATOR))
    {
        QVector<CLibraryTreeViewItem*> currentSelection = GetSelectedTreeItems();
        if (overrideSelection.isEmpty())
        {
            return currentSelection;
        }
        for (unsigned int i = 0, _e = currentSelection.count(); i < _e; i++)
        {
            if (currentSelection[i]->GetFullPath().compare(overrideSelection) == 0)
            {
                return currentSelection;
            }
        }
    }
    QStringList splitSelection = overrideSelection.split(QTUI_UTILS_NAME_SEPARATOR);
    QVector<CLibraryTreeViewItem*> result;
    while (splitSelection.count() > 0)
    {
        CLibraryTreeViewItem* overrideItem = GetTreeItemFromPath(splitSelection.takeFirst());
        if (overrideItem && !result.contains(overrideItem))
        {
            result += overrideItem;
        }
    }
    return result;
}

void DockableLibraryPanel::RenameLibraryItem(const QString& fullNameOfItem, const QString& newPath, const bool overrideSafety /*= false*/)
{
    //get library name then rename item
    QString libName = "";
    QString itemNameWithoutLib = "";
    {
        QStringList splitPath = fullNameOfItem.split('.');
        libName = splitPath.takeFirst();
        itemNameWithoutLib = splitPath.join('.');
    }
    if (m_libraryTreeViews.contains(libName))
    {
        CLibraryTreeViewItem* item = m_libraryTreeViews[libName]->GetItemFromName(itemNameWithoutLib);
        m_libraryTreeViews[libName]->RenameItem(item, newPath, overrideSafety);
    }
}

void DockableLibraryPanel::RemapHotKeys()
{
    UnregisterActions();
    RegisterActions();
}

void DockableLibraryPanel::ActionOpenInNewTab(const QString& overrideSelection)
{
    QVector<CLibraryTreeViewItem*> selection = GetActionItems(overrideSelection);

    if (selection.count() == 0)
    {
        return;
    }

    for (CLibraryTreeViewItem* item : selection)
    {
        emit SignalOpenInNewTab(item->GetItem());
    }
}

void DockableLibraryPanel::ActionMoveOneFolder(const QString& destPath, const QString& overrideSelection /*= ""*/)
{
    // Use Full path instead of tree item. In case item is get modified.
    QString fullPath = overrideSelection;
    QStringList pathList = fullPath.split(".");
    QString libName = pathList.takeFirst();
    QString itemName = pathList.join(".");
    CLibraryTreeViewItem* item = m_libraryTreeViews[libName]->GetItemFromName(itemName);
    if (!item)
    {
        return;
    }
    bool isVirtual = item->IsVirtualItem();
    // Add a duplicate of the item to new position
    AddDuplicateTreeItem(fullPath, isVirtual, destPath);
}


void DockableLibraryPanel::ActionAddItem(const QString& librarySelection)
{
    QString libraryName = GetSelectedLibraryName();
    QString addToPath = "";
    CLibraryTreeViewItem* item = nullptr;
    QVector<CLibraryTreeViewItem*> selection = GetActionItems(librarySelection);
    if (selection.size() > 0)
    {
        item = selection.first();
        addToPath = item->GetPath();
        libraryName = item->GetLibraryName();
    }
    else if (!librarySelection.isEmpty()) // librarySelection  might override the library
    {
        QStringList splitName = librarySelection.split('.');
        if (splitName.size() > 0)
        {
            QString overrideLibName = splitName.takeFirst();
            if (!overrideLibName.isEmpty())
            {
                libraryName = overrideLibName;
            }
        }
    }
    DockableLibraryTreeView* libDock = nullptr;
    if (m_libraryTreeViews.contains(libraryName))
    {
        libDock = m_libraryTreeViews[libraryName];
    }
    if (libDock)
    {
        //start a undo batch because it may include create and rename
        EBUS_EVENT(EditorLibraryUndoRequestsBus, BeginUndoBatch, "Add Item");

        //library modification undo and disable other undo's
        EditorUIPlugin::ScopedLibraryModifyUndoPtr modifyUndo;
        EBUS_EVENT_RESULT(modifyUndo, EditorLibraryUndoRequestsBus, AddScopedLibraryModifyUndo, AZStd::string(libraryName.toUtf8().data()));
        EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
        EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

        m_lastAddedItem = libDock->AddLibItem(addToPath);
        if (!m_lastAddedItem.isEmpty()) // if the item is added correctly
        {
            EBUS_EVENT(LibraryItemCacheRequestsBus, UpdateItemCache, (m_lastAddedItem).toUtf8().data());
            ActionRename(m_lastAddedItem);
        }
        else
        {
            //end batch since no rename happened
            EBUS_EVENT(EditorLibraryUndoRequestsBus, EndUndoBatch);
        }
    }
}

void DockableLibraryPanel::ActionAddFolder(const QString& librarySelection)
{
    QString libraryName = GetSelectedLibraryName();
    QString shortParentPath = "";
    CLibraryTreeViewItem* item = nullptr;
    QVector<CLibraryTreeViewItem*> selection = GetActionItems(librarySelection);
    if (selection.size() > 0)
    {
        item = selection.first();
        if (!item->IsVirtualItem()) // Folder could not be added under a particle item
        {
            QMessageBox::warning(this, "Warning", tr("Folders cannot be added under particle items\n"), QMessageBox::Button::Close);
            return;
        }
        else
        {
            shortParentPath = item->GetPath();
            libraryName = item->GetLibraryName();
        }
    }
    else if (!librarySelection.isEmpty()) // librarySelection  might override the library
    {
        QStringList splitName = librarySelection.split('.');
        if (splitName.size() > 0)
        {
            QString overrideLibName = splitName.takeFirst();
            if (!overrideLibName.isEmpty())
            {
                libraryName = overrideLibName;
            }
        }
    }

    DockableLibraryTreeView* libDock = nullptr;
    if (m_libraryTreeViews.contains(libraryName))
    {
        libDock = m_libraryTreeViews[libraryName];
    }
    if (libDock)
    { 
        //start a undo batch because it may include create and rename
        EBUS_EVENT(EditorLibraryUndoRequestsBus, BeginUndoBatch, "Add Folder");

        //library modification undo and disable other undo's
        EditorUIPlugin::ScopedLibraryModifyUndoPtr modifyUndo;
        EBUS_EVENT_RESULT(modifyUndo, EditorLibraryUndoRequestsBus, AddScopedLibraryModifyUndo, AZStd::string(libraryName.toUtf8().data()));
        EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
        EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

        QString newFolderFullName = libDock->AddLibFolder(shortParentPath);
        if (!newFolderFullName.isEmpty()) // if the folder is added correctly
        {
            ActionRename(newFolderFullName);
        }
        else
        {
            //end batch since no rename happened
            EBUS_EVENT(EditorLibraryUndoRequestsBus, EndUndoBatch);
        }
    }
}

EditorUIPlugin::LibTreeExpandInfo DockableLibraryPanel::GetLibTreeExpandInfo(const AZStd::string& libId)
{
    EditorUIPlugin::LibTreeExpandInfo expandInfo;
    expandInfo.LibState = GetLibExpandInfo(libId.c_str()).toStdString().data();
    expandInfo.ItemState = GetLibItemExpandInfo(libId.c_str()).toStdString().data();
    return expandInfo;
}

void DockableLibraryPanel::LoadLibTreeExpandInfo(const AZStd::string& libId, const EditorUIPlugin::LibTreeExpandInfo& expandInfo)
{
    RestoreExpandLibInfoForLibrary(libId.c_str(), expandInfo.LibState.c_str());
    RestoreExpandItemInfoForLibrary(libId.c_str(), expandInfo.ItemState.c_str());
}

#include <DockableLibraryPanel.moc>
