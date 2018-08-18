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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include <cmath>
#include <cstdlib>
#include <IAIAction.h>
#include <IAISystem.h>
#include <IAgent.h>

#include "IViewPane.h"
#include "Objects/EntityObject.h"
#include "Objects/SelectionGroup.h"
#include "Clipboard.h"

#include "ItemDescriptionDlg.h"

#include "AIManager.h"
#include "Util/AutoDirectoryRestoreFileDialog.h"

#include "AIDebugger.h"
#include "AIDebuggerView.h"
#include "IAIRecorder.h"

#include <QApplication>
#include <QFileDialog>
#include <QSettings>
#include "QtViewPaneManager.h"

#include <AI/ui_AIDebugger.h>

//////////////////////////////////////////////////////////////////////////
const char* CAIDebugger::ClassName()
{
    return LyViewPane::AIDebugger;
}

//////////////////////////////////////////////////////////////////////////
const GUID& CAIDebugger::GetClassID()
{
    // {2D185A30-4487-4d1c-A076-E2A5F3367760}
    static const GUID guid = {
        0x2d185a30, 0x4487, 0x4d1c, { 0xa0, 0x76, 0xe2, 0xa5, 0xf3, 0x36, 0x77, 0x60 }
    };
    return guid;
}

//////////////////////////////////////////////////////////////////////////
void CAIDebugger::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.paneRect = QRect(10, 250, 710, 500);
    options.showInMenu = GetIEditor()->IsLegacyUIEnabled();
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;

    AzToolsFramework::RegisterViewPane<CAIDebugger>(LyViewPane::AIDebugger, LyViewPane::CategoryOther, options);
}

//////////////////////////////////////////////////////////////////////////
CAIDebugger::CAIDebugger(QWidget* parent /* = nullptr */)
    : QMainWindow(parent)
    , m_ui(new Ui::AIDebugger)
{
    m_ui->setupUi(this);

    connect(m_ui->actionLoad, &QAction::triggered, this, &CAIDebugger::FileLoad);
    connect(m_ui->actionSave, &QAction::triggered, this, &CAIDebugger::FileSave);
    connect(m_ui->actionLoadAs, &QAction::triggered, this, &CAIDebugger::FileLoadAs);
    connect(m_ui->actionSaveAs, &QAction::triggered, this, &CAIDebugger::FileSaveAs);

    GetIEditor()->RegisterNotifyListener(this);

    connect(m_ui->debuggerView, &CAIDebuggerView::scrollRequest, m_ui->scrollArea, [=](const QPoint& point) { m_ui->scrollArea->ensureVisible(point.x(), point.y()); });

    QSettings settings;
    settings.beginGroup("AIDebugger");
    if (settings.contains("state"))
    {
        QByteArray state = settings.value("state").toByteArray();
        if (!state.isEmpty())
        {
            restoreState(state);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CAIDebugger::~CAIDebugger()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void CAIDebugger::closeEvent(QCloseEvent* event)
{
    QByteArray state = saveState();
    QSettings settings;
    settings.beginGroup("AIDebugger");
    settings.setValue("state", state);
    settings.endGroup();
    settings.sync();
}

//////////////////////////////////////////////////////////////////////////
// Called by the editor to notify the listener about the specified event.
void CAIDebugger::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnEndSimulationMode:
    case eNotify_OnEndGameMode:             // Send when editor goes out of game mode.
    {
        UpdateAll();
        update();
    }
    break;
    case eNotify_OnEndSceneOpen:
    {
        UpdateAll();
        update();
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebugger::UpdateAll(void)
{
    m_ui->debuggerView->UpdateView();
    m_ui->debuggerView->update();
}

void CAIDebugger::FileLoad(void)
{
    IAIRecorder* pRecorder = gEnv->pAISystem->GetIAIRecorder();
    if (!pRecorder || !pRecorder->Load())
    {
        gEnv->pLog->LogError("Failed to complete loading AI recording");
    }
    else
    {
        gEnv->pLog->Log("Successfully loaded AI recording");
        UpdateAll();
    }
}

void CAIDebugger::FileSave(void)
{
    IAIRecorder* pRecorder = gEnv->pAISystem->GetIAIRecorder();
    if (pRecorder)
    {
        pRecorder->Save();
    }
}

void CAIDebugger::FileSaveAs(void)
{
    char szFilters[] = "AI Recorder Files (*.rcd)";
    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptSave, QFileDialog::AnyFile, "rcd", "AIRECORD.rcd", szFilters, {}, {}, this);
    // Show the dialog
    if (dlg.exec())
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QString fileName = dlg.selectedFiles().first();
        QFileInfo info(fileName);
        if (info.suffix().toLower() == "rcd")
        {
            IAIRecorder* pRecorder = gEnv->pAISystem->GetIAIRecorder();
            if (pRecorder)
            {
                pRecorder->Save(fileName.toStdString().c_str());
            }
        }

        QApplication::restoreOverrideCursor();
    }
    // Either way, retrieve the focus
    setFocus();
}


void CAIDebugger::FileLoadAs(void)
{
    char szFilters[] = "AI Recorder Files (*.rcd)";
    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptOpen, QFileDialog::ExistingFile, "rcd", "AIRECORD.rcd", szFilters, {}, {}, this);
    // Show the dialog
    if (dlg.exec())
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QString fileName = dlg.selectedFiles().first();
        QFileInfo info(fileName);
        if (info.suffix().toLower() == "rcd")
        {
            IAIRecorder* pRecorder = GetISystem()->GetAISystem()->GetIAIRecorder();
            if (!pRecorder || !pRecorder->Load(fileName.toStdString().c_str()))
            {
                gEnv->pLog->LogError("Failed to complete loading AI recording");
            }
            else
            {
                gEnv->pLog->Log("Successfully loaded AI recording");
                UpdateAll();
            }
        }
        QApplication::restoreOverrideCursor();
    }

    // Either way, retrieve the focus
    setFocus();
}

void CAIDebugger::DebugOptionsEnableAll(void)
{
    m_ui->debuggerView->DebugEnableAll();
}

void CAIDebugger::DebugOptionsDisableAll(void)
{
    m_ui->debuggerView->DebugDisableAll();
}

#include <AI/AIDebugger.moc>
