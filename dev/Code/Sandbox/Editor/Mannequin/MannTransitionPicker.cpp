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
#include "MannTransitionPicker.h"
#include "MannequinDialog.h"
#include <Mannequin/ui_MannTransitionPickerDialog.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>

#include "StringDlg.h"

#include <ICryMannequin.h>
#include <IGameFramework.h>
#include <ICryMannequinEditor.h>


//////////////////////////////////////////////////////////////////////////
CMannTransitionPickerDlg::CMannTransitionPickerDlg(FragmentID& fromFragID, FragmentID& toFragID, SFragTagState& fromFragTag, SFragTagState& toFragTag, QWidget* pParent)
    : QDialog(pParent)
    , m_ui(new Ui::MannTransitionPickerDialog)
    , m_IDFrom(fromFragID)
    , m_IDTo(toFragID)
    , m_TagsFrom(fromFragTag)
    , m_TagsTo(toFragTag)
{
    m_ui->setupUi(this);
    PopulateComboBoxes();

    connect(m_ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &CMannTransitionPickerDlg::OnOk);
    connect(m_ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &QDialog::reject);

    setFixedSize(size());
}

//////////////////////////////////////////////////////////////////////////
CMannTransitionPickerDlg::~CMannTransitionPickerDlg()
{
}

//////////////////////////////////////////////////////////////////////////
void CMannTransitionPickerDlg::UpdateFragmentIDs()
{
    // Setup the combobox lookup values
    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();
    const SControllerDef* pControllerDef = pContexts->m_controllerDef;

    // retrieve per-detail values now
    int fromIdx = m_ui->FRAGMENTID_FROM_COMBO->currentIndex();
    if (fromIdx >= 0 && fromIdx < m_ui->FRAGMENTID_FROM_COMBO->count())
    {
        QString fromText = m_ui->FRAGMENTID_FROM_COMBO->currentText();
        m_IDFrom = pControllerDef->m_fragmentIDs.Find(fromText.toStdString().c_str());
    }

    int toIdx = m_ui->FRAGMENTID_TO_COMBO->currentIndex();
    if (toIdx >= 0 && toIdx < m_ui->FRAGMENTID_TO_COMBO->count())
    {
        QString toText = m_ui->FRAGMENTID_TO_COMBO->currentText();
        m_IDTo = pControllerDef->m_fragmentIDs.Find(toText.toStdString().c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTransitionPickerDlg::PopulateComboBoxes()
{
    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();
    const SControllerDef* pControllerDef = pContexts->m_controllerDef;

    // figure out what scopes we've got available
    ActionScopes scopesFrom = ACTION_SCOPES_NONE;
    ActionScopes scopesTo       = ACTION_SCOPES_NONE;
    if (TAG_ID_INVALID != m_IDFrom)
    {
        scopesFrom = pControllerDef->GetScopeMask(m_IDFrom, m_TagsFrom);
    }
    if (TAG_ID_INVALID != m_IDTo)
    {
        scopesTo = pControllerDef->GetScopeMask(m_IDTo, m_TagsTo);
    }

    const ActionScopes scopes = ((scopesFrom | scopesTo) == ACTION_SCOPES_NONE) ? ACTION_SCOPES_ALL : (scopesFrom | scopesTo);

    // Add two blank strings
    m_ui->FRAGMENTID_FROM_COMBO->addItem("");
    m_ui->FRAGMENTID_TO_COMBO->addItem("");

    // populate the combo boxes filtered by the scopes
    const TagID numTags = pControllerDef->m_fragmentIDs.GetNum();
    for (TagID itag = 0; itag < numTags; itag++)
    {
        const bool bValidFromScope  = (pControllerDef->GetScopeMask(itag, m_TagsFrom) & scopes) != ACTION_SCOPES_NONE;
        const bool bValidToScope        = (pControllerDef->GetScopeMask(itag, m_TagsTo) & scopes) != ACTION_SCOPES_NONE;
        const char* pName = pControllerDef->m_fragmentIDs.GetTagName(itag);

        if (bValidFromScope)
        {
            m_ui->FRAGMENTID_FROM_COMBO->addItem(pName);
        }
        if (bValidToScope)
        {
            m_ui->FRAGMENTID_TO_COMBO->addItem(pName);
        }
    }

    // select the current fragment ID if there's a valid one
    if (TAG_ID_INVALID != m_IDFrom)
    {
        m_ui->FRAGMENTID_FROM_COMBO->setEnabled(false);
        m_ui->FRAGMENTID_FROM_COMBO->setCurrentIndex(m_ui->FRAGMENTID_FROM_COMBO->findText(QString(pControllerDef->m_fragmentIDs.GetTagName(m_IDFrom))));
    }

    if (TAG_ID_INVALID != m_IDTo)
    {
        m_ui->FRAGMENTID_TO_COMBO->setEnabled(false);
        m_ui->FRAGMENTID_TO_COMBO->setCurrentIndex(m_ui->FRAGMENTID_FROM_COMBO->findText(QString(pControllerDef->m_fragmentIDs.GetTagName(m_IDTo))));
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTransitionPickerDlg::OnOk()
{
    UpdateFragmentIDs();

    // Validate and provide feedback on what went wrong
    if (m_IDTo == TAG_ID_INVALID && m_IDFrom == TAG_ID_INVALID)
    {
        QMessageBox::warning(this, tr("Validation"), tr("Please choose fragment IDs to preview the transition to and from."), QMessageBox::Ok);
    }
    else if (m_IDFrom == TAG_ID_INVALID)
    {
        QMessageBox::warning(this, tr("Validation"), tr("Please choose a fragment ID to preview the transition from."), QMessageBox::Ok);
    }
    else if (m_IDTo == TAG_ID_INVALID)
    {
        QMessageBox::warning(this, tr("Validation"), tr("Please choose a fragment ID to preview the transition to."), QMessageBox::Ok);
    }
    else
    {
        // if everything is ok with the data entered.
        QDialog::accept();
    }
}


#include <Mannequin/MannTransitionPicker.moc>