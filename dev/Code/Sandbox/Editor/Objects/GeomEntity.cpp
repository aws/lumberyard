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
#include "GeomEntity.h"
#include "PanelTreeBrowser.h"
#include "BrushObject.h"

//////////////////////////////////////////////////////////////////////////
bool CGeomEntity::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool bRes = false;
    if (file.isEmpty())
    {
        bRes = CEntityObject::Init(ie, prev, "");
        SetClass("GeomEntity");
    }
    else
    {
        bRes = CEntityObject::Init(ie, prev, "GeomEntity");
        SetClass("GeomEntity");
        SetGeometryFile(file);
        QString name = Path::GetFileName(file);
        if (!name.isEmpty())
        {
            SetUniqueName(name);
        }
    }

    return bRes;
}

//////////////////////////////////////////////////////////////////////////
void CGeomEntity::InitVariables()
{
    AddVariable(mv_geometry, "Geometry", functor(*this, &CGeomEntity::OnGeometryFileChange), IVariable::DT_OBJECT);
    CEntityObject::InitVariables();
}

//////////////////////////////////////////////////////////////////////////
void CGeomEntity::SetGeometryFile(const QString& filename)
{
    if (filename != mv_geometry)
    {
        mv_geometry = filename;
    }

    OnGeometryFileChange(&mv_geometry);
}

//////////////////////////////////////////////////////////////////////////
void CGeomEntity::OnGeometryFileChange(IVariable* pVar)
{
    if (m_pEntity)
    {
        if (!m_bVisible)
        {
            m_pEntity->Hide(false);    // Unhide to clear hidden state
        }
        QByteArray filename = static_cast<QString>(mv_geometry).toUtf8();
        const char* ext = PathUtil::GetExt(filename);
        if (_stricmp(ext, CRY_SKEL_FILE_EXT) == 0 || _stricmp(ext, CRY_CHARACTER_DEFINITION_FILE_EXT) == 0 || _stricmp(ext, CRY_ANIM_GEOMETRY_FILE_EXT) == 0)
        {
            m_pEntity->LoadCharacter(0, filename, IEntity::EF_AUTO_PHYSICALIZE);   // Physicalize geometry.
        }
        else
        {
            m_pEntity->LoadGeometry(0, filename, NULL, IEntity::EF_AUTO_PHYSICALIZE);   // Physicalize geometry.
        }
        if (!m_bVisible)
        {
            m_pEntity->Hide(true);    // Hide if hidden
        }
        CalcBBox();
        InvalidateTM(0);
        OnRenderFlagsChange(0);
        m_statObjValidator.Validate(m_pEntity->GetStatObj(ENTITY_SLOT_ACTUAL), GetRenderMaterial(), m_pEntity->GetPhysics());
        if (ms_pTreePanel)
        {
            ms_pTreePanel->SelectFile(QtUtil::ToQString(filename));
        }
        InstallStatObjListeners();
    }
}

//////////////////////////////////////////////////////////////////////////
void CGeomEntity::SpawnEntity()
{
    CEntityObject::SpawnEntity();
    SetGeometryFile(mv_geometry);
}

//////////////////////////////////////////////////////////////////////////
QString CGeomEntity::GetGeometryFile() const
{
    return mv_geometry;
}

//////////////////////////////////////////////////////////////////////////
bool CGeomEntity::ConvertFromObject(CBaseObject* object)
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

        SpawnEntity();
        SetGeometryFile(prefab->GetFilePath());
        return true;
    }
    else if (qobject_cast<CEntityObject*>(object))
    {
        CEntityObject* pObject = ( CEntityObject* )object;
        if (!pObject->GetIEntity())
        {
            return false;
        }

        IStatObj* pStatObj = pObject->GetIEntity()->GetStatObj(0);
        if (!pStatObj)
        {
            return false;
        }

        SpawnEntity();
        SetGeometryFile(pStatObj->GetFilePath());
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CGeomEntity::BeginEditParams(IEditor* ie, int flags)
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
                ms_pTreePanel = new CPanelTreeBrowser(functor(*this, &CGeomEntity::OnFileChange), GetClassDesc()->GetFileSpec(), flags);
            }
            if (ms_treePanelId == 0)
            {
                ms_treePanelId = AddUIPage(tr("Geometry").toUtf8().data(), ms_pTreePanel);
            }
        }

        if (ms_pTreePanel)
        {
            ms_pTreePanel->SetSelectCallback(functor(*this, &CGeomEntity::OnFileChange));
            ms_pTreePanel->SelectFile(filename);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CGeomEntity::OnFileChange(QString filename)
{
    CUndo undo("Entity Geom Modify");
    StoreUndo("Entity Geom Modify");
    SetGeometryFile(filename);
}

//////////////////////////////////////////////////////////////////////////
//! Analyze errors for this object.
void CGeomEntity::Validate(IErrorReport* report)
{
    CEntityObject::Validate(report);

    // Checks if object loaded.
}

//////////////////////////////////////////////////////////////////////////
bool CGeomEntity::IsSimilarObject(CBaseObject* pObject)
{
    if (pObject->GetClassDesc() == GetClassDesc() && pObject->metaObject() == metaObject())
    {
        CGeomEntity* pEntity = ( CGeomEntity* )pObject;
        if (GetGeometryFile() == pEntity->GetGeometryFile())
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CGeomEntity::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef node = CEntityObject::Export(levelPath, xmlNode);
    if (node)
    {
        node->setAttr("Geometry", static_cast<QString>(mv_geometry).toUtf8().data());
    }
    return node;
}

//////////////////////////////////////////////////////////////////////////
void CGeomEntity::OnEvent(ObjectEvent event)
{
    CEntityObject::OnEvent(event);
    switch (event)
    {
    case EVENT_INGAME:
    case EVENT_OUTOFGAME:
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
void CGeomEntity::GetVerticesInWorld(std::vector<Vec3>& vertices) const
{
    vertices.clear();
    if (m_pEntity == NULL)
    {
        return;
    }
    IStatObj* pStatObj = m_pEntity->GetStatObj(0);
    if (pStatObj == NULL)
    {
        return;
    }
    IIndexedMesh* pIndexedMesh = pStatObj->GetIndexedMesh(true);
    if (pIndexedMesh == NULL)
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

#include <Objects/GeomEntity.moc>