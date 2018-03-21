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

// include required headers
#include "MainWindow.h"
#include "EMStudioManager.h"
#include "PluginManager.h"
#include "PreferencesWindow.h"
#include "Workspace.h"
#include "KeyboardShortcutsWindow.h"
#include "DockWidgetPlugin.h"
#include "LoadActorSettingsWindow.h"
#include "UnitScaleWindow.h"
#include "UnitSetupWindow.h"

#include <LyViewPaneNames.h>

// include Qt related
#include <QMenu>
#include <QStatusBar>
#include <QMenuBar>
#include <QSignalMapper>
#include <QTextEdit>
#include <QDir>
#include <QMessageBox>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QSettings>
#include <QApplication>
#include <QDesktopServices>
#include <QCheckBox>
#include <QMimeData>
#include <QDirIterator>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QAbstractEventDispatcher>

// include MCore related
#include <MCore/Source/LogManager.h>
#include <MCore/Source/FileSystem.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <ISystem.h>

// Include this on windows to detect device remove and insert messages, used for the game controller support.
#ifdef MCORE_PLATFORM_WINDOWS
    #include <dbt.h>
#endif


namespace EMStudio
{
    class SaveDirtyWorkspaceCallback
        : public SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyWorkspaceCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        SaveDirtyWorkspaceCallback()
            : SaveDirtyFilesCallback()                                                              {}
        ~SaveDirtyWorkspaceCallback()                                                               {}

        enum
        {
            TYPE_ID = 0x000002345
        };
        uint32 GetType() const override                                                             { return TYPE_ID; }
        uint32 GetPriority() const override                                                         { return 0; }
        bool GetIsPostProcessed() const override                                                    { return false; }

