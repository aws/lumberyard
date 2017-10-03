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

// Description : Dialog for python script terminal


#ifndef CRYINCLUDE_EDITOR_SCRIPTTERMDIALOG_H
#define CRYINCLUDE_EDITOR_SCRIPTTERMDIALOG_H
#pragma once

#include "Util/BoostPythonHelpers.h"

#include <QWidget>

#include <QScopedPointer>
#include <QColor>

#define SCRIPT_TERM_WINDOW_NAME "Script Terminal"

class QStringListModel;
namespace Ui {
    class CScriptTermDialog;
}

class CScriptTermDialog
    : public QWidget
    , public PyScript::IPyScriptListener
{
    Q_OBJECT
public:
    CScriptTermDialog();
    ~CScriptTermDialog();

    void AppendText(const char* pText);
    void AppendError(const char* pText);
    static void RegisterViewClass();

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private slots:
    void OnScriptHelp();
    void OnOK();
    void OnScriptInputTextChanged(const QString& text);

private:
    void InitCompleter();

    void ExecuteAndPrint(const char* cmd);

    void AppendToConsole(const QString& string, const QColor& color = Qt::white);

    virtual void OnStdOut(const char* pString) { AppendText(pString); }
    virtual void OnStdErr(const char* pString) { AppendError(pString); }

    QScopedPointer<Ui::CScriptTermDialog> ui;
    QStringListModel* m_completionModel;
    QStringListModel* m_lastCommandModel;
    QStringList m_lastCommands;

    int m_upArrowLastCommandIndex = -1;
};

#endif // CRYINCLUDE_EDITOR_SCRIPTTERMDIALOG_H
