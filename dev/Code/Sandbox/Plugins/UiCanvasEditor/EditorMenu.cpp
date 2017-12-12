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
#include "FeedbackDialog.h"

static const bool debugViewUndoStack = false;

void EditorWindow::EditorMenu_Open(QString optional_selectedFile, EditorWrapper* parentWrapper)
{
    if (!CanExitNow())
    {
        // Nothing to do.
        return;
    }

    if (optional_selectedFile.isEmpty())
    {
        QString dir;
        QStringList recentFiles = ReadRecentFiles();

        // If we had recently opened canvases, open the most recent one's directory
        if (recentFiles.size() > 0)
        {
            dir = Path::GetPath(recentFiles.front());
        }
        // Else go to the default canvas directory
        else
        {
            dir = FileHelpers::GetAbsoluteDir(UICANVASEDITOR_CANVAS_DIRECTORY);
        }

        optional_selectedFile = QFileDialog::getOpenFileName(this,
                QString(),
                dir,
                "*." UICANVASEDITOR_CANVAS_EXTENSION);
    }

    if (optional_selectedFile.isEmpty())
    {
        // Nothing more to do.
        return;
    }

    QMetaObject::invokeMethod(parentWrapper, "Restart", Qt::QueuedConnection, Q_ARG(QString, optional_selectedFile));
}

void EditorWindow::AddMenu_File(EditorWrapper* parentWrapper)
{
    QMenu* menu = menuBar()->addMenu("&File");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    // Create a new canvas.
    {
        QAction* action = new QAction("&New Canvas...", this);
        action->setShortcut(QKeySequence::New);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            [ this, parentWrapper ](bool checked)
            {
                if (CanExitNow())
                {
                    QMetaObject::invokeMethod(parentWrapper, "Restart", Qt::QueuedConnection, Q_ARG(QString, QString()));
                }
            });
        menu->addAction(action);
        addAction(action);
    }

    // Load a canvas.
    {
        QAction* action = new QAction("&Open Canvas...", this);
        action->setShortcut(QKeySequence::Open);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            [ this, parentWrapper ](bool checked)
            {
                EditorMenu_Open("", parentWrapper);
            });
        menu->addAction(action);
        addAction(action);
    }

    string canvasFilename;
    EBUS_EVENT_ID_RESULT(canvasFilename, GetCanvas(), UiCanvasBus, GetPathname);

    // Save the canvas.
    {
        QFileInfo fileInfo(GetCanvasSourcePathname().c_str());
        QAction* action = new QAction(QString("&Save " + (fileInfo.fileName().isEmpty() ? "Canvas" : fileInfo.fileName())), this);
        if (!canvasFilename.empty())
        {
            action->setShortcut(QKeySequence::Save);
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        }
        QObject::connect(action,
            &QAction::triggered,
            [ this, parentWrapper ](bool checked)
            {
                bool ok = SaveCanvasToXml(false);
                if (!ok)
                {
                    return;
                }

                // Refresh the File menu to update the
                // "Recent Files" and "Save".
                RefreshEditorMenu(parentWrapper);
            });
        menu->addAction(action);
        addAction(action);

        // If there's no filename,
        // we want the menu to be visible, but disabled.
        action->setEnabled(!canvasFilename.empty());
    }

    // Save the canvas.
    {
        QAction* action = new QAction("Save Canvas &As...", this);

        if (canvasFilename.empty())
        {
            action->setShortcut(QKeySequence::Save);
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        }

        QObject::connect(action,
            &QAction::triggered,
            [ this, parentWrapper ](bool checked)
            {
                bool ok = SaveCanvasToXml(true);
                if (!ok)
                {
                    return;
                }

                // Refresh the File menu to update the
                // "Recent Files" and "Save".
                RefreshEditorMenu(parentWrapper);
            });
        menu->addAction(action);
        addAction(action);
    }

    menu->addSeparator();

    // "Save as Prefab..." file menu option
    {
        HierarchyWidget* widget = GetHierarchy();
        QAction* action = PrefabHelpers::CreateSavePrefabAction(widget);

        // This menu option is always available to the user
        menu->addAction(action);
        addAction(action);
    }

    menu->addSeparator();

    // Recent Files.
    {
        QStringList recentFiles = ReadRecentFiles();

        // List of recent files.
        {
            QMenu* recentMenu = menu->addMenu("&Recent Files");
            recentMenu->setEnabled(!recentFiles.isEmpty());

            // QStringList -> QMenu.
            for (auto && fileName : recentFiles)
            {
                QAction* action = new QAction(fileName, this);
                QObject::connect(action,
                    &QAction::triggered,
                    [ this, parentWrapper, fileName ](bool checked)
                    {
                        EditorMenu_Open(fileName, parentWrapper);
                    });
                recentMenu->addAction(action);
                addAction(action);
            }
        }

        // Clear Recent Files.
        {
            QAction* action = new QAction("Clear Recent Files", this);
            action->setEnabled(!recentFiles.isEmpty());

            QObject::connect(action,
                &QAction::triggered,
                [ this, parentWrapper ](bool checked)
                {
                    ClearRecentFile();

                    RefreshEditorMenu(parentWrapper);
                });
            menu->addAction(action);
            addAction(action);
        }
    }
}

void EditorWindow::AddMenuItems_Edit(QMenu* menu)
{
    // Undo.
    {
        QAction* action = GetUndoGroup()->createUndoAction(this);
        action->setShortcut(QKeySequence::Undo);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        menu->addAction(action);
        addAction(action);
    }

    // Redo.
    {
        QAction* action = GetUndoGroup()->createRedoAction(this);

        // IMPORTANT: We CAN'T just provide QKeySequence::Redo as a
        // shortcut because the menu will show CTRL+Y as the shortcut.
        // To display CTRL+SHIFT+Z by default, we have to provide the
        // list of shortcuts explicitly.
        {
            action->setShortcuts(QList<QKeySequence>{QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z),
                                                     QKeySequence(Qt::META + Qt::SHIFT + Qt::Key_Z),
                                                     QKeySequence(QKeySequence::Redo)});
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        }

        menu->addAction(action);
        addAction(action);
    }

    menu->addSeparator();

    // Select All.
    {
        QAction* action = new QAction("Select &All", this);
        action->setShortcut(QKeySequence::SelectAll);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            [this](bool checked)
            {
                GetHierarchy()->selectAll();
            });
        menu->addAction(action);
        addAction(action);
    }

    menu->addSeparator();

    bool itemsAreSelected = !GetHierarchy()->selectedItems().isEmpty();
    bool thereIsContentInTheClipboard = ClipboardContainsOurDataType();

    // Cut.
    {
        QAction* action = new QAction("Cu&t", this);
        action->setShortcut(QKeySequence::Cut);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        action->setEnabled(itemsAreSelected);
        QObject::connect(action,
            &QAction::triggered,
            GetHierarchy(),
            &HierarchyWidget::Cut);
        menu->addAction(action);
        addAction(action);  // Qt::WidgetWithChildrenShortcut works on the associated widget, not parent widget. The associated widget is a menu, and menus don't have focus, so also add the action to the window

        m_actionsEnabledWithSelection.push_back(action);
    }

    // Copy.
    {
        QAction* action = new QAction("&Copy", this);
        action->setShortcut(QKeySequence::Copy);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        action->setEnabled(itemsAreSelected);
        QObject::connect(action,
            &QAction::triggered,
            GetHierarchy(),
            &HierarchyWidget::Copy);
        menu->addAction(action);
        addAction(action);  // Qt::WidgetWithChildrenShortcut works on the associated widget, not parent widget. The associated widget is a menu, and menus don't have focus, so also add the action to the window

        m_actionsEnabledWithSelection.push_back(action);
    }

    // Paste.
    {
        // Paste as silbing.
        {
            QAction* action = new QAction(itemsAreSelected ? "&Paste as sibling" : "&Paste", this);
            action->setShortcut(QKeySequence::Paste);
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            action->setEnabled(thereIsContentInTheClipboard);
            QObject::connect(action,
                &QAction::triggered,
                GetHierarchy(),
                &HierarchyWidget::PasteAsSibling);
            menu->addAction(action);
            addAction(action);  // Qt::WidgetWithChildrenShortcut works on the associated widget, not parent widget. The associated widget is a menu, and menus don't have focus, so also add the action to the window

            m_pasteAsSiblingAction = action;
        }

        // Paste as child.
        {
            QAction* action = new QAction("Paste as c&hild", this);
            {
                action->setShortcuts(QList<QKeySequence>{QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_V),
                                                         QKeySequence(Qt::META + Qt::SHIFT + Qt::Key_V)});
                action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            }
            action->setEnabled(thereIsContentInTheClipboard && itemsAreSelected);
            QObject::connect(action,
                &QAction::triggered,
                GetHierarchy(),
                &HierarchyWidget::PasteAsChild);
            menu->addAction(action);
            addAction(action);  // Qt::WidgetWithChildrenShortcut works on the associated widget, not parent widget. The associated widget is a menu, and menus don't have focus, so also add the action to the window

            m_pasteAsChildAction = action;
        }
    }

    if (debugViewUndoStack)
    {
        QAction* action = new QAction("[DEBUG] View undo stack", this);
        QObject::connect(action,
            &QAction::triggered,
            [this](bool checked)
            {
                static QUndoView* undoView = nullptr;
                if (undoView)
                {
                    undoView->setGroup(GetUndoGroup());
                }
                else
                {
                    undoView = new QUndoView(GetUndoGroup());
                    undoView->setWindowTitle("[DEBUG] Undo stack");
                    undoView->setAttribute(Qt::WA_QuitOnClose, false);
                }
                undoView->show();
            });
        menu->addAction(action);
        addAction(action);
    }

    menu->addSeparator();

    // Delete.
    {
        QAction* action = new QAction("Delete", this);
        action->setShortcut(QKeySequence::Delete);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        action->setEnabled(itemsAreSelected);
        QObject::connect(action,
            &QAction::triggered,
            [this](bool checked)
            {
                GetHierarchy()->DeleteSelectedItems();
            });
        menu->addAction(action);
        addAction(action);  // Qt::WidgetWithChildrenShortcut works on the associated widget, not parent widget. The associated widget is a menu, and menus don't have focus, so also add the action to the window

        m_actionsEnabledWithSelection.push_back(action);
    }
}

void EditorWindow::AddMenu_Edit()
{
    QMenu* menu = menuBar()->addMenu("&Edit");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    AddMenuItems_Edit(menu);
}

void EditorWindow::AddMenu_View(EditorWrapper* parentWrapper)
{
    QMenu* menu = menuBar()->addMenu("&View");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    // Zoom options
    {
        // Zoom in
        {
            QAction* action = new QAction("Zoom &In", this);
            action->setShortcut(QKeySequence::ZoomIn);
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            QObject::connect(action,
                &QAction::triggered,
                [this](bool checked)
                {
                    GetViewport()->GetViewportInteraction()->IncreaseCanvasToViewportScale();
                });
            menu->addAction(action);
            addAction(action);
        }

        // Zoom out
        {
            QAction* action = new QAction("Zoom &Out", this);
            action->setShortcut(QKeySequence::ZoomOut);
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            QObject::connect(action,
                &QAction::triggered,
                [this](bool checked)
                {
                    GetViewport()->GetViewportInteraction()->DecreaseCanvasToViewportScale();
                });
            menu->addAction(action);
            addAction(action);
        }

        // Zoom to fit
        {
            QAction* action = new QAction("&Fit Canvas", this);
            {
                action->setShortcuts(QList<QKeySequence>{QKeySequence(Qt::CTRL + Qt::Key_0),
                                                         QKeySequence(Qt::META + Qt::Key_0)});
                action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            }
            QObject::connect(action,
                &QAction::triggered,
                [this](bool checked)
                {
                    GetViewport()->GetViewportInteraction()->CenterCanvasInViewport();
                });
            menu->addAction(action);
            addAction(action);
        }

        // Actual size
        {
            QAction* action = new QAction("Actual &Size", this);
            {
                action->setShortcuts(QList<QKeySequence>{QKeySequence(Qt::CTRL + Qt::Key_1),
                                                         QKeySequence(Qt::META + Qt::Key_1)});
                action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            }
            QObject::connect(action,
                &QAction::triggered,
                [this](bool checked)
                {
                    // Center the canvas then update scale
                    GetViewport()->GetViewportInteraction()->CenterCanvasInViewport();
                    GetViewport()->GetViewportInteraction()->ResetCanvasToViewportScale();
                });
            menu->addAction(action);
            addAction(action);
        }
    }

    menu->addSeparator();

    // Add all Edit mode QDockWidget panes.
    {
        QList<QDockWidget*> list = findChildren<QDockWidget*>();

        for (auto p : list)
        {
            // findChildren is recursive. But we only want dock widgets that are immediate
            // children since the Animation pane has some dock widgets of its own
            if (p->parent() == this && !IsPreviewModeDockWidget(p))
            {
                menu->addAction(p->toggleViewAction());
            }
        }
    }

    // Add all Edit mode QToolBar panes.
    {
        QList<QToolBar*> list = findChildren<QToolBar*>();
        for (auto p : list)
        {
            if (p->parent() == this && !IsPreviewModeToolbar(p))
            {
                menu->addAction(p->toggleViewAction());
            }
        }
    }

    menu->addSeparator();

    // Add sub-menu to control which elements have borders drawn on them
    {
        QMenu* drawElementBordersMenu = menu->addMenu("Draw &Borders on Unselected Elements");

        auto viewport = GetViewport();

        // Add option to draw borders on all unselected elements (subject to "Include" options below)
        {
            QAction* action = new QAction("&Draw Borders", this);
            action->setCheckable(true);
            action->setChecked(viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Unselected));
            QObject::connect(action,
                &QAction::triggered,
                [viewport, this, parentWrapper](bool checked)
                {
                    viewport->ToggleDrawElementBorders(ViewportWidget::DrawElementBorders_Unselected);
                    RefreshEditorMenu(parentWrapper);
                });
            drawElementBordersMenu->addAction(action);
            addAction(action);
        }

        // Add option to include visual elements.
        {
            QAction* action = new QAction("Include &Visual Elements", this);
            action->setCheckable(true);
            action->setChecked(viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Visual));
            action->setEnabled(viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Unselected));
            QObject::connect(action,
                &QAction::triggered,
                [viewport](bool checked)
                {
                    viewport->ToggleDrawElementBorders(ViewportWidget::DrawElementBorders_Visual);
                });
            drawElementBordersMenu->addAction(action);
            addAction(action);
        }

        // Add option to include parent elements.
        {
            QAction* action = new QAction("Include &Parent Elements", this);
            action->setCheckable(true);
            action->setChecked(viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Parent));
            action->setEnabled(viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Unselected));
            QObject::connect(action,
                &QAction::triggered,
                [viewport](bool checked)
                {
                    viewport->ToggleDrawElementBorders(ViewportWidget::DrawElementBorders_Parent);
                });
            drawElementBordersMenu->addAction(action);
            addAction(action);
        }

        // Add option to include hidden elements.
        {
            QAction* action = new QAction("Include &Hidden Elements", this);
            action->setCheckable(true);
            action->setChecked(viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Hidden));
            action->setEnabled(viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Unselected));
            QObject::connect(action,
                &QAction::triggered,
                [viewport](bool checked)
                {
                    viewport->ToggleDrawElementBorders(ViewportWidget::DrawElementBorders_Hidden);
                });
            drawElementBordersMenu->addAction(action);
            addAction(action);
        }
    }

    AddMenu_View_LanguageSetting(menu);
}

