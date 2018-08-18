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
#include "TextureAssetDbFilterDlg.h"
#include "IAssetViewer.h"

#include <AssetBrowser/AssetTypes/Texture/ui_TextureAssetDbFilterDlg.h>
// CTextureAssetDbFilterDlg dialog

CTextureAssetDbFilterDlg::CTextureAssetDbFilterDlg(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , ui(new Ui::TextureAssetDbFilterDlg)
{
    ui->setupUi(this);

    ui->ASSET_FILTERS_TEXTURES_MIN_WIDTH_COMBO->setCurrentIndex(0);
    ui->ASSET_FILTERS_TEXTURES_MAX_WIDTH_COMBO->setCurrentIndex(ui->ASSET_FILTERS_TEXTURES_MAX_WIDTH_COMBO->count() - 1);

    ui->ASSET_FILTERS_TEXTURES_MIN_HEIGHT_COMBO->setCurrentIndex(0);
    ui->ASSET_FILTERS_TEXTURES_MAX_HEIGHT_COMBO->setCurrentIndex(ui->ASSET_FILTERS_TEXTURES_MAX_HEIGHT_COMBO->count() - 1);

    ui->ASSET_FILTERS_TEXTURES_MIN_MIPS_COMBO->setCurrentIndex(0);
    ui->ASSET_FILTERS_TEXTURES_MAX_MIPS_COMBO->setCurrentIndex(ui->ASSET_FILTERS_TEXTURES_MAX_MIPS_COMBO->count() - 1);

    ui->ASSET_FILTERS_TEXTURES_MIN_WIDTH_COMBO->setCurrentIndex(0);
    ui->ASSET_FILTERS_TEXTURES_MAX_WIDTH_COMBO->setCurrentIndex(ui->ASSET_FILTERS_TEXTURES_MAX_WIDTH_COMBO->count() - 1);

    ui->ASSET_FILTERS_TEXTURES_TYPE_COMBO->setCurrentIndex(0);
    ui->ASSET_FILTERS_TEXTURES_USAGE_COMBO->setCurrentIndex(0);

    connect(ui->ASSET_FILTERS_TEXTURES_MIN_WIDTH_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMinimumWidth()));
    connect(ui->ASSET_FILTERS_TEXTURES_MAX_WIDTH_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMaximumWidth()));
    connect(ui->ASSET_FILTERS_TEXTURES_MIN_HEIGHT_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMinimumHeight()));
    connect(ui->ASSET_FILTERS_TEXTURES_MAX_HEIGHT_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMaximumHeight()));
    connect(ui->ASSET_FILTERS_TEXTURES_MIN_MIPS_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMinimumMips()));
    connect(ui->ASSET_FILTERS_TEXTURES_MAX_MIPS_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMaximumMips()));
    connect(ui->ASSET_FILTERS_TEXTURES_TYPE_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboTextureType()));
    connect(ui->ASSET_FILTERS_TEXTURES_USAGE_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboTextureUsage()));
}

CTextureAssetDbFilterDlg::~CTextureAssetDbFilterDlg()
{
}

void CTextureAssetDbFilterDlg::SetComboToValidValue(QComboBox* min, QComboBox* max, COMBO_CHANGED changed /*combobox value changed*/)
{
    int minValue = min->currentIndex();
    int maxValue = max->currentIndex();

    if (minValue > maxValue)
    {
        if (changed == MIN)
        {
            min->setCurrentIndex(maxValue);
            return;
        }
        else
        {
            max->setCurrentIndex(minValue);
            return;
        }
    }

    ApplyFilter();
}

void CTextureAssetDbFilterDlg::UpdateFilterUI()
{
    auto filters = m_pAssetViewer->GetCurrentFilters();

    {
        SAssetField& field = filters["width"];

        int minFound = ui->ASSET_FILTERS_TEXTURES_MIN_WIDTH_COMBO->findText(field.m_filterValue);
        if (minFound != -1)
        {
            ui->ASSET_FILTERS_TEXTURES_MIN_WIDTH_COMBO->setCurrentIndex(minFound);
        }

        int maxFound = ui->ASSET_FILTERS_TEXTURES_MAX_WIDTH_COMBO->findText(field.m_filterValue);
        if (maxFound != -1)
        {
            ui->ASSET_FILTERS_TEXTURES_MAX_WIDTH_COMBO->setCurrentIndex(maxFound);
        }
    }

    {
        SAssetField& field = filters["height"];

        int minFound = ui->ASSET_FILTERS_TEXTURES_MIN_HEIGHT_COMBO->findText(field.m_filterValue);
        if (minFound != -1)
        {
            ui->ASSET_FILTERS_TEXTURES_MIN_HEIGHT_COMBO->setCurrentIndex(minFound);
        }

        int maxFound = ui->ASSET_FILTERS_TEXTURES_MAX_HEIGHT_COMBO->findText(field.m_filterValue);
        if (maxFound != -1)
        {
            ui->ASSET_FILTERS_TEXTURES_MAX_HEIGHT_COMBO->setCurrentIndex(maxFound);
        }
    }

    {
        SAssetField& field = filters["mips"];

        int minFound = ui->ASSET_FILTERS_TEXTURES_MIN_MIPS_COMBO->findText(field.m_filterValue);
        if (minFound != -1)
        {
            ui->ASSET_FILTERS_TEXTURES_MIN_MIPS_COMBO->setCurrentIndex(minFound);
        }

        int maxFound = ui->ASSET_FILTERS_TEXTURES_MAX_MIPS_COMBO->findText(field.m_filterValue);
        if (maxFound != -1)
        {
            ui->ASSET_FILTERS_TEXTURES_MAX_MIPS_COMBO->setCurrentIndex(maxFound);
        }
    }

    {
        SAssetField& field = filters["type"];

        int value = ui->ASSET_FILTERS_TEXTURES_TYPE_COMBO->findText(field.m_filterValue);
        if (value != -1)
        {
            ui->ASSET_FILTERS_TEXTURES_TYPE_COMBO->setCurrentIndex(value);
        }
    }

    {
        SAssetField& field = filters["texture_usage"];
        int value = ui->ASSET_FILTERS_TEXTURES_USAGE_COMBO->findText(field.m_filterValue);
        if (value != -1)
        {
            ui->ASSET_FILTERS_TEXTURES_USAGE_COMBO->setCurrentIndex(value);
        }
    }
}

void CTextureAssetDbFilterDlg::ApplyFilter()
{
    auto filters = m_pAssetViewer->GetCurrentFilters();
    QString str;

    {
        SAssetField& field = filters["width"];

        field.m_fieldName = "width";
        field.m_filterCondition = SAssetField::eCondition_InsideRange;
        str = ui->ASSET_FILTERS_TEXTURES_MIN_WIDTH_COMBO->currentText();
        field.m_filterValue = str;
        str = ui->ASSET_FILTERS_TEXTURES_MAX_WIDTH_COMBO->currentText();
        field.m_maxFilterValue = str;
        field.m_fieldType = SAssetField::eType_Int32;
    }

    {
        SAssetField& field = filters["height"];

        field.m_fieldName = "height";
        field.m_filterCondition = SAssetField::eCondition_InsideRange;
        str = ui->ASSET_FILTERS_TEXTURES_MIN_HEIGHT_COMBO->currentText();
        field.m_filterValue = str;
        str = ui->ASSET_FILTERS_TEXTURES_MAX_HEIGHT_COMBO->currentText();
        field.m_maxFilterValue = str;
        field.m_fieldType = SAssetField::eType_Int32;
    }

    {
        SAssetField& field = filters["mips"];

        field.m_fieldName = "mips";
        field.m_filterCondition = SAssetField::eCondition_InsideRange;
        str = ui->ASSET_FILTERS_TEXTURES_MIN_MIPS_COMBO->currentText();
        field.m_filterValue = str;
        str = ui->ASSET_FILTERS_TEXTURES_MAX_MIPS_COMBO->currentText();
        field.m_maxFilterValue = str;
        field.m_fieldType = SAssetField::eType_Int32;
    }

    {
        str = ui->ASSET_FILTERS_TEXTURES_TYPE_COMBO->currentText();

        if (str != "Any")
        {
            SAssetField& field = filters["type"];

            field.m_fieldName = "type";
            field.m_filterCondition = SAssetField::eCondition_Equal;
            field.m_filterValue = str;
            field.m_fieldType = SAssetField::eType_String;
        }
        else
        {
            auto iter = filters.find("type");

            if (iter != filters.end())
            {
                filters.erase(iter);
            }
        }
    }

    {
        str = ui->ASSET_FILTERS_TEXTURES_USAGE_COMBO->currentText();

        if (str != "Any")
        {
            SAssetField& field = filters["texture_usage"];

            field.m_fieldName = "texture_usage";
            field.m_filterCondition = SAssetField::eCondition_Equal;
            field.m_filterValue = str;
            field.m_fieldType = SAssetField::eType_String;
        }
        else
        {
            auto iter = filters.find("texture_usage");

            if (iter != filters.end())
            {
                filters.erase(iter);
            }
        }
    }

    m_pAssetViewer->ApplyFilters(filters);
}

void CTextureAssetDbFilterDlg::OnBnClickedOk()
{
    // do nothing here
}

void CTextureAssetDbFilterDlg::OnBnClickedCancel()
{
    // do nothing here
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMinimumWidth()
{
    SetComboToValidValue(ui->ASSET_FILTERS_TEXTURES_MIN_WIDTH_COMBO, ui->ASSET_FILTERS_TEXTURES_MAX_WIDTH_COMBO, MIN);
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMaximumWidth()
{
    SetComboToValidValue(ui->ASSET_FILTERS_TEXTURES_MIN_WIDTH_COMBO, ui->ASSET_FILTERS_TEXTURES_MAX_WIDTH_COMBO, MAX);
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMinimumHeight()
{
    SetComboToValidValue(ui->ASSET_FILTERS_TEXTURES_MIN_HEIGHT_COMBO, ui->ASSET_FILTERS_TEXTURES_MAX_HEIGHT_COMBO, MIN);
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMaximumHeight()
{
    SetComboToValidValue(ui->ASSET_FILTERS_TEXTURES_MIN_HEIGHT_COMBO, ui->ASSET_FILTERS_TEXTURES_MAX_HEIGHT_COMBO, MAX);
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMinimumMips()
{
    SetComboToValidValue(ui->ASSET_FILTERS_TEXTURES_MIN_MIPS_COMBO, ui->ASSET_FILTERS_TEXTURES_MAX_MIPS_COMBO, MIN);
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMaximumMips()
{
    SetComboToValidValue(ui->ASSET_FILTERS_TEXTURES_MIN_MIPS_COMBO, ui->ASSET_FILTERS_TEXTURES_MAX_MIPS_COMBO, MAX);
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboTextureType()
{
    ApplyFilter();
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboTextureUsage()
{
    ApplyFilter();
}

#include <AssetBrowser/AssetTypes/Texture/TextureAssetDbFilterDlg.moc>