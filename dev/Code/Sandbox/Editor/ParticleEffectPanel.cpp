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

// Description : implementation file


#include "StdAfx.h"
#include "ParticleEffectPanel.h"
#include "Objects/ParticleEffectObject.h"

#include <ui_ParticleEffectPanel.h>

CParticleEffectPanel::CParticleEffectPanel(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , ui(new Ui::CParticleEffectPanel)
{
    ui->setupUi(this);

    connect(ui->m_buttonGoToDatabase, &QPushButton::clicked, this, &CParticleEffectPanel::OnBnClickedGotodatabase);
}

CParticleEffectPanel::~CParticleEffectPanel()
{
}

/////////////////////////////////////////////////////////////////////////////
// CParticleEffectPanel message handlers
void CParticleEffectPanel::SetParticleEffectEntity(CParticleEffectObject* entity)
{
    assert(entity);
    m_pEntity = entity;
}

void CParticleEffectPanel::OnBnClickedGotodatabase()
{
    if (m_pEntity)
    {
        m_pEntity->OnMenuGoToDatabase();
    }
}

// Confetti Begin: Jurecka
void CParticleEffectPanel::OnBnClickedGotoEditor()
{
    if (m_pEntity)
    {
        m_pEntity->OnMenuGoToEditor();
    }
}
// Confetti End: Jurecka

#include <ParticleEffectPanel.moc>
