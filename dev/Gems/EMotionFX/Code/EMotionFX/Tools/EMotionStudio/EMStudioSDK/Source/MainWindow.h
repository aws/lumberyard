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

#pragma once

// include the required headers
#include "EMStudioConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/UnicodeString.h>
#include <AzCore/Debug/Timer.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include "NodeSelectionWindow.h"
#include "SaveChangedFilesManager.h"
#include <MysticQt/Source/RecentFiles.h>
#include <MysticQt/Source/KeyboardShortcutManager.h>
#include <MysticQt/Source/PropertyWidget.h>
#include <MysticQt/Source/ComboBox.h>
#include "FileManager.h"
#include <QMainWindow>
#include <QMenu>
#include <QTimer>
#include <QDropEvent>
#include <QUrl>
#include <QCheckBox>
#include <QAbstractNativeEventFilter>

// forward declarations
QT_FORWARD_DECLARE_CLASS(QSignalMapper)
QT_FORWARD_DECLARE_CLASS(QPushButton)


namespace EMStudio
{
    // forward declaration
    class PreferencesWindow;
    class NodeSelectionWindow;
    class EMStudioPlugin;
    class MainWindow;


    class NativeEventFilter
        : public QAbstractNativeEventFilter
    {
    public:
        NativeEventFilter(MainWindow* mainWindow)
            : QAbstractNativeEventFilter(),
              m_MainWindow(mainWindow)
        {
        }

        virtual bool nativeEventFilter(const QByteArray& /*eventType*/, void* message, long* /*result*/) Q_DECL_OVERRIDE;

    private:
        MainWindow*         m_MainWindow;
    };


