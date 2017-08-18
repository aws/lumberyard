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

#ifndef CRYINCLUDE_EDITOR_AI_AIDEBUGGER_H
#define CRYINCLUDE_EDITOR_AI_AIDEBUGGER_H
#pragma once

#include <QMainWindow>
#include <IAISystem.h>


class CAIDebuggerView;

namespace Ui
{
    class AIDebugger;
}

//////////////////////////////////////////////////////////////////////////
//
// Main Dialog for AI Debugger
//
//////////////////////////////////////////////////////////////////////////
class CAIDebugger
    : public QMainWindow
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    static const char* ClassName();
    static const GUID& GetClassID();
    static void RegisterViewClass();

    CAIDebugger(QWidget* parent = nullptr);
    virtual ~CAIDebugger();

protected:
    void closeEvent(QCloseEvent* event) override;

    void FileLoad(void);
    void FileSave(void);
    void FileLoadAs(void);
    void FileSaveAs(void);


    //KDAB These are unused?
    void DebugOptionsEnableAll(void);
    void DebugOptionsDisableAll(void);

    // Called by the editor to notify the listener about the specified event.
    void OnEditorNotifyEvent(EEditorNotifyEvent event) Q_DECL_OVERRIDE;

    void UpdateAll(void);

protected:
    QScopedPointer<Ui::AIDebugger> m_ui;
};

#endif // CRYINCLUDE_EDITOR_AI_AIDEBUGGER_H
