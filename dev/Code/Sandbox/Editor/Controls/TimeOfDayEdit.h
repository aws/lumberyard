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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_TIMEOFDAYEDIT_H
#define CRYINCLUDE_EDITOR_CONTROLS_TIMEOFDAYEDIT_H
#pragma once


#define TIMEOFDAYN_CHANGE  0x0800

class CTimeOfDayEdit
    : public CXTTimeEdit
{
public:
    CTimeOfDayEdit();

    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual bool ProcessMask(UINT& nChar, int nEndPos);

    COLORREF GetTextColor();
    bool IsCompleteTimeStr(CString szText);
    CString ForceMakeComleteValue(CString szText);

    void SetTime(float time);
    float GetTime() const { return m_fCurrentTime; }

    // Use SetTime(float) instead
    virtual void SetTime(int hours, int minutes) { assert(false); };

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnPaint();
    afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);

    bool m_bProcessingEvent;
    bool m_bValidTimeValue;
    CBrush m_EditBrush;
    float m_fCurrentTime;
    DWORD m_lastBackgroundColor;
};


#endif // CRYINCLUDE_EDITOR_CONTROLS_TIMEOFDAYEDIT_H
