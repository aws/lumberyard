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

#include "ICryMannequin.h"
#include "IForceFeedbackSystem.h"

#include <Mannequin/Serialization.h>

struct SForceFeedbackClipParams
    : public IProceduralParams
{
    SForceFeedbackClipParams()
        : scale(1.f)
        , delay(0.f)
        , onlyLocal(true)
    {
    }

    virtual void Serialize(Serialization::IArchive& ar)
    {
        ar(Serialization::Decorators::ForceFeedbackIdName<TProcClipString>(forceFeedbackId), "ForceFeedbackID", "ForceFeedbackID");
        ar(scale, "Scale", "Scale");
        ar(delay, "Delay", "Delay");
        ar(onlyLocal, "OnlyLocal", "OnlyLocal");
    }

    virtual void GetExtraDebugInfo(StringWrapper& extraInfoOut) const override
    {
        extraInfoOut = forceFeedbackId.c_str();
    }

    TProcClipString forceFeedbackId;
    float scale;
    float delay;
    bool onlyLocal;
};

class CProceduralClipForceFeedback
    : public TProceduralClip<SForceFeedbackClipParams>
{
public:
    CProceduralClipForceFeedback()
    {
    }

    virtual void OnEnter(float blendTime, float duration, const SForceFeedbackClipParams& params)
    {
        if (!m_entity)
        {
            CryLogAlways("Procedural clip force feedback does not support AZ Entity.");
            return;
        }

        IGameFramework* pGameFrameWork = gEnv->pGame->GetIGameFramework();

        if ((gEnv->IsEditor() && pGameFrameWork->GetMannequinInterface().IsSilentPlaybackMode())
            ||  (params.onlyLocal && pGameFrameWork->GetClientActorId() != m_entity->GetId()))
        {
            return;
        }

        IForceFeedbackSystem* pForceFeedback = CCryAction::GetCryAction()->GetIForceFeedbackSystem();
        CRY_ASSERT(pForceFeedback);
        ForceFeedbackFxId fxId = pForceFeedback->GetEffectIdByName(params.forceFeedbackId.c_str());

        if (fxId != InvalidForceFeedbackFxId)
        {
            float actionScale = 0.f;
            float ffScale = params.scale;

            if (GetParam("ffScale", actionScale))
            {
                ffScale *= actionScale;
            }

            SForceFeedbackRuntimeParams runtimeParams(ffScale, params.delay);
            pForceFeedback->PlayForceFeedbackEffect(fxId, runtimeParams, eIDT_Gamepad);
        }
    }

    virtual void OnExit(float blendTime) {}

    virtual void Update(float timePassed) {}
};

REGISTER_PROCEDURAL_CLIP(CProceduralClipForceFeedback, "ForceFeedback");
