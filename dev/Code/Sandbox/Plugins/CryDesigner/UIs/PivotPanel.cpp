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
#include "PivotPanel.h"
#include "DesignerPanel.h"
#include "Tools/Select/PivotTool.h"
#include "Tools/DesignerTool.h"

#include <QRadioButton>
#include <QBoxLayout>

void PivotPanel::Done()
{
    QSettings settings;
    for (auto g : QString(CD::sessionName).split('\\'))
        settings.beginGroup(g);

    if (m_pBoundBoxRadioBox->isChecked())
    {
        settings.setValue("PIVOTSELECTIONTYPE", 0);
    }
    else
    {
        settings.setValue("PIVOTSELECTIONTYPE", 1);
    }
    CD::RemoveAttributeWidget(m_pPivotTool);
}

PivotPanel::PivotPanel(PivotTool* pPivotTool)
    : m_pPivotTool(pPivotTool)
{
    QBoxLayout* pBoxLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    m_pBoundBoxRadioBox = new QRadioButton("Bound Box");
    m_pMeshRadioBox = new QRadioButton("Mesh");

    pBoxLayout->addWidget(m_pBoundBoxRadioBox);
    pBoxLayout->addWidget(m_pMeshRadioBox);

    QObject::connect(m_pBoundBoxRadioBox, &QRadioButton::clicked, this, [ = ] {m_pPivotTool->SetSelectionType(PivotTool::ePST_BoundBox);
        });
    QObject::connect(m_pMeshRadioBox, &QRadioButton::clicked, this, [ = ] {m_pPivotTool->SetSelectionType(PivotTool::ePST_Designer);
        });

    QSettings settings;
    for (auto g : QString(CD::sessionName).split('\\'))
        settings.beginGroup(g);

    if (settings.value("PIVOTSELECTIONTYPE", 0).toInt() == 0)
    {
        m_pBoundBoxRadioBox->setChecked(true);
        m_pPivotTool->SetSelectionType(PivotTool::ePST_BoundBox, true);
    }
    else
    {
        m_pMeshRadioBox->setChecked(true);
        m_pPivotTool->SetSelectionType(PivotTool::ePST_Designer, true);
    }

    setLayout(pBoxLayout);
    CD::SetAttributeWidget(m_pPivotTool, this);
}