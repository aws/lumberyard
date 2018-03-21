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

#include "Maestro_precompiled.h"
#include "AnimComponentNode.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>
#include <AzFramework/Components/TransformComponent.h>
#include <LmbrCentral/Animation/CharacterAnimationBus.h>
#include <LmbrCentral/Animation/SimpleAnimationComponentBus.h>
#include <Maestro/Bus/EditorSequenceComponentBus.h>
#include <Maestro/Bus/SequenceComponentBus.h>
#include <Maestro/Types/AnimNodeType.h>
#include <Maestro/Types/AnimValueType.h>
#include <Maestro/Types/AnimParamType.h>

#include "CharacterTrack.h"

/*static*/ AZStd::unordered_map<AZ::Uuid, IAnimNode::AnimParamInfos> CAnimComponentNode::s_componentTypeToNonBehaviorPropertiesMap;
/*static*/ IAnimNode::AnimParamInfos CAnimComponentNode::s_emptyAnimatableProperties;
/*static*/ bool CAnimComponentNode::IsComponentAnimatedOutsideBehaviorContext(const AZ::Uuid& componentTypeId)
{
    return (s_componentTypeToNonBehaviorPropertiesMap.find(componentTypeId) != s_componentTypeToNonBehaviorPropertiesMap.end());
}

CAnimComponentNode::CAnimComponentNode(int id)
    : CAnimNode(id, AnimNodeType::Component)
    , m_refCount(0)
    , m_componentTypeId(AZ::Uuid::CreateNull())
    , m_componentId(AZ::InvalidComponentId)
    , m_skipComponentAnimationUpdates(false)
{
    if (s_componentTypeToNonBehaviorPropertiesMap.empty())
    {
        // Register all animatable properties that are handled outside of Behavior Contexts - this is a back-door 'hack' that animates
        // data outside of the component's behavior, effectively 'short-circuiting' it.

        // SimpleAnimation Component specialized params
        SParamInfo simpleAnimationParamInfo;

        simpleAnimationParamInfo.paramType = "Animation";                   // intialize as a eAnimParam_byString to set the name for the param
        simpleAnimationParamInfo.paramType = AnimParamType::Animation;      // this sets the type but leaves the name
        simpleAnimationParamInfo.valueType = AnimValueType::CharacterAnim;
        simpleAnimationParamInfo.flags = (IAnimNode::ESupportedParamFlags)eSupportedParamFlags_MultipleTracks;

        IAnimNode::AnimParamInfos simpleAnimationProperties;
        simpleAnimationProperties.push_back(simpleAnimationParamInfo);

        s_componentTypeToNonBehaviorPropertiesMap[SimpleAnimationComponentTypeId] = simpleAnimationProperties;
        s_componentTypeToNonBehaviorPropertiesMap[EditorSimpleAnimationComponentTypeId] = simpleAnimationProperties;
    }
}

CAnimComponentNode::CAnimComponentNode()
    : CAnimComponentNode(0)
{
}

CAnimComponentNode::~CAnimComponentNode()
{
    if (m_characterTrackAnimator)
    {
        delete m_characterTrackAnimator;
        m_characterTrackAnimator = nullptr;
    }
}

void CAnimComponentNode::OnStart()
{
    // This is for AI/Physics Sim and Game modes, we want to stop any SimpleAnimation component animations that may be playing on the entity
    StopComponentSimpleAnimations();
}

void CAnimComponentNode::OnResume()
{
    // This is for AI/Physics Sim and Game modes, we want to stop any SimpleAnimation component animations that may be playing on the entity
    StopComponentSimpleAnimations();
}

void CAnimComponentNode::StopComponentSimpleAnimations() const
{
    // if we're connected to a EditorSimpleAnimationComponent node and we have an Animation Track, stop all animations on that component
    if ((m_componentTypeId == EditorSimpleAnimationComponentTypeId || m_componentTypeId == SimpleAnimationComponentTypeId) && GetTrackForParameter(CAnimParamType(AnimParamType::Animation)))
    {
        LmbrCentral::SimpleAnimationComponentRequestBus::Event(GetParentAzEntityId(), &LmbrCentral::SimpleAnimationComponentRequestBus::Events::StopAllAnimations);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::OnReset()
{
    // OnReset is called when sequences are loaded
    if (m_characterTrackAnimator)
    {
        m_characterTrackAnimator->OnReset(this);
    }
    UpdateDynamicParams();
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::OnResetHard()
{
    OnReset();
    if (m_pOwner)
    {
        m_pOwner->OnNodeReset(this);
    }
}

//////////////////////////////////////////////////////////////////////////
CAnimParamType CAnimComponentNode::GetParamType(unsigned int nIndex) const
{ 
    (void)nIndex; 
    return AnimParamType::Invalid;
};

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::SetComponent(AZ::ComponentId componentId, const AZ::Uuid& componentTypeId)
{
    m_componentId = componentId;
    m_componentTypeId = componentTypeId;

    // call OnReset() to update dynamic params 
    // (i.e. virtual properties from the exposed EBuses from the BehaviorContext)
    OnReset();
}

//////////////////////////////////////////////////////////////////////////
bool CAnimComponentNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    auto findIter = m_paramTypeToBehaviorPropertyInfoMap.find(paramId);
    if (findIter != m_paramTypeToBehaviorPropertyInfoMap.end())
    {
        info = findIter->second.m_animNodeParamInfo;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
const IAnimNode::AnimParamInfos& CAnimComponentNode::GetNonBehaviorAnimatedProperties(const AZ::Uuid& componentTypeId) const
{
    auto findIter = s_componentTypeToNonBehaviorPropertiesMap.find(componentTypeId);
    if (findIter != s_componentTypeToNonBehaviorPropertiesMap.end())
    {
        return findIter->second;
    }
    return s_emptyAnimatableProperties;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimComponentNode::SetTrackMultiplier(IAnimTrack* track) const
{
    bool trackMultiplierWasSet = false;

    CAnimParamType paramType(track->GetParameterType());

    if (paramType.GetType() == AnimParamType::ByString)
    {
        // check to see if we need to use a track multiplier

        // Get Property TypeId
        Maestro::SequenceComponentRequests::AnimatablePropertyAddress propertyAddress(m_componentId, paramType.GetName());
        AZ::Uuid propertyTypeId = AZ::Uuid::CreateNull();
        Maestro::SequenceComponentRequestBus::EventResult(propertyTypeId, m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedAddressTypeId,
            GetParentAzEntityId(), propertyAddress);

        if (propertyTypeId == AZ::Color::TYPEINFO_Uuid())
        {
            track->SetMultiplier(255.0f);
            trackMultiplierWasSet = true;
        }
    }

    return trackMultiplierWasSet;
}

int CAnimComponentNode::SetKeysForChangedBoolTrackValue(IAnimTrack* track, int keyIdx, float time)
{
    int retNumKeysSet = 0;
    bool currTrackValue;
    track->GetValue(time, currTrackValue);
    Maestro::SequenceComponentRequests::AnimatedBoolValue currValue(currTrackValue);
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, track->GetParameterType().GetName());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, currValue, GetParentAzEntityId(), animatableAddress);

    if (currTrackValue != currValue.GetBoolValue())
    {
        keyIdx = track->FindKey(time);
        if (keyIdx == -1)
        {
            keyIdx = track->CreateKey(time);
        }

        // no need to set a value of a Bool key - it's existence implies a Boolean toggle.
        retNumKeysSet++;
    }
    return retNumKeysSet;
}

int CAnimComponentNode::SetKeysForChangedFloatTrackValue(IAnimTrack* track, int keyIdx, float time)
{
    int retNumKeysSet = 0;
    float currTrackValue;
    track->GetValue(time, currTrackValue);
    Maestro::SequenceComponentRequests::AnimatedFloatValue currValue(currTrackValue);
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, track->GetParameterType().GetName());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, currValue, GetParentAzEntityId(), animatableAddress);

    if (currTrackValue != currValue.GetFloatValue())
    {
        keyIdx = track->FindKey(time);
        if (keyIdx == -1)
        {
            keyIdx = track->CreateKey(time);
        }

        if (track->GetValueType() == AnimValueType::DiscreteFloat)
        {
            IDiscreteFloatKey key;
            track->GetKey(keyIdx, &key);
            key.SetValue(currValue.GetFloatValue());
        }
        else
        {
            I2DBezierKey key;
            track->GetKey(keyIdx, &key);
            key.value.y = currValue.GetFloatValue();
            track->SetKey(keyIdx, &key);
        }
        retNumKeysSet++;
    }
    return retNumKeysSet;
}

int CAnimComponentNode::SetKeysForChangedVector3TrackValue(IAnimTrack* track, int keyIdx, float time, bool applyTrackMultiplier, float isChangedTolerance)
{
    int retNumKeysSet = 0;
    AZ::Vector3 currTrackValue;
    track->GetValue(time, currTrackValue, applyTrackMultiplier);
    Maestro::SequenceComponentRequests::AnimatedVector3Value currValue(currTrackValue);
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, track->GetParameterType().GetName());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, currValue, GetParentAzEntityId(), animatableAddress);
    AZ::Vector3 currVector3Value;
    currValue.GetValue(currVector3Value);
    if (!currTrackValue.IsClose(currVector3Value, isChangedTolerance))
    {
        // track will be a CCompoundSplineTrack. For these we can simply call SetValue at the and keys will be added if needed.
        track->SetValue(time, currVector3Value, false, applyTrackMultiplier);
        retNumKeysSet++;    // we treat the compound vector as a single key for simplicity and speed - if needed, we can go through each component and count them up if this is important.
    }
    return retNumKeysSet;
}

int CAnimComponentNode::SetKeysForChangedQuaternionTrackValue(IAnimTrack* track, int keyIdx, float time)
{
    int retNumKeysSet = 0;
    AZ::Quaternion currTrackValue;
    track->GetValue(time, currTrackValue);
    Maestro::SequenceComponentRequests::AnimatedQuaternionValue currValue(currTrackValue);
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, track->GetParameterType().GetName());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, currValue, GetParentAzEntityId(), animatableAddress);
    AZ::Quaternion currQuaternionValue;
    currValue.GetValue(currQuaternionValue);

    if (!currTrackValue.IsClose(currQuaternionValue))
    {
        // track will be a CCompoundSplineTrack. For these we can simply call SetValue at the and keys will be added if needed.
        track->SetValue(time, currQuaternionValue, false);
        retNumKeysSet++;    // we treat the compound vector as a single key for simplicity and speed - if needed, we can go through each component and count them up if this is important.
    }
    return retNumKeysSet;
}

//////////////////////////////////////////////////////////////////////////
int CAnimComponentNode::SetKeysForChangedTrackValues(float time)
{
    int retNumKeysSet = 0;

    for (int i = GetTrackCount(); --i >= 0;)
    {
        IAnimTrack* track = GetTrackByIndex(i);
        int keyIdx = -1;

        switch (track->GetValueType())
        {
            case AnimValueType::Bool:
                retNumKeysSet += SetKeysForChangedBoolTrackValue(track, keyIdx, time);
                break;
            case AnimValueType::Float:
            case AnimValueType::DiscreteFloat:
                retNumKeysSet += SetKeysForChangedFloatTrackValue(track, keyIdx, time);
                break;
            case AnimValueType::RGB:
                retNumKeysSet += SetKeysForChangedVector3TrackValue(track, keyIdx, time, true, (1.0f) / 255.0f);
                break;
            case AnimValueType::Vector:
                retNumKeysSet += SetKeysForChangedVector3TrackValue(track, keyIdx, time, true);
                break;
            case AnimValueType::Quat:
                retNumKeysSet += SetKeysForChangedQuaternionTrackValue(track, keyIdx, time);
                break;
            case AnimValueType::Vector4:
                AZ_Warning("TrackView", false, "Vector4's are not supported for recording.");
                break;
        }
    }

    return retNumKeysSet;
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::OnStartPlayInEditor()
{
    // reset key states for entering AI/Physics SIM mode
    ForceAnimKeyChangeInCharacterTrackAnimator();
}

void CAnimComponentNode::OnStopPlayInEditor()
{
    // reset key states for returning to Editor mode
    ForceAnimKeyChangeInCharacterTrackAnimator();
}

void CAnimComponentNode::SetNodeOwner(IAnimNodeOwner* pOwner)
{
    CAnimNode::SetNodeOwner(pOwner);
    if (pOwner && gEnv->IsEditor())
    {
        // SetNodeOwner is called when a node is added on undo/redo - we have to update dynamic params in such a case
        UpdateDynamicParams();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::GetParentWorldTransform(AZ::Transform& retTransform) const
{
    AZ::EntityId parentId;
    AZ::TransformBus::EventResult(parentId, GetParentAzEntityId(), &AZ::TransformBus::Events::GetParentId);

    if (parentId.IsValid())
    {
        AZ::TransformBus::EventResult(retTransform, parentId, &AZ::TransformBus::Events::GetWorldTM);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::ConvertBetweenWorldAndLocalPosition(Vec3& position, ETransformSpaceConversionDirection conversionDirection) const
{
    AZ::Vector3 pos(position.x, position.y, position.z);
    AZ::Transform parentTransform = AZ::Transform::Identity();

    GetParentWorldTransform(parentTransform);
    if (conversionDirection == eTransformConverstionDirection_toLocalSpace)
    {
        parentTransform.InvertFast();
    }
    pos = parentTransform * pos;

    position.Set(pos.GetX(), pos.GetY(), pos.GetZ());
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::ConvertBetweenWorldAndLocalRotation(Quat& rotation, ETransformSpaceConversionDirection conversionDirection) const
{
    AZ::Quaternion rot(rotation.v.x, rotation.v.y, rotation.v.z, rotation.w);
    AZ::Transform rotTransform = AZ::Transform::CreateFromQuaternion(rot);
    rotTransform.ExtractScale();

    AZ::Transform parentTransform = AZ::Transform::Identity();
    GetParentWorldTransform(parentTransform);
    if (conversionDirection == eTransformConverstionDirection_toLocalSpace)
    {
        parentTransform.InvertFast();
    }

    rotTransform = parentTransform * rotTransform;
    rot = AZ::Quaternion::CreateFromTransform(rotTransform);

    rotation = Quat(rot);
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::ConvertBetweenWorldAndLocalScale(Vec3& scale, ETransformSpaceConversionDirection conversionDirection) const
{
    AZ::Transform parentTransform = AZ::Transform::Identity();
    AZ::Transform scaleTransform = AZ::Transform::CreateScale(AZ::Vector3(scale.x, scale.y, scale.z));

    GetParentWorldTransform(parentTransform);
    if (conversionDirection == eTransformConverstionDirection_toLocalSpace)
    {
        parentTransform.InvertFast();
    }
    scaleTransform = parentTransform * scaleTransform;

    AZ::Vector3 vScale = scaleTransform.RetrieveScale();
    scale.Set(vScale.GetX(), vScale.GetY(), vScale.GetZ());
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::SetPos(float time, const Vec3& pos)
{
    if (m_componentTypeId == AZ::Uuid(ToolsTransformComponentTypeId) || m_componentTypeId == AzFramework::TransformComponent::TYPEINFO_Uuid())
    {
        bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (GetParent()->GetFlags() & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded

        IAnimTrack* posTrack = GetTrackForParameter(AnimParamType::Position);
        if (posTrack)
        {
            // pos is in world position, even if the entity is parented - because Component Entity AZ::Transforms do not correctly set
            // CBaseObject parenting. This should probably be fixed, but for now, we explicitly change from World to Local space here
            Vec3 localPos(pos);
            ConvertBetweenWorldAndLocalPosition(localPos, eTransformConverstionDirection_toLocalSpace);

            posTrack->SetValue(time, localPos, bDefault);
        }

        if (!bDefault)
        {
            GetCMovieSystem()->Callback(IMovieCallback::CBR_CHANGETRACK, this);
        }
    }
}

Vec3 CAnimComponentNode::GetPos()
{
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, "Position");
    Maestro::SequenceComponentRequests::AnimatedVector3Value posValue(AZ::Vector3::CreateZero());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, posValue, GetParentAzEntityId(), animatableAddress);

    // Always return world position because Component Entity AZ::Transforms do not correctly set
    // CBaseObject parenting. This should probably be fixed, but for now, we explicitly change from Local to World space here.
    Vec3 worldPos(posValue.GetVector3Value());
    ConvertBetweenWorldAndLocalPosition(worldPos, eTransformConverstionDirection_toWorldSpace);

    return worldPos;
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::SetRotate(float time, const Quat& rotation)
{
    if (m_componentTypeId == AZ::Uuid(ToolsTransformComponentTypeId) || m_componentTypeId == AzFramework::TransformComponent::TYPEINFO_Uuid())
    {
        bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (GetParent()->GetFlags() & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded

        IAnimTrack* rotTrack = GetTrackForParameter(AnimParamType::Rotation);
        if (rotTrack)
        {
            // Rotation is in world space, even if the entity is parented - because Component Entity AZ::Transforms do not correctly set
            // CBaseObject parenting, so we convert it to Local space here. This should probably be fixed, but for now, we explicitly change from World to Local space here.
            Quat localRot(rotation);
            ConvertBetweenWorldAndLocalRotation(localRot, eTransformConverstionDirection_toLocalSpace);
            rotTrack->SetValue(time, localRot, bDefault);
        }

        if (!bDefault)
        {
            GetCMovieSystem()->Callback(IMovieCallback::CBR_CHANGETRACK, this);
        }
    }
}

Quat CAnimComponentNode::GetRotate(float time)
{
    Quat worldRot;

    // If there is rotation track data, get the rotation from there.
    // Otherwise just use the current entity rotation value.
    IAnimTrack* rotTrack = GetTrackForParameter(AnimParamType::Rotation);
    if (rotTrack != nullptr && rotTrack->GetNumKeys() > 0)
    {
        rotTrack->GetValue(time, worldRot);
    }
    else
    {
        worldRot = GetRotate();
    }

    return worldRot;
}

Quat CAnimComponentNode::GetRotate()
{
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, "Rotation");
    Maestro::SequenceComponentRequests::AnimatedQuaternionValue rotValue(AZ::Quaternion::CreateIdentity());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, rotValue, GetParentAzEntityId(), animatableAddress);

    // Always return world rotation because Component Entity AZ::Transforms do not correctly set
    // CBaseObject parenting. This should probably be fixed, but for now, we explicitly change from Local to World space here.
    Quat worldRot(rotValue.GetQuaternionValue());
    ConvertBetweenWorldAndLocalRotation(worldRot, eTransformConverstionDirection_toWorldSpace);

    return worldRot;
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::SetScale(float time, const Vec3& scale)
{
    if (m_componentTypeId == AZ::Uuid(ToolsTransformComponentTypeId) || m_componentTypeId == AzFramework::TransformComponent::TYPEINFO_Uuid())
    {
        bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (GetParent()->GetFlags() & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded

        IAnimTrack* scaleTrack = GetTrackForParameter(AnimParamType::Scale);
        if (scaleTrack)
        {
            // Scale is in World space, even if the entity is parented - because Component Entity AZ::Transforms do not correctly set
            // CBaseObject parenting, so we convert it to Local space here. This should probably be fixed, but for now, we explicitly change from World to Local space here.
            Vec3 localScale(scale);
            ConvertBetweenWorldAndLocalScale(localScale, eTransformConverstionDirection_toLocalSpace);
            scaleTrack->SetValue(time, localScale, bDefault);
        }

        if (!bDefault)
        {
            GetCMovieSystem()->Callback(IMovieCallback::CBR_CHANGETRACK, this);
        }
    }
}

Vec3 CAnimComponentNode::GetScale()
{
    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, "Scale");
    Maestro::SequenceComponentRequests::AnimatedVector3Value scaleValue(AZ::Vector3::CreateZero());
    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, scaleValue, GetParentAzEntityId(), animatableAddress);

    // Always return World scale because Component Entity AZ::Transforms do not correctly set
    // CBaseObject parenting. This should probably be fixed, but for now, we explicitly change from Local to World space here.
    Vec3 worldScale(scaleValue.GetVector3Value());
    ConvertBetweenWorldAndLocalScale(worldScale, eTransformConverstionDirection_toWorldSpace);

    return worldScale;
}

///////////////////////////////////////////////////////////////////////////
ICharacterInstance* CAnimComponentNode::GetCharacterInstance()
{
    ICharacterInstance* retCharInstance = nullptr;
    LmbrCentral::CharacterAnimationRequestBus::EventResult(retCharInstance, GetParentAzEntityId(), &LmbrCentral::CharacterAnimationRequestBus::Events::GetCharacterInstance);
    return retCharInstance;
}

///////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::ForceAnimKeyChangeInCharacterTrackAnimator()
{
    if (m_characterTrackAnimator)
    {
        IAnimTrack* animTrack = GetTrackForParameter(AnimParamType::Animation);
        if (animTrack && animTrack->HasKeys())
        {
            // resets anim key change states so animation will update correctly on the next Animate()
            m_characterTrackAnimator->ForceAnimKeyChange();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CAnimComponentNode::CreateTrack(const CAnimParamType& paramType)
{
    IAnimTrack* retTrack = CAnimNode::CreateTrack(paramType);

    if (retTrack)
    {
        SetTrackMultiplier(retTrack);
        if (paramType.GetType() == AnimParamType::Animation && !m_characterTrackAnimator)
        {
            m_characterTrackAnimator = new CCharacterTrackAnimator();
        }
    }

    return retTrack;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimComponentNode::RemoveTrack(IAnimTrack* pTrack)
{
    if (pTrack && pTrack->GetParameterType().GetType() == AnimParamType::Animation && m_characterTrackAnimator)
    {
        delete m_characterTrackAnimator;
        m_characterTrackAnimator = nullptr;
    }

    return CAnimNode::RemoveTrack(pTrack);
}

//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
void CAnimComponentNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
    if (bLoading)
    {
        XmlString uuidString;

        xmlNode->getAttr("ComponentId", m_componentId);
        if (xmlNode->getAttr("ComponentTypeId", uuidString))
        {
            m_componentTypeId = uuidString.c_str();
        }
        else
        {
            m_componentTypeId = AZ::Uuid::CreateNull();
        }
    }
    else
    {
        // saving
        char uuidStringBuf[AZ::Uuid::MaxStringBuffer] = { 0 };

        xmlNode->setAttr("ComponentId", m_componentId);
        m_componentTypeId.ToString(uuidStringBuf, AZ::Uuid::MaxStringBuffer);
        xmlNode->setAttr("ComponentTypeId", uuidStringBuf);
    }
}


//////////////////////////////////////////////////////////////////////////
// Property Value types are detected in this function
void CAnimComponentNode::AddPropertyToParamInfoMap(const CAnimParamType& paramType)
{
    BehaviorPropertyInfo propertyInfo;                  // the default value type is AnimValueType::Float

    // Check first if the paramType is handled outside of Component Behaviors, covered in s_componentTypeToNonBehaviorPropertiesMap
    // Add any such parameters for this component
    auto findIter = s_componentTypeToNonBehaviorPropertiesMap.find(m_componentTypeId);
    if (findIter != s_componentTypeToNonBehaviorPropertiesMap.end())
    {
        for (int i = 0; i < findIter->second.size(); i++)
        {
            const SParamInfo& paramInfo = findIter->second[i];
            if (paramInfo.paramType == paramType)
            {
                propertyInfo = paramInfo.paramType.GetName();   // sets up the name (which is the track name) from s_componentTypeToNonBehaviorPropertiesMap
                propertyInfo.m_animNodeParamInfo.valueType = paramInfo.valueType;
                propertyInfo.m_animNodeParamInfo.flags = paramInfo.flags;
                propertyInfo.m_animNodeParamInfo.paramType = paramInfo.paramType;
            }
        }
    }
    else
    {
        // property is handled by Component animation (Behavior Context getter/setters). Regardless of the param Type, it must have a non-empty name
        // (the VirtualProperty name)
        AZ_Assert(paramType.GetName() && strlen(paramType.GetName()), "All AnimParamTypes animated on Components must have a name for its VirtualProperty");

        // Initialize property name string, which sets to AnimParamType::ByString by default
        propertyInfo = paramType.GetName();

        if (paramType.GetType() != AnimParamType::ByString)
        {
            // This sets the eAnimParamType enumeration but leaves the string name untouched
            propertyInfo.m_animNodeParamInfo.paramType = paramType.GetType();
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////
        //! Detect the value type from reflection in the Behavior Context
        //
        // Query the property type Id from the Sequence Component and set it if a supported type is found
        AZ::Uuid propertyTypeId = AZ::Uuid::CreateNull();
        Maestro::SequenceComponentRequests::AnimatablePropertyAddress propertyAddress(m_componentId, propertyInfo.m_displayName.c_str());

        Maestro::SequenceComponentRequestBus::EventResult(propertyTypeId, m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedAddressTypeId,
            GetParentAzEntityId(), propertyAddress);

        if (propertyTypeId == AZ::Vector3::TYPEINFO_Uuid())
        {
            propertyInfo.m_animNodeParamInfo.valueType = AnimValueType::Vector;
        }
        else if (propertyTypeId == AZ::Color::TYPEINFO_Uuid())
        {
            propertyInfo.m_animNodeParamInfo.valueType = AnimValueType::RGB;
        }
        else if (propertyTypeId == AZ::Quaternion::TYPEINFO_Uuid())
        {
            propertyInfo.m_animNodeParamInfo.valueType = AnimValueType::Quat;
        }
        else if (propertyTypeId == AZ::AzTypeInfo<bool>::Uuid())
        {
            propertyInfo.m_animNodeParamInfo.valueType = AnimValueType::Bool;
        }
        // the fall-through default type is propertyInfo.m_animNodeParamInfo.valueType = AnimValueType::Float
    }

    m_paramTypeToBehaviorPropertyInfoMap[paramType] = propertyInfo;
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::AppendNonBehaviorAnimatableProperties(IAnimNode::AnimParamInfos& animatableParams) const
{
    // retrieve any non-behavior animated properties for our component type and append them
    const IAnimNode::AnimParamInfos& nonBehaviorParams = GetNonBehaviorAnimatedProperties(m_componentTypeId);
    for (int i = 0; i < nonBehaviorParams.size(); i++)
    {
        animatableParams.push_back(nonBehaviorParams[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CAnimComponentNode, CAnimNode>()
        ->Version(1)
        ->Field("ComponentID", &CAnimComponentNode::m_componentId)
        ->Field("ComponentTypeID", &CAnimComponentNode::m_componentTypeId);
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::UpdateDynamicParams_Editor()
{
    IAnimNode::AnimParamInfos animatableParams;

    // add all parameters supported by the component
    Maestro::EditorSequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::EditorSequenceComponentRequestBus::Events::GetAllAnimatablePropertiesForComponent, 
                                                          animatableParams, GetParentAzEntityId(), m_componentId);

    // add any additional non-behavior context properties we handle ourselves
    AppendNonBehaviorAnimatableProperties(animatableParams);

    for (int i = 0; i < animatableParams.size(); i++)
    {
        AddPropertyToParamInfoMap(animatableParams[i].paramType);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::UpdateDynamicParams_Game()
{
    // Fill m_paramTypeToBehaviorPropertyInfoMap based on our animated tracks
    for (uint32 i = 0; i < m_tracks.size(); ++i)
    {
        AddPropertyToParamInfoMap(m_tracks[i]->GetParameterType());
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::UpdateDynamicParamsInternal()
{
    m_paramTypeToBehaviorPropertyInfoMap.clear();

    // editor stores *all* properties of *every* entity used in an AnimEntityNode.
    // In pure game mode we just need to store the properties that we know are going to be used in a track, so we can save a lot of memory.
    if (gEnv->IsEditor() && !gEnv->IsEditorSimulationMode() && !gEnv->IsEditorGameMode())
    {
        UpdateDynamicParams_Editor();
    }
    else
    {
        UpdateDynamicParams_Game();
    }

    // Go through all tracks and set Multipliers if required
    for (uint32 i = 0; i < m_tracks.size(); ++i)
    {
        AZStd::intrusive_ptr<IAnimTrack> track = m_tracks[i];
        SetTrackMultiplier(track.get());
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType)
{
    // Initialize new track to property value
    if (paramType.GetType() == AnimParamType::ByString && pTrack)
    {
        auto findIter = m_paramTypeToBehaviorPropertyInfoMap.find(paramType);
        if (findIter != m_paramTypeToBehaviorPropertyInfoMap.end())
        {
            BehaviorPropertyInfo& propertyInfo = findIter->second;

            bool boolValue = false;
            Maestro::SequenceComponentRequests::AnimatablePropertyAddress address(m_componentId, propertyInfo.m_animNodeParamInfo.name);

            switch (pTrack->GetValueType())
            {
                case AnimValueType::Float:
                {
                    Maestro::SequenceComponentRequests::AnimatedFloatValue defaultValue(.0f);

                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, defaultValue, GetParentAzEntityId(), address);
                    pTrack->SetValue(0, defaultValue.GetFloatValue(), true);
                    break;
                }
                case AnimValueType::Vector:
                {
                    Maestro::SequenceComponentRequests::AnimatedVector3Value defaultValue(AZ::Vector3::CreateZero());
                    AZ::Vector3 vector3Value = AZ::Vector3::CreateZero();

                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, defaultValue, GetParentAzEntityId(), address);
                    defaultValue.GetValue(vector3Value);

                    pTrack->SetValue(0, Vec3(vector3Value.GetX(), vector3Value.GetY(), vector3Value.GetZ()), true);
                    break;
                }
                case AnimValueType::Quat:
                {
                    Maestro::SequenceComponentRequests::AnimatedQuaternionValue defaultValue(AZ::Quaternion::CreateIdentity());
                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, defaultValue, GetParentAzEntityId(), address);
                    pTrack->SetValue(0, Quat(defaultValue.GetQuaternionValue()), true);
                    break;
                }
                case AnimValueType::RGB:
                {
                    Maestro::SequenceComponentRequests::AnimatedVector3Value defaultValue(AZ::Vector3::CreateOne());
                    AZ::Vector3 vector3Value = AZ::Vector3::CreateOne();

                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, defaultValue, GetParentAzEntityId(), address);
                    defaultValue.GetValue(vector3Value);
                    
                    pTrack->SetValue(0, Vec3(clamp_tpl((float)vector3Value.GetX(), .0f, 1.0f), clamp_tpl((float)vector3Value.GetY(), .0f, 1.0f), clamp_tpl((float)vector3Value.GetZ(), .0f, 1.0f)), /*setDefault=*/ true, /*applyMultiplier=*/ true);
                    break;
                }
                case AnimValueType::Bool:
                {
                    Maestro::SequenceComponentRequests::AnimatedBoolValue defaultValue(true);

                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, defaultValue, GetParentAzEntityId(), address);

                    pTrack->SetValue(0, defaultValue.GetBoolValue(), true);
                    break;
                }
                default:
                {
                    AZ_Warning("TrackView", false, "Unsupported value type requested for Component Node Track %s, skipping...", paramType.GetName());
                    break;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimComponentNode::Animate(SAnimContext& ac)
{
    if (m_skipComponentAnimationUpdates)
    {
        return;
    }

    // Evaluate all tracks

    // indices used for character animation (SimpleAnimationComponent)
    int characterAnimationLayer = 0;
    int characterAnimationTrackIdx = 0;

    int trackCount = NumTracks();
    for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
    {
        CAnimParamType paramType = m_tracks[paramIndex]->GetParameterType();
        IAnimTrack* pTrack = m_tracks[paramIndex].get();

        if ((pTrack->HasKeys() == false) || (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled) || pTrack->IsMasked(ac.trackMask))
        {
            continue;
        }

        if (!ac.bResetting)
        {
            if (paramType.GetType() == AnimParamType::Animation)
            {
                // special handling for Character Animation. We short-circuit the SimpleAnimation behavior using m_characterTrackAnimator
                if (!m_characterTrackAnimator)
                {
                    m_characterTrackAnimator = new CCharacterTrackAnimator;
                }
                
                // bForceEntityActivation = true;   // for legacy entities, we forced IEntity::Activate() for cut-scenes to "solve problems on first frame entity becomes visible because it won't be active.
                // not sure if this is needed for AZ::Entities. See EntityNode::Animate() for legacy implmementation

                if (characterAnimationLayer < MAX_CHARACTER_TRACKS + ADDITIVE_LAYERS_OFFSET)
                {
                    int index = characterAnimationLayer;
                    CCharacterTrack* pCharTrack = (CCharacterTrack*)pTrack;
                    if (pCharTrack->GetAnimationLayerIndex() >= 0)   // If the track has an animation layer specified,
                    {
                        assert(pCharTrack->GetAnimationLayerIndex() < ISkeletonAnim::LayerCount);
                        index = pCharTrack->GetAnimationLayerIndex();  // use it instead.
                    }

                    m_characterTrackAnimator->AnimateTrack(pCharTrack, ac, index, characterAnimationTrackIdx);

                    if (characterAnimationLayer == 0)
                    {
                        characterAnimationLayer += ADDITIVE_LAYERS_OFFSET;
                    }
                    ++characterAnimationLayer;
                    ++characterAnimationTrackIdx;
                }   
            }
            else
            {
                // handle all other non-specialized Components
                auto findIter = m_paramTypeToBehaviorPropertyInfoMap.find(paramType);
                if (findIter != m_paramTypeToBehaviorPropertyInfoMap.end())
                {
                    BehaviorPropertyInfo& propertyInfo = findIter->second;
                    Maestro::SequenceComponentRequests::AnimatablePropertyAddress animatableAddress(m_componentId, propertyInfo.m_animNodeParamInfo.name);

                    switch (pTrack->GetValueType())
                    {
                        case AnimValueType::Float:
                        {
                            if (pTrack->HasKeys())
                            {
                                float floatValue = .0f;
                                pTrack->GetValue(ac.time, floatValue, /*applyMultiplier= */ true);
                                Maestro::SequenceComponentRequests::AnimatedFloatValue value(floatValue);

                                Maestro::SequenceComponentRequests::AnimatedFloatValue prevValue(floatValue);
                                Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevValue, GetParentAzEntityId(), animatableAddress);
                                if (!value.IsClose(prevValue))
                                {
                                    // only set the value if it's changed
                                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), animatableAddress, value);
                                }
                            }
                            break;
                        }
                        case AnimValueType::Vector:     // fall-through
                        case AnimValueType::RGB:
                        {
                            float tolerance = AZ::g_fltEps;
                            Vec3 vec3Value(.0f, .0f, .0f);
                            pTrack->GetValue(ac.time, vec3Value, /*applyMultiplier= */ true);
                            AZ::Vector3 vector3Value(vec3Value.x, vec3Value.y, vec3Value.z);

                            if (pTrack->GetValueType() == AnimValueType::RGB)
                            {
                                vec3Value.x = clamp_tpl(vec3Value.x, 0.0f, 1.0f);
                                vec3Value.y = clamp_tpl(vec3Value.y, 0.0f, 1.0f);
                                vec3Value.z = clamp_tpl(vec3Value.z, 0.0f, 1.0f);

                                // set tolerance to just under 1 unit in normalized RGB space
                                tolerance = (1.0f - AZ::g_fltEps) / 255.0f;
                            }

                            Maestro::SequenceComponentRequests::AnimatedVector3Value value(AZ::Vector3(vec3Value.x, vec3Value.y, vec3Value.z));

                            Maestro::SequenceComponentRequests::AnimatedVector3Value prevValue(AZ::Vector3(vec3Value.x, vec3Value.y, vec3Value.z));
                            Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevValue, GetParentAzEntityId(), animatableAddress);
                            AZ::Vector3 vector3PrevValue;
                            prevValue.GetValue(vector3PrevValue);

                            // Check sub-tracks for keys. If there are none, use the prevValue for that track (essentially making a non-keyed track a no-op)                    
                            vector3Value.Set(pTrack->GetSubTrack(0)->HasKeys() ? vector3Value.GetX() : vector3PrevValue.GetX(),
                                pTrack->GetSubTrack(1)->HasKeys() ? vector3Value.GetY() : vector3PrevValue.GetY(),
                                pTrack->GetSubTrack(2)->HasKeys() ? vector3Value.GetZ() : vector3PrevValue.GetZ());
                            value.SetValue(vector3Value);

                            if (!value.IsClose(prevValue, tolerance))
                            {
                                // only set the value if it's changed
                                Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), animatableAddress, value);
                            }
                            break;
                        }
                        case AnimValueType::Quat:
                        {
                            if (pTrack->HasKeys())
                            {
                                float tolerance = AZ::g_fltEps;

                                AZ::Quaternion quaternionValue(AZ::Quaternion::CreateIdentity());
                                pTrack->GetValue(ac.time, quaternionValue);
                                Maestro::SequenceComponentRequests::AnimatedQuaternionValue value(quaternionValue);
                                Maestro::SequenceComponentRequests::AnimatedQuaternionValue prevValue(quaternionValue);
                                Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevValue, GetParentAzEntityId(), animatableAddress);
                                AZ::Quaternion prevQuaternionValue;
                                prevValue.GetValue(prevQuaternionValue);

                                if (!prevQuaternionValue.IsClose(quaternionValue, tolerance))
                                {
                                    // only set the value if it's changed
                                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), animatableAddress, value);
                                }
                            }
                            break;
                        }
                        case AnimValueType::Bool:
                        {
                            if (pTrack->HasKeys())
                            {
                                bool boolValue = true;
                                pTrack->GetValue(ac.time, boolValue);
                                Maestro::SequenceComponentRequests::AnimatedBoolValue value(boolValue);

                                Maestro::SequenceComponentRequests::AnimatedBoolValue prevValue(boolValue);
                                Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAnimatedPropertyValue, prevValue, GetParentAzEntityId(), animatableAddress);
                                if (!value.IsClose(prevValue))
                                {
                                    // only set the value if it's changed
                                    Maestro::SequenceComponentRequestBus::Event(m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentRequestBus::Events::SetAnimatedPropertyValue, GetParentAzEntityId(), animatableAddress, value);
                                }
                            }
                            break;
                        }
                        default:
                        {
                            AZ_Warning("TrackView", false, "Unsupported value type %d requested for Component Node Track %s, skipping...", pTrack->GetValueType(), paramType.GetName());
                            break;
                        }
                    }
                }
            }
        }
    }

    if (m_pOwner)
    {
        m_bIgnoreSetParam = true; // Prevents feedback change of track.
        m_pOwner->OnNodeAnimated(this);
        m_bIgnoreSetParam = false;
    }
}
