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
#include "PickEntitiesPanel.h"
#include "Objects/BaseObject.h"
#include "Objects/EntityObject.h"
#include "Controls/PickObjectButton.h"
#include "Viewport.h"
#include <ui_PickEntitiesPanel.h>
#include "QtUtil.h"

// CShapePanel dialog

//////////////////////////////////////////////////////////////////////////
CPickEntitiesPanel::CPickEntitiesPanel(QWidget* pParent /* = nullptr */)
    : QWidget(pParent)
    , ui(new Ui::CPickEntitiesPanel)
{
    ui->setupUi(this);
    connect(ui->SELECT, &QPushButton::clicked, this, &CPickEntitiesPanel::OnBnClickedSelect);
    connect(ui->REMOVE, &QPushButton::clicked, this, &CPickEntitiesPanel::OnBnClickedRemove);
    m_entities = ui->ENTITIES;
    connect(m_entities, &QListWidget::itemDoubleClicked, this, &CPickEntitiesPanel::OnLbnDblclkEntities);
    ui->PICK->SetPickCallback(this, "Pick Entity");
}

CPickEntitiesPanel::~CPickEntitiesPanel()
{
}

// CShapePanel message handlers

void CPickEntitiesPanel::SetOwner(IPickEntitesOwner* pOwner)
{
    assert(pOwner);
    m_pOwner = pOwner;
    ReloadEntities();
}

//////////////////////////////////////////////////////////////////////////
void CPickEntitiesPanel::OnPick(CBaseObject* picked)
{
    assert(m_pOwner);
    CUndo undo("[PickEntityOwner] Add Entity");
    m_pOwner->AddEntity((CEntityObject*)picked);
    ReloadEntities();
}

//////////////////////////////////////////////////////////////////////////
bool CPickEntitiesPanel::OnPickFilter(CBaseObject* picked)
{
    assert(picked != 0);
    return picked->GetType() == OBJTYPE_ENTITY;
}

//////////////////////////////////////////////////////////////////////////
void CPickEntitiesPanel::OnCancelPick()
{
}

//////////////////////////////////////////////////////////////////////////
void CPickEntitiesPanel::OnBnClickedSelect()
{
    assert(m_pOwner);
    int sel = m_entities->selectedItems().empty() ? -1 : m_entities->row(m_entities->selectedItems().front());
    if (sel != -1)
    {
        CBaseObject* obj = m_pOwner->GetEntity(sel);
        if (obj)
        {
            CUndo undo("Select Object");
            GetIEditor()->ClearSelection();
            GetIEditor()->SelectObject(obj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPickEntitiesPanel::ReloadEntities()
{
    if (!m_pOwner)
    {
        return;
    }

    m_entities->clear();
    for (int i = 0; i < m_pOwner->GetEntityCount(); i++)
    {
        CBaseObject* obj = m_pOwner->GetEntity(i);
        if (obj)
        {
            m_entities->addItem(obj->GetName());
        }
        else
        {
            m_entities->addItem("<Null>");
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPickEntitiesPanel::OnBnClickedRemove()
{
    assert(m_pOwner);
    int sel = m_entities->selectedItems().empty() ? -1 : m_entities->row(m_entities->selectedItems().front());
    if (sel != -1)
    {
        CUndo undo("[PickEntityOwner] Remove Entity");
        if (sel < m_pOwner->GetEntityCount())
        {
            m_pOwner->RemoveEntity(sel);
        }
        ReloadEntities();
    }
}

void CPickEntitiesPanel::OnLbnDblclkEntities(QListWidgetItem* item)
{
    // Select current entity.
    OnBnClickedSelect();
}
