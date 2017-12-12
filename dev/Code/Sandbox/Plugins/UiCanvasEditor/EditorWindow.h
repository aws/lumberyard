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

#include "Animation/UiEditorAnimationBus.h"
#include "UiEditorDLLBus.h"
#include "UiEditorEntityContext.h"
#include <QList>
#include <QMetaObject>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <LyShine/Bus/UiEditorChangeNotificationBus.h>

class AssetTreeEntry;

class EditorWindow
    : public QMainWindow
    , public IEditorNotifyListener
    , public UiEditorDLLBus::Handler
    , public UiEditorChangeNotificationBus::Handler
    , public AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationsBus::Handler
{
    Q_OBJECT

public:

    explicit EditorWindow(EditorWrapper* parentWrapper,
        const QString& canvasFilename,
        QWidget* parent,
        Qt::WindowFlags flags);
    virtual ~EditorWindow();

    void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;

    // UiEditorDLLInterface
    LyShine::EntityArray GetSelectedElements() override;
    AZ::EntityId GetActiveCanvasId() override;
    UndoStack* GetActiveUndoStack() override;
    // ~UiEditorDLLInterface

    // UiEditorChangeNotificationBus
    void OnEditorTransformPropertiesNeedRefresh() override;
    // ~UiEditorChangeNotificationBus

    // AssetBrowserModelNotificationsBus
    void EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
    void EntryRemoved(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
    // ~AssetBrowserModelNotificationsBus

    AZ::EntityId GetCanvas();
    string GetCanvasSourcePathname();

    EditorWrapper* GetEditorWrapper();
    HierarchyWidget* GetHierarchy();
    ViewportWidget* GetViewport();
    PropertiesWidget* GetProperties();
    MainToolbar* GetMainToolbar();
    ModeToolbar* GetModeToolbar();
    PreviewToolbar* GetPreviewToolbar();
    NewElementToolbarSection* GetNewElementToolbarSection();
    CoordinateSystemToolbarSection* GetCoordinateSystemToolbarSection();
    CanvasSizeToolbarSection* GetCanvasSizeToolbarSection();

    bool CanExitNow();

    void DestroyCanvas();

    //! Return true when ok.
    //! forceAskingForFilename should only be true for "Save As...", not "Save".
    bool SaveCanvasToXml(bool forceAskingForFilename);

    bool GetChangesHaveBeenMade();

    bool GetDestroyCanvasWasCausedByRestart();
    void SetDestroyCanvasWasCausedByRestart(bool d);

    QUndoGroup* GetUndoGroup();
    UndoStack* GetActiveStack();

    AssetTreeEntry* GetSliceLibraryTree();

    //! WARNING: This is a VERY slow function.
    void UpdatePrefabFiles();
    IFileUtil::FileArray& GetPrefabFiles();
    void AddPrefabFile(const QString& prefabFilename);

    void RefreshEditorMenu(EditorWrapper* parentWrapper);

    //! Returns the current mode of the editor (Edit or Preview)
    UiEditorMode GetEditorMode() { return m_editorMode; }

    //! Toggle the editor mode between Edit and Preview
    void ToggleEditorMode();

    //! Get the copy of the canvas that is used in Preview mode (will return invalid entity ID if not in preview mode)
    AZ::EntityId GetPreviewModeCanvas() { return m_previewModeCanvasEntityId; }

    //! Get the preview canvas size.  (0,0) means use viewport size
    AZ::Vector2 GetPreviewCanvasSize();

    //! Set the preview canvas size. (0,0) means use viewport size
    void SetPreviewCanvasSize(AZ::Vector2 previewCanvasSize);

    //! Check if the given toolbar should only be shown in preview mode
    bool IsPreviewModeToolbar(const QToolBar* toolBar);

    //! Check if the given dockwidget should only be shown in preview mode
    bool IsPreviewModeDockWidget(const QDockWidget* dockWidget);

    void SaveEditorWindowSettings();

    UiSliceManager* GetSliceManager() { return m_sliceManager.get(); }

    UiEditorEntityContext* GetEntityContext() { return m_entityContext.get(); }

    void ReplaceEntityContext(UiEditorEntityContext* entityContext);

    QMenu* createPopupMenu() override;

signals:

    void EditorModeChanged(UiEditorMode mode);
    void SignalCoordinateSystemCycle();
    void SignalSnapToGridToggle();

protected:

    bool event(QEvent* ev) override;
    void keyReleaseEvent(QKeyEvent* ev) override;

public slots:
    void RestoreEditorWindowSettings();

private:

    void UpdateActionsEnabledState();

    void AddMenu_File(EditorWrapper* parentWrapper);
    void AddMenuItems_Edit(QMenu* menu);
    void AddMenu_Edit();
    void AddMenu_View(EditorWrapper* parentWrapper);
    void AddMenu_View_LanguageSetting(QMenu* viewMenu);
    void AddMenu_Preview();
    void AddMenu_PreviewView();
    void AddMenu_Help();
    void EditorMenu_Open(QString optional_selectedFile, EditorWrapper* parentWrapper);

    void SortPrefabsList();

    void SaveModeSettings(UiEditorMode mode, bool syncSettings);
    void RestoreModeSettings(UiEditorMode mode);

    void SubmitUnloadSavedCanvasMetricEvent();
    int GetCanvasMaxHierarchyDepth(const LyShine::EntityArray& childElements);

    void DeleteSliceLibraryTree();

    EditorWrapper* m_editorWrapper;

    AZ::EntityId m_canvasEntityId;
    string m_canvasSourceAssetPathname;
    QUndoGroup* m_undoGroup;
    std::unique_ptr< UndoStack > m_undoStack;

    HierarchyWidget* m_hierarchy;
    PropertiesWrapper* m_properties;
    ViewportWidget* m_viewport;
    CUiAnimViewDialog* m_animationWidget;
    PreviewActionLog* m_previewActionLog;
    PreviewAnimationList* m_previewAnimationList;

    MainToolbar* m_mainToolbar;
    ModeToolbar* m_modeToolbar;
    EnterPreviewToolbar* m_enterPreviewToolbar;
    PreviewToolbar* m_previewToolbar;

    QDockWidget* m_hierarchyDockWidget;
    QDockWidget* m_propertiesDockWidget;
    QDockWidget* m_animationDockWidget;
    QDockWidget* m_previewActionLogDockWidget;
    QDockWidget* m_previewAnimationListDockWidget;

    bool m_destroyCanvasWasCausedByRestart;

    UiEditorMode m_editorMode;

    //! This is used to determine if changes have been made to m_canvas since its last
    //! save, or if not saved yet, since it was loaded/created.
    AZStd::string m_originalCanvasXml;

    //! Specifies whether a canvas has been modified and saved since it was loaded/created
    bool m_canvasChangedAndSaved;

    //! This tree caches the folder view of all the slice assets under the slice library path
    AssetTreeEntry* m_sliceLibraryTree = nullptr;

    IFileUtil::FileArray m_prefabFiles;

    //! This is used to change the enabled state
    //! of these actions as the selection changes.
    QList<QAction*> m_actionsEnabledWithSelection;
    QAction* m_pasteAsSiblingAction;
    QAction* m_pasteAsChildAction;

    AZ::EntityId m_previewModeCanvasEntityId;

    AZ::Vector2 m_previewModeCanvasSize;

    QMetaObject::Connection m_clipboardConnection;

    //! Local copy of QSetting value of startup location of localization folder
    QString m_startupLocFolderName;

    AZStd::unique_ptr<UiEditorEntityContext> m_entityContext;
    AZStd::unique_ptr<UiSliceManager> m_sliceManager;
};
