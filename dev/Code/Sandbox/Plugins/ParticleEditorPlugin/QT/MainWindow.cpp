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

// QT
#include <QKeySequence>
#include <QSettings>
#include <QFile>
#include <QMenuBar>
#include <qfiledialog.h>
#include <QMessageBox>
#include <QShortcut>
#include <qxmlstream.h>

//Editor
#include <EditorDefs.h>
#include <IEditorParticleUtils.h>
#include <Include/IEditorParticleManager.h>
#include <Undo/UndoVariableChange.h>
#include <Clipboard.h>
#include "../../../../CryEngine/CryCommon/ParticleParams.h"
#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <MathConversion.h>
#include <Viewport.h>
#include <LmbrCentral/Rendering/ParticleComponentBus.h>

//EditorCore
#include <Undo/Undo.h>

//EditorUI_QT
#include <DockableLibraryPanel.h>
#include <DockablePreviewPanel.h>
#include <Utils.h>
#include <VariableWidgets/QCustomColorDialog.h>
#include <VariableWidgets/QKeySequenceEditorDialog.h>
#include <VariableWidgets/QGradientColorDialog.h>
#include <VariableWidgets/QColorEyeDropper.h>
#include <VariableWidgets/QCopyModalDialog.h>
#include <UIFactory.h>
#include <LibraryTreeViewItem.h>
#include <LibraryTreeView.h>
#include <DefaultViewWidget.h>
#include <AttributeItem.h>
#include <Undo/EditorLibraryUndoManager.h>

//Local
#include "MainWindow.h"
#include "ParticleEditorPlugin.h"
#include "DockableAttributePanel.h"
#include "DockableLODPanel.h"
#include "DockableParticleLibraryPanel.h"
#include "QAttributePresetWidget.h"

#define PARTICLE_EDITOR_DATA_SEPARATOR '~'
#define MAX_NUMBER_OF_LAYOUT_MENU 10
#define SETTINGS_LAYOUT_MENU_GROUP "LayoutMenu/"
#define SETTINGS_LAYOUT_MENU_SIZE_KEY "LayoutMenuSize"
#define PARTICLE_EDITOR_LAYOUT_KEY "LayoutParticleEditor"
#define PARTICLE_EDITOR_SETTINGS_KEY "EditorSettingsParticleEditor"

////////////////////////////////////////////////////////////////
//These are default options ...
static int s_width = 800;
static int s_height = 600;
static int s_x = 0;
static int s_y = 0;
////////////////////////////////////////////////////////////////

#define STYLESHEET_PATH_LIGHT "Editor/Styles/EditorStyleSheet.qss"
#define STYLESHEET_PATH_DARK "Editor/Styles/stylesheet_Dark.qss"
#define DEFAULT_PARTICLE_EDITOR_LAYOUT_PATH "Editor/Default.editor_layout"

CMainWindow::CMainWindow()
    : m_libraryTreeViewDock(nullptr)
    , m_attributeViewDock(nullptr)
    , m_previewDock(nullptr)
    , m_libraryMenu(nullptr)
    , m_layoutMenu(nullptr)
    , m_showLayoutMenu(nullptr)
    , m_viewMenu(nullptr)
    , m_fileMenu(nullptr)
    , m_editMenu(nullptr)
    , m_libraryMenuActionGroup(nullptr)
    , m_menuBar(nullptr)
    , m_bIsRefreshing(false)
    , m_RequestedClose(false)
    , m_isFirstSceneSinceLaunch(true)
    , m_iRefreshDelay(-1)
    , m_preset(nullptr)
    , m_stylesheetPreprocessor(new AzQtComponents::StylesheetPreprocessor(this))
    , m_needLibraryRefresh(false)
    , m_requireLayoutReload(false)
{
    setWindowTitle(tr("Particle Editor"));

    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());


    GetIEditor()->GetParticleUtils()->HotKey_LoadExisting();

    //Move to last location.
    move(s_x, s_y);

    //Size to last size.
    resize(s_width, s_height);

    CreateDockWindows();
    CreateMenuBar();
    CreateShortcuts();

    //Save the editor layout for reset before we load any user settings
    m_defaultEditorLayout = this->saveState(0);

    GetIEditor()->RegisterNotifyListener(this);

    // Push an event onto the end of the event queue to load our state
    // It has to be on the end of the event queue, so that our layout
    // restore happens after the QtViewPaneManager does it's restore
    QTimer::singleShot(0, this, [this]()
        {
            StateLoad();

            UpdatePalette();

            GetIEditor()->GetParticleUtils()->ToolTip_LoadConfigXML(QString("Editor\\Plugins\\ParticleEditorPlugin\\settings\\ToolTips.xml"));

            RegisterActions();

            m_layoutMenu = new QMenu();
            StateLoad_layoutMenu();
            View_PopulateMenu();
        });

    m_undoManager = new EditorUIPlugin::EditorLibraryUndoManager(GetIEditor()->GetParticleManager());

    LibraryItemUIRequests::Bus::Handler::BusConnect();
    LibraryChangeEvents::Bus::Handler::BusConnect();
}

CMainWindow::~CMainWindow(void)
{
    LibraryChangeEvents::Bus::Handler::BusDisconnect();
    LibraryItemUIRequests::Bus::Handler::BusDisconnect();

    CRY_ASSERT(GetIEditor());

    //Store my size and position for later.
    s_width = width();
    s_height = height();
    QPoint position = pos();
    s_x = position.x();
    s_y = position.y();

    SAFE_DELETE(m_undoManager);
    
    GetIEditor()->UnregisterNotifyListener(this);
    GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);

    for (unsigned int i = 0; i < m_shortcuts.count(); i++)
    {
        delete m_shortcuts[i];
    }
    m_shortcuts.clear();

    SAFE_DELETE(m_libraryMenuActionGroup);
    SAFE_DELETE(m_libraryTreeViewDock);
    SAFE_DELETE(m_attributeViewDock);
    SAFE_DELETE(m_previewDock);
    SAFE_DELETE(m_menuBar);
    SAFE_DELETE(m_libraryMenu);
    SAFE_DELETE(m_layoutMenu);
    SAFE_DELETE(m_showLayoutMenu);
    SAFE_DELETE(m_viewMenu);
    SAFE_DELETE(m_fileMenu);
    SAFE_DELETE(m_editMenu);
    SAFE_DELETE(m_stylesheetPreprocessor);
}

const GUID& CMainWindow::GetClassID()
{
    // {19C5E05B-2AEE-497E-9ABA-BE4DEF9112B9}
    static const GUID guid = {
        0x19c5e05b, 0x2aee, 0x497e, { 0x9a, 0xba, 0xbe, 0x4d, 0xef, 0x91, 0x12, 0xb9 }
    };
    return guid;
}

void CMainWindow::CreateMenuBar()
{
    //Setup menu bar
    m_menuBar = new QMenuBar(this);
    QAction* action = nullptr;
#define ADD_ACTION(text, callback, tooltip, icon, key)                               \
    {                                                                                \
        action = menu->addAction(QIcon("Editor/UI/Icons/toolbar/" icon), tr(text)); \
        if (key != 0) {                                                              \
            action->setShortcut(key); }                                              \
        action->setToolTip(tr(tooltip));                                             \
        connect(action, &QAction::triggered, this, [=]() {                           \
                if (GetIEditor()->GetParticleUtils()->HotKey_IsEnabled())            \
                {                                                                    \
                    this->callback();                                                \
                }                                                                    \
            });                                                                      \
    }
#define ADD_ACTION_MANUAL(text, tooltip, icon, key)                                  \
    {                                                                                \
        action = menu->addAction(QIcon("Editor/UI/Icons/toolbar/" icon), tr(text)); \
        if (key != 0) {                                                              \
            action->setShortcut(key); }                                              \
        action->setToolTip(tr(tooltip));                                             \
    }
    //Use this to make sure that the action is only enabled when an item is selected in the library
#define SETUP_LIBRARY_ACTION()                   \
    {                                            \
        action->setEnabled(false);               \
        m_itemSelectedActions.push_back(action); \
    }

    // File Menu
    m_fileMenu = new QMenu(tr("File"));
    File_PopulateMenu();
    m_menuBar->addMenu(m_fileMenu);

    // Edit Menu
    m_editMenu = new QMenu(tr("Edit"));
    Edit_PopulateMenu();
    connect(m_editMenu, &QMenu::aboutToShow, this, [this] { UpdateEditActions(true); });
    connect(m_editMenu, &QMenu::aboutToHide, this, [this] { UpdateEditActions(false);});
    m_menuBar->addMenu(m_editMenu);

    // View Menu
    m_viewMenu = new QMenu(tr("View"));
    connect(m_viewMenu, &QMenu::aboutToShow, this, &CMainWindow::View_PopulateMenu);
    m_menuBar->addMenu(m_viewMenu);

    setMenuBar(m_menuBar);
#undef ADD_ACTION
#undef SETUP_LIBRARY_ACTION
}

void CMainWindow::UpdateEditActions(bool menuShown)
{
    if (menuShown)
    {
        // Undo
        bool hasUndo = false;
        EBUS_EVENT_RESULT(hasUndo, EditorLibraryUndoRequestsBus, HasUndo);
        m_undoAction->setEnabled(hasUndo);

        // Redo
        bool hasRedo = false;
        EBUS_EVENT_RESULT(hasRedo, EditorLibraryUndoRequestsBus, HasRedo);
        m_redoAction->setEnabled(hasRedo);

        // Copy
        QVector<CBaseLibraryItem*> selectedItems = m_libraryTreeViewDock->GetSelectedItems();
        auto firstSelectedItem = selectedItems.isEmpty() ? nullptr : selectedItems.first();
        m_copyAction->setEnabled(firstSelectedItem);

        // Duplicate
        m_duplicateAction->setEnabled(firstSelectedItem && selectedItems.count() == 1);

        // Add LOD
        bool enable = false;
        if (firstSelectedItem)
        {
            auto pParticle = static_cast<CParticleItem*>(firstSelectedItem);
            enable = pParticle->IsParticleItem && m_libraryTreeViewDock->GetTreeItemFromPath(QString(selectedItems.first()->GetFullName())) != nullptr;
        }
        m_addLODAction->setEnabled(enable);

        // Others
        m_resetSelectedItem->setEnabled(firstSelectedItem);
        m_deleteSelectedItem->setEnabled(firstSelectedItem);
        m_renameSelectedItem->setEnabled(firstSelectedItem);
    }
    else
    {
        // All actions need to be enabled when menu is hidden, so they can respond to shortcuts
        m_undoAction->setEnabled(true);
        m_redoAction->setEnabled(true);
        m_copyAction->setEnabled(true);
        m_duplicateAction->setEnabled(true);
        m_resetSelectedItem->setEnabled(true);
        m_deleteSelectedItem->setEnabled(true);
        m_renameSelectedItem->setEnabled(true);
    }
}

void CMainWindow::OnEditorNotifyEvent(EEditorNotifyEvent e)
{
    // Do this before the early return because the libraries need to be refreshed regardless of whether
    // the editor is visible at the time. If it's not visible, the actual refresh will happen when
    // the editor is shown.
    if (eNotify_OnBeginNewScene == e || eNotify_OnEndSceneOpen == e || eNotify_OnDataBaseUpdate == e || eNotify_OnSceneClosed == e)
    {
        m_needLibraryRefresh = true;
    }

    if (!this->isVisible())
    {
        return;
    }

    switch (e)
    {
    case eNotify_OnBeginNewScene:
        {
            m_undoManager->Reset();
            if (m_isFirstSceneSinceLaunch)
            {
                m_AutoRecovery.AttemptRecovery();
            }

            m_isFirstSceneSinceLaunch = false;

            RefreshLibraries();

            break;
        }
    case eNotify_OnEndSceneOpen:
        {
            m_undoManager->Reset();
            if (m_isFirstSceneSinceLaunch)
            {
                m_AutoRecovery.AttemptRecovery();
            }

            m_isFirstSceneSinceLaunch = false;

            RefreshLibraries();

            break;
        }
    case eNotify_OnIdleUpdate:
        {
            // Run invoke queue
            Utils::runInvokeQueue();

            if (m_RequestedClose)
            {
                CRY_ASSERT(GetIEditor());
                m_RequestedClose = false;
                bool userDidNotCancelClose = OnCloseWindowCheck();
                if (userDidNotCancelClose)
                {
                    CleanupOnClose();
                    GetIEditor()->CloseView(CParticleEditorPlugin::m_RegisteredQtViewPaneName);
                }
            }
            break;
        }
    case eNotify_OnStyleChanged:
        {
            UpdatePalette();
            break;
        }
    case eNotify_OnDataBaseUpdate:
        {
            if (m_isFirstSceneSinceLaunch)
            {
                m_AutoRecovery.AttemptRecovery();
            }

            m_isFirstSceneSinceLaunch = false;

            RefreshLibraries();

            break;
        }
    case eNotify_OnSceneClosed:
        RefreshLibraries();
        break;
    case eNotify_OnBeginLoad:
        //disable input during level loading
        setEnabled(false);
        break;
    case eNotify_OnEndLoad:
        //enable input
        setEnabled(true);
        break;
    default:
        break;
    }
}

