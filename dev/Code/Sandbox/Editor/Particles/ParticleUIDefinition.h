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
#ifndef CRYINCLUDE_EDITOR_CORE_PARTICLES_PARTICLEUIDEFINITION_H
#define CRYINCLUDE_EDITOR_CORE_PARTICLES_PARTICLEUIDEFINITION_H
#pragma once

#include "Include/EditorCoreAPI.h"

// CryCommon
#include <ParticleParams.h>

class CVarBlock;
struct IVariable;
struct SLodInfo;

class EDITOR_CORE_API CParticleUIDefinition
{
public:
    CParticleUIDefinition(){}
    ~CParticleUIDefinition(){}
    
    void AddForceUpdateVariable(IVariable* pVar);

    void ResetUIState();

    CVarBlockPtr CreateVars();

    void SetFromParticles(CParticleItem* pParticles, SLodInfo* pLevelOfDetail = nullptr);

    void SyncParticleSizeCurves(ParticleParams& newParams, const ParticleParams& originalParams);

    // Confetti Begin: Vladimir/Leroy
    // Added a way to skip validation (validator changes aspect ratio incorrectly when pasting an entire particle)
    bool SetToParticles(CParticleItem* pParticles, SLodInfo* pLevelOfDetail = nullptr);
    // Confetti End

    void ResetParticles(CParticleItem* pParticles, SLodInfo* pLevelOfDetail = nullptr);

public:
    ParticleParams m_localParams;
    ParticleParams m_defaultParams;
    CVarBlockPtr m_vars;
    IVariable::OnSetCallback m_onSetCallback = nullptr;
    IParticleEffect* m_localParticleEffect = nullptr; // pointer to the particle effect that's being selected in the UI
    std::vector<IVariable*> m_pForceUpdateVars;

    float m_aspectRatioYX = 1;
    float m_aspectRatioZX = 1;

    bool m_ignoreSetToParticle = false;

private:
    CVariableArray* AddGroup(const char* sName);

    void CalculateAspectRatios(const ParticleParams& params);
};

#endif // CRYINCLUDE_EDITOR_CORE_PARTICLES_PARTICLEUIDEFINITION_H
