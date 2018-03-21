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
#include "MannTransitionSettings.h"
#include "MannequinDialog.h"
#include "QtViewPaneManager.h"
#include "StringDlg.h"
#include "QtUtil.h"
#include <Mannequin/ui_MannTransitionSettings.h>

#include <ICryMannequin.h>
#include <IGameFramework.h>
#include <ICryMannequinEditor.h>

//////////////////////////////////////////////////////////////////////////
CMannTransitionSettingsDlg::CMannTransitionSettingsDlg(const QString& windowCaption, FragmentID& fromFragID, FragmentID& toFragID, SFragTagState& fromFragTag, SFragTagState& toFragTag, QWidget* pParent)
    : QDialog(pParent)
    , m_IDFrom(fromFragID)
    , m_IDTo(toFragID)
    , m_TagsFrom(fromFragTag)
    , m_TagsTo(toFragTag)
    , m_tagFromPanelFragID(FRAGMENT_ID_INVALID)
    , m_tagToPanelFragID(FRAGMENT_ID_INVALID)
{
    OnInitDialog();
    setWindowTitle(windowCaption);
}

//////////////////////////////////////////////////////////////////////////
CMannTransitionSettingsDlg::~CMannTransitionSettingsDlg()
{
    ui->widget->setParent(0);
}

//////////////////////////////////////////////////////////////////////////
bool CMannTransitionSettingsDlg::OnInitDialog()
{
    ui.reset(new Ui::CMannTransitionSettingsDlg);
    ui->setupUi(this);

    m_tagsFromPanel.Setup();
    m_tagsToPanel.Setup();

    QHBoxLayout* l = new QHBoxLayout;
    l->addWidget(&m_tagsFromPanel);
    l->addWidget(&m_tagsToPanel);
    ui->widget->setLayout(l);

    connect(ui->FRAGMENTID_FROM_COMBO, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CMannTransitionSettingsDlg::OnComboChange);
    connect(ui->FRAGMENTID_TO_COMBO, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CMannTransitionSettingsDlg::OnComboChange);

    connect(ui->CANCELBTN, &QPushButton::clicked, this, &CMannTransitionSettingsDlg::reject);
    connect(ui->OKBTN, &QPushButton::clicked, this, &CMannTransitionSettingsDlg::OnOk);

    m_tagFromVars.reset(new CVarBlock());

    m_tagToVars.reset(new CVarBlock());

    PopulateControls();

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CMannTransitionSettingsDlg::UpdateFragmentIDs()
{
    // Setup the combobox lookup values
    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();
    const SControllerDef* pControllerDef = pContexts->m_controllerDef;

    // retrieve per-detail values now
    {
        const int selIndex = ui->FRAGMENTID_FROM_COMBO->currentIndex();
        if (selIndex >= 0 && selIndex < ui->FRAGMENTID_FROM_COMBO->count())
        {
            QString fromName = ui->FRAGMENTID_FROM_COMBO->itemText(selIndex);
            m_IDFrom = pControllerDef ? pControllerDef->m_fragmentIDs.Find(fromName.toUtf8().data()) : -1;
        }
    }

    {
        const int selIndex = ui->FRAGMENTID_TO_COMBO->currentIndex();
        if (selIndex >= 0 && selIndex < ui->FRAGMENTID_TO_COMBO->count())
        {
            QString fromName = ui->FRAGMENTID_TO_COMBO->itemText(selIndex);
            m_IDTo = pControllerDef ? pControllerDef->m_fragmentIDs.Find(fromName.toUtf8().data()) : -1;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTransitionSettingsDlg::PopulateComboBoxes()
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

    QSignalBlocker fromComboBlocker(ui->FRAGMENTID_FROM_COMBO);
    QSignalBlocker toComboBlocker(ui->FRAGMENTID_TO_COMBO);

    // Add two blank strings
    ui->FRAGMENTID_FROM_COMBO->addItem("");
    ui->FRAGMENTID_TO_COMBO->addItem("");

    // populate the combo boxes filtered by the scopes
    const TagID numTags = pControllerDef ? pControllerDef->m_fragmentIDs.GetNum() : 0;
    for (TagID itag = 0; itag < numTags; itag++)
    {
        const bool bValidFromScope  = (pControllerDef->GetScopeMask(itag, m_TagsFrom) & scopes) != ACTION_SCOPES_NONE;
        const bool bValidToScope        = (pControllerDef->GetScopeMask(itag, m_TagsTo) & scopes) != ACTION_SCOPES_NONE;
        const char* pName = pControllerDef->m_fragmentIDs.GetTagName(itag);

        if (bValidFromScope)
        {
            ui->FRAGMENTID_FROM_COMBO->addItem(pName);
        }
        if (bValidToScope)
        {
            ui->FRAGMENTID_TO_COMBO->addItem(pName);
        }
    }

    // select the current fragment ID if there's a valid one
    if (TAG_ID_INVALID != m_IDFrom)
    {
        ui->FRAGMENTID_FROM_COMBO->setCurrentText(pControllerDef->m_fragmentIDs.GetTagName(m_IDFrom));
    }

    if (TAG_ID_INVALID != m_IDTo)
    {
        ui->FRAGMENTID_TO_COMBO->setCurrentText(pControllerDef->m_fragmentIDs.GetTagName(m_IDTo));
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTransitionSettingsDlg::PopulateTagPanels()
{
    // Setup the combobox lookup values
    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();
    const SControllerDef* pControllerDef = pContexts->m_controllerDef;

    UpdateFragmentIDs();

    // Repopulate tag panels if fragment ID has changed from last populate
    if (m_tagFromPanelFragID != m_IDFrom)
    {
        m_tagFromVars->DeleteAllVariables();
        m_tagsFromPanel.DeleteVars();
        m_tagFromControls.Clear();
        m_tagFromFragControls.Clear();

        if (m_IDFrom != FRAGMENT_ID_INVALID)
        {
            m_tagFromControls.Init(m_tagFromVars, pControllerDef->m_tags);
            m_tagFromControls.Set(m_TagsFrom.globalTags);

            const CTagDefinition* pFromFragTags = (m_IDFrom == FRAGMENT_ID_INVALID) ? NULL : pControllerDef->GetFragmentTagDef(m_IDFrom);
            if (pFromFragTags)
            {
                m_tagFromFragControls.Init(m_tagFromVars, *pFromFragTags);
                m_tagFromFragControls.Set(m_TagsFrom.fragmentTags);
            }

            m_tagsFromPanel.SetVarBlock(m_tagFromVars.get(), NULL);
        }
        m_tagFromPanelFragID = m_IDFrom;
    }

    if (m_tagToPanelFragID != m_IDTo)
    {
        m_tagToVars->DeleteAllVariables();
        m_tagsToPanel.DeleteVars();
        m_tagToControls.Clear();
        m_tagToFragControls.Clear();

        if (m_IDTo != FRAGMENT_ID_INVALID)
        {
            m_tagToControls.Init(m_tagToVars, pControllerDef->m_tags);
            m_tagToControls.Set(m_TagsTo.globalTags);

            const CTagDefinition* pToFragTags = (m_IDTo == FRAGMENT_ID_INVALID) ? NULL : pControllerDef->GetFragmentTagDef(m_IDTo);
            if (pToFragTags)
            {
                m_tagToFragControls.Init(m_tagToVars, *pToFragTags);
                m_tagToFragControls.Set(m_TagsTo.fragmentTags);
            }

            m_tagsToPanel.SetVarBlock(m_tagToVars.get(), NULL);
        }
        m_tagToPanelFragID = m_IDTo;
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannTransitionSettingsDlg::PopulateControls()
{
    PopulateComboBoxes();
    PopulateTagPanels();
}

//////////////////////////////////////////////////////////////////////////
void CMannTransitionSettingsDlg::OnOk()
{
    UpdateFragmentIDs();

    // retrieve per-detail values now
    m_TagsFrom.globalTags = m_tagFromControls.Get();
    m_TagsFrom.fragmentTags = m_tagFromFragControls.Get();

    m_TagsTo.globalTags = m_tagToControls.Get();
    m_TagsTo.fragmentTags = m_tagToFragControls.Get();

    accept();
}

void CMannTransitionSettingsDlg::OnComboChange()
{
    UpdateFragmentIDs();
    PopulateTagPanels();
}

#include <Mannequin/MannTransitionSettings.moc>
