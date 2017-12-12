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

#pragma once

class QWidget;
class QParentWndWidget;

class QtWindowWrapper
    : public CDialog
{
public:

    DECLARE_DYNAMIC(QtWindowWrapper)
    DECLARE_MESSAGE_MAP()

public:

    QtWindowWrapper(int PanelID, CWnd* pParent = NULL);

    afx_msg void OnSize(UINT nType, int cx, int cy);
    void OnOK() {};
    void OnCancel() {};

    void PostNcDestroy() { delete this; }

    void SetQtWindows(QWidget* pQtWnd, QParentWndWidget* pParentQtWnd);

private:

    QWidget* m_pQtWnd;
    QParentWndWidget* m_pParentQtWnd;
};
