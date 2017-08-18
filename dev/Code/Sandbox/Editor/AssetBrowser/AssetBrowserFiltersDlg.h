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

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERFILTERSDLG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERFILTERSDLG_H
#pragma once
#include "IAssetItemDatabase.h"

#include "Controls/QRollupCtrl.h"

class CGeneralAssetDbFilterDlg;
class QScrollArea;
class QVBoxLayout;

class CAssetBrowserFiltersDlg
    : public QWidget
{
public:
    CAssetBrowserFiltersDlg(QWidget* pParent = NULL);   // standard constructor
    virtual ~CAssetBrowserFiltersDlg();

    void CreateFilterGroupsRollup();
    void DestroyFilterGroupsRollup();
    void RefreshVisibleAssetDbsRollups();
    void UpdateAllFiltersUI();
    CGeneralAssetDbFilterDlg* GetGeneralDlg()
    {
        return m_pGeneralDlg;
    }

protected:
    QRollupCtrl* m_rollupCtrl;
    std::map<IAssetItemDatabase*, QWidget*> m_filterDlgs;
    CGeneralAssetDbFilterDlg* m_pGeneralDlg;
    std::vector<QWidget*> m_panelIds;

private:
    QVBoxLayout* m_layout;
    QScrollArea* m_scrollArea;
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERFILTERSDLG_H
