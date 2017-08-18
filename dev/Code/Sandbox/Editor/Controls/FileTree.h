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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_FILETREE_H
#define CRYINCLUDE_EDITOR_CONTROLS_FILETREE_H
#pragma once

// FileTree.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFileTree window

class CFileTree
    : public CTreeCtrl
{
    // Construction
public:
    CFileTree();

    // Attributes
public:

    // Operations
public:

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFileTree)
protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

    // Implementation
public:
    CString GetSelectedFile();
    virtual ~CFileTree();

    // Sets the search pattern that decides which files should be displayed
    void SetFileSpec(CString strFileSpec) { m_strFileSpec = strFileSpec; };

    // Spezifies the root folder of the search, it is constructed as follows:
    // X:\%MasterCD%\%SearchFolder%
    void SetSearchFolder(CString strSearchFolder) { m_strSearchFolder = strSearchFolder; };
    CString GetSearchFolder() { return m_strSearchFolder; };

    // Generated message map functions
protected:
    //{{AFX_MSG(CFileTree)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG

    CImageList m_cImageList;

    void RecurseDirectory(char szFolder[_MAX_PATH], HTREEITEM hRoot, PSTR pszFileSpec);
    typedef std::map<HTREEITEM, CString> Files;
    Files m_cFileNames;

    CString m_strFileSpec;
    CString m_strSearchFolder;

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_CONTROLS_FILETREE_H