void EditorWindow::AddMenu_View_LanguageSetting(QMenu* viewMenu)
{
    QMenu* setCurrentLanguageMenu = viewMenu->addMenu("Set Current &Language");

    // Group language settings together by action group to only allow one
    // selection/language to be active at a time
    QActionGroup* actionGroup = new QActionGroup(setCurrentLanguageMenu);

    // Iterate through the subdirectories of the localization folder. Each
    // directory corresponds to a different language containing localization
    // translations for that language.
    string fullLocPath(string(gEnv->pFileIO->GetAlias("@assets@")) + "/" + string(m_startupLocFolderName.toUtf8().constData()));
    QDir locDir(fullLocPath.c_str());
    locDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    locDir.setSorting(QDir::Name);
    QFileInfoList infoList(locDir.entryInfoList());
    for (auto subDirectory : infoList)
    {
        QString directoryName(subDirectory.fileName().toLower());

        // The loc system expects XML assets stored in a language-specific 
        // folder with an "_xml" suffix in the name. Truncate the displayed
        // name so the user just sees the language name (this isn't required
        // though).
        const QString xmlPostFix("_xml");
        if (directoryName.endsWith(xmlPostFix))
        {
            directoryName.chop(xmlPostFix.length());
        }

        QAction* action = setCurrentLanguageMenu->addAction(directoryName);
        action->setCheckable(true);

        // When a language is selected, update the localization folder CVar
        QObject::connect(action,
            &QAction::triggered,
            [this, directoryName](bool checked)
        {
            // First try to locate the directory by name, without the "_xml"
            // suffix (in case it actually exists by this name).
            QString fullLocPath(QString(gEnv->pFileIO->GetAlias("@assets@")) + "/" + m_startupLocFolderName + "/" + directoryName);
            QDir locDir(fullLocPath);

            // Try the directory with the expected suffix
            if (!locDir.exists())
            {
                locDir.setPath(locDir.path() + "_xml");
            }

            // Once the new CVar value is set, the loc system will auto-parse
            // the folder contents. See CSystem::OnLocalizationFolderCVarChanged.
            ICVar* locFolderCvar = gEnv->pConsole->GetCVar("sys_localization_folder");
            AZ_Assert(locFolderCvar,
                "sys_localization_folder no longer defined! This should be created in CSystem::CreateSystemVars().");

            if (locFolderCvar)
            {
                locFolderCvar->Set(locDir.path().toUtf8().constData());

                // Might as well throw a message if our dependencies change
                AZ_Assert(
                    locFolderCvar->GetOnChangeCallback(),
                    "sys_localization_folder CVar callback missing! "
                    "This used to be set to CSystem::OnLocalizationFolderCVarChanged but is now missing. "
                    "UI Editor language-switching features are no longer working.");
            }
            
            // Update the language setting; this will allow font families to
            // load language-specific font assets
            ICVar* languageCvar = gEnv->pConsole->GetCVar("g_language");
            AZ_Assert(languageCvar,
                "g_language no longer defined! This should be created in CSystem::CreateSystemVars().");

            if (languageCvar)
            {
                languageCvar->Set(directoryName.toUtf8().constData());

                // Make sure that our callback pipeline is setup properly
                AZ_Assert(
                    languageCvar->GetOnChangeCallback(),
                    "g_language CVar callback missing! "
                    "This used to be set to CSystem::OnLangaugeCVarChanged but is now missing. "
                    "UI Editor language-switching features are no longer working.");
            }
        });

        actionGroup->addAction(action);
    }
}

void EditorWindow::AddMenu_Preview()
{
    QMenu* menu = menuBar()->addMenu("&Preview");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    // Toggle preview.
    {
        QString menuItemName;
        if (m_editorMode == UiEditorMode::Edit)
        {
            menuItemName = "&Preview";
        }
        else
        {
            menuItemName = "End &Preview";
        }

        QAction* action = new QAction(menuItemName, this);
        action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            [this](bool checked)
            {
                ToggleEditorMode();
            });
        menu->addAction(action);
        addAction(action);  // Qt::WidgetWithChildrenShortcut works on the associated widget, not parent widget. The associated widget is a menu, and menus don't have focus, so also add the action to the window
    }
}

void EditorWindow::AddMenu_PreviewView()
{
    QMenu* menu = menuBar()->addMenu("&View");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    // Add all Preview mode QDockWidget panes.
    {
        QList<QDockWidget*> list = findChildren<QDockWidget*>();

        for (auto p : list)
        {
            // findChildren is recursive. But we only want dock widgets that are immediate
            // children since the Animation pane has some dock widgets of its own
            if (p->parent() == this && IsPreviewModeDockWidget(p))
            {
                menu->addAction(p->toggleViewAction());
            }
        }
    }

    // Add all Preview mode QToolBar panes.
    {
        QList<QToolBar*> list = findChildren<QToolBar*>();

        for (auto p : list)
        {
            if (p->parent() == this && IsPreviewModeToolbar(p))
            {
                menu->addAction(p->toggleViewAction());
            }
        }
    }
}

void EditorWindow::AddMenu_Help()
{
    const char* documentationUrl = "http://docs.aws.amazon.com/lumberyard/latest/userguide/ui-editor-intro.html";
    const char* tutorualsUrl = "https://gamedev.amazon.com/forums/tutorials#ui_creation";
    const char* forumUrl = "https://gamedev.amazon.com/forums/spaces/141/ui-2d.html";

    QMenu* menu = menuBar()->addMenu("&Help");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    // Documentation
    {
        QAction* action = new QAction("&Documentation", this);

        QObject::connect(action,
            &QAction::triggered,
            [documentationUrl](bool checked)
            {
                QDesktopServices::openUrl(QUrl(documentationUrl));
            });
        menu->addAction(action);
        addAction(action);
    }

    // Tutorials
    {
        QAction* action = new QAction("&Tutorials", this);

        QObject::connect(action,
            &QAction::triggered,
            [tutorualsUrl](bool checked)
            {
                QDesktopServices::openUrl(QUrl(tutorualsUrl));
            });
        menu->addAction(action);
        addAction(action);
    }

    // Forum
    {
        QAction* action = new QAction("&Forum", this);

        QObject::connect(action,
            &QAction::triggered,
            [forumUrl](bool checked)
            {
                QDesktopServices::openUrl(QUrl(forumUrl));
            });
        menu->addAction(action);
        addAction(action);
    }

    // Give Us Feedback
    {
        QAction* action = new QAction("&Give Us Feedback", this);

        QObject::connect(action,
            &QAction::triggered,
            [this](bool checked)
            {
                FeedbackDialog dialog(this);
                dialog.exec();
            });
        menu->addAction(action);
        addAction(action);
    }
}

void EditorWindow::UpdateActionsEnabledState()
{
    bool itemsAreSelected = !GetHierarchy()->selectedItems().isEmpty();
    bool thereIsContentInTheClipboard = ClipboardContainsOurDataType();

    for (QAction* action : m_actionsEnabledWithSelection)
    {
        action->setEnabled(itemsAreSelected);
    }

    if (m_pasteAsSiblingAction)
    {
        m_pasteAsSiblingAction->setEnabled(thereIsContentInTheClipboard);
    }
    if (m_pasteAsChildAction)
    {
        m_pasteAsChildAction->setEnabled(thereIsContentInTheClipboard && itemsAreSelected);
    }
}

void EditorWindow::RefreshEditorMenu(EditorWrapper* parentWrapper)
{
    m_actionsEnabledWithSelection.clear();
    m_pasteAsSiblingAction = nullptr;
    m_pasteAsChildAction = nullptr;

    auto actionList = actions();
    for (QAction* action : actionList)
    {
        removeAction(action);
        action->deleteLater();
    }

    menuBar()->clear();

    if (GetEditorMode() == UiEditorMode::Edit)
    {
        AddMenu_File(parentWrapper);
        AddMenu_Edit();
        AddMenu_View(parentWrapper);
        AddMenu_Preview();
        AddMenu_Help();
    }
    else
    {
        AddMenu_PreviewView();
        AddMenu_Preview();
        AddMenu_Help();
    }
}
