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
#include "BrushPanel.h"
#include <Dialogs/ui_BrushPanel.h>

#include "../Objects/BrushObject.h"
#include "EditMode/VertexMode.h"

// CBrushPanel dialog

CBrushPanel::CBrushPanel(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , ui(new Ui::CBrushPanel)
{
    m_brushObj = 0;
    ui->setupUi(this);

    ui->GEOMETRY_EDIT->SetToolClass(&CSubObjectModeTool::staticMetaObject);
    connect(ui->SAVE_TO_CGF, &QPushButton::clicked, this, &CBrushPanel::OnBnClickedSavetoCGF);
    connect(ui->REFRESH, &QPushButton::clicked, this, &CBrushPanel::OnBnClickedReload);
}

//////////////////////////////////////////////////////////////////////////
CBrushPanel::~CBrushPanel()
{
}

//////////////////////////////////////////////////////////////////////////
void CBrushPanel::SetBrush(CBrushObject* obj)
{
    m_brushObj = obj;
}

//////////////////////////////////////////////////////////////////////////
void CBrushPanel::OnCbnSelendokSides()
{
}

//////////////////////////////////////////////////////////////////////////
void CBrushPanel::OnBnClickedSavetoCGF()
{
    if (m_brushObj)
    {
        QString filename;
        if (CFileUtil::SelectSaveFile("CGF Files (*.cgf)", CRY_GEOMETRY_FILE_EXT, (Path::GetEditingGameDataFolder() + "\\objects").c_str(), filename))
        {
            if (!filename.isEmpty())
            {
                m_brushObj->SaveToCGF(filename);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushPanel::OnBnClickedReload()
{
    if (m_brushObj)
    {
        m_brushObj->ReloadGeometry();
    }
    else
    {
        // Reset all selected brushes.
        CSelectionGroup* selection = GetIEditor()->GetSelection();
        for (int i = 0; i < selection->GetCount(); i++)
        {
            CBaseObject* pBaseObj = selection->GetObject(i);
            if (qobject_cast<CBrushObject*>(pBaseObj))
            {
                ((CBrushObject*)pBaseObj)->ReloadGeometry();
            }
        }
    }
}
