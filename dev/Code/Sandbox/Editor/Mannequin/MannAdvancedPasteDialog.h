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

#ifndef __MANN_ADVANCED_PASTE_DIALOG__
#define __MANN_ADVANCED_PASTE_DIALOG__

#include <ICryMannequin.h>
#include "Controls/TagSelectionControl.h"

#include <QDialog>

namespace Ui
{
    class MannAdvancedPasteDialog;
}

class CMannAdvancedPasteDialog
    : public QDialog
{
    Q_OBJECT
public:
    CMannAdvancedPasteDialog(const IAnimationDatabase& animDB, FragmentID fragmentID, const SFragTagState& tagState, QWidget* parent = nullptr);
    ~CMannAdvancedPasteDialog();

    SFragTagState GetTagState() const { return m_tagState; }

protected:
    void OnInitDialog();
    void accept() override;

private:
    const IAnimationDatabase&  m_database;
    const FragmentID           m_fragmentID;
    SFragTagState              m_tagState;

    CTagSelectionControl       m_globalTagsSelectionControl;
    CTagSelectionControl       m_fragTagsSelectionControl;

    QScopedPointer<Ui::MannAdvancedPasteDialog> m_ui;
};

#endif