void CMainWindow::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    if (event == ESYSTEM_EVENT_LEVEL_UNLOAD)
    {
        PrepareForParticleSystemUnload();
    }
}

void CMainWindow::PrepareForParticleSystemUnload()
{
    m_libraryTreeViewDock->ResetSelection();
    m_attributeViewDock->CloseAllTabs();
}

void CMainWindow::CreateDockWindows()
{
    m_preset = new QAttributePresetWidget(this);
    m_preset->hide();

    setObjectName("dwParticleMain");
    // Enable Dock Nesting so that windows can be configured in better configurations.
    setDockNestingEnabled(true);

    //////////////////////////////////////////////////////////////////////////////////
    // * Tree View
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleManager());
    m_libraryTreeViewDock = new DockableParticleLibraryPanel(this);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalDecorateDefaultView, this, &CMainWindow::Library_DecorateTreesDefaultView, Qt::DirectConnection);
    m_libraryTreeViewDock->Init(tr("Libraries"), GetIEditor()->GetParticleManager());
    m_libraryTreeViewDock->SetHotkeyHandler(m_libraryTreeViewDock, [&](QKeyEvent* e){ keyPressEvent(e); }); //Routing this to MainWindow to handle the global items
    m_libraryTreeViewDock->SetShortcutHandler(m_libraryTreeViewDock, [&](QShortcutEvent* e){ return ShortcutEvent(e); }); //Routing this to MainWindow to handle the global items
    m_libraryTreeViewDock->setObjectName("dwLibraryTreeView");
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalPopulateTitleBarMenu, this, &CMainWindow::Library_PopulateTitleBarMenu);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalPopulateItemContextMenu, this, &CMainWindow::Library_PopulateItemContextMenu);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalPopulateLibraryContextMenu, this, &CMainWindow::Library_PopulateLibraryContextMenu);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemAboutToBeRenamed, this, &CMainWindow::Library_ItemNameValidationRequired);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemRenamed, this, &CMainWindow::Library_ItemRenamed);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemCheckLod, this, &CMainWindow::Library_UpdateTreeItemStyle);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemAdded, this, &CMainWindow::Library_ItemAdded);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalTreeFilledFromLibrary, this, &CMainWindow::Library_TreeFilledFromLibrary);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemAboutToBeDragged, this, &CMainWindow::Library_ItemDragged);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalDragOperationFinished, this, &CMainWindow::Library_DragOperationFinished);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemEnableStateChanged, this, &CMainWindow::Library_ItemEnabledStateChanged);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalDuplicateItem, this, &CMainWindow::Library_ItemDuplicated);
    connect(m_libraryTreeViewDock, &DockableParticleLibraryPanel::SignalSaveAllLibs, &m_AutoRecovery, &ParticleLibraryAutoRecovery::Discard);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemPasted, this, &CMainWindow::Library_ItemPasted);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalPasteItemsToFolder, this, &CMainWindow::Library_ItemsPastedToFolder);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalCopyItems, this, &CMainWindow::Library_CopyTreeItems);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalCopyItem, this, &CMainWindow::Library_ItemCopied);

    addDockWidget(Qt::LeftDockWidgetArea, m_libraryTreeViewDock);

    //////////////////////////////////////////////////////////////////////////////////
    // * Attribute View
    m_attributeViewDock = new DockableAttributePanel(this);
    m_attributeViewDock->Init(tr("Attributes"), GetIEditor()->GetParticleManager());
    m_attributeViewDock->SetHotkeyHandler(m_attributeViewDock, [&](QKeyEvent* e){ keyPressEvent(e); }); //Routing this to MainWindow to handle the global items
    m_attributeViewDock->SetShortcutHandler(m_attributeViewDock, [&](QShortcutEvent* e){ return ShortcutEvent(e); }); //Routing this to MainWindow to handle the global items
    m_attributeViewDock->setObjectName("dwAttributeView");
    connect(m_attributeViewDock, &DockableAttributePanel::SignalPopulateTitleBarMenu, this, &CMainWindow::Attribute_PopulateTitleBarMenu);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemSelected, m_attributeViewDock, &DockableAttributePanel::ItemSelectionChanged);
    connect(m_attributeViewDock, &DockableAttributePanel::SignalTabSelectionChanged, m_libraryTreeViewDock, &DockableLibraryPanel::SelectLibraryAndItemByName);
    connect(m_attributeViewDock, &DockableAttributePanel::SignalPopulateTabBarContextMenu, this, &CMainWindow::Attribute_PopulateTabBarContextMenu);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemRenamed, m_attributeViewDock, &DockableAttributePanel::ItemNameChanged);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalUpdateTabName, m_attributeViewDock, &DockableAttributePanel::UpdateItemName);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemDeleted, m_attributeViewDock, &DockableAttributePanel::CloseTab);
    connect(m_preset, &QAttributePresetWidget::SignalCustomPanel, m_attributeViewDock, &DockableAttributePanel::ImportPanelFromQString);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalOpenInNewTab, m_attributeViewDock, &DockableAttributePanel::OpenInTab);

    connect(m_attributeViewDock, &DockableAttributePanel::SignalBuildCustomAttributeList, m_preset, &QAttributePresetWidget::BuilPresetMenu);
    connect(m_attributeViewDock, &DockableAttributePanel::SignalAddPreset, m_preset, &QAttributePresetWidget::AddPreset);
    connect(m_attributeViewDock, &DockableAttributePanel::SignalResetPresetList, m_preset, &QAttributePresetWidget::BuildDefaultLibrary);

    //POSSIBLE LEGACY ONLY SYNC USAGE HERE
    connect(m_attributeViewDock, &DockableAttributePanel::SignalParameterChanged, this, &CMainWindow::Attribute_ParameterChanged);
    //POSSIBLE LEGACY ONLY SYNC USAGE HERE

    connect(m_attributeViewDock, &DockableAttributePanel::SignalItemUndoPoint, this, &CMainWindow::OnItemUndoPoint);
    addDockWidget(Qt::RightDockWidgetArea, m_attributeViewDock);

    //////////////////////////////////////////////////////////////////////////////////
    // * Preview view
    m_previewDock = new DockablePreviewPanel(this);
    m_previewDock->Init(tr("Preview"));
    m_previewDock->setObjectName("dwPreviewCtl");
    m_previewDock->SetHotkeyHandler(m_previewDock, [&](QKeyEvent* e){ keyPressEvent(e); }); //Routing this to MainWindow to handle the global items
    m_previewDock->SetShortcutHandler(m_previewDock, [&](QShortcutEvent* e){ return ShortcutEvent(e); }); //Routing this to MainWindow to handle the global items
    connect(m_previewDock, &DockablePreviewPanel::SignalPopulateTitleBarMenu, this, &CMainWindow::Preview_PopulateTitleBarMenu);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemSelected, m_previewDock, &DockablePreviewPanel::ItemSelectionChanged);
    addDockWidget(Qt::LeftDockWidgetArea, m_previewDock);

    //////////////////////////////////////////////////////////////////////////////////
    // * Level of detail
    m_LoDDock = new DockableLODPanel(this);
    m_LoDDock->Init(tr("Level of detail"));
    m_LoDDock->setObjectName("dwLODPanel");
    m_LoDDock->SetHotkeyHandler(m_LoDDock, [&](QKeyEvent* e){ keyPressEvent(e); }); //Routing this to MainWindow to handle the global items
    m_LoDDock->SetShortcutHandler(m_LoDDock, [&](QShortcutEvent* e){ return ShortcutEvent(e); }); //Routing this to MainWindow to handle the global items
    connect(m_attributeViewDock, &DockableAttributePanel::SignalLODParticleHighLight, m_LoDDock, &DockableLODPanel::updateTreeItemHighLight);
    connect(m_LoDDock, &DockableLODPanel::SignalPopulateTitleBarMenu, this, &CMainWindow::LOD_PopulateTitleBarMenu);
    connect(m_LoDDock, &DockableLODPanel::SignalChangeLODIcon, m_libraryTreeViewDock, &DockableLibraryPanel::UpdateIconStyle);
    connect(m_libraryTreeViewDock, &DockableLibraryPanel::SignalItemSelected, m_LoDDock, &DockableLODPanel::ItemSelectionChanged);
    connect(m_LoDDock, &DockableLODPanel::SignalLodItemSelectionChanged, m_attributeViewDock, &DockableAttributePanel::LodItemSelectionChanged);
    connect(m_LoDDock, &DockableLODPanel::SignalLodItemSelectionChanged, this, [=](CBaseLibraryItem* item, SLodInfo* lod)
        {
            m_libraryTreeViewDock->blockSignals(true);
            m_libraryTreeViewDock->SelectLibraryAndItemByName(QString(item->GetLibrary()->GetName()), QString(item->GetName()));
            m_libraryTreeViewDock->blockSignals(false);
            m_previewDock->LodSelectionChanged(item, lod);
        });

    addDockWidget(Qt::LeftDockWidgetArea, m_LoDDock);
}

void CMainWindow::ResetToDefaultEditorLayout()
{
    //reset main window layout
    restoreState(m_defaultEditorLayout);
    //reset attribute view layout
    CRY_ASSERT(m_attributeViewDock);
    m_attributeViewDock->ResetToDefaultLayout();
    //reset previewer layout
    CRY_ASSERT(m_previewDock);
    m_previewDock->ResetToDefaultLayout();
    //reset library layout include dont prompt settings
    UIFactory::GetQTUISettings()->ResetToDefault();
}

void CMainWindow::OnActionViewDocumentation()
{
    static const char* documentationUrl = "https://docs.aws.amazon.com/lumberyard/userguide/particle-editor";
    QDesktopServices::openUrl(QUrl(documentationUrl));
}

#pragma region Actions: View Menu

void CMainWindow::OnRefreshViewMenu()
{
    QList<QAction*> viewActions = m_viewMenu->actions();
    for (int i = 0, numActions = viewActions.size(); i < numActions; i++)
    {
        QAction* action = viewActions[i];
        if (action->text().contains("attribute"))
        {
            if (m_attributeViewDock->isHidden())
            {
                QString newName = "Show attribute";
                action->setText(newName);
            }
            else
            {
                QString newName = "Hide attribute";
                action->setText(newName);
            }
        }
        else if (action->text().contains("library"))
        {
            if (m_libraryTreeViewDock->isHidden())
            {
                QString newName = "Show library";
                action->setText(newName);
            }
            else
            {
                QString newName = "Hide library";
                action->setText(newName);
            }
        }
        else if (action->text().contains("preview"))
        {
            if (m_previewDock->isHidden())
            {
                QString newName = "Show preview";
                action->setText(newName);
            }
            else
            {
                QString newName = "Hide preview";
                action->setText(newName);
            }
        }
        else if (action->text().contains("level of detail"))
        {
            if (m_previewDock->isHidden())
            {
                QString newName = "Show level of detail";
                action->setText(newName);
            }
            else
            {
                QString newName = "Hide level of detail";
                action->setText(newName);
            }
        }
    }
}

void CMainWindow::OnAddNewLayout(QString location, bool loading)
{
    if (m_layoutMenu)
    {
        bool isAdded = false;

        QList<QAction*> menuActions = m_layoutMenu->actions();
        int menu_size = menuActions.size();
        for (int i = 0; i < menu_size; i++)
        {
            QAction* action = menuActions[i];
            if (action->text().contains(location))
            {
                action->setChecked(true);
                isAdded = true;
            }
            else
            {
                action->setChecked(false);
            }
        }

        if (!isAdded)
        {
            QAction* action = new QAction(location, m_layoutMenu);
            action->setCheckable(true);
            action->setChecked(!loading);
            action->setData(this->saveState(0));
            connect(action, &QAction::triggered, this, [this, action]()
                {
                    RestoreAllLayout(action->text());
                    OnAddNewLayout(action->text(), false);
                });
            m_layoutMenu->addAction(action);
            if (!loading)
            {
                StateSave_layoutMenu();
            }
        }
    }
}

void CMainWindow::OnActionViewImportLayout()
{
    QString location = QFileDialog::getOpenFileName(this, "Select location to load layout...", "", tr("Editor Layout Files (*.editor_layout)"));

    RestoreAllLayout(location);
}

void CMainWindow::RestoreAllLayout(QString location)
{
    if (!location.length()) //No path is an invalid path, this was probably
    {
        return;
    }
    QFile file(location);
    if (!file.exists()) // The selected path does not exist on disk
    {
        QMessageBox::warning(this, tr("Warning"), tr("The layout file at: \"%1\" could not be loaded.\n").arg(location));
        return;
    }

    QList<QByteArray> data_separator;

    file.open(QIODevice::ReadOnly);
    QByteArray data_to_restore = file.readAll();
    file.close();
    QDataStream stream(data_to_restore);
    stream.setByteOrder(QDataStream::BigEndian);
    quint32 fileFormat;
    stream >> fileFormat;
    quint32 fileVersion;
    stream >> fileVersion;
    quint32 mainWindowSize;
    stream >> mainWindowSize;
    quint32 attributeSize;
    stream >> attributeSize;
    quint32 previewSize;
    stream >> previewSize;

    QByteArray data_mainWindow;
    QByteArray data_attribute;
    QByteArray data_preview;
    QByteArray data_settings;



    switch (fileVersion)
    {
    case 0x1:
        {
            data_mainWindow.resize(mainWindowSize);
            stream.readRawData(data_mainWindow.data(), mainWindowSize);

            data_attribute.resize(attributeSize);
            stream.readRawData(data_attribute.data(), attributeSize);

            data_preview.resize(previewSize);
            stream.readRawData(data_preview.data(), previewSize);

            break;
        }
    case 0x2:
        {
            quint32 editorSettingSize;
            stream >> editorSettingSize;

            data_mainWindow.resize(mainWindowSize);
            stream.readRawData(data_mainWindow.data(), mainWindowSize);

            data_attribute.resize(attributeSize);
            stream.readRawData(data_attribute.data(), attributeSize);

            data_preview.resize(previewSize);
            stream.readRawData(data_preview.data(), previewSize);

            data_settings.resize(editorSettingSize);
            stream.readRawData(data_settings.data(), editorSettingSize);

            UIFactory::GetQTUISettings()->LoadSetting(data_settings);
            break;
        }
    default:
        {
            //The format is not correct. Try load it using previous version.
            bool loadSuccess = restoreState(data_to_restore, 0);
            if (loadSuccess) // Load old version, notify user to update the layout file
            {
                OnAddNewLayout(location, false);
                OnRefreshViewMenu();
            }
            else // Load fails
            {
                QMessageBox dlg("Warning", tr("The layout file format is incorrect."),
                    QMessageBox::Icon::Warning, QMessageBox::Button::Close, QMessageBox::Button::Cancel, 0, this);
                dlg.exec();
            }
            return;
        }
        break;
    }

    if (fileFormat != PARTICLE_EDITOR_LAYOUT_IDENTIFIER)
    {
        QMessageBox::warning(this, tr("Warning"), tr("The layout file format is incorrect."));
        return;
    }

    restoreState(data_mainWindow, 0); //works
    OnAddNewLayout(location, false);

    OnRefreshViewMenu();

    CRY_ASSERT(m_attributeViewDock);
    m_attributeViewDock->LoadAttributeLayout(data_attribute); //does not work

    CRY_ASSERT(m_previewDock);
    m_previewDock->LoadSessionState(data_preview); //does not work
}

void CMainWindow::OnActionViewExportLayout()
{
    QString location = QFileDialog::getSaveFileName(this, "Select location to save layout...", "", tr("Editor Layout Files (*.editor_layout)"));
    if (!location.length())     //if the user hit cancel
    {
        return;
    }

    QFile file(location);

    //layout setting for mainwindow
    QByteArray data_mainWindow = saveState(0);
    OnAddNewLayout(location, false);

    //layout setting for attribute panel
    QByteArray data_attribute;
    m_attributeViewDock->SaveAttributeLayout(data_attribute);

    //layout setting for preview window
    QByteArray data_preview;
    m_previewDock->SaveSessionState(data_preview);

    QByteArray data_settings;
    UIFactory::GetQTUISettings()->SaveSetting(data_settings);

    quint32 fileFormat = PARTICLE_EDITOR_LAYOUT_IDENTIFIER;
    quint32 fileVersion = PARTICLE_EDITOR_LAYOUT_VERSION;
    quint32 mainWindowSize = data_mainWindow.size();
    quint32 attributeSize = data_attribute.size();
    quint32 previewSize = data_preview.size();
    quint32 settingSize = data_settings.size();

    file.open(QIODevice::WriteOnly);
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << fileFormat;
    stream << fileVersion;
    stream << mainWindowSize;
    stream << attributeSize;
    stream << previewSize;
    stream << settingSize;

    int result = stream.writeRawData(data_mainWindow, mainWindowSize);
    result = stream.writeRawData(data_attribute, attributeSize);
    result = stream.writeRawData(data_preview, previewSize);
    result = stream.writeRawData(data_settings, settingSize);
    QDataStream::Status stats = stream.status();
    qDebug() << stats;
    file.close();
}

void CMainWindow::OnActionViewHideAttributes()
{
    if (m_attributeViewDock->isHidden())
    {
        m_attributeViewDock->show();
    }
    else
    {
        m_attributeViewDock->hide();
    }
}
void CMainWindow::OnActionViewHideLibrary()
{
    if (m_libraryTreeViewDock->isHidden())
    {
        m_libraryTreeViewDock->show();
    }
    else
    {
        m_libraryTreeViewDock->hide();
    }
}
void CMainWindow::OnActionViewHidePreview()
{
    if (m_previewDock->isHidden())
    {
        m_previewDock->show();
    }
    else
    {
        m_previewDock->hide();
    }
}

void CMainWindow::OnActionViewHideLOD()
{
    if (m_LoDDock->isHidden())
    {
        m_LoDDock->show();
    }
    else
    {
        m_LoDDock->hide();
    }
}

#pragma endregion

#pragma region Actions: File Menu
void CMainWindow::OnActionClose()
{
    m_RequestedClose = true;
}
#pragma endregion

#pragma region Actions: Standard (Copy/Paste/Undo/Redo)
void CMainWindow::OnActionStandardUndo()
{
    EBUS_EVENT(EditorLibraryUndoRequestsBus, Undo);
}

void CMainWindow::OnActionStandardRedo()
{
    EBUS_EVENT(EditorLibraryUndoRequestsBus, Redo);
}
#pragma endregion

#pragma region State persistence

// NOTE: Make sure all dock widgets and widget contained therein have an object name! (using setObjectName).
//       This is used by Qt to correctly identify widgets when storing/loading data.

void CMainWindow::StateLoad_Properties()
{
    CRY_ASSERT(m_attributeViewDock);
    m_attributeViewDock->LoadSessionState();
    CRY_ASSERT(m_preset);
    m_preset->LoadSessionPresets();
}

void CMainWindow::StateLoad_Preview()
{
    CRY_ASSERT(m_previewDock);
    m_previewDock->LoadSessionState();
}

void CMainWindow::StateSave()
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());

    QSettings settings("Amazon", "Lumberyard");
    settings.setValue(PARTICLE_EDITOR_LAYOUT_KEY, this->saveState(0));
    settings.sync();
    GetIEditor()->GetParticleUtils()->HotKey_SaveCurrent();

    QByteArray qsettingsdata;
    UIFactory::GetQTUISettings()->SaveSetting(qsettingsdata);
    settings.setValue(PARTICLE_EDITOR_SETTINGS_KEY, qsettingsdata);

    CRY_ASSERT(m_attributeViewDock);
    m_attributeViewDock->SaveSessionState();

    CRY_ASSERT(m_previewDock);
    m_previewDock->SaveSessionState();

    CRY_ASSERT(m_preset);
    m_preset->StoreSessionPresets();
}


void CMainWindow::StateSave_layoutMenu()
{
    if (m_layoutMenu)
    {
        QSettings settings("Amazon", "Lumberyard");
        settings.beginGroup(SETTINGS_LAYOUT_MENU_GROUP);
        {
            //REMOVE OLD SETTINGS
            settings.remove("");
            settings.sync();
            QList<QAction*> menuActions = m_layoutMenu->actions();
            int menu_size = menuActions.size();
            QString tag;
            QString path;
            settings.setValue(SETTINGS_LAYOUT_MENU_SIZE_KEY, menu_size);

            
            for (int i = 0; i < menu_size; i++)
            {
                QAction* childAction = menuActions[i];
                CRY_ASSERT(childAction);
                
                if (childAction)
                {
                    tag = QString::number(i);
                    path = childAction->text();
                    settings.setValue(tag, path);
                }
            }
        }
        settings.endGroup();
        settings.sync();
    }
}

void CMainWindow::StateLoad_layoutMenu()
{
    QSettings settings("Amazon", "Lumberyard");
    settings.beginGroup(SETTINGS_LAYOUT_MENU_GROUP);
    int menu_size = settings.value(SETTINGS_LAYOUT_MENU_SIZE_KEY, 0).toInt();
    QString tag;
    QString path;
    for (int i = 0; i < menu_size; i++)
    {
        tag = QString::number(i);
        path = settings.value(tag, "").toString();
        OnAddNewLayout(path, true);
    }
}

void CMainWindow::StateLoad()
{
    QSettings settings("Amazon", "Lumberyard");

    StateLoad_Properties();     // loads layout of the Properties screen
    StateLoad_Preview();        // loads layout of the preview window
    if (settings.contains(PARTICLE_EDITOR_LAYOUT_KEY))
    {
        restoreState(settings.value(PARTICLE_EDITOR_LAYOUT_KEY).toByteArray(), 0);
    }
    if (settings.contains(PARTICLE_EDITOR_SETTINGS_KEY))
    {
        UIFactory::GetQTUISettings()->LoadSetting(settings.value(PARTICLE_EDITOR_SETTINGS_KEY).toByteArray());
    }
    OnAddNewLayout("Last User State", false);
}

void CMainWindow::RefreshLibraries()
{
    if (m_needLibraryRefresh)
    {
        m_libraryTreeViewDock->RebuildFromEngineData(/*Expansion data is likely corrupt here, don't save it.*/true);
        m_libraryTreeViewDock->ResetSelection();
        m_attributeViewDock->CloseAllTabs();
        m_needLibraryRefresh = false;
    }
}

void CMainWindow::closeEvent(QCloseEvent* event)
{
    // NOTE: this event must be handled here and now, without being queued.
    // Otherwise, when the user closes the LY Editor, the Particle Editor will block everything closing.

    bool userDidNotCancelClose = OnCloseWindowCheck();
    if (userDidNotCancelClose)
    {
        CleanupOnClose();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void CMainWindow::showEvent(QShowEvent * event)
{
    if (m_requireLayoutReload)
    {
        StateLoad();
        StateLoad_layoutMenu();
        m_requireLayoutReload = false;
    }

    RefreshLibraries();
}

void CMainWindow::CleanupOnClose()
{
    StateSave();
    StateSave_layoutMenu();

    QList<QDockWidget*> dockWidgets = findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (int i = 0; i < dockWidgets.size(); ++i)
    {
        dockWidgets[i]->hide();
    }
    m_requireLayoutReload = true;

    m_undoManager->Reset();
    m_RequestedClose = false;
}


bool CMainWindow::OnCloseWindowCheck()
{
    QString unsavedLibraries = tr("Libraries with unsaved changes: \n");
    bool needToWarn = false;
    CRY_ASSERT(GetIEditor());
    IEditorParticleManager* manager = GetIEditor()->GetParticleManager();
    CRY_ASSERT(manager);
    for (unsigned int i = 0; i < manager->GetLibraryCount(); i++)
    {
        IDataBaseLibrary* lib = manager->GetLibrary(i);
        CRY_ASSERT(lib);
        if (!lib->IsLevelLibrary() && lib->IsModified())
        {
            needToWarn = true;
            unsavedLibraries.append(lib->GetName() + tr("\n"));
        }
    }

    bool canClose = false;

    if (needToWarn)
    {
        QMessageBox dlg(QMessageBox::Warning, tr("Warning"), tr("There are unsaved libraries, these will be lost upon exiting the editor.\n") + unsavedLibraries,
            QMessageBox::Close | QMessageBox::Cancel, this);
        if (dlg.exec() == QMessageBox::Button::Close)
        {
            canClose = true;
        }
    }
    else
    {
        canClose = true;
    }
    
    return canClose;
}


#pragma endregion

void CMainWindow::NotifyExceptSelf(EEditorNotifyEvent notification)
{
    CRY_ASSERT(GetIEditor());
    GetIEditor()->NotifyExcept((EEditorNotifyEvent)notification, this);
}

bool CMainWindow::ShortcutEvent(QShortcutEvent* e)
{
    bool eventHandled = false;

    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
    QString pathOfShortcut = GetIEditor()->GetParticleUtils()->HotKey_GetPressedHotkey(e);
    QKeySequence hotkey = GetIEditor()->GetParticleUtils()->HotKey_GetShortcut(pathOfShortcut.toUtf8());
    if (!pathOfShortcut.isEmpty())
    {
        for (unsigned int i = 0; i < m_shortcuts.count(); i++)
        {
            if (m_shortcuts[i] && hotkey == m_shortcuts[i]->key())
            {
                m_shortcuts[i]->activated();
                e->accept();
                eventHandled = true;
            }
        }
    }

    return eventHandled;
}

void CMainWindow::keyPressEvent(QKeyEvent* event)
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
    QString pathOfShortcut = GetIEditor()->GetParticleUtils()->HotKey_GetPressedHotkey(event);
    QKeySequence hotkey = GetIEditor()->GetParticleUtils()->HotKey_GetShortcut(pathOfShortcut.toUtf8());

    if (!pathOfShortcut.isEmpty())
    {
        bool handled = false;
        for (unsigned int i = 0; i < m_shortcuts.count(); i++)
        {
            if (m_shortcuts[i] && hotkey == m_shortcuts[i]->key())
            {
                m_shortcuts[i]->activated();
                handled = true;
            }
        }

        if (handled)
        {
            event->setAccepted(true);
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

bool CMainWindow::event(QEvent* e)
{
    if (e->type() == QEvent::Shortcut)
    {
        if (ShortcutEvent((QShortcutEvent*)e))
        {
            return true;
        }
    }

    return QMainWindow::event(e);
}

void CMainWindow::Preview_PopulateTitleBarMenu(QMenu* toAddTo)
{
    CRY_ASSERT(m_libraryTreeViewDock);

    QMenu* menu = toAddTo;
    QAction* action = nullptr;

#define ADD_ACTION(text, callback, tooltip, icon, key)                               \
    {                                                                                \
        action = menu->addAction(QIcon("Editor/UI/Icons/toolbar/" icon), tr(text)); \
        if (key != 0) {                                                              \
            action->setShortcut(key); }                                              \
        action->setToolTip(tr(tooltip));                                             \
        connect(action, &QAction::triggered, this, &CMainWindow::callback);          \
    }

    ADD_ACTION("Close", OnActionViewHidePreview, "Close preview panel", "", QKeySequence());
#undef ADD_ACTION
}

void CMainWindow::Library_PopulateTitleBarMenu(QMenu* toAddTo)
{
    CRY_ASSERT(m_libraryTreeViewDock);

    QMenu* menu = toAddTo;

    // Set connection type to QueuedConnection to make sure the action triggered after menu get deleted, otherwise it may cause duplicate deletion.
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::ADD, tr("Add library"), false, menu, Qt::QueuedConnection));
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::IMPORT, tr("Import..."), false, menu, Qt::QueuedConnection));
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::IMPORT_LEVEL_LIBRARY, tr("Import Level Library"), false, menu, Qt::QueuedConnection));
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::SAVE_ALL, tr("Save all"), false, menu, Qt::QueuedConnection));

    QAction* action = nullptr;
#define ADD_ACTION(text, callback, tooltip, icon, key)                               \
    {                                                                                \
        action = menu->addAction(QIcon("Editor/UI/Icons/toolbar/" icon), tr(text)); \
        if (key != 0) {                                                              \
            action->setShortcut(key); }                                              \
        action->setToolTip(tr(tooltip));                                             \
        connect(action, &QAction::triggered, this, &CMainWindow::callback);          \
    }

    ADD_ACTION("Close", OnActionViewHideLibrary, "Close library panel", "", QKeySequence());

#undef ADD_ACTION
}

void CMainWindow::Attribute_PopulateTitleBarMenu(QMenu* toAddTo)
{
    CRY_ASSERT(m_libraryTreeViewDock);
    CRY_ASSERT(m_attributeViewDock);
    QMenu* menu = toAddTo;
    QAction* action = nullptr;

#define ADD_ACTION(text, callback, tooltip, icon, key)                               \
    {                                                                                \
        action = menu->addAction(QIcon("Editor/UI/Icons/toolbar/" icon), tr(text)); \
        if (key != 0) {                                                              \
            action->setShortcut(key); }                                              \
        action->setToolTip(tr(tooltip));                                             \
        connect(action, &QAction::triggered, this, &CMainWindow::callback);          \
    }

    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::ADD_ITEM, "", tr("Create new emitter"), false, menu, Qt::QueuedConnection));
    m_attributeViewDock->AddLayoutMenuItemsTo(menu); //would be better setup with GetMenuAction like requests. (see above)
    ADD_ACTION("Close", OnActionViewHideAttributes, "Close attributes panel", "", QKeySequence());
#undef ADD_ACTION
}

void CMainWindow::LOD_PopulateTitleBarMenu(QMenu* toAddTo)
{
    CRY_ASSERT(m_LoDDock);
    QMenu* menu = toAddTo;
    QAction* action = nullptr;


    #define ADD_ACTION(menuD, textD, actionEnum)                                                              \
    {                                                                                                         \
        QAction* actiond = menuD->addAction(textD);                                                           \
        connect(actiond, &QAction::triggered, this, [=](){ m_LoDDock->PerformTitleMenuAction(actionEnum); }); \
    }
    //End define

    //AddLevel
    ADD_ACTION(menu, "Add level", DockableLODPanel::TitleMenuActions::AddLevel);

    //Arrange
    QMenu* arrangeMenu = menu->addMenu("Arrange");

    //Arrange-> Move up
    ADD_ACTION(arrangeMenu, "Move Up", DockableLODPanel::TitleMenuActions::MoveUp);

    //Arrange > Move down
    ADD_ACTION(arrangeMenu, "Move Down", DockableLODPanel::TitleMenuActions::MoveDown);

    //Arrange > Move to top
    ADD_ACTION(arrangeMenu, "Move to top", DockableLODPanel::TitleMenuActions::MoveToTop);

    //Arrange > Move to bottom
    ADD_ACTION(arrangeMenu, "Move to bottom", DockableLODPanel::TitleMenuActions::MoveToBottom);

    //Jump to first
    ADD_ACTION(menu, "Jump to first", DockableLODPanel::TitleMenuActions::JumpToFirst);

    //Jump to last
    ADD_ACTION(menu, "Jump to Last", DockableLODPanel::TitleMenuActions::JumpToLast);

    //Remove
    ADD_ACTION(menu, "Remove", DockableLODPanel::TitleMenuActions::Remove);

    //Remove All
    ADD_ACTION(menu, "Remove all", DockableLODPanel::TitleMenuActions::RemoveAll);

    //Close
    action = menu->addAction("Close");
    connect(action, &QAction::triggered, this, [=](){ OnActionViewHideLOD(); });
#undef ADD_ACTION
}

void CMainWindow::Library_PopulateItemContextMenu(CLibraryTreeViewItem* focusedItem, ContextMenu* toAddTo)
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
    CRY_ASSERT(m_libraryTreeViewDock);
    CRY_ASSERT(m_attributeViewDock);
    CRY_ASSERT(toAddTo);

    QAction* action = nullptr;
    QString itemName = focusedItem ? focusedItem->GetFullPath() : "";
    QVector<CLibraryTreeViewItem*> selectedItems = m_libraryTreeViewDock->GetSelectedTreeItems();

    //OPEN IN NEW TAB/////////////////////////////////////////////////////////
    action = m_libraryTreeViewDock->GetMenuAction(DockableParticleLibraryPanel::ItemActions::OPEN_IN_NEW_TAB, itemName, tr("Open in new tab"), false, toAddTo, Qt::QueuedConnection);
    toAddTo->addAction(action);
    action->setDisabled(focusedItem->IsVirtualItem());
    toAddTo->addSeparator();
    //ADD NEW EMITTER/FOLDER//////////////////////////////////////////////////
    QMenu* submenu = new ContextMenu("Add New", toAddTo);
    toAddTo->addMenu(submenu);
    action = m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::ADD_ITEM, itemName, tr("Add Particle"), false, toAddTo, Qt::QueuedConnection);
    submenu->addAction(action);
    if (focusedItem && focusedItem->IsVirtualItem() && selectedItems.count() <= 1)
    {
        action = m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::ADD_FOLDER, itemName, tr("Add Folder"), false, toAddTo, Qt::QueuedConnection);
        submenu->addAction(action);
    }
    else if (focusedItem && selectedItems.count() <= 1) //we are on an item not a folder
    {
        action = toAddTo->addAction(tr("Add LOD"));
        connect(action, &QAction::triggered, this, [=]()
            {
                m_LoDDock->OnAddLod(focusedItem);
            });
    }
    toAddTo->addSeparator();
    //COPY////////////////////////////////////////////////////////////////////
    toAddTo->addAction(m_libraryTreeViewDock->GetParticleAction(DockableParticleLibraryPanel::ParticleActions::COPY, itemName, tr("Copy"), false, toAddTo, Qt::QueuedConnection));
    //PASTE///////////////////////////////////////////////////////////////////
    toAddTo->addAction(m_libraryTreeViewDock->GetParticleAction(DockableParticleLibraryPanel::ParticleActions::PASTE, itemName, tr("Paste"), false, toAddTo, Qt::QueuedConnection));
    //DUPLICATE///////////////////////////////////////////////////////////////
    action = toAddTo->addAction(tr("Duplicate"));
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Duplicate"));
    if (focusedItem && !focusedItem->IsVirtualItem() && selectedItems.count() <= 1)
    {
        CParticleItem* pParticle = static_cast<CParticleItem*>(focusedItem->GetItem());
        connect(action, &QAction::triggered, this, [=]()
            {
                Library_ItemDuplicated(QString(pParticle->GetFullName()), "");
            }, Qt::QueuedConnection);
    }
    else
    {
        action->setEnabled(false);
    }
    //COPY PATH///////////////////////////////////////////////////////////////
    toAddTo->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::COPY_PATH, itemName, tr("Copy Path"), false, toAddTo, Qt::QueuedConnection));
    //RENAME//////////////////////////////////////////////////////////////////
    toAddTo->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::RENAME, itemName, tr("Rename"), false, toAddTo, Qt::QueuedConnection));
    //DELETE//////////////////////////////////////////////////////////////////
    toAddTo->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::DESTROY, itemName, tr("Delete"), false, toAddTo, Qt::QueuedConnection));

    //ACTIVATE TOGGLE/////////////////////////////////////////////////////////
    //If the item is a folder then it does not have concept of enable/disable
    // folders are treeview items that are not considered particles.
    if (focusedItem && !focusedItem->IsVirtualItem())
    {
        CParticleItem* pParticle = static_cast<CParticleItem*>(focusedItem->GetItem());
        //ENABLE//////////////////////////////////////////////////////////////////
        toAddTo->addAction(m_libraryTreeViewDock->GetParticleAction(DockableParticleLibraryPanel::ParticleActions::ENABLE, itemName, tr("Enable"), false, toAddTo, Qt::QueuedConnection));
        //DISABLE/////////////////////////////////////////////////////////////////
        toAddTo->addAction(m_libraryTreeViewDock->GetParticleAction(DockableParticleLibraryPanel::ParticleActions::DISABLE, itemName, tr("Disable"), false, toAddTo, Qt::QueuedConnection));
    }

    //ACTIVATE ALL TOGGLE/////////////////////////////////////////////////////
    if (focusedItem && focusedItem->childCount() > 0)
    {
        action = toAddTo->addAction(tr("Enable All"));
        action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("File Menu.Enable All"));
        connect(action, &QAction::triggered, this, [=]()
            {
                EditorUIPlugin::ScopedBatchUndoPtr batchUndo;
                EBUS_EVENT_RESULT(batchUndo, EditorLibraryUndoRequestsBus, AddScopedBatchUndo, "Enable All");
                QtRecurseAll(focusedItem, [=](QTreeWidgetItem* item)
                    {
                        CLibraryTreeViewItem* titem = static_cast<CLibraryTreeViewItem*>(item);
                        if (!titem->IsVirtualItem())
                        {
                            CParticleItem* pParticle = static_cast<CParticleItem*>(titem->GetItem());
                            m_attributeViewDock->SetEnabledParameter(pParticle, true);
                            titem->setCheckState(LIBRARY_TREEVIEW_INDICATOR_COLUMN, Qt::CheckState::Checked);
                        }
                    });
            });

        action = toAddTo->addAction(tr("Disable All"));
        action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("File Menu.Disable All"));
        connect(action, &QAction::triggered, this, [=]()
            {
                EditorUIPlugin::ScopedBatchUndoPtr batchUndo;
                EBUS_EVENT_RESULT(batchUndo, EditorLibraryUndoRequestsBus, AddScopedBatchUndo, "Disable All");

                QtRecurseAll(focusedItem, [=](QTreeWidgetItem* item)
                    {
                        CLibraryTreeViewItem* titem = static_cast<CLibraryTreeViewItem*>(item);
                        if (!titem->IsVirtualItem())
                        {
                            CParticleItem* pParticle = static_cast<CParticleItem*>(titem->GetItem());
                            m_attributeViewDock->SetEnabledParameter(pParticle, false);
                            titem->setCheckState(LIBRARY_TREEVIEW_INDICATOR_COLUMN, Qt::CheckState::Unchecked);
                        }
                    });
            });
    }
    toAddTo->addSeparator();
    //EXPAND ALL//////////////////////////////////////////////////////////////
    toAddTo->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::EXPAND_ALL, itemName, tr("Expand All"), false, toAddTo, Qt::QueuedConnection));
    //COLLAPSE ALL////////////////////////////////////////////////////////////
    toAddTo->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::COLLAPSE_ALL, itemName, tr("Collapse All"), false, toAddTo, Qt::QueuedConnection));
    toAddTo->addSeparator();
    //GROUP///////////////////////////////////////////////////////////////////
    toAddTo->addAction(m_libraryTreeViewDock->GetParticleAction(DockableParticleLibraryPanel::ParticleActions::GROUP, itemName, tr("Group"), false, toAddTo, Qt::QueuedConnection));
    //UNGROUP/////////////////////////////////////////////////////////////////
    toAddTo->addAction(m_libraryTreeViewDock->GetParticleAction(DockableParticleLibraryPanel::ParticleActions::UNGROUP, itemName, tr("Ungroup"), false, toAddTo, Qt::QueuedConnection));
    //RESET TO DEFAULT////////////////////////////////////////////////////////
    action = toAddTo->addAction(tr("Reset to default"));
    if (focusedItem && !focusedItem->IsVirtualItem() && selectedItems.count() <= 1)
    {
        CParticleItem* pParticle = static_cast<CParticleItem*>(focusedItem->GetItem());
        connect(action, &QAction::triggered, this, [=]()
            {
                Library_ItemReset(pParticle);
                m_libraryTreeViewDock->ForceLibrarySync(focusedItem->GetLibraryName());
            }, Qt::QueuedConnection);
    }
    else
    {
        action->setEnabled(false);
    }
}

void CMainWindow::Attribute_PopulateTabBarContextMenu(const QString& libraryName, const QString& itemName, ContextMenu* toAddTo)
{
    QAction* action = m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::ADD_ITEM, itemName, tr("Create new emitter"), false, toAddTo, Qt::QueuedConnection);

    //DUPLICATE///////////////////////////////////////////////////////////////
    action = toAddTo->addAction(tr("Duplicate"));
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Duplicate"));
    if (!libraryName.isEmpty() && !itemName.isEmpty())
    {
        CLibraryTreeViewItem* treeItem = m_libraryTreeViewDock->GetTreeItemFromPath(libraryName + "." + itemName);
        if (treeItem && !treeItem->IsVirtualItem())
        {
            connect(action, &QAction::triggered, this, [=]()
                {
                    Library_ItemDuplicated(libraryName + "." + itemName, "");
                });
        }
        else
        {
            action->setEnabled(false);
        }
    }
    else
    {
        action->setEnabled(false);
    }

    toAddTo->addAction(m_attributeViewDock->GetMenuAction(DockableAttributePanel::TabActions::CLOSE, itemName, tr("Close"), toAddTo));
    toAddTo->addAction(m_attributeViewDock->GetMenuAction(DockableAttributePanel::TabActions::CLOSE_ALL, itemName, tr("Close all"), toAddTo));
    toAddTo->addAction(m_attributeViewDock->GetMenuAction(DockableAttributePanel::TabActions::CLOSE_ALL_BUT_THIS, itemName, tr("Close all but this"), toAddTo));

    //RESET TO DEFAULT////////////////////////////////////////////////////////
    action = toAddTo->addAction(tr("Reset to default"));
    if (!libraryName.isEmpty() && !itemName.isEmpty())
    {
        CLibraryTreeViewItem* treeItem = m_libraryTreeViewDock->GetTreeItemFromPath(itemName);
        if (treeItem && !treeItem->IsVirtualItem())
        {
            CParticleItem* pParticle = static_cast<CParticleItem*>(treeItem->GetItem());
            connect(action, &QAction::triggered, this, [=]()
                {
                    Library_ItemReset(pParticle, m_attributeViewDock->GetCurrentLod());
                    m_libraryTreeViewDock->ForceLibrarySync(libraryName);
                });
        }
        else
        {
            action->setEnabled(false);
        }
    }
}

//POSSIBLE LEGACY ONLY SYNC USAGE HERE
void CMainWindow::Attribute_ParameterChanged(const QString& libraryName, const QString& itemName)
{
    //Currently it is not cared what item from what library has changed as this is being done to just generically notify others that a particle has updated
    // NOTE: used for legacy editor syncronization
    NotifyExceptSelf(eNotify_OnParticleUpdate);
    m_previewDock->ForceParticleEmitterRestart();
}
//POSSIBLE LEGACY ONLY SYNC USAGE HERE


void CMainWindow::OnItemUndoPoint(const QString& itemFullName, bool selected, SLodInfo* lod)
{
    bool isSuspend = false;
    EBUS_EVENT_RESULT(isSuspend, EditorLibraryUndoRequestsBus, IsSuspend);
    if (isSuspend)
    {
        return;
    }

    //all particle item's attribute changes will landed here and need to be saved for undo
    CParticleItem* item = static_cast<CParticleItem*>(GetIEditor()->GetParticleManager()->FindItemByName(itemFullName));
    if (item)
    {
        int lodIdx = item->GetEffect()->GetLevelOfDetailIndex(lod);
        EBUS_EVENT(EditorLibraryUndoRequestsBus, AddItemUndo, itemFullName.toUtf8().data(), selected, lodIdx);
        item->SetModified(true);
    }
}

void CMainWindow::File_PopulateMenu()
{
    CRY_ASSERT(m_fileMenu);
    CRY_ASSERT(m_libraryTreeViewDock);
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
    QMenu* menu = m_fileMenu;
    menu->clear();
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::ADD, tr("Add library"), false, menu, Qt::QueuedConnection));
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::IMPORT, tr("Import..."), false, menu, Qt::QueuedConnection));
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::IMPORT_LEVEL_LIBRARY, tr("Import Level Library"), false, menu, Qt::QueuedConnection));
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::SAVE_ALL, tr("Save all"), false, menu, Qt::QueuedConnection));

    QAction* action = nullptr;
#define ADD_ACTION(text, callback, tooltip, icon, key)                               \
    {                                                                                \
        action = menu->addAction(QIcon("Editor/UI/Icons/toolbar/" icon), tr(text)); \
        if (key != 0) {                                                              \
            action->setShortcut(key); }                                              \
        action->setToolTip(tr(tooltip));                                             \
        connect(action, &QAction::triggered, this, &CMainWindow::callback);          \
    }

    ADD_ACTION("Close", OnActionClose, "Close Editor", "", GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("File Menu.Close"));
#undef ADD_ACTION
}

void CMainWindow::Edit_PopulateMenu()
{
    CRY_ASSERT(m_editMenu);
    CRY_ASSERT(m_libraryTreeViewDock);
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());

    QMenu* menu = m_editMenu;
    menu->clear();
    QAction* action = nullptr;
#define ADD_ACTION(text, callback, tooltip, icon, key)                               \
    {                                                                                \
        action = menu->addAction(QIcon("Editor/UI/Icons/toolbar/" icon), tr(text)); \
        if (key != 0) {                                                              \
            action->setShortcut(key); }                                              \
        action->setToolTip(tr(tooltip));                                             \
        connect(action, &QAction::triggered, this, &CMainWindow::callback);          \
    }

    ADD_ACTION("Undo", OnActionStandardUndo, "Undo", "standardUndo.png", GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Undo"));
    m_undoAction = action;
    bool hasUndo = false;
    EBUS_EVENT_RESULT(hasUndo, EditorLibraryUndoRequestsBus, HasUndo);
    action->setEnabled(hasUndo);
    ADD_ACTION("Redo", OnActionStandardRedo, "Redo", "standardRedo.png", GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Redo"));    
    m_redoAction = action;
    bool hasRedo = false;
    EBUS_EVENT_RESULT(hasRedo, EditorLibraryUndoRequestsBus, HasRedo);
    action->setEnabled(hasRedo);

    //COPY////////////////////////////////////////////////////////////////////
    m_copyAction = menu->addAction(tr("Copy"));
    m_copyAction->setIcon(QIcon("Editor/UI/Icons/toolbar/standardCopy.png"));
    m_copyAction->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Copy"));
    connect(m_copyAction, &QAction::triggered, this, [this]
        {
            QVector<CBaseLibraryItem*> selectedItems = m_libraryTreeViewDock->GetSelectedItems();
             if (!selectedItems.isEmpty() && selectedItems.first())
             {
                 Library_ItemCopied(selectedItems.first());
             }
        });

    //PASTE///////////////////////////////////////////////////////////////////
    menu->addAction(m_libraryTreeViewDock->GetParticleAction(DockableParticleLibraryPanel::ParticleActions::PASTE, "", tr("Paste"), false, menu, Qt::QueuedConnection));
    
    //DUPLICATE///////////////////////////////////////////////////////////////
    m_duplicateAction = menu->addAction(tr("Duplicate"));
    m_duplicateAction->setIcon(QIcon("Editor/UI/Icons/toolbar/standardDuplicate.png"));
    m_duplicateAction->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Duplicate"));
    connect(m_duplicateAction, &QAction::triggered, this, [this]
        {
            QVector<CBaseLibraryItem*> selectedItems = m_libraryTreeViewDock->GetSelectedItems();
            if (selectedItems.count() == 1 && selectedItems.first())
            {
                Library_ItemDuplicated("", "");
            }
        });

    ////////////////////////////////////////////////////
    //Action Add LOD
    m_addLODAction = menu->addAction(tr("Add LOD"));
    connect(m_addLODAction, &QAction::triggered, this, [this]
        {
            QVector<CBaseLibraryItem*> selectedItems = m_libraryTreeViewDock->GetSelectedItems();
            if (selectedItems.count() && selectedItems.first())
            {
                auto pParticle = static_cast<CParticleItem*>(selectedItems.first());
                if (pParticle->IsParticleItem)
                {
                    CLibraryTreeViewItem* treeitem = m_libraryTreeViewDock->GetTreeItemFromPath(QString(selectedItems.first()->GetFullName()));
                    if (treeitem)
                    {
                        m_LoDDock->OnAddLod(treeitem);
                    }
                }
            }
        });


    m_renameSelectedItem = m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::RENAME, "", tr("Rename"), false, menu, Qt::QueuedConnection);
    menu->addAction(m_renameSelectedItem);

    m_deleteSelectedItem = m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::DESTROY, "", tr("Delete"), false, menu, Qt::QueuedConnection);
    menu->addAction(m_deleteSelectedItem);
    menu->actions().back()->setIcon(QIcon("Editor/UI/Icons/toolbar/itemRemove.png"));

    //////////////////////////////////////////////////////////////////////////
    //Reset selected items to default state
    m_resetSelectedItem = menu->addAction(tr("Reset to default"));
    connect(m_resetSelectedItem, &QAction::triggered, this, [this]
        {
            QVector<CBaseLibraryItem*> selectedItems = m_libraryTreeViewDock->GetSelectedItems();
            if (selectedItems.count() && selectedItems.first())
            {
                QVector<CBaseLibraryItem*> tempSelectionList = m_libraryTreeViewDock->GetSelectedItems();
                while (tempSelectionList.count() > 0)
                {
                    CParticleItem* pParticle = static_cast<CParticleItem*>(tempSelectionList.takeFirst());
                    Library_ItemReset(pParticle);
                }
            }
        });
    //////////////////////////////////////////////////////////////////////////

    action = menu->addAction(tr("Edit Hotkeys"));
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Edit Hotkeys"));
    connect(action, &QAction::triggered, this, [&]()
        {
            QKeySequenceEditorDialog* dlg = UIFactory::GetHotkeyEditor();
            if (dlg->exec() == QDialog::Accepted)
            {
                GetIEditor()->GetParticleUtils()->HotKey_SaveCurrent();
                UnregisterActions();
                CreateMenuBar();
                CreateShortcuts();
                RegisterActions();
                m_libraryTreeViewDock->RemapHotKeys();
            }
        });
#undef ADD_ACTION
}

void CMainWindow::View_PopulateMenu()
{
    CRY_ASSERT(m_viewMenu);
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
    QMenu* menu = m_viewMenu;
    menu->clear();
    QAction* action = nullptr;
    m_showLayoutMenu = new QMenu(tr("Show Layouts"), menu);
#define ADD_ACTION(text, callback, tooltip, icon, key)                                            \
    {                                                                                             \
        action = menu->addAction(QIcon("Editor/UI/Icons/toolbar/" icon), tr(text));              \
        if (key != 0) {                                                                           \
            action->setShortcut(key); }                                                           \
        action->setToolTip(tr(tooltip));                                                          \
        connect(action, &QAction::triggered, this, &CMainWindow::callback, Qt::QueuedConnection); \
    }

    QList<QAction*> menuActions = m_layoutMenu->actions();
    for (int i = 0, numActions = menuActions.size(); i < numActions; ++i)
    {
        action = menuActions[i];
        QFileInfo fileInfo(action->text());
        if (!fileInfo.exists())
        {
            m_layoutMenu->removeAction(action);
            action->setParent(NULL);
            delete action;
        }
    }
    StateSave_layoutMenu();


    menuActions = m_layoutMenu->actions();
    for (int i = 1, numActions = menuActions.size(); i <= MAX_NUMBER_OF_LAYOUT_MENU && i <= numActions; i++)
    {
        action = menuActions[numActions - i];
        m_showLayoutMenu->addAction(action);
    }

    //Add the "default" layout option to show layouts
    action = m_showLayoutMenu->addAction("Default Layout");
    connect(action, &QAction::triggered, this, &CMainWindow::ResetToDefaultEditorLayout);

    menu->addMenu(m_showLayoutMenu);
    ADD_ACTION("Import layout...", OnActionViewImportLayout, "Import layout", "", GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("View.Import layouts"));
    ADD_ACTION("Export layout...", OnActionViewExportLayout, "Export layout", "", GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("View.Export layouts"));
    ADD_ACTION((m_attributeViewDock->isHidden() ? "Show attributes" : "Hide attributes"), OnActionViewHideAttributes, "Show attribute view", "", GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("View.Show attribute"));
    ADD_ACTION((m_libraryTreeViewDock->isHidden() ? "Show library" : "Hide library"), OnActionViewHideLibrary, "Show Library View", "", GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("View.show library"));
    ADD_ACTION((m_previewDock->isHidden() ? "Show preview" : "Hide preview"), OnActionViewHidePreview, "Show Preview View", "", GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("View.show preview"));
    ADD_ACTION((m_LoDDock->isHidden() ? "Show Level of Detail" : "Hide Level of Detail"), OnActionViewHideLOD, "Show Level of Detail View", "", GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("View.show level of detail"));

    ADD_ACTION("Reset to Default Layout", ResetToDefaultEditorLayout, "Reset layout", "", GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("View Menu.Reset Layout"));

    ADD_ACTION("Documentation", OnActionViewDocumentation, "Open Particle Editor documentation.", "", 0);

#undef ADD_ACTION
}

void CMainWindow::UpdatePalette()
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetEditorSettings());

    QString stylesheetPath = "Editor/Styles/stylesheet_Dark.qss";
    //Reload Style sheet with the new colors
    QString processedStyle = ProcessStyleSheet(stylesheetPath);

    if (processedStyle.length() > 0)
    {
        hide();
        setStyleSheet(processedStyle); // Stylesheet also propagates to the docks

        QCustomColorDialog* cdlg = UIFactory::GetColorPicker();
        cdlg->setStyleSheet(processedStyle);

        QGradientColorDialog* gdlg = UIFactory::GetGradientEditor(SCurveEditorContent(), { QGradientStop(0.0, Qt::white), QGradientStop(1.0, Qt::white) });
        gdlg->setStyleSheet(processedStyle);
        gdlg->UpdateColors(m_StyleColors);

        QKeySequenceEditorDialog* hkdlg = UIFactory::GetHotkeyEditor();
        hkdlg->setStyleSheet(processedStyle);

        QColorEyeDropper* cedp = UIFactory::GetColorEyeDropper();
        cedp->setStyleSheet(processedStyle);

        m_attributeViewDock->UpdateColors(m_StyleColors);
        m_libraryTreeViewDock->UpdateColors(m_StyleColors);
        ensurePolished();
        show();
    }
}

QString CMainWindow::ProcessStyleSheet(QString const& fileName)
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetSystem());
    CRY_ASSERT(GetIEditor()->GetSystem()->GetILog());

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        char buffer[512];
        sprintf(buffer, "Stylesheet '%s' is invalid.", fileName.toUtf8().data());
        GetIEditor()->GetSystem()->GetILog()->LogWarning(buffer);
        return "";
    }

    //////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////

    QXmlStreamReader stream(&file);
    QString qss = "";
    while (!stream.atEnd())
    {
        stream.readNext();
        if (stream.isStartElement())
        {
            //COLORDEF
            if (stream.name() == "COLORDEF")
            {
                QPair<QString, QColor> key;
                key.second = QColor(Qt::red);
                QXmlStreamAttributes att = stream.attributes();
                for (QXmlStreamAttribute attr : att)
                {
                    if (attr.name().compare(QLatin1String("name"), Qt::CaseInsensitive) == 0)
                    {
                        key.first = attr.value().toString();
                    }
                    if (attr.name().compare(QLatin1String("value"), Qt::CaseInsensitive) == 0)
                    {
                        QString fullColor = attr.value().toString();
                        QColor temp;
                        temp.setRgba(fullColor.toUInt(0, 16));
                        key.second = temp;
                    }
                }
                if (!key.first.isEmpty())
                {
                    m_StyleColors.insert(key.first, key.second);
                }
            }

            //QSS
            if (stream.name() == "QSS")
            {
                qss = stream.readElementText();
            }
        }
    }
    file.close();

    //////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////

    QString out = "";
    QString varName = "";
    auto i = qss.cbegin();
    //Normal State
normal:
    while (i != qss.end())
    {
        char c = i->toLatin1();
        switch (c)
        {
        case '@':
            i++;
            goto var;
        case '\0':
            goto end;
        default:
            out.append(*i);
            i++;
        }
    }
    goto end;
var:
    //Reading Var State
    while (i != qss.end())
    {
        char c = i->toLatin1();

        //All characters valid in indentifier
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z')
            )
        {
            varName.append(*i);
            i++;
        }
        else
        {
            switch (c)
            {
            case '\0':
                goto end;
            default:
                //We are finished with reading the current varName
                out.append(GetColorStringByName(varName));
                varName.clear();
                out.append(*i);
                i++;
                goto normal;
            }
        }
    }
    goto end;
end:

    //////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////

    return out;
}

QString CMainWindow::GetColorStringByName(QString const& name)
{
    auto it = m_StyleColors.find(name);
    if (it != m_StyleColors.end())
    {
        int r, g, b, a;
        it->getRgb(&r, &g, &b, &a);
        QString str;
        str = str.sprintf("rgba(%d,%d,%d,%d)", r, g, b, a);
        return str;
    }
    else
    {
        return "#FFFF00FF"; //Debug color
    }
}


void CMainWindow::CreateShortcuts()
{
    CRY_ASSERT(m_attributeViewDock);
    CRY_ASSERT(m_previewDock);

    while (m_shortcuts.count() > 0)
    {
        m_shortcuts.removeLast();
    }

    //File Menu
    m_shortcuts.push_back(new QShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Menus.File Menu"), this));
    connect(m_shortcuts.back(), &QShortcut::activated, this, [&]()
        {
            CRY_ASSERT(GetIEditor());
            CRY_ASSERT(GetIEditor()->GetParticleUtils());
            if (!GetIEditor()->GetParticleUtils()->HotKey_IsEnabled())
            {
                return;
            }
            if (m_fileMenu && m_menuBar)
            {
                QPoint p = m_menuBar->mapToGlobal(m_menuBar->pos());
                p.setX(m_menuBar->mapToGlobal(m_fileMenu->pos()).x());
                p.setY(p.y() + m_menuBar->height());
                m_fileMenu->exec(p);
            }
        });

    //Edit Menu
    m_shortcuts.push_back(new QShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Menus.Edit Menu"), this));
    connect(m_shortcuts.back(), &QShortcut::activated, this, [&]()
        {
            CRY_ASSERT(GetIEditor());
            CRY_ASSERT(GetIEditor()->GetParticleUtils());
            if (!GetIEditor()->GetParticleUtils()->HotKey_IsEnabled())
            {
                return;
            }
            if (m_editMenu && m_menuBar)
            {
                QPoint p = m_menuBar->mapToGlobal(m_menuBar->pos());
                p.setX(m_menuBar->mapToGlobal(m_editMenu->pos()).x());
                p.setY(p.y() + m_menuBar->height());
                m_editMenu->exec(p);
            }
        });

    //View Menu
    m_shortcuts.push_back(new QShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Menus.View Menu"), this));
    connect(m_shortcuts.back(), &QShortcut::activated, this, [&]()
        {
            CRY_ASSERT(GetIEditor());
            CRY_ASSERT(GetIEditor()->GetParticleUtils());
            if (!GetIEditor()->GetParticleUtils()->HotKey_IsEnabled())
            {
                return;
            }
            if (m_viewMenu && m_menuBar)
            {
                QPoint p = m_menuBar->mapToGlobal(m_menuBar->pos());
                p.setX(m_menuBar->mapToGlobal(m_viewMenu->pos()).x());
                p.setY(p.y() + m_menuBar->height());
                m_viewMenu->exec(p);
            }
        });

    //Insert Comment
    m_shortcuts.push_back(m_attributeViewDock->GetShortcut(DockableAttributePanel::ParamShortcuts::InsertComment));

    //Focus Camera
    m_shortcuts.push_back(m_previewDock->GetShortcut(DockablePreviewPanel::Shortcuts::FocusCameraOnEmitter));
}

void CMainWindow::Library_ItemNameValidationRequired(IDataBaseItem* item, const QString& currentName, const QString& nextName, bool& proceed)
{
    if (nextName.isEmpty() || currentName == nextName)
    {
        proceed = false;
        return; // block name change
    }

    if (nextName.toStdString().find_first_not_of(VALID_LIBRARY_ITEM_CHARACTERS) != std::string::npos)
    {
        QMessageBox::warning(this, "Warning", "Invalid Item Name.Symbols / glyphs is not allowed in the name.\n");
        proceed = false;
        return; // block name change
    }
    proceed = true;
}

void CMainWindow::Library_ItemRenamed(IDataBaseItem* item, const QString& oldName, const QString& currentName, const QString newLib)
{
    if (!item)
    {
        return; //non-database item was modified (i.e. folder)
    }
    int lastGroup = currentName.lastIndexOf('.');
    QString newParent = currentName.left(lastGroup);
    QString libName = item->GetLibrary()->GetName();

    IEditorParticleManager* pParticleMgr = GetIEditor()->GetParticleManager();
    CParticleItem* pParent = static_cast<CParticleItem*>(pParticleMgr->FindItemByName(libName + "." + newParent));
    CParticleItem* pParticle = static_cast<CParticleItem*>(item);
    if (pParent && pParent->IsParticleItem && pParent != pParticle)
    {
        pParticle->SetParent(pParent);
    }
    else
    {
        pParticle->SetParent(NULL);
    }
    NotifyExceptSelf(eNotify_OnDataBaseUpdate);
}

void CMainWindow::Library_UpdateTreeItemStyle(IDataBaseItem* item, int column)
{
    if (!item)
    {
        return;
    }

    if (item->GetType() == EDB_TYPE_PARTICLE)
    {
        CParticleItem* pParticle = static_cast<CParticleItem*>(item);
        bool isLod = pParticle->GetEffect()->HasLevelOfDetail();
        CLibraryTreeViewItem* treeItem = m_libraryTreeViewDock->GetTreeItemFromPath(tr(item->GetFullName().toUtf8().data()));
        if (!treeItem)
        {
            //when grouping/ungrouping we can sometimes attempt to update a style before the treeitem actually exists.
            //when this happens we return to prevent crashing.
            return;
        }
        CRY_ASSERT(pParticle->IsParticleItem);
        // After add in group particle. The icon should updated based on the group.
        ParticleParams params = pParticle->GetEffect()->GetParticleParams();
        if (isLod)
        {
            if (params.bGroup)
            {
                treeItem->setIcon(column, QIcon("Editor/UI/Icons/treeview/ParticleEditor/group_with_lod_icon.png"));
            }
            else
            {
                treeItem->setIcon(column, QIcon("Editor/UI/Icons/treeview/ParticleEditor/lod_icon.png"));
            }
        }
        else
        {
            if (params.bGroup)
            {
                treeItem->setIcon(column, QIcon("Editor/UI/Icons/treeview/ParticleEditor/group_icon.png"));
            }
            else
            {
                treeItem->setIcon(column, QIcon("Editor/UI/Icons/treeview/ParticleEditor/empty_icon.png"));
            }
        }
    }
}

void CMainWindow::Library_ItemCopied(IDataBaseItem* item)
{
    if (!item)
    {
        return;
    }
    XmlNodeRef node = GetIEditor()->GetSystem()->CreateXmlNode("Particles");
    CBaseLibraryItem::SerializeContext ctx(node, false);
    ctx.bCopyPaste = true;

    CClipboard clipboard(this);
    item->Serialize(ctx);
    clipboard.Put(node);
}

void CMainWindow::Library_ItemPasted(IDataBaseItem* target, bool overrideSafety /*= false*/)
{
    if (!target)
    {
        return;
    }
    if (target->GetLibrary())
    {
        m_libraryTreeViewDock->SelectLibraryAndItemByName(QString(target->GetLibrary()->GetName()), QString(target->GetName()));
    }

    CClipboard clipboard(this);
    if (clipboard.IsEmpty())
    {
        return;
    }
    XmlNodeRef node = clipboard.Get();

    if (!node)
    {
        return;
    }

    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(GetIEditor()->GetParticleManager());
    
    EditorUIPlugin::ScopedLibraryModifyUndoPtr modifyUndo;    
    AZStd::string libName = target->GetLibrary()->GetName().toUtf8().data();
    EBUS_EVENT_RESULT(modifyUndo, EditorLibraryUndoRequestsBus, AddScopedLibraryModifyUndo, libName);

    if (strcmp(node->getTag(), "Childs") == 0)
    {
        //copy the selected particles to the children of the selected item
        //it is not possible to only copy params as the params may overwrite eachother.
        //so we don't prompt

        //get the xml of the target
        XmlNodeRef particleNode = GetIEditor()->GetSystem()->CreateXmlNode("Particles");
        CBaseLibraryItem::SerializeContext ctx(particleNode, false);
        ctx.bCopyPaste = true;
        target->Serialize(ctx);
        //check for an existing "Child" tab if found remove ours
        XmlNodeRef childRoot = particleNode->findChild("Childs");
        if (childRoot == NULL)
        {
            particleNode->addChild(node);
        }
        else
        {
            for (int i = 0; i < node->getChildCount(); i++)
            {
                XmlNodeRef copiedEmitter = node->getChild(i);
                childRoot->addChild(copiedEmitter);
            }
        }
        GetIEditor()->GetParticleManager()->PasteToParticleItem(static_cast<CParticleItem*>(target), particleNode, true);
        if (target->GetLibrary())
        {
            m_libraryTreeViewDock->ForceLibrarySync(QString(target->GetLibrary()->GetName()));
        }

        return;
    }

    bool replaceAll = true;
    if (!overrideSafety)
    {
        QCopyModalDialog dlg(this, QString(target->GetFullName()));
        bool result = dlg.exec(replaceAll);
        if (!result)
        {
            return;
        }
    }

    if (strcmp(node->getTag(), "Particles") == 0)
    {
        CParticleItem* pParticle = static_cast<CParticleItem*>(target);
        CRY_ASSERT(pParticle);
        node->delAttr("Name");
        node->setAttr("Name", target->GetName().toUtf8().data());
        GetIEditor()->GetParticleManager()->PasteToParticleItem(pParticle, node, replaceAll);
        if (target->GetLibrary())
        {
            m_libraryTreeViewDock->ForceLibrarySync(QString(target->GetLibrary()->GetName()));
        }
    }
}

void CMainWindow::Library_ItemsPastedToFolder(IDataBaseLibrary* lib, const QStringList& PasteList)
{
    CClipboard clipboard(this);
    if (clipboard.IsEmpty())
    {
        return;
    }
    XmlNodeRef node = clipboard.Get();

    if (!node)
    {
        return;
    }

    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(GetIEditor()->GetParticleManager());
    //This function is only for multiple selections
    if (strcmp(node->getTag(), "Childs") != 0)
    {
        return;
    }

    EditorUIPlugin::ScopedLibraryModifyUndoPtr modifyUndo;
    AZStd::string libName = lib->GetName().toUtf8().data();
    EBUS_EVENT_RESULT(modifyUndo, EditorLibraryUndoRequestsBus, AddScopedLibraryModifyUndo, libName);

    for (int i = 0; i < node->getChildCount(); i++)
    {
        //The order in the clipboard matches the order in the list so we can just loop through
        XmlNodeRef copiedEmitter = node->getChild(i);
        CParticleItem* target = static_cast<CParticleItem*>(lib->GetManager()->FindItemByName(PasteList[i]));
        if (target)
        {
            GetIEditor()->GetParticleManager()->PasteToParticleItem(target, copiedEmitter, true);
        }
    }
}

void CMainWindow::Library_ItemDuplicated(const QString& itemPath, QString pasteTo)
{
    QVector<CLibraryTreeViewItem*> selectedItems;
    if (itemPath.isEmpty())
    {
        selectedItems = m_libraryTreeViewDock->GetSelectedTreeItems();
    }
    else
    {
        selectedItems = m_libraryTreeViewDock->GetActionItems(itemPath);
    }

    //duplicate function only works with one selected item. The Library_ItemPasted will modify selected item list.
    if (selectedItems.count() != 1)
    {
        return;
    }

    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(GetIEditor()->GetParticleManager());
   

    CLibraryTreeViewItem* item = selectedItems.takeFirst();
    if (item->IsVirtualItem()) // Could not duplicate folders
    {
        return;
    }
    QString fullPath = item->GetFullPath();
    bool isVirtual = item->IsVirtualItem();

    //undo batch for duplicate item: add item, paste item
    EditorUIPlugin::ScopedBatchUndoPtr batchUndo;
    EBUS_EVENT_RESULT(batchUndo, EditorLibraryUndoRequestsBus, AddScopedBatchUndo, "Add Item");

    Library_ItemCopied(item->GetItem());
    CLibraryTreeViewItem* newItem = m_libraryTreeViewDock->AddDuplicateTreeItem(fullPath, isVirtual, pasteTo);

    if (newItem)
    {
        Library_ItemPasted(newItem->GetItem(), true); //don't prompt
    }
}

void CMainWindow::Library_ItemReset(IDataBaseItem* target, SLodInfo *curLod)
{
    if (!target)
    {
        return;
    }
    CParticleItem* pParticle = static_cast<CParticleItem*>(target);
    if (!pParticle)
    {
        return;
    }

    EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, true);

    pParticle->ResetToDefault(curLod);

    // Update particles.
    pParticle->Update();
    m_attributeViewDock->RefreshParameterUI(pParticle, curLod);

    EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, false);

    //save item undo
    OnItemUndoPoint(target->GetFullName(), false, curLod);
}

void CMainWindow::Library_TreeFilledFromLibrary(IDataBaseLibrary* lib, CLibraryTreeView* view)
{
    //set all the disabled items
    QVector<CLibraryTreeViewItem*> items = view->GetChildrenOfTreeItem(""); //"" is the root node for trees
    //start at 1 to avoid the "InvisibleRootItem"
    for (unsigned int i = 1; i < items.count(); i++)
    {
        if (items[i]->IsVirtualItem())
        {
            continue;
        }
        else if (items[i]->GetItem())
        {
            CParticleItem* pParticle = static_cast<CParticleItem*>(items[i]->GetItem());
            IParticleEffect* pEffect = pParticle->GetEffect();
            if (pEffect)
            {
                items[i]->SetEnabled(pEffect->IsEnabled());
            }
        }
    }
    m_attributeViewDock->RefreshCurrentTab();
    m_LoDDock->RefreshGUI();
}

void CMainWindow::Library_ItemDragged(CLibraryTreeViewItem* item)
{
    if (!item || item->IsVirtualItem())
    {
        return;
    }

    //if it's not top effect, return
    CParticleItem* pParticle = static_cast<CParticleItem*>(item->GetItem());
    if ((!pParticle) || (!pParticle->GetEffect()) || pParticle->GetEffect()->GetParent())
    {
        return;
    }

    GetIEditor()->GetParticleUtils()->SetViewportDragOperation([](CViewport* vp, int dpx, int dpy, void* custom)
    {
        CParticleItem* parItem = (CParticleItem*)custom;

        if (parItem->GetLibrary()->IsModified())
        {
            parItem->GetLibrary()->Save();
        }

        //create an entity with EditorParticleComponent; modified from ComponentDataModel::DragDropHandler
        AzToolsFramework::ScopedUndoBatch undo("Create entity from particle effect");

        const AZStd::string name = AZStd::string::format("Particle_%s", parItem->GetEffect()->GetFullName().c_str());

        AZ::Entity* newEntity = aznew AZ::Entity(name.c_str());
        if (newEntity)
        {
            EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, AddRequiredComponents, *newEntity);
            // Set initial transform.
            auto* transformComponent = newEntity->FindComponent<AzToolsFramework::Components::TransformComponent>();
            if (transformComponent)
            {
                bool collideWithTerrain = true;
                QPoint screenPoint = { dpx, dpy };
                Vec3 worldPosition = vp->ViewToWorld(screenPoint, &collideWithTerrain);
                transformComponent->SetWorldTM(AZ::Transform::CreateTranslation(LYVec3ToAZVec3(worldPosition)));
            }

            // Add the entity to the editor context, which activates it and creates the sandbox object.
            EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, AddEditorEntity, newEntity);

            // Create Entity metrics event
            EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, EntityCreated, newEntity->GetId());
                        
            // Add the EditorParticleComponent to entity
            newEntity->Deactivate();
            AZ::Uuid componentType = "{0F35739E-1B40-4497-860D-D6FF5D87A9D9}"; //class id of EditorParticleComponent
            AZ::Component* newComponent = newEntity->CreateComponent(componentType);
            AZ_Assert(newComponent, "Can't create EditorParticleComponent with uuid %s", componentType.ToString<AZStd::string>().c_str());

            // Add Component metrics event
            EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, ComponentAdded, newEntity->GetId(), componentType);

            //reactive entity with component so the component can recieve following ebus requests
            newEntity->Activate();

            //setup library and emitter
            //try find the lib item
            IDataBaseItem* libItem = GetIEditor()->GetParticleManager()->FindItemByName(parItem->GetEffect()->GetFullName().c_str());
            if (libItem != nullptr)
            {
                AZStd::string libName = libItem->GetLibrary()->GetFilename().toUtf8().data();
                EBUS_EVENT_ID(newEntity->GetId(), LmbrCentral::EditorParticleComponentRequestBus, SetEmitter, AZStd::string(parItem->GetEffect()->GetFullName()), libName);
            }
            
            // Prepare undo command last so it captures the final state of the entity.
            AzToolsFramework::EntityCreateCommand* command = aznew AzToolsFramework::EntityCreateCommand(static_cast<AZ::u64>(newEntity->GetId()));
            command->Capture(newEntity);
            command->SetParent(undo.GetUndoBatch());

            // Select the new entity (and deselect others).
            AzToolsFramework::EntityIdList selection = { newEntity->GetId() };
            EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, SetSelectedEntities, selection);
        }

    }, item->GetItem());
}

void CMainWindow::Library_ItemAdded(CBaseLibraryItem* item, const QString& name)
{
    //handle the rename to correct hierarchy of the particle system
    Library_ItemRenamed(item, "", name);
    //paste in the amazon default here
    GetIEditor()->GetParticleManager()->LoadDefaultEmitter(static_cast<CParticleItem*>(item)->GetEffect());
}

void CMainWindow::Library_DragOperationFinished()
{
    GetIEditor()->GetParticleUtils()->SetViewportDragOperation(nullptr, nullptr);
    m_attributeViewDock->RefreshCurrentTab();
    NotifyExceptSelf(eNotify_OnDataBaseUpdate);
}

void CMainWindow::Library_ItemEnabledStateChanged(CBaseLibraryItem* item, const bool& state)
{
    CParticleItem* pParticle = static_cast<CParticleItem*>(item);
    m_attributeViewDock->SetEnabledParameter(pParticle, state);
    OnItemUndoPoint(item->GetFullName(), false, nullptr);
}

void CMainWindow::Library_PopulateLibraryContextMenu(ContextMenu* toAddTo, const QString& libName)
{
    CRY_ASSERT(m_libraryTreeViewDock);

    QMenu* menu = toAddTo;
    //ADD NEW EMITTER/FOLDER//////////////////////////////////////////////////
    QMenu* submenu = new ContextMenu("Add New", toAddTo);
    toAddTo->addMenu(submenu);
    submenu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::ADD_ITEM, libName, tr("Add Particle"), false, submenu, Qt::QueuedConnection));
    submenu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::ADD_FOLDER, libName, tr("Add Folder"), false, submenu, Qt::QueuedConnection));
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::SAVE, tr("Save"), false, menu, Qt::QueuedConnection));
    toAddTo->addSeparator();
    //DUPLICATE//////////////////////////////////////////////////////////////////
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::DUPLICATE_LIB, tr("Duplicate"), false, menu, Qt::QueuedConnection));
    //ENABLE-DISABLE ALL//////////////////////////////////////////////////////
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::ENABLE_ALL, tr("Enable All"), false, menu, Qt::QueuedConnection));
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::DISABLE_ALL, tr("Disable All"), false, menu, Qt::QueuedConnection));
    //EXPAND-COLLAPSE ALL/////////////////////////////////////////////////////
    toAddTo->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::TreeActions::EXPAND_ALL, tr("Expand All"), false, toAddTo, Qt::QueuedConnection));
    toAddTo->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::TreeActions::COLLAPSE_ALL, tr("Collapse All"), false, toAddTo, Qt::QueuedConnection));
    menu->addSeparator();
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::REMOVE, tr("Remove"), false, menu, Qt::QueuedConnection));
    menu->addAction(m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::LibraryActions::RELOAD, tr("Reload"), false, menu, Qt::QueuedConnection));
}

void CMainWindow::Library_DecorateTreesDefaultView(const QString& lib, DefaultViewWidget* view)
{
    CRY_ASSERT(view);
    view->SetLabel(tr("Particle library is empty\nAdd a new folder or emitter to continue."));
    view->AddButton("Add Particle", m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::ADD_ITEM, lib, tr("Add Particle"), false, view, Qt::QueuedConnection));
    view->AddButton("Add Folder", m_libraryTreeViewDock->GetMenuAction(DockableLibraryPanel::ItemActions::ADD_FOLDER, lib, tr("Add Particle"), false, view, Qt::QueuedConnection));
}

void CMainWindow::Library_SelecteItem(const QString& fullname)
{
    if (!fullname.isNull())
    {
        if (fullname == m_attributeViewDock->GetCurrentTabText())
        {   
            //already opened, refresh tab
            m_attributeViewDock->RefreshCurrentTab();
        }
        else
        {
            //wasn't selected. select the item
            QStringList undoParticleNameList = fullname.split(".");
            QString libname = undoParticleNameList.takeFirst();
            QString name = undoParticleNameList.join(".");
            m_libraryTreeViewDock->SelectLibraryAndItemByName(libname, name);
        }
    }
}

void CMainWindow::Library_CopyTreeItems(QVector<CLibraryTreeViewItem*> items, bool copyAsChild /*= false*/)
{
    if (items.count() == 1 && !copyAsChild)
    {
        if (!items.first())
        {
            return;
        }
        return Library_ItemCopied((items.first())->GetItem());
    }

    CClipboard clipboard(this);
    XmlNodeRef node = GetIEditor()->GetSystem()->CreateXmlNode("Childs");
    while (items.count() > 0)
    {
        CBaseLibraryItem* item = items.takeFirst()->GetItem();
        if (!item)
        {
            continue;
        }
        XmlNodeRef _node = node->newChild("Particles");
        CBaseLibraryItem::SerializeContext ctx(_node, false);
        ctx.bCopyPaste = true;
        item->Serialize(ctx);
    }
    clipboard.Put(node);
}

void CMainWindow::RegisterActions()
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
    IEditorParticleUtils* utils = GetIEditor()->GetParticleUtils();

    //////////////////////////////////////////////////////////////////////////
    //General Purpose
    //Undo
    QAction* action = new QAction(this);
    action->setShortcut(utils->HotKey_GetShortcut("Edit Menu.Undo"));
    connect(action, &QAction::triggered, this, [=]() { OnActionStandardUndo(); });
    addAction(action);
    //Redo
    action = new QAction(this);
    action->setShortcut(utils->HotKey_GetShortcut("Edit Menu.Redo"));
    connect(action, &QAction::triggered, this, [=]() { OnActionStandardRedo(); });
    addAction(action);
    //DUPLICATE///////////////////////////////////////////////////////////////////
    action = new QAction(this);
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Duplicate"));
    connect(action, &QAction::triggered, this, [this]()
        {
            Library_ItemDuplicated("", ""); // Default duplicate. Duplicate selected item.
        });
    addAction(action);

    //CLOSE///////////////////////////////////////////////////////////////////
    action = new QAction(this);
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("File Menu.Close"));
    connect(action, &QAction::triggered, this, [this]()
        {
            m_RequestedClose = true;
        });
    addAction(action);

    //HOTKEY REMAPPING////////////////////////////////////////////////////////
    action = new QAction(this);
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Edit Hotkeys"));
    connect(action, &QAction::triggered, this, [&]()
        {
            QKeySequenceEditorDialog* dlg = UIFactory::GetHotkeyEditor();
            if (dlg->exec() == QDialog::Accepted)
            {
                GetIEditor()->GetParticleUtils()->HotKey_SaveCurrent();
                UnregisterActions();
                CreateMenuBar();
                CreateShortcuts();
                RegisterActions();
                m_libraryTreeViewDock->RemapHotKeys();
            }
        });
    addAction(action);
}

void CMainWindow::UnregisterActions()
{
    for (QAction* action : actions())
    {
        action->setParent(nullptr);
        removeAction(action);
        SAFE_DELETE(action);
    }
}

void CMainWindow::RefreshItemUI()
{
    if (m_LoDDock)
    {
       m_LoDDock->RefreshGUI();
    }
}

void CMainWindow::UpdateItemUI(const AZStd::string& itemId, bool selected, int lodIdx)
{
    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

    CParticleItem* item = static_cast<CParticleItem*>(GetIEditor()->GetParticleManager()->FindItemByName(itemId.c_str()));
    if (item)
    {
        if (selected)
        {
            //select the lib and item
            QStringList nameList = QString(itemId.c_str()).split(".");
            QString libname = nameList.takeFirst();
            QString name = nameList.join(".");
            m_libraryTreeViewDock->SelectLibraryAndItemByName(libname, name);

            //set lod
            SLodInfo* lod = item->GetEffect()->GetLevelOfDetail(lodIdx);
            if (lod)
            {
                m_LoDDock->SelectLod(lod);
            }
        }

        //update enable/disable and lod icon
        CLibraryTreeViewItem* titem = m_libraryTreeViewDock->GetTreeItemFromPath(itemId.c_str());
        if (titem)
        {
            bool enable = item->GetEffect()->GetParticleParams().bEnabled;
            titem->setCheckState(LIBRARY_TREEVIEW_INDICATOR_COLUMN, enable? Qt::CheckState::Checked:Qt::CheckState::Unchecked);
            m_libraryTreeViewDock->UpdateIconStyle(item->GetLibrary()->GetName(), itemId.c_str());
            m_LoDDock->RefreshGUI();
        }
    }
}

void CMainWindow::LibraryChangedInManager(const char* libraryName)
{
    if (GetIEditor()->GetParticleManager()->FindLibrary(QString::fromUtf8(libraryName)))
    {
        m_needLibraryRefresh = true;
        RefreshLibraries();
        
        CParticleItem* item = static_cast<CParticleItem*>(GetIEditor()->GetParticleManager()->GetSelectedItem());
        if (item)
        {
            AZStd::string itemFullName = item->GetFullName().toUtf8().data();
            UpdateItemUI(itemFullName, true, 0);
        }
    }
}

#include <QT/MainWindow.moc>
