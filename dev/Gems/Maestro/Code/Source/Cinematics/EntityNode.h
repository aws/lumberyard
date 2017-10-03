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

#ifndef CRYINCLUDE_CRYMOVIE_ENTITYNODE_H
#define CRYINCLUDE_CRYMOVIE_ENTITYNODE_H

#pragma once

#include <set>
#include "AnimNode.h"
#include "SoundTrack.h"
#include "StlUtils.h"
#include "CharacterTrackAnimator.h"

#include <IFacialAnimation.h>

#define ENTITY_SOUNDTRACKS  3
#define ENTITY_EXPRTRACKS       3
#define DEFAULT_LOOKIK_LAYER 15

// When preloading of a trackview sequence is triggered, the animations in
// the interval [0 ; ANIMATION_KEY_PRELOAD_INTERVAL] are preloaded into memory.
// When the animation is running Animate() makes sure that all animation keys
// between in the interval [currTime; currTime + ANIMATION_KEY_PRELOAD_INTERVAL]
// are kept in memory.
// This value should be adjusted so that the streaming system has enough
// time to stream the keys in.
#define ANIMATION_KEY_PRELOAD_INTERVAL 2.0f

// remove comment to enable the check.
//#define CHECK_FOR_TOO_MANY_ONPROPERTY_SCRIPT_CALLS


struct ISkeletonAnim;

class CAnimationCacher
{
public:
    CAnimationCacher() { m_cachedAnims.reserve(MAX_CHARACTER_TRACKS); }
    ~CAnimationCacher() { ClearCache(); }
    void AddToCache(uint32 animPathCRC);
    void RemoveFromCache(uint32 animPathCRC);
    void ClearCache();
private:
    std::vector<uint32> m_cachedAnims;
};

class CAnimEntityNode
    : public CAnimNode
{
    struct SScriptPropertyParamInfo;
    struct SAnimState;

public:
    AZ_CLASS_ALLOCATOR(CAnimEntityNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimEntityNode, "{B84FED66-38AD-497E-AB39-BCA51F1A99E4}", CAnimNode);

    CAnimEntityNode();
    CAnimEntityNode(const int id, EAnimNodeType nodeType);
    ~CAnimEntityNode();
    static void Initialize();

    void EnableEntityPhysics(bool bEnable);

    virtual void AddTrack(IAnimTrack* pTrack);

    virtual void SetEntityGuid(const EntityGUID& guid);
    virtual EntityGUID* GetEntityGuid() { return &m_entityGuid; }
    virtual void SetEntityId(const int entityId);
    virtual int GetEntityId() const { return m_EntityId; }

    virtual IEntity* GetEntity();

    virtual void SetEntityGuidTarget(const EntityGUID& guid);
    virtual void SetEntityGuidSource(const EntityGUID& guid);

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CAnimNode
    //////////////////////////////////////////////////////////////////////////
    virtual void StillUpdate();
    void Animate(SAnimContext& ec) override;

    virtual void CreateDefaultTracks();

    void PrecacheStatic(float startTime) override;
    void PrecacheDynamic(float time) override;

    void SetPos(float time, const Vec3& pos) override;
    void SetRotate(float time, const Quat& quat) override;
    void SetScale(float time, const Vec3& scale) override;

    IAnimTrack* CreateTrack(const CAnimParamType& paramType) override;
    bool RemoveTrack(IAnimTrack* pTrack) override;

    Vec3 GetPos() override { return m_pos; };
    Quat GetRotate() override { return m_rotate; };
    Vec3 GetScale() override { return m_scale; };

    virtual void Activate(bool bActivate);

    ICharacterInstance* GetCharacterInstance() override;

    //////////////////////////////////////////////////////////////////////////
    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);
    void OnStart() override;
    void OnReset() override;
    void OnResetHard() override;
    void OnPause() override;
    void OnStop() override;
    void OnLoop() override;

    //////////////////////////////////////////////////////////////////////////
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;
    virtual const char* GetParamName(const CAnimParamType& param) const;

    static int GetParamCountStatic();
    static bool GetParamInfoStatic(int nIndex, SParamInfo& info);

    ILINE void SetSkipInterpolatedCameraNode(const bool bSkipNodeAnimation) override { m_bSkipInterpolatedCameraNodeAnimation = bSkipNodeAnimation; }
    ILINE bool IsSkipInterpolatedCameraNodeEnabled() const { return m_bSkipInterpolatedCameraNodeAnimation; }

    ILINE const Vec3& GetCameraInterpolationPosition() const { return m_vInterpPos; }
    ILINE void SetCameraInterpolationPosition(const Vec3& vNewPos){ m_vInterpPos = vNewPos; }

    ILINE const Quat& GetCameraInterpolationRotation() const { return m_interpRot; }
    ILINE void SetCameraInterpolationRotation(const Quat& rotation){ m_interpRot = rotation; }

    static void Reflect(AZ::SerializeContext* serializeContext);

protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

    void ReleaseSounds();   // Stops audio
    void ApplyEventKey(class CEventTrack* track, int keyIndex, IEventKey& key);
    void ApplyAudioKey(char const* const sTriggerName, bool const bPlay = true) override;
    Vec3 Adjust3DSoundOffset(bool bVoice, IEntity* pEntity, Vec3& oSoundPos) const;
    
    void AnimateLookAt(class CLookAtTrack* pTrack, SAnimContext& ec);
    bool AnimateScriptTableProperty(IAnimTrack* pTrack, SAnimContext& ec, const char* name);

    virtual void OnStartAnimation(const char* sAnimation) {}
    virtual void OnEndAnimation(const char* sAnimation);

    void PrepareAnimations();

    void EnableProceduralFacialAnimation(bool enable);
    bool GetProceduralFacialAnimationEnabled();

    // functions involved in the process to parse and store lua animated properties
    void UpdateDynamicParamsInternal() override;
    virtual void UpdateDynamicParams_Editor();
    virtual void UpdateDynamicParams_PureGame();

    void FindDynamicPropertiesRec(IScriptTable* pScriptTable, const string& currentPath, unsigned int depth);
    void AddPropertyToParamInfoMap(const char* pKey, const string& currentPath, SScriptPropertyParamInfo& paramInfo);
    bool ObtainPropertyTypeInfo(const char* pKey, IScriptTable* pScriptTable, SScriptPropertyParamInfo& paramInfo);
    void FindScriptTableForParameterRec(IScriptTable* pScriptTable, const string& path, string& propertyName, SmartScriptTable& propertyScriptTable);

    virtual void InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType) override;

    void ResetSounds() override;

    enum EUpdateEntityFlags
    {
        eUpdateEntity_Position = 1 << 0,
        eUpdateEntity_Rotation = 1 << 1,
        eUpdateEntity_Animation = 1 << 2,
    };

protected:
    Vec3 m_pos;
    Quat m_rotate;
    Vec3 m_scale;

private:
    void UpdateEntityPosRotVel(const Vec3& targetPos, const Quat& targetRot, const bool initialState, const int flags, float fTime);
    void StopEntity();
    void UpdateTargetCamera(IEntity* pEntity, const Quat& rotation);

    //! Reference to game entity.
    EntityGUID m_entityGuid;
    EntityId m_EntityId;


    CAnimationCacher m_animationCacher;

    //! Pointer to target animation node.
    AZStd::intrusive_ptr<IAnimNode> m_target;

    // Cached parameters of node at given time.
    float m_time;
    Vec3 m_velocity;
    Vec3 m_angVelocity;

    // Camera2Camera interpolation values
    Vec3 m_vInterpPos;
    Quat m_interpRot;
    bool m_bSkipInterpolatedCameraNodeAnimation;

    //! Last animated key in Entity track.
    int m_lastEntityKey;

    bool m_visible;
    bool m_bInitialPhysicsStatus;

    std::vector<SSoundInfo> m_SoundInfo;
    int m_iCurMannequinKey;

    string m_lookAtTarget;
    EntityId m_lookAtEntityId;
    bool m_lookAtLocalPlayer;
    bool m_allowAdditionalTransforms;
    string m_lookPose;

    bool m_proceduralFacialAnimationEnabledOld;

    //! Reference LookAt entities.
    EntityGUID m_entityGuidTarget;
    EntityId m_EntityIdTarget;
    EntityGUID m_entityGuidSource;
    EntityId m_EntityIdSource;

    // Pos/rot noise parameters
    struct Noise
    {
        float m_amp;
        float m_freq;

        Vec3 Get(float time) const;

        Noise()
            : m_amp(0.0f)
            , m_freq(0.0f) {}
    };

    Noise m_posNoise;
    Noise m_rotNoise;

    struct SScriptPropertyParamInfo
    {
        string variableName;
        string displayName;
        SmartScriptTable scriptTable;
        bool isVectorTable;
        SParamInfo animNodeParamInfo;
    };

    std::vector< SScriptPropertyParamInfo > m_entityScriptPropertiesParamInfos;
    typedef AZStd::unordered_map< string, size_t, stl::hash_string_caseless<string>, stl::equality_string_caseless<string> > TScriptPropertyParamInfoMap;
    TScriptPropertyParamInfoMap m_nameToScriptPropertyParamInfo;
    #ifdef CHECK_FOR_TOO_MANY_ONPROPERTY_SCRIPT_CALLS
    uint32 m_OnPropertyCalls;
    #endif

    // helper class responsible for animating Character Tracks (aka 'Animation' tracks in the TrackView UI)
    CCharacterTrackAnimator* m_characterTrackAnimator = nullptr;
};

#endif // CRYINCLUDE_CRYMOVIE_ENTITYNODE_H
