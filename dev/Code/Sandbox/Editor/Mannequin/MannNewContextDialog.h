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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNNEWCONTEXTDIALOG_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNNEWCONTEXTDIALOG_H
#pragma once

#include "MannequinBase.h"
#include <QDialog>
#include <QScopedPointer>

// fwd decl'
struct SScopeContextData;
namespace Ui {
    class MannNewContextDialog;
}

class CMannNewContextDialog
    : public QDialog
{
public:
    enum EContextDialogModes
    {
        eContextDialog_New = 0,
        eContextDialog_Edit,
        eContextDialog_CloneAndEdit
    };
    CMannNewContextDialog(SScopeContextData* context, const EContextDialogModes mode, QWidget* parent = nullptr);
    virtual ~CMannNewContextDialog();

    void accept() override;
    void OnBrowsemodelButton();
    void OnNewAdbButton();
    void OnAcceptButton();

protected:
    void OnInitDialog();

private:
    void PopulateControls();
    void PopulateDatabaseCombo();
    void PopulateControllerDefCombo();

    void CloneScopeContextData(const SScopeContextData* context);

    //bool m_editing;
    const EContextDialogModes m_mode;
    SScopeContextData* m_pData;
    TSmartPtr<CVarBlock> m_tagVars;
    CTagControl m_tagControls;

    QScopedPointer<Ui::MannNewContextDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNNEWCONTEXTDIALOG_H
