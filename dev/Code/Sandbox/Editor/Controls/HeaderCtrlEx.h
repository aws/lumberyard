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

// Description : Extended ListCtrl Header control.


#ifndef CRYINCLUDE_EDITOR_CONTROLS_HEADERCTRLEX_H
#define CRYINCLUDE_EDITOR_CONTROLS_HEADERCTRLEX_H
#pragma once

// Notification sent to the parent when

#define NM_HEADER_ENDTRACK (NM_FIRST - 2001)
#define NM_HEADER_BEGINTRACK (NM_FIRST - 2002)

//////////////////////////////////////////////////////////////////////////
// Extended ListCtrl Header control.
//////////////////////////////////////////////////////////////////////////
class CHeaderCtrlEx
    : public CHeaderCtrl
{
public:
    CHeaderCtrlEx() {};
    ~CHeaderCtrlEx() {};

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnBeginTrack(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnEndTrack(NMHDR* pNMHDR, LRESULT* pResult);
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_HEADERCTRLEX_H
