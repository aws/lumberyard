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

#ifndef CRYINCLUDE_EDITOR_STARTUPLOGODIALOG_H
#define CRYINCLUDE_EDITOR_STARTUPLOGODIALOG_H

#pragma once
// StartupLogoDialog.h : header file
//

#include <QWidget>

/////////////////////////////////////////////////////////////////////////////
// CStartupLogoDialog dialog

namespace Ui
{
    class StartupLogoDialog;
}

class CStartupLogoDialog
    : public QWidget
    , public IInitializeUIInfo
{
    Q_OBJECT
    // Construction
public:
    CStartupLogoDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CStartupLogoDialog();

    void SetVersion(const SFileVersion& v);
    void SetInfo(const char* text);

    static void SetText(const char* text);

    virtual void SetInfoText(const char* text);

    static CStartupLogoDialog* instance() { return s_pLogoWindow; }

    // Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CStartupLogoDialog)
    void OnInitDialog();
    void paintEvent(QPaintEvent* event) override;

    static CStartupLogoDialog* s_pLogoWindow;

    QScopedPointer<Ui::StartupLogoDialog> m_ui;

private:
    QPixmap                     m_hBitmap;          // Struct to hold the background bitmap
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_STARTUPLOGODIALOG_H
