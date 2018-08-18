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
#include "CloudGroupPanel.h"
#include "Objects/CloudGroup.h"
#include <Cloud/ui_CloudGroupPanel.h>
#include "QtUtil.h"

// CCloudGroupPanel dialog

CCloudGroupPanel::CCloudGroupPanel(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CCloudGroupPanel)
{
    m_cloud = 0;
    ui->setupUi(this);
    connect(ui->CLOUDS_GENERATE, &QPushButton::clicked, this, &CCloudGroupPanel::OnCloudsGenerate);
    connect(ui->CLOUDS_IMPORT, &QPushButton::clicked, this, &CCloudGroupPanel::OnCloudsImport);
    connect(ui->CLOUDS_EXPORT, &QPushButton::clicked, this, &CCloudGroupPanel::OnCloudsExport);
    connect(ui->CLOUDS_BROWSE, &QToolButton::clicked, this, &CCloudGroupPanel::OnCloudsBrowse);
}

//////////////////////////////////////////////////////////////////////////
CCloudGroupPanel::~CCloudGroupPanel()
{
    m_cloud = 0;
}

//////////////////////////////////////////////////////////////////////////

void CCloudGroupPanel::Init(CCloudGroup* cloud)
{
    m_cloud = cloud;
    SetFileBrowseText();
}

void CCloudGroupPanel::OnCloudsGenerate()
{
    if (m_cloud)
    {
        m_cloud->Generate();
    }
}

void CCloudGroupPanel::OnCloudsImport()
{
    if (m_cloud)
    {
        m_cloud->Import();
    }
}


void CCloudGroupPanel::OnCloudsBrowse()
{
    if (m_cloud)
    {
        m_cloud->BrowseFile();
    }
}

void CCloudGroupPanel::OnCloudsExport()
{
    if (m_cloud)
    {
        m_cloud->Export();
    }
}

void CCloudGroupPanel::SetFileBrowseText()
{
    if (m_cloud)
    {
        ui->CLOUDS_FILE->setText(m_cloud->GetExportFileName());
    }
}