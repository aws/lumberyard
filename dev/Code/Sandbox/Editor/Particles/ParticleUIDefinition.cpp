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


void CParticleUIDefinition::OnForceUpdateVariable(IVariable* pVar)
{
    if (m_CanForceUpdate)
    {
        m_ShouldForceUpdate = true;
    }
}

void CParticleUIDefinition::AddForceUpdateVariable(IVariable* pVar)
{
    m_pForceUpdateVars.push_back(pVar);
    pVar->AddOnSetCallback(functor(*this, &CParticleUIDefinition::OnForceUpdateVariable));

    for (int childVarIx = 0; childVarIx < pVar->GetNumVariables(); ++childVarIx)
    {
        AddForceUpdateVariable(pVar->GetVariable(childVarIx));
    }
}

void CParticleUIDefinition::ResetUIState()
{
    m_AspectRatioInitialized = false;
    m_CanForceUpdate = true;
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
        return;
    }
    IParticleEffect* pEffect = pParticles->GetEffect();
    if (!pEffect)
    {
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


    // Have to disallow forced updates here or else every variable will cause a forced update here
    m_CanForceUpdate = false;

    // Copy to local params, then run update on all vars.
    m_localParams = pEffect->GetParticleParams();
    m_defaultParams = pEffect->GetDefaultParams();

    //change to use default param based on emitter type if it's not inheritant from parent
    if (m_localParams.eInheritance != ParticleParams::EInheritance::Parent)
    {
        m_defaultParams = GetIEditor()->GetParticleManager()->GetEmitterDefaultParams(m_localParams.GetEmitterShape());
    }
    m_vars->SetRecreateSplines();
    m_vars->OnSetValues();

    // Reenable forced updates
    m_CanForceUpdate = true;
}

void CParticleUIDefinition::SyncParticleSizeCurves(ParticleParams& newParams, const ParticleParams& originalParams)
{
    // Initialize aspect ratio if it's never been initialized before (or is marked as needing to be initialized due to zero handling)
    if (!m_AspectRatioInitialized)
    {
        if (abs(originalParams.fSizeY.Base()) <= FLOAT_EPSILON)
        {
            // Use -1 when divide by zero would occur
            m_AspectRatio = -1.0f;
        }
        else
        {
            // Old aspect ratio calculation.
            m_AspectRatio = originalParams.fSizeX / originalParams.fSizeY;
        }
        m_AspectRatioInitialized = true;
    }

    if (newParams.bMaintainAspectRatio)
    {
        // check if fSizeY has been changed - if so push changes to fSizeX (that way you can change either X or Y and have everything propogate to the other)
        if (!(newParams.fSizeY.GetEmitterStrength() == originalParams.fSizeY.GetEmitterStrength()))
        {
            newParams.fSizeX.SetEmitterStrength(newParams.fSizeY.GetEmitterStrength());
        }
        else if (!(newParams.fSizeY.GetParticleAge() == originalParams.fSizeY.GetParticleAge()))
        {
            newParams.fSizeX.SetParticleAge(newParams.fSizeY.GetParticleAge());
        }

        // sync all Y values with X values and use new size Y calculated from maintaining aspect ratio
        if (!(newParams.fSizeY.GetEmitterStrength() == newParams.fSizeX.GetEmitterStrength()))
        {
            newParams.fSizeY.SetEmitterStrength(newParams.fSizeX.GetEmitterStrength());
        }
        if (!(newParams.fSizeY.GetParticleAge() == newParams.fSizeX.GetParticleAge()))
        {
            newParams.fSizeY.SetParticleAge(newParams.fSizeX.GetParticleAge());
        }
    }
    else if (newParams.fSizeX.Base() != originalParams.fSizeX.Base() || newParams.fSizeY.Base() != originalParams.fSizeY.Base())
    {
        // Basically the same as above when aspect ratio is initially calculated but only do this on size X or Y changes and use the new values to calculate the ratio.
        if (abs(newParams.fSizeY.Base()) <= FLOAT_EPSILON)
        {
            m_AspectRatio = -1.0f; // don't divide by zero, store negative one so we can check when we validate size changes when aspect ratio is maintained
        }
        else
        {
            m_AspectRatio = newParams.fSizeX / newParams.fSizeY;
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

    if (pLevelOfDetail)
    {
        IParticleEffect* lodEffect = pEffect->GetLodParticle(pLevelOfDetail);
        if (lodEffect)
        {
            pEffect = lodEffect;
        }
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

    bool shouldUpdate = bInheritanceChanged || m_ShouldForceUpdate;
    m_ShouldForceUpdate = false;

    return shouldUpdate;
}

void CParticleUIDefinition::ResetParticles(CParticleItem* pParticles, SLodInfo* pLevelOfDetail)
{
    IParticleEffect* pEffect = pParticles->GetEffect();
    if (!pEffect)
    {
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
