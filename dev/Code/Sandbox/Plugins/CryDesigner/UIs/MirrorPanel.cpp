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
#include "DesignerPanel.h"
#include "MirrorPanel.h"
#include "Core/Model.h"
#include "Tools/Modify/MirrorTool.h"
#include "Tools/DesignerTool.h"
#include <QPushButton>
#include <QGridLayout>

MirrorPanel::MirrorPanel(MirrorTool* pMirrorTool)
    : m_pMirrorTool(pMirrorTool)
{
    QGridLayout* pGridLayout = new QGridLayout;

    m_pApplyButton = new QPushButton("Apply");
    m_pInvertButton = new QPushButton("Invert");
    m_pFreezeButton = new QPushButton("Freeze");
    m_pCenterPivotButton = new QPushButton("Center Pivot");
    m_pAlignXButton = new QPushButton("AlignX");
    m_pAlignYButton = new QPushButton("AlignY");
    m_pAlignZButton = new QPushButton("AlignZ");

    pGridLayout->addWidget(m_pApplyButton, 0, 0, 1, 3, Qt::AlignTop);
    pGridLayout->addWidget(m_pFreezeButton, 0, 3, 1, 3, Qt::AlignTop);
    pGridLayout->addWidget(m_pInvertButton, 1, 0, 1, 3, Qt::AlignTop);
    pGridLayout->addWidget(m_pCenterPivotButton, 1, 3, 1, 3, Qt::AlignTop);
    pGridLayout->addWidget(m_pAlignXButton, 2, 0, 1, 2, Qt::AlignTop);
    pGridLayout->addWidget(m_pAlignYButton, 2, 2, 1, 2, Qt::AlignTop);
    pGridLayout->addWidget(m_pAlignZButton, 2, 4, 1, 2, Qt::AlignTop);

    QObject::connect(m_pApplyButton, &QPushButton::clicked, this, [ = ] {m_pMirrorTool->ApplyMirror();
        });
    QObject::connect(m_pInvertButton, &QPushButton::clicked, this, [ = ] {m_pMirrorTool->InvertSlicePlane();
        });
    QObject::connect(m_pFreezeButton, &QPushButton::clicked, this, [ = ] {m_pMirrorTool->FreezeModel();
        });
    QObject::connect(m_pCenterPivotButton, &QPushButton::clicked, this, [ = ] {m_pMirrorTool->CenterPivot();
        });
    QObject::connect(m_pAlignXButton, &QPushButton::clicked, this, [ = ] {m_pMirrorTool->AlignSlicePlane(BrushVec3(1, 0, 0));
        });
    QObject::connect(m_pAlignYButton, &QPushButton::clicked, this, [ = ] {m_pMirrorTool->AlignSlicePlane(BrushVec3(0, 1, 0));
        });
    QObject::connect(m_pAlignZButton, &QPushButton::clicked, this, [ = ] {m_pMirrorTool->AlignSlicePlane(BrushVec3(0, 0, 1));
        });

    Update();

    setLayout(pGridLayout);

    CD::SetAttributeWidget(m_pMirrorTool, this);
}

void MirrorPanel::Done()
{
    CD::RemoveAttributeWidget(m_pMirrorTool);
}

void MirrorPanel::Update()
{
    QPushButton* buttons[] = { m_pAlignXButton, m_pAlignYButton, m_pAlignZButton, m_pApplyButton, m_pInvertButton, m_pCenterPivotButton };
    BOOL bMirrorType = m_pMirrorTool->GetModel()->CheckModeFlag(CD::eDesignerMode_Mirror);
    for (int i = 0; i < sizeof(buttons) / sizeof(*buttons); ++i)
    {
        QPushButton* pButton = buttons[i];
        pButton->setEnabled(!bMirrorType);
    }
    m_pFreezeButton->setEnabled(bMirrorType);
}