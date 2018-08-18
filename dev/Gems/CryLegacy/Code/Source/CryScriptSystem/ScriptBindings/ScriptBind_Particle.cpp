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

#include "CryLegacy_precompiled.h"
#include "ScriptBind_Particle.h"
#include <ISystem.h>
#include <I3DEngine.h>
#include <IEntitySystem.h>
#include <CryParticleSpawnInfo.h>
#include <ICryAnimation.h>
#include "ParticleParams.h"
#include "Components/IComponentRender.h"

// TypeInfo implementations
#ifndef AZ_MONOLITHIC_BUILD
    #include "ParticleParams_TypeInfo.h"
#else
    #include <CryTypeInfo.h>
#endif

//------------------------------------------------------------------------
CScriptBind_Particle::CScriptBind_Particle(IScriptSystem* pScriptSystem, ISystem* pSystem)
{
    CScriptableBase::Init(pScriptSystem, pSystem);
    SetGlobalName("Particle");

    m_pSystem = pSystem;
    m_p3DEngine = gEnv->p3DEngine;

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_Particle::

    SCRIPT_REG_TEMPLFUNC(CreateEffect, "name, params");
    SCRIPT_REG_TEMPLFUNC(DeleteEffect, "name");
    SCRIPT_REG_TEMPLFUNC(IsEffectAvailable, "name");
    SCRIPT_REG_TEMPLFUNC(SpawnEffect, "effectName, pos, dir, [scale], [entityId], [partId]");
    SCRIPT_REG_TEMPLFUNC(SpawnEffectLine, "effectName, startPos, endPos, dir, scale, slices");
    SCRIPT_REG_TEMPLFUNC(SpawnParticles, "params, pos, dir");
    SCRIPT_REG_TEMPLFUNC(CreateDecal, "pos, normal, size, lifeTime, textureName, [angle], [hitDirection], [entityId], [partid]");
    SCRIPT_REG_TEMPLFUNC(CreateMatDecal, "pos, normal, size, lifeTime, materialName, [angle], [hitDirection], [entityId], [partid]");
    SCRIPT_REG_FUNC(Attach);
    SCRIPT_REG_FUNC(Detach);

    RegisterGlobal("CRYPARTICLE_ONE_TIME_SPAWN", CryParticleSpawnInfo::FLAGS_ONE_TIME_SPAWN);
    RegisterGlobal("CRYPARTICLE_RAIN_MODE", CryParticleSpawnInfo::FLAGS_RAIN_MODE);
}

CScriptBind_Particle::~CScriptBind_Particle()
{
}

//------------------------------------------------------------------------
int CScriptBind_Particle::CreateEffect(IFunctionHandler* pH, const char* name, SmartScriptTable params)
{
    IParticleEffect* pEffect = gEnv->pParticleManager->CreateEffect();
    pEffect->SetName(name);

    ParticleParams effectParams;

    ReadParams(params, &effectParams, pEffect);

    // ?? Where are params set in effect?

    return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Particle::DeleteEffect(IFunctionHandler* pH, const char* name)
{
    IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(name, "Particle.DeleteEffect");

    if (pEffect)
    {
        gEnv->pParticleManager->DeleteEffect(pEffect);
    }

    return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Particle::IsEffectAvailable(IFunctionHandler* pH, const char* name)
{
    IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(name, "Particle.IsEffectAvailable");

    return pH->EndFunction(pEffect != 0);
}

//------------------------------------------------------------------------
int CScriptBind_Particle::SpawnEffect(IFunctionHandler* pH, const char* effectName, Vec3 pos, Vec3 dir)
{
    IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(effectName, "Particle.SpawnEffect");

    if (pEffect)
    {
        float scale = 1.0f;
        if ((pH->GetParamCount() > 3) && (pH->GetParamType(4) == svtNumber))
        {
            pH->GetParam(4, scale);
        }

        ScriptHandle entityId(0);
        if ((pH->GetParamCount() > 4) && (pH->GetParamType(5) == svtPointer))
        {
            pH->GetParam(5, entityId);
        }

        int partId = 0;
        if ((pH->GetParamCount() > 5) && (pH->GetParamType(6) == svtNumber))
        {
            pH->GetParam(6, partId);
        }

        IEntity* pEntity = NULL;
        if (entityId.n)
        {
            pEntity = gEnv->pEntitySystem->GetEntity((EntityId)entityId.n);
        }
        if (pEntity && pEffect->GetParticleParams().eAttachType != GeomType_None)
        {
            pEntity->LoadParticleEmitter(-1, pEffect);
        }
        else
        {
            pEffect->Spawn(true, IParticleEffect::ParticleLoc(pos, dir, scale));
        }
    }

    return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Particle::SpawnEffectLine(IFunctionHandler* pH, const char* effectName, Vec3 startPos, Vec3 endPos, Vec3 dir, float scale, int slices)
{
    IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(effectName, "Particle.SpawnEffectLine");

    if (!pEffect)
    {
        return pH->EndFunction();
    }

    Vec3 spawnPoint(startPos);
    Vec3 dp((endPos - startPos) / (float)max(1, slices));

    slices = slices + 1;

    for (int i = 0; i <= slices; i++)
    {
        pEffect->Spawn(true, IParticleEffect::ParticleLoc(spawnPoint, dir, scale));

        spawnPoint += dp;
    }

    return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Particle::SpawnParticles(IFunctionHandler* pH, SmartScriptTable params, Vec3 pos, Vec3 dir)
{
    ParticleParams particleParams;

    ReadParams(params, &particleParams, 0);

    gEnv->pParticleManager->CreateEmitter(IParticleEffect::ParticleLoc(pos, dir), particleParams, ePEF_Independent);

    return pH->EndFunction();
}

//------------------------------------------------------------------------

void CScriptBind_Particle::CreateDecalInternal(IFunctionHandler* pH, const Vec3& pos, const Vec3& normal, float size, float lifeTime, const char* name, bool nameIsMaterial)
{
    CryEngineDecalInfo decal;

    decal.vPos = pos;
    decal.vNormal = normal;
    decal.fSize = size;
    decal.fLifeTime = lifeTime;

    if (nameIsMaterial)
    {
        cry_strcpy(decal.szMaterialName, name);
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "CScriptBind_Particle::CreateDecalInternal: Decal material name is not specified");
    }

    if ((pH->GetParamCount() > 5) && (pH->GetParamType(6) != svtNull))
    {
        pH->GetParam(6, decal.fAngle);
    }

    if ((pH->GetParamCount() > 6) && (pH->GetParamType(7) != svtNull))
    {
        pH->GetParam(7, decal.vHitDirection);
    }
    else
    {
        decal.vHitDirection = -normal;
    }

    if ((pH->GetParamCount() > 7) && (pH->GetParamType(8) != svtNull))
    {
        ScriptHandle entityId;

        pH->GetParam(8, entityId);

        IEntity* pEntity = gEnv->pEntitySystem->GetEntity((EntityId)entityId.n);

        if (pEntity)
        {
            IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();

            if (pRenderComponent)
            {
                decal.ownerInfo.pRenderNode = pRenderComponent->GetRenderNode();
            }
        }
    }
    else if ((pH->GetParamCount() > 8) && (pH->GetParamType(9) != svtNull))
    {
        ScriptHandle renderNode;
        pH->GetParam(9, renderNode);
        decal.ownerInfo.pRenderNode = (IRenderNode*)renderNode.ptr;
    }

    if ((pH->GetParamCount() > 9) && (pH->GetParamType(10) != svtNull))
    {
        float growTime;

        pH->GetParam(10, growTime);
        decal.fGrowTime = growTime;
    }

    if ((pH->GetParamCount() > 10) && (pH->GetParamType(11) != svtNull))
    {
        float growTimeAlpha = 0.0f;

        pH->GetParam(11, growTimeAlpha);
        decal.fGrowTimeAlpha = growTimeAlpha;
    }

    // [*DavidR | 27/Sep/2010] ToDo: 12 parameters? this bind needs to work with a table as parameter instead of this
    if ((pH->GetParamCount() > 11) && (pH->GetParamType(12) != svtNull))
    {
        bool skip = false;

        pH->GetParam(12, skip);
        decal.bSkipOverlappingTest = skip;
    }

    m_p3DEngine->CreateDecal(decal);
}

int CScriptBind_Particle::CreateDecal(IFunctionHandler* pH, Vec3 pos, Vec3 normal, float size, float lifeTime, const char* textureName)
{
    CreateDecalInternal(pH, pos, normal, size, lifeTime, textureName, false);
    return pH->EndFunction();
}

int CScriptBind_Particle::CreateMatDecal(IFunctionHandler* pH, Vec3 pos, Vec3 normal, float size, float lifeTime, const char* materialName)
{
    CreateDecalInternal(pH, pos, normal, size, lifeTime, materialName, true);
    return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Particle::Attach(IFunctionHandler* pH)
{
    /*
    SmartScriptTable  pObj;
    SmartScriptTable  pChildObj;
    Vec3 v3Pos,v3SysDir,v3Offset(0,0,0);
    int nID,nFlags=0;
    float fSpawnRate = 0;
    const char *szBoneName;
    ParticleParams sParam;
    CryParticleSpawnInfo cpsi;

    if(!pH->GetParam(1,nID))
        m_pSS->RaiseError( "<Particle::Attach> parameter 1 not specified or nil (entity id to attach to)" );
    if(!pH->GetParam(2,pObj))
        m_pSS->RaiseError( "<Particle::Attach> parameter 2 not specified or nil(particle stuct)" );
    if(!pH->GetParam(3,fSpawnRate))
        m_pSS->RaiseError( "<Particle::Attach> spawn rate not specified" );


    IEntity *pEntity = gEnv->pEntitySystem->GetEntity(nID);
    if (!pEntity)
    {
        m_pSS->RaiseError( "<Particle::Attach> specified entity ID does not exist)" );
        return pH->EndFunction();
    }

    cpsi.fSpawnRate = fSpawnRate;

    ICharacterInstance *pInstance = pEntity->GetCharacter(0);

    if (pH->GetParam(4,szBoneName))
    {
        if (pInstance)
        {
            cpsi.nBone = pInstance->GetModel()->GetBoneByName(szBoneName);
        }

        cpsi.nFlags |= CryParticleSpawnInfo::FLAGS_SPAWN_FROM_BONE;

        cpsi.vBonePos.Set(0,0,0);
        pH->GetParam(5,cpsi.vBonePos);

        if (pH->GetParam(6,nFlags))
            cpsi.nFlags = nFlags;
    }

    ReadParticleTable(*pObj, sParam);

    return pH->EndFunction(pInstance->AddParticleEmitter(sParam,cpsi));
    */
    return pH->EndFunction();
}

int CScriptBind_Particle::Detach(IFunctionHandler* pH)
{
    /*
    int nID,nHandle;
    if (!pH->GetParams(nID,nHandle))
        return pH->EndFunction();

    IEntity *pEntity = gEnv->pEntitySystem->GetEntity(nID);
    if (!pEntity)
    {
        m_pSS->RaiseError( "<Particle::Detach> specified entity ID does not exist)" );
        return pH->EndFunction();
    }

    ICharacterInstance *pInstance = pEntity->GetCharacter(0);

    pInstance->RemoveParticleEmitter(nHandle);
*/
    return pH->EndFunction();
}

template<class T>
bool TryReadType(const CTypeInfo::CVarInfo& var, T* data, SmartScriptTable& table, cstr key)
{
    T val;
    return table.GetPtr()->GetValue(key, val)
           && var.Type.FromValue(data, &val, TypeInfo(&val));
}

void CScriptBind_Particle::ReadParams(SmartScriptTable& table, ParticleParams* pParams, IParticleEffect* pEffect)
{
    // Iterate params using TypeInfo, read from script table.
    const CTypeInfo& paramsType = TypeInfo(pParams);

    IScriptTable::Iterator it = table.GetPtr()->BeginIteration();
    while (table.GetPtr()->MoveNext(it))
    {
        const CTypeInfo::CVarInfo* pVar = paramsType.FindSubVar(it.sKey);
        if (!pVar)
        {
            m_pSS->RaiseError("<Particle::ReadParams> Invalid parameter %s", it.sKey);
            continue;
        }

        // Strange LUA stuff here.
        void* data = pVar->GetAddress(pParams);

        if (&pVar->Type == &TypeInfo((string*)data))
        {
            cstr val;
            if (table.GetPtr()->GetValue(it.sKey, val))
            {
                *(string*)data = val;
                continue;
            }
        }
        else if (TryReadType(*pVar, (bool*)data, table, it.sKey) ||
                 TryReadType(*pVar, (int*)data, table, it.sKey) ||
                 TryReadType(*pVar, (float*)data, table, it.sKey))
        {
            continue;
        }

        m_pSS->RaiseError("<Particle::ReadParams> Cannot read parameter %s of type %s", it.sKey, pVar->Type.Name);
    }
    table.GetPtr()->EndIteration(it);

    if (pEffect)
    {
        pEffect->SetParticleParams(*pParams);
    }
}
