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

#include "MannAdvancedPasteDialog.h"
#include <Mannequin/ui_MannAdvancedPasteDialog.h>

//////////////////////////////////////////////////////////////////////////
void CreateTagControl(CTagSelectionControl& tagControl, const int tagControlId, QGroupBox* hostGroupBox, const CTagDefinition* const pTagDef, const TagState& defaultTagState)
{
    hostGroupBox->layout()->addWidget(&tagControl);

    tagControl.SetTagDef(pTagDef);
    tagControl.SetTagState(defaultTagState);
}

//////////////////////////////////////////////////////////////////////////
CMannAdvancedPasteDialog::CMannAdvancedPasteDialog(const IAnimationDatabase& database, const FragmentID fragmentID, const SFragTagState& tagState, QWidget* parent)
    : QDialog(parent)
    , m_fragmentID(fragmentID)
    , m_database(database)
    , m_tagState(tagState)
    , m_ui(new Ui::MannAdvancedPasteDialog)
{
    OnInitDialog();
}

CMannAdvancedPasteDialog::~CMannAdvancedPasteDialog()
{
}

//////////////////////////////////////////////////////////////////////////
void CMannAdvancedPasteDialog::OnInitDialog()
{
    m_ui->setupUi(this);

    const stack_string fragmentName = m_database.GetFragmentDefs().GetTagName(m_fragmentID);
    const stack_string windowName = stack_string().Format("Pasting fragments in '%s'", fragmentName.c_str());
    setWindowTitle(windowName.c_str());

    // Create the scope alias selection control
    CreateTagControl(m_globalTagsSelectionControl, IDC_GROUPBOX_GLOBALTAGS, m_ui->m_globalTagsGroupBox, &m_database.GetTagDefs(), m_tagState.globalTags);
    CreateTagControl(m_fragTagsSelectionControl, IDC_GROUPBOX_FRAGMENTTAGS, m_ui->m_fragTagsGroupBox, m_database.GetTagDefs().GetSubTagDefinition(m_fragmentID), m_tagState.fragmentTags);
}

void CMannAdvancedPasteDialog::accept()
{
    m_tagState.globalTags = m_globalTagsSelectionControl.GetTagState();
    m_tagState.fragmentTags = m_fragTagsSelectionControl.GetTagState();

    QDialog::accept();
}

#include <Mannequin/MannAdvancedPasteDialog.moc>