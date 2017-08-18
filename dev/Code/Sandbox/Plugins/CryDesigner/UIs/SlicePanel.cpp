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
#include "SlicePanel.h"
#include "DesignerPanel.h"
#include "Tools/Modify/SliceTool.h"
#include "Tools/DesignerTool.h"
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

SlicePanel::SlicePanel(SliceTool* pSliceTool)
    : m_pSliceTool(pSliceTool)
{
    QGridLayout* pGridLayout = new QGridLayout;

    QPushButton* pTakeFrontAwayButton = new QPushButton("Take Front Away");
    QPushButton* pTakeBackAwayButton = new QPushButton("Take Back Away");
    QPushButton* pClipButton = new QPushButton("Clip");
    QPushButton* pDivideButton = new QPushButton("Divide");
    QPushButton* pAlignXButton = new QPushButton("Align X");
    QPushButton* pAlignYButton = new QPushButton("Align Y");
    QPushButton* pAlignZButton = new QPushButton("Align Z");
    QPushButton* pInvertButton = new QPushButton("Invert");
    m_SliceNumberEdit = new QLineEdit();
    m_SliceNumberEdit->setText(QString("%1").arg(m_pSliceTool->GetNumberSlicePlane()));

    pGridLayout->addWidget(pTakeFrontAwayButton, 0, 0, 1, 3);
    pGridLayout->addWidget(pTakeBackAwayButton, 0, 3, 1, 3);
    pGridLayout->addWidget(pClipButton, 1, 0, 1, 3);
    pGridLayout->addWidget(pDivideButton, 1, 3, 1, 3);
    pGridLayout->addWidget(new QLabel("Slice Number"), 2, 0, 1, 2);
    pGridLayout->addWidget(m_SliceNumberEdit, 2, 2, 1, 4);
    pGridLayout->addWidget(pAlignXButton, 3, 0, 1, 2);
    pGridLayout->addWidget(pAlignYButton, 3, 2, 1, 2);
    pGridLayout->addWidget(pAlignZButton, 3, 4, 1, 2);
    pGridLayout->addWidget(pInvertButton, 4, 0, 1, 6);

    QObject::connect(pTakeFrontAwayButton, &QPushButton::clicked, this, [ = ] {m_pSliceTool->SliceFrontPart();
        });
    QObject::connect(pTakeBackAwayButton, &QPushButton::clicked, this, [ = ] {m_pSliceTool->SliceBackPart();
        });
    QObject::connect(pClipButton, &QPushButton::clicked, this, [ = ] {m_pSliceTool->Clip();
        });
    QObject::connect(pDivideButton, &QPushButton::clicked, this, [ = ] {m_pSliceTool->Divide();
        });
    QObject::connect(pAlignXButton, &QPushButton::clicked, this, [ = ] {m_pSliceTool->AlignSlicePlane(BrushVec3(1, 0, 0));
        });
    QObject::connect(pAlignYButton, &QPushButton::clicked, this, [ = ] {m_pSliceTool->AlignSlicePlane(BrushVec3(0, 1, 0));
        });
    QObject::connect(pAlignZButton, &QPushButton::clicked, this, [ = ] {m_pSliceTool->AlignSlicePlane(BrushVec3(0, 0, 1));
        });
    QObject::connect(pInvertButton, &QPushButton::clicked, this, [ = ] {m_pSliceTool->InvertSlicePlane();
        });
    QObject::connect(m_SliceNumberEdit, &QLineEdit::editingFinished, this, [ = ] {m_pSliceTool->SetNumberSlicePlane(m_SliceNumberEdit->text().toInt());
        });

    setLayout(pGridLayout);

    CD::SetAttributeWidget(m_pSliceTool, this);
}

void SlicePanel::Done()
{
    CD::RemoveAttributeWidget(m_pSliceTool);
}