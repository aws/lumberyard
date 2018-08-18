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
#include "ProtEntityObject.h"

#include "EntityPrototype.h"
#include "EntityPrototypeManager.h"
#include "Material/MaterialManager.h"

#include "IEntityPoolManager.h"

//////////////////////////////////////////////////////////////////////////
// CProtEntityObject implementation.
//////////////////////////////////////////////////////////////////////////
CProtEntityObject::CProtEntityObject()
{
    ZeroStruct(m_prototypeGUID);
    m_prototypeName = "Unknown Archetype";
}

//////////////////////////////////////////////////////////////////////////
bool CProtEntityObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool result = CEntityObject::Init(ie, prev, "");

    if (prev)
    {
        CProtEntityObject* pe = (CProtEntityObject*)prev;
        if (pe->m_prototype)
        {
            SetPrototype(pe->m_prototype, true, true);
        }
        else
        {
            m_prototypeGUID = pe->m_prototypeGUID;
            m_prototypeName = pe->m_prototypeName;
        }
    }
    else if (!file.isEmpty())
    {
        SetPrototype(GuidUtil::FromString(file.toUtf8().data()));
        SetUniqueName(m_prototypeName);
    }

    //////////////////////////////////////////////////////////////////////////
    // Disable variables that are in archetype.
    //////////////////////////////////////////////////////////////////////////
    mv_castShadow.SetFlags(mv_castShadow.GetFlags() | IVariable::UI_DISABLED);
    mv_castShadowMinSpec->SetFlags(mv_castShadowMinSpec->GetFlags() | IVariable::UI_DISABLED);

    mv_ratioLOD.SetFlags(mv_ratioLOD.GetFlags() | IVariable::UI_DISABLED);
    mv_viewDistanceMultiplier.SetFlags(mv_viewDistanceMultiplier.GetFlags() | IVariable::UI_DISABLED);
    mv_recvWind.SetFlags(mv_recvWind.GetFlags() | IVariable::UI_DISABLED);

    return result;
}

//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::Done()
{
    if (m_prototype)
    {
        m_prototype->RemoveUpdateListener(functor(*this, &CProtEntityObject::OnPrototypeUpdate));
    }
    m_prototype = 0;
    ZeroStruct(m_prototypeGUID);
    CEntityObject::Done();
}

//////////////////////////////////////////////////////////////////////////
bool CProtEntityObject::CreateGameObject()
{
    if (m_prototype)
    {
        return CEntityObject::CreateGameObject();
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::SpawnEntity()
{
    if (m_prototype)
    {
        CEntityObject::SpawnEntity();

        PostSpawnEntity();
    }
}
//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::PostSpawnEntity()
{
    // Here we adjust any additional property.
    if (m_prototype && m_pEntity)
    {
        if (!GetMaterial())
        {
            QString strPrototypeMaterial;
            CVarBlock* poVarBlock = m_prototype->GetProperties();
            if (poVarBlock)
            {
                IVariable* poPrototypeMaterial = poVarBlock->FindVariable("PrototypeMaterial");
                if (poPrototypeMaterial)
                {
                    poPrototypeMaterial->Get(strPrototypeMaterial);
                    if (!strPrototypeMaterial.isEmpty())
                    {
                        CMaterial* poMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(strPrototypeMaterial);
                        if (poMaterial)
                        {
                            // We must add a reference here, as when calling GetMatInfo()...
                            // ... it will decrease the refcount, and since LoadMaterial doesn't
                            // increase the refcount, it would get wrongly deallocated.
                            poMaterial->AddRef();
                            m_pEntity->SetMaterial(poMaterial->GetMatInfo());
                        }
                        else
                        {
                            Log("[EDITOR][CEntity::SpawnEntity()]Failed to load material %s", strPrototypeMaterial.toUtf8().data());
                        }
                    }
                }
            }
        }
    }
}
//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::SetPrototype(REFGUID guid, bool bForceReload)
{
    if (m_prototypeGUID == guid && bForceReload == false)
    {
        return;
    }

    m_prototypeGUID = guid;

    //m_fullPrototypeName = prototypeName;
    CEntityPrototypeManager* protMan = GetIEditor()->GetEntityProtManager();
    CEntityPrototype* prototype = protMan->FindPrototype(guid);
    if (!prototype)
    {
        m_prototypeName = "Unknown Archetype";

        CErrorRecord err;
        err.error = tr("Cannot find Entity Archetype: %1 for Entity %2").arg(GuidUtil::ToString(guid), GetName());
        err.pObject = this;
        err.severity = CErrorRecord::ESEVERITY_WARNING;
        GetIEditor()->GetErrorReport()->ReportError(err);
        //Warning( "Cannot find Entity Archetype: %s for Entity %s",GuidUtil::ToString(guid),(const char*)GetName() );
        return;
    }
    SetPrototype(prototype, bForceReload);

    IEntityPoolManager* pPoolManager = gEnv->pEntitySystem ? gEnv->pEntitySystem->GetIEntityPoolManager() : nullptr;
    if (pPoolManager && pPoolManager->IsClassDefaultBookmarked(prototype->GetEntityClassName().toUtf8().data()))
    {
        mv_createdThroughPool = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::BeginEditParams(IEditor* ie, int flags)
{
    CEntityObject::BeginEditParams(ie, flags);
}

//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::EndEditParams(IEditor* ie)
{
    CEntityObject::EndEditParams(ie);
}

//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::BeginEditMultiSelParams(bool bAllOfSameType)
{
    CEntityObject::BeginEditMultiSelParams(bAllOfSameType);
}

//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::EndEditMultiSelParams()
{
    CEntityObject::EndEditMultiSelParams();
}

//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::Serialize(CObjectArchive& ar)
{
    if (ar.bLoading)
    {
        // Serialize name at first.
        QString name;
        ar.node->getAttr("Name", name);
        SetName(name);

        // Loading.
        GUID guid;
        ar.node->getAttr("Prototype", guid);
        SetPrototype(guid);
        if (ar.bUndo)
        {
            SpawnEntity();
        }
    }
    else
    {
        // Saving.
        ar.node->setAttr("Prototype", m_prototypeGUID);
    }
    CEntityObject::Serialize(ar);

    if (ar.bLoading)
    {
        // If baseobject loading overrided some variables.
        SyncVariablesFromPrototype(true);
    }
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CProtEntityObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    return CEntityObject::Export(levelPath, xmlNode);
}

//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::OnPrototypeUpdate()
{
    // Callback from prototype.
    OnPropertyChange(0);

    if (m_prototype)
    {
        m_prototypeName = m_prototype->GetName();
        SyncVariablesFromPrototype(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::SetPrototype(CEntityPrototype* prototype, bool bForceReload, bool bOnlyDisabled)
{
    assert(prototype);

    if (prototype == m_prototype && !bForceReload)
    {
        return;
    }

    bool bRespawn = m_pEntity != 0;

    StoreUndo("Set Archetype");

    if (m_prototype)
    {
        m_prototype->RemoveUpdateListener(functor(*this, &CProtEntityObject::OnPrototypeUpdate));
    }

    m_prototype = prototype;
    m_prototype->AddUpdateListener(functor(*this, &CProtEntityObject::OnPrototypeUpdate));
    m_pProperties = 0;
    m_prototypeGUID = m_prototype->GetGUID();
    m_prototypeName = m_prototype->GetName();

    SyncVariablesFromPrototype(bOnlyDisabled);

    SetClass(m_prototype->GetEntityClassName(), bForceReload, true);
    if (bRespawn)
    {
        SpawnEntity();
    }
}

//////////////////////////////////////////////////////////////////////////
void CProtEntityObject::SyncVariablesFromPrototype(bool bOnlyDisabled)
{
    if (m_prototype)
    {
        CVarBlock* pVarsInPrototype = m_prototype->GetObjectVarBlock();
        mv_castShadow.CopyValue(pVarsInPrototype->FindVariable(mv_castShadow.GetName().toUtf8().data()));
        mv_castShadowMinSpec->CopyValue(pVarsInPrototype->FindVariable(mv_castShadowMinSpec->GetName().toUtf8().data()));
        mv_ratioLOD.CopyValue(pVarsInPrototype->FindVariable(mv_ratioLOD.GetName().toUtf8().data()));
        mv_viewDistanceMultiplier.CopyValue(pVarsInPrototype->FindVariable(mv_viewDistanceMultiplier.GetName().toUtf8().data()));
        mv_recvWind.CopyValue(pVarsInPrototype->FindVariable(mv_recvWind.GetName().toUtf8().data()));
        mv_obstructionMultiplier.CopyValue(pVarsInPrototype->FindVariable(mv_obstructionMultiplier.GetName().toUtf8().data()));
        if (!bOnlyDisabled)
        {
            mv_outdoor.CopyValue(pVarsInPrototype->FindVariable(mv_outdoor.GetName().toUtf8().data()));
            mv_hiddenInGame.CopyValue(pVarsInPrototype->FindVariable(mv_hiddenInGame.GetName().toUtf8().data()));
            mv_renderNearest.CopyValue(pVarsInPrototype->FindVariable(mv_renderNearest.GetName().toUtf8().data()));
            mv_noDecals.CopyValue(pVarsInPrototype->FindVariable(mv_noDecals.GetName().toUtf8().data()));
            mv_createdThroughPool.CopyValue(pVarsInPrototype->FindVariable(mv_createdThroughPool.GetName().toUtf8().data()));
        }
    }
}

#include <Objects/ProtEntityObject.moc>
