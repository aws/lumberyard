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
#include "Snow_precompiled.h"
#include "Snow.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CSnow, CSnow)

CSnow::CSnow()
{
}

CSnow::~CSnow()
{
}

//------------------------------------------------------------------------
bool CSnow::Init(IGameObject* pGameObject)
{
    SetGameObject(pGameObject);

    return Reset();
}

//------------------------------------------------------------------------
void CSnow::PostInit(IGameObject* pGameObject)
{
    GetGameObject()->EnableUpdateSlot(this, 0);
}

//------------------------------------------------------------------------
bool CSnow::ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params)
{
    ResetGameObject();

    CRY_ASSERT_MESSAGE(false, "CSnow::ReloadExtension not implemented");

    return false;
}

//------------------------------------------------------------------------
bool CSnow::GetEntityPoolSignature(TSerialize signature)
{
    CRY_ASSERT_MESSAGE(false, "CSnow::GetEntityPoolSignature not implemented");

    return true;
}

//------------------------------------------------------------------------
void CSnow::FullSerialize(TSerialize ser)
{
    ser.Value("bEnabled", m_bEnabled);
    ser.Value("fRadius", m_fRadius);
    ser.Value("fSnowAmount", m_fSnowAmount);
    ser.Value("fFrostAmount", m_fFrostAmount);
    ser.Value("fSurfaceFreezing", m_fSurfaceFreezing);
    ser.Value("nSnowFlakeCount", m_nSnowFlakeCount);
    ser.Value("fSnowFlakeSize", m_fSnowFlakeSize);
    ser.Value("fBrightness", m_fSnowFallBrightness);
    ser.Value("fGravityScale", m_fSnowFallGravityScale);
    ser.Value("fWindScale", m_fSnowFallWindScale);
    ser.Value("fTurbulenceStrength", m_fSnowFallTurbulence);
    ser.Value("fTurbulenceFreq", m_fSnowFallTurbulenceFreq);
}

//------------------------------------------------------------------------
void CSnow::Update(SEntityUpdateContext& ctx, int updateSlot)
{
    const IActor* pClient = GetISystem()->GetIGame() && GetISystem()->GetIGame()->GetIGameFramework() ? GetISystem()->GetIGame()->GetIGameFramework()->GetClientActor() : nullptr;
    if (pClient && Reset())
    {
        if (!m_bEnabled || gEnv->IsEditor() && !gEnv->IsEditorSimulationMode() && !gEnv->IsEditorGameMode())
        {
            gEnv->p3DEngine->SetSnowSurfaceParams(Vec3(), 0.0f, 0.0f, 0.0f, 0.0f);
            gEnv->p3DEngine->SetSnowFallParams(0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        else
        {
            // todo: only update when things have changed.
            gEnv->p3DEngine->SetSnowSurfaceParams(GetEntity()->GetWorldPos(), m_fRadius, m_fSnowAmount, m_fFrostAmount, m_fSurfaceFreezing);
            gEnv->p3DEngine->SetSnowFallParams(m_nSnowFlakeCount, m_fSnowFlakeSize, m_fSnowFallBrightness, m_fSnowFallGravityScale, m_fSnowFallWindScale, m_fSnowFallTurbulence, m_fSnowFallTurbulenceFreq);
        }
    }
}

//------------------------------------------------------------------------
void CSnow::HandleEvent(const SGameObjectEvent& event)
{
}

//------------------------------------------------------------------------
void CSnow::ProcessEvent(SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_RESET:
        Reset();
        break;
    case ENTITY_EVENT_HIDE:
    case ENTITY_EVENT_DONE:
        if (gEnv && gEnv->p3DEngine)
        {
            static const Vec3 vZero(ZERO);
            gEnv->p3DEngine->SetSnowSurfaceParams(vZero, 0, 0, 0, 0);
            gEnv->p3DEngine->SetSnowFallParams(0, 0, 0, 0, 0, 0, 0);
        }
        break;
    }
}

//------------------------------------------------------------------------
void CSnow::SetAuthority(bool auth)
{
}

void CSnow::GetSnowSurfaceParams(float& fRadius, float& fSnowAmount, float& fFrostAmount, float& fSurfaceFreezing)
{
    fRadius = m_fRadius;
    fSnowAmount = m_fSnowAmount;
    fFrostAmount = m_fFrostAmount;
    fSurfaceFreezing = m_fSurfaceFreezing;
}
void CSnow::GetSnowFallParams(int& nSnowFlakeCount, float& fSnowFlakeSize, float& fSnowFallBrightness, float& fSnowFallGravityScale, float& fSnowFallWindScale, float& fSnowFallTurbulence, float& fSnowFallTurbulenceFreq)
{
    nSnowFlakeCount = m_nSnowFlakeCount;
    fSnowFlakeSize = m_fSnowFlakeSize;
    fSnowFallBrightness = m_fSnowFallBrightness;
    fSnowFallGravityScale = m_fSnowFallGravityScale;
    fSnowFallWindScale = m_fSnowFallWindScale;
    fSnowFallTurbulence = m_fSnowFallTurbulence;
    fSnowFallTurbulenceFreq = m_fSnowFallTurbulenceFreq;
}

//------------------------------------------------------------------------
bool CSnow::Reset()
{
    //Initialize default values before (in case ScriptTable fails)
    m_bEnabled = false;
    m_fRadius = 50.f;
    m_fSnowAmount = 10.f;
    m_fFrostAmount = 1.f;
    m_fSurfaceFreezing = 1.f;
    m_nSnowFlakeCount = 100;
    m_fSnowFlakeSize = 1.f;
    m_fSnowFallBrightness = 1.f;
    m_fSnowFallGravityScale = 0.1f;
    m_fSnowFallWindScale = 0.1f;
    m_fSnowFallTurbulence = 0.5f;
    m_fSnowFallTurbulenceFreq = 0.1f;
    
    IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
    if (!pScriptTable)
    {
        return false;
    }

    SmartScriptTable props;
    if (!pScriptTable->GetValue("Properties", props))
    {
        return false;
    }

    SmartScriptTable surface;
    if (!props->GetValue("Surface", surface))
    {
        return false;
    }

    SmartScriptTable snowFall;
    if (!props->GetValue("SnowFall", snowFall))
    {
        return false;
    }

    // Get entity properties.
    props->GetValue("bEnabled", m_bEnabled);
    props->GetValue("fRadius", m_fRadius);

    // Get surface properties.
    surface->GetValue("fSnowAmount", m_fSnowAmount);
    surface->GetValue("fFrostAmount", m_fFrostAmount);
    surface->GetValue("fSurfaceFreezing", m_fSurfaceFreezing);

    // Get snowfall properties.
    snowFall->GetValue("nSnowFlakeCount", m_nSnowFlakeCount);
    snowFall->GetValue("fSnowFlakeSize", m_fSnowFlakeSize);
    snowFall->GetValue("fBrightness", m_fSnowFallBrightness);
    snowFall->GetValue("fGravityScale", m_fSnowFallGravityScale);
    snowFall->GetValue("fWindScale", m_fSnowFallWindScale);
    snowFall->GetValue("fTurbulenceStrength", m_fSnowFallTurbulence);
    snowFall->GetValue("fTurbulenceFreq", m_fSnowFallTurbulenceFreq);

    return true;
}
