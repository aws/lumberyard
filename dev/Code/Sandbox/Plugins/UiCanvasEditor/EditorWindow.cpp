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
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/sort.h>
#include <Util/PathUtil.h>
#include <LyMetricsProducer/LyMetricsAPI.h>
#include "EditorDefs.h"
#include "Settings.h"
#include "AnchorPresets.h"
#include "PivotPresets.h"
#include "Animation/UiAnimViewDialog.h"
#include <AzQtComponents/Components/StyledDockWidget.h>
#include "AssetTreeEntry.h"
#include <LyShine/UiComponentTypes.h>

#define UICANVASEDITOR_SETTINGS_EDIT_MODE_STATE_KEY     (QString("Edit Mode State") + " " + FileHelpers::GetAbsoluteGameDir())
#define UICANVASEDITOR_SETTINGS_EDIT_MODE_GEOM_KEY      (QString("Edit Mode Geometry") + " " + FileHelpers::GetAbsoluteGameDir())
#define UICANVASEDITOR_SETTINGS_PREVIEW_MODE_STATE_KEY  (QString("Preview Mode State") + " " + FileHelpers::GetAbsoluteGameDir())
#define UICANVASEDITOR_SETTINGS_PREVIEW_MODE_GEOM_KEY   (QString("Preview Mode Geometry") + " " + FileHelpers::GetAbsoluteGameDir())
#define UICANVASEDITOR_SETTINGS_WINDOW_STATE_VERSION    (1)

