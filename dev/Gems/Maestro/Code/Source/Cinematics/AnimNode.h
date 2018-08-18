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

// Description : Base of all Animation Nodes


#ifndef CRYINCLUDE_CRYMOVIE_ANIMNODE_H
#define CRYINCLUDE_CRYMOVIE_ANIMNODE_H

#pragma once

#include "IMovieSystem.h"
#include "Movie.h"

// forward declaration
struct SSoundInfo;

/*!
        Base class for all Animation nodes,
        can host multiple animation tracks, and execute them other time.
        Animation node is reference counted.
 */
class CAnimNode
    : public IAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimNode, "{57736B48-5EE7-4530-8051-657ACC9BA1EE}", IAnimNode);

    CAnimNode();
    CAnimNode(const CAnimNode& other);
    CAnimNode(const int id, AnimNodeType nodeType);
    ~CAnimNode();

    AnimNodeType GetType() const override { return m_nodeType; }

    //////////////////////////////////////////////////////////////////////////
    void add_ref() override;
    void release() override;
    //////////////////////////////////////////////////////////////////////////

    void SetName(const char* name) override { m_name = name; };
    const char* GetName() { return m_name.c_str(); };

    void SetSequence(IAnimSequence* sequence) override { m_pSequence = sequence; }
    // Return Animation Sequence that owns this node.
    IAnimSequence* GetSequence() const override { return m_pSequence; };

    virtual void SetEntityGuid(const EntityGUID& guid) {};
    virtual void SetEntityGuidTarget(const EntityGUID& guid) {};
    virtual void SetEntityGuidSource(const EntityGUID& guid) {};

    virtual EntityGUID* GetEntityGuid() { return NULL; };

    // animNode's aren't bound to Entities - the derived CAnimEntityNode is - return no bound Entity by default
    IEntity*     GetEntity() override { return nullptr; };

    void         SetEntityId(const int id) override {};

    // CAnimNode's aren't bound to AZ::Entities, CAnimAzEntityNodes are. return InvalidEntityId by default
    void         SetAzEntityId(const AZ::EntityId& id) override {}
    AZ::EntityId GetAzEntityId() const override { return AZ::EntityId(); }

    void SetFlags(int flags) override;
    int GetFlags() const override;
    bool AreFlagsSetOnNodeOrAnyParent(EAnimNodeFlags flagsToCheck) const override;

    IMovieSystem*   GetMovieSystem() const { return gEnv->pMovieSystem; };

    virtual void OnStart() {}
    void OnReset() override {}
    virtual void OnResetHard() { OnReset(); }
    virtual void OnPause() {}
    virtual void OnResume() {}
    virtual void OnStop() {}
    virtual void OnLoop() {}

    //////////////////////////////////////////////////////////////////////////
    // Space position/orientation scale.
    //////////////////////////////////////////////////////////////////////////
    void SetPos(float time, const Vec3& pos) override {};
    void SetRotate(float time, const Quat& quat) override {};
    void SetScale(float time, const Vec3& scale) override {};

    Vec3 GetPos() override { return Vec3(0, 0, 0); };
    Quat GetRotate() override { return Quat(0, 0, 0, 0); };
    Quat GetRotate(float /*time*/) override { return Quat(0, 0, 0, 0); };
    Vec3 GetScale() override { return Vec3(0, 0, 0); };

    virtual Matrix34 GetReferenceMatrix() const;

    //////////////////////////////////////////////////////////////////////////
    bool IsParamValid(const CAnimParamType& paramType) const;
    virtual const char* GetParamName(const CAnimParamType& param) const;
    virtual AnimValueType GetParamValueType(const CAnimParamType& paramType) const;
    virtual IAnimNode::ESupportedParamFlags GetParamFlags(const CAnimParamType& paramType) const;
    virtual unsigned int GetParamCount() const { return 0; };

    bool SetParamValue(float time, CAnimParamType param, float val);
    bool SetParamValue(float time, CAnimParamType param, const Vec3& val);
    bool SetParamValue(float time, CAnimParamType param, const Vec4& val);
    bool GetParamValue(float time, CAnimParamType param, float& val);
    bool GetParamValue(float time, CAnimParamType param, Vec3& val);
    bool GetParamValue(float time, CAnimParamType param, Vec4& val);

    void SetTarget(IAnimNode* node) {};
    IAnimNode* GetTarget() const { return 0; };

    void StillUpdate() {}
    void Animate(SAnimContext& ec) override;

    virtual void PrecacheStatic(float startTime) {}
    virtual void PrecacheDynamic(float time) {}

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;
    void InitPostLoad(IAnimSequence* sequence) override;

    void SetNodeOwner(IAnimNodeOwner* pOwner) override;
    IAnimNodeOwner* GetNodeOwner() override { return m_pOwner; };

    // Called by sequence when needs to activate a node.
    virtual void Activate(bool bActivate);

    //////////////////////////////////////////////////////////////////////////
    void SetParent(IAnimNode* parent) override;
    IAnimNode* GetParent() const override { return m_pParentNode; };
    IAnimNode* HasDirectorAsParent() const override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Track functions.
    //////////////////////////////////////////////////////////////////////////
    int  GetTrackCount() const override;
    IAnimTrack* GetTrackByIndex(int nIndex) const override;
    IAnimTrack* GetTrackForParameter(const CAnimParamType& paramType) const override;
    IAnimTrack* GetTrackForParameter(const CAnimParamType& paramType, uint32 index) const override;

    uint32 GetTrackParamIndex(const IAnimTrack* pTrack) const override;

    void SetTrack(const CAnimParamType& paramType, IAnimTrack* track) override;
    IAnimTrack* CreateTrack(const CAnimParamType& paramType) override;
    void InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType) override {}
    void SetTimeRange(Range timeRange) override;
    void AddTrack(IAnimTrack* pTrack) override;
    bool RemoveTrack(IAnimTrack* pTrack) override;
    void CreateDefaultTracks() override {};

    void SerializeAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;
    //////////////////////////////////////////////////////////////////////////

    virtual void PostLoad();

    int GetId() const { return m_id; }
    void SetId(int id) { m_id = id; }
    const char* GetNameFast() const { return m_name.c_str(); }

    virtual void Render(){}

    void UpdateDynamicParams() final;

    void TimeChanged(float newTime) override;

    void SetExpanded(bool expanded) override;
    bool GetExpanded() const override;

    static void Reflect(AZ::SerializeContext* serializeContext);

protected:
    virtual void UpdateDynamicParamsInternal() {};
    virtual bool GetParamInfoFromType(const CAnimParamType& paramType, SParamInfo& info) const { return false; };

    int  NumTracks() const { return (int)m_tracks.size(); }
    IAnimTrack* CreateTrackInternal(const CAnimParamType& paramType, EAnimCurveType trackType, AnimValueType valueType);

    IAnimTrack* CreateTrackInternalVector4(const CAnimParamType& paramType) const;
    IAnimTrack* CreateTrackInternalQuat(EAnimCurveType trackType, const CAnimParamType& paramType) const;
    IAnimTrack* CreateTrackInternalVector(EAnimCurveType trackType, const CAnimParamType& paramType, const AnimValueType animValue) const;
    IAnimTrack* CreateTrackInternalFloat(int trackType) const;

    // sets track animNode pointer to this node and sorts tracks
    void RegisterTrack(IAnimTrack* pTrack);

    CMovieSystem* GetCMovieSystem() const { return (CMovieSystem*)gEnv->pMovieSystem; }

    virtual bool NeedToRender() const { return false; }

    // nodes which support sounds should override this to reset their start/stop sound states
    virtual void ResetSounds() {}

    //////////////////////////////////////////////////////////////////////////
    // AnimateSound() calls ApplyAudioKey() to trigger audio on sound key frames. Nodes which support audio must override
    // this to trigger audio
    virtual void ApplyAudioKey(char const* const sTriggerName, bool const bPlay = true) {};
    void AnimateSound(std::vector<SSoundInfo>& nodeSoundInfo, SAnimContext& ec, IAnimTrack* pTrack, size_t numAudioTracks);
    //////////////////////////////////////////////////////////////////////////

    int m_refCount;
    AnimNodeType m_nodeType;
    int m_id;
    AZStd::string m_name;
    IAnimSequence* m_pSequence;
    IAnimNodeOwner* m_pOwner;
    IAnimNode* m_pParentNode;
    int m_nLoadedParentNodeId;  // only used in legacy Serialize()
    int m_parentNodeId;
    int m_flags;
    unsigned int m_bIgnoreSetParam : 1; // Internal flags.

    typedef AZStd::vector<AZStd::intrusive_ptr<IAnimTrack> > AnimTracks;
    AnimTracks m_tracks;
    bool m_expanded;

private:
    void SortTracks();
    bool IsTimeOnSoundKey(float queryTime) const;

    static bool TrackOrder(const AZStd::intrusive_ptr<IAnimTrack>& left, const AZStd::intrusive_ptr<IAnimTrack>& right);

    AZStd::mutex m_updateDynamicParamsLock;
};

#endif // CRYINCLUDE_CRYMOVIE_ANIMNODE_H
