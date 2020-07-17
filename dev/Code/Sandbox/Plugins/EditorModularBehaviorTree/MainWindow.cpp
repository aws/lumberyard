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
#include "MainWindow.h"

#include <IEditor.h>
#include <ICryPak.h>

#include <QDockWidget>
#include <QAction>

#include <QFileDialog>

#include <QMenuBar>

#include <QCloseEvent>

#include "TreePanel.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowFlags(Qt::Widget);

    QMenuBar* menu = new QMenuBar(this);
    QMenu* fileMenu = menu->addMenu("&File");
    connect(fileMenu->addAction("&New"), SIGNAL(triggered()), this, SLOT(OnMenuActionNew()));
    connect(fileMenu->addAction("&Open"), SIGNAL(triggered()), this, SLOT(OnMenuActionOpen()));
    // todo : add recent files menu
    connect(fileMenu->addAction("&Save"), SIGNAL(triggered()), this, SLOT(OnMenuActionSave()));
    connect(fileMenu->addAction("Save &as..."), SIGNAL(triggered()), this, SLOT(OnMenuActionSaveToFile()));

    setMenuBar(menu);

    GetIEditor()->RegisterNotifyListener(this);

    m_treePanel = new TreePanel(this);
    setCentralWidget(m_treePanel);
}

MainWindow::~MainWindow()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void MainWindow::OnEditorNotifyEvent(EEditorNotifyEvent editorNotifyEvent)
{
}

void MainWindow::closeEvent(QCloseEvent* closeEvent)
{
    if (!m_treePanel->HandleCloseEvent())
    {
        closeEvent->ignore();
        return;
    }

    QMainWindow::closeEvent(closeEvent);
}

void MainWindow::OnMenuActionNew()
{
    m_treePanel->OnWindowEvent_NewFile();
}

void MainWindow::OnMenuActionOpen()
{
    m_treePanel->OnWindowEvent_OpenFile();
}

void MainWindow::OnMenuActionSave()
{
    m_treePanel->OnWindowEvent_Save();
}

void MainWindow::OnMenuActionSaveToFile()
{
    m_treePanel->OnWindowEvent_SaveToFile();
}

#include <MainWindow.moc>
