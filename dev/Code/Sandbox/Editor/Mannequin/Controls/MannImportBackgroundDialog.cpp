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

// Description : This is the source file for importing background objects into Mannequin.
//               The recomended way to call this dialog is through DoModal() method.


#include "stdafx.h"
#include "MannImportBackgroundDialog.h"

//////////////////////////////////////////////////////////////////////////

CMannImportBackgroundDialog::CMannImportBackgroundDialog(std::vector<CString>& loadedObjects)
    : CDialog(IDD_MANN_IMPORT_BACKGROUND)
    , m_loadedObjects(loadedObjects)
    , m_selectedRoot(-1)
{
}

//////////////////////////////////////////////////////////////////////////

void CMannImportBackgroundDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_ROOT_OBJECT, m_comboRoot);
}

//////////////////////////////////////////////////////////////////////////

BOOL CMannImportBackgroundDialog::OnInitDialog()
{
    if (CDialog::OnInitDialog() == FALSE)
    {
        return FALSE;
    }

    // Context combos
    const uint32 numObjects = m_loadedObjects.size();
    m_comboRoot.AddString("None");
    for (uint32 i = 0; i < numObjects; i++)
    {
        m_comboRoot.AddString(m_loadedObjects[i]);
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////

void CMannImportBackgroundDialog::OnOK()
{
    CDialog::OnOK();

    m_selectedRoot = m_comboRoot.GetCurSel() - 1;
}

//////////////////////////////////////////////////////////////////////////
