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

#ifndef CRYINCLUDE_EDITOR_DIALOGS_BASEFRAMEWND_H
#define CRYINCLUDE_EDITOR_DIALOGS_BASEFRAMEWND_H
#pragma once


//////////////////////////////////////////////////////////////////////////
//
// Base class for Sandbox frame windows
//
//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CBaseFrameWnd
    : public CXTPFrameWnd
{
public:
    CBaseFrameWnd();

    CXTPDockingPaneManager* GetDockingPaneManager() { return &m_paneManager; }
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd);

    // Will automatically save and restore frame docking layout.
    // Place at the end of the window dock panes creation.
    bool AutoLoadFrameLayout(const CString& name, int nVersion = 0);
    // Every docking pane windows should be registered with this function
    CXTPDockingPane* CreateDockingPane(const char* sPaneTitle, CWnd* pWindow, UINT nID, CRect rc, XTPDockingPaneDirection direction, CXTPDockingPaneBase* pNeighbour = NULL);

protected:

    // This methods are used to save and load layout of the frame to the registry.
    // Activated by the call to AutoSaveFrameLayout
    virtual bool LoadFrameLayout();
    virtual void SaveFrameLayout();

    // Override this to provide code to cancel a window close, e.g. for unsaved changes
    virtual bool CanClose() { return true; }

    DECLARE_MESSAGE_MAP()

    virtual void OnOK() {};
    virtual void OnCancel() {};
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void PostNcDestroy();

    afx_msg void OnDestroy();

    //////////////////////////////////////////////////////////////////////////
    // Implement in derived class
    //////////////////////////////////////////////////////////////////////////
    virtual BOOL OnInitDialog() { return TRUE; };
    virtual LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnFrameCanClose(WPARAM wParam, LPARAM lParam);
    //////////////////////////////////////////////////////////////////////////

protected:
    // Docking panes manager.
    CXTPDockingPaneManager m_paneManager;
    CString m_frameLayoutKey;
    int m_frameLayoutVersion;

    std::vector< std::pair<int, CWnd*> > m_dockingPaneWindows;
};

#endif // CRYINCLUDE_EDITOR_DIALOGS_BASEFRAMEWND_H
