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
#include "ResetXFormPanel.h"
#include "DesignerPanel.h"
#include "Tools/Misc/ResetXFormTool.h"
#include "Tools/DesignerTool.h"

#include <QGridLayout>
#include <QPushButton>
#include <QCheckBox>

void ResetXFormPanel::Done()
{
    QSettings settings;
    for (auto g : QString(CD::sessionName).split('\\'))
        settings.beginGroup(g);

    settings.setValue("ResetXForm_Position", m_pPositionCheckBox->isChecked() ? 1 : 0);
    settings.setValue("ResetXForm_Rotation", m_pRotationCheckBox->isChecked() ? 1 : 0);
    settings.setValue("ResetXForm_Scale", m_pScaleCheckBox->isChecked() ? 1 : 0);
    CD::RemoveAttributeWidget(m_pResetXFormTool);
}

ResetXFormPanel::ResetXFormPanel(ResetXFormTool* pResetXFormTool)
    : m_pResetXFormTool(pResetXFormTool)
{
    QGridLayout* pGridLayout = new QGridLayout;

    m_pPositionCheckBox = new QCheckBox("Position");
    m_pRotationCheckBox = new QCheckBox("Rotation");
    m_pScaleCheckBox = new QCheckBox("Scale");
    QPushButton* pResetXFormPushButton = new QPushButton("Reset X Form");

    QSettings settings;
    for (auto g : QString(CD::sessionName).split('\\'))
        settings.beginGroup(g);

    if (settings.value("ResetXForm_Position", 1).toInt())
    {
        m_pPositionCheckBox->setChecked(true);
    }
    else
    {
        m_pPositionCheckBox->setChecked(false);
    }

    if (settings.value("ResetXForm_Rotation", 1).toInt())
    {
        m_pRotationCheckBox->setChecked(true);
    }
    else
    {
        m_pRotationCheckBox->setChecked(false);
    }

    if (settings.value("ResetXForm_Scale", 1).toInt())
    {
        m_pScaleCheckBox->setChecked(true);
    }
    else
    {
        m_pScaleCheckBox->setChecked(false);
    }

    pGridLayout->addWidget(pResetXFormPushButton, 0, 0, 1, 3);
    pGridLayout->addWidget(m_pPositionCheckBox, 1, 0);
    pGridLayout->addWidget(m_pRotationCheckBox, 1, 1);
    pGridLayout->addWidget(m_pScaleCheckBox, 1, 2);

    QObject::connect(pResetXFormPushButton, &QPushButton::clicked, this, [ = ] {OnResetXFormButtonClick();
        });

    setLayout(pGridLayout);

    CD::SetAttributeWidget(m_pResetXFormTool, this);
}

void ResetXFormPanel::OnResetXFormButtonClick()
{
    int nResetFlag = 0;

    if (m_pRotationCheckBox->isChecked())
    {
        nResetFlag |= CD::eResetXForm_Rotation;
    }

    if (m_pScaleCheckBox->isChecked())
    {
        nResetFlag |= CD::eResetXForm_Scale;
    }

    if (m_pPositionCheckBox->isChecked())
    {
        nResetFlag |= CD::eResetXForm_Position;
    }

    m_pResetXFormTool->FreezeXForm(nResetFlag);
    if (CD::GetDesigner())
    {
        CD::GetDesigner()->SwitchToSelectTool();
    }
}