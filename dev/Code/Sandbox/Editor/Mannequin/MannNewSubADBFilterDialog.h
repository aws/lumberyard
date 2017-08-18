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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNNEWSUBADBFILTERDIALOG_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNNEWSUBADBFILTERDIALOG_H
#pragma once

#include "MannequinBase.h"
#include <QDialog>

class QString;
class QLineEdit;
class QStringListModel;

namespace Ui {
    class MannNewSubADBFilterDialog;
}

class CMannNewSubADBFilterDialog
    : public QDialog
{
    Q_OBJECT
public:
    enum EContextDialogModes
    {
        eContextDialog_New = 0,
        eContextDialog_Edit,
        eContextDialog_CloneAndEdit
    };
    CMannNewSubADBFilterDialog(SMiniSubADB* context, IAnimationDatabase* animDB, const string& parentName, const EContextDialogModes mode, QWidget* pParent = nullptr);
    virtual ~CMannNewSubADBFilterDialog();

private slots:
    void OnOk();
    void OnFrag2Used();
    void OnUsed2Frag();
    void OnFilenameChanged();

private:
    void OnInitDialog();

    void EnableControls();
    bool VerifyFilename();
    void PopulateControls();
    void PopulateFragIDList();
    void CloneSubADBData(const SMiniSubADB* context);
    void CopySubADBData(const SMiniSubADB* pData);
    void SaveNewSubADBFile(const string& filename);
    void FindFragmentIDsMissing(SMiniSubADB::TFragIDArray& outFrags, const SMiniSubADB::TFragIDArray& AFrags, const SMiniSubADB::TFragIDArray& BFrags);

    QScopedPointer<Ui::MannNewSubADBFilterDialog> m_ui;

    //bool m_editing;
    const EContextDialogModes m_mode;
    SMiniSubADB* m_pData;
    SMiniSubADB* m_pDataCopy;
    IAnimationDatabase* m_animDB;
    const string& m_parentName;

    QScopedPointer<QStringListModel>                    m_fragIDList;
    QScopedPointer<QStringListModel>                    m_fragUsedIDList;

    TSmartPtr<CVarBlock> m_tagVars;
    CTagControl m_tagControls;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNNEWSUBADBFILTERDIALOG_H
