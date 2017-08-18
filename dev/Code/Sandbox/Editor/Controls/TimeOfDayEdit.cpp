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
#include "TimeOfDayEdit.h"

BEGIN_MESSAGE_MAP(CTimeOfDayEdit, CXTTimeEdit)
ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()

CTimeOfDayEdit::CTimeOfDayEdit()
{
    m_bMilitary = true;
    m_bValidTimeValue = true;
    DWORD m_lastBackgroundColor = GetSysColor(COLOR_WINDOW);
    m_EditBrush.CreateSolidBrush(m_lastBackgroundColor);
    m_fCurrentTime = -1.f;
}

bool IsNumberKey(WPARAM wParam)
{
    // Check if wParam is a number (numpad or number keys which are represented by ASCII '0' - '9')
    if (wParam == VK_NUMPAD0 ||
        wParam == VK_NUMPAD1 ||
        wParam == VK_NUMPAD2 ||
        wParam == VK_NUMPAD3 ||
        wParam == VK_NUMPAD4 ||
        wParam == VK_NUMPAD5 ||
        wParam == VK_NUMPAD6 ||
        wParam == VK_NUMPAD7 ||
        wParam == VK_NUMPAD8 ||
        wParam == VK_NUMPAD9 ||
        wParam == 0x30 ||
        wParam == 0x31 ||
        wParam == 0x32 ||
        wParam == 0x33 ||
        wParam == 0x34 ||
        wParam == 0x35 ||
        wParam == 0x36 ||
        wParam == 0x37 ||
        wParam == 0x38 ||
        wParam == 0x39)
    {
        return true;
    }
    return false;
}

BOOL CTimeOfDayEdit::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP)
    {
        if (IsNumberKey(pMsg->wParam))
        {
            CString str;
            GetWindowText(str);

            if (IsCompleteTimeStr(str))
            {
                uint hour = 0;
                uint minute = 0;
                m_bValidTimeValue = true;

                sscanf(str, _T("%02d:%02d"), &hour, &minute);
                const float fTime = (float)hour + (float)minute / 60.0f;

                SetTime(fTime);
            }
            else
            {
                m_bValidTimeValue = false;
            }
        }
    }

    return CXTTimeEdit::PreTranslateMessage(pMsg);
}

bool CTimeOfDayEdit::ProcessMask(UINT& nChar, int nEndPos)
{
    // check the key against the mask
    switch (m_strMask.GetAt(nEndPos))
    {
    case '0':       // digit only //completely changed this
    {
        if (_istdigit((TCHAR)nChar))
        {
            switch (nEndPos)
            {
            case 0:
                if (m_bMilitary)
                {
                    if (nChar < 48 || nChar > 50)
                    {
                        MessageBeep((UINT)-1);
                        return false;
                    }
                }
                else
                {
                    if (nChar < 48 || nChar > 49)
                    {
                        MessageBeep((UINT)-1);
                        return false;
                    }
                }
                break;

            case 1:
                if (m_bMilitary)
                {
                    if (nChar < 48 || nChar > 57)
                    {
                        MessageBeep((UINT)-1);
                        return false;
                    }
                }
                else
                {
                    if (nChar < 48 || nChar > 50)
                    {
                        MessageBeep((UINT)-1);
                        return false;
                    }
                }
                break;

            case 3:
                if (nChar < 48 || nChar > 53)
                {
                    MessageBeep((UINT)-1);
                    return false;
                }
                break;

            case 4:
                if (nChar < 48 || nChar > 57)
                {
                    MessageBeep((UINT)-1);
                    return false;
                }
                break;
            }

            return true;
        }
        break;
    }
    }

    MessageBeep((UINT)-1);
    return false;
}

COLORREF CTimeOfDayEdit::GetTextColor()
{
    if (!m_bValidTimeValue)
    {
        return RGB(68, 186, 231);
    }

    return GetSysColor(COLOR_WINDOWTEXT);
}

bool CTimeOfDayEdit::IsCompleteTimeStr(CString szText)
{
    int mid = szText.Find(":");
    CString szHour = szText.Left(mid);
    CString szMinute = szText.Right(mid);

    if (szHour.GetLength() != 2 || szMinute.GetLength() != 2 || isdigit(szHour[0]) == 0 || isdigit(szHour[1]) == 0 || isdigit(szMinute[0]) == 0 || isdigit(szMinute[1]) == 0)
    {
        return false;
    }

    return true;
}

CString CTimeOfDayEdit::ForceMakeComleteValue(CString szText)
{
    int mid = szText.Find(":");
    uint hour = atoi(szText.Left(mid));
    uint minute = atoi(szText.Right(mid));

    CString szCompleteValue;
    szCompleteValue.Format("%02d:%02d", hour, minute);

    return szCompleteValue;
}

void CTimeOfDayEdit::SetTime(float fTime)
{
    if (m_fCurrentTime != fTime)
    {
        m_fCurrentTime = fTime;
        ::SendMessage(GetOwner()->GetSafeHwnd(), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), TIMEOFDAYN_CHANGE), (LPARAM)GetSafeHwnd());

        int nHour = floor(fTime);
        int nMins = (fTime - floor(fTime)) * 60.0f;

        CXTTimeEdit::SetTime(nHour, nMins);
    }
}

HBRUSH CTimeOfDayEdit::CtlColor(CDC* pDC, UINT nCtlColor)
{
    pDC->SetTextColor(GetTextColor());
    pDC->SetBkColor(GetSysColor(COLOR_WINDOW));

    DWORD newColor = GetSysColor(COLOR_WINDOW);
    if (newColor != m_lastBackgroundColor)
    {
        m_lastBackgroundColor = newColor;
        m_EditBrush.DeleteObject();
        m_EditBrush.CreateSolidBrush(newColor);
    }

    return m_EditBrush;
}