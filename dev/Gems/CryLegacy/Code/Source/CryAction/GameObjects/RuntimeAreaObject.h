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

#ifndef RUNTIMEAREAOBJECT_H_INCLUDED
#define RUNTIMEAREAOBJECT_H_INCLUDED

class CRuntimeAreaObject
    : public CGameObjectExtensionHelper<CRuntimeAreaObject, IGameObjectExtension>
{
public:
    DECLARE_COMPONENT_TYPE("RuntimeAreaObject", 0x8EFC7E5B0ED14A70, 0x803590F4C32ED240)

    typedef unsigned int TSurfaceCRC;

    struct SAudioControls
    {
        Audio::TAudioControlID  nTriggerID;
        Audio::TAudioControlID  nRtpcID;

        explicit SAudioControls(
            Audio::TAudioControlID nPassedTriggerID = INVALID_AUDIO_CONTROL_ID,
            Audio::TAudioControlID nPassedRtpcID = INVALID_AUDIO_CONTROL_ID)
            : nTriggerID(nPassedTriggerID)
            , nRtpcID(nPassedRtpcID)
        {}
    };

    typedef std::map<TSurfaceCRC, SAudioControls> TAudioControlMap;

    static TAudioControlMap m_cAudioControls;

    CRuntimeAreaObject();
    virtual ~CRuntimeAreaObject() override;

    // IGameObjectExtension
    virtual bool Init(IGameObject* pGameObject) override;
    virtual void InitClient(ChannelId channelId) override {}
    virtual void PostInit(IGameObject* pGameObject) override {}
    virtual void PostInitClient(ChannelId channelId) override {}
    virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
    virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override {}
    virtual bool GetEntityPoolSignature(TSerialize signature) override;
    virtual void FullSerialize(TSerialize ser) override {}
    virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) override;
    virtual void PostSerialize() override {}
    virtual void SerializeSpawnInfo(TSerialize ser, IEntity* entity) override {}
    virtual ISerializableInfoPtr GetSpawnInfo() override {return NULL; }
    virtual void Update(SEntityUpdateContext& ctx, int slot) override {}
    virtual void HandleEvent(const SGameObjectEvent& gameObjectEvent) override {}
    virtual void ProcessEvent(SEntityEvent& entityEvent) override;
    virtual void SetChannelId(ChannelId id) override {}
    virtual void SetAuthority(bool auth) override {}
    virtual void PostUpdate(float frameTime) override { CRY_ASSERT(false); }
    virtual void PostRemoteSpawn() override {}
    virtual void GetMemoryUsage(ICrySizer* pSizer) const override;
    // ~IGameObjectExtension

private:

    struct SAreaSoundInfo
    {
        SAudioControls  oAudioControls;
        float                       fParameter;

        explicit SAreaSoundInfo(SAudioControls const& rPassedAudioControls, float const fPassedParameter = 0.0f)
            : oAudioControls(rPassedAudioControls)
            , fParameter(fPassedParameter)
        {}

        ~SAreaSoundInfo()
        {}
    };

    typedef std::map<TSurfaceCRC, SAreaSoundInfo>   TAudioParameterMap;
    typedef std::map<EntityId, TAudioParameterMap> TEntitySoundsMap;

    void UpdateParameterValues(IEntity* const pEntity, TAudioParameterMap& rParamMap);
    void StopEntitySounds(EntityId const nEntityID, TAudioParameterMap& rParamMap);

    TEntitySoundsMap m_cActiveEntitySounds;
};

#endif // RUNTIMEAREAOBJECT_H_INCLUDED