EditorWindow::EditorWindow(EditorWrapper* parentWrapper,
    const QString& canvasFilename,
    QWidget* parent,
    Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , IEditorNotifyListener()
    , m_editorWrapper(parentWrapper)
    , m_canvasEntityId()
    , m_canvasSourceAssetPathname()
    , m_undoGroup(new QUndoGroup(this))
    , m_undoStack(new UndoStack(m_undoGroup))
    , m_hierarchy(new HierarchyWidget(this))
    , m_properties(new PropertiesWrapper(m_hierarchy, this))
    , m_viewport(new ViewportWidget(this))
    , m_animationWidget(new CUiAnimViewDialog(this))
    , m_previewActionLog(new PreviewActionLog(this))
    , m_previewAnimationList(new PreviewAnimationList(this))
    , m_mainToolbar(new MainToolbar(this))
    , m_modeToolbar(new ModeToolbar(this))
    , m_enterPreviewToolbar(new EnterPreviewToolbar(this))
    , m_previewToolbar(new PreviewToolbar(this))
    , m_hierarchyDockWidget(nullptr)
    , m_propertiesDockWidget(nullptr)
    , m_animationDockWidget(nullptr)
    , m_previewActionLogDockWidget(nullptr)
    , m_previewAnimationListDockWidget(nullptr)
    , m_destroyCanvasWasCausedByRestart(false)
    , m_editorMode(UiEditorMode::Edit)
    , m_originalCanvasXml()
    , m_canvasChangedAndSaved(false)
    , m_prefabFiles()
    , m_actionsEnabledWithSelection()
    , m_pasteAsSiblingAction(nullptr)
    , m_pasteAsChildAction(nullptr)
    , m_previewModeCanvasEntityId()
    , m_previewModeCanvasSize(0.0f, 0.0f)
    , m_clipboardConnection()
{
    // Store local copy of startup localization value
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);
    settings.beginGroup(UICANVASEDITOR_NAME_SHORT);
    m_startupLocFolderName = settings.value(UICANVASEDITOR_SETTINGS_STARTUP_LOC_FOLDER_KEY).toString();
    settings.endGroup();

    m_undoStack->setActive(true);

    m_viewport->GetViewportInteraction()->UpdateZoomFactorLabel();

    // update menus when the selection changes
    connect(m_hierarchy, &HierarchyWidget::SetUserSelection, [this](HierarchyItemRawPtrList*)
        {
            UpdateActionsEnabledState();
        });
    m_clipboardConnection = connect(QApplication::clipboard(), &QClipboard::dataChanged, [this]()
            {
                UpdateActionsEnabledState();
            });

    UpdatePrefabFiles();

    // Signal: Hierarchical tree -> Properties pane.
    QObject::connect(m_hierarchy,
        SIGNAL(SetUserSelection(HierarchyItemRawPtrList*)),
        m_properties->GetProperties(),
        SLOT(UserSelectionChanged(HierarchyItemRawPtrList*)));

    // Signal: Hierarchical tree -> Viewport pane.
    QObject::connect(m_hierarchy,
        SIGNAL(SetUserSelection(HierarchyItemRawPtrList*)),
        GetViewport(),
        SLOT(UserSelectionChanged(HierarchyItemRawPtrList*)));

    m_viewport->setFocusPolicy(Qt::StrongFocus);
    setCentralWidget(m_viewport);

    m_entityContext.reset(new UiEditorEntityContext(this));

    // Load the canvas.
    //
    // IMPORTANT: This MUST be done BEFORE RefreshEditorMenu().
    // That's because the menu needs to know the filename
    // loaded into the canvas.
    if (canvasFilename.isEmpty())
    {
        m_canvasEntityId = gEnv->pLyShine->CreateCanvasInEditor(m_entityContext.get());
    }
    else
    {
        string sourceAssetPathName = canvasFilename.toLatin1().data();
        string assetIdPathname = Path::FullPathToGamePath(sourceAssetPathName.c_str());
        m_canvasEntityId = gEnv->pLyShine->LoadCanvasInEditor(assetIdPathname, sourceAssetPathName, m_entityContext.get());
        if (m_canvasEntityId.IsValid())
        {
            AddRecentFile(canvasFilename);
            m_canvasSourceAssetPathname = sourceAssetPathName;

            // create the hierarchy tree from the loaded canvas
            LyShine::EntityArray childElements;
            EBUS_EVENT_ID_RESULT(childElements, m_canvasEntityId, UiCanvasBus, GetChildElements);
            m_hierarchy->CreateItems(childElements);
            if (!childElements.empty())
            {
                m_hierarchy->SetUniqueSelectionHighlight(childElements[0]);
            }

            // restore the expanded state of all items
            m_hierarchy->ApplyElementIsExpanded();
        }
        else
        {
            // there was an error loading the file. Report an error and create a new blank canvas
            QMessageBox::critical(parent, tr("Error"), tr("Failed to load requested UI Canvas file. See log for details"));

            m_canvasEntityId = gEnv->pLyShine->CreateCanvasInEditor(m_entityContext.get());
        }
    }

    m_originalCanvasXml = HierarchyClipboard::GetXmlForDiff(m_canvasEntityId);

    // Create the slice manager
    m_sliceManager.reset(new UiSliceManager(m_entityContext->GetContextId()));

    // by default the BottomDockWidgetArea will be the full width of the main window
    // and will make the Hierarchy and Properties panes less tall. This makes the
    // Hierarchy and Properties panes occupy the corners and makes the animation pane less wide.
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // Hierarchy pane.
    {
        m_hierarchyDockWidget = new AzQtComponents::StyledDockWidget("Hierarchy");
        m_hierarchyDockWidget->setObjectName("HierarchyDockWidget");    // needed to save state
        m_hierarchyDockWidget->setWidget(m_hierarchy);
        // needed to get keyboard shortcuts properly
        m_hierarchy->setFocusPolicy(Qt::StrongFocus);
        addDockWidget(Qt::LeftDockWidgetArea, m_hierarchyDockWidget, Qt::Vertical);
    }

    // Properties pane.
    {
        m_propertiesDockWidget = new AzQtComponents::StyledDockWidget("Properties");
        m_propertiesDockWidget->setObjectName("PropertiesDockWidget");    // needed to save state
        m_propertiesDockWidget->setWidget(m_properties);
        m_properties->setFocusPolicy(Qt::StrongFocus);
        addDockWidget(Qt::RightDockWidgetArea, m_propertiesDockWidget, Qt::Vertical);
    }

    // Animation pane.
    {
        m_animationDockWidget = new AzQtComponents::StyledDockWidget("Animation Editor");
        m_animationDockWidget->setObjectName("AnimationDockWidget");    // needed to save state
        m_animationDockWidget->setWidget(m_animationWidget);
        m_animationWidget->setFocusPolicy(Qt::StrongFocus);
        addDockWidget(Qt::BottomDockWidgetArea, m_animationDockWidget, Qt::Horizontal);
    }

    // Preview action log pane (only shown in preview mode)
    {
        m_previewActionLogDockWidget = new AzQtComponents::StyledDockWidget("Action Log");
        m_previewActionLogDockWidget->setObjectName("PreviewActionLog");    // needed to save state
        m_previewActionLogDockWidget->setWidget(m_previewActionLog);
        m_previewActionLog->setFocusPolicy(Qt::StrongFocus);
        addDockWidget(Qt::BottomDockWidgetArea, m_previewActionLogDockWidget, Qt::Horizontal);
    }

    // Preview animation list pane (only shown in preview mode)
    {
        m_previewAnimationListDockWidget = new AzQtComponents::StyledDockWidget("Animation List");
        m_previewAnimationListDockWidget->setObjectName("PreviewAnimationList");    // needed to save state
        m_previewAnimationListDockWidget->setWidget(m_previewAnimationList);
        m_previewAnimationList->setFocusPolicy(Qt::StrongFocus);
        addDockWidget(Qt::LeftDockWidgetArea, m_previewAnimationListDockWidget, Qt::Vertical);
    }

    // We start out in edit mode so hide the preview mode widgets
    m_previewActionLogDockWidget->hide();
    m_previewAnimationListDockWidget->hide();
    m_previewToolbar->hide();

    // Main menu.
    //
    // IMPORTANT: This MUST be done AFTER we load the canvas, AND ALSO after
    // adding all the QDockWidget. That's because the menu needs to know the
    // filename loaded into the canvas, AND all the existing QDockWidget for
    // the "View" menu.
    RefreshEditorMenu(m_editorWrapper);

    GetIEditor()->RegisterNotifyListener(this);

    m_viewport->GetViewportInteraction()->InitializeToolbars();

    // Start listening for any queries on the UiEditorDLLBus
    UiEditorDLLBus::Handler::BusConnect();

    // Start listening for any queries on the UiEditorChangeNotificationBus
    UiEditorChangeNotificationBus::Handler::BusConnect();

    // Tell the UI animation system that the active canvas has changed
    EBUS_EVENT(UiEditorAnimationBus, CanvasLoaded);

    // disable rendering of the editor window until we have restored the window state
    setVisible(false);

    // Show the canvas properties.
    m_hierarchy->clearSelection();
    m_properties->GetProperties()->UserSelectionChanged(nullptr);

    AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationsBus::Handler::BusConnect();
}

EditorWindow::~EditorWindow()
{
    AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationsBus::Handler::BusDisconnect();

    QObject::disconnect(m_clipboardConnection);

    GetIEditor()->UnregisterNotifyListener(this);

    UiEditorDLLBus::Handler::BusDisconnect();
    UiEditorChangeNotificationBus::Handler::BusDisconnect();

    DestroyCanvas();

    // unload the preview mode canvas if it exists (e.g. if we close the editor window while in preview mode)
    if (m_previewModeCanvasEntityId.IsValid())
    {
        gEnv->pLyShine->ReleaseCanvas(m_previewModeCanvasEntityId, false);
    }

    delete m_sliceLibraryTree;
}

LyShine::EntityArray EditorWindow::GetSelectedElements()
{
    LyShine::EntityArray elements = SelectionHelpers::GetSelectedElements(
            m_hierarchy,
            m_hierarchy->selectedItems());

    return elements;
}

AZ::EntityId EditorWindow::GetActiveCanvasId()
{
    return m_canvasEntityId;
}

UndoStack* EditorWindow::GetActiveUndoStack()
{
    return GetActiveStack();
}

void EditorWindow::OnEditorTransformPropertiesNeedRefresh()
{
    AZ::Uuid transformComponentUuid = LyShine::UiTransform2dComponentUuid;
    GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues, &transformComponentUuid);
}

void EditorWindow::EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* /*entry*/)
{
    DeleteSliceLibraryTree();
}

void EditorWindow::EntryRemoved(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* /*entry*/)
{
    DeleteSliceLibraryTree();
}

void EditorWindow::DestroyCanvas()
{
    if (m_destroyCanvasWasCausedByRestart)
    {
        // IMPORTANT: We trigger EditorWrapper::Restart() after
        // LoadFromXml(). We therefore DON'T want to ReleaseCanvas()
        // the m_canvas we JUST created.
        //
        // Nothing to do.
    }
    else
    {
        // Submit metrics for a canvas that has changed since it was last
        // loaded/created, and all changes have been saved.
        if (m_canvasChangedAndSaved && !GetChangesHaveBeenMade())
        {
            SubmitUnloadSavedCanvasMetricEvent();
        }

        // if we are running in-game ReleaseCanvas will not necessary unload the canvas
        // we need to avoid the destructor for the hierarchyWidget deleting all the element Entities
        // in this case. So clear the m_entityIds for all the widgets.
        // If this canvas is NOT already loaded in game then ReleaseCanvas will delete all the
        // element entities anyway, so this avoids the double attempt at deleting.
        GetHierarchy()->ClearAllHierarchyItemEntityIds();

        // tell the UI animation system the canvas is unloading
        EBUS_EVENT(UiEditorAnimationBus, CanvasUnloading);

        gEnv->pLyShine->ReleaseCanvas(m_canvasEntityId, true);
        m_canvasEntityId.SetInvalid();
    }
}

bool EditorWindow::SaveCanvasToXml(bool forceAskingForFilename)
{
    // Default the pathname to where the current canvas was loaded from or last saved to
    QString filename = m_canvasSourceAssetPathname;

    if (!forceAskingForFilename)
    {
        // Before saving, make sure the file contains an extension we're expecting
        if ((!filename.isEmpty()) &&
            (!FileHelpers::FilenameHasExtension(filename, UICANVASEDITOR_CANVAS_EXTENSION)))
        {
            QMessageBox::warning(this, tr("Warning"), tr("Please save with the expected extension: *.%1").arg(UICANVASEDITOR_CANVAS_EXTENSION));
            forceAskingForFilename = true;
        }
    }

    if (filename.isEmpty() || forceAskingForFilename)
    {
        QString dir;
        QStringList recentFiles = ReadRecentFiles();

        // If the canvas we are saving already has a name
        if (!filename.isEmpty())
        {
            // Default to where it was loaded from or last saved to
            // Also notice that we directly assign dir to the filename
            // This allows us to have its existing name already entered in
            // the File Name field.
            dir = filename;
        }
        // Else if we had recently opened canvases, open the most recent one's directory
        else if (recentFiles.size() > 0)
        {
            dir = Path::GetPath(recentFiles.front());
        }
        // Else go to the default canvas directory
        else
        {
            dir = FileHelpers::GetAbsoluteDir(UICANVASEDITOR_CANVAS_DIRECTORY);
        }

        filename = QFileDialog::getSaveFileName(nullptr,
                QString(),
                dir,
                "*." UICANVASEDITOR_CANVAS_EXTENSION,
                nullptr,
                QFileDialog::DontConfirmOverwrite);
        if (filename.isEmpty())
        {
            return false;
        }
    }

    FileHelpers::AppendExtensionIfNotPresent(filename, UICANVASEDITOR_CANVAS_EXTENSION);

    string sourceAssetPathName = filename.toStdString().c_str(); // Full path.
    string assetIdPathname = Path::FullPathToGamePath(sourceAssetPathName.c_str()); // Relative path.

    FileHelpers::SourceControlAddOrEdit(filename.toStdString().c_str(), this);

    bool saveSuccessful = false;
    EBUS_EVENT_ID_RESULT(saveSuccessful, m_canvasEntityId, UiCanvasBus, SaveToXml,
        assetIdPathname, sourceAssetPathName);

    if (saveSuccessful)
    {
        AddRecentFile(filename);

        if (!m_canvasChangedAndSaved)
        {
            m_canvasChangedAndSaved = GetChangesHaveBeenMade();
        }
        m_originalCanvasXml = HierarchyClipboard::GetXmlForDiff(m_canvasEntityId);
        m_canvasSourceAssetPathname = sourceAssetPathName;

        return true;
    }

    QMessageBox(QMessageBox::Critical,
        "Error",
        "Unable to save file. Is the file read-only?",
        QMessageBox::Ok, this).exec();

    return false;
}

AZ::EntityId EditorWindow::GetCanvas()
{
    return m_canvasEntityId;
}

string EditorWindow::GetCanvasSourcePathname()
{
    return m_canvasSourceAssetPathname;
}

EditorWrapper* EditorWindow::GetEditorWrapper()
{
    return m_editorWrapper;
}

HierarchyWidget* EditorWindow::GetHierarchy()
{
    AZ_Assert(m_hierarchy, "Missing hierarchy widget");
    return m_hierarchy;
}

ViewportWidget* EditorWindow::GetViewport()
{
    AZ_Assert(m_viewport, "Missing viewport widget");
    return m_viewport;
}

PropertiesWidget* EditorWindow::GetProperties()
{
    AZ_Assert(m_properties, "Missing properties wrapper");
    AZ_Assert(m_properties->GetProperties(), "Missing properties widget");
    return m_properties->GetProperties();
}

MainToolbar* EditorWindow::GetMainToolbar()
{
    AZ_Assert(m_mainToolbar, "Missing main toolbar");
    return m_mainToolbar;
}

ModeToolbar* EditorWindow::GetModeToolbar()
{
    AZ_Assert(m_modeToolbar, "Missing mode toolbar");
    return m_modeToolbar;
}

PreviewToolbar* EditorWindow::GetPreviewToolbar()
{
    AZ_Assert(m_previewToolbar, "Missing preview toolbar");
    return m_previewToolbar;
}

NewElementToolbarSection* EditorWindow::GetNewElementToolbarSection()
{
    AZ_Assert(m_mainToolbar, "Missing main toolbar");
    return m_mainToolbar->GetNewElementToolbarSection();
}

CoordinateSystemToolbarSection* EditorWindow::GetCoordinateSystemToolbarSection()
{
    AZ_Assert(m_mainToolbar, "Missing main toolbar");
    return m_mainToolbar->GetCoordinateSystemToolbarSection();
}

CanvasSizeToolbarSection* EditorWindow::GetCanvasSizeToolbarSection()
{
    AZ_Assert(m_mainToolbar, "Missing main toolbar");
    return m_mainToolbar->GetCanvasSizeToolbarSection();
}

void EditorWindow::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
    switch (ev)
    {
    case eNotify_OnIdleUpdate:
        m_viewport->Refresh();
        break;
    case eNotify_OnStyleChanged:
    {
        // change skin
        RefreshEditorMenu(m_editorWrapper);
        m_viewport->UpdateViewportBackground();
        break;
    }
    case eNotify_OnUpdateViewports:
    {
        // provides a way for the animation editor to force updates of the properties dialog during
        // an animation
        GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values);
        break;
    }
    }
}

bool EditorWindow::CanExitNow()
{
    if (GetChangesHaveBeenMade())
    {
        const auto defaultButton = QMessageBox::Cancel;
        int result = QMessageBox::question(this,
                tr("Changes have been made"),
                tr("Save changes to the UI canvas?"),
                (QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel),
                defaultButton);

        if (result == QMessageBox::Save)
        {
            bool ok = SaveCanvasToXml(false);
            if (!ok)
            {
                return false;
            }
        }
        else if (result == QMessageBox::Discard)
        {
            // Nothing to do
        }
        else // if( result == QMessageBox::Cancel )
        {
            return false;
        }
    }

    return true;
}

bool EditorWindow::GetChangesHaveBeenMade()
{
    AZStd::string canvasXml = HierarchyClipboard::GetXmlForDiff(m_canvasEntityId);

    return (!!canvasXml.compare(m_originalCanvasXml));
}

bool EditorWindow::GetDestroyCanvasWasCausedByRestart()
{
    return m_destroyCanvasWasCausedByRestart;
}

void EditorWindow::SetDestroyCanvasWasCausedByRestart(bool d)
{
    m_destroyCanvasWasCausedByRestart = d;
}

QUndoGroup* EditorWindow::GetUndoGroup()
{
    return m_undoGroup;
}

UndoStack* EditorWindow::GetActiveStack()
{
    return qobject_cast< UndoStack* >(m_undoGroup->activeStack());
}

AssetTreeEntry* EditorWindow::GetSliceLibraryTree()
{
    if (!m_sliceLibraryTree)
    {
        const AZStd::string pathToSearch("ui/slices/library/");
        const AZ::Data::AssetType sliceAssetType(AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());

        m_sliceLibraryTree = AssetTreeEntry::BuildAssetTree(sliceAssetType, pathToSearch);
    }

    return m_sliceLibraryTree;
}

void EditorWindow::UpdatePrefabFiles()
{
    m_prefabFiles.clear();

    // IMPORTANT: ScanDirectory() is VERY slow. It can easily take as much
    // as a whole second to execute. That's why we want to cache its result
    // up front and ONLY access the cached data.
    GetIEditor()->GetFileUtil()->ScanDirectory("", "*." UICANVASEDITOR_PREFAB_EXTENSION, m_prefabFiles);
    SortPrefabsList();
}

IFileUtil::FileArray& EditorWindow::GetPrefabFiles()
{
    return m_prefabFiles;
}

void EditorWindow::AddPrefabFile(const QString& prefabFilename)
{
    IFileUtil::FileDesc fd;
    fd.filename = prefabFilename;
    m_prefabFiles.push_back(fd);
    SortPrefabsList();
}

void EditorWindow::SortPrefabsList()
{
    AZStd::sort<IFileUtil::FileArray::iterator>(m_prefabFiles.begin(), m_prefabFiles.end(),
        [](const IFileUtil::FileDesc& fd1, const IFileUtil::FileDesc& fd2)
        {
            // Some of the files in the list are in different directories, so we
            // explicitly sort by filename only.
            AZStd::string fd1Filename;
            AzFramework::StringFunc::Path::GetFileName(fd1.filename.toLatin1().data(), fd1Filename);

            AZStd::string fd2Filename;
            AzFramework::StringFunc::Path::GetFileName(fd2.filename.toLatin1().data(), fd2Filename);
            return fd1Filename < fd2Filename;
        });
}

void EditorWindow::ToggleEditorMode()
{
    m_editorMode = (m_editorMode == UiEditorMode::Edit) ? UiEditorMode::Preview : UiEditorMode::Edit;

    emit EditorModeChanged(m_editorMode);

    m_viewport->ClearUntilSafeToRedraw();

    if (m_editorMode == UiEditorMode::Edit)
    {
        // unload the preview mode canvas
        if (m_previewModeCanvasEntityId.IsValid())
        {
            m_previewActionLog->Deactivate();
            m_previewAnimationList->Deactivate();

            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, m_previewModeCanvasEntityId);
            if (entity)
            {
                gEnv->pLyShine->ReleaseCanvas(m_previewModeCanvasEntityId, false);
            }
            m_previewModeCanvasEntityId.SetInvalid();
        }

        SaveModeSettings(UiEditorMode::Preview, false);
        RestoreModeSettings(UiEditorMode::Edit);
    }
    else
    {
        SaveModeSettings(UiEditorMode::Edit, false);
        RestoreModeSettings(UiEditorMode::Preview);

        GetPreviewToolbar()->UpdatePreviewCanvasScale(m_viewport->GetPreviewCanvasScale());

        // clone the editor canvas to create a temporary preview mode canvas
        if (m_canvasEntityId.IsValid())
        {
            AZ_Assert(!m_previewModeCanvasEntityId.IsValid(), "There is an existing preview mode canvas");

            // Get the canvas size
            AZ::Vector2 canvasSize = GetPreviewCanvasSize();
            if (canvasSize.GetX() == 0.0f && canvasSize.GetY() == 0.0f)
            {
                // special value of (0,0) means use the viewport size
                canvasSize = AZ::Vector2(m_viewport->size().width(), m_viewport->size().height());
            }

            AZ::Entity* clonedCanvas = nullptr;
            EBUS_EVENT_ID_RESULT(clonedCanvas, m_canvasEntityId, UiCanvasBus, CloneCanvas, canvasSize);

            if (clonedCanvas)
            {
                m_previewModeCanvasEntityId = clonedCanvas->GetId();
            }
        }

        m_previewActionLog->Activate(m_previewModeCanvasEntityId);

        m_previewAnimationList->Activate(m_previewModeCanvasEntityId);

        // In Preview mode we want keyboard input to go to to the ViewportWidget so set the
        // it to be focused
        m_viewport->setFocus();
    }

    // Update the menus for this mode
    RefreshEditorMenu(m_editorWrapper);
}

AZ::Vector2 EditorWindow::GetPreviewCanvasSize()
{
    return m_previewModeCanvasSize;
}

void EditorWindow::SetPreviewCanvasSize(AZ::Vector2 previewCanvasSize)
{
    m_previewModeCanvasSize = previewCanvasSize;
}

bool EditorWindow::IsPreviewModeToolbar(const QToolBar* toolBar)
{
    bool result = false;
    if (toolBar == m_previewToolbar)
    {
        result = true;
    }
    return result;
}

bool EditorWindow::IsPreviewModeDockWidget(const QDockWidget* dockWidget)
{
    bool result = false;
    if (dockWidget == m_previewActionLogDockWidget ||
        dockWidget == m_previewAnimationListDockWidget)
    {
        result = true;
    }
    return result;
}

void EditorWindow::RestoreEditorWindowSettings()
{
    RestoreModeSettings(m_editorMode);

    // allow the editor window to draw now that we have restored state
    setVisible(true);
}

void EditorWindow::SaveEditorWindowSettings()
{
    // This prevents the canvas shifting in the viewport as we switch canvases or close the editor window
    m_viewport->ClearUntilSafeToRedraw();

    // This saves the dock position, size and visibility of all the dock widgets and tool bars
    // for the current mode (it also syncs the settings for the other mode that have already been saved to settings)
    SaveModeSettings(m_editorMode, true);
}

void EditorWindow::ReplaceEntityContext(UiEditorEntityContext* entityContext)
{
    m_entityContext.reset(entityContext);
}

QMenu* EditorWindow::createPopupMenu()
{
    QMenu* menu = new QMenu(this);

    // Add all QDockWidget panes for the current editor mode
    {
        QList<QDockWidget*> list = findChildren<QDockWidget*>();

        for (auto p : list)
        {
            // findChildren is recursive, but we only want dock widgets that are immediate children
            if (p->parent() == this)
            {
                bool isPreviewModeDockWidget = IsPreviewModeDockWidget(p);
                if (m_editorMode == UiEditorMode::Edit && !isPreviewModeDockWidget ||
                    m_editorMode == UiEditorMode::Preview && isPreviewModeDockWidget)
                {
                    menu->addAction(p->toggleViewAction());
                }
            }
        }
    }

    // Add all QToolBar panes for the current editor mode
    {
        QList<QToolBar*> list = findChildren<QToolBar*>();
        for (auto p : list)
        {
            if (p->parent() == this)
            {
                bool isPreviewModeToolbar = IsPreviewModeToolbar(p);
                if (m_editorMode == UiEditorMode::Edit && !isPreviewModeToolbar ||
                    m_editorMode == UiEditorMode::Preview && isPreviewModeToolbar)
                {
                    menu->addAction(p->toggleViewAction());
                }
            }
        }
    }

    return menu;
}

void EditorWindow::SaveModeSettings(UiEditorMode mode, bool syncSettings)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);
    settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

    if (mode == UiEditorMode::Edit)
    {
        // save the edit mode state
        settings.setValue(UICANVASEDITOR_SETTINGS_EDIT_MODE_STATE_KEY, saveState(UICANVASEDITOR_SETTINGS_WINDOW_STATE_VERSION));
        settings.setValue(UICANVASEDITOR_SETTINGS_EDIT_MODE_GEOM_KEY, saveGeometry());
    }
    else
    {
        // save the preview mode state
        settings.setValue(UICANVASEDITOR_SETTINGS_PREVIEW_MODE_STATE_KEY, saveState(UICANVASEDITOR_SETTINGS_WINDOW_STATE_VERSION));
        settings.setValue(UICANVASEDITOR_SETTINGS_PREVIEW_MODE_GEOM_KEY, saveGeometry());
    }

    settings.endGroup();    // UI canvas editor

    if (syncSettings)
    {
        settings.sync();
    }
}

void EditorWindow::RestoreModeSettings(UiEditorMode mode)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);
    settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

    if (mode == UiEditorMode::Edit)
    {
        // restore the edit mode state
        restoreState(settings.value(UICANVASEDITOR_SETTINGS_EDIT_MODE_STATE_KEY).toByteArray(),  UICANVASEDITOR_SETTINGS_WINDOW_STATE_VERSION);
        restoreGeometry(settings.value(UICANVASEDITOR_SETTINGS_EDIT_MODE_GEOM_KEY).toByteArray());
    }
    else
    {
        // restore the preview mode state
        bool stateRestored = restoreState(settings.value(UICANVASEDITOR_SETTINGS_PREVIEW_MODE_STATE_KEY).toByteArray(),  UICANVASEDITOR_SETTINGS_WINDOW_STATE_VERSION);
        bool geomRestored = restoreGeometry(settings.value(UICANVASEDITOR_SETTINGS_PREVIEW_MODE_GEOM_KEY).toByteArray());

        // if either of the above failed then manually hide and show widgets,
        // this will happen the first time someone uses preview mode
        if (!stateRestored || !geomRestored)
        {
            m_hierarchyDockWidget->hide();
            m_propertiesDockWidget->hide();
            m_animationDockWidget->hide();
            m_mainToolbar->hide();
            m_modeToolbar->hide();
            m_enterPreviewToolbar->hide();

            m_previewToolbar->show();
            m_previewActionLogDockWidget->show();
            m_previewAnimationListDockWidget->show();
        }
    }

    settings.endGroup();    // UI canvas editor
}

static const char* UIEDITOR_UNLOAD_SAVED_CANVAS_METRIC_EVENT_NAME = "UiEditorUnloadSavedCanvas";
static const char* UIEDITOR_CANVAS_ID_ATTRIBUTE_NAME = "CanvasId";
static const char* UIEDITOR_CANVAS_WIDTH_METRIC_NAME = "CanvasWidth";
static const char* UIEDITOR_CANVAS_HEIGHT_METRIC_NAME = "CanvasHeight";
static const char* UIEDITOR_CANVAS_MAX_HIERARCHY_DEPTH_METRIC_NAME = "MaxHierarchyDepth";
static const char* UIEDITOR_CANVAS_NUM_ELEMENT_METRIC_NAME = "NumElement";
static const char* UIEDITOR_CANVAS_NUM_ELEMENTS_WITH_COMPONENT_PREFIX_METRIC_NAME = "Num";
static const char* UIEDITOR_CANVAS_NUM_ELEMENTS_WITH_CUSTOM_COMPONENT_METRIC_NAME = "NumCustomElement";
static const char* UIEDITOR_CANVAS_NUM_UNIQUE_CUSTOM_COMPONENT_NAME = "NumUniqueCustomComponent";
static const char* UIEDITOR_CANVAS_NUM_AVAILABLE_CUSTOM_COMPONENT_NAME = "NumAvailableCustomComponent";
static const char* UIEDITOR_CANVAS_NUM_ANCHOR_PRESETS_ATTRIBUTE_NAME = "NumAnchorPreset";
static const char* UIEDITOR_CANVAS_NUM_ANCHOR_CUSTOM_ATTRIBUTE_NAME = "NumAnchorCustom";
static const char* UIEDITOR_CANVAS_NUM_PIVOT_PRESETS_ATTRIBUTE_NAME = "NumPivotPreset";
static const char* UIEDITOR_CANVAS_NUM_PIVOT_CUSTOM_ATTRIBUTE_NAME = "NumPivotCustom";
static const char* UIEDITOR_CANVAS_NUM_ROTATED_ELEMENT_METRIC_NAME = "NumRotatedElement";
static const char* UIEDITOR_CANVAS_NUM_SCALED_ELEMENT_METRIC_NAME = "NumScaledElement";
static const char* UIEDITOR_CANVAS_NUM_SCALE_TO_DEVICE_ELEMENT_METRIC_NAME = "NumScaleToDeviceElement";

void EditorWindow::SubmitUnloadSavedCanvasMetricEvent()
{
    // Create an unload canvas event
    auto eventId = LyMetrics_CreateEvent(UIEDITOR_UNLOAD_SAVED_CANVAS_METRIC_EVENT_NAME);

    // Add unique canvas Id attribute
    AZ::u64 uniqueId = 0;
    EBUS_EVENT_ID_RESULT(uniqueId, m_canvasEntityId, UiCanvasBus, GetUniqueCanvasId);
    QString uniqueIdString;
    uniqueIdString.setNum(uniqueId);
    LyMetrics_AddAttribute(eventId, UIEDITOR_CANVAS_ID_ATTRIBUTE_NAME, qPrintable(uniqueIdString));

    // Add canvas size metric
    AZ::Vector2 canvasSize;
    EBUS_EVENT_ID_RESULT(canvasSize, m_canvasEntityId, UiCanvasBus, GetCanvasSize);
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_WIDTH_METRIC_NAME, canvasSize.GetX());
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_HEIGHT_METRIC_NAME, canvasSize.GetY());

    // Add max hierarchy depth metric
    LyShine::EntityArray childElements;
    EBUS_EVENT_ID_RESULT(childElements, m_canvasEntityId, UiCanvasBus, GetChildElements);
    int maxDepth = GetCanvasMaxHierarchyDepth(childElements);
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_MAX_HIERARCHY_DEPTH_METRIC_NAME, maxDepth);

    // Get a list of the component types that can be added to a UI element
    // The ComponentTypeData struct has a flag to say whether the component is an LyShine component
    AZStd::vector<ComponentHelpers::ComponentTypeData> uiComponentTypes = ComponentHelpers::GetAllComponentTypesThatCanAppearInAddComponentMenu();

    // Make a list of all elements of this canvas
    LyShine::EntityArray allElements;
    EBUS_EVENT_ID(m_canvasEntityId, UiCanvasBus, FindElements,
        [](const AZ::Entity* entity) { return true; },
        allElements);

    // Add total number of elements metric
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_NUM_ELEMENT_METRIC_NAME, allElements.size());

    std::vector<int> numElementsWithComponent(uiComponentTypes.size(), 0);
    int numElementsWithCustomComponent = 0;
    int numCustomComponentsAvailable = 0;
    for (int i = 0; i < uiComponentTypes.size(); i++)
    {
        if (!uiComponentTypes[i].isLyShineComponent)
        {
            ++numCustomComponentsAvailable;
        }
    }
    std::vector<int> numElementsWithAnchorPreset(AnchorPresets::PresetIndexCount, 0);
    int numElementsWithCustomAnchors = 0;
    std::vector<int> numElementsWithPivotPreset(PivotPresets::PresetIndexCount, 0);
    int numElementsWithCustomPivot = 0;
    int numRotatedElements = 0;
    int numScaledElements = 0;
    int numScaleToDeviceElements = 0;

    for (auto entity : allElements)
    {
        // Check which components this element has
        bool elementHasCustomComponent = false;
        for (int i = 0; i < uiComponentTypes.size(); i++)
        {
            if (entity->FindComponent(uiComponentTypes[i].classData->m_typeId))
            {
                numElementsWithComponent[i]++;

                if (!uiComponentTypes[i].isLyShineComponent)
                {
                    elementHasCustomComponent = true;
                }
            }
        }

        if (elementHasCustomComponent)
        {
            numElementsWithCustomComponent++;
        }

        // Check if this element is controlled by its parent
        bool isControlledByParent = false;
        AZ::Entity* parentElement = EntityHelpers::GetParentElement(entity);
        if (parentElement)
        {
            EBUS_EVENT_ID_RESULT(isControlledByParent, parentElement->GetId(), UiLayoutBus, IsControllingChild, entity->GetId());
        }

        if (!isControlledByParent)
        {
            // Check if this element is scaled
            AZ::Vector2 scale(1.0f, 1.0f);
            EBUS_EVENT_ID_RESULT(scale, entity->GetId(), UiTransformBus, GetScale);
            if (scale.GetX() != 1.0f || scale.GetY() != 1.0f)
            {
                numScaledElements++;
            }

            // Check if this element is rotated
            float rotation = 0.0f;
            EBUS_EVENT_ID_RESULT(rotation, entity->GetId(), UiTransformBus, GetZRotation);
            if (rotation != 0.0f)
            {
                numRotatedElements++;
            }

            // Check if this element is using an anchor preset
            UiTransform2dInterface::Anchors anchors;
            EBUS_EVENT_ID_RESULT(anchors, entity->GetId(), UiTransform2dBus, GetAnchors);
            AZ::Vector4 anchorVect(anchors.m_left, anchors.m_top, anchors.m_right, anchors.m_bottom);
            int anchorPresetIndex = AnchorPresets::AnchorToPresetIndex(anchorVect);
            if (anchorPresetIndex >= 0)
            {
                numElementsWithAnchorPreset[anchorPresetIndex]++;
            }
            else
            {
                numElementsWithCustomAnchors++;
            }

            // Check if this element is using a pivot preset
            AZ::Vector2 pivot;
            EBUS_EVENT_ID_RESULT(pivot, entity->GetId(), UiTransformBus, GetPivot);
            AZ::Vector2 pivotVect(pivot.GetX(), pivot.GetY());
            int pivotPresetIndex = PivotPresets::PivotToPresetIndex(pivotVect);
            if (pivotPresetIndex >= 0)
            {
                numElementsWithPivotPreset[pivotPresetIndex]++;
            }
            else
            {
                numElementsWithCustomPivot++;
            }
        }

        // Check if this element is scaled to device
        bool scaleToDevice = false;
        EBUS_EVENT_ID_RESULT(scaleToDevice, entity->GetId(), UiTransformBus, GetScaleToDevice);
        if (scaleToDevice)
        {
            numScaleToDeviceElements++;
        }
    }

    // Add metric for each internal component representing the number of elements having that component
    int numCustomComponentsUsed = 0;
    for (int i = 0; i < uiComponentTypes.size(); i++)
    {
        auto& componentType = uiComponentTypes[i];
        if (componentType.isLyShineComponent)
        {
            AZ::Edit::ClassData* editInfo = componentType.classData->m_editData;
            if (editInfo)
            {
                int count = numElementsWithComponent[i];
                AZStd::string metricName(UIEDITOR_CANVAS_NUM_ELEMENTS_WITH_COMPONENT_PREFIX_METRIC_NAME);
                metricName += editInfo->m_name;
                LyMetrics_AddMetric(eventId, metricName.c_str(), count);
            }
        }
        else
        {
            numCustomComponentsUsed++;
        }
    }

    // Add metric for the number of elements that have a custom component
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_NUM_ELEMENTS_WITH_CUSTOM_COMPONENT_METRIC_NAME, numElementsWithCustomComponent);

    // Add metric for the number of unique custom components used
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_NUM_UNIQUE_CUSTOM_COMPONENT_NAME, numCustomComponentsUsed);

    // Add a metric for the number of available custom components
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_NUM_AVAILABLE_CUSTOM_COMPONENT_NAME, numCustomComponentsAvailable);

    // Construct a string representing the number of elements that use each anchor preset e.g. {0, 1, 6, 20, ...}
    QString anchorPresetString("{");
    for (int i = 0; i < AnchorPresets::PresetIndexCount; i++)
    {
        anchorPresetString.append(QString::number(numElementsWithAnchorPreset[i]));
        if (i < AnchorPresets::PresetIndexCount - 1)
        {
            anchorPresetString.append(", ");
        }
    }
    anchorPresetString.append("}");

    // Add attribute representing the number of elements using each anchor preset
    LyMetrics_AddAttribute(eventId, UIEDITOR_CANVAS_NUM_ANCHOR_PRESETS_ATTRIBUTE_NAME, qPrintable(anchorPresetString));

    // Add metric representing the number of elements with their anchors set to a custom value
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_NUM_ANCHOR_CUSTOM_ATTRIBUTE_NAME, numElementsWithCustomAnchors);

    // Construct a string representing the number of elements that use each pivot preset e.g. {0, 1, 6, 20, ...}
    QString pivotPresetString("{");
    for (int i = 0; i < PivotPresets::PresetIndexCount; i++)
    {
        pivotPresetString.append(QString::number(numElementsWithPivotPreset[i]));
        if (i < PivotPresets::PresetIndexCount - 1)
        {
            pivotPresetString.append(", ");
        }
    }
    pivotPresetString.append("}");

    // Add attribute representing the number of elements using each pivot preset
    LyMetrics_AddAttribute(eventId, UIEDITOR_CANVAS_NUM_PIVOT_PRESETS_ATTRIBUTE_NAME, qPrintable(pivotPresetString));

    // Add metric representing the number of elements with their pivot set to a custom value
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_NUM_PIVOT_CUSTOM_ATTRIBUTE_NAME, numElementsWithCustomPivot);

    // Add number of rotated elements metric
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_NUM_ROTATED_ELEMENT_METRIC_NAME, numRotatedElements);

    // Add number of scaled elements metric
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_NUM_SCALED_ELEMENT_METRIC_NAME, numScaledElements);

    // Add metric representing the number of elements that are scaled to device
    LyMetrics_AddMetric(eventId, UIEDITOR_CANVAS_NUM_SCALE_TO_DEVICE_ELEMENT_METRIC_NAME, numScaleToDeviceElements);

    // Submit the event
    LyMetrics_SubmitEvent(eventId);
}

int EditorWindow::GetCanvasMaxHierarchyDepth(const LyShine::EntityArray& rootChildElements)
{
    int depth = 0;

    if (rootChildElements.empty())
    {
        return depth;
    }

    int numChildrenCurLevel = rootChildElements.size();
    int numChildrenNextLevel = 0;
    std::list<AZ::Entity*> elementList(rootChildElements.begin(), rootChildElements.end());
    while (!elementList.empty())
    {
        auto& entity = elementList.front();

        LyShine::EntityArray childElements;
        EBUS_EVENT_ID_RESULT(childElements, entity->GetId(), UiElementBus, GetChildElements);
        if (!childElements.empty())
        {
            elementList.insert(elementList.end(), childElements.begin(), childElements.end());
            numChildrenNextLevel += childElements.size();
        }

        elementList.pop_front();
        numChildrenCurLevel--;

        if (numChildrenCurLevel == 0)
        {
            depth++;
            numChildrenCurLevel = numChildrenNextLevel;
            numChildrenNextLevel = 0;
        }
    }

    return depth;
}

void EditorWindow::DeleteSliceLibraryTree()
{
    // this just deletes the tree so that we know it is dirty
    if (m_sliceLibraryTree)
    {
        delete m_sliceLibraryTree;
        m_sliceLibraryTree = nullptr;
    }
}

bool EditorWindow::event(QEvent* ev)
{
    if (ev->type() == QEvent::ShortcutOverride)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
        QKeySequence keySequence(keyEvent->key() | keyEvent->modifiers());

        if (keySequence == UICANVASEDITOR_COORDINATE_SYSTEM_CYCLE_SHORTCUT_KEY_SEQUENCE)
        {
            ev->accept();
            return true;
        }
        else if (keySequence == UICANVASEDITOR_SNAP_TO_GRID_TOGGLE_SHORTCUT_KEY_SEQUENCE)
        {
            ev->accept();
            return true;
        }
    }

    return QMainWindow::event(ev);
}

void EditorWindow::keyReleaseEvent(QKeyEvent* ev)
{
    QKeySequence keySequence(ev->key() | ev->modifiers());

    if (keySequence == UICANVASEDITOR_COORDINATE_SYSTEM_CYCLE_SHORTCUT_KEY_SEQUENCE)
    {
        SignalCoordinateSystemCycle();
    }
    else if (keySequence == UICANVASEDITOR_SNAP_TO_GRID_TOGGLE_SHORTCUT_KEY_SEQUENCE)
    {
        SignalSnapToGridToggle();
    }
}

#include <EditorWindow.moc>
