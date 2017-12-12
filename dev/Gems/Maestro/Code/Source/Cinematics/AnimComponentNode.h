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

#ifndef CRYINCLUDE_CRYMOVIE_ANIMCOMPONENTNODE_H
#define CRYINCLUDE_CRYMOVIE_ANIMCOMPONENTNODE_H

#pragma once

#include "AnimNode.h"
#include "CharacterTrackAnimator.h"

/**
 * CAnimComponentNode
 *
 * All animation on AZ::Entity nodes are keyed to tracks on CAnimComponentNodes.
 *
 */

class CAnimComponentNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimComponentNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimComponentNode, "{722F3D0D-7AEB-46B7-BF13-D5C7A828E9BD}", CAnimNode);

    CAnimComponentNode(const int id);
    CAnimComponentNode();
    ~CAnimComponentNode();

    void SetEntityId(const int id) override {};     // legacy CryEntityId, not used here

    AZ::EntityId GetParentAzEntityId() const { return m_pParentNode ? m_pParentNode->GetAzEntityId() : AZ::EntityId(); }

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CAnimNode
    void Animate(SAnimContext& ac) override;
    
    void OnStart() override;
    void OnResume() override;

    void OnReset() override;
    void OnResetHard() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Overrides from IAnimNode
    // ComponentNodes use reflection for typing - return invalid for this pure virtual for the legacy system
    CAnimParamType GetParamType(unsigned int nIndex) const override { (void)nIndex; return eAnimParamType_Invalid; };

    void     SetComponent(AZ::ComponentId componentId, const AZ::Uuid& typeId) override;

    // returns the componentId of the component the node is associate with, if applicable, or a AZ::InvalidComponentId otherwise
    AZ::ComponentId GetComponentId() const override { return m_componentId; }

    int SetKeysForChangedTrackValues(float time) override;

    void OnStartPlayInEditor() override;
    void OnStopPlayInEditor() override;

    void SetNodeOwner(IAnimNodeOwner* pOwner) override;

    void SetPos(float time, const Vec3& pos) override;
    void SetRotate(float time, const Quat& quat) override;
    void SetScale(float time, const Vec3& scale) override;

    Vec3 GetPos() override;
    Quat GetRotate() override;
    Vec3 GetScale() override;

    ICharacterInstance* GetCharacterInstance() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Override CreateTrack to handle trackMultipliers for Component Tracks
    IAnimTrack* CreateTrack(const CAnimParamType& paramType) override;
    bool RemoveTrack(IAnimTrack* pTrack) override;

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    // search our Entity which contains the associated component for any component properties that are animated outside of behaviors
    void AppendNonBehaviorAnimatableProperties(IAnimNode::AnimParamInfos& animatableParams) const override;

    const AZ::Uuid& GetComponentTypeId() const { return m_componentTypeId; }

    // returns true if the given component type is animated outside of the behavior context by CAnimComponentNode
    static bool IsComponentAnimatedOutsideBehaviorContext(const AZ::Uuid& componentTypeId);

    // Skips Event Bus update of Components during animation - used for when another system is overriding a component's properties,
    // such as during camera interpolation between two Transforms. This will silently make the Animate() method do nothing - use only if you
    // know what you're doing!
    void SetSkipComponentAnimationUpdates(bool skipAnimationUpdates)
    {
        m_skipComponentAnimationUpdates = skipAnimationUpdates;
    }

    static void Reflect(AZ::SerializeContext* serializeContext);

protected:
    // functions involved in the process to parse and store component behavior context animated properties
    void UpdateDynamicParamsInternal() override;
    void UpdateDynamicParams_Editor();
    void UpdateDynamicParams_Game();
    
    void InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType) override;

    bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const override;


private:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // certain components, such as SimpleAnimation, are mapped to specialized types and handled withing CryMovie, as opposed
    // to using getters/setters in VirtualProperties
    const IAnimNode::AnimParamInfos& GetNonBehaviorAnimatedProperties(const AZ::Uuid& componentTypeId) const;

    // Utility function to query the units for a track and set the track multiplier if needed. Returns true if track multiplier was set.
    bool SetTrackMultiplier(IAnimTrack* track) const;

    // Stops any animation in SimpleAnimation components. Does nothing if this node is not connected to a SimpleAnimation component
    void StopComponentSimpleAnimations() const;

    void ForceAnimKeyChangeInCharacterTrackAnimator();

    // typed support functions for SetKeysForChangedTrackValues()
    int SetKeysForChangedBoolTrackValue(IAnimTrack* track, int keyIdx, float time);
    int SetKeysForChangedFloatTrackValue(IAnimTrack* track, int keyIdx, float time);
    int SetKeysForChangedVector3TrackValue(IAnimTrack* track, int keyIdx, float time, bool applyTrackMultiplier = true, float isChangedTolerance = AZ::g_simdTolerance);
    int SetKeysForChangedQuaternionTrackValue(IAnimTrack* track, int keyIdx, float time, float isChangedTolerance = AZ::g_simdTolerance);

    class BehaviorPropertyInfo
    {
    public:
        BehaviorPropertyInfo() {}
        BehaviorPropertyInfo(const string& name)
        {
            *this = name;
        }
        BehaviorPropertyInfo(const BehaviorPropertyInfo& other)
        {
            m_displayName = other.m_displayName;
            m_animNodeParamInfo.paramType = other.m_displayName;
            m_animNodeParamInfo.name = &m_displayName[0];
        }
        BehaviorPropertyInfo& operator=(const string& str)
        {
            // TODO: clean this up - this weird memory sharing was copied from legacy Cry - could be better.
            m_displayName = str;
            m_animNodeParamInfo.paramType = str;   // set type to eAnimParamType_ByString by assigning a string
            m_animNodeParamInfo.name = &m_displayName[0];
            return *this;
        }

        string     m_displayName;
        SParamInfo m_animNodeParamInfo;
    };
    
    void AddPropertyToParamInfoMap(const CAnimParamType& paramType);

    int m_refCount;     // intrusive_ptr ref counter

    AZ::Uuid                                m_componentTypeId;
    AZ::ComponentId                         m_componentId;

    // a mapping of CAnimParmTypes to SBehaviorPropertyInfo structs for each virtual property
    AZStd::unordered_map<CAnimParamType, BehaviorPropertyInfo>   m_paramTypeToBehaviorPropertyInfoMap;

    // a mapping of component type ID's to specialized param types (such as animation). If a component is not in this map,
    // eAnimParamType_ByString is used with the Virtual Property name as the string value
    static AZStd::unordered_map<AZ::Uuid, IAnimNode::AnimParamInfos> s_componentTypeToNonBehaviorPropertiesMap;

    // used to return a reference to an empty vector
    static IAnimNode::AnimParamInfos s_emptyAnimatableProperties;

    // helper class responsible for animating Character Tracks (aka 'Animation' tracks in the TrackView UI)
    CCharacterTrackAnimator*   m_characterTrackAnimator = nullptr;

    bool m_skipComponentAnimationUpdates;
};
#endif // CRYINCLUDE_CRYMOVIE_ANIMCOMPONENTNODE_H
