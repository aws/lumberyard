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

#ifndef CRYINCLUDE_EDITOR_SOURCECONTROLADDDLG_H
#define CRYINCLUDE_EDITOR_SOURCECONTROLADDDLG_H
#pragma once

#include "Include/ISourceControl.h"
#include "Include/EditorCoreAPI.h"
#include <QDialog>

// SourceControlDescDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSourceControlAddDlg dialog

namespace Ui {
    class CSourceControlAddDlg;
}

class EDITOR_CORE_API CSourceControlAddDlg : public QDialog
{
    Q_OBJECT

public:

    // Construction
    CSourceControlAddDlg(const QString& sFilename, QWidget* pParent = nullptr);   // standard constructor
    virtual ~CSourceControlAddDlg();

    // Return the state of the dialog
    ESccFlags GetSCFlags() const {
        return m_scFlags;
    };

    // Implementation
protected:
    void OnBnClickedOk();
    void OnBnClickedAddDefault();

public:

    QString m_sDesc;
    QString m_sFilename;

private:
    ESccFlags m_scFlags;
    QScopedPointer<Ui::CSourceControlAddDlg> ui;
};

#endif // CRYINCLUDE_EDITOR_SOURCECONTROLADDDLG_H
