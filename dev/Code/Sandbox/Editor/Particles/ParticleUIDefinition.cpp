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

#include "ParticleUIDefinition.h"
#include "ParticleItem.h"

#include <Include/IEditorParticleManager.h>
#include <Util/VariableTypeInfo.h>


void CParticleUIDefinition::AddForceUpdateVariable(IVariable* pVar)
{
    m_pForceUpdateVars.push_back(pVar);
}

void CParticleUIDefinition::ResetUIState()
{
}

CVarBlockPtr CParticleUIDefinition::CreateVars()
{
    m_vars = new CVarBlock;

    int nNumGroups = 0;

    // Create UI vars, using ParticleParams TypeInfo.
    CVariableArray* pVarTable = 0;

    const CTypeInfo& partInfo = TypeInfo(&m_localParams);

    for AllSubVars(pVarInfo, partInfo)
    {
        if (*pVarInfo->GetName() == '_')
        {
            continue;
        }

        string sGroup;
        if (pVarInfo->GetAttr("Group", sGroup))
        {
            pVarTable = AddGroup(sGroup);
            if (nNumGroups++ > 5)
            {
                pVarTable->SetFlags(pVarTable->GetFlags() | IVariable::UI_ROLLUP2);
            }
            if (pVarInfo->Type.IsType<void>())
            {
                continue;
            }
        }

        IVariable* pVar = CVariableTypeInfo::Create(*pVarInfo, &m_localParams, &m_defaultParams);

        // Variables with this attribute will trigger a dialog redraw when they are updated.
        if (pVarInfo->GetAttr("ForceUpdate"))
        {
            AddForceUpdateVariable(pVar);
        }

        // Add to group.
        pVar->AddRef();                             // Variables are local and must not be released by CVarBlock.
        if (pVarTable)
        {
            pVarTable->AddVariable(pVar);
        }
    }

    return m_vars;
}

void CParticleUIDefinition::SetFromParticles(CParticleItem* pParticles, SLodInfo* pLevelOfDetail)
{
    if (!pParticles)
    {
        m_localParticleEffect = nullptr;
        return;
    }
    IParticleEffect* pEffect = pParticles->GetEffect();
    if (!pEffect)
    {
        m_localParticleEffect = nullptr;
        return;
    }

    if (pLevelOfDetail)
    {
        IParticleEffect* lodEffect = pEffect->GetLodParticle(pLevelOfDetail);
        if (lodEffect)
        {
            pEffect = lodEffect;
        }
    }
    
    // Copy to local params, then run update on all vars.
    m_localParams = pEffect->GetParticleParams();
    m_defaultParams = pEffect->GetDefaultParams();

    m_localParticleEffect = pEffect;

    //change to use default param based on emitter type if it's not inheritant from parent
    if (m_localParams.eInheritance != ParticleParams::EInheritance::Parent)
    {
        m_defaultParams = GetIEditor()->GetParticleManager()->GetEmitterDefaultParams(m_localParams.GetEmitterShape());
    }
    m_vars->SetRecreateSplines();
    m_vars->OnSetValues();
}

void CParticleUIDefinition::CalculateAspectRatios(const ParticleParams& params)
{
    //calculate aspect ratios
    m_aspectRatioYX = 1;
    m_aspectRatioZX = 1;
    if (abs(params.fSizeX.Base()) > FLOAT_EPSILON)
    {
        m_aspectRatioYX = params.fSizeY / params.fSizeX;
        m_aspectRatioZX = params.fSizeZ / params.fSizeX;
    }
}

void CParticleUIDefinition::SyncParticleSizeCurves(ParticleParams& newParams, const ParticleParams& originalParams)
{
    if (newParams.bMaintainAspectRatio && !originalParams.bMaintainAspectRatio)
    {       
        CalculateAspectRatios(originalParams);
        
        //Sync Y and Z's random range and curves to be same as X's but keep the base of Y and Z
        newParams.fSizeY.Set(newParams.fSizeY.Base(), newParams.fSizeX.GetRandomRange());
        newParams.fSizeZ.Set(newParams.fSizeZ.Base(), newParams.fSizeX.GetRandomRange());
        newParams.fSizeY.SetEmitterStrength(newParams.fSizeX.GetEmitterStrength());
        newParams.fSizeZ.SetEmitterStrength(newParams.fSizeX.GetEmitterStrength());
        newParams.fSizeY.SetParticleAge(newParams.fSizeX.GetParticleAge());
        newParams.fSizeZ.SetParticleAge(newParams.fSizeX.GetParticleAge());
    }
    else if (newParams.bMaintainAspectRatio)
    {
        //when bMaintainAspectRatio is enabled, we only allow size X can be changed
        //sync the size if the size X changed
        bool modified = false;
        if (newParams.fSizeX.Base() != originalParams.fSizeX.Base() || 
            newParams.fSizeX.GetRandomRange() != originalParams.fSizeX.GetRandomRange())
        {
            newParams.fSizeY.Set(newParams.fSizeX.Base()* m_aspectRatioYX, newParams.fSizeX.GetRandomRange());
            newParams.fSizeZ.Set(newParams.fSizeX.Base()* m_aspectRatioZX, newParams.fSizeX.GetRandomRange());
            modified = true;
        }

        if (!(newParams.fSizeX.GetEmitterStrength() == originalParams.fSizeX.GetEmitterStrength()))
        {
            newParams.fSizeY.SetEmitterStrength(newParams.fSizeX.GetEmitterStrength());
            newParams.fSizeZ.SetEmitterStrength(newParams.fSizeX.GetEmitterStrength());
            modified = true;
        }

        if (!(newParams.fSizeX.GetParticleAge() == originalParams.fSizeX.GetParticleAge()))
        {
            newParams.fSizeY.SetParticleAge(newParams.fSizeX.GetParticleAge());
            newParams.fSizeZ.SetParticleAge(newParams.fSizeX.GetParticleAge());
            modified = true;
        }

        //recreate spline and refresh widgets of the synced variables
        if (modified)
        {
            for (int index = 0; index < m_pForceUpdateVars.size(); index++)
            {
                IVariable* var = m_pForceUpdateVars[index];
                var->SetFlagRecursive(IVariable::UI_CREATE_SPLINE);
                var->OnSetValue(true);
            }
        }
    }
}

bool CParticleUIDefinition::SetToParticles(CParticleItem* pParticles, SLodInfo* pLevelOfDetail)
{
    IParticleEffect* pEffect = pParticles->GetEffect();
    if (!pEffect)
    {
        return false;
    }

    //avoid calling itself
    if (m_ignoreSetToParticle)
    {
        return false;
    }

    m_ignoreSetToParticle = true;

    if (pLevelOfDetail)
    {
        IParticleEffect* lodEffect = pEffect->GetLodParticle(pLevelOfDetail);
        if (lodEffect)
        {
            pEffect = lodEffect;
        }
    }

    if (pEffect != m_localParticleEffect)
    {
        AZ_Warning("Particle Editor", false, "Assigning values to an invalid particle emitter!");
        return false;
    }

    // Detect whether inheritance changed, update defaults.
    bool bInheritanceChanged = m_localParams.eInheritance != pEffect->GetParticleParams().eInheritance;

    SyncParticleSizeCurves(m_localParams, pEffect->GetParticleParams());

    pEffect->SetParticleParams(m_localParams);

    if (bInheritanceChanged)
    {
        m_defaultParams = pEffect->GetDefaultParams();
        //change to use default param based on emitter type if it's not inheritant from parent
        if (m_localParams.eInheritance != ParticleParams::EInheritance::Parent)
        {
            m_defaultParams = GetIEditor()->GetParticleManager()->GetEmitterDefaultParams(m_localParams.GetEmitterShape());
        }
    }

    // Update particles.
    pParticles->Update();
    
    m_ignoreSetToParticle = false;

    return false;
}

void CParticleUIDefinition::ResetParticles(CParticleItem* pParticles, SLodInfo* pLevelOfDetail)
{
    IParticleEffect* pEffect = pParticles->GetEffect();
    if (!pEffect)
    {
        m_localParticleEffect = nullptr;
        return;
    }

    if (pLevelOfDetail)
    {
        IParticleEffect* lodEffect = pEffect->GetLodParticle(pLevelOfDetail);
        if (lodEffect)
        {
            pEffect = lodEffect;
        }
    }

    // Set params to defaults, using current inheritance value.
    auto eSave = m_localParams.eInheritance;
    m_localParams = pEffect->GetParticleParams();
    m_localParams.eInheritance = eSave;

    pEffect->SetParticleParams(m_localParams);
    m_localParticleEffect = pEffect;
    // Update particles.
    pParticles->Update();
}

CVariableArray* CParticleUIDefinition::AddGroup(const char* sName)
{
    CVariableArray* pArray = new CVariableArray();

    pArray->AddRef();
    pArray->SetFlags(IVariable::UI_BOLD);
    if (sName)
    {
        pArray->SetName(sName);
    }
    m_vars->AddVariable(pArray);

    return pArray;
}
