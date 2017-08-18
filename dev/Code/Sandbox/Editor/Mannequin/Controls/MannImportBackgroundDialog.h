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

// Description : This is the header file for importing background objects into Mannequin.
//               The recomended way to call this dialog is through DoModal   method


#ifndef MannImportBackgroundDialogDialog_h__
#define MannImportBackgroundDialogDialog_h__

class CMannImportBackgroundDialog
    : public CDialog
{
    //////////////////////////////////////////////////////////////////////////
    // Methods
public:
    CMannImportBackgroundDialog(std::vector<CString>& loadedObjects);

    void DoDataExchange(CDataExchange* pDX);
    BOOL OnInitDialog();
    void OnOK();

    int GetCurrentRoot() const
    {
        return m_selectedRoot;
    }

protected:
    const std::vector<CString>& m_loadedObjects;
    CComboBox m_comboRoot;
    int m_selectedRoot;
};

#endif // StringInputDialog_h__