    // the main window
    class EMSTUDIO_API MainWindow
        : public QMainWindow
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MainWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = nullptr);
        ~MainWindow();

        void StopRendering();
        void StartRendering();

        void UpdateCreateWindowMenu();
        void UpdateLayoutsMenu();
        void UpdateUndoRedo();
        void DisableUndoRedo();
        void Init();

        MCORE_INLINE QMenu* GetLayoutsMenu()                                    { return mLayoutsMenu; }

        void LoadActor(const char* fileName, bool replaceCurrentScene);
        void LoadCharacter(const AZ::Data::AssetId& actorAssetId, const AZ::Data::AssetId& animgraphId, const AZ::Data::AssetId& motionSetId);
        void LoadFile(const MCore::String& fileName, int32 contextMenuPosX = 0, int32 contextMenuPosY = 0, bool contextMenuEnabled = true, bool reload = false);
        void LoadFiles(const AZStd::vector<AZStd::string>& filenames, int32 contextMenuPosX = 0, int32 contextMenuPosY = 0, bool contextMenuEnabled = true, bool reload = false);

        void SetLastUsedApplicationModeString(const QString& lastUsedApplicationMode, bool saveToConfigFile);
        void SetMaxRecentFiles(uint32 numRecentFiles, bool saveToConfigFile);
        MCORE_INLINE uint32 GetMaxRecentFiles()                                 { return mMaxNumRecentFiles; }
        MysticQt::RecentFiles* GetRecentWorkspaces()                            { return &mRecentWorkspaces; }

        void Reset(bool clearActors = true, bool clearMotionSets = true, bool clearMotions = true, bool clearAnimGraphs = true, MCore::CommandGroup* commandGroup = nullptr);

        // settings
        void SavePreferences();
        void LoadPreferences();

        void UpdateResetAndSaveAllMenus();
        void EnableMergeActorMenu();
        void DisableMergeActorMenu();
        void EnableSaveSelectedActorsMenu();
        void DisableSaveSelectedActorsMenu();

        void OnWorkspaceSaved(const char* filename);

        void RegisterDirtyWorkspaceCallback();

        MCORE_INLINE MysticQt::ComboBox* GetApplicationModeComboBox()           { return mApplicationMode; }
        MCORE_INLINE QString GetLastUsedApplicationModeString()                 { return mLastUsedMode; }
        DirtyFileManager*   GetDirtyFileManager() const                         { return mDirtyFileManager; }
        FileManager*        GetFileManager() const                              { return mFileManager; }
        PreferencesWindow*  GetPreferencesWindow() const                        { return mPreferencesWindow; }

        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        uint32 GetNumLayouts() const                                            { return mLayoutNames.GetLength(); }
        const char* GetLayoutName(uint32 index) const                           { return mLayoutNames[index].AsChar(); }
        const char* GetCurrentLayoutName() const;

        static const char* GetEMotionFXPaneName() { return "EMotion FX Animation Editor (PREVIEW)"; }
        MysticQt::KeyboardShortcutManager* GetShortcutManager() const           { return mShortcutManager; }

    public slots:
        void OnAutosaveTimeOut();
        void OnRealtimeInterfaceUpdate();
        void OnScaleSelectedActors();
        void OnScaleSelectedMotions();
        void OnScaleSelectedAnimGraphs();
        void StartEditorFirstTime();

    protected:
        void moveEvent(QMoveEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void LoadCharacterFiles();
        void LoadDefaultLayout();

    private:
        QMenu*                  mCreateWindowMenu;
        QMenu*                  mLayoutsMenu;
        QAction*                mUndoAction;
        QAction*                mRedoAction;

        // keyboard shortcut manager
        MysticQt::KeyboardShortcutManager* mShortcutManager;

        // layouts (application modes)
        MCore::Array<MCore::String> mLayoutNames;

        // menu actions
        QAction*                mResetAction;
        QAction*                mSaveAllAction;
        QAction*                mMergeActorAction;
        QAction*                mSaveSelectedActorsAction;
#ifdef EMFX_DEVELOPMENT_BUILD
        QAction*                mSaveSelectedActorAsAttachmentsAction;
#endif

        // application mode
        MysticQt::ComboBox*     mApplicationMode;
        QString                 mLastUsedMode;

        PreferencesWindow*      mPreferencesWindow;

        // node selection window
        NodeSelectionWindow*    mNodeSelectionWindow;

        EMStudio::FileManager*  mFileManager;

        MysticQt::RecentFiles   mRecentActors;
        MysticQt::RecentFiles   mRecentWorkspaces;
        uint32                  mMaxNumRecentFiles;

        // dirty files
        DirtyFileManager*       mDirtyFileManager;

        void SetAspiredRenderingFPS(int32 fps);
        void SetWindowTitleFromFileName(const MCore::String& fileName);

        // drag & drop support
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        MCore::String           mDroppedActorFileName;

        // general properties (preferences dialog)
        MysticQt::PropertyWidget::Property*     mEnableAutosaveProperty;
        MysticQt::PropertyWidget::Property*     mAutosaveIntervalProperty;
        MysticQt::PropertyWidget::Property*     mAutosaveNumberOfFilesProperty;
        MysticQt::PropertyWidget::Property*     mNotificationVisibleTimeProperty;
        MysticQt::PropertyWidget::Property*     mMaxHistoryItemsProperty;
        MysticQt::PropertyWidget::Property*     mMaxRecentFilesProperty;
        MysticQt::PropertyWidget::Property*     mImporterDetailedLogging;
        MysticQt::PropertyWidget::Property*     mAutoLoadLastWorkspaceProperty;
        MysticQt::PropertyWidget::Property*     mAspiredRenderingFPSProperty;

        QTimer*                                 mAutosaveTimer;
        QTimer*                                 mRealtimeInterfaceTimer;
        AZ::Debug::Timer                        mFrameTimer;
        int32                                   mAspiredRenderFPS;
        int32                                   mNotificationVisibleTime;
        int32                                   mAutosaveInterval;
        int32                                   mAutosaveNumberOfFiles;
        bool                                    mEnableAutosave;
        bool                                    mFirstTimeOpenWindow;

        AZStd::vector<AZStd::string>             mCharacterFiles;

        void closeEvent(QCloseEvent* event) override;
        void showEvent(QShowEvent* event) override;

        // declare the callbacks
        MCORE_DEFINECOMMANDCALLBACK(CommandImportActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveActorInstanceCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandImportMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandLoadMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandLoadAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSaveWorkspaceCallback);
        CommandImportActorCallback*         mImportActorCallback;
        CommandRemoveActorCallback*         mRemoveActorCallback;
        CommandRemoveActorInstanceCallback* mRemoveActorInstanceCallback;
        CommandImportMotionCallback*        mImportMotionCallback;
        CommandRemoveMotionCallback*        mRemoveMotionCallback;
        CommandCreateMotionSetCallback*     mCreateMotionSetCallback;
        CommandRemoveMotionSetCallback*     mRemoveMotionSetCallback;
        CommandLoadMotionSetCallback*       mLoadMotionSetCallback;
        CommandCreateAnimGraphCallback*     mCreateAnimGraphCallback;
        CommandRemoveAnimGraphCallback*     mRemoveAnimGraphCallback;
        CommandLoadAnimGraphCallback*       mLoadAnimGraphCallback;
        CommandSelectCallback*              mSelectCallback;
        CommandUnselectCallback*            mUnselectCallback;
        CommandSaveWorkspaceCallback*       mSaveWorkspaceCallback;


    public slots:
        void OnValueChanged(MysticQt::PropertyWidget::Property* property);
        void OnFileOpenActor();
        void OnFileSaveSelectedActors();
        void OnReset();
        void OnFileMergeActor();
        void OnOpenDroppedActor();
        void OnRecentFile(QAction* action);
        void OnMergeDroppedActor();
        void OnFileNewWorkspace();
        void OnFileOpenWorkspace();
        void OnFileSaveWorkspace();
        void OnFileSaveWorkspaceAs();
        void OnWindowCreate(bool checked);
        void OnLayoutSaveAs();
        void OnRemoveLayout();
        void OnLoadLayout();
        void OnUndo();
        void OnRedo();
        void OnOpenAutosaveFolder();
        void OnOpenSettingsFolder();
        void OnPreferences();
        void OnSelectAllActorInstances();
        void OnUnselectAllActorInstances();
        void OnAdjustNodeSelection();
        void OnNodeSelected(MCore::Array<SelectionItem> selection);
        void OnSaveAll();
        void ApplicationModeChanged(const QString& text);
        void OnUpdateRenderPlugins();

     signals:
        void HardwareChangeDetected();
    };


    class EMSTUDIO_API ResetSettingsWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ResetSettingsWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        ResetSettingsWindow(QWidget* parent);
        ~ResetSettingsWindow() {}

    public slots:
        void OnOKButton()                   { emit accept(); }
        void OnCancelButton()               { emit reject(); }
        void OnActorCheckbox(int state)     { MCORE_UNUSED(state); mActors = mActorCheckbox->isChecked(); }
        void OnMotionCheckbox(int state)    { MCORE_UNUSED(state); mMotions = mMotionCheckbox->isChecked(); }
        void OnMotionSetCheckbox(int state) { MCORE_UNUSED(state); mMotionSets = mMotionSetCheckbox->isChecked(); }
        void OnAnimGraphCheckbox(int state){ MCORE_UNUSED(state); mAnimGraphs = mAnimGraphCheckbox->isChecked(); }

    private:
        QPushButton*        mOK;
        QPushButton*        mCancel;
        QCheckBox*          mActorCheckbox;
        QCheckBox*          mMotionSetCheckbox;
        QCheckBox*          mMotionCheckbox;
        QCheckBox*          mAnimGraphCheckbox;

    public:
        bool                mActors;
        bool                mMotionSets;
        bool                mMotions;
        bool                mAnimGraphs;
    };
} // namespace EMStudio
