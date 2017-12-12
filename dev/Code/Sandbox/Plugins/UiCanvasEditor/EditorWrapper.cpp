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

namespace
{
    //! \brief Writes the current value of the sys_localization_folder CVar to the editor settings file (Amazon.ini)
    void SaveStartupLocalizationFolderSetting()
    {
        if (gEnv && gEnv->pConsole)
        {
            ICVar* locFolderCvar = gEnv->pConsole->GetCVar("sys_localization_folder");

            QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);
            settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

            settings.setValue(UICANVASEDITOR_SETTINGS_STARTUP_LOC_FOLDER_KEY, locFolderCvar->GetString());

            settings.endGroup();
            settings.sync();
        }
    }

    //! \brief Reads loc folder value from Amazon.ini and re-sets the CVar accordingly
    void RestoreStartupLocalizationFolderSetting()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);
        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        QString startupLocFolder(settings.value(UICANVASEDITOR_SETTINGS_STARTUP_LOC_FOLDER_KEY).toString());
        if (!startupLocFolder.isEmpty() && gEnv && gEnv->pConsole)
        {
            ICVar* locFolderCvar = gEnv->pConsole->GetCVar("sys_localization_folder");
            locFolderCvar->Set(startupLocFolder.toUtf8().constData());
        }

        settings.endGroup();
        settings.sync();
    }
}

EditorWrapper::EditorWrapper(QWidget* parent,
    Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    // Since the lifetime of EditorWindow and the UI Editor itself aren't the
    // same, we use the initial opening of the UI Editor to save the current
    // value of the loc folder CVar since the user can temporarily change its
    // value while using the UI Editor.
    SaveStartupLocalizationFolderSetting();

    m_editor = std::unique_ptr< EditorWindow >(CreateEditorWindow(QString()));

    PropertyHandlers::Register();    
}

EditorWrapper::~EditorWrapper()
{
    // We must restore the original loc folder CVar value otherwise we will
    // have no way of obtaining the original loc folder location (in case
    // the user chooses to open the UI Editor once more).
    RestoreStartupLocalizationFolderSetting();
}

EditorWindow* EditorWrapper::CreateEditorWindow(const QString& canvasFilename)
{
    // IMPORTANT: The third parameter of EditorWindow() is the
    // parent QWidget. If we pass "this" as the QWidget parent,
    // the new EditorWindow WON'T be visible.
    EditorWindow* ew = new EditorWindow(this, canvasFilename, nullptr, 0);

    setCentralWidget(ew);

    // We need to do this after setCentralWidget and give Qt a tick to get the EditorWindow
    // main window resized before restoring the editor window dock settings etc
    QTimer::singleShot(0, ew, SLOT(RestoreEditorWindowSettings()));

    return ew;
}

void EditorWrapper::Restart(const QString& pathname)
{
    // Don't allow a restart if there is a context menu up. Restarting doesn't delete the
    // context menu, so it would be referencing an invalid window. Another option is to
    // close the context menu on restart, but the main editor's behavior seems to be to ignore
    // the main keyboard shortcuts if a context menu is up
    QWidget* widget = QApplication::activePopupWidget();
    if (widget)
    {
        return;
    }

    // Save the current state. It is safest to do this before destroying the canvas
    m_editor->SaveEditorWindowSettings();

    // Replace the canvas.
    {
        // Get rid of the current canvas.
        m_editor->DestroyCanvas();
        AZ_Assert(!m_editor->GetCanvas().IsValid(), "Destroy canvas failed");
    }

    // Restart the UI.
    m_editor->SetDestroyCanvasWasCausedByRestart(true);

    // Delete the previous EditorWindow before creating a new one.
    m_editor.reset();
    m_editor.reset(CreateEditorWindow(pathname));
}

void EditorWrapper::paintEvent(QPaintEvent* paintEvent)
{
    QMainWindow::paintEvent(paintEvent);

    if (m_editor != nullptr)
    {
        m_editor->GetViewport()->Refresh();
    }
}

void EditorWrapper::closeEvent(QCloseEvent* closeEvent)
{
    if (m_editor != nullptr)
    {
        if (!m_editor->CanExitNow())
        {
            // Nothing to do.
            closeEvent->ignore();
            return;
        }

        // Save the current window state
        m_editor->SaveEditorWindowSettings();
    }

    QMainWindow::closeEvent(closeEvent);
}

#include <EditorWrapper.moc>
