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
#include "EditorUI_QT_Precompiled.h"
#include "ToolbarStandard.h"

#include <QtWidgets/QToolBar>
#include <QtCore/QCoreApplication>

CToolbarStandard::CToolbarStandard(QMainWindow* mainWindow)
    : IToolbar(mainWindow, TO_CSTR(CToolbarStandard))
    , m_actionUndo(nullptr)
    , m_actionRedo(nullptr)
{
    setWindowTitle("Standard");
}

CToolbarStandard::~CToolbarStandard()
{
}

void CToolbarStandard::init()
{
    QAction* action = nullptr;
    m_actionUndo = action = createAction("actionStandardUndo", "Undo", "Undo", "standardUndo.png");
    action->setEnabled(false);
    m_actionRedo = action = createAction("actionStandardRedo", "Redo", "Redo", "standardRedo.png");
    action->setEnabled(false);
    action = createAction("actionStandardCopy", "Copy", "Copy item", "standardCopy.png");
    insertSeparator(action);
    action = createAction("actionStandardPaste", "Paste", "Paste item", "standardPaste.png");
}

void CToolbarStandard::SignalNumUndoRedo(const unsigned int& numUndo, const unsigned int& numRedo)
{
    if (!m_actionUndo || !m_actionRedo)
    {
        return;
    }

    m_actionUndo->setEnabled(numUndo > 0);
    m_actionRedo->setEnabled(numRedo > 0);
}