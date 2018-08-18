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

    m_radius = ui->radiusSlider;
    uint64 nTerrainWidth(1024);
    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    if (pHeightmap)
    {
        nTerrainWidth = GetIEditor()->GetTerrainManager()->GetHeightmap()->GetWidth();
    }
    float fMax = nTerrainWidth * 2.0f;
    m_radius->setRange(1, 100 * log10(fMax));
    SetRadius();
    connect(m_radius, &QSlider::valueChanged, this, &CTerrainHolePanel::OnReleasedcaptureRadius);

    m_removeHole = ui->HOLE_REMOVE;
    m_makeHole = ui->HOLE_MAKE;
    m_makeHole->setChecked(m_tool->GetMakeHole());
    connect(m_makeHole, &QRadioButton::toggled, this, &CTerrainHolePanel::OnHoleMake);
}

/////////////////////////////////////////////////////////////////////////////
// CTerrainHolePanel message handlers

void CTerrainHolePanel::OnHoleMake(bool v)
{
    SetMakeHole(v);
    m_tool->SetMakeHole(v);
}

void CTerrainHolePanel::SetRadius()
{
    m_radius->setValue(log10(m_tool->GetBrushRadius() / 0.5f) * 100.0f);
}

void CTerrainHolePanel::OnReleasedcaptureRadius(int value)
{
    float fRadius = pow(10.0f, value / 100.0f);
    m_tool->SetBrushRadius(fRadius * 0.5f);
}

void CTerrainHolePanel::SetMakeHole(bool bEnable)
{
    m_makeHole->setChecked(bEnable);
}