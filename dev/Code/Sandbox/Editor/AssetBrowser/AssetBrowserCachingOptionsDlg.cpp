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
#include "AssetBrowserCachingOptionsDlg.h"
#include "IAssetItemDatabase.h"
#include "AssetBrowserDialog.h"

// CAssetBrowserCachingOptionsDlg dialog

#include <AssetBrowser/ui_AssetBrowserCachingOptionsDlg.h>

#include <QPushButton>

CAssetBrowserCachingOptionsDlg::CAssetBrowserCachingOptionsDlg(QWidget* pParent)
    : QDialog(pParent)
    , m_ui(new Ui::AssetBrowserCachingOptionsDlg)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Start"));
    setFixedSize(size());

    OnInitDialog();
}

CAssetBrowserCachingOptionsDlg::~CAssetBrowserCachingOptionsDlg()
{
}



// CAssetBrowserCachingOptionsDlg message handlers

void CAssetBrowserCachingOptionsDlg::OnInitDialog()
{
    m_bForceCache = false;

    std::vector<IClassDesc*> assetDatabasePlugins;
    IAssetItemDatabase* pAssetDB = NULL;
    IEditorClassFactory* pClassFactory = GetIEditor()->GetClassFactory();

    pClassFactory->GetClassesByCategory("Asset Item DB", assetDatabasePlugins);

    for (size_t i = 0; i < assetDatabasePlugins.size(); ++i)
    {
        if (assetDatabasePlugins[i]->QueryInterface(__uuidof(IAssetItemDatabase), (void**)&pAssetDB) == S_OK)
        {
            m_ui->m_lstDatabases->addItem(pAssetDB->GetDatabaseName());
            m_databases.push_back(pAssetDB);
        }
    }

    m_ui->m_lstDatabases->selectAll();

    m_ui->m_cbThumbSize->setCurrentIndex(m_ui->m_cbThumbSize->findText(QString::number(CAssetBrowserDialog::Instance()->GetAssetViewer().GetAssetThumbSize())));
}

TAssetDatabases CAssetBrowserCachingOptionsDlg::GetSelectedDatabases() const
{
    return m_selectedDBs;
}

bool CAssetBrowserCachingOptionsDlg::IsForceCache() const
{
    return m_bForceCache;
}

UINT CAssetBrowserCachingOptionsDlg::GetThumbSize() const
{
    return m_thumbSize;
}

void CAssetBrowserCachingOptionsDlg::accept()
{
    for (size_t i = 0; i < m_databases.size(); ++i)
    {
        if (m_ui->m_lstDatabases->item(i)->isSelected())
        {
            m_selectedDBs.push_back(m_databases[i]);
        }
    }

    m_bForceCache = m_ui->m_checkForceCache->isChecked();
    m_thumbSize = m_ui->m_cbThumbSize->currentText().toInt();

    QDialog::accept();
}

#include <AssetBrowser/AssetBrowserCachingOptionsDlg.moc>