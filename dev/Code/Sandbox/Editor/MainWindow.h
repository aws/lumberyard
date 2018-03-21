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

#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <QComboBox>
#include <QMainWindow>
#include <QScopedPointer>
#include <QSettings>
#include <QList>
#include <QPointer>
#include <QToolButton>
#include <QTimer>
#include <QAbstractNativeEventFilter>

#include "Include/SandboxAPI.h"
#include "ActionManager.h"
#include <AzQtComponents/Components/ToolButtonComboBox.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <QAbstractNativeEventFilter>

class AssetImporterManager;
class NetPromoterScoreDialog;
class DayCountManager;
class LevelEditorMenuHandler;
class CMainFrame;
class UndoStackStateAdapter;
class QComboBox;
class KeyboardCustomizationSettings;
class QToolButton;
class MainStatusBar;
class CLayoutWnd;
struct QtViewPane;
class CLayoutViewPane;
class QtViewport;
class QtViewPaneManager;
class EngineConnectionListener;
class QRollupCtrl;
class ToolbarManager;
class ToolbarCustomizationDialog;
class QWidgetAction;

namespace AzToolsFramework
{
    class Ticker;
}

namespace AzToolsFramework
{
    class QtSourceControlNotificationHandler;
}

#define MAINFRM_LAYOUT_NORMAL "NormalLayout"
#define MAINFRM_LAYOUT_PREVIEW "PreviewLayout"

// Subclassing so we can add slots to our toolbar widgets
// Using lambdas is crashy since the lamdba doesn't know when the widget is deleted.

class RefCoordComboBox
    : public QComboBox
{
    Q_OBJECT
public:
    explicit RefCoordComboBox(QWidget* parent);
public Q_SLOTS:
    void ToggleRefCoordSys();
    void UpdateRefCoordSys();
private:
    QStringList coordSysList() const;
};

class SelectionComboBox
    : public AzQtComponents::ToolButtonComboBox
{
    Q_OBJECT
public:
    explicit SelectionComboBox(QAction* action, QWidget* parent);
public Q_SLOTS:
    void DeleteSelection();
};

class UndoRedoToolButton
    : public QToolButton
{
    Q_OBJECT
public:
    explicit UndoRedoToolButton(QWidget* parent);
public Q_SLOTS:
    void Update(int count);
};

class SANDBOX_API MainWindow
    : public QMainWindow
    , public IEditorNotifyListener
    , private AzToolsFramework::SourceControlNotificationBus::Handler
#ifdef Q_OS_WIN
    , public QAbstractNativeEventFilter
#endif
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

#ifdef Q_OS_WIN
    HWND GetNativeHandle();
#endif // #ifdef Q_OS_WIN

    ActionManager* GetActionManager() const;
    
    void Initialize();

    // Returns the old and original main frame which we're porting away from.
    // ActionManager still needs it, to SendMessage() to it.
    CMainFrame* GetOldMainFrame() const;

    bool IsPreview() const;
    int SelectRollUpBar(int rollupBarId);
    QRollupCtrl* GetRollUpControl(int rollupBarId = ROLLUP_OBJECTS);

    // The singleton is just a hack for now, it should be removed once everything
    // is ported to Qt.
    static MainWindow* instance();

    MainStatusBar* StatusBar() const;
    CLayoutWnd* GetLayout() const;

    KeyboardCustomizationSettings* GetShortcutManager() const;
    ToolbarManager* GetToolbarManager() const;

    void OpenViewPane(int paneId);
    void OpenViewPane(QtViewPane* pane);

    void SetActiveView(CLayoutViewPane* vp);

    QMenu* createPopupMenu() override;
    bool IsCustomizingToolbars() const;

    /**
     * Returns the active view layout (Perspective, Top, Bottom, or Left, etc).
     * This particularly useful when in multi-layout mode, it represents the default viewport to use
     * when needing to interact with one.
     * When the user gives mouse focus to a viewport it becomes the active one, when unfocusing it
     * however, it remains the active one, unless another viewport got focus.
     */
    CLayoutViewPane* GetActiveView() const;
    QtViewport* GetActiveViewport() const;

    void AdjustToolBarIconSize();
    void InvalidateControls();
    void OnCustomizeToolbar();
    void SaveConfig();
    void RefreshStyle();

    //! Reset timers used for auto saving.
    void ResetAutoSaveTimers(bool bForceInit = false);
    void ResetBackgroundUpdateTimer();

    void UpdateToolsMenu();

    int ViewPaneVersion() const;
    void MatEditSend(int param);

    LevelEditorMenuHandler* GetLevelEditorMenuHandler() { return m_levelEditorMenuHandler; }

#ifdef Q_OS_WIN
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;
#endif
    bool event(QEvent* event) override;

Q_SIGNALS:
    void ToggleRefCoordSys();
    void UpdateRefCoordSys();
    void DeleteSelection();

protected:
    void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* e) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent *event) override;

    bool focusNextPrevChild(bool next) override;

private:
    void OnGameModeChanged(bool inGameMode);
    QWidget* CreateToolbarWidget(int id);
    void ShowCustomizeToolbarDialog();
    void OnGotoSelected();
    void ToggleConsole();
    void ToggleRollupBar();
    void RegisterOpenWndCommands();
    void InitCentralWidget();
    void InitActions();
    void InitToolActionHandlers();
    void InitToolBars();
    void InitStatusBar();
    void OnUpdateSnapToGrid(QAction* action);
    void OnViewPaneCreated(const QtViewPane* pane);

    // AzToolsFramework::SourceControlNotificationBus::Handler:
    void ConnectivityStateChanged(const AzToolsFramework::SourceControlState state) override;

    QToolButton* CreateLayerSelectButton();
    QToolButton* CreateSnapToGridButton();
    QToolButton* CreateSnapToAngleButton();

    QComboBox* CreateSelectionMaskComboBox();
    QComboBox* CreateRefCoordComboBox();
    QWidget* CreateSelectObjectComboBox();

    QToolButton* CreateUndoRedoButton(int command);

private Q_SLOTS:
    void ShowKeyboardCustomization();
    void ExportKeyboardShortcuts();
    void ImportKeyboardShortcuts();
    void OnStopAllSounds();
    void OnRefreshAudioSystem();
    void SaveLayout();
    void ViewDeletePaneLayout(const QString& layoutName);
    void ViewRenamePaneLayout(const QString& layoutName);
    void ViewLoadPaneLayout(const QString& layoutName);
    void ViewSavePaneLayout(const QString& layoutName);
    void OnConnectionStatusClicked();
    void OnUpdateConnectionStatus();
    void ShowConnectionDisconnectedDialog();
	void CGPMenuClicked();
    void OnEscapeAction();

    // When signal is sent from ActionManager and MainWindow receives it, call this function as slot to send metrics event
    void SendMetricsEvent(const char* viewPaneName, const char* openLocation);

    void OnOpenAssetImporterManager(const QStringList& list);

private:
    bool IsGemEnabled(const QString& uuid, const QString& version) const;

    // Broadcast the SystemTick event
    void SystemTick();

    QStringList coordSysList() const;
    void RegisterStdViewClasses();
    CMainFrame* m_oldMainFrame;
    QtViewPaneManager* m_viewPaneManager;
    ActionManager* m_actionManager;
    UndoStackStateAdapter* m_undoStateAdapter;

    KeyboardCustomizationSettings* m_keyboardCustomization;
    CLayoutViewPane* m_activeView;
    QSettings m_settings;
    ToolbarManager* const m_toolbarManager;

    AssetImporterManager* m_assetImporterManager;
    LevelEditorMenuHandler* m_levelEditorMenuHandler;

    DayCountManager* m_dayCountManager;
    NetPromoterScoreDialog* m_NetPromoterScoreDialog;

    CLayoutWnd* m_pLayoutWnd;

    AZStd::shared_ptr<EngineConnectionListener> m_connectionListener;
    QTimer* m_connectionLostTimer;

    QPointer<ToolbarCustomizationDialog> m_toolbarCustomizationDialog;
    QScopedPointer<AzToolsFramework::QtSourceControlNotificationHandler> m_sourceControlNotifHandler;

    static MainWindow* m_instance;

    bool m_enableLegacyCryEntities;

    QMainWindow* m_viewPaneHost;

    QTimer* m_autoSaveTimer;
    QTimer* m_autoRemindTimer;
    QTimer* m_backgroundUpdateTimer;

    bool m_connectedToAssetProcessor = false;
    bool m_showAPDisconnectDialog = false;
    bool m_projectExternal = false;

    friend class ToolbarManager;
    friend class WidgetAction;
    friend class LevelEditorMenuHandler;
};

