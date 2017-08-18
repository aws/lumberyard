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
#include "stdafx.h"
#include "Resource.h"

#include "CustomizeKeyboardPage.h"

/////////////////////////////////////////////////////////////////////////////
// CustomizeKeyboardPage property page

CustomizeKeyboardPage::CustomizeKeyboardPage(CXTPCustomizeSheet* pSheet)
    : CXTPCustomizeKeyboardPage(pSheet)
{
}

CustomizeKeyboardPage::~CustomizeKeyboardPage()
{
}

BEGIN_MESSAGE_MAP(CustomizeKeyboardPage, CXTPCustomizeKeyboardPage)
//{{AFX_MSG_MAP(CustomizeKeyboardPage)
// NOTE: the ClassWizard will add message map macros here
//}}AFX_MSG_MAP
ON_BN_CLICKED(XTP_IDC_BTN_ASSIGN, OnAssign)
END_MESSAGE_MAP()

void CustomizeKeyboardPage::OnAssign()
{
    int iIndex = m_lboxCommands.GetCurSel();
    if (iIndex == -1)
    {
        return;
    }

    CXTPControl* pControl = (CXTPControl*)m_lboxCommands.GetItemDataPtr(iIndex);
    if (!pControl)
    {
        return;
    }

    CXTPShortcutManagerAccel* pAccel = m_editShortcutKey.GetAccel();
    if (!pAccel)
    {
        return;
    }

    pAccel->cmd = static_cast<WORD>(pControl->GetID());

    if (m_pSheet->GetCommandBars()->GetShortcutManager()->OnBeforeAdd(pAccel))
    {
        return;
    }

    CXTPShortcutManagerAccelTable* pAccelTable = GetFrameAccelerator();

    for (int i = 0; i < pAccelTable->GetCount(); i++)
    {
        CXTPShortcutManagerAccel* accel = pAccelTable->GetAt(i);
        if (CXTPShortcutManager::CKeyHelper::EqualAccels(accel, pAccel))
        {
            CString keyCmdStr;
            accel->m_pManager->FindDefaultAccelerator(accel->cmd, keyCmdStr);

            CXTPControl* pFoundControl = nullptr;
            for (int j = 0; j < m_lboxCommands.GetCount(); j++)
            {
                CXTPControl* pAltControl = (CXTPControl*)m_lboxCommands.GetItemDataPtr(j);
                if (pAltControl && pAltControl->GetID() == accel->cmd)
                {
                    pFoundControl = pAltControl;
                    break;
                }
            }

            CString strMessage;
            if (pFoundControl)
            {
                CString cmdName = pFoundControl->GetCaption();
                CXTPPaintManager::StripMnemonics(cmdName);
                strMessage.Format(CString(MAKEINTRESOURCE(IDS_CONFIRM_KEYBOARD_REASSIGN_CMD)), keyCmdStr, cmdName);
            }
            else
            {
                strMessage.Format(CString(MAKEINTRESOURCE(IDS_CONFIRM_KEYBOARD_REASSIGN_NO_CMD)), keyCmdStr);
            }

            if (XTPResourceManager()->ShowMessageBox(strMessage, MB_ICONWARNING | MB_YESNO) != IDYES)
            {
                return;
            }

            pAccelTable->RemoveAt(i);
            break;
        }
    }

    pAccelTable->Add(*pAccel);

    OnSelchangeCommands();
    m_editShortcutKey.ResetKey();
    EnableControls();
}