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
#include "BloodSplats.h"
#include "IPostEffectGroup.h"

void CBloodSplats::Init(int type, float maxTime)
{
    m_type = type;
    m_maxTime = maxTime;
}

bool CBloodSplats::Update(float delta)
{
    m_currentTime -= delta;

    if (m_currentTime < 0.0f)
    {
        return true;
    }

    float scale = (2.0f * m_currentTime) / m_maxTime;
    if (scale > 1.0f)
    {
        scale = 1.0f;
    }

    gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("BloodSplats_Amount", scale * scale);

    return false;
}

bool CBloodSplats::OnActivate()
{
    gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("BloodSplats_Type", (float)m_type);
    gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("BloodSplats_Active", 1.0f);
    gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("BloodSplats_Amount", 1.0f);
    gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("BloodSplats_Spawn", 1.0f);
    m_currentTime = m_maxTime;
    return true;
}

bool CBloodSplats::OnDeactivate()
{
    gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("BloodSplats_Active", 0.0f);
    return true;
}

void CBloodSplats::GetMemoryUsage(ICrySizer* s) const
{
    s->Add(*this);
}
