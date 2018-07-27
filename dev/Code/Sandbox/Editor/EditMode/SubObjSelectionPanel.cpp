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
#include "SubObjSelectionPanel.h"
#include "Objects/SubObjSelection.h"

#include <EditMode/ui_SubObjSelectionPanel.h>
// CSubObjSelectionPanel dialog

CSubObjSelectionPanel::CSubObjSelectionPanel(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CSubObjSelectionPanel)
{
    ui->setupUi(this);

    SSubObjSelOptions& opt = g_SubObjSelOptions;

    ui->SELECT_BY_VERTEX->setChecked(opt.bSelectByVertex);
    ui->IGNORE_BACKFACING->setChecked(opt.bIgnoreBackfacing);
    ui->SOFT_SELECTION->setChecked(opt.bSoftSelection);

    ui->FALLOFF_SPINBOX->setRange(0, 100);
    ui->FALLOFF_SPINBOX->setValue(opt.fSoftSelFalloff);

    void(QSpinBox::* valueChanged)(int) = &QSpinBox::valueChanged;
    connect(ui->SELECT_BY_VERTEX, &QCheckBox::stateChanged, this, &CSubObjSelectionPanel::OnSelectByVertex);
    connect(ui->IGNORE_BACKFACING, &QCheckBox::stateChanged, this, &CSubObjSelectionPanel::OnIgnoreBackfacing);
    connect(ui->SOFT_SELECTION, &QCheckBox::stateChanged, this, &CSubObjSelectionPanel::onSoftSelection);
    connect(ui->FALLOFF_SPINBOX, valueChanged, this, &CSubObjSelectionPanel::OnFallOff);
}

CSubObjSelectionPanel::~CSubObjSelectionPanel()
{
}

void CSubObjSelectionPanel::OnSelectByVertex()
{
    SSubObjSelOptions& opt = g_SubObjSelOptions;
    opt.bSelectByVertex = ui->SELECT_BY_VERTEX->isChecked();
}

void CSubObjSelectionPanel::OnIgnoreBackfacing()
{
    SSubObjSelOptions& opt = g_SubObjSelOptions;
    opt.bIgnoreBackfacing = ui->IGNORE_BACKFACING->isChecked();
}

void CSubObjSelectionPanel::onSoftSelection()
{
    SSubObjSelOptions& opt = g_SubObjSelOptions;
    opt.bSoftSelection = ui->SOFT_SELECTION->isChecked();
}

void CSubObjSelectionPanel::OnFallOff()
{
    SSubObjSelOptions& opt = g_SubObjSelOptions;
    opt.fSoftSelFalloff = ui->FALLOFF_SPINBOX->value();
}

#include <EditMode/SubObjSelectionPanel.moc>