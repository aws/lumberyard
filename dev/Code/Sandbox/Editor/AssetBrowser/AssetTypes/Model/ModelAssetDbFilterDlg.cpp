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
#include "ModelAssetDbFilterDlg.h"
#include "IAssetViewer.h"

// CModelAssetDbFilterDlg dialog
#include <AssetBrowser/AssetTypes/Model/ui_ModelAssetDbFilterDlg.h>

CModelAssetDbFilterDlg::CModelAssetDbFilterDlg(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , ui(new Ui::ModelAssetDbFilterDlg)
{
    ui->setupUi(this);

    ui->ASSET_FILTERS_MODELS_MIN_TRIANGLES_COMBO->setCurrentIndex(0);
    ui->ASSET_FILTERS_MODELS_MAX_TRIANGLES_COMBO->setCurrentIndex(ui->ASSET_FILTERS_MODELS_MAX_TRIANGLES_COMBO->count() - 1);

    ui->ASSET_FILTERS_MODELS_MIN_MATERIALS_COMBO->setCurrentIndex(0);
    ui->ASSET_FILTERS_MODELS_MAX_MATERIALS_COMBO->setCurrentIndex(ui->ASSET_FILTERS_MODELS_MAX_MATERIALS_COMBO->count() - 1);

    connect(ui->ASSET_FILTERS_MODELS_MIN_TRIANGLES_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMinTriangles(int)));
    connect(ui->ASSET_FILTERS_MODELS_MAX_TRIANGLES_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMaxTriangles(int)));
    connect(ui->ASSET_FILTERS_MODELS_MIN_MATERIALS_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMinMaterials(int)));
    connect(ui->ASSET_FILTERS_MODELS_MAX_MATERIALS_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMaxMaterials(int)));
    connect(ui->ASSET_FILTERS_MODELS_HIDE_LODS, &QCheckBox::stateChanged, this, &CModelAssetDbFilterDlg::OnBnClickedCheckHideLods);
}

CModelAssetDbFilterDlg::~CModelAssetDbFilterDlg()
{
}

void CModelAssetDbFilterDlg::UpdateFilterUI()
{
    auto filters = m_pAssetViewer->GetCurrentFilters();
    QString str;

    {
        SAssetField& field = filters["trianglecount"];

        int minFound = ui->ASSET_FILTERS_MODELS_MIN_TRIANGLES_COMBO->findText(field.m_filterValue);
        if (minFound != -1)
        {
            ui->ASSET_FILTERS_MODELS_MIN_TRIANGLES_COMBO->setCurrentIndex(minFound);
        }

        int maxFound = ui->ASSET_FILTERS_MODELS_MAX_TRIANGLES_COMBO->findText(field.m_maxFilterValue);
        if (maxFound != -1)
        {
            ui->ASSET_FILTERS_MODELS_MAX_TRIANGLES_COMBO->setCurrentIndex(maxFound);
        }
    }

    {
        SAssetField& field = filters["mtlcount"];

        int minFound = ui->ASSET_FILTERS_MODELS_MIN_MATERIALS_COMBO->findText(field.m_filterValue);
        if (minFound != -1)
        {
            ui->ASSET_FILTERS_MODELS_MIN_MATERIALS_COMBO->setCurrentIndex(minFound);
        }

        int maxFound = ui->ASSET_FILTERS_MODELS_MAX_MATERIALS_COMBO->findText(field.m_maxFilterValue);
        if (maxFound != -1)
        {
            ui->ASSET_FILTERS_MODELS_MAX_MATERIALS_COMBO->setCurrentIndex(maxFound);
        }
    }

    {
        SAssetField& field = filters["islod"];
        ui->ASSET_FILTERS_MODELS_HIDE_LODS->setCheckState(field.m_filterValue == "No" ? Qt::Checked : Qt::Unchecked);
    }
}

void CModelAssetDbFilterDlg::ApplyFilter()
{
    auto filters = m_pAssetViewer->GetCurrentFilters();
    QString str;

    {
        SAssetField& field = filters["trianglecount"];

        field.m_fieldName = "trianglecount";
        field.m_filterCondition = SAssetField::eCondition_InsideRange;
        str = ui->ASSET_FILTERS_MODELS_MIN_TRIANGLES_COMBO->currentText();
        field.m_filterValue = str;

        str = ui->ASSET_FILTERS_MODELS_MAX_TRIANGLES_COMBO->currentText();
        field.m_maxFilterValue = str;
        field.m_fieldType = SAssetField::eType_Int32;
    }

    {
        SAssetField& field = filters["mtlcount"];

        field.m_fieldName = "mtlcount";
        field.m_filterCondition = SAssetField::eCondition_InsideRange;
        str = ui->ASSET_FILTERS_MODELS_MIN_MATERIALS_COMBO->currentText();
        field.m_filterValue = str;

        str = ui->ASSET_FILTERS_MODELS_MAX_MATERIALS_COMBO->currentText();
        field.m_maxFilterValue = str;
        field.m_fieldType = SAssetField::eType_Int32;
    }

    {
        SAssetField& field = filters["islod"];

        if (ui->ASSET_FILTERS_MODELS_HIDE_LODS->checkState() == Qt::Checked)
        {
            field.m_fieldName = "islod";
            field.m_filterCondition = SAssetField::eCondition_Equal;
            field.m_filterValue =  "No";
            field.m_fieldType = SAssetField::eType_Bool;
        }
        else
        {
            auto iter = filters.find("islod");

            if (iter != filters.end())
            {
                filters.erase(iter);
            }
        }
    }

    m_pAssetViewer->ApplyFilters(filters);
}

void CModelAssetDbFilterDlg::OnCbnSelchangeComboMinTriangles(int index)
{
    int max = ui->ASSET_FILTERS_MODELS_MAX_TRIANGLES_COMBO->currentIndex();
    if (index > max)
    {
        ui->ASSET_FILTERS_MODELS_MIN_TRIANGLES_COMBO->setCurrentIndex(max);
        return;
    }

    ApplyFilter();
}

void CModelAssetDbFilterDlg::OnCbnSelchangeComboMaxTriangles(int index)
{
    int min = ui->ASSET_FILTERS_MODELS_MIN_TRIANGLES_COMBO->currentIndex();
    if (index < min)
    {
        ui->ASSET_FILTERS_MODELS_MAX_TRIANGLES_COMBO->setCurrentIndex(min);
        return;
    }

    ApplyFilter();
}

void CModelAssetDbFilterDlg::OnCbnSelchangeComboMinMaterials(int index)
{
    int max = ui->ASSET_FILTERS_MODELS_MAX_MATERIALS_COMBO->currentIndex();
    if (index > max)
    {
        ui->ASSET_FILTERS_MODELS_MIN_MATERIALS_COMBO->setCurrentIndex(max);
        return;
    }

    ApplyFilter();
}

void CModelAssetDbFilterDlg::OnCbnSelchangeComboMaxMaterials(int index)
{
    int min = ui->ASSET_FILTERS_MODELS_MIN_MATERIALS_COMBO->currentIndex();
    if (index < min)
    {
        ui->ASSET_FILTERS_MODELS_MAX_MATERIALS_COMBO->setCurrentIndex(min);
        return;
    }

    ApplyFilter();
}

void CModelAssetDbFilterDlg::OnBnClickedCheckHideLods()
{
    ApplyFilter();
}

#include <AssetBrowser/AssetTypes/Model/ModelAssetDbFilterDlg.moc>