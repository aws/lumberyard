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
#include "TerrainMoveToolPanel.h"
#include <ui_TerrainMoveToolPanel.h>
#include "TerrainMoveTool.h"

#include <I3DEngine.h>
#include "TerrainMoveTool.h"

#include "Terrain/Heightmap.h"

/////////////////////////////////////////////////////////////////////////////
// CTerrainMoveToolPanel dialog

CTerrainMoveToolPanel::CTerrainMoveToolPanel(CTerrainMoveTool* tool, QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CTerrainMoveToolPanel)
{
    m_tool = tool;
    assert(tool != 0);
    ui->setupUi(this);

    m_cbTargetRot = ui->ROT_TARGET;
    m_selectSource = ui->SELECT_SOURCE;
    m_selectTarget = ui->SELECT_TARGET;
    m_dymX = ui->DYMX;
    m_dymY = ui->DYMY;
    m_dymZ = ui->DYMZ;

    ui->SYNC_HEIGHT->setChecked(m_tool->GetSyncHeight());

    const QChar degreeChar(0260);
    m_cbTargetRot->addItem(QString("0%1").arg(degreeChar));
    m_cbTargetRot->addItem(QString("90%1").arg(degreeChar));
    m_cbTargetRot->addItem(QString("180%1").arg(degreeChar));
    m_cbTargetRot->addItem(QString("270%1").arg(degreeChar));

    switch (m_tool->GetTargetRot())
    {
        case ImageRotationDegrees::Rotate90:
            m_cbTargetRot->setCurrentIndex(1);
            break;
        case ImageRotationDegrees::Rotate180:
            m_cbTargetRot->setCurrentIndex(2);
            break;
        case ImageRotationDegrees::Rotate270:
            m_cbTargetRot->setCurrentIndex(3);
            break;
        default:
            m_cbTargetRot->setCurrentIndex(0);
            break;
    }

    m_dymX->setValue(m_tool->GetDym().x);
    m_dymY->setValue(m_tool->GetDym().y);
    m_dymZ->setValue(m_tool->GetDym().z);

    UpdateButtons();

    connect(m_cbTargetRot, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CTerrainMoveToolPanel::OnChangeTargetRot);
    connect(m_dymX, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CTerrainMoveToolPanel::OnUpdateNumbers);
    connect(m_dymY, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CTerrainMoveToolPanel::OnUpdateNumbers);
    connect(m_dymZ, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CTerrainMoveToolPanel::OnUpdateNumbers);
    connect(m_selectSource, &QPushButton::clicked, this, &CTerrainMoveToolPanel::OnSelectSource);
    connect(m_selectTarget, &QPushButton::clicked, this, &CTerrainMoveToolPanel::OnSelectTarget);
    connect(ui->APPLY, &QPushButton::clicked, this, &CTerrainMoveToolPanel::OnApplyButton);
    connect(ui->SYNC_HEIGHT, &QCheckBox::toggled, this, &CTerrainMoveToolPanel::OnSyncHeight);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveToolPanel::OnApplyButton()
{
    // Call Move() only if we have at least one action selected
    if (ui->V_MOVE->isChecked() || ui->V_COPY->isChecked() || ui->T_COPY->isChecked())
    {
        bool copyVegetation = !ui->V_MOVE->isChecked();
        bool onlyVegetation = ui->T_IGNORE->isChecked();
        bool onlyTerrain = ui->V_IGNORE->isChecked();
        
        // Only vegetation can be moved, but both vegetation and terrain
        // can be copied to different portions of the terrain.
        m_tool->Move(copyVegetation, onlyVegetation, onlyTerrain);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveToolPanel::OnSelectSource()
{
    m_selectSource->setChecked(true);
    m_selectTarget->setChecked(false);
    m_tool->Select(1);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveToolPanel::OnSelectTarget()
{
    m_selectSource->setChecked(false);
    m_selectTarget->setChecked(true);
    m_tool->Select(2);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveToolPanel::OnUpdateNumbers()
{
    m_tool->SetDym(Vec3(m_dymX->value(), m_dymY->value(), m_dymZ->value()));
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveToolPanel::UpdateButtons()
{
    if (m_tool->m_source.isCreated && m_tool->m_target.isCreated)
    {
        ui->APPLY->setEnabled(true);
    }
    else
    {
        ui->APPLY->setEnabled(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveToolPanel::OnChangeTargetRot()
{
    int rot = m_cbTargetRot->currentIndex();
    switch (rot)
    {
        case 1:
            m_tool->SetTargetRot(ImageRotationDegrees::Rotate90);
            break;
        case 2:
            m_tool->SetTargetRot(ImageRotationDegrees::Rotate180);
            break;
        case 3:
            m_tool->SetTargetRot(ImageRotationDegrees::Rotate270);
            break;
        default:
            m_tool->SetTargetRot(ImageRotationDegrees::Rotate0);
            break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveToolPanel::OnSyncHeight()
{
    m_tool->SetSyncHeight(ui->SYNC_HEIGHT->isChecked());
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveToolPanel::UpdateOffsetText(const Vec3& offset, bool bReset)
{
    SetOffsetText(ui->MOVETOOL_OFFSETX, "OffX:", offset.x, bReset);
    SetOffsetText(ui->MOVETOOL_OFFSETY, "OffY:", offset.y, bReset);
    SetOffsetText(ui->MOVETOOL_OFFSETZ, "OffZ:", offset.z, bReset);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveToolPanel::SetOffsetText(QLabel* label, QString title, float offset, bool bReset)
{
    QString offsetText("");

    if (!bReset)
    {
        offsetText = QString("%1 %2").arg(title).arg(offset, 0, 'f', 2);
    }
    label->setText(offsetText);
}
