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
#include "AssetBrowserPreviewCharacterDlgFooter.h"
#include "AssetCharacterItem.h"

#include <AssetBrowser/AssetTypes/Character/ui_AssetBrowserPreviewCharacterDlgFooter.h>

CAssetBrowserPreviewCharacterDlgFooter::CAssetBrowserPreviewCharacterDlgFooter(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , m_ui(new Ui::AssetBrowserPreviewCharacterDlgFooter)
{
    m_ui->setupUi(this);

    m_minLodDefault = 0;

    void (QComboBox::* currentIndexChanged)(int) = &QComboBox::currentIndexChanged;

    connect(m_ui->m_CBLodLevel, currentIndexChanged, this, &CAssetBrowserPreviewCharacterDlgFooter::OnLodLevelChanged);
    connect(m_ui->m_sliderAmbient, &QAbstractSlider::sliderMoved, this, &CAssetBrowserPreviewCharacterDlgFooter::OnHScroll);

    OnInitDialog();
}

CAssetBrowserPreviewCharacterDlgFooter::~CAssetBrowserPreviewCharacterDlgFooter()
{
}

// CAssetBrowserPreviewCharacterDlgFooter message handlers
void CAssetBrowserPreviewCharacterDlgFooter::OnInitDialog()
{
    m_ui->m_CBLodLevel->clear();

    int lodmax = -1;

    ICVar* pCvar = gEnv->pConsole->GetCVar("e_LodMax");

    if (pCvar)
    {
        lodmax = pCvar->GetIVal();
    }

    for (int idx = 0; idx <= lodmax; idx++)
    {
        m_ui->m_CBLodLevel->addItem(QString::number(idx));
    }

    m_minLodDefault = gEnv->pConsole->GetCVar("e_LodMin")->GetIVal();
    m_ui->m_CBLodLevel->setCurrentIndex(m_minLodDefault);
    m_ui->m_sliderAmbient->setRange(0, 100);
    m_ui->m_sliderAmbient->setValue((int)AssetBrowser::kCharacterAssetAmbienceMultiplier);

    m_ui->m_labelValue->setText(QString::number(m_ui->m_sliderAmbient->value()));
}

void CAssetBrowserPreviewCharacterDlgFooter::OnHScroll()
{
    CAssetCharacterItem::s_fAmbience = m_ui->m_sliderAmbient->value();
    m_ui->m_labelValue->setText(QString::number(m_ui->m_sliderAmbient->value()));
    window()->findChild<QWidget*>("m_wndAssetRender")->update();
}

void CAssetBrowserPreviewCharacterDlgFooter::Init()
{
}

void CAssetBrowserPreviewCharacterDlgFooter::Reset()
{
    gEnv->pConsole->GetCVar("e_LodMin")->Set(0);
    m_ui->m_CBLodLevel->setCurrentIndex(0);
    OnLodLevelChanged();
}

void CAssetBrowserPreviewCharacterDlgFooter::OnLodLevelChanged()
{
    gEnv->pConsole->GetCVar("e_LodMin")->Set(m_ui->m_CBLodLevel->currentIndex());
}

#include <AssetBrowser/AssetTypes/Character/AssetBrowserPreviewCharacterDlgFooter.moc>
