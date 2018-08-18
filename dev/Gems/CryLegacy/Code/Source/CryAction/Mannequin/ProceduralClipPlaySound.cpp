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

#include <AzCore/std/containers/vector.h>

#include <ICryMannequin.h>
#include <Mannequin/Serialization.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>

#include <Components/IComponentAudio.h>

using namespace Audio;
using namespace LmbrCentral;



//=========================================================================
SERIALIZATION_ENUM_BEGIN(EAudioObjectObstructionCalcType, "SoundObstructionType");
    SERIALIZATION_ENUM(eAOOCT_IGNORE, "Ignore", "Ignore");
    SERIALIZATION_ENUM(eAOOCT_SINGLE_RAY, "SingleRay", "Single Ray");
    SERIALIZATION_ENUM(eAOOCT_MULTI_RAY, "MultiRay", "Multi Ray");
SERIALIZATION_ENUM_END();


//=========================================================================
class CAudioContext
    : public IProceduralContext
{
private:
    using BaseClass = IProceduralContext;

public:
    PROCEDURAL_CONTEXT(CAudioContext, "AudioContext", 0xC6C087F64CE14854, 0xADCA544D252834BD);

    void InitialiseLegacy(IEntity& entity, IActionController& actionController) override
    {
        BaseClass::InitialiseLegacy(entity, actionController);

        m_pAudioComponent = entity.GetOrCreateComponent<IComponentAudio>();
    }

    void Initialise(const AZ::EntityId& entityId, IActionController& actionController) override
    {
        BaseClass::Initialise(entityId, actionController);

        AZ_Warning("ProceduralClip Audio", LmbrCentral::AudioProxyComponentRequestBus::FindFirstHandler(entityId),
            "Attempting to play an Audio ProcClip on EntityId%s but the entity has no Audio components!", entityId.ToString().c_str());
    }

    void Update(float timePassed) override
    {
    }

    void ExecuteAudioTrigger(const Audio::TAudioControlID nID, const Audio::EAudioObjectObstructionCalcType eObstOcclCalcType, bool playFacial)
    {
        if (m_pAudioComponent)
        {
            m_pAudioComponent->SetObstructionCalcType(eObstOcclCalcType);
            m_pAudioComponent->ExecuteTrigger(nID, playFacial ? eLSM_MatchAnimationToSoundName : eLSM_None);
        }
        else
        {
            AudioProxyComponentRequestBus::Event(m_entityId, &AudioProxyComponentRequests::SetObstructionCalcType, eObstOcclCalcType);
            AudioProxyComponentRequestBus::Event(m_entityId, &AudioProxyComponentRequests::ExecuteTrigger, nID, Audio::SAudioCallBackInfos::GetEmptyObject());
        }
    }

    void StopAudioTrigger(const Audio::TAudioControlID nID)
    {
        if (m_pAudioComponent)
        {
            m_pAudioComponent->StopTrigger(nID);
        }
        else
        {
            AudioProxyComponentRequestBus::Event(m_entityId, &AudioProxyComponentRequests::KillTrigger, nID);
        }
    }

    void SetAudioObjectPos(const QuatT& rOffset)
    {
        if (m_pAudioComponent)
        {
            m_pAudioComponent->SetAuxAudioProxyOffset(Audio::SATLWorldPosition(rOffset.t));
        }
        else
        {
            AudioProxyComponentRequestBus::Event(m_entityId, &AudioProxyComponentRequests::SetPosition, Audio::SATLWorldPosition(rOffset.t));
        }
    }

private:
    IComponentAudioPtr m_pAudioComponent;
};

CAudioContext::CAudioContext()
    : m_pAudioComponent(nullptr)
{
}

CAudioContext::~CAudioContext()
{
}

CRYREGISTER_CLASS(CAudioContext);


//=========================================================================
struct SAudioParams
    : public IProceduralParams
{
    SAudioParams()
        : stopTrigger("do_nothing")
        , audioObjectObstructionCalcType(Audio::eAOOCT_IGNORE)
        , radius(0.f)
        , isVoice(false)
        , playFacial(false)
    {
    }

    void Serialize(Serialization::IArchive& ar) override
    {
        ar(Serialization::Decorators::SoundName<TProcClipString>(startTrigger), "StartTrigger", "Start Trigger");
        ar(Serialization::Decorators::SoundName<TProcClipString>(stopTrigger), "StopTrigger", "Stop Trigger");
        ar(audioObjectObstructionCalcType, "SoundObstructionType", "Sound Obstruction Type");
        ar(Serialization::Decorators::JointName<SProcDataCRC>(attachmentJoint), "AttachmentJoint", "Joint Name");
        if (!ar.IsEdit())
        {
            // Disabled the following deprecated options in the UI, but still read/write old data in case we need them back
            ar(radius, "Radius", "AI Radius");
            ar(isVoice, "IsVoice", "Is Voice");
        }
        ar(playFacial, "PlayFacial", "Request Anim Matching Start Trigger");
    }

    void GetExtraDebugInfo(StringWrapper& extraInfoOut) const override
    {
        extraInfoOut = startTrigger.c_str();
    }

    TProcClipString startTrigger;
    TProcClipString stopTrigger;
    Audio::EAudioObjectObstructionCalcType audioObjectObstructionCalcType;
    SProcDataCRC attachmentJoint;
    float radius;
    bool synchStop;
    bool forceStopOnExit;
    bool isVoice;
    bool playFacial;
};


//=========================================================================
class CProceduralClipAudio
    : public TProceduralContextualClip<CAudioContext, SAudioParams>
{
public:
    CProceduralClipAudio()
        : m_nAudioControlIDStart(INVALID_AUDIO_CONTROL_ID)
        , m_nAudioControlIDStop(INVALID_AUDIO_CONTROL_ID)
        , m_eObstOcclCalcType(Audio::eAOOCT_IGNORE)
    {
    }

private:
    struct SAudioParamInfo
    {
        SAudioParamInfo()
            : paramIndex(-1)
            , paramCRC(0)
            , paramValue(-1.f)
        {}
        SAudioParamInfo(int _index, uint32 _crc, float _value)
            : paramIndex(_index)
            , paramCRC(_crc)
            , paramValue(_value)
        {}

        int paramIndex;
        uint32 paramCRC;
        float paramValue;
    };

    using TAudioParamVec = AZStd::vector<SAudioParamInfo>;

public:
    void OnEnter(float blendTime, float duration, const SAudioParams& params) override
    {
        m_referenceJointID = -1;
        const bool playFacial = params.playFacial;
        const bool isVoice = (playFacial || params.isVoice);

        bool bIsSilentPlaybackMode = (gEnv->IsEditor() && gEnv->pGame->GetIGameFramework()->GetMannequinInterface().IsSilentPlaybackMode());

        const ICharacterInstance* const pCharacterInstance = m_scope->GetCharInst();

        if (pCharacterInstance)
        {
            m_referenceJointID = pCharacterInstance->GetIDefaultSkeleton().GetJointIDByCRC32(params.attachmentJoint.ToUInt32());
        }

        if (!bIsSilentPlaybackMode)
        {
            m_eObstOcclCalcType = params.audioObjectObstructionCalcType;

            if (!params.startTrigger.empty())
            {
                Audio::AudioSystemRequestBus::BroadcastResult(m_nAudioControlIDStart, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, params.startTrigger.c_str());

                if (m_nAudioControlIDStart != INVALID_AUDIO_CONTROL_ID)
                {
                    m_context->ExecuteAudioTrigger(m_nAudioControlIDStart, m_eObstOcclCalcType, playFacial);
                }
            }

            if (!params.stopTrigger.empty())
            {
                Audio::AudioSystemRequestBus::BroadcastResult(m_nAudioControlIDStop, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, params.stopTrigger.c_str());
            }
        }
    }

    void OnExit(float blendTime) override
    {
        if (m_nAudioControlIDStop != INVALID_AUDIO_CONTROL_ID)
        {
            m_context->ExecuteAudioTrigger(m_nAudioControlIDStop, m_eObstOcclCalcType, false);
        }
        else if (m_nAudioControlIDStart != INVALID_AUDIO_CONTROL_ID)
        {
            m_context->StopAudioTrigger(m_nAudioControlIDStart);
        }

        m_nAudioControlIDStart = INVALID_AUDIO_CONTROL_ID;
        m_nAudioControlIDStop = INVALID_AUDIO_CONTROL_ID;
    }

    void Update(float timePassed) override
    {
        UpdateSoundParams();
        UpdateSoundPosition();
    }

private:

    void UpdateSoundParams()
    {
        if (!m_oAudioParams.empty())
        {
            const size_t numParams = m_oAudioParams.size();
            for (size_t i = 0; i < numParams; i++)
            {
                float paramValue = 0.f;
                if (GetParam(m_oAudioParams[i].paramCRC, paramValue) && (m_oAudioParams[i].paramValue != paramValue))
                {
                    // Audio: update RTPCs
                    m_oAudioParams[i].paramValue = paramValue;
                }
            }
        }
    }

    void UpdateSoundPosition()
    {
        if (m_referenceJointID < 0)
        {
            return;
        }

        m_context->SetAudioObjectPos(GetBoneAbsLocationByID(m_referenceJointID));
    }

    QuatT GetBoneAbsLocationByID(const int jointID)
    {
        ICharacterInstance* pCharacterInstance = m_scope->GetCharInst();
        if (pCharacterInstance && (jointID >= 0))
        {
            return pCharacterInstance->GetISkeletonPose()->GetAbsJointByID(jointID);
        }

        return QuatT(ZERO, IDENTITY);
    }

    TAudioParamVec m_oAudioParams;

    int  m_referenceJointID;

    Audio::TAudioControlID m_nAudioControlIDStart;
    Audio::TAudioControlID m_nAudioControlIDStop;
    Audio::EAudioObjectObstructionCalcType m_eObstOcclCalcType;
};


//=========================================================================
typedef CProceduralClipAudio CProceduralClipPlaySound;

REGISTER_PROCEDURAL_CLIP(CProceduralClipPlaySound, "PlaySound");
REGISTER_PROCEDURAL_CLIP(CProceduralClipAudio, "Audio");
