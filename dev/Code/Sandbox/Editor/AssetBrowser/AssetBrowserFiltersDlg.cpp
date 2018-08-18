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

#include "StdAfx.h"
#include "AssetBrowserFiltersDlg.h"
#include "AssetBrowserManager.h"
#include "AssetBrowserDialog.h"
#include "GeneralAssetDbFilterDlg.h"
#include "IAssetItemDatabase.h"

#include <QScrollArea>
#include <QVBoxLayout>

const int FILTERS_MINIMUM_WIDTH = 300;

CAssetBrowserFiltersDlg::CAssetBrowserFiltersDlg(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , m_pGeneralDlg(nullptr)
    , m_rollupCtrl(nullptr)
    , m_layout(new QVBoxLayout(this))
    , m_scrollArea(new QScrollArea(this))
{
}

CAssetBrowserFiltersDlg::~CAssetBrowserFiltersDlg()
{
    auto iter = m_filterDlgs.begin();
    while (iter != m_filterDlgs.end())
    {
        iter->second->deleteLater();
        ++iter;
    }

    m_rollupCtrl->clear();
}

void CAssetBrowserFiltersDlg::CreateFilterGroupsRollup()
{
    m_rollupCtrl = new QRollupCtrl(this);
    m_rollupCtrl->setMinimumSize(QSize(FILTERS_MINIMUM_WIDTH, this->height()));
    m_pGeneralDlg = new CGeneralAssetDbFilterDlg(this);
    m_rollupCtrl->addItem(m_pGeneralDlg, "General");

    m_scrollArea->setWidget(m_rollupCtrl);
    m_scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->adjustSize();
    m_layout->addWidget(m_scrollArea);
    m_layout->setContentsMargins(0, 0, 0, 0);

    CAssetViewer* assetViewer = &CAssetBrowserDialog::Instance()->GetAssetViewer();
    m_pGeneralDlg->SetAssetViewer(assetViewer);
    m_panelIds.clear();

    auto pAssetBrowserMgr = CAssetBrowserManager::Instance();
    for (size_t i = 0; i < pAssetBrowserMgr->GetAssetDatabases().size(); ++i)
    {
        auto pAssetDb = pAssetBrowserMgr->GetAssetDatabases()[i];
        auto pDlg = pAssetDb->CreateDbFilterDialog(this, &CAssetBrowserDialog::Instance()->GetAssetViewer());
        m_rollupCtrl->addItem(pDlg, QtUtil::ToQString(pAssetDb->GetDatabaseName()));
        m_filterDlgs[pAssetDb] = pDlg;
        m_panelIds.push_back(pDlg);
    }
}

void CAssetBrowserFiltersDlg::DestroyFilterGroupsRollup()
{
    m_filterDlgs.clear();
    m_rollupCtrl->clear();
}

void CAssetBrowserFiltersDlg::RefreshVisibleAssetDbsRollups()
{
    for (size_t i = 0; i < m_panelIds.size(); ++i)
    {
        m_rollupCtrl->removeItem(m_panelIds[i]);
    }

    m_panelIds.clear();

    TAssetDatabases visibleDbs = CAssetBrowserDialog::Instance()->GetAssetViewer().GetDatabases();
    for (size_t i = 0; i < visibleDbs.size(); ++i)
    {
        auto pAssetDb = visibleDbs[i];
        m_rollupCtrl->addItem(m_filterDlgs[pAssetDb], QtUtil::ToQString(pAssetDb->GetDatabaseName()));
        m_panelIds.push_back(m_filterDlgs[pAssetDb]);
    }
}

void CAssetBrowserFiltersDlg::UpdateAllFiltersUI()
{
    for (auto iter = m_filterDlgs.begin(); iter != m_filterDlgs.end(); ++iter)
    {
        if (!iter->first)
        {
            continue;
        }

        iter->first->UpdateDbFilterDialogUI(iter->second);
    }
}
