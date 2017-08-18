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
#include "HeaderCtrlEx.h"


BEGIN_MESSAGE_MAP(CHeaderCtrlEx, CHeaderCtrl)
//ON_NOTIFY_REFLECT(HDN_ENDTRACKW, OnEndTrack)
//ON_NOTIFY_REFLECT(HDN_ENDTRACKA, OnEndTrack)
//ON_NOTIFY_REFLECT(HDN_BEGINTRACKW, OnBeginTrack)
//ON_NOTIFY_REFLECT(HDN_BEGINTRACKA, OnBeginTrack)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
void CHeaderCtrlEx::OnEndTrack(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMHEADER* pIn = (NMHEADER*)pNMHDR;
    CWnd* pParent = GetParent();
    if (pParent)
    {
        NMHEADER notify = *pIn;
        notify.hdr.code = NM_HEADER_ENDTRACK;
        notify.hdr.hwndFrom = m_hWnd;
        notify.hdr.idFrom = GetDlgCtrlID();
        pParent->SendMessage(WM_NOTIFY, (WPARAM)GetDlgCtrlID(), (LPARAM)(&notify));
    }
}
//////////////////////////////////////////////////////////////////////////
void CHeaderCtrlEx::OnBeginTrack(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMHEADER* pIn = (NMHEADER*)pNMHDR;
    CWnd* pParent = GetParent();
    if (pParent)
    {
        NMHEADER notify = *pIn;
        notify.hdr.code = NM_HEADER_BEGINTRACK;
        notify.hdr.hwndFrom = m_hWnd;
        notify.hdr.idFrom = GetDlgCtrlID();
        pParent->SendMessage(WM_NOTIFY, (WPARAM)GetDlgCtrlID(), (LPARAM)(&notify));
    }
}
