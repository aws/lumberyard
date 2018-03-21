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
#include "SimpleEntity.h"
#include "PanelTreeBrowser.h"
#include "BrushObject.h"

//////////////////////////////////////////////////////////////////////////
bool CSimpleEntity::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool bRes = false;
    if (file.isEmpty())
    {
        bRes = CEntityObject::Init(ie, prev, "");
    }
    else
    {
        bRes = CEntityObject::Init(ie, prev, "BasicEntity");
        SetClass(m_entityClass);
        SetGeometryFile(file);
    }

    return bRes;
}

//////////////////////////////////////////////////////////////////////////
void CSimpleEntity::SetGeometryFile(const QString& filename)
{
    if (m_pProperties)
    {
        IVariable* pModelVar = m_pProperties->FindVariable("object_Model");
        if (pModelVar)
        {
            pModelVar->Set(filename);
            SetModified(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
QString CSimpleEntity::GetGeometryFile() const
{
    QString filename;
    if (m_pProperties)
    {
        IVariable* pModelVar = m_pProperties->FindVariable("object_Model");
        if (pModelVar)
        {
            pModelVar->Get(filename);
        }
    }
    return filename;
}

//////////////////////////////////////////////////////////////////////////
bool CSimpleEntity::ConvertFromObject(CBaseObject* object)
{
    CEntityObject::ConvertFromObject(object);

    if (qobject_cast<CBrushObject*>(object))
    {
        CBrushObject* pBrushObject = ( CBrushObject* )object;

        IStatObj* prefab = pBrushObject->GetIStatObj();
        if (!prefab)
        {
            return false;
        }

        // Copy entity shadow parameters.
        int rndFlags = pBrushObject->GetRenderFlags();

        mv_castShadowMinSpec = (rndFlags & ERF_CASTSHADOWMAPS) ? CONFIG_LOW_SPEC : END_CONFIG_SPEC_ENUM;

        mv_outdoor = (rndFlags & ERF_OUTDOORONLY) != 0;
        mv_ratioLOD = pBrushObject->GetRatioLod();
        mv_viewDistanceMultiplier = pBrushObject->GetViewDistanceMultiplier();
        mv_recvWind = (rndFlags & ERF_RECVWIND) != 0;
        mv_noDecals = (rndFlags & ERF_NO_DECALNODE_DECALS) != 0;

        SetClass("BasicEntity");
        SpawnEntity();
        SetGeometryFile(prefab->GetFilePath());
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CSimpleEntity::BeginEditParams(IEditor* ie, int flags)
{
    CEntityObject::BeginEditParams(ie, flags);

    QString filename = GetGeometryFile();

    if (gSettings.bGeometryBrowserPanel)
    {
        if (!filename.isEmpty())
        {
            if (!ms_pTreePanel)
            {
                auto flags = CPanelTreeBrowser::NO_DRAGDROP | CPanelTreeBrowser::NO_PREVIEW | CPanelTreeBrowser::SELECT_ONCLICK;
                ms_pTreePanel = new CPanelTreeBrowser(functor(*this, &CSimpleEntity::OnFileChange), GetClassDesc()->GetFileSpec(), flags);
            }
            if (ms_treePanelId == 0)
            {
                ms_treePanelId = AddUIPage(tr("Geometry").toUtf8().data(), ms_pTreePanel);
            }
        }

        if (ms_pTreePanel)
        {
            ms_pTreePanel->SetSelectCallback(functor(*this, &CSimpleEntity::OnFileChange));
            ms_pTreePanel->SelectFile(filename);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSimpleEntity::OnFileChange(QString filename)
{
    CUndo undo("Entity Geom Modify");
    StoreUndo("Entity Geom Modify");
    SetGeometryFile(filename);
}

//////////////////////////////////////////////////////////////////////////
//! Analyze errors for this object.
void CSimpleEntity::Validate(IErrorReport* report)
{
    CEntityObject::Validate(report);

    // Checks if object loaded.
}

//////////////////////////////////////////////////////////////////////////
bool CSimpleEntity::IsSimilarObject(CBaseObject* pObject)
{
    if (pObject->GetClassDesc() == GetClassDesc() && pObject->metaObject() == metaObject())
    {
        CSimpleEntity* pEntity = ( CSimpleEntity* )pObject;
        if (GetGeometryFile() == pEntity->GetGeometryFile())
        {
            return true;
        }
    }
    return false;
}

#include <Objects/SimpleEntity.moc>
