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
#include "SequencerKeyPropertiesDlg.h"
#include "SequencerSequence.h"
#include "AnimationContext.h"

#include <Mannequin/ui_SequencerKeyPropertiesDlg.h>

//////////////////////////////////////////////////////////////////////////
void CSequencerKeyUIControls::OnInternalVariableChange(IVariable* pVar)
{
    SelectedKeys keys;
    CSequencerUtils::GetSelectedKeys(m_pKeyPropertiesDlg->GetSequence(), keys);

    OnUIChange(pVar, keys);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerKeyUIControls::RefreshSequencerKeys()
{
    GetIEditor()->Notify(eNotify_OnUpdateSequencerKeys);
}

//////////////////////////////////////////////////////////////////////////
CSequencerKeyPropertiesDlg::CSequencerKeyPropertiesDlg(QWidget* parent, bool overrideConstruct)
    : QWidget(parent)
    , m_pLastTrackSelected(NULL)
    , m_wndProps(new ReflectedPropertyControl(this))
    , m_wndTrackProps(new CSequencerTrackPropsDlg(this))
{
    m_wndProps->Setup(false, 175);

    QBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_wndTrackProps);
    m_wndProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(m_wndProps);
    layout->addStretch(1);

    m_pVarBlock = new CVarBlock;

    if (!overrideConstruct)
    {
        CreateAllVars();
    }
}

CSequencerKeyPropertiesDlg::~CSequencerKeyPropertiesDlg()
{
}

//////////////////////////////////////////////////////////////////////////
//BOOL CSequencerKeyPropertiesDlg::PreTranslateMessage( MSG* pMsg )
//{
//if (pMsg->message == WM_KEYDOWN)
//{
//  // In case of arrow keys, pass the message to keys window so that it can handle
//  // a key selection change by arrow keys.
//  int nVirtKey = (int) pMsg->wParam;
//  if (nVirtKey == VK_UP || nVirtKey == VK_DOWN || nVirtKey == VK_RIGHT || nVirtKey == VK_LEFT)
//  {
//      if (m_keysCtrl)
//          m_keysCtrl->SendMessage(WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
//  }
//}

//return QWidget::PreTranslateMessage(pMsg);
//return false;
//}

//////////////////////////////////////////////////////////////////////////
void CSequencerKeyPropertiesDlg::OnVarChange(IVariable* pVar)
{
}

//////////////////////////////////////////////////////////////////////////
void CSequencerKeyPropertiesDlg::CreateAllVars()
{
    for (int i = 0; i < (int)m_keyControls.size(); i++)
    {
        m_keyControls[i]->SetKeyPropertiesDlg(this);
        m_keyControls[i]->CreateVars();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerKeyPropertiesDlg::PopulateVariables()
{
    //SetVarBlock( m_pVarBlock,functor(*this,&CSequencerKeyPropertiesDlg::OnVarChange) );

    // Must first clear any selection in properties window.
    m_wndProps->ClearSelection();
    m_wndProps->RemoveAllItems();
    m_wndProps->AddVarBlock(m_pVarBlock);

    m_wndProps->SetUpdateCallback(functor(*this, &CSequencerKeyPropertiesDlg::OnVarChange));
    //m_wndProps->ExpandAll();


    ReloadValues();
}

//////////////////////////////////////////////////////////////////////////
void CSequencerKeyPropertiesDlg::PopulateVariables(ReflectedPropertyControl& propCtrl)
{
    propCtrl.ClearSelection();
    propCtrl.RemoveAllItems();
    propCtrl.AddVarBlock(m_pVarBlock);

    propCtrl.ReloadValues();
}

//////////////////////////////////////////////////////////////////////////
void CSequencerKeyPropertiesDlg::OnKeySelectionChange()
{
    CSequencerUtils::SelectedKeys selectedKeys;
    CSequencerUtils::GetSelectedKeys(GetSequence(), selectedKeys);

    if (m_wndTrackProps)
    {
        m_wndTrackProps->OnKeySelectionChange(selectedKeys);
    }

    bool bSelectChangedInSameTrack
        = m_pLastTrackSelected
            && selectedKeys.keys.size() == 1
            && selectedKeys.keys[0].pTrack == m_pLastTrackSelected;

    if (selectedKeys.keys.size() == 1)
    {
        m_pLastTrackSelected = selectedKeys.keys[0].pTrack;
    }
    else
    {
        m_pLastTrackSelected = NULL;
    }

    if (bSelectChangedInSameTrack == false)
    {
        m_pVarBlock->DeleteAllVariables();
    }

    bool bAssigned = false;
    if (selectedKeys.keys.size() > 0)
    {
        for (int i = 0; i < (int)m_keyControls.size(); i++)
        {
            const ESequencerParamType valueType = selectedKeys.keys[0].pTrack->GetParameterType();
            if (m_keyControls[i]->SupportTrackType(valueType))
            {
                if (bSelectChangedInSameTrack == false)
                {
                    AddVars(m_keyControls[i]);
                }
                if (m_keyControls[i]->OnKeySelectionChange(selectedKeys))
                {
                    bAssigned = true;
                }

                break;
            }
        }
        m_wndProps->setEnabled(true);
    }
    else
    {
        m_wndProps->setEnabled(false);
    }

    if (bSelectChangedInSameTrack)
    {
        m_wndProps->ClearSelection();
        ReloadValues();
    }
    else
    {
        PopulateVariables();
    }

    if (selectedKeys.keys.size() > 1 || !bAssigned)
    {
        m_wndProps->SetDisplayOnlyModified(true);
    }
    else
    {
        m_wndProps->SetDisplayOnlyModified(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerKeyPropertiesDlg::AddVars(CSequencerKeyUIControls* pUI)
{
    CVarBlock* pVB = pUI->GetVarBlock();
    for (int i = 0, num = pVB->GetNumVariables(); i < num; i++)
    {
        IVariable* pVar = pVB->GetVariable(i);
        m_pVarBlock->AddVariable(pVar);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequencerKeyPropertiesDlg::ReloadValues()
{
    if (m_wndProps)
    {
        m_wndProps->ReloadValues();
    }
}

void CSequencerKeyPropertiesDlg::SetSequence(CSequencerSequence* pSequence)
{
    m_pSequence = pSequence;
    m_wndTrackProps->SetSequence(pSequence);
}


void CSequencerKeyPropertiesDlg::SetVarValue(const char* varName, const char* varValue)
{
    if (varName == NULL || varName[ 0 ] == 0)
    {
        return;
    }

    if (varValue == NULL || varValue[ 0 ] == 0)
    {
        return;
    }

    if (!m_pVarBlock)
    {
        return;
    }

    IVariable* pVar = NULL;

    string varNameList = varName;
    int start = 0;
    string token = varNameList.Tokenize(".", start);
    CRY_ASSERT(!token.empty());

    while (!token.empty())
    {
        if (pVar == NULL)
        {
            const bool recursiveSearch = true;
            pVar = m_pVarBlock->FindVariable(token.c_str(), recursiveSearch);
        }
        else
        {
            const bool recursiveSearch = false;
            pVar = pVar->FindVariable(token.c_str(), recursiveSearch);
        }

        if (pVar == NULL)
        {
            return;
        }

        token = varNameList.Tokenize(".", start);
    }

    pVar->Set(varValue);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CSequencerTrackPropsDlg::CSequencerTrackPropsDlg(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::SequencerTrackPropsDlg)
    , m_inOnKeySelectionChange(false)
{
    m_ui->setupUi(this);

    m_track = 0;
    m_key = -1;

    connect(m_ui->m_keySpinBtn, SIGNAL(valueChanged(int)), this, SLOT(OnDeltaposPrevnext()), Qt::QueuedConnection);
    connect(m_ui->m_time, SIGNAL(valueChanged(double)), this, SLOT(OnUpdateTime()));
}

CSequencerTrackPropsDlg::~CSequencerTrackPropsDlg()
{
}

//////////////////////////////////////////////////////////////////////////
void CSequencerTrackPropsDlg::SetSequence(CSequencerSequence* pSequence)
{
    if (pSequence)
    {
        Range range = pSequence->GetTimeRange();
        m_ui->m_time->setRange(range.start, range.end);
    }
}


//////////////////////////////////////////////////////////////////////////
bool CSequencerTrackPropsDlg::OnKeySelectionChange(CSequencerUtils::SelectedKeys& selectedKeys)
{
    if (m_inOnKeySelectionChange)
    { 
        return false;
    }
    m_inOnKeySelectionChange = true;
    m_track = 0;
    m_key = 0;

    if (selectedKeys.keys.size() == 1)
    {
        m_track = selectedKeys.keys[0].pTrack;
        m_key = selectedKeys.keys[0].nKey;
    }

    if (m_track != NULL)
    {
        m_ui->m_time->setValue(m_track->GetKeyTime(m_key));
        m_ui->m_keySpinBtn->setRange(1, m_track->GetNumKeys());
        m_ui->m_keySpinBtn->setValue(m_key + 1);

        m_ui->m_keySpinBtn->setEnabled(true);
        m_ui->m_time->setEnabled(true);
    }
    else
    {
        m_ui->m_keySpinBtn->setEnabled(false);
        m_ui->m_time->setEnabled(false);
    }
    m_inOnKeySelectionChange = false;
    return true;
}

void CSequencerTrackPropsDlg::OnDeltaposPrevnext()
{
    if (!m_track)
    {
        return;
    }

    int nkey = m_ui->m_keySpinBtn->value() - 1;
    if (nkey < 0)
    {
        nkey = m_track->GetNumKeys() - 1;
    }
    if (nkey > m_track->GetNumKeys() - 1)
    {
        nkey = 0;
    }

    SetCurrKey(nkey);
}

void CSequencerTrackPropsDlg::OnUpdateTime()
{
    if (!m_track)
    {
        return;
    }

    if (m_key < 0 || m_key >= m_track->GetNumKeys())
    {
        return;
    }

    float time = m_ui->m_time->value();
    m_track->SetKeyTime(m_key, time);
    m_track->SortKeys();

    int k = m_track->FindKey(time);
    if (k != m_key)
    {
        SetCurrKey(k);
    }
}

void CSequencerTrackPropsDlg::SetCurrKey(int nkey)
{
    if (m_key >= 0 && m_key < m_track->GetNumKeys())
    {
        m_track->SelectKey(m_key, false);
    }
    if (nkey >= 0 && nkey < m_track->GetNumKeys())
    {
        m_track->SelectKey(nkey, true);
    }
    GetIEditor()->Notify(eNotify_OnUpdateSequencerKeySelection);
}

#include <Mannequin/SequencerKeyPropertiesDlg.moc>