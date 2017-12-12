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

// Description : implementation of the CLoadingScreen class.


#include "stdafx.h"
/*
#include "stdafx.h"
#include "LoadingScreen.h"


// Static member variables
CLoadingDialog CLoadingScreen::m_cLoadingDialog;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLoadingScreen::CLoadingScreen()
{

}

CLoadingScreen::~CLoadingScreen()
{
    // Hide the screen
    CLoadingScreen::Hide();
}

void CLoadingScreen::Show()
{
    ////////////////////////////////////////////////////////////////////////
    // Show the loading screen and register it in CLogFile
    ////////////////////////////////////////////////////////////////////////

    // Display the modelless loading dialog
    VERIFY(m_cLoadingDialog.Create(IDD_LOADING));
    m_cLoadingDialog.ShowWindow(SW_SHOWNORMAL);
    m_cLoadingDialog.UpdateWindow();

    // Register the listbox control for receiving the log file entries
    CLogFile::AttachListBox(m_cLoadingDialog.GetDlgItem(IDC_CONSOLE_OUTPUT)->m_hWnd);

    ::SetWindowPos(m_cLoadingDialog.m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void CLoadingScreen::Hide()
{
    ////////////////////////////////////////////////////////////////////////
    // Hide the loading screen and register it in CLogFile
    ////////////////////////////////////////////////////////////////////////

    // Unregister the listbox control
    CLogFile::AttachListBox(NULL);

    // Destroy the dialog window
    m_cLoadingDialog.DestroyWindow();
}
*/