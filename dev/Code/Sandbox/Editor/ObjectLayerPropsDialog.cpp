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
#include "ObjectLayerPropsDialog.h"
#include <ui_ObjectLayerPropsDialog.h>

// CObjectLayerPropsDialog dialog

CObjectLayerPropsDialog::CObjectLayerPropsDialog(QWidget* parent /*=NULL*/)
    : QDialog(parent)
    , m_bVisible(TRUE)
    , m_bFrozen(FALSE)
    , m_bExternal(TRUE)
    , m_bExportToGame(TRUE)
    , m_bExportLayerPak(TRUE)
    , m_bHavePhysics(TRUE)
    , m_specs(eSpecType_All)
    , m_bDefaultLoaded(FALSE)
    , ui(new Ui::CObjectLayerPropsDialog)
{
    ui->setupUi(this);

    m_bMainLayer = FALSE;
    m_color = palette().color(QPalette::Button);

    connect(ui->PLATFORM_ALL, &QAbstractButton::clicked, this, &CObjectLayerPropsDialog::OnBnClickedPlatform);
    connect(ui->PLATFORM_SPEC, &QAbstractButton::clicked, this, &CObjectLayerPropsDialog::OnBnClickedPlatform);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CObjectLayerPropsDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &CObjectLayerPropsDialog::reject);
}

CObjectLayerPropsDialog::~CObjectLayerPropsDialog()
{
}

void CObjectLayerPropsDialog::UpdateData(bool fromUi)
{
    if (fromUi)
    {
        m_bVisible = ui->VISIBLE->isChecked();
        m_bFrozen = ui->FROZEN->isChecked();
        m_bExternal = ui->EXTERNAL->isChecked();
        m_bExportToGame = ui->EXPORT_TO_GAME->isChecked();
        m_bExportLayerPak = ui->EXPORT_LAYERPAK->isChecked();
        m_bDefaultLoaded = ui->LAYER_DEFAULT_LOADED->isChecked();
        m_bHavePhysics = ui->HAVE_PHYSICS->isChecked();
        m_name = ui->STRING->text();
        m_color = ui->colorButton->Color();
        ReadSpecsUI();
    }
    else
    {
        ui->VISIBLE->setChecked(m_bVisible);
        ui->FROZEN->setChecked(m_bFrozen);
        ui->EXTERNAL->setChecked(m_bExternal);
        ui->EXPORT_TO_GAME->setChecked(m_bExportToGame);
        ui->EXPORT_LAYERPAK->setChecked(m_bExportLayerPak);
        ui->LAYER_DEFAULT_LOADED->setChecked(m_bDefaultLoaded);
        ui->HAVE_PHYSICS->setChecked(m_bHavePhysics);
        ui->STRING->setText(m_name);
        ui->colorButton->SetColor(m_color);
        UpdateSpecsUI();
    }
}

void CObjectLayerPropsDialog::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);

    ui->colorButton->SetColor(m_color);

    if (m_bMainLayer)
    {
        ui->EXTERNAL->setEnabled(false);
    }

    ui->PLATFORM_PC->setChecked(true);
    ui->PLATFORM_XBOXONE->setChecked(true);
    ui->PLATFORM_PS4->setChecked(true);

    UpdateSpecsUI();

    UpdateData(false);
}

void CObjectLayerPropsDialog::UpdateSpecsUI()
{
    BOOL bNotAllSpecTypes = m_specs != eSpecType_All ? true : false;

    ui->PLATFORM_ALL->setChecked(!bNotAllSpecTypes);
    ui->PLATFORM_SPEC->setChecked(bNotAllSpecTypes);
    ui->PLATFORM_PC->setEnabled(bNotAllSpecTypes);
    ui->PLATFORM_XBOXONE->setEnabled(bNotAllSpecTypes);
    ui->PLATFORM_PS4->setEnabled(bNotAllSpecTypes);

    if (m_specs != eSpecType_All)
    {
        ui->PLATFORM_PC->setChecked(m_specs & eSpecType_PC ? true : false);
        ui->PLATFORM_XBOXONE->setChecked(m_specs & eSpecType_XBoxOne ? true : false);
        ui->PLATFORM_PS4->setChecked(m_specs & eSpecType_PS4 ? true : false);
    }
}

void CObjectLayerPropsDialog::ReadSpecsUI()
{
    if (ui->PLATFORM_ALL->isChecked())
    {
        m_specs = eSpecType_All;
    }
    else
    {
        m_specs = 0;
        if (ui->PLATFORM_PC->isChecked())
        {
            m_specs |= eSpecType_PC;
        }

        if (ui->PLATFORM_XBOXONE->isChecked())
        {
            m_specs |= eSpecType_XBoxOne;
        }

        if (ui->PLATFORM_PS4->isChecked())
        {
            m_specs |= eSpecType_PS4;
        }
    }
}

void CObjectLayerPropsDialog::OnBnClickedPlatform()
{
    ReadSpecsUI();
    UpdateSpecsUI();
}

void CObjectLayerPropsDialog::accept()
{
    UpdateData(true);
    QDialog::accept();
}

