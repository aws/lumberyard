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
#include "SoundAssetDbFilterDlg.h"
#include "IAssetViewer.h"

#include <AssetBrowser/AssetTypes/Sound/ui_SoundAssetDbFilterDlg.h>
// CSoundAssetDbFilterDlg dialog

CSoundAssetDbFilterDlg::CSoundAssetDbFilterDlg(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , ui(new Ui::SoundAssetDbFilterDlg)
{
    ui->setupUi(this);

    ui->ASSET_FILTERS_SOUNDS_MIN_LENGTH_COMBO->setCurrentIndex(0);
    ui->ASSET_FILTERS_SOUNDS_MAX_LENGTH_COMBO->setCurrentIndex(ui->ASSET_FILTERS_SOUNDS_MAX_LENGTH_COMBO->count() - 1);

    connect(ui->ASSET_FILTERS_SOUNDS_MIN_LENGTH_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMinimumLength(int)));
    connect(ui->ASSET_FILTERS_SOUNDS_MAX_LENGTH_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMaximumLength(int)));
    connect(ui->ASSET_FILTERS_SOUNDS_LOOPING, &QCheckBox::stateChanged, this, &CSoundAssetDbFilterDlg::OnBnClickedCheckLoopingSounds);
}

CSoundAssetDbFilterDlg::~CSoundAssetDbFilterDlg()
{
}

void CSoundAssetDbFilterDlg::UpdateFilterUI()
{
    auto filters = m_pAssetViewer->GetCurrentFilters();

    {
        SAssetField& field = filters["length"];

        int minFound = ui->ASSET_FILTERS_SOUNDS_MIN_LENGTH_COMBO->findText(field.m_filterValue);
        if (minFound != -1)
        {
            ui->ASSET_FILTERS_SOUNDS_MIN_LENGTH_COMBO->setCurrentIndex(minFound);
        }

        int maxFound = ui->ASSET_FILTERS_SOUNDS_MAX_LENGTH_COMBO->findText(field.m_maxFilterValue);
        if (maxFound != -1)
        {
            ui->ASSET_FILTERS_SOUNDS_MAX_LENGTH_COMBO->setCurrentIndex(maxFound);
        }
    }

    {
        SAssetField& field = filters["loopsound"];
        ui->ASSET_FILTERS_SOUNDS_LOOPING->setCheckState(field.m_filterValue == "Yes" ? Qt::Checked : Qt::Unchecked);
    }
}

void CSoundAssetDbFilterDlg::ApplyFilter()
{
    auto filters = m_pAssetViewer->GetCurrentFilters();
    QString str;

    {
        SAssetField& field = filters["length"];

        field.m_fieldName = "length";
        field.m_filterCondition = SAssetField::eCondition_InsideRange;
        str = ui->ASSET_FILTERS_SOUNDS_MIN_LENGTH_COMBO->currentText();
        field.m_filterValue = str;

        str = ui->ASSET_FILTERS_SOUNDS_MAX_LENGTH_COMBO->currentText();
        field.m_maxFilterValue = str;
        field.m_fieldType = SAssetField::eType_Int32;
    }

    if (ui->ASSET_FILTERS_SOUNDS_LOOPING->checkState() == Qt::Checked)
    {
        SAssetField& field = filters["loopsound"];

        field.m_fieldName = "loopsound";
        field.m_filterCondition = SAssetField::eCondition_Equal;
        field.m_filterValue = "Yes";
        field.m_fieldType = SAssetField::eType_Bool;
    }
    else
    {
        auto iter = filters.find("loopsound");

        if (iter != filters.end())
        {
            filters.erase(iter);
        }
    }

    m_pAssetViewer->ApplyFilters(filters);
}

void CSoundAssetDbFilterDlg::OnCbnSelchangeComboMinimumLength(int index)
{
    int max = ui->ASSET_FILTERS_SOUNDS_MAX_LENGTH_COMBO->currentIndex();
    if (index > max)
    {
        ui->ASSET_FILTERS_SOUNDS_MIN_LENGTH_COMBO->setCurrentIndex(max);
        return;
    }

    ApplyFilter();
}

void CSoundAssetDbFilterDlg::OnCbnSelchangeComboMaximumLength(int index)
{
    int min = ui->ASSET_FILTERS_SOUNDS_MIN_LENGTH_COMBO->currentIndex();
    if (index < min)
    {
        ui->ASSET_FILTERS_SOUNDS_MAX_LENGTH_COMBO->setCurrentIndex(min);
        return;
    }

    ApplyFilter();
}

void CSoundAssetDbFilterDlg::OnBnClickedCheckLoopingSounds()
{
    ApplyFilter();
}

#include <AssetBrowser/AssetTypes/Sound/SoundAssetDbFilterDlg.moc>