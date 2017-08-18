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

#include "stdafx.h"
#include "FileTree.h"

#include <io.h>

/////////////////////////////////////////////////////////////////////////////
// CFileTree

CFileTree::CFileTree()
{
    CLogFile::WriteLine("File tree control created");

    m_strFileSpec = "*.cgf";
    m_strSearchFolder = "Objects";
}

CFileTree::~CFileTree()
{
    CLogFile::WriteLine("File tree control destoried");
}


BEGIN_MESSAGE_MAP(CFileTree, CTreeCtrl)
//{{AFX_MSG_MAP(CFileTree)
ON_WM_CREATE()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileTree message handlers

BOOL CFileTree::PreCreateWindow(CREATESTRUCT& cs)
{
    ////////////////////////////////////////////////////////////////////////
    // Modify the window styles
    ////////////////////////////////////////////////////////////////////////

    cs.style |= WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT;
    cs.dwExStyle |= WS_EX_CLIENTEDGE;

    return CTreeCtrl::PreCreateWindow(cs);
}

int CFileTree::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    ////////////////////////////////////////////////////////////////////////
    // Fill the tree control with the correct data
    ////////////////////////////////////////////////////////////////////////

    HTREEITEM hRoot;
    char szSearchPath[_MAX_PATH];
    char szRootName[_MAX_PATH + 512];

    if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    // Create the list
    //m_cImageList.Create(IDB_TREE_VIEW, 16, 1, RGB (255, 0, 255));
    CMFCUtils::LoadTrueColorImageList(m_cImageList, IDB_TREE_VIEW, 16, RGB(255, 0, 255));

    // Attach it to the control
    SetImageList(&m_cImageList, TVSIL_NORMAL);

    // Create name of the root
    sprintf_s(szRootName, "Master CD %s Folder", m_strSearchFolder.GetBuffer(0));

    // Add the Master CD folder to the list
    hRoot = InsertItem(szRootName, 2, 2, TVI_ROOT);

    // Create the folder path
    sprintf_s(szSearchPath, "%s%s", GetIEditor()->GetMasterCDFolder(), (const char*)m_strSearchFolder);

    // Fill the list
    RecurseDirectory(szSearchPath, hRoot, m_strFileSpec.GetBuffer(0));
    Expand(hRoot, TVE_EXPAND);

    return 0;
}

void CFileTree::RecurseDirectory(char szFolder[_MAX_PATH], HTREEITEM hRoot, PSTR pszFileSpec)
{
    ////////////////////////////////////////////////////////////////////////
    // Enumerate all files in the passed directory which match to the the
    // passed pattern. Also continue with adding all subdirectories
    ////////////////////////////////////////////////////////////////////////

    CFileEnum cTempFiles;
    __finddata64_t sFile;
    char szFilePath[_MAX_PATH];
    HTREEITEM hNewRoot, hNewItem;

    ASSERT(pszFileSpec);

    // Make the path ready for appening a folder or filename
    PathAddBackslash(szFolder);

    // Start the enumeration of the files
    if (cTempFiles.StartEnumeration(szFolder, "*.*", &sFile))
    {
        do
        {
            // Construct the full filepath of the current file
            cry_strcpy(szFilePath, szFolder);
            cry_strcat(szFilePath, sFile.name);

            // Have we found a directory ?
            if (sFile.attrib & _A_SUBDIR)
            {
                // Skip the parent directory entries
                if (_stricmp(sFile.name, ".") == 0 ||
                    _stricmp(sFile.name, "..") == 0)
                {
                    continue;
                }

                // Add it to the list and start recursion
                hNewRoot = InsertItem(sFile.name, 0, 0, hRoot);
                RecurseDirectory(szFilePath, hNewRoot, pszFileSpec);

                continue;
            }

            // Check if the file name maches the pattern
            if (!PathMatchSpec(sFile.name, pszFileSpec))
            {
                continue;
            }

            // Remove the extension from the name
            PathRenameExtension(sFile.name, "");

            // Add the file to the list
            hNewItem = InsertItem(sFile.name, 1, 1, hRoot);

            // Associate the handle of this item with the path
            // of its file
            m_cFileNames[hNewItem] = CString(szFilePath);
        } while (cTempFiles.GetNextFile(&sFile));
    }
}

CString CFileTree::GetSelectedFile()
{
    ////////////////////////////////////////////////////////////////////////
    // Return the path of the currently selected file. If there is no
    // currently selected file, just a NULL terminated string will be
    // returned
    ////////////////////////////////////////////////////////////////////////

    Files::iterator it = m_cFileNames.find(GetSelectedItem());
    if (it != m_cFileNames.end())
    {
        return it->second;
    }
    else
    {
        return CString("");
    }
}