        void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) override
        {
            Workspace* workspace = GetManager()->GetWorkspace();
            if (workspace->GetDirtyFlag())
            {
                // add the filename to the dirty filenames array
                outFileNames->push_back(workspace->GetFilename());

                // add the link to the actual object
                ObjectPointer objPointer;
                objPointer.mWorkspace = workspace;
                outObjects->push_back(objPointer);
            }
        }

        int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) override
        {
            MCORE_UNUSED(filenamesToSave);

            const size_t numObjects = objects.size();
            for (size_t i = 0; i < numObjects; ++i)
            {
                // get the current object pointer and skip directly if the type check fails
                ObjectPointer objPointer = objects[i];
                if (objPointer.mWorkspace == nullptr)
                {
                    continue;
                }

                Workspace* workspace = objPointer.mWorkspace;

                // has the workspace been saved already or is it a new one?
                if (workspace->GetFilenameString().empty())
                {
                    // open up save as dialog so that we can choose a filename
                    const AZStd::string filename = GetMainWindow()->GetFileManager()->SaveWorkspaceFileDialog(GetMainWindow());
                    if (filename.empty())
                    {
                        return DirtyFileManager::CANCELED;
                    }

                    // save the workspace using the newly selected filename
                    AZStd::string command = AZStd::string::format("SaveWorkspace -filename \"%s\"", filename.c_str());
                    commandGroup->AddCommandString(command);
                }
                else
                {
                    // save workspace using its filename
                    AZStd::string command = AZStd::string::format("SaveWorkspace -filename \"%s\"", workspace->GetFilename());
                    commandGroup->AddCommandString(command);
                }
            }

            return DirtyFileManager::FINISHED;
        }

        const char* GetExtension() const override       { return "emfxworkspace"; }
        const char* GetFileType() const override        { return "workspace"; }
    };

    // constructor
    MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags)
        : QMainWindow(parent, flags)
    {
        mAutosaveTimer                  = nullptr;
        mPreferencesWindow              = nullptr;
        mNodeSelectionWindow            = nullptr;
        mApplicationMode                = nullptr;
        mDirtyFileManager               = nullptr;
        mFileManager                    = nullptr;
        mShortcutManager                = nullptr;
        mImportActorCallback            = nullptr;
        mRemoveActorCallback            = nullptr;
        mRemoveActorInstanceCallback    = nullptr;
        mImportMotionCallback           = nullptr;
        mRemoveMotionCallback           = nullptr;
        mCreateMotionSetCallback        = nullptr;
        mRemoveMotionSetCallback        = nullptr;
        mLoadMotionSetCallback          = nullptr;
        mCreateAnimGraphCallback        = nullptr;
        mRemoveAnimGraphCallback        = nullptr;
        mLoadAnimGraphCallback          = nullptr;
        mSelectCallback                 = nullptr;
        mUnselectCallback               = nullptr;
        mSaveWorkspaceCallback          = nullptr;
    }


    // destructor
    MainWindow::~MainWindow()
    {
        if (mAutosaveTimer)
        {
            mAutosaveTimer->stop();
        }

        SavePreferences();

        // Delete all actor instances back to front which belong to the Animation Editor and are not managed by the asset system yet.
        for (int i = EMotionFX::GetActorManager().GetNumActorInstances() - 1; i >= 0; i--)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            actorInstance->Destroy();
        }

        // Same for actors.
        for (int i = EMotionFX::GetActorManager().GetNumActors() - 1; i >= 0; i--)
        {
            EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

            if (actor->GetIsOwnedByRuntime())
            {
                continue;
            }

            actor->Destroy();
        }

        delete mShortcutManager;
        delete mFileManager;
        delete mDirtyFileManager;

        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mImportActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(mImportMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mLoadMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mLoadAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSaveWorkspaceCallback, false);
        delete mImportActorCallback;
        delete mRemoveActorCallback;
        delete mRemoveActorInstanceCallback;
        delete mImportMotionCallback;
        delete mRemoveMotionCallback;
        delete mCreateMotionSetCallback;
        delete mRemoveMotionSetCallback;
        delete mLoadMotionSetCallback;
        delete mCreateAnimGraphCallback;
        delete mRemoveAnimGraphCallback;
        delete mLoadAnimGraphCallback;
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mSaveWorkspaceCallback;
    }


    // init the main window
    void MainWindow::Init()
    {
        // tell the mystic Qt library about the main window
        MysticQt::GetMysticQt()->SetMainWindow(this);

        QSettings settings(this);

        settings.beginGroup("EMotionFX");

        // set the size
        const int32 sizeX = settings.value("mainWindowSizeX", 1920).toInt();
        const int32 sizeY = settings.value("mainWindowSizeY", 1080).toInt();
        resize(sizeX, sizeY);

        // set the position
        const bool containsPosX = settings.contains("mainWindowPosX");
        const bool containsPosY = settings.contains("mainWindowPosY");
        if ((containsPosX) && (containsPosY))
        {
            const int32 posX = settings.value("mainWindowPosX", 0).toInt();
            const int32 posY = settings.value("mainWindowPosY", 0).toInt();
            move(posX, posY);
        }
        else
        {
            QDesktopWidget desktopWidget;
            const QRect primaryScreenRect = desktopWidget.availableGeometry(desktopWidget.primaryScreen());
            const int32 posX = (primaryScreenRect.width() / 2) - (sizeX / 2);
            const int32 posY = (primaryScreenRect.height() / 2) - (sizeY / 2);
            move(posX, posY);
        }

#if !defined(EMFX_EMSTUDIOLYEMBEDDED)

        // maximized state
        const bool isMaximized = settings.value("mainWindowMaximized", true).toBool();
        if (isMaximized)
        {
            showMaximized();
        }
        else
        {
            showNormal();
        }

#endif // EMFX_EMSTUDIOLYEMBEDDED

        // enable drag&drop support
        setAcceptDrops(true);

        setDockNestingEnabled(true);

        setFocusPolicy(Qt::StrongFocus);

        // create the menu bar
        QWidget* menuWidget = new QWidget();
        QHBoxLayout* menuLayout = new QHBoxLayout(menuWidget);

        QMenuBar* menuBar = new QMenuBar(menuWidget);
        menuBar->setStyleSheet("QMenuBar { min-height: 10px;}"); // menu fix (to get it working with the Ly style)
        menuLayout->setMargin(0);
        menuLayout->setSpacing(0);
        menuLayout->addWidget(menuBar);

        QLabel* modeLabel = new QLabel("Layout: ");
        mApplicationMode = new MysticQt::ComboBox();
        menuLayout->addWidget(mApplicationMode);

        setMenuWidget(menuWidget);

        // read the maximum number of recent files
        const int32 maxRecentFiles = settings.value("maxRecentFiles", 16).toInt(); // default to 16 recent files in case we start EMStudio the first time

        // file actions
        QMenu* menu = menuBar->addMenu(tr("&File"));

        // reset action
        mResetAction = menu->addAction(tr("&Reset"), this, SLOT(OnReset()), QKeySequence::New);
        mResetAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Menu/Refresh.png"));

        // save all
        mSaveAllAction = menu->addAction(tr("Save All..."), this, SLOT(OnSaveAll()), QKeySequence::Save);
        mSaveAllAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Menu/FileSave.png"));

        // disable the reset and save all menus until one thing is loaded
        mResetAction->setDisabled(true);
        mSaveAllAction->setDisabled(true);

        menu->addSeparator();

        // actor file actions
        QAction* openAction = menu->addAction(tr("&Open Actor"), this, SLOT(OnFileOpenActor()), QKeySequence::Open);
        openAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Open.png"));
        mMergeActorAction = menu->addAction(tr("&Merge Actor"), this, SLOT(OnFileMergeActor()), Qt::CTRL + Qt::Key_I);
        mMergeActorAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Open.png"));
        mSaveSelectedActorsAction = menu->addAction(tr("&Save Selected Actors"), this, SLOT(OnFileSaveSelectedActors()));
        mSaveSelectedActorsAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Menu/FileSave.png"));

        // disable the merge actor menu until one actor is in the scene
        DisableMergeActorMenu();

        // disable the save selected actor menu until one actor is selected
        DisableSaveSelectedActorsMenu();

        // recent actors submenu
        mRecentActors.Init(menu, maxRecentFiles, "Recent Actors", "recentActorFiles");
        connect(&mRecentActors, SIGNAL(OnRecentFile(QAction*)), this, SLOT(OnRecentFile(QAction*)));

        // workspace file actions
        menu->addSeparator();
        QAction* newWorkspaceAction = menu->addAction(tr("New Workspace"), this, SLOT(OnFileNewWorkspace()));
        newWorkspaceAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
        QAction* openWorkspaceAction = menu->addAction(tr("Open Workspace"), this, SLOT(OnFileOpenWorkspace()));
        openWorkspaceAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Open.png"));
        QAction* saveWorkspaceAction = menu->addAction(tr("Save Workspace"), this, SLOT(OnFileSaveWorkspace()));
        saveWorkspaceAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Menu/FileSave.png"));
        QAction* saveWorkspaceAsAction = menu->addAction(tr("Save Workspace As"), this, SLOT(OnFileSaveWorkspaceAs()));
        saveWorkspaceAsAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Menu/FileSaveAs.png"));

        // recent workspace submenu
        mRecentWorkspaces.Init(menu, maxRecentFiles, "Recent Workspaces", "recentWorkspaces");
        connect(&mRecentWorkspaces, SIGNAL(OnRecentFile(QAction*)), this, SLOT(OnRecentFile(QAction*)));

        // edit menu
        menu = menuBar->addMenu(tr("&Edit"));
        QAction* undoAction = mUndoAction = menu->addAction(tr("Undo"), this, SLOT(OnUndo()), QKeySequence::Undo);
        undoAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Menu/Undo.png"));
        QAction* redoAction = mRedoAction = menu->addAction(tr("Redo"), this, SLOT(OnRedo()), QKeySequence::Redo);
        redoAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Menu/Redo.png"));
        mUndoAction->setDisabled(true);
        mRedoAction->setDisabled(true);
        menu->addSeparator();
        QAction* preferencesAction = menu->addAction(tr("&Preferences"), this, SLOT(OnPreferences()));
        preferencesAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Menu/Preferences.png"));

        // selection menu
        menu = menuBar->addMenu(tr("&Select"));
        menu->addAction(tr("Select All Actor Instances"), this, SLOT(OnSelectAllActorInstances()));
        menu->addAction(tr("Unselect All Actor Instances"), this, SLOT(OnUnselectAllActorInstances()));
        menu->addAction(tr("Adjust Selection"), this, SLOT(OnAdjustNodeSelection()));

        // layouts item
        mLayoutsMenu = menuBar->addMenu(tr("&Layouts"));
        UpdateLayoutsMenu();

        // reset the application mode selection and connect it
        mApplicationMode->setCurrentIndex(-1);
        connect(mApplicationMode, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(ApplicationModeChanged(const QString&)));
        mLayoutLoaded = false;

        // window item
        menu = menuBar->addMenu(tr("&Window"));
        mCreateWindowMenu = menu;

        // help menu
        menu = menuBar->addMenu(tr("&Help"));

        QMenu* folders = menu->addMenu("Folders");
        folders->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Open.png"));
        folders->addAction("Open autosave folder", this, SLOT(OnOpenAutosaveFolder()));
        folders->addAction("Open settings folder", this, SLOT(OnOpenSettingsFolder()));

        // create the node selection window
        mNodeSelectionWindow = new NodeSelectionWindow(this, false);
        connect(mNodeSelectionWindow->GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)), this, SLOT(OnNodeSelected(MCore::Array<SelectionItem>)));

        // set the window title without filename, as new workspace
        SetWindowTitleFromFileName("<not saved yet>");

        // create the autosave timer
        mAutosaveTimer = new QTimer(this);
        connect(mAutosaveTimer, SIGNAL(timeout()), this, SLOT(OnAutosaveTimeOut()));

        // load preferences
        LoadPreferences();

        // init dirty file manager
        mDirtyFileManager = new DirtyFileManager;

        // init the file manager
        mFileManager = new EMStudio::FileManager(this);

        ////////////////////////////////////////////////////////////////////////
        // Keyboard Shortcut Manager
        ////////////////////////////////////////////////////////////////////////

        // create the shortcut manager
        mShortcutManager = new MysticQt::KeyboardShortcutManager();

        // load the old shortcuts
        QSettings shortcutSettings(MCore::String(GetManager()->GetAppDataFolder() + "EMStudioKeyboardShortcuts.cfg").AsChar(), QSettings::IniFormat, this);
        mShortcutManager->Load(&shortcutSettings);

        // add the application mode group
        const char* layoutGroupName = "Layouts";
        mShortcutManager->RegisterKeyboardShortcut("AnimGraph", layoutGroupName, Qt::Key_1, false, true, false);
        mShortcutManager->RegisterKeyboardShortcut("Animation", layoutGroupName, Qt::Key_2, false, true, false);
        mShortcutManager->RegisterKeyboardShortcut("Character", layoutGroupName, Qt::Key_3, false, true, false);

        // create and register the command callbacks
        mImportActorCallback = new CommandImportActorCallback(false);
        mRemoveActorCallback = new CommandRemoveActorCallback(false);
        mRemoveActorInstanceCallback = new CommandRemoveActorInstanceCallback(false);
        mImportMotionCallback = new CommandImportMotionCallback(false);
        mRemoveMotionCallback = new CommandRemoveMotionCallback(false);
        mCreateMotionSetCallback = new CommandCreateMotionSetCallback(false);
        mRemoveMotionSetCallback = new CommandRemoveMotionSetCallback(false);
        mLoadMotionSetCallback = new CommandLoadMotionSetCallback(false);
        mCreateAnimGraphCallback = new CommandCreateAnimGraphCallback(false);
        mRemoveAnimGraphCallback = new CommandRemoveAnimGraphCallback(false);
        mLoadAnimGraphCallback = new CommandLoadAnimGraphCallback(false);
        mSelectCallback = new CommandSelectCallback(false);
        mUnselectCallback = new CommandUnselectCallback(false);
        mSaveWorkspaceCallback = new CommandSaveWorkspaceCallback(false);
        GetCommandManager()->RegisterCommandCallback("ImportActor", mImportActorCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActor", mRemoveActorCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActorInstance", mRemoveActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("ImportMotion", mImportMotionCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveMotion", mRemoveMotionCallback);
        GetCommandManager()->RegisterCommandCallback("CreateMotionSet", mCreateMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveMotionSet", mRemoveMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("LoadMotionSet", mLoadMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("CreateAnimGraph", mCreateAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveAnimGraph", mRemoveAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("LoadAnimGraph", mLoadAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("SaveWorkspace", mSaveWorkspaceCallback);

        QAbstractEventDispatcher::instance()->installNativeEventFilter(new NativeEventFilter(this));

        settings.endGroup();
    }


    bool NativeEventFilter::nativeEventFilter(const QByteArray& /*eventType*/, void* message, long* /*result*/)
    {
        #ifdef MCORE_PLATFORM_WINDOWS
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_DEVICECHANGE)
        {
            if (msg->wParam == DBT_DEVICEREMOVECOMPLETE || msg->wParam == DBT_DEVICEARRIVAL || msg->wParam == DBT_DEVNODES_CHANGED)
            {
                // The reason why there are multiple of such messages is because it emits messages for all related hardware nodes.
                // But we do not know the name of the hardware to look for here either, so we can't filter that.
                AZ_TracePrintf("EMotionFX", "Hardware changes detected\n");
                emit m_MainWindow->HardwareChangeDetected();
            }
        }
        #endif

        return false;
    }


    bool MainWindow::CommandImportActorCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // enable the merge menu only if one actor is in the scene
        if (EMotionFX::GetActorManager().GetNumActors() > 0)
        {
            GetManager()->GetMainWindow()->EnableMergeActorMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableMergeActorMenu();
        }

        // update the reset and save all menus
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();

        // succeeded
        return true;
    }


    bool MainWindow::CommandImportActorCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // enable the merge menu only if one actor is in the scene
        if (EMotionFX::GetActorManager().GetNumActors() > 0)
        {
            GetManager()->GetMainWindow()->EnableMergeActorMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableMergeActorMenu();
        }

        // update the reset and save all menus
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();

        // succeeded
        return true;
    }


    bool MainWindow::CommandRemoveActorCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // enable the merge menu only if one actor is in the scene
        if (EMotionFX::GetActorManager().GetNumActors() > 0)
        {
            GetManager()->GetMainWindow()->EnableMergeActorMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableMergeActorMenu();
        }

        // enable the actor save selected menu only if one actor or actor instance is selected
        // it's needed to check here because if one actor is removed it's not selected anymore
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActors = selectionList.GetNumSelectedActors();
        const uint32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();
        if ((numSelectedActors > 0) || (numSelectedActorInstances > 0))
        {
            GetManager()->GetMainWindow()->EnableSaveSelectedActorsMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableSaveSelectedActorsMenu();
        }

        // update the reset and save all menus
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();

        // succeeded
        return true;
    }


    bool MainWindow::CommandRemoveActorCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // enable the merge menu only if one actor is in the scene
        if (EMotionFX::GetActorManager().GetNumActors() > 0)
        {
            GetManager()->GetMainWindow()->EnableMergeActorMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableMergeActorMenu();
        }

        // enable the actor save menu only if one actor or actor instance is selected
        // it's needed to check here because if one actor is removed it's not selected anymore
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActors = selectionList.GetNumSelectedActors();
        const uint32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();
        if ((numSelectedActors > 0) || (numSelectedActorInstances > 0))
        {
            GetManager()->GetMainWindow()->EnableSaveSelectedActorsMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableSaveSelectedActorsMenu();
        }

        // update the reset and save all menus
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();

        // succeeded
        return true;
    }


    bool MainWindow::CommandRemoveActorInstanceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // enable the actor save menu only if one actor or actor instance is selected
        // it's needed to check here because if one actor is removed it's not selected anymore
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActors = selectionList.GetNumSelectedActors();
        const uint32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();
        if ((numSelectedActors > 0) || (numSelectedActorInstances > 0))
        {
            GetManager()->GetMainWindow()->EnableSaveSelectedActorsMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableSaveSelectedActorsMenu();
        }

        // succeeded
        return true;
    }


    bool MainWindow::CommandRemoveActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // enable the actor save menu only if one actor or actor instance is selected
        // it's needed to check here because if one actor is removed it's not selected anymore
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActors = selectionList.GetNumSelectedActors();
        const uint32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();
        if ((numSelectedActors > 0) || (numSelectedActorInstances > 0))
        {
            GetManager()->GetMainWindow()->EnableSaveSelectedActorsMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableSaveSelectedActorsMenu();
        }

        // succeeded
        return true;
    }


    bool MainWindow::CommandImportMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandImportMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandCreateMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandCreateMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandLoadMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandLoadMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandCreateAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandCreateAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandLoadAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandLoadAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // enable the actor save menu only if one actor or actor instance is selected
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActors = selectionList.GetNumSelectedActors();
        const uint32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();
        if ((numSelectedActors > 0) || (numSelectedActorInstances > 0))
        {
            GetManager()->GetMainWindow()->EnableSaveSelectedActorsMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableSaveSelectedActorsMenu();
        }

        // succeeded
        return true;
    }


    bool MainWindow::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // enable the actor save menu only if one actor or actor instance is selected
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActors = selectionList.GetNumSelectedActors();
        const uint32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();
        if ((numSelectedActors > 0) || (numSelectedActorInstances > 0))
        {
            GetManager()->GetMainWindow()->EnableSaveSelectedActorsMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableSaveSelectedActorsMenu();
        }

        // succeeded
        return true;
    }


    bool MainWindow::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // enable the actor save menu only if one actor or actor instance is selected
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActors = selectionList.GetNumSelectedActors();
        const uint32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();
        if ((numSelectedActors > 0) || (numSelectedActorInstances > 0))
        {
            GetManager()->GetMainWindow()->EnableSaveSelectedActorsMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableSaveSelectedActorsMenu();
        }

        // succeeded
        return true;
    }


    bool MainWindow::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // enable the actor save menu only if one actor or actor instance is selected
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActors = selectionList.GetNumSelectedActors();
        const uint32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();
        if ((numSelectedActors > 0) || (numSelectedActorInstances > 0))
        {
            GetManager()->GetMainWindow()->EnableSaveSelectedActorsMenu();
        }
        else
        {
            GetManager()->GetMainWindow()->DisableSaveSelectedActorsMenu();
        }

        // succeeded
        return true;
    }


    bool MainWindow::CommandSaveWorkspaceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCore::String filename;
        commandLine.GetValue("filename", command, &filename);
        GetManager()->GetMainWindow()->OnWorkspaceSaved(filename.AsChar());
        return true;
    }


    bool MainWindow::CommandSaveWorkspaceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return true;
    }


    void MainWindow::OnWorkspaceSaved(const char* filename)
    {
        mRecentWorkspaces.AddRecentFile(filename);
        SetWindowTitleFromFileName(filename);
    }


    void MainWindow::RegisterDirtyWorkspaceCallback()
    {
        SaveDirtyWorkspaceCallback* dirtyWorkspaceCallback = new SaveDirtyWorkspaceCallback;
        mDirtyFileManager->AddCallback(dirtyWorkspaceCallback);
    }


    void MainWindow::UpdateResetAndSaveAllMenus()
    {
        // enable the menus if at least one actor
        if (EMotionFX::GetActorManager().GetNumActors() > 0)
        {
            mResetAction->setEnabled(true);
            mSaveAllAction->setEnabled(true);
            return;
        }

        // enable the menus if at least one motion
        if (EMotionFX::GetMotionManager().GetNumMotions() > 0)
        {
            mResetAction->setEnabled(true);
            mSaveAllAction->setEnabled(true);
            return;
        }

        // enable the menus if at least one motion set
        if (EMotionFX::GetMotionManager().GetNumMotionSets() > 0)
        {
            mResetAction->setEnabled(true);
            mSaveAllAction->setEnabled(true);
            return;
        }

        // enable the menus if at least one anim graph
        if (EMotionFX::GetAnimGraphManager().GetNumAnimGraphs() > 0)
        {
            mResetAction->setEnabled(true);
            mSaveAllAction->setEnabled(true);
            return;
        }

        // nothing loaded, disable the menus
        mResetAction->setDisabled(true);
        mSaveAllAction->setDisabled(true);
    }


    void MainWindow::EnableMergeActorMenu()
    {
        mMergeActorAction->setEnabled(true);
    }


    void MainWindow::DisableMergeActorMenu()
    {
        mMergeActorAction->setDisabled(true);
    }


    void MainWindow::EnableSaveSelectedActorsMenu()
    {
        mSaveSelectedActorsAction->setEnabled(true);
    }


    void MainWindow::DisableSaveSelectedActorsMenu()
    {
        mSaveSelectedActorsAction->setDisabled(true);
    }


    void MainWindow::SetWindowTitleFromFileName(const MCore::String& fileName)
    {
        // get only the version number of EMotion FX
        MCore::String emfxVersionString = EMotionFX::GetEMotionFX().GetVersionString();
        emfxVersionString.RemoveAllParts("EMotion FX ");

        // set the window title
        // only set the EMotion FX version if the filename is empty
        MCore::String windowTitle;
        windowTitle.Format("EMotion Studio %s (BUILD %s)", emfxVersionString.AsChar(), MCORE_DATE);
        if (fileName.GetIsEmpty() == false)
        {
            windowTitle.FormatAdd(" - %s", fileName.AsChar());
        }
        setWindowTitle(windowTitle.AsChar());
    }


    void MainWindow::SetMaxRecentFiles(uint32 numRecentFiles, bool saveToConfigFile)
    {
        // update the max recent files
        mMaxNumRecentFiles = numRecentFiles;
        mRecentActors.SetMaxRecentFiles(numRecentFiles);
        mRecentWorkspaces.SetMaxRecentFiles(numRecentFiles);

        // save the preferences
        if (saveToConfigFile)
        {
            SavePreferences();
        }
    }


    // update the items inside the Window->Create menu item
    void MainWindow::UpdateCreateWindowMenu()
    {
        // get the plugin manager
        PluginManager* pluginManager = GetPluginManager();

        // get the number of plugins
        const uint32 numPlugins = pluginManager->GetNumPlugins();

        // add each plugin name in an array to sort them
        MCore::Array<MCore::String> sortedPlugins;
        sortedPlugins.Reserve(numPlugins);
        for (uint32 p = 0; p < numPlugins; ++p)
        {
            EMStudioPlugin* plugin = pluginManager->GetPlugin(p);
            sortedPlugins.Add(plugin->GetName());
        }
        sortedPlugins.Sort();

        // clear the window menu
        mCreateWindowMenu->clear();

        // for all registered plugins, create a menu items
        for (uint32 p = 0; p < numPlugins; ++p)
        {
            // get the plugin
            const uint32 pluginIndex = pluginManager->FindPluginByTypeString(sortedPlugins[p]);
            EMStudioPlugin* plugin = pluginManager->GetPlugin(pluginIndex);

            // don't add invisible plugins to the list
            if (plugin->GetPluginType() == EMStudioPlugin::PLUGINTYPE_INVISIBLE)
            {
                continue;
            }

            // check if multiple instances allowed
            // on this case the plugin is not one action but one submenu
            if (plugin->AllowMultipleInstances())
            {
                // create the menu
                mCreateWindowMenu->addMenu(plugin->GetName());

                // TODO: add each instance inside the submenu
            }
            else
            {
                // create the action
                QAction* action = mCreateWindowMenu->addAction(plugin->GetName());
                action->setData(plugin->GetName());

                // connect the action to activate the plugin when clicked on it
                connect(action, SIGNAL(triggered(bool)), this, SLOT(OnWindowCreate(bool)));

                // set the action checkable
                action->setCheckable(true);

                // set the checked state of the action
                const bool checked = pluginManager->FindActivePlugin(plugin->GetClassID());
                action->setChecked(checked);
            }
        }
    }


    // create a new given window
    void MainWindow::OnWindowCreate(bool checked)
    {
        // get the plugin name
        QAction* action = (QAction*)sender();
        const QString pluginName = action->data().toString();

        // checked is the new state
        // activate the plugin if the menu is not checked
        // show and focus on the actual window if the menu is already checked
        if (checked)
        {
            // try to create the new window
            EMStudioPlugin* newPlugin = EMStudio::GetPluginManager()->CreateWindowOfType(FromQtString(pluginName).AsChar());
            if (newPlugin == nullptr)
            {
                MCore::LogError("Failed to create window using plugin '%s'", FromQtString(pluginName).AsChar());
                return;
            }

            // if we have a dock widget plugin here, making floatable and change its window size
            if (newPlugin->GetPluginType() == EMStudioPlugin::PLUGINTYPE_DOCKWIDGET)
            {
                DockWidgetPlugin* dockPlugin = static_cast<DockWidgetPlugin*>(newPlugin);
                dockPlugin->GetDockWidget()->setFloating(true);
                const QSize s = dockPlugin->GetInitialWindowSize();
                dockPlugin->GetDockWidget()->resize(s.width(), s.height());
            }
        }
        else // (checked == false)
        {
            EMStudioPlugin* plugin = EMStudio::GetPluginManager()->GetActivePluginByTypeString(FromQtString(pluginName).AsChar());
            AZ_Assert(plugin, "Failed to get plugin, since it was checked it should be active");
            EMStudio::GetPluginManager()->RemoveActivePlugin(plugin);
        }

        // update the window menu
        UpdateCreateWindowMenu();
    }

    // open the autosave folder
    void MainWindow::OnOpenAutosaveFolder()
    {
        const QUrl url(("file:///" + GetManager()->GetAutosavesFolder()).AsChar());
        QDesktopServices::openUrl(url);
    }


    // open the settings folder
    void MainWindow::OnOpenSettingsFolder()
    {
        const QUrl url(("file:///" + GetManager()->GetAppDataFolder()).AsChar());
        QDesktopServices::openUrl(url);
    }


    // show the preferences dialog
    void MainWindow::OnPreferences()
    {
        if (mPreferencesWindow == nullptr)
        {
            mPreferencesWindow = new PreferencesWindow(this);
            mPreferencesWindow->Init();

            const char* categoryName = "General";
            MysticQt::PropertyWidget* generalPropertyWidget = mPreferencesWindow->FindPropertyWidgetByName(categoryName);
            if (generalPropertyWidget == nullptr)
            {
                generalPropertyWidget = mPreferencesWindow->AddCategory(categoryName, "Images/Preferences/General.png", false);
            }

            connect(generalPropertyWidget, SIGNAL(ValueChanged(MysticQt::PropertyWidget::Property*)), this, SLOT(OnValueChanged(MysticQt::PropertyWidget::Property*)));

            mMaxRecentFilesProperty             = generalPropertyWidget->AddIntProperty("", "Maximum Recent Files", GetMaxRecentFiles(), 16, 1, 99);
            mMaxHistoryItemsProperty            = generalPropertyWidget->AddIntProperty("", "Undo History Size", GetCommandManager()->GetMaxHistoryItems(), 256, 1, 9999);
            mNotificationVisibleTimeProperty    = generalPropertyWidget->AddIntProperty("", "Notification Visible Time", mNotificationVisibleTime, 5, 1, 10);
            mAutosaveIntervalProperty           = generalPropertyWidget->AddIntProperty("", "Autosave Interval (Minutes)", mAutosaveInterval, 10, 1, 60);
            mAutosaveNumberOfFilesProperty      = generalPropertyWidget->AddIntProperty("", "Autosave Number Of Files", mAutosaveNumberOfFiles, 5, 1, 99);
            mEnableAutosaveProperty             = generalPropertyWidget->AddBoolProperty("", "Enable Autosave", mEnableAutosave, true);
            mImporterDetailedLogging            = generalPropertyWidget->AddBoolProperty("", "Importer Detailed Logging", EMotionFX::GetImporter().GetLogDetails(), false);
            mAutoLoadLastWorkspaceProperty      = generalPropertyWidget->AddBoolProperty("", "Auto Load Last Workspace", GetManager()->GetAutoLoadLastWorkspace(), false);

            const uint32 numGeneralPropertyWidgetColumns = generalPropertyWidget->columnCount();
            for (uint32 i = 0; i < numGeneralPropertyWidgetColumns; ++i)
            {
                generalPropertyWidget->resizeColumnToContents(i);
            }

            // keyboard shortcuts
            categoryName = "Keyboard\nShortcuts";
            KeyboardShortcutsWindow* shortcutsWindow = new KeyboardShortcutsWindow(mPreferencesWindow);
            mPreferencesWindow->AddCategory(shortcutsWindow, categoryName, "Images/Preferences/KeyboardShortcuts.png", false);

            // unit setup
            categoryName = "Unit Setup";
            UnitSetupWindow* unitSetupWindow  = new UnitSetupWindow(mPreferencesWindow);
            mPreferencesWindow->AddCategory(unitSetupWindow, categoryName, "Images/Preferences/UnitSetup.png", false);

            // add all categories from the plugins
            mPreferencesWindow->AddCategoriesFromPlugin(nullptr);
        }

        mPreferencesWindow->show();
    }


    void MainWindow::OnValueChanged(MysticQt::PropertyWidget::Property* property)
    {
        // set the maximum number of recent files
        if (property == mMaxRecentFilesProperty)
        {
            SetMaxRecentFiles(property->AsInt(), false);
        }

        // set the maximum number of history items in the command manager
        if (property == mMaxHistoryItemsProperty)
        {
            GetCommandManager()->SetMaxHistoryItems(property->AsInt());
        }

        // set the notification visible time
        if (property == mNotificationVisibleTimeProperty)
        {
            mNotificationVisibleTime = property->AsInt();
            GetNotificationWindowManager()->SetVisibleTime(mNotificationVisibleTime);
        }

        // enable or disable the autosave timer
        if (property == mEnableAutosaveProperty)
        {
            mEnableAutosave = property->AsBool();
            if (mEnableAutosave)
            {
                mAutosaveTimer->start();
            }
            else
            {
                mAutosaveTimer->stop();
            }
        }

        // set the autosave interval
        if (property == mAutosaveIntervalProperty)
        {
            mAutosaveTimer->stop();
            mAutosaveInterval = property->AsInt();
            mAutosaveTimer->setInterval(mAutosaveInterval * 60 * 1000);
            mAutosaveTimer->start();
        }

        // set the autosave number of files
        if (property == mAutosaveNumberOfFilesProperty)
        {
            mAutosaveNumberOfFiles = property->AsInt();
        }

        // set if the detail logging of the importer is enabled or not
        if (property == mImporterDetailedLogging)
        {
            EMotionFX::GetImporter().SetLogDetails(property->AsBool());
        }

        // set if auto loading the last workspace is enabled or not
        if (property == mAutoLoadLastWorkspaceProperty)
        {
            GetManager()->SetAutoLoadLastWorkspace(property->AsBool());
        }

        // save preferences
        SavePreferences();
    }


    // save the preferences
    void MainWindow::SavePreferences()
    {
        // open the config file
        QSettings settings(this);
        settings.beginGroup("EMotionFX");

        // save the unit type
        settings.setValue("unitType", MCore::Distance::UnitTypeToString(EMotionFX::GetEMotionFX().GetUnitType()));

        // save the maximum number of items in the command history
        settings.setValue("maxHistoryItems", GetCommandManager()->GetMaxHistoryItems());

        // save the notification visible time
        settings.setValue("notificationVisibleTime", mNotificationVisibleTime);

        // save the autosave settings
        settings.setValue("enableAutosave", mEnableAutosave);
        settings.setValue("autosaveInterval", mAutosaveInterval);
        settings.setValue("autosaveNumberOfFiles", mAutosaveNumberOfFiles);

        // save the new maximum number of recent files
        settings.setValue("maxRecentFiles", mMaxNumRecentFiles);

        // save the log details flag for the importer
        settings.setValue("importerLogDetailsEnabled", EMotionFX::GetImporter().GetLogDetails());

        // save the last used application mode string
        settings.setValue("applicationMode", mLastUsedMode);

        // save the auto load last workspace flag
        settings.setValue("autoLoadLastWorkspace", GetManager()->GetAutoLoadLastWorkspace());

        // main window position
        settings.setValue("mainWindowPosX", pos().x());
        settings.setValue("mainWindowPosY", pos().y());

        // main window size
        settings.setValue("mainWindowSizeX", size().width());
        settings.setValue("mainWindowSizeY", size().height());

        // maximized state
        const bool isMaximized = windowState() & Qt::WindowMaximized;
        settings.setValue("mainWindowMaximized", isMaximized);

        settings.endGroup();
    }


    // load the preferences
    void MainWindow::LoadPreferences()
    {
        // open the config file
        QSettings settings(this);
        settings.beginGroup("EMotionFX");

        // read the unit type
        QString unitTypeString = settings.value("unitType", "meters").toString();
        MCore::Distance::EUnitType unitType;
        MCore::Distance::StringToUnitType(unitTypeString.toUtf8().data(), &unitType);
        EMotionFX::GetEMotionFX().SetUnitType(unitType);

        // read the maximum number of items in the command history
        const int32 maxHistoryItems = settings.value("maxHistoryItems", GetCommandManager()->GetMaxHistoryItems()).toInt();
        GetCommandManager()->SetMaxHistoryItems(maxHistoryItems);

        // read the notification visible time
        mNotificationVisibleTime = settings.value("notificationVisibleTime", 5).toInt();
        GetNotificationWindowManager()->SetVisibleTime(mNotificationVisibleTime);

        // read the autosave settings
        mEnableAutosave = settings.value("enableAutosave", true).toBool();
        mAutosaveInterval = settings.value("autosaveInterval", 10).toInt();
        mAutosaveNumberOfFiles = settings.value("autosaveNumberOfFiles", 5).toInt();

        // set the autosave timer
        mAutosaveTimer->setInterval(mAutosaveInterval * 60 * 1000);
        if (mEnableAutosave)
        {
            mAutosaveTimer->start();
        }
        else
        {
            mAutosaveTimer->stop();
        }

        // read the maximum number of recent files
        const int32 maxRecentFiles = settings.value("maxRecentFiles", 16).toInt();
        SetMaxRecentFiles(maxRecentFiles, false);

        // save the new maximum number of recent files
        settings.setValue("maxRecentFiles", mMaxNumRecentFiles);

        // read the log details flag for the importer
        const bool importerLogDetails = settings.value("importerLogDetailsEnabled", EMotionFX::GetImporter().GetLogDetails()).toBool();
        EMotionFX::GetImporter().SetLogDetails(importerLogDetails);

        // read the last used application mode string
        mLastUsedMode = settings.value("applicationMode", "AnimGraph").toString();

        settings.endGroup();

        // load the auto load last workspace flag
        const bool autoLoadLastWorkspace = settings.value("autoLoadLastWorkspace", GetManager()->GetAutoLoadLastWorkspace()).toBool();
        GetManager()->SetAutoLoadLastWorkspace(autoLoadLastWorkspace);
        settings.endGroup();
    }


    void MainWindow::LoadActor(const char* fileName, bool replaceCurrentScene)
    {
        // create the final command
        MCore::String commandResult;

        // set the command group name based on the parameters
        const MCore::String commandGroupName = (replaceCurrentScene) ? "Open actor" : "Merge actor";

        // create the command group
        MCore::String outResult;
        MCore::CommandGroup commandGroup(commandGroupName.AsChar());

        // clear the scene if not merging
        // clear the actors and actor instances selection if merging
        if (replaceCurrentScene)
        {
            CommandSystem::ClearScene(true, true, &commandGroup);
        }
        else
        {
            commandGroup.AddCommandString("Unselect -actorInstanceID SELECT_ALL -actorID SELECT_ALL");
        }

        // create the load command
        MCore::String loadActorCommand;

        // add the import command
        loadActorCommand.Format("ImportActor -filename \"%s\" ", fileName);

        // add the load actor settings
        LoadActorSettingsWindow::LoadActorSettings loadActorSettings;
        loadActorCommand.FormatAdd("-loadMeshes %d ",          loadActorSettings.mLoadMeshes);
        loadActorCommand.FormatAdd("-loadTangents %d ",        loadActorSettings.mLoadTangents);
        loadActorCommand.FormatAdd("-autoGenTangents %d ",     loadActorSettings.mAutoGenerateTangents);
        loadActorCommand.FormatAdd("-loadLimits %d ",          loadActorSettings.mLoadLimits);
        loadActorCommand.FormatAdd("-loadGeomLods %d ",        loadActorSettings.mLoadGeometryLODs);
        loadActorCommand.FormatAdd("-loadMorphTargets %d ",    loadActorSettings.mLoadMorphTargets);
        loadActorCommand.FormatAdd("-loadCollisionMeshes %d ", loadActorSettings.mLoadCollisionMeshes);
        loadActorCommand.FormatAdd("-loadMaterialLayers %d ",  loadActorSettings.mLoadStandardMaterialLayers);
        loadActorCommand.FormatAdd("-loadSkinningInfo %d ",    loadActorSettings.mLoadSkinningInfo);
        loadActorCommand.FormatAdd("-loadSkeletalLODs %d ",    loadActorSettings.mLoadSkeletalLODs);
        loadActorCommand.FormatAdd("-dualQuatSkinning %d ",    loadActorSettings.mDualQuaternionSkinning);

        // add the load and the create instance commands
        commandGroup.AddCommandString(loadActorCommand.AsChar());
        commandGroup.AddCommandString("CreateActorInstance -actorID %LASTRESULT%");

        // if the current scene is replaced or merge on an empty scene, focus on the new actor instance
        if (replaceCurrentScene || EMotionFX::GetActorManager().GetNumActorInstances() == 0)
        {
            commandGroup.AddCommandString("ReInitRenderActors -resetViewCloseup true");
        }

        // execute the group command
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError("Could not load actor '%s'.", fileName);
        }

        // add the actor in the recent actor list
        // if the same actor is already in the list, the duplicate is removed
        mRecentActors.AddRecentFile(fileName);
    }



    void MainWindow::LoadCharacter(const AZ::Data::AssetId& actorAssetId, const AZ::Data::AssetId& animgraphId, const AZ::Data::AssetId& motionSetId)
    {
        mCharacterFiles.clear();
        AZStd::string cachePath = gEnv->pFileIO->GetAlias("@assets@");
        AZStd::string filename;
        AzFramework::StringFunc::AssetDatabasePath::Normalize(cachePath);

        AZStd::string actorFilename;
        EBUS_EVENT_RESULT(actorFilename, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, actorAssetId);
        AzFramework::StringFunc::AssetDatabasePath::Join(cachePath.c_str(), actorFilename.c_str(), filename, true);
        actorFilename = filename;

        AZStd::string animgraphFilename;
        EBUS_EVENT_RESULT(animgraphFilename, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, animgraphId);
        bool found;
        if (!animgraphFilename.empty())
        {
            EBUS_EVENT_RESULT(found, AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, animgraphFilename.c_str(), filename);
            if (found)
            {
                animgraphFilename = filename;
            }
        }

        AZStd::string motionSetFilename;
        EBUS_EVENT_RESULT(motionSetFilename, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, motionSetId);
        if (!motionSetFilename.empty())
        {
            EBUS_EVENT_RESULT(found, AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, motionSetFilename.c_str(), filename);
            if (found)
            {
                motionSetFilename = filename;
            }
        }

        // if the name is empty we stop looking for it
        bool foundActor = actorFilename.empty();
        bool foundAnimgraph = animgraphFilename.empty();
        bool foundMotionSet = motionSetFilename.empty();

        // Gather the list of dirty files
        AZStd::vector<AZStd::string> filenames;
        AZStd::vector<SaveDirtyFilesCallback::ObjectPointer> objects;
        AZStd::vector<SaveDirtyFilesCallback::ObjectPointer> dirtyObjects;

        const size_t numDirtyFilesCallbacks = mDirtyFileManager->GetNumCallbacks();
        for (size_t i = 0; i < numDirtyFilesCallbacks; ++i)
        {
            SaveDirtyFilesCallback* callback = mDirtyFileManager->GetCallback(i);
            callback->GetDirtyFileNames(&filenames, &objects);
            const size_t numFileNames = filenames.size();
            for (size_t j = 0; j < numFileNames; ++j)
            {
                // bypass if the filename is empty
                // it's the case when the file is not already saved
                if (filenames[j].empty())
                {
                    continue;
                }

                if (!foundActor && filenames[j] == actorFilename)
                {
                    foundActor = true;
                }
                else if (!foundAnimgraph && filenames[j] == animgraphFilename)
                {
                    foundAnimgraph = true;
                }
                else if (!foundMotionSet && filenames[j] == motionSetFilename)
                {
                    foundMotionSet = true;
                }
            }
        }

        // Dont reload dirty files that are already open.
        if (!foundActor)
        {
            mCharacterFiles.push_back(actorFilename);
        }
        if (!foundAnimgraph)
        {
            mCharacterFiles.push_back(animgraphFilename);
        }
        if (!foundMotionSet)
        {
            mCharacterFiles.push_back(motionSetFilename);
        }

        if (isVisible() && mLayoutLoaded)
        {
            LoadCharacterFiles();
        }
    }

    void MainWindow::OnFileNewWorkspace()
    {
        // save all files that have been changed
        if (mDirtyFileManager->SaveDirtyFiles() == DirtyFileManager::CANCELED)
        {
            return;
        }

        // Are you sure?
        if (QMessageBox::warning(this, "New Workspace", "Are you sure you want to create a new workspace?\n\nThis will reset the entire scene.\n\nClick Yes to reset the scene and create a new workspace, No in case you want to cancel the process.", QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        {
            return;
        }

        // create th command group
        MCore::CommandGroup commandGroup("New workspace", 32);

        // clear everything
        Reset(true, true, true, true, &commandGroup);

        // execute the group command
        AZStd::string result;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            // clear the history
            GetCommandManager()->ClearHistory();

            // set the window title to not saved yet
            SetWindowTitleFromFileName("<not saved yet>");
        }
        else
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        Workspace* workspace = GetManager()->GetWorkspace();
        workspace->SetFilename("");
        workspace->SetDirtyFlag(false);
    }


    void MainWindow::OnFileOpenWorkspace()
    {
        const AZStd::string filename = mFileManager->LoadWorkspaceFileDialog(this);
        if (filename.empty())
        {
            return;
        }

        LoadFile(filename.c_str());
    }


    void MainWindow::OnSaveAll()
    {
        mDirtyFileManager->SaveDirtyFiles(MCORE_INVALIDINDEX32, MCORE_INVALIDINDEX32, QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    }


    void MainWindow::OnFileSaveWorkspace()
    {
        // save all files that have been changed, filter to not show the workspace files
        if (mDirtyFileManager->SaveDirtyFiles(MCORE_INVALIDINDEX32, SaveDirtyWorkspaceCallback::TYPE_ID) == DirtyFileManager::CANCELED)
        {
            return;
        }

        Workspace* workspace = GetManager()->GetWorkspace();

        // save using the current filename or show the dialog
        if (workspace->GetFilenameString().empty())
        {
            // open up save as dialog so that we can choose a filename
            const AZStd::string filename = GetMainWindow()->GetFileManager()->SaveWorkspaceFileDialog(GetMainWindow());
            if (filename.empty())
            {
                return;
            }

            // save the workspace using the newly selected filename
            const AZStd::string command = AZStd::string::format("SaveWorkspace -filename \"%s\"", filename.c_str());

            AZStd::string result;
            if (EMStudio::GetCommandManager()->ExecuteCommand(command, result))
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "Workspace <font color=green>successfully</font> saved");
            }
            else
            {
                AZ_Error("EMotionFX", false, result.c_str());
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "Workspace <font color=red>failed</font> to save");
            }
        }
        else
        {
            const AZStd::string command = AZStd::string::format("SaveWorkspace -filename \"%s\"", workspace->GetFilename());

            AZStd::string result;
            if (EMStudio::GetCommandManager()->ExecuteCommand(command, result))
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "Workspace <font color=green>successfully</font> saved");
            }
            else
            {
                AZ_Error("EMotionFX", false, result.c_str());
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "Workspace <font color=red>failed</font> to save");
            }
        }
    }


    void MainWindow::OnFileSaveWorkspaceAs()
    {
        // save all files that have been changed, filter to not show the workspace files
        if (mDirtyFileManager->SaveDirtyFiles(MCORE_INVALIDINDEX32, SaveDirtyWorkspaceCallback::TYPE_ID) == DirtyFileManager::CANCELED)
        {
            return;
        }

        // open up save as dialog so that we can choose a filename
        const AZStd::string filename = GetMainWindow()->GetFileManager()->SaveWorkspaceFileDialog(GetMainWindow());
        if (filename.empty())
        {
            return;
        }

        // save the workspace using the newly selected filename
        const AZStd::string command = AZStd::string::format("SaveWorkspace -filename \"%s\"", filename.c_str());

        AZStd::string result;
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "Workspace <font color=green>successfully</font> saved");
        }
        else
        {
            MCore::LogError(result.c_str());
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "Workspace <font color=red>failed</font> to save");
        }
    }


    void MainWindow::Reset(bool clearActors, bool clearMotionSets, bool clearMotions, bool clearAnimGraphs, MCore::CommandGroup* commandGroup)
    {
        // create and relink to a temporary new command group in case the input command group has not been specified
        MCore::CommandGroup newCommandGroup("Reset Scene");

        // add commands in the command group if one is valid
        if (commandGroup == nullptr)
        {
            if (clearActors)
            {
                CommandSystem::ClearScene(true, true, &newCommandGroup);
            }
            if (clearAnimGraphs)
            {
                CommandSystem::ClearAnimGraphsCommand(&newCommandGroup);
            }
            if (clearMotionSets)
            {
                CommandSystem::ClearMotionSetsCommand(&newCommandGroup);
            }
            if (clearMotions)
            {
                CommandSystem::ClearMotions(&newCommandGroup, true);
            }
        }
        else
        {
            if (clearActors)
            {
                CommandSystem::ClearScene(true, true, commandGroup);
            }
            if (clearAnimGraphs)
            {
                CommandSystem::ClearAnimGraphsCommand(commandGroup);
            }
            if (clearMotionSets)
            {
                CommandSystem::ClearMotionSetsCommand(commandGroup);
            }
            if (clearMotions)
            {
                CommandSystem::ClearMotions(commandGroup, true);
            }
        }

        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(newCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }

        Workspace* workspace = GetManager()->GetWorkspace();
        workspace->SetFilename("");
        workspace->SetDirtyFlag(false);
    }


    // constructor
    ResetSettingsWindow::ResetSettingsWindow(QWidget* parent)
        : QDialog(parent)
    {
        // update title of the dialog
        setWindowTitle("Reset Workspace");

        QVBoxLayout* vLayout = new QVBoxLayout(this);
        vLayout->setAlignment(Qt::AlignTop);

        setObjectName("StyledWidgetDark");

        QLabel* topLabel = new QLabel("<b>Select one or more items that you want to reset:</b>");
        topLabel->setStyleSheet("background-color: rgb(40, 40, 40); padding: 6px;");
        topLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        vLayout->addWidget(topLabel);
        vLayout->setMargin(0);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(5);
        layout->setSpacing(4);
        vLayout->addLayout(layout);

        mActorCheckbox = new QCheckBox("Actors");
        mActors = EMotionFX::GetActorManager().GetNumActors() > 0;
        if (mActors)
        {
            mActorCheckbox->setChecked(true);
        }
        else
        {
            mActorCheckbox->setChecked(false);
            mActorCheckbox->setDisabled(true);
        }

        mMotionCheckbox = new QCheckBox("Motions");
        mMotions = EMotionFX::GetMotionManager().GetNumMotions() > 0;
        if (mMotions)
        {
            mMotionCheckbox->setChecked(true);
        }
        else
        {
            mMotionCheckbox->setChecked(false);
            mMotionCheckbox->setDisabled(true);
        }

        mMotionSetCheckbox = new QCheckBox("Motion Sets");
        mMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets() > 0;
        if (mMotionSets)
        {
            mMotionSetCheckbox->setChecked(true);
        }
        else
        {
            mMotionSetCheckbox->setChecked(false);
            mMotionSetCheckbox->setDisabled(true);
        }

        mAnimGraphCheckbox = new QCheckBox("Anim Graphs");
        mAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs() > 0;
        if (mAnimGraphs)
        {
            mAnimGraphCheckbox->setChecked(true);
        }
        else
        {
            mAnimGraphCheckbox->setChecked(false);
            mAnimGraphCheckbox->setDisabled(true);
        }

        layout->addWidget(mActorCheckbox, Qt::AlignLeft);
        layout->addWidget(mMotionCheckbox, Qt::AlignLeft);
        layout->addWidget(mMotionSetCheckbox, Qt::AlignLeft);
        layout->addWidget(mAnimGraphCheckbox, Qt::AlignLeft);

        connect(mActorCheckbox,        SIGNAL(stateChanged(int)), this, SLOT(OnActorCheckbox(int)));
        connect(mMotionCheckbox,       SIGNAL(stateChanged(int)), this, SLOT(OnMotionCheckbox(int)));
        connect(mMotionSetCheckbox,    SIGNAL(stateChanged(int)), this, SLOT(OnMotionSetCheckbox(int)));
        connect(mAnimGraphCheckbox,   SIGNAL(stateChanged(int)), this, SLOT(OnAnimGraphCheckbox(int)));

        /*  layout->addWidget( new QLabel( "Actors" ), 0, 1 );
            layout->addWidget( new QLabel( "Motions" ), 1, 1 );
            layout->addWidget( new QLabel( "Motion Sets" ), 2, 1 );
            layout->addWidget( new QLabel( "Anim Graphs" ), 3, 1 );*/

        QHBoxLayout* hLayout = new QHBoxLayout();
        mOK = new QPushButton("OK");
        mCancel = new QPushButton("Cancel");
        hLayout->addWidget(mOK);
        hLayout->addWidget(mCancel);
        hLayout->setMargin(5);
        vLayout->addLayout(hLayout);

        connect(mOK, SIGNAL(clicked()), this, SLOT(OnOKButton()));
        connect(mCancel, SIGNAL(clicked()), this, SLOT(OnCancelButton()));

        setMinimumSize(325, 150);
        setMaximumSize(325, 150);
    }


    // reset
    void MainWindow::OnReset()
    {
        if (mDirtyFileManager->SaveDirtyFiles() == DirtyFileManager::CANCELED)
        {
            return;
        }

        ResetSettingsWindow resetWindow(this);
        if (resetWindow.exec() == QDialog::Accepted)
        {
            Reset(resetWindow.mActors, resetWindow.mMotionSets, resetWindow.mMotions, resetWindow.mAnimGraphs);
        }
    }


    // open an actor
    void MainWindow::OnFileOpenActor()
    {
        AZStd::vector<AZStd::string> filenames = mFileManager->LoadActorsFileDialog(this);
        if (filenames.empty())
        {
            return;
        }

        const size_t numFilenames = filenames.size();
        for (size_t i = 0; i < numFilenames; ++i)
        {
            LoadActor(filenames[i].c_str(), i == 0);
        }
    }


    // merge an actor
    void MainWindow::OnFileMergeActor()
    {
        AZStd::vector<AZStd::string> filenames = mFileManager->LoadActorsFileDialog(this);
        if (filenames.empty())
        {
            return;
        }

        for (const AZStd::string& filename : filenames)
        {
            LoadActor(filename.c_str(), false);
        }
    }


    // save selected actors
    void MainWindow::OnFileSaveSelectedActors()
    {
        // get the current selection list
        const CommandSystem::SelectionList& selectionList             = GetCommandManager()->GetCurrentSelection();
        const uint32                        numSelectedActors         = selectionList.GetNumSelectedActors();
        const uint32                        numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();

        // create the saving actor array
        AZStd::vector<EMotionFX::Actor*> savingActors;
        savingActors.reserve(numSelectedActors + numSelectedActorInstances);

        // add all selected actors to the list
        for (uint32 i = 0; i < numSelectedActors; ++i)
        {
            savingActors.push_back(selectionList.GetActor(i));
        }

        // check all actors of all selected actor instances and put them in the list if they are not in yet
        for (uint32 i = 0; i < numSelectedActorInstances; ++i)
        {
            EMotionFX::Actor* actor = selectionList.GetActorInstance(i)->GetActor();

            if (actor->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (AZStd::find(savingActors.begin(), savingActors.end(), actor) == savingActors.end())
            {
                savingActors.push_back(actor);
            }
        }

        // Save all selected actors.
        const size_t numActors = savingActors.size();
        for (size_t i = 0; i < numActors; ++i)
        {
            EMotionFX::Actor* actor = savingActors[i];
            GetMainWindow()->GetFileManager()->SaveActor(actor);
        }
    }


    void MainWindow::OnRecentFile(QAction* action)
    {
        const AZStd::string filename = action->data().toString().toUtf8().data();

        // Load the recent file.
        // No further error handling needed here as the commands do that all internally.
        LoadFile(filename.c_str(), 0, 0, false);
    }


    // save the current layout to a file
    void MainWindow::OnLayoutSaveAs()
    {
        EMStudio::GetLayoutManager()->SaveLayoutAs();
    }


    // select all actor instances
    void MainWindow::OnSelectAllActorInstances()
    {
        MCore::String outResult;
        if (GetCommandManager()->ExecuteCommand("Select -actorInstanceID SELECT_ALL", outResult) == false)
        {
            MCore::LogError(outResult.AsChar());
        }
    }


    // unselect all actor instances
    void MainWindow::OnUnselectAllActorInstances()
    {
        MCore::String outResult;
        if (GetCommandManager()->ExecuteCommand("Unselect -actorInstanceID SELECT_ALL", outResult) == false)
        {
            MCore::LogError(outResult.AsChar());
        }
    }


    // adjust node selection
    void MainWindow::OnAdjustNodeSelection()
    {
        // show the node selection window
        mNodeSelectionWindow->Update(MCORE_INVALIDINDEX32);
        mNodeSelectionWindow->show();
    }


    void MainWindow::OnNodeSelected(MCore::Array<SelectionItem> selection)
    {
        uint32 i;
        CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();

        // clear the old selection
        selectionList.ClearNodeSelection();

        const uint32 numSelectedItems = selection.GetLength();

        MCore::Array<uint32> selectedActorInstances;
        for (i = 0; i < numSelectedItems; ++i)
        {
            const uint32                actorInstanceID = selection[i].mActorInstanceID;
            EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
            if (actorInstance == nullptr)
            {
                continue;
            }

            // if the node name is empty we have selected the actor instance
            if (selection[i].GetNodeNameString().GetIsEmpty())
            {
                selectedActorInstances.Add(actorInstanceID);
            }
        }

        // get the number of selected actor instances and iterate through them
        const uint32 numSelectedActorInstances = selectedActorInstances.GetLength();

        // create our import motion command group
        MCore::String outResult;
        MCore::CommandGroup group("Select actor instances");

        group.AddCommandString("ClearSelection");

        // iterate over all selected actor instances
        for (i = 0; i < numSelectedActorInstances; ++i)
        {
            group.AddCommandString(MCore::String().Format("Select -actorInstanceID %i", selectedActorInstances[i]).AsChar());
        }

        // execute the group command
        GetCommandManager()->ExecuteCommandGroup(group, outResult);

        // get the number of selected items and iterate through them
        for (i = 0; i < numSelectedItems; ++i)
        {
            const uint32                actorInstanceID = selection[i].mActorInstanceID;
            EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
            if (actorInstance == nullptr)
            {
                continue;
            }

            // if the node name is empty we have selected the actor instance
            if (selection[i].GetNodeNameString().GetIsEmpty() == false)
            {
                EMotionFX::Actor*   actor = actorInstance->GetActor();
                EMotionFX::Node*    node = actor->GetSkeleton()->FindNodeByName(selection[i].GetNodeName());
                if (node)
                {
                    selectionList.AddNode(node);
                }
            }
        }
    }


    // update the layouts menu
    void MainWindow::UpdateLayoutsMenu()
    {
        // clear the current menu
        mLayoutsMenu->clear();

        // generate the layouts path
        QString layoutsPath = MysticQt::GetDataDir().AsChar();
        layoutsPath += "Layouts/";

        // open the dir
        QDir dir(layoutsPath);
        dir.setFilter(QDir::Files | QDir::NoSymLinks);
        dir.setSorting(QDir::Name);

        // add each layout
        mLayoutNames.Clear();
        MCore::String filename;
        const QFileInfoList list = dir.entryInfoList();
        const int listSize = list.size();
        for (int i = 0; i < listSize; ++i)
        {
            // get the filename
            const QFileInfo fileInfo = list[i];
            FromQtString(fileInfo.fileName(), &filename);

            // check the extension, only ".layout" are accepted
            if (filename.ExtractFileExtension().Lowered() == "layout")
            {
                filename.RemoveFileExtension();
                mLayoutNames.Add(filename);
            }
        }

        // add each menu
        const uint32 numLayoutNames = mLayoutNames.GetLength();
        for (uint32 i = 0; i < numLayoutNames; ++i)
        {
            QAction* action = mLayoutsMenu->addAction(mLayoutNames[i].AsChar());
            connect(action, SIGNAL(triggered()), this, SLOT(OnLoadLayout()));
        }

        // add the separator only if at least one layout
        if (numLayoutNames > 0)
        {
            mLayoutsMenu->addSeparator();
        }

        // add the save current menu
        QAction* saveCurrentAction = mLayoutsMenu->addAction("Save Current");
        connect(saveCurrentAction, SIGNAL(triggered()), this, SLOT(OnLayoutSaveAs()));

        // remove menu is needed only if at least one layout
        if (numLayoutNames > 0)
        {
            // add the remove menu
            QMenu* removeMenu = mLayoutsMenu->addMenu("Remove");

            // add each layout in the remove menu
            for (uint32 i = 0; i < numLayoutNames; ++i)
            {
                QAction* action = removeMenu->addAction(mLayoutNames[i].AsChar());
                connect(action, SIGNAL(triggered()), this, SLOT(OnRemoveLayout()));
            }
        }

        // disable signals to avoid to switch of layout
        mApplicationMode->blockSignals(true);

        // update the combo box
        mApplicationMode->clear();
        for (uint32 i = 0; i < numLayoutNames; ++i)
        {
            mApplicationMode->addItem(mLayoutNames[i].AsChar());
        }

        // update the current selection of combo box
        const int layoutIndex = mApplicationMode->findText(mLastUsedMode);
        mApplicationMode->setCurrentIndex(layoutIndex);

        // enable signals
        mApplicationMode->blockSignals(false);
    }


    // set the last used application mode and save if asked
    void MainWindow::SetLastUsedApplicationModeString(const QString& lastUsedApplicationMode, bool saveToConfigFile)
    {
        mLastUsedMode = lastUsedApplicationMode;
        if (saveToConfigFile)
        {
            SavePreferences();
        }
    }


    // called when the application mode combo box changed
    void MainWindow::ApplicationModeChanged(const QString& text)
    {
        // update the last used layout and save it in the preferences file
        mLastUsedMode = text;
        SavePreferences();

        // generate the filename
        MCore::String filename;
        filename.Format("%sLayouts/%s.layout", MysticQt::GetDataDir().AsChar(), FromQtString(text).AsChar());

        // try to load it
        if (GetLayoutManager()->LoadLayout(filename.AsChar()) == false)
        {
            MCore::LogError("Failed to load layout from file '%s'", filename.AsChar());
        }
    }


    // remove a given layout
    void MainWindow::OnRemoveLayout()
    {
        // make sure we really want to remove it
        QMessageBox msgBox(QMessageBox::Warning, "Remove The Selected Layout?", "Are you sure you want to remove the selected layout?<br>Note: This cannot be undone.", QMessageBox::Yes | QMessageBox::No, this);
        msgBox.setTextFormat(Qt::RichText);
        if (msgBox.exec() != QMessageBox::Yes)
        {
            return;
        }

        // generate the filename
        QAction* action = qobject_cast<QAction*>(sender());
        const QString filename = QString(MysticQt::GetDataDir().AsChar()) + "Layouts/" + action->text() + ".layout";

        // try to remove the file
        QFile file(filename);
        if (file.remove() == false)
        {
            MCore::LogError("Failed to remove layout file '%s'", FromQtString(filename).AsChar());
            return;
        }
        else
        {
            MCore::LogInfo("Successfullly removed layout file '%s'", FromQtString(filename).AsChar());
        }

        // check if the layout removed is the current used
        if (mLastUsedMode == action->text())
        {
            // find the layout index on the application mode combo box
            const int layoutIndex = mApplicationMode->findText(action->text());

            // set the new layout index, take the previous if the last layout is removed, the next is taken otherwise
            const int newLayoutIndex = (layoutIndex == (mApplicationMode->count() - 1)) ? layoutIndex - 1 : layoutIndex + 1;

            // select the layout, it also keeps it and saves to config
            mApplicationMode->setCurrentIndex(newLayoutIndex);
        }

        // update the layouts menu
        UpdateLayoutsMenu();
    }


    // load a given layout
    void MainWindow::OnLoadLayout()
    {
        // get the menu action
        QAction* action = qobject_cast<QAction*>(sender());

        // update the last used layout and save it in the preferences file
        mLastUsedMode = action->text();
        SavePreferences();

        // generate the filename
        MCore::String filename;
        filename.Format("%sLayouts/%s.layout", MysticQt::GetDataDir().AsChar(), FromQtString(action->text()).AsChar());

        // try to load it
        if (GetLayoutManager()->LoadLayout(filename.AsChar()))
        {
            // update the combo box
            mApplicationMode->blockSignals(true);
            const int layoutIndex = mApplicationMode->findText(action->text());
            mApplicationMode->setCurrentIndex(layoutIndex);
            mApplicationMode->blockSignals(false);
        }
        else
        {
            MCore::LogError("Failed to load layout from file '%s'", filename.AsChar());
        }
    }


    // undo
    void MainWindow::OnUndo()
    {
        // check if we can undo
        if (GetCommandManager()->GetNumHistoryItems() > 0 && GetCommandManager()->GetHistoryIndex() >= 0)
        {
            // perform the undo
            MCore::String outResult;
            const bool result = GetCommandManager()->Undo(outResult);

            // log the results if there are any
            if (outResult.GetLength() > 0)
            {
                if (result == false)
                {
                    MCore::LogError(outResult.AsChar());
                }
            }
        }

        // enable or disable the undo/redo menu options
        UpdateUndoRedo();
    }


    // redo
    void MainWindow::OnRedo()
    {
        // check if we can redo
        if (GetCommandManager()->GetNumHistoryItems() > 0 && GetCommandManager()->GetHistoryIndex() < (int32)GetCommandManager()->GetNumHistoryItems() - 1)
        {
            // perform the redo
            MCore::String outResult;
            const bool result = GetCommandManager()->Redo(outResult);

            // log the results if there are any
            if (outResult.GetLength() > 0)
            {
                if (result == false)
                {
                    MCore::LogError(outResult.AsChar());
                }
            }
        }

        // enable or disable the undo/redo menu options
        UpdateUndoRedo();
    }


    // update the undo and redo status in the menu (disabled or enabled)
    void MainWindow::UpdateUndoRedo()
    {
        // check the undo status
        if (GetCommandManager()->GetNumHistoryItems() > 0 && GetCommandManager()->GetHistoryIndex() >= 0)
        {
            mUndoAction->setEnabled(true);
        }
        else
        {
            mUndoAction->setEnabled(false);
        }

        // check the redo status
        if (GetCommandManager()->GetNumHistoryItems() > 0 && GetCommandManager()->GetHistoryIndex() < (int32)GetCommandManager()->GetNumHistoryItems() - 1)
        {
            mRedoAction->setEnabled(true);
        }
        else
        {
            mRedoAction->setEnabled(false);
        }
    }


    // disable undo/redo
    void MainWindow::DisableUndoRedo()
    {
        mUndoAction->setEnabled(false);
        mRedoAction->setEnabled(false);
    }


    void MainWindow::LoadFile(const MCore::String& fileName, int32 contextMenuPosX, int32 contextMenuPosY, bool contextMenuEnabled, bool reload)
    {
        AZStd::vector<AZStd::string> filenames;
        filenames.push_back(AZStd::string(fileName.AsChar()));
        LoadFiles(filenames, contextMenuPosX, contextMenuPosY, contextMenuEnabled, reload);
    }


    void MainWindow::LoadFiles(const AZStd::vector<AZStd::string>& filenames, int32 contextMenuPosX, int32 contextMenuPosY, bool contextMenuEnabled, bool reload)
    {
        if (filenames.empty())
        {
            return;
        }

        AZStd::vector<AZStd::string> actorFilenames;
        AZStd::vector<AZStd::string> motionFilenames;
        AZStd::vector<AZStd::string> animGraphFilenames;
        AZStd::vector<AZStd::string> workspaceFilenames;
        AZStd::vector<AZStd::string> motionSetFilenames;

        // get the number of urls and iterate over them
        AZStd::string filename;
        AZStd::string extension;
        const uint32 numFiles = filenames.size();
        for (uint32 i = 0; i < numFiles; ++i)
        {
            // get the complete file name and extract the extension
            filename = filenames[i];
            AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false);

            if (AzFramework::StringFunc::Equal(extension.c_str(), "actor"))
            {
                actorFilenames.push_back(filename);
            }
            else
            if (AzFramework::StringFunc::Equal(extension.c_str(), "motion"))
            {
                motionFilenames.push_back(filename);
            }
            else
            if (AzFramework::StringFunc::Equal(extension.c_str(), "animgraph"))
            {
                // Force-load from asset source folder.
                AZStd::string assetSourceFilename = filename;
                if (GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(assetSourceFilename))
                {
                    animGraphFilenames.push_back(assetSourceFilename);
                }
            }
            else
            if (AzFramework::StringFunc::Equal(extension.c_str(), "emfxworkspace"))
            {
                workspaceFilenames.push_back(filename);
            }
            else
            if (AzFramework::StringFunc::Equal(extension.c_str(), "motionset"))
            {
                // Force-load from asset source folder.
                AZStd::string assetSourceFilename = filename;
                if (GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(assetSourceFilename))
                {
                    motionSetFilenames.push_back(assetSourceFilename);
                }
            }
        }

        //--------------------

        if (!motionFilenames.empty())
        {
            CommandSystem::LoadMotionsCommand(motionFilenames, reload);
        }
        if (!motionSetFilenames.empty())
        {
            CommandSystem::LoadMotionSetsCommand(motionSetFilenames, reload, false);
        }

        CommandSystem::LoadAnimGraphsCommand(animGraphFilenames, reload);

        //--------------------

        const size_t actorCount = actorFilenames.size();
        if (actorCount == 1)
        {
            mDroppedActorFileName = actorFilenames[0].c_str();
            mRecentActors.AddRecentFile(mDroppedActorFileName.AsChar());

            if (contextMenuEnabled)
            {
                if (EMotionFX::GetActorManager().GetNumActors() > 0)
                {
                    // create the drop context menu
                    QMenu menu(this);
                    QAction* openAction = menu.addAction("Open Actor");
                    QAction* mergeAction = menu.addAction("Merge Actor");
                    connect(openAction, SIGNAL(triggered()), this, SLOT(OnOpenDroppedActor()));
                    connect(mergeAction, SIGNAL(triggered()), this, SLOT(OnMergeDroppedActor()));

                    // show the menu at the given position
                    menu.exec(mapToGlobal(QPoint(contextMenuPosX, contextMenuPosY)));
                }
                else
                {
                    OnOpenDroppedActor();
                }
            }
            else
            {
                OnOpenDroppedActor();
            }
        }
        else
        {
            // Load and merge all actors.
            for (const AZStd::string& actorFilename : actorFilenames)
            {
                LoadActor(actorFilename.c_str(), false);
            }
        }

        //--------------------

        const size_t numWorkspaces = workspaceFilenames.size();
        if (numWorkspaces > 0)
        {
            // make sure we did not cancel load workspace
            if (mDirtyFileManager->SaveDirtyFiles() != DirtyFileManager::CANCELED)
            {
                // add the workspace in the recent workspace list
                // if the same workspace is already in the list, the duplicate is removed
                mRecentWorkspaces.AddRecentFile(workspaceFilenames[0]);

                // create the command group
                MCore::CommandGroup workspaceCommandGroup("Load workspace", 64);

                // clear everything before laoding a new workspace file
                Reset(true, true, true, true, &workspaceCommandGroup);
                workspaceCommandGroup.SetReturnFalseAfterError(true);

                // load the first workspace of the list as more doesn't make sense anyway
                Workspace* workspace = GetManager()->GetWorkspace();
                if (workspace->Load(workspaceFilenames[0].c_str(), &workspaceCommandGroup))
                {
                    // execute the group command
                    AZStd::string result;
                    if (GetCommandManager()->ExecuteCommandGroup(workspaceCommandGroup, result))
                    {
                        // set the workspace not dirty
                        workspace->SetDirtyFlag(false);

                        // for all registered plugins, call the after load workspace callback
                        PluginManager* pluginManager = GetPluginManager();
                        const uint32 numPlugins = pluginManager->GetNumActivePlugins();
                        for (uint32 p = 0; p < numPlugins; ++p)
                        {
                            EMStudioPlugin* plugin = pluginManager->GetActivePlugin(p);
                            plugin->OnAfterLoadProject();
                        }

                        // clear the history
                        GetCommandManager()->ClearHistory();

                        // set the window title using the workspace filename
                        SetWindowTitleFromFileName(workspaceFilenames[0].c_str());
                    }
                    else
                    {
                        AZ_Error("EMotionFX", false, result.c_str());
                    }
                }
            }
        }
    }

    void MainWindow::LoadLayoutAfterShow()
    {
        if (!mLayoutLoaded)
        {
            mLayoutLoaded = true;

            LoadDefaultLayout();
            if (!mCharacterFiles.empty())
            {
                // Need to defer loading the character until the layout is ready. We also
                // need a couple of initializeGL/paintGL to happen before the character
                // is being loaded.
                QTimer::singleShot(500, this, SLOT(LoadCharacterFiles()));
            }
        }
    }

    void MainWindow::RaiseFloatingWidgets()
    {
        const QList<MysticQt::DockWidget*> dockWidgetList = findChildren<MysticQt::DockWidget*>();
        for (MysticQt::DockWidget* dockWidget : dockWidgetList)
        {
            if (dockWidget->isFloating())
            {
                // There is some weird behavior with floating QDockWidget. After showing it,
                // the widget doesn't seem to remain when we move/maximize or do some changes in the
                // window that contains it. Setting it as floating false then true seems to workaround
                // the problem
                dockWidget->setFloating(false);
                dockWidget->setFloating(true);

                dockWidget->show();
                dockWidget->raise();
            }
        }
    }

    // Load default layout.
    void MainWindow::LoadDefaultLayout()
    {
        int layoutIndex = mApplicationMode->findText(mLastUsedMode);

        // If searching for the last used layout fails load the default or viewer layout if they exist
        if (layoutIndex == -1)
        {
            layoutIndex = mApplicationMode->findText("AnimGraph");
        }
        if (layoutIndex == -1)
        {
            layoutIndex = mApplicationMode->findText("Character");
        }
        if (layoutIndex == -1)
        {
            layoutIndex = mApplicationMode->findText("Animation");
        }

        mApplicationMode->setCurrentIndex(layoutIndex);
    }

    void MainWindow::LoadCharacterFiles()
    {
        if (!mCharacterFiles.empty())
        {
            LoadFiles(mCharacterFiles, 0, 0, false, true);
            mCharacterFiles.clear();

            // for all registered plugins, call the after load actors callback
            PluginManager* pluginManager = GetPluginManager();
            const uint32 numPlugins = pluginManager->GetNumActivePlugins();
            for (uint32 p = 0; p < numPlugins; ++p)
            {
                EMStudioPlugin* plugin = pluginManager->GetActivePlugin(p);
                plugin->OnAfterLoadActors();
            }
        }
    }


    // accept drops
    void MainWindow::dragEnterEvent(QDragEnterEvent* event)
    {
        // this is needed to actually reach the drop event function
        event->acceptProposedAction();
    }


    // gets called when the user drag&dropped an actor to the application and then chose to open it in the context menu
    void MainWindow::OnOpenDroppedActor()
    {
        LoadActor(mDroppedActorFileName.AsChar(), true);
    }


    // gets called when the user drag&dropped an actor to the application and then chose to merge it in the context menu
    void MainWindow::OnMergeDroppedActor()
    {
        LoadActor(mDroppedActorFileName.AsChar(), false);
    }


    // handle drop events
    void MainWindow::dropEvent(QDropEvent* event)
    {
        // check if we dropped any files to the application
        const QMimeData* mimeData = event->mimeData();

        AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*> entries;
        AzToolsFramework::AssetBrowser::AssetBrowserEntry::FromMimeData(mimeData, entries);

        AZStd::vector<AZStd::string> fileNames;
        for (const auto& entry : entries)
        {
            AZStd::vector<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*> productEntries;
            entry->GetChildrenRecursively<AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry>(productEntries);
            for (const auto& productEntry : productEntries)
            {
                fileNames.emplace_back(FileManager::GetAssetFilenameFromAssetId(productEntry->GetAssetId()));
            }
        }
        LoadFiles(fileNames, event->pos().x(), event->pos().y());

        event->acceptProposedAction();
    }


    void MainWindow::closeEvent(QCloseEvent* event)
    {
        if (mDirtyFileManager->SaveDirtyFiles() == DirtyFileManager::CANCELED)
        {
            event->ignore();
        }
        else
        {
            mAutosaveTimer->stop();

            PluginManager* pluginManager = GetPluginManager();

            // The close event does not hide floating widgets, so we are doing that manually here
            const QList<MysticQt::DockWidget*> dockWidgetList = findChildren<MysticQt::DockWidget*>();
            for (MysticQt::DockWidget* dockWidget : dockWidgetList)
            {
                if (dockWidget->isFloating())
                {
                    dockWidget->hide();
                }
            }

            // get a copy of the active plugins since some plugins may choose
            // to get inactive when the main window closes
            const AZStd::vector<EMStudioPlugin*> activePlugins = pluginManager->GetActivePlugins();
            for (EMStudioPlugin* activePlugin : activePlugins)
            {
                AZ_Assert(activePlugin, "Unexpected null active plugin");
                activePlugin->OnMainWindowClosed();
            }

            QMainWindow::closeEvent(event);
        }

        // We mark it as false so next time is shown the layout is re-loaded if
        // necessary
        mLayoutLoaded = false;
    }


    void MainWindow::showEvent(QShowEvent* event)
    {
        if (mEnableAutosave)
        {
            mAutosaveTimer->start();
        }

        // EMotionFX dock widget is created the first time it's opened, so we need to load layout after that
        // The singleShot is needed because show event is fired before the dock widget resizes (in the same function dock widget is created)
        // So we want to load layout after that. It's a bit hacky, but most sensible at the moment.
        if (!mLayoutLoaded)
        {
            QTimer::singleShot(0, this, SLOT(LoadLayoutAfterShow()));
        }

        QMainWindow::showEvent(event);

        // This show event ends up being called twice from QtViewPaneManager::OpenPane. At the end of the method
        // is doing a "raise" on this window. Since we cannot intercept that raise (raise is a slot and doesn't
        // have an event associated) we are deferring a call to RaiseFloatingWidgets which will raise the floating
        // widgets (this needs to happen after the raise from OpenPane).
        QTimer::singleShot(0, this, SLOT(RaiseFloatingWidgets()));
    }

    void MainWindow::keyPressEvent(QKeyEvent* event)
    {
        const char* layoutGroupName = "Layouts";
        const uint32 numLayouts = GetMainWindow()->GetNumLayouts();
        for (uint32 i = 0; i < numLayouts; ++i)
        {
            if (mShortcutManager->Check(event, GetLayoutName(i), layoutGroupName))
            {
                mApplicationMode->setCurrentIndex(i);
                event->accept();
                return;
            }
        }

        event->ignore();
    }


    void MainWindow::keyReleaseEvent(QKeyEvent* event)
    {
        const char* layoutGroupName = "Layouts";
        const uint32 numLayouts = GetNumLayouts();
        for (uint32 i = 0; i < numLayouts; ++i)
        {
            if (mShortcutManager->Check(event, layoutGroupName, GetLayoutName(i)))
            {
                event->accept();
                return;
            }
        }

        event->ignore();
    }


    // get the name of the currently active layout
    const char* MainWindow::GetCurrentLayoutName() const
    {
        // get the selected layout
        const int currentLayoutIndex = mApplicationMode->currentIndex();

        // if the index is out of range, return empty name
        if ((currentLayoutIndex < 0) || (currentLayoutIndex >= (int32)GetNumLayouts()))
        {
            return "";
        }

        // return the layout name
        return GetLayoutName(currentLayoutIndex);
    }

    const char* MainWindow::GetEMotionFXPaneName()
    {
        return LyViewPane::AnimationEditor;
    }


    void MainWindow::OnAutosaveTimeOut()
    {
        AZStd::vector<AZStd::string> filenames;
        AZStd::vector<AZStd::string> dirtyFileNames;
        AZStd::vector<SaveDirtyFilesCallback::ObjectPointer> objects;
        AZStd::vector<SaveDirtyFilesCallback::ObjectPointer> dirtyObjects;

        const size_t numDirtyFilesCallbacks = mDirtyFileManager->GetNumCallbacks();
        for (size_t i = 0; i < numDirtyFilesCallbacks; ++i)
        {
            SaveDirtyFilesCallback* callback = mDirtyFileManager->GetCallback(i);
            callback->GetDirtyFileNames(&filenames, &objects);
            const size_t numFileNames = filenames.size();
            for (size_t j = 0; j < numFileNames; ++j)
            {
                // bypass if the filename is empty
                // it's the case when the file is not already saved
                if (filenames[j].empty())
                {
                    continue;
                }

                // avoid duplicate of filename and object
                if (AZStd::find(dirtyFileNames.begin(), dirtyFileNames.end(), filenames[j]) == dirtyFileNames.end())
                {
                    dirtyFileNames.push_back(filenames[j]);
                    dirtyObjects.push_back(objects[j]);
                }
            }
        }

        // Skip directly in case there are no dirty files.
        if (dirtyFileNames.empty() && dirtyObjects.empty())
        {
            return;
        }

        // create the command group
        AZStd::string command;
        MCore::CommandGroup commandGroup("Autosave");

        // get the autosaves folder
        const MCore::String autosavesFolder = GetManager()->GetAutosavesFolder();

        // save each dirty object
        QStringList entryList;
        AZStd::string filename, extension, startWithAutosave;
        const size_t numFileNames = dirtyFileNames.size();
        for (size_t i = 0; i < numFileNames; ++i)
        {
            // get the full path
            filename = dirtyFileNames[i];

            // get the base name with autosave
            AzFramework::StringFunc::Path::GetFileName(filename.c_str(), startWithAutosave);
            startWithAutosave += "_Autosave";

            // get the extension
            AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false);

            // open the dir and get the file list
            const QDir dir = QDir(autosavesFolder.AsChar());
            entryList = dir.entryList(QDir::Files, QDir::Time | QDir::Reversed);

            // generate the autosave file list
            int maxAutosaveFileNumber = 0;
            QList<QString> autosaveFileList;
            const int numEntry = entryList.size();
            for (int j = 0; j < numEntry; ++j)
            {
                // get the file info
                const QFileInfo fileInfo = QFileInfo(autosavesFolder + entryList[j]);

                // check the extension
                if (fileInfo.suffix() != extension.c_str())
                {
                    continue;
                }

                // check the base name
                const QString baseName = fileInfo.baseName();
                if (baseName.startsWith(startWithAutosave.c_str()))
                {
                    // extract the number
                    const int numberExtracted = baseName.mid(static_cast<int>(startWithAutosave.size())).toInt();

                    // check if the number is valid
                    if (numberExtracted > 0)
                    {
                        // add the file in the list
                        autosaveFileList.append(autosavesFolder + entryList[j]);
                        AZ_Printf("EMotionFX", "Appending '%s' #%i\n", entryList[j].toUtf8().data(), numberExtracted);

                        // Update the maximum autosave file number that already exists on disk.
                        maxAutosaveFileNumber = MCore::Max<int>(maxAutosaveFileNumber, numberExtracted);
                    }
                }
            }

            // check if the length is upper than the max num files
            if (autosaveFileList.length() >= mAutosaveNumberOfFiles)
            {
                // number of files to delete
                // one is added because one space needs to be free for the new file
                const int numFilesToDelete = (autosaveFileList.size() - mAutosaveNumberOfFiles) + 1;

                // delete each file
                for (int j = 0; j < numFilesToDelete; ++j)
                {
                    AZ_Printf("EMotionFX", "Removing '%s'\n", autosaveFileList[j].toUtf8().data());
                    QFile::remove(autosaveFileList[j]);
                }
            }

            // Set the new autosave file number and prevent an integer overflow.
            int newAutosaveFileNumber = maxAutosaveFileNumber + 1;
            if (newAutosaveFileNumber == std::numeric_limits<int>::max())
            {
                // Restart counting autosave file numbers from the beginning.
                newAutosaveFileNumber = 1;
            }

            // save the new file
            AZStd::string newFileFilename;
            newFileFilename = AZStd::string::format("%s%s%d.%s", autosavesFolder.AsChar(), startWithAutosave.c_str(), newAutosaveFileNumber, extension.c_str());
            AZ_Printf("EMotionFX", "Saving to '%s'\n", newFileFilename.c_str());

            // Backing up actors and motions doesn't work anymore as we just update the .assetinfos and the asset processor does the rest.
            if (dirtyObjects[i].mMotionSet)
            {
                command = AZStd::string::format("SaveMotionSet -motionSetID %i -filename \"%s\" -updateFilename false -updateDirtyFlag false -sourceControl false", dirtyObjects[i].mMotionSet->GetID(), newFileFilename.c_str());
                commandGroup.AddCommandString(command);
            }
            else if (dirtyObjects[i].mAnimGraph)
            {
                const uint32 animGraphIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(dirtyObjects[i].mAnimGraph);
                command = AZStd::string::format("SaveAnimGraph -index %i -filename \"%s\" -updateFilename false -updateDirtyFlag false -sourceControl false", animGraphIndex, newFileFilename.c_str());
                commandGroup.AddCommandString(command);
            }
            else if (dirtyObjects[i].mWorkspace)
            {
                Workspace* workspace = GetManager()->GetWorkspace();
                workspace->Save(newFileFilename.c_str(), false, false);
            }
        }

        // execute the command group
        AZStd::string result;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, result, false))
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "Autosave <font color=green>completed</font>");
        }
        else
        {
            MCore::LogError(result.c_str());
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "Autosave <font color=red>failed</font>");
        }
    }

    
    void MainWindow::moveEvent(QMoveEvent* event)
    {
        MCORE_UNUSED(event);
        GetManager()->GetNotificationWindowManager()->OnMovedOrResized();
    }


    void MainWindow::resizeEvent(QResizeEvent* event)
    {
        MCORE_UNUSED(event);
        GetManager()->GetNotificationWindowManager()->OnMovedOrResized();
    }


    void MainWindow::OnUpdateRenderPlugins()
    {
        // sort the active plugins based on their priority
        PluginManager* pluginManager = GetPluginManager();
        pluginManager->SortActivePlugins();

        // get the number of active plugins, iterate through them and call the process frame method
        const uint32 numPlugins = pluginManager->GetNumActivePlugins();
        for (uint32 p = 0; p < numPlugins; ++p)
        {
            EMStudioPlugin* plugin = pluginManager->GetActivePlugin(p);
            if (plugin->GetPluginType() == EMStudioPlugin::PLUGINTYPE_RENDERING)
            {
                plugin->ProcessFrame(0.0f);
            }
        }
    }


    // scale the selected actors
    void MainWindow::OnScaleSelectedActors()
    {
        UnitScaleWindow scaleWindow(this);
        if (scaleWindow.exec() != QDialog::Accepted)
        {
            return;
        }

        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActors = selectionList.GetNumSelectedActors();
        if (numSelectedActors == 0)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Scale actor data");
        MCore::String tempString;
        for (uint32 i = 0; i < numSelectedActors; ++i)
        {
            EMotionFX::Actor* actor = selectionList.GetActor(i);
            tempString.Format("ScaleActorData -id %d -scaleFactor %.8f", actor->GetID(), scaleWindow.GetScaleFactor());
            commandGroup.AddCommandString(tempString.AsChar());
        }

        // execute the command group
        MCore::String outResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.AsChar());
        }
    }


    // scale the selected motions
    void MainWindow::OnScaleSelectedMotions()
    {
        UnitScaleWindow scaleWindow(this);
        if (scaleWindow.exec() != QDialog::Accepted)
        {
            return;
        }

        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelected = selectionList.GetNumSelectedMotions();
        if (numSelected == 0)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Scale motion data");
        MCore::String tempString;
        for (uint32 i = 0; i < numSelected; ++i)
        {
            EMotionFX::Motion* motion = selectionList.GetMotion(i);
            const char* skipInterfaceUpdate = (i == (numSelected - 1)) ? "false" : "true";
            tempString.Format("ScaleMotionData -id %d -scaleFactor %.8f -skipInterfaceUpdate %s", motion->GetID(), scaleWindow.GetScaleFactor(), skipInterfaceUpdate);
            commandGroup.AddCommandString(tempString.AsChar());
        }

        // execute the command group
        MCore::String outResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.AsChar());
        }
    }


    // scale the selected motions
    void MainWindow::OnScaleSelectedAnimGraphs()
    {
        UnitScaleWindow scaleWindow(this);
        if (scaleWindow.exec() != QDialog::Accepted)
        {
            return;
        }

        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelected = selectionList.GetNumSelectedAnimGraphs();
        if (numSelected == 0)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Scale anim graph data");
        MCore::String tempString;
        for (uint32 i = 0; i < numSelected; ++i)
        {
            EMotionFX::AnimGraph* animGraph = selectionList.GetAnimGraph(i);
            tempString.Format("ScaleAnimGraphData -id %d -scaleFactor %.8f", animGraph->GetID(), scaleWindow.GetScaleFactor());
            commandGroup.AddCommandString(tempString.AsChar());
        }

        // execute the command group
        MCore::String outResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.AsChar());
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MainWindow.moc>
