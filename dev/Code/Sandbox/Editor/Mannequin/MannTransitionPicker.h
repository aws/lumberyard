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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNTRANSITIONPICKER_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNTRANSITIONPICKER_H
#pragma once

#include "MannequinBase.h"
#include <QDialog>
#include <QScopedPointer>

// fwd decl'
struct SScopeContextData;
namespace Ui {
    class MannTransitionPickerDialog;
}

class CMannTransitionPickerDlg
    : public QDialog
{
public:
    CMannTransitionPickerDlg(FragmentID& fromFragID, FragmentID& toFragID, SFragTagState& fromFragTag, SFragTagState& toFragTag, QWidget* pParent = NULL);
    virtual ~CMannTransitionPickerDlg();

protected:
    void OnOk();

private:
    void UpdateFragmentIDs();
    void PopulateComboBoxes();

    //
    FragmentID& m_IDFrom;
    FragmentID& m_IDTo;

    //
    SFragTagState& m_TagsFrom;
    SFragTagState& m_TagsTo;

    QScopedPointer<Ui::MannTransitionPickerDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNTRANSITIONPICKER_H
