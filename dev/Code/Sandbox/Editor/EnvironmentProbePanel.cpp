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
#include "EnvironmentProbePanel.h"
#include "Objects/EnvironmentProbeObject.h"
#include "Util/CubemapUtils.h"
#include <ui_EnvironmentProbePanel.h>

/*

  CEnvironmentProbePanel

*/

//
CEnvironmentProbePanel::CEnvironmentProbePanel(QWidget* parent /*= nullptr*/)
    : QWidget(parent)
    , ui(new Ui::CEnvironmentProbePanel)
    , m_pEntity(nullptr)
{
    ui->setupUi(this);
    connect(ui->GENERATECUBEMAPS_BUTTON, &QPushButton::clicked, this, &CEnvironmentProbePanel::OnGenerateCubemap);
    connect(ui->GENERATE_ALL_CUBEMAPS_BUTTON, &QPushButton::clicked, this, &CEnvironmentProbePanel::OnGenerateAllCubemaps);
}

void CEnvironmentProbePanel::SetEntity(CEnvironementProbeObject* pEntity)
{
    m_pEntity = pEntity;
}

//function will generate the cubemap for this probes position
void CEnvironmentProbePanel::OnGenerateCubemap()
{
    m_pEntity->GenerateCubemap();
}

//function will recurse all probes and generate a cubemap for each
void CEnvironmentProbePanel::OnGenerateAllCubemaps()
{
    CubemapUtils::RegenerateAllEnvironmentProbeCubemaps();
}