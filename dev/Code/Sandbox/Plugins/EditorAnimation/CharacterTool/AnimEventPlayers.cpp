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

#include "stdafx.h"
#include <ICryAnimation.h>
#include <IAnimEventPlayer.h>
#include <IAudioSystem.h>
#include <IMaterialEffects.h>
#include <CryExtension/Impl/ClassWeaver.h>
#include <CryExtension/CryCreateClassInstance.h>
#include "EffectPlayer.h"

#include "Serialization/Decorators/ResourcesAudio.h"
#include "Serialization/Decorators/Resources.h"

namespace CharacterTool {
    using Serialization::IArchive;
    // A hack to move Character Tool audio objects away from level sounds
    // (we have to share the same audio world).
    static const Vec3 AUDIO_OFFSET(-8192, -8192, -8192);

    void SerializeParameterAudioTrigger(string& parameter, IArchive& ar)
    {
        ar(Serialization::AudioTrigger(parameter), "parameter", "^");
        ar.Doc("Audio triggers are defined in ATL Controls Browser.");
    }

    void SerializeParameterString(string& parameter, IArchive& ar) { ar(parameter, "parameter", "^"); }
    void SerializeParameterEffect(string& parameter, IArchive& ar) { ar(Serialization::ParticleName(parameter), "parameter", "^"); }
    void SerializeParameterNone(string& parameter, IArchive& ar) { ar(parameter, "parameter", 0); }

    static void SpliterParameterValue(string* switchName, string* switchValue, const char* parameter)
    {
        const char* sep = strchr(parameter, '=');
        if (!sep)
        {
            *switchName = parameter;
            *switchValue = string();
        }
        else
        {
            *switchName = string(parameter, sep);
            *switchValue = sep + 1;
        }
    }

    void SerializeParameterAudioSwitch(string& parameter, IArchive& ar)
    {
        string switchName;
        string switchState;
        SpliterParameterValue(&switchName, &switchState, parameter.c_str());
        ar(Serialization::AudioSwitch(switchName), "switchName", "^");
        ar(Serialization::AudioSwitchState(switchState), "switchState", "^");
        if (ar.IsInput())
        {
            parameter = switchName + "=" + switchState;
        }
    }

    void SerializeParameterAudioRTPC(string& parameter, IArchive& ar)
    {
        string name;
        string valueString;
        SpliterParameterValue(&name, &valueString, parameter.c_str());
        float value = (float)atof(valueString.c_str());
        ar(Serialization::AudioRTPC(name), "rtpcName", "^");
        ar(value, "rtpcValue", "^");

        char newValue[32] = { 0 };
        sprintf(newValue, "%f", value);
        if (ar.IsInput())
        {
            parameter = name + "=" + newValue;
        }
    }

    typedef void(* TSerializeParameterFunc)(string&, IArchive&);

    static SCustomAnimEventType g_atlEvents[] = {
        { "audio_trigger", ANIM_EVENT_USES_BONE, "Plays audio trigger using ATL" },
        { "audio_switch", 0, "Sets audio switch using ATL" },
        { "audio_rtpc", 0, "Sets RTPC value using ATL" },
        { "sound", ANIM_EVENT_USES_BONE, "Play audio trigger using ATL. Obsolete in favor of audio_trigger" },
    };

    TSerializeParameterFunc g_atlEventFunctions[sizeof(g_atlEvents) / sizeof(g_atlEvents[0])] = {
        &SerializeParameterAudioTrigger,
        &SerializeParameterAudioSwitch,
        &SerializeParameterAudioRTPC,
        &SerializeParameterAudioTrigger
    };

    static Vec3 GetBonePosition(ICharacterInstance* character, const QuatTS& physicalLocation, const char* boneName)
    {
        int jointId = -1;
        if (boneName && boneName[0] != '\0')
        {
            jointId = character->GetIDefaultSkeleton().GetJointIDByName(boneName);
        }
        if (jointId != -1)
        {
            return (physicalLocation * character->GetISkeletonPose()->GetAbsJointByID(jointId)).t;
        }
        else
        {
            return character->GetAABB().GetCenter();
        }
    }

    class AnimEventPlayer_AudioTranslationLayer
        : public IAnimEventPlayer
    {
    public:
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(IAnimEventPlayer)
        CRYINTERFACE_END()
        CRYGENERATE_CLASS(AnimEventPlayer_AudioTranslationLayer, "AnimEventPlayer_AudioTranslationLayer", 0xa9fefa2dfe04dec4, 0xa6169d6e3ac635b0)

        const SCustomAnimEventType * GetCustomType(int customTypeIndex) const override
        {
            int count = GetCustomTypeCount();
            if (customTypeIndex < 0 || customTypeIndex >= count)
            {
                return 0;
            }
            return &g_atlEvents[customTypeIndex];
        }

        void Initialize() override
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_audioProxy, &Audio::AudioSystemRequestBus::Events::GetFreeAudioProxy);
            if (!m_audioProxy)
            {
                return;
            }
            m_audioProxy->Initialize("Character Tool", false);
            SetPredefinedSwitches();
        }

        void Reset() override
        {
            if (!m_audioProxy)
            {
                return;
            }
            SetPredefinedSwitches();
        }

        void SetPredefinedSwitches()
        {
            if (!m_audioProxy)
            {
                return;
            }
            size_t num = m_predefinedSwitches.size();
            for (size_t i = 0; i < num; ++i)
            {
                SetSwitch(m_predefinedSwitches[i].name.c_str(), m_predefinedSwitches[i].state.c_str());
            }
        }

        int GetCustomTypeCount() const override
        {
            return sizeof(g_atlEvents) / sizeof(g_atlEvents[0]);
        }

        void Serialize(Serialization::IArchive& ar) override
        {
            ar.Doc("Allows to play triggers and control switches and RPC through Audio Translation Layer.");

            ar(m_predefinedSwitches, "predefinedSwitches", "Predefined Switches");
            ar.Doc("Defines a list of switches that are set before audio triggers are played.\n"
                "Can be used to test switches that are normally set by the game code.\n"
                "Such switches could be specific to the character, environment or story.");
        }

        const char* SerializeCustomParameter(const char* parameterValue, Serialization::IArchive& ar, int customTypeIndex) override
        {
            if (customTypeIndex < 0 || customTypeIndex >= GetCustomTypeCount())
            {
                return "";
            }
            m_parameter = parameterValue;
            g_atlEventFunctions[customTypeIndex](m_parameter, ar);
            return m_parameter.c_str();
        }


        void Update(ICharacterInstance* character, const QuatT& playerLocation, const Matrix34& cameraMatrix) override
        {
            m_playerLocation = playerLocation;

            if (!m_audioEnabled)
            {
                return;
            }

            Audio::SAudioRequest request;
            request.nAudioObjectID = INVALID_AUDIO_OBJECT_ID;
            request.nFlags = Audio::eARF_PRIORITY_NORMAL;
            request.pOwner = 0;

            Matrix34 cameraWithOffset = cameraMatrix;
            cameraWithOffset.SetTranslation(cameraMatrix.GetTranslation() + AUDIO_OFFSET);
            Audio::SAudioListenerRequestData<Audio::eALRT_SET_POSITION> requestData(cameraWithOffset);
            requestData.oNewPosition.NormalizeForwardVec();
            requestData.oNewPosition.NormalizeUpVec();
            request.pData = &requestData;


            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, request);

            if (m_audioProxy)
            {
                Audio::SATLWorldPosition pos;
                pos.mPosition = Matrix34(playerLocation);
                pos.mPosition.SetTranslation(pos.mPosition.GetTranslation() + AUDIO_OFFSET);
                m_audioProxy->SetPosition(pos);
            }
        }

        void EnableAudio(bool enableAudio) override
        {
            m_audioEnabled = enableAudio;
        }

        bool Play(ICharacterInstance* character, const AnimEventInstance& event) override
        {
            if (!m_audioProxy)
            {
                return false;
            }
            const char* name = event.m_EventName ? event.m_EventName : "";
            if (azstricmp(name, "sound") == 0 || azstricmp(name, "sound_tp") == 0)
            {
                PlayTrigger(event.m_CustomParameter);
                return true;
            }
            // If the event is an effect event, spawn the event.
            else if (azstricmp(name, "audio_trigger") == 0)
            {
                PlayTrigger(event.m_CustomParameter);
                return true;
            }
            else if (azstricmp(name, "audio_rtpc") == 0)
            {
                string rtpcName;
                string rtpcValue;
                SpliterParameterValue(&rtpcName, &rtpcValue, event.m_CustomParameter);

                SetRTPC(rtpcName.c_str(), float(atof(rtpcValue.c_str())));
                return true;
            }
            else if (azstricmp(name, "audio_switch") == 0)
            {
                string switchName;
                string switchValue;
                SpliterParameterValue(&switchName, &switchValue, event.m_CustomParameter);
                SetSwitch(switchName.c_str(), switchValue.c_str());
                return true;
            }
            return false;
        }

        void SetRTPC(const char* name, float value)
        {
            Audio::TAudioControlID rtpcId = INVALID_AUDIO_CONTROL_ID;
            Audio::AudioSystemRequestBus::BroadcastResult(rtpcId, &Audio::AudioSystemRequestBus::Events::GetAudioRtpcID, name);
            m_audioProxy->SetRtpcValue(rtpcId, value);
        }

        void SetSwitch(const char* name, const char* state)
        {
            Audio::TAudioControlID switchId = INVALID_AUDIO_CONTROL_ID;
            Audio::TAudioControlID stateId = INVALID_AUDIO_CONTROL_ID;
            Audio::AudioSystemRequestBus::BroadcastResult(switchId, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchID, name);
            Audio::AudioSystemRequestBus::BroadcastResult(stateId, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchStateID, switchId, state);
            m_audioProxy->SetSwitchState(switchId, stateId);
        }

        void PlayTrigger(const char* trigger)
        {
            Audio::TAudioControlID audioControlId = INVALID_AUDIO_CONTROL_ID;
            Audio::AudioSystemRequestBus::BroadcastResult(audioControlId, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, trigger);
            if (audioControlId != INVALID_AUDIO_CONTROL_ID)
            {
                m_audioProxy->ExecuteTrigger(audioControlId, eLSM_None);
            }
        }

    private:
        struct PredefinedSwitch
        {
            string name;
            string state;

            void Serialize(Serialization::IArchive& ar)
            {
                ar(Serialization::AudioSwitch(name), "name", "^");
                ar(Serialization::AudioSwitchState(state), "state", "^");
            }
        };

        std::vector<PredefinedSwitch> m_predefinedSwitches;

        bool m_audioEnabled;
        Audio::IAudioProxy* m_audioProxy;
        string m_parameter;

        QuatT m_playerLocation;
    };

    AnimEventPlayer_AudioTranslationLayer::AnimEventPlayer_AudioTranslationLayer()
        : m_audioEnabled(false)
        , m_audioProxy()
    {
    }

    AnimEventPlayer_AudioTranslationLayer::~AnimEventPlayer_AudioTranslationLayer()
    {
        if (m_audioProxy)
        {
            m_audioProxy->Release();
            m_audioProxy = 0;
        }
    }

    CRYREGISTER_CLASS(AnimEventPlayer_AudioTranslationLayer)

    // ---------------------------------------------------------------------------

    static SCustomAnimEventType g_mfxEvents[] = {
        { "foley", ANIM_EVENT_USES_BONE, "Foley are used for sounds that are specific to character and material that is being contacted." },
        { "footstep", ANIM_EVENT_USES_BONE, "Footstep are specific to character and surface being stepped on." },
        { "groundEffect", ANIM_EVENT_USES_BONE },
    };

    TSerializeParameterFunc g_mfxEventFuncitons[sizeof(g_mfxEvents) / sizeof(g_mfxEvents[0])] = {
        &SerializeParameterString,
        &SerializeParameterString,
        &SerializeParameterString,
    };

    class AnimEventPlayerMaterialEffects
        : public IAnimEventPlayer
    {
    public:
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(IAnimEventPlayer)
        CRYINTERFACE_END()
        CRYGENERATE_CLASS(AnimEventPlayerMaterialEffects, "AnimEventPlayer_MaterialEffects", 0xa9fffa2dae34d6c4, 0xa6969d6e3ac732b0)

        const SCustomAnimEventType * GetCustomType(int customTypeIndex) const override
        {
            int count = GetCustomTypeCount();
            if (customTypeIndex < 0 || customTypeIndex >= count)
            {
                return 0;
            }
            return &g_mfxEvents[customTypeIndex];
        }

        int GetCustomTypeCount() const override
        {
            return sizeof(g_mfxEvents) / sizeof(g_mfxEvents[0]);
        }

        void Serialize(Serialization::IArchive& ar) override
        {
            ar.Doc("Material Effects allow to define sound and particle response to interaction with certain surface types.");

            ar(m_foleyLibrary, "foleyLibrary", "Foley Library");
            ar.Doc("Foley libraries are located in Libs/MaterialEffects/FXLibs");
            ar(m_footstepLibrary, "footstepLibrary", "Footstep Library");
            ar.Doc("Foostep libraries are located in Libs/MaterialEffects/FXLibs");
            ar(m_defaultFootstepEffect, "footstepEffectName", "Default Footstep Effect");
            ar.Doc("Foostep effect that is used when no effect name specified in AnimEvent.");
        }

        const char* SerializeCustomParameter(const char* parameterValue, Serialization::IArchive& ar, int customTypeIndex) override
        {
            if (customTypeIndex < 0 || customTypeIndex >= GetCustomTypeCount())
            {
                return "";
            }
            m_parameter = parameterValue;
            g_mfxEventFuncitons[customTypeIndex](m_parameter, ar);
            return m_parameter.c_str();
        }


        void Update(ICharacterInstance* character, const QuatT& playerLocation, const Matrix34& cameraMatrix) override
        {
            m_playerLocation = playerLocation;
        }

        void EnableAudio(bool enableAudio) override
        {
            m_audioEnabled = enableAudio;
        }

        bool Play(ICharacterInstance* character, const AnimEventInstance& event) override
        {
            const char* name = event.m_EventName ? event.m_EventName : "";
            if (azstricmp(name, "footstep") == 0)
            {
                SMFXRunTimeEffectParams params;
                params.pos = GetBonePosition(character, m_playerLocation, event.m_BonePathName);
                params.angle = 0;

                IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects;
                TMFXEffectId effectId = InvalidEffectId;

                if (pMaterialEffects)
                {
                    const char* effectName = (event.m_CustomParameter && event.m_CustomParameter[0]) ? event.m_CustomParameter : m_defaultFootstepEffect.c_str();
                    effectId = pMaterialEffects->GetEffectIdByName(m_footstepLibrary.c_str(), effectName);
                }

                if (effectId != InvalidEffectId)
                {
                    pMaterialEffects->ExecuteEffect(effectId, params);
                }
                else
                {
                    gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find material for footstep sounds");
                }
                return true;
            }
            else if (azstricmp(name, "groundEffect") == 0)
            {
                // setup sound params
                SMFXRunTimeEffectParams params;
                params.pos = GetBonePosition(character, m_playerLocation, event.m_BonePathName);
                params.angle = 0;

                IMaterialEffects* materialEffects = gEnv->pMaterialEffects;
                TMFXEffectId effectId = InvalidEffectId;
                if (materialEffects)
                {
                    effectId = materialEffects->GetEffectIdByName(event.m_CustomParameter, "default");
                }

                if (effectId != 0)
                {
                    materialEffects->ExecuteEffect(effectId, params);
                }
                else
                {
                    gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find material for groundEffect anim event");
                }
                return true;
            }
            else if (azstricmp(name, "foley") == 0)
            {
                // setup sound params
                SMFXRunTimeEffectParams params;
                params.angle = 0;
                params.pos = GetBonePosition(character, m_playerLocation, event.m_BonePathName);

                IMaterialEffects* materialEffects = gEnv->pMaterialEffects;
                TMFXEffectId effectId = InvalidEffectId;

                // replace with "foley"
                if (materialEffects)
                {
                    effectId = materialEffects->GetEffectIdByName(m_foleyLibrary.c_str(),
                            event.m_CustomParameter[0] ? event.m_CustomParameter : "default");

                    if (effectId != InvalidEffectId)
                    {
                        materialEffects->ExecuteEffect(effectId, params);
                    }
                    else
                    {
                        gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find effect entry for foley sounds");
                    }
                }
                return true;
            }
            return false;
        }

    private:
        bool m_audioEnabled;
        string m_parameter;
        QuatT m_playerLocation;
        string m_foleyLibrary;
        string m_footstepLibrary;
        string m_defaultFootstepEffect;
    };

    AnimEventPlayerMaterialEffects::AnimEventPlayerMaterialEffects()
        : m_foleyLibrary("foley")
        , m_footstepLibrary("footstep")
        , m_defaultFootstepEffect("wood_hollow")
    {
    }

    AnimEventPlayerMaterialEffects::~AnimEventPlayerMaterialEffects()
    {
    }

    CRYREGISTER_CLASS(AnimEventPlayerMaterialEffects)

    // ---------------------------------------------------------------------------

    static SCustomAnimEventType g_particleEvents[] = {
        { "effect", ANIM_EVENT_USES_BONE | ANIM_EVENT_USES_OFFSET_AND_DIRECTION, "Spawns a particle effect." }
    };

    TSerializeParameterFunc g_particleEventFuncitons[sizeof(g_particleEvents) / sizeof(g_particleEvents[0])] = {
        &SerializeParameterEffect,
    };

    class AnimEventPlayer_Particles
        : public IAnimEventPlayer
    {
    public:
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(IAnimEventPlayer)
        CRYINTERFACE_END()
        CRYGENERATE_CLASS(AnimEventPlayer_Particles, "AnimEventPlayer_Particles", 0xa9fef72df304dec4, 0xa9162a6e4ac635b0)

        const SCustomAnimEventType * GetCustomType(int customTypeIndex) const override
        {
            int count = GetCustomTypeCount();
            if (customTypeIndex < 0 || customTypeIndex >= count)
            {
                return 0;
            }
            return &g_particleEvents[customTypeIndex];
        }

        void Initialize() override
        {
            m_effectPlayer.reset(new EffectPlayer());
        }

        int GetCustomTypeCount() const override
        {
            return sizeof(g_particleEvents) / sizeof(g_particleEvents[0]);
        }

        void Serialize(Serialization::IArchive& ar) override
        {
            ar.Doc("Spawns a particle effect for each \"effect\" AnimEvent.");
        }

        const char* SerializeCustomParameter(const char* parameterValue, Serialization::IArchive& ar, int customTypeIndex) override
        {
            if (customTypeIndex < 0 || customTypeIndex >= GetCustomTypeCount())
            {
                return "";
            }
            m_parameter = parameterValue;
            g_particleEventFuncitons[customTypeIndex](m_parameter, ar);
            return m_parameter.c_str();
        }

        void Reset() override
        {
            if (m_effectPlayer)
            {
                m_effectPlayer->KillAllEffects();
            }
        }

        void Render(const QuatT& playerPosition, SRendParams& params, const SRenderingPassInfo& passInfo) override
        {
            if (m_effectPlayer)
            {
                m_effectPlayer->Update(playerPosition);
            }
        }

        void Update(ICharacterInstance* character, const QuatT& playerLocation, const Matrix34& cameraMatrix) override
        {
            if (m_effectPlayer)
            {
                m_effectPlayer->SetSkeleton(character->GetISkeletonAnim(), character->GetISkeletonPose(), &character->GetIDefaultSkeleton());
            }
        }

        bool Play(ICharacterInstance* character, const AnimEventInstance& event) override
        {
            if (azstricmp(event.m_EventName, "effect") == 0)
            {
                if (m_effectPlayer)
                {
                    m_effectPlayer->SetSkeleton(character->GetISkeletonAnim(), character->GetISkeletonPose(), &character->GetIDefaultSkeleton());
                    m_effectPlayer->SpawnEffect(event.m_AnimID, event.m_AnimPathName, event.m_CustomParameter,
                        event.m_BonePathName, event.m_vOffset, event.m_vDir);
                }
                return true;
            }
            return false;
        }


    private:
        string m_parameter;
        std::unique_ptr<EffectPlayer> m_effectPlayer;
    };

    AnimEventPlayer_Particles::AnimEventPlayer_Particles()
    {
    }

    AnimEventPlayer_Particles::~AnimEventPlayer_Particles()
    {
    }

    CRYREGISTER_CLASS(AnimEventPlayer_Particles)

    // ---------------------------------------------------------------------------

    class AnimEventPlayer_CharacterTool
        : public IAnimEventPlayer
    {
    public:
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(IAnimEventPlayer)
        CRYINTERFACE_END()
        CRYGENERATE_CLASS(AnimEventPlayer_CharacterTool, "AnimEventPlayer_CharacterTool", 0xa5fefb2dfe05dec4, 0xa8169d6e3ac635b0)

        const SCustomAnimEventType * GetCustomType(int customTypeIndex) const override
        {
            int listSize = m_list.size();
            int lowerLimit = 0;
            for (int i = 0; i < listSize; ++i)
            {
                if (!m_list[i])
                {
                    continue;
                }
                int typeCount = m_list[i]->GetCustomTypeCount();
                if (customTypeIndex >= lowerLimit && customTypeIndex < lowerLimit + typeCount)
                {
                    return m_list[i]->GetCustomType(customTypeIndex - lowerLimit);
                }
                lowerLimit += typeCount;
            }
            return 0;
        }

        void Initialize() override
        {
            size_t listSize = m_list.size();
            for (size_t i = 0; i < listSize; ++i)
            {
                if (!m_list[i].get())
                {
                    continue;
                }
                m_list[i]->Initialize();
            }
        }

        int GetCustomTypeCount() const override
        {
            int typeCount = 0;

            size_t listSize = m_list.size();
            for (size_t i = 0; i < listSize; ++i)
            {
                if (!m_list[i].get())
                {
                    continue;
                }
                typeCount += m_list[i]->GetCustomTypeCount();
            }

            return typeCount;
        }

        void Serialize(Serialization::IArchive& ar) override
        {
            // m_list could be serialized directly to unlock full customization of the list:
            //   ar(m_list, "list", "^");

            size_t num = m_list.size();
            for (size_t i = 0; i < num; ++i)
            {
                ar(*m_list[i], m_list[i]->GetFactory()->GetName(), m_names[i].c_str());
            }
        }

        const char* SerializeCustomParameter(const char* parameterValue, Serialization::IArchive& ar, int customTypeIndex) override
        {
            int listSize = m_list.size();
            int lowerLimit = 0;
            for (int i = 0; i < listSize; ++i)
            {
                if (!m_list[i])
                {
                    continue;
                }
                int typeCount = m_list[i]->GetCustomTypeCount();
                if (customTypeIndex >= lowerLimit && customTypeIndex < lowerLimit + typeCount)
                {
                    return m_list[i]->SerializeCustomParameter(parameterValue, ar, customTypeIndex - lowerLimit);
                }
                lowerLimit += typeCount;
            }

            return "";
        }

        void Reset() override
        {
            int listSize = m_list.size();
            for (int i = 0; i < listSize; ++i)
            {
                if (!m_list[i])
                {
                    continue;
                }
                m_list[i]->Reset();
            }
        }

        void Render(const QuatT& playerPosition, SRendParams& params, const SRenderingPassInfo& passInfo) override
        {
            int listSize = m_list.size();
            for (int i = 0; i < listSize; ++i)
            {
                if (!m_list[i])
                {
                    continue;
                }
                m_list[i]->Render(playerPosition, params, passInfo);
            }
        }

        void Update(ICharacterInstance* character, const QuatT& playerLocation, const Matrix34& cameraMatrix) override
        {
            int listSize = m_list.size();
            for (int i = 0; i < listSize; ++i)
            {
                if (!m_list[i])
                {
                    continue;
                }
                m_list[i]->Update(character, playerLocation, cameraMatrix);
            }
        }

        void EnableAudio(bool enableAudio) override
        {
            int listSize = m_list.size();
            for (int i = 0; i < listSize; ++i)
            {
                if (!m_list[i])
                {
                    continue;
                }
                m_list[i]->EnableAudio(enableAudio);
            }
        }

        bool Play(ICharacterInstance* character, const AnimEventInstance& event) override
        {
            int listSize = m_list.size();
            for (int i = 0; i < listSize; ++i)
            {
                if (!m_list[i])
                {
                    continue;
                }
                if (m_list[i]->Play(character, event))
                {
                    return true;
                }
            }
            return false;
        }

    private:
        std::vector<IAnimEventPlayerPtr> m_list;
        std::vector<string> m_names;
        string m_defaultGamePlayerName;
    };

    AnimEventPlayer_CharacterTool::AnimEventPlayer_CharacterTool()
    {
        if (ICVar* gameName = gEnv->pConsole->GetCVar("sys_game_name"))
        {
            // Expect game dll to have its own player, add it first into the list,
            // so it has the highest priority.
            string defaultGamePlayerName = "AnimEventPlayer_";
            defaultGamePlayerName += gameName->GetString();

            IAnimEventPlayerPtr defaultGamePlayer;
            if (::CryCreateClassInstance<IAnimEventPlayer>(defaultGamePlayerName.c_str(), defaultGamePlayer))
            {
                m_list.push_back(defaultGamePlayer);
                m_names.push_back(gameName->GetString());
            }
        }

        IAnimEventPlayerPtr player;
        if (::CryCreateClassInstance<IAnimEventPlayer>("AnimEventPlayer_AudioTranslationLayer", player))
        {
            m_list.push_back(player);
            m_names.push_back("Audio Translation Layer");
        }
        if (::CryCreateClassInstance<IAnimEventPlayer>("AnimEventPlayer_MaterialEffects", player))
        {
            m_list.push_back(player);
            m_names.push_back("Material Effects");
        }
        if (::CryCreateClassInstance<IAnimEventPlayer>("AnimEventPlayer_Particles", player))
        {
            m_list.push_back(player);
            m_names.push_back("Particles");
        }
    }

    AnimEventPlayer_CharacterTool::~AnimEventPlayer_CharacterTool()
    {
    }

    CRYREGISTER_CLASS(AnimEventPlayer_CharacterTool)
}
