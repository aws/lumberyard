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
#ifndef CRYINCLUDE_EDITOR_VIEWPANEMANAGER_H
#define CRYINCLUDE_EDITOR_VIEWPANEMANAGER_H

struct SViewPaneDesc;
class QWidget;

class CViewPaneManager
{
public:
    CViewPaneManager();

    void Install(CWnd* window, bool themedFloatingFrames);
    void SetDockingHelpers(bool dockingHelpers);
    void SetTheme(int xtpTheme);
    LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);

    void StoreDefaultLayout();
    void RestoreDefaultLayout();
    void SaveLayoutToRegistry(const char* sectionName);
    bool LoadLayoutFromRegistry(const char* sectionName);
    void SaveLayoutToFile(const char* fullFilename, const char* layoutName);
    void LoadLayoutFromFile(const char* fullFilename, const char* layoutName);

    void SavePaneHistoryToRegistry();
    void LoadPaneHistoryFromRegistry();

    void CreatePane(CWnd* paneWindow, int id, const char* title, const CRect& rect, int dockTo);
    CWnd* OpenPane(const char* paneClassName, bool reuseOpened);
    CWnd* OpenPaneByTitle(const char* paneTitle);
    CWnd* FindPane(const char* paneClassName);
    CWnd* FindPaneByTitle(const char* title);

    QWidget* FindWidgetPaneByTitle(const char* title);
    void CloseAllOpenedTools();

    bool TogglePane(int id);
    bool IsPaneOpen(int id);
    void RedrawPanes();

    // special for CRenderViewport
    CFrameWnd* FindFrameWndByPaneID(int id);
    void ReattachPane(int id);
    // specially for CSettingsManagerDialog
    CXTPDockingPaneManager* GetDockingPaneManager() { return &m_paneManager; }
protected:
    void SetLayout(const CXTPDockingPaneLayout& layout);
    void SetPane(int id, const SViewPaneDesc& desc);
    bool AttachToPane(CXTPDockingPane* pDockPane);
    bool IsPaneDocked(const char* paneClassName);
    SViewPaneDesc* FindPaneByCategory(const char* sPaneCategory);
    SViewPaneDesc* FindPaneByClass(IViewPaneClass* pClass);
    CXTPDockingPane* FindDockedPaneByTitle(const char* windowTitle);

    CXTPDockingPaneManager m_paneManager;
    CXTPDockingPaneLayout* m_pDefaultLayout;

    typedef std::vector<CString> TDefaultPaneNames;
    TDefaultPaneNames m_defaultPaneNames;

    typedef std::map<int, SViewPaneDesc> PanesMap;
    PanesMap m_panesMap;

    typedef std::map<int, CWnd*> DefaultPaneWindows;
    DefaultPaneWindows m_defaultPaneWindows;

    std::set<CXTPDockingPane*> m_enteredPanes;

    bool m_isLoadingLayout;
    std::set<CXTPDockingPane*> m_pendingPanes;

    struct SPaneHistory
    {
        CRect rect; // Last known size of this panel.
        XTPDockingPaneDirection dockDir; // Last known dock style.
    };
    typedef std::map<CString, SPaneHistory> PanesHistoryMap;
    PanesHistoryMap m_panesHistoryMap;
};

#endif // CRYINCLUDE_EDITOR_VIEWPANEMANAGER_H
