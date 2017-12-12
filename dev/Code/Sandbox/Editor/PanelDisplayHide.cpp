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
#include "PanelDisplayHide.h"
#include "DisplaySettings.h"
#include <ui_PanelDisplayHide.h>

/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayHide dialog


CPanelDisplayHide::CPanelDisplayHide(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CPanelDisplayHide)
    , m_initialized(false)
{
    ui->setupUi(this);

    connect(ui->HIDE_ALL, &QPushButton::clicked, this, &CPanelDisplayHide::OnHideAll);
    connect(ui->HIDE_NONE, &QPushButton::clicked, this, &CPanelDisplayHide::OnHideNone);
    connect(ui->HIDE_INVERT, &QPushButton::clicked, this, &CPanelDisplayHide::OnHideInvert);
    connect(ui->HIDE_ENTITY, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);
    connect(ui->HIDE_PREFABS, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);
    connect(ui->HIDE_GROUP, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);
    connect(ui->HIDE_PATH, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);
    connect(ui->HIDE_TAGPOINT, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);
    connect(ui->HIDE_VOLUME, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);
    connect(ui->HIDE_BRUSH, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);
    connect(ui->HIDE_DECALS, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);
    connect(ui->HIDE_SOLIDS, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);
    connect(ui->HIDE_GEOMCACHES, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);
    connect(ui->HIDE_ROADS, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);
    connect(ui->HIDE_OTHER, &QCheckBox::clicked, this, &CPanelDisplayHide::OnChangeHideMask);

    //KDAB--no resource found for these checkboxes.
    //connect(ui->HIDE_BUILDING, &QCheckBox::toggled, this, &CPanelDisplayHide::OnChangeHideMask);
    //connect(ui->HIDE_SOUND, &QCheckBox::toggled, this, &CPanelDisplayHide::OnChangeHideMask);
    //connect(ui->HIDE_STATOBJ, &QCheckBox::toggled, this, &CPanelDisplayHide::OnChangeHideMask);

    GetIEditor()->RegisterNotifyListener(this);
}

CPanelDisplayHide::~CPanelDisplayHide()
{
    GetIEditor()->UnregisterNotifyListener(this);
}
/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayHide message handlers

void CPanelDisplayHide::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    if (!m_initialized)
    {
        m_initialized = true;
        m_mask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
        SetCheckButtons();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayHide::SetMask()
{
    GetIEditor()->GetDisplaySettings()->SetObjectHideMask(m_mask);
    GetIEditor()->GetObjectManager()->InvalidateVisibleList();
    GetIEditor()->UpdateViews(eUpdateObjects);
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayHide::OnHideAll()
{
    m_mask = 0xFFFFFFFF;
    SetCheckButtons();
    SetMask();
}

void CPanelDisplayHide::OnHideNone()
{
    m_mask = 0;
    SetCheckButtons();
    SetMask();
}

void CPanelDisplayHide::OnHideInvert()
{
    m_mask = ~m_mask;
    SetCheckButtons();
    SetMask();
}

void CPanelDisplayHide::SetCheckButtons()
{
    // Check or uncheck buttons.
    ui->HIDE_ENTITY->setChecked(m_mask & OBJTYPE_ENTITY);
    ui->HIDE_PREFABS->setChecked(m_mask & OBJTYPE_PREFAB);
    ui->HIDE_GROUP->setChecked(m_mask & OBJTYPE_GROUP);
    ui->HIDE_TAGPOINT->setChecked(m_mask & OBJTYPE_TAGPOINT);
    ui->HIDE_PATH->setChecked(m_mask & OBJTYPE_SHAPE);
    ui->HIDE_VOLUME->setChecked(m_mask & OBJTYPE_VOLUME);
    ui->HIDE_BRUSH->setChecked(m_mask & OBJTYPE_BRUSH);
    ui->HIDE_AIPOINT->setChecked(m_mask & OBJTYPE_AIPOINT);
    ui->HIDE_DECALS->setChecked(m_mask & OBJTYPE_DECAL);
    ui->HIDE_SOLIDS->setChecked((m_mask & OBJTYPE_SOLID) | (m_mask & OBJTYPE_VOLUMESOLID));
    ui->HIDE_ROADS->setChecked(m_mask & OBJTYPE_ROAD);
    ui->HIDE_GEOMCACHES->setChecked(m_mask & OBJTYPE_GEOMCACHE);
    ui->HIDE_OTHER->setChecked(m_mask & OBJTYPE_OTHER);
}

void CPanelDisplayHide::OnChangeHideMask()
{
    // TODO: Add your control notification handler code here
    m_mask = 0;

    // Check or uncheck buttons.
    m_mask |= ui->HIDE_ENTITY->isChecked() ? OBJTYPE_ENTITY : 0;
    m_mask |= ui->HIDE_PREFABS->isChecked() ? OBJTYPE_PREFAB : 0;
    m_mask |= ui->HIDE_GROUP->isChecked() ? OBJTYPE_GROUP : 0;
    m_mask |= ui->HIDE_TAGPOINT->isChecked() ? OBJTYPE_TAGPOINT : 0;
    m_mask |= ui->HIDE_AIPOINT->isChecked() ? OBJTYPE_AIPOINT : 0;
    m_mask |= ui->HIDE_PATH->isChecked() ? OBJTYPE_SHAPE : 0;
    m_mask |= ui->HIDE_VOLUME->isChecked() ? OBJTYPE_VOLUME : 0;
    m_mask |= ui->HIDE_BRUSH->isChecked() ? OBJTYPE_BRUSH : 0;
    m_mask |= ui->HIDE_DECALS->isChecked() ? OBJTYPE_DECAL : 0;
    m_mask |= ui->HIDE_SOLIDS->isChecked() ? OBJTYPE_SOLID | OBJTYPE_VOLUMESOLID : 0;
    m_mask |= ui->HIDE_ROADS->isChecked() ? OBJTYPE_ROAD : 0;
    m_mask |= ui->HIDE_GEOMCACHES->isChecked() ? OBJTYPE_GEOMCACHE : 0;
    m_mask |= ui->HIDE_OTHER->isChecked() ? OBJTYPE_OTHER : 0;

    SetMask();
}

void CPanelDisplayHide::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnDisplayRenderUpdate:
        m_mask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
        SetCheckButtons();
        break;
    }
}

#include <PanelDisplayHide.moc>