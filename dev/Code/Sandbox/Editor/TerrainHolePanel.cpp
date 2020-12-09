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
#include "TerrainHolePanel.h"
#include "TerrainHoleTool.h"
#include "Terrain/TerrainManager.h"
#include <ui_TerrainHolePanel.h>

/////////////////////////////////////////////////////////////////////////////
// CTerrainHolePanel dialog


CTerrainHolePanel::CTerrainHolePanel(CTerrainHoleTool* tool, QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CTerrainHolePanel)
{
    assert(tool != 0);
    m_tool = tool;

    ui->setupUi(this);

    uint64 nTerrainWidth(1024);
    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    if (pHeightmap)
    {
        nTerrainWidth = GetIEditor()->GetTerrainManager()->GetHeightmap()->GetWidth();
    }
    ui->radiusSlider->setCurveMidpoint(0.1);
    ui->radiusSlider->setRange(0.1, nTerrainWidth);

    SetRadius();
    auto sliderDoubleComboValueChanged = static_cast<void(AzQtComponents::SliderDoubleCombo::*)()>(&AzQtComponents::SliderDoubleCombo::valueChanged);
    connect(ui->radiusSlider, sliderDoubleComboValueChanged, this, &CTerrainHolePanel::OnRadiusSliderValueChanged);

    m_removeHole = ui->HOLE_REMOVE;
    m_removeHole->setChecked(!m_tool->GetMakeHole());

    m_makeHole = ui->HOLE_MAKE;
    m_makeHole->setChecked(m_tool->GetMakeHole());

    connect(m_makeHole, &QRadioButton::toggled, this, &CTerrainHolePanel::OnHoleMake);
    connect(m_removeHole, &QRadioButton::toggled, this, &CTerrainHolePanel::OnHoleRemove);
}

/////////////////////////////////////////////////////////////////////////////
// CTerrainHolePanel message handlers

void CTerrainHolePanel::OnHoleMake(bool v)
{
    m_removeHole->setChecked(!v);
    m_tool->SetMakeHole(v);
}

void CTerrainHolePanel::OnHoleRemove(bool v)
{
    m_makeHole->setChecked(!v);
    m_tool->SetMakeHole(!v);
}

void CTerrainHolePanel::SetRadius()
{
    ui->radiusSlider->setValue(m_tool->GetBrushRadius());
}

void CTerrainHolePanel::OnRadiusSliderValueChanged()
{
    m_tool->SetBrushRadius(ui->radiusSlider->value());
}

void CTerrainHolePanel::SetMakeHole(bool bEnable)
{
    m_makeHole->setChecked(bEnable);
    m_removeHole->setChecked(!bEnable);
}
