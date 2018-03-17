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
#include "StdAfx.h"
#include "ActorEntity.h"
#include "PanelTreeBrowser.h"
#include "BrushObject.h"

//////////////////////////////////////////////////////////////////////////
bool CActorEntity::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool bRes = false;
    if (file.isEmpty())
    {
        bRes = CEntityObject::Init(ie, prev, "");
        SetClass("CActorWrapper");
    }
    else
    {
        bRes = CEntityObject::Init(ie, prev, "CActorWrapper");
        SetClass("CActorWrapper", false, true);
        SetEntityPropertyString("Prototype", file);
        SetPrototypeFile(file);
        QString name = Path::GetFileName(file);
        if (!name.isEmpty())
        {
            SetUniqueName(name);
        }
    }

    return bRes;
}

//////////////////////////////////////////////////////////////////////////
void CActorEntity::InitVariables()
{
    AddVariable(mv_prototype, "Prototype", functor(*this, &CActorEntity::OnPrototypeFileChange), IVariable::DT_OBJECT);
    CEntityObject::InitVariables();
}

//////////////////////////////////////////////////////////////////////////
void CActorEntity::SetPrototypeFile(const QString& filename)
{
    if (filename != mv_prototype)
    {
        mv_prototype = filename;
    }

    OnPrototypeFileChange(&mv_prototype);
}

//////////////////////////////////////////////////////////////////////////
void CActorEntity::OnPrototypeFileChange(IVariable* pVar)
{
    if (m_pEntity)
    {
        if (!m_bVisible)
        {
            m_pEntity->Hide(false);    // Unhide to clear hidden state
        }

        SetEntityPropertyString("Prototype", mv_prototype);
        QString filename = static_cast<QString>(mv_prototype);

        if (!m_bVisible)
        {
            m_pEntity->Hide(true);
        }
        CalcBBox();
        InvalidateTM(0);
        OnRenderFlagsChange(nullptr);
        m_statObjValidator.Validate(m_pEntity->GetStatObj(ENTITY_SLOT_ACTUAL), GetRenderMaterial(), m_pEntity->GetPhysics());
        if (ms_pTreePanel)
        {
            ms_pTreePanel->SelectFile(filename);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CActorEntity::SpawnEntity()
{
    CEntityObject::SpawnEntity();
    SetPrototypeFile(mv_prototype);
}

//////////////////////////////////////////////////////////////////////////
QString CActorEntity::GetPrototypeFile() const
{
    return mv_prototype;
}

//////////////////////////////////////////////////////////////////////////
bool CActorEntity::ConvertFromObject(CBaseObject* object)
{
    return CEntityObject::ConvertFromObject(object);
}

//////////////////////////////////////////////////////////////////////////
void CActorEntity::BeginEditParams(IEditor* ie, int flags)
{
    CEntityObject::BeginEditParams(ie, flags);

    QString filename = GetPrototypeFile();

    if (gSettings.bGeometryBrowserPanel)
    {
        if (!filename.isEmpty())
        {
            if (!ms_pTreePanel)
            {
                auto flags = CPanelTreeBrowser::NO_DRAGDROP | CPanelTreeBrowser::NO_PREVIEW | CPanelTreeBrowser::SELECT_ONCLICK;
                ms_pTreePanel = new CPanelTreeBrowser(functor(*this, &CActorEntity::OnFileChange), GetClassDesc()->GetFileSpec(), flags);
            }
            if (ms_treePanelId == 0)
            {
                ms_treePanelId = AddUIPage(tr("Prototypes").toUtf8().data(), ms_pTreePanel, false);
            }
        }

        if (ms_pTreePanel)
        {
            ms_pTreePanel->SetSelectCallback(functor(*this, &CActorEntity::OnFileChange));
            ms_pTreePanel->SelectFile(filename);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CActorEntity::EndEditParams(IEditor* ie)
{
    if (ms_treePanelId != 0)
    {
        RemoveUIPage(ms_treePanelId);
        ms_treePanelId = 0;
    }

    CEntityObject::EndEditParams(ie);
}

//////////////////////////////////////////////////////////////////////////
void CActorEntity::OnFileChange(QString filename)
{
    CUndo undo("Entity Actor Modify");
    StoreUndo("Entity Actor Modify");
    SetPrototypeFile(filename);
}

//////////////////////////////////////////////////////////////////////////
//! Analyze errors for this object.
void CActorEntity::Validate(IErrorReport* report)
{
    CEntityObject::Validate(report);

    // Checks if object loaded.
}

//////////////////////////////////////////////////////////////////////////
bool CActorEntity::IsSimilarObject(CBaseObject* pObject)
{
    if (pObject->GetClassDesc() == GetClassDesc() && pObject->metaObject() == metaObject())
    {
        CActorEntity* pEntity = static_cast<CActorEntity*>(pObject);
        if (GetPrototypeFile() == pEntity->GetPrototypeFile())
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CActorEntity::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef node = CEntityObject::Export(levelPath, xmlNode);
    if (node)
    {
        node->setAttr("Prototype", static_cast<QString>(mv_prototype).toUtf8().data());
    }
    return node;
}

//////////////////////////////////////////////////////////////////////////
void CActorEntity::OnEvent(ObjectEvent event)
{
    CEntityObject::OnEvent(event);
    if (event == EVENT_INGAME || event == EVENT_OUTOFGAME)
    {
        if (m_pEntity)
        {
            // Make entity sleep on in/out game events.
            IPhysicalEntity* pe = m_pEntity->GetPhysics();
            if (pe)
            {
                pe_action_awake aa;
                aa.bAwake = 0;
                pe->Action(&aa);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CActorEntity::GetVerticesInWorld(std::vector<Vec3>& vertices) const
{
    vertices.clear();
    if (!m_pEntity)
    {
        return;
    }
    IStatObj* pStatObj = m_pEntity->GetStatObj(0);
    if (!pStatObj)
    {
        return;
    }
    IIndexedMesh* pIndexedMesh = pStatObj->GetIndexedMesh(true);
    if (!pIndexedMesh)
    {
        return;
    }

    IIndexedMesh::SMeshDescription meshDesc;
    pIndexedMesh->GetMeshDescription(meshDesc);

    vertices.reserve(meshDesc.m_nVertCount);

    const Matrix34 tm = GetWorldTM();

    if (meshDesc.m_pVerts)
    {
        for (int v = 0; v < meshDesc.m_nVertCount; ++v)
        {
            vertices.push_back(tm.TransformPoint(meshDesc.m_pVerts[v]));
        }
    }
    else
    {
        for (int v = 0; v < meshDesc.m_nVertCount; ++v)
        {
            vertices.push_back(tm.TransformPoint(meshDesc.m_pVertsF16[v].ToVec3()));
        }
    }
}

#include <Objects/ActorEntity.moc>
