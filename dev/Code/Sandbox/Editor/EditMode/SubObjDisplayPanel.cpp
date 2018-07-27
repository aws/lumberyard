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
#include "SubObjDisplayPanel.h"
#include "Objects/SubObjSelection.h"
#include <EditMode/ui_SubObjDisplayPanel.h>


// CSubObjDisplayPanel dialog

CSubObjDisplayPanel::CSubObjDisplayPanel(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CSubObjDisplayPanel)
{
    ui->setupUi(this);
    SSubObjSelOptions& opt = g_SubObjSelOptions;

    ui->DISPLAY_BACKFACING->setChecked(opt.bDisplayBackfacing);
    ui->DISPLAY_NORMALS_DISPLAY->setChecked(opt.bDisplayNormals);
    ui->DISPLAY_NORMALS_LENGTH_SPINBOX->setRange(0, 10);
    ui->DISPLAY_NORMALS_LENGTH_SPINBOX->setValue(opt.fNormalsLength);

    switch (opt.displayType)
    {
    case SO_DISPLAY_WIREFRAME:
        ui->WIREFRAME->setChecked(true);
        break;
    case SO_DISPLAY_FLAT:
        ui->FLAT_SHADED->setChecked(true);
        break;
    case SO_DISPLAY_GEOMETRY:
        ui->FULL_GEOMETRY->setChecked(true);
        break;
    }

    void(QSpinBox::* valueChanged)(int) = &QSpinBox::valueChanged;
    connect(ui->DISPLAY_BACKFACING, &QCheckBox::stateChanged, this, &CSubObjDisplayPanel::OnDisplayBackfacing);
    connect(ui->DISPLAY_NORMALS_DISPLAY, &QCheckBox::stateChanged, this, &CSubObjDisplayPanel::OnNormalDisplay);
    connect(ui->DISPLAY_NORMALS_LENGTH_SPINBOX, valueChanged, this, &CSubObjDisplayPanel::OnNormalLength);
    connect(ui->DISPLAY_TYPE_GPBOX, &QGroupBox::clicked, this, &CSubObjDisplayPanel::OnDisplayType);
}

CSubObjDisplayPanel::~CSubObjDisplayPanel()
{
}

void CSubObjDisplayPanel::OnDisplayBackfacing()
{
    SSubObjSelOptions& opt = g_SubObjSelOptions;
    opt.bDisplayBackfacing = ui->DISPLAY_BACKFACING->isChecked();
}

void CSubObjDisplayPanel::OnNormalDisplay()
{
    SSubObjSelOptions& opt = g_SubObjSelOptions;
    opt.bDisplayNormals = ui->DISPLAY_NORMALS_DISPLAY->isChecked();
}

void CSubObjDisplayPanel::OnNormalLength()
{
    SSubObjSelOptions& opt = g_SubObjSelOptions;
    opt.fNormalsLength = ui->DISPLAY_NORMALS_LENGTH_SPINBOX->value();
}

void CSubObjDisplayPanel::OnDisplayType()
{
    SSubObjSelOptions& opt = g_SubObjSelOptions;

    if (ui->WIREFRAME->isChecked())
    {
        opt.displayType = SO_DISPLAY_WIREFRAME;
    }
    else if (ui->FLAT_SHADED->isChecked())
    {
        opt.displayType = SO_DISPLAY_FLAT;
    }
    else if (ui->FULL_GEOMETRY->isChecked())
    {
        opt.displayType = SO_DISPLAY_GEOMETRY;
    }
    else
    {
        Q_ASSERT(false);
    }
}

#include <EditMode/SubObjDisplayPanel.moc>