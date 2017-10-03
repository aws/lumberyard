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

#pragma once

#include <LyShine/Animation/IUiAnimation.h>
#include "UiAnimationSystem.h"

/*!
        Base class for all Animation nodes,
        can host multiple animation tracks, and execute them other time.
        Animation node is reference counted.
 */
class CUiAnimNode
    : public IUiAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CUiAnimNode, AZ::SystemAllocator, 0)
    AZ_RTTI(CUiAnimNode, "{1ECF3B73-FCED-464D-82E8-CFAF31BB63DC}", IUiAnimNode);

    struct SParamInfo
    {
        SParamInfo()
            : name("")
            , valueType(eUiAnimValue_Float)
            , flags(ESupportedParamFlags(0)) {};
        SParamInfo(const char* _name, CUiAnimParamType _paramType, EUiAnimValue _valueType, ESupportedParamFlags _flags)
            : name(_name)
            , paramType(_paramType)
            , valueType(_valueType)
            , flags(_flags) {};

        const char* name;           // parameter name.
        CUiAnimParamType paramType;     // parameter id.
        EUiAnimValue valueType;       // value type, defines type of track to use for animating this parameter.
        ESupportedParamFlags flags; // combination of flags from ESupportedParamFlags.
    };

public:
    CUiAnimNode(const int id);
    CUiAnimNode();
    ~CUiAnimNode();

    //////////////////////////////////////////////////////////////////////////
    void add_ref() override;
    void release() override;
    //////////////////////////////////////////////////////////////////////////

    void SetName(const char* name) { m_name = name; };
    const char* GetName() { return m_name.c_str(); };

    void SetSequence(IUiAnimSequence* pSequence) { m_pSequence = pSequence; }
    // Return Animation Sequence that owns this node.
    IUiAnimSequence* GetSequence() { return m_pSequence; };

    void SetFlags(int flags);
    int GetFlags() const;

    IUiAnimationSystem* GetUiAnimationSystem() const { return m_pSequence->GetUiAnimationSystem(); };

    virtual void OnStart() {}
    virtual void OnReset() {}
    virtual void OnResetHard() { OnReset(); }
    virtual void OnPause() {}
    virtual void OnResume() {}
    virtual void OnStop() {}
    virtual void OnLoop() {}

    virtual Matrix34 GetReferenceMatrix() const;

    //////////////////////////////////////////////////////////////////////////
    bool IsParamValid(const CUiAnimParamType& paramType) const;
    virtual const char* GetParamName(const CUiAnimParamType& param) const;
    virtual EUiAnimValue GetParamValueType(const CUiAnimParamType& paramType) const;
    virtual IUiAnimNode::ESupportedParamFlags GetParamFlags(const CUiAnimParamType& paramType) const;
    virtual unsigned int GetParamCount() const { return 0; };

    bool SetParamValue(float time, CUiAnimParamType param, float val);
    bool SetParamValue(float time, CUiAnimParamType param, const Vec3& val);
    bool SetParamValue(float time, CUiAnimParamType param, const Vec4& val);
    bool GetParamValue(float time, CUiAnimParamType param, float& val);
    bool GetParamValue(float time, CUiAnimParamType param, Vec3& val);
    bool GetParamValue(float time, CUiAnimParamType param, Vec4& val);

    void SetTarget(IUiAnimNode* node) {};
    IUiAnimNode* GetTarget() const { return 0; };

    void StillUpdate() {}
    void Animate(SUiAnimContext& ec);

    virtual void PrecacheStatic(float startTime) {}
    virtual void PrecacheDynamic(float time) {}

    virtual void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);
    virtual void InitPostLoad(IUiAnimSequence* pSequence, bool remapIds, LyShine::EntityIdMap* entityIdMap);

    virtual void SetNodeOwner(IUiAnimNodeOwner* pOwner);
    virtual IUiAnimNodeOwner* GetNodeOwner() { return m_pOwner; };

    // Called by sequence when needs to activate a node.
    virtual void Activate(bool bActivate);

    //////////////////////////////////////////////////////////////////////////
    virtual void SetParent(IUiAnimNode* pParent);
    virtual IUiAnimNode* GetParent() const { return m_pParentNode; };
    virtual IUiAnimNode* HasDirectorAsParent() const;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Track functions.
    //////////////////////////////////////////////////////////////////////////
    virtual int  GetTrackCount() const;
    virtual IUiAnimTrack* GetTrackByIndex(int nIndex) const;
    virtual IUiAnimTrack* GetTrackForParameter(const CUiAnimParamType& paramType) const;
    virtual IUiAnimTrack* GetTrackForParameter(const CUiAnimParamType& paramType, uint32 index) const;

    virtual uint32 GetTrackParamIndex(const IUiAnimTrack* pTrack) const;

    IUiAnimTrack* GetTrackForAzField(const UiAnimParamData& param) const override { return nullptr; }
    IUiAnimTrack* CreateTrackForAzField(const UiAnimParamData& param) override { return nullptr; }

    virtual void SetTrack(const CUiAnimParamType& paramType, IUiAnimTrack* track);
    virtual IUiAnimTrack* CreateTrack(const CUiAnimParamType& paramType);
    virtual void SetTimeRange(Range timeRange);
    virtual void AddTrack(IUiAnimTrack* pTrack);
    virtual bool RemoveTrack(IUiAnimTrack* pTrack);
    virtual void SerializeUiAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);
    virtual void CreateDefaultTracks() {};
    //////////////////////////////////////////////////////////////////////////

    virtual void PostLoad();

    int GetId() const { return m_id; }
    const char* GetNameFast() const { return m_name.c_str(); }

    virtual void Render(){}

    virtual void UpdateDynamicParams() {}

    static void Reflect(AZ::SerializeContext* serializeContext);

protected:
    virtual bool GetParamInfoFromType(const CUiAnimParamType& paramType, SParamInfo& info) const { return false; };

    int  NumTracks() const { return (int)m_tracks.size(); }
    IUiAnimTrack* CreateTrackInternal(const CUiAnimParamType& paramType, EUiAnimCurveType trackType, EUiAnimValue valueType);

    IUiAnimTrack* CreateTrackInternalVector2(const CUiAnimParamType& paramType) const;
    IUiAnimTrack* CreateTrackInternalVector3(const CUiAnimParamType& paramType) const;
    IUiAnimTrack* CreateTrackInternalVector4(const CUiAnimParamType& paramType) const;
    IUiAnimTrack* CreateTrackInternalQuat(EUiAnimCurveType trackType, const CUiAnimParamType& paramType) const;
    IUiAnimTrack* CreateTrackInternalVector(EUiAnimCurveType trackType, const CUiAnimParamType& paramType, const EUiAnimValue animValue) const;
    IUiAnimTrack* CreateTrackInternalFloat(int trackType) const;
    UiAnimationSystem* GetUiAnimationSystemImpl() const { return (UiAnimationSystem*)GetUiAnimationSystem(); }

    virtual bool NeedToRender() const { return false; }

protected:
    int m_refCount;
    int m_id;
    AZStd::string m_name;
    IUiAnimSequence* m_pSequence;
    IUiAnimNodeOwner* m_pOwner;
    IUiAnimNode* m_pParentNode;
    int m_parentNodeId;
    int m_nLoadedParentNodeId;  // only used by old serialize
    int m_flags;
    unsigned int m_bIgnoreSetParam : 1; // Internal flags.

    typedef AZStd::vector<AZStd::intrusive_ptr<IUiAnimTrack> > AnimTracks;
    AnimTracks m_tracks;

private:
    static bool TrackOrder(const AZStd::intrusive_ptr<IUiAnimTrack>& left, const AZStd::intrusive_ptr<IUiAnimTrack>& right);
};

//////////////////////////////////////////////////////////////////////////
class CUiAnimNodeGroup
    : public CUiAnimNode
{
public:
    CUiAnimNodeGroup(const int id)
        : CUiAnimNode(id) { SetFlags(GetFlags() | eUiAnimNodeFlags_CanChangeName); }
    EUiAnimNodeType GetType() const { return eUiAnimNodeType_Group; }

    virtual CUiAnimParamType GetParamType(unsigned int nIndex) const { return eUiAnimParamType_Invalid; }
};
