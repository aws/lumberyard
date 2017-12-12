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

#include "StdAfx.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "AnimNode.h"
#include "AnimTrack.h"
#include "AnimSequence.h"
#include "CharacterTrack.h"
#include "AnimSplineTrack.h"
#include "BoolTrack.h"
#include "SelectTrack.h"
#include "EventTrack.h"
#include "SoundTrack.h"
#include "ConsoleTrack.h"
#include "LookAtTrack.h"
#include "TrackEventTrack.h"
#include "SequenceTrack.h"
#include "CompoundSplineTrack.h"
#include "GotoTrack.h"
#include "CaptureTrack.h"
#include "CommentTrack.h"
#include "MannequinTrack.h"
#include "ScreenFaderTrack.h"
#include "TimeRangesTrack.h"
#include "SoundTrack.h"

#include <AzCore/std/sort.h>
#include <AzCore/Math/MathUtils.h>
#include <I3DEngine.h>
#include <ctime>

//////////////////////////////////////////////////////////////////////////
// Old deprecated IDs
//////////////////////////////////////////////////////////////////////////
#define APARAM_CHARACTER4 (eAnimParamType_User + 0x10)
#define APARAM_CHARACTER5 (eAnimParamType_User + 0x11)
#define APARAM_CHARACTER6 (eAnimParamType_User + 0x12)
#define APARAM_CHARACTER7 (eAnimParamType_User + 0x13)
#define APARAM_CHARACTER8 (eAnimParamType_User + 0x14)
#define APARAM_CHARACTER9 (eAnimParamType_User + 0x15)
#define APARAM_CHARACTER10 (eAnimParamType_User + 0x16)

//////////////////////////////////////////////////////////////////////////
static const EAnimCurveType DEFAULT_TRACK_TYPE = eAnimCurveType_BezierFloat;

// Old serialization values that are no longer
// defined in IMovieSystem.h, but needed for conversion:
static const int OLD_APARAM_USER = 100;
static const int OLD_ACURVE_GOTO = 21;
static const int OLD_APARAM_PARTICLE_COUNT_SCALE = 95;
static const int OLD_APARAM_PARTICLE_PULSE_PERIOD = 96;
static const int OLD_APARAM_PARTICLE_SCALE = 97;
static const int OLD_APARAM_PARTICLE_SPEED_SCALE = 98;
static const int OLD_APARAM_PARTICLE_STRENGTH = 99;

//////////////////////////////////////////////////////////////////////////
// CAnimNode.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CAnimNode::Activate(bool bActivate)
{
}

//////////////////////////////////////////////////////////////////////////
int CAnimNode::GetTrackCount() const
{
    return m_tracks.size();
}

const char* CAnimNode::GetParamName(const CAnimParamType& paramType) const
{
    SParamInfo info;
    if (GetParamInfoFromType(paramType, info))
    {
        return info.name;
    }

    return "Unknown";
}

EAnimValue CAnimNode::GetParamValueType(const CAnimParamType& paramType) const
{
    SParamInfo info;
    if (GetParamInfoFromType(paramType, info))
    {
        return info.valueType;
    }

    return eAnimValue_Unknown;
}

IAnimNode::ESupportedParamFlags CAnimNode::GetParamFlags(const CAnimParamType& paramType) const
{
    SParamInfo info;
    if (GetParamInfoFromType(paramType, info))
    {
        return info.flags;
    }

    return IAnimNode::ESupportedParamFlags(0);
}

IAnimTrack* CAnimNode::GetTrackForParameter(const CAnimParamType& paramType) const
{
    for (int i = 0, num = (int)m_tracks.size(); i < num; i++)
    {
        if (m_tracks[i]->GetParameterType() == paramType)
        {
            return m_tracks[i].get();
        }

        // Search the sub-tracks also if any.
        for (int k = 0; k < m_tracks[i]->GetSubTrackCount(); ++k)
        {
            if (m_tracks[i]->GetSubTrack(k)->GetParameterType() == paramType)
            {
                return m_tracks[i]->GetSubTrack(k);
            }
        }
    }
    return 0;
}

IAnimTrack* CAnimNode::GetTrackForParameter(const CAnimParamType& paramType, uint32 index) const
{
    SParamInfo paramInfo;
    GetParamInfoFromType(paramType, paramInfo);

    if ((paramInfo.flags & IAnimNode::eSupportedParamFlags_MultipleTracks) == 0)
    {
        return GetTrackForParameter(paramType);
    }

    uint32 count = 0;
    for (int i = 0, num = (int)m_tracks.size(); i < num; i++)
    {
        if (m_tracks[i]->GetParameterType() == paramType && count++ == index)
        {
            return m_tracks[i].get();
        }

        // For this case, no subtracks are considered.
    }
    return 0;
}

uint32 CAnimNode::GetTrackParamIndex(const IAnimTrack* pTrack) const
{
    assert(pTrack);
    uint32 index = 0;
    CAnimParamType paramType = pTrack->GetParameterType();

    SParamInfo paramInfo;
    GetParamInfoFromType(paramType, paramInfo);

    if ((paramInfo.flags & IAnimNode::eSupportedParamFlags_MultipleTracks) == 0)
    {
        return 0;
    }

    for (int i = 0, num = (int)m_tracks.size(); i < num; i++)
    {
        if (m_tracks[i].get() == pTrack)
        {
            return index;
        }

        if (m_tracks[i]->GetParameterType() == paramType)
        {
            ++index;
        }

        // For this case, no subtracks are considered.
    }
    assert(!"CAnimNode::GetTrackParamIndex() called with an invalid argument!");
    return 0;
}

IAnimTrack* CAnimNode::GetTrackByIndex(int nIndex) const
{
    if (nIndex >= (int)m_tracks.size())
    {
        assert("nIndex>=m_tracks.size()" && false);
        return NULL;
    }
    return m_tracks[nIndex].get();
}

void CAnimNode::SetTrack(const CAnimParamType& paramType, IAnimTrack* pTrack)
{
    if (pTrack)
    {
        for (unsigned int i = 0; i < m_tracks.size(); i++)
        {
            if (m_tracks[i]->GetParameterType() == paramType)
            {
                m_tracks[i].reset(pTrack);
                return;
            }
        }

        AddTrack(pTrack);
    }
    else
    {
        // Remove track at this id.
        for (unsigned int i = 0; i < m_tracks.size(); i++)
        {
            if (m_tracks[i]->GetParameterType() == paramType)
            {
                m_tracks.erase(m_tracks.begin() + i);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::TrackOrder(const AZStd::intrusive_ptr<IAnimTrack>& left, const AZStd::intrusive_ptr<IAnimTrack>& right)
{
    return left->GetParameterType() < right->GetParameterType();
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::AddTrack(IAnimTrack* pTrack)
{
    RegisterTrack(pTrack);
    m_tracks.push_back(AZStd::intrusive_ptr<IAnimTrack>(pTrack));
    SortTracks();
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::RegisterTrack(IAnimTrack* pTrack)
{
    pTrack->SetTimeRange(GetSequence()->GetTimeRange());
    pTrack->SetNode(this);
}

void CAnimNode::SortTracks()
{
    AZStd::allocator allocator;
    AZStd::stable_sort(m_tracks.begin(), m_tracks.end(), TrackOrder, allocator);
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::RemoveTrack(IAnimTrack* pTrack)
{
    for (unsigned int i = 0; i < m_tracks.size(); i++)
    {
        if (m_tracks[i].get() == pTrack)
        {
            m_tracks.erase(m_tracks.begin() + i);
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CAnimNode>()
        ->Version(1)
        ->Field("ID", &CAnimNode::m_id)
        ->Field("Name", &CAnimNode::m_name)
        ->Field("Flags", &CAnimNode::m_flags)
        ->Field("Tracks", &CAnimNode::m_tracks)
        ->Field("Parent", &CAnimNode::m_parentNodeId)
        ->Field("Type", &CAnimNode::m_nodeType);
}

//////////////////////////////////////////////////////////////////////////
void CAnimNodeGroup::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CAnimNodeGroup, CAnimNode>()
        ->Version(1);
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CAnimNode::CreateTrackInternal(const CAnimParamType& paramType, EAnimCurveType trackType, EAnimValue valueType)
{
    if (valueType == eAnimValue_Unknown)
    {
        SParamInfo info;

        // Try to get info from paramType, else we can't determine the track data type
        if (!GetParamInfoFromType(paramType, info))
        {
            return 0;
        }

        valueType = info.valueType;
    }

    IAnimTrack* pTrack = NULL;

    switch (paramType.GetType())
    {
    // Create sub-classed tracks
    case eAnimParamType_Event:
        pTrack = aznew CEventTrack(m_pSequence->GetTrackEventStringTable());
        break;
    case eAnimParamType_Sound:
        pTrack = aznew CSoundTrack;
        break;
    case eAnimParamType_Animation:
        pTrack = aznew CCharacterTrack;
        break;
    case eAnimParamType_Mannequin:
        pTrack = aznew CMannequinTrack;
        break;
    case eAnimParamType_Console:
        pTrack = aznew CConsoleTrack;
        break;
    case eAnimParamType_LookAt:
        pTrack = aznew CLookAtTrack;
        break;
    case eAnimParamType_TrackEvent:
        pTrack = aznew CTrackEventTrack(m_pSequence->GetTrackEventStringTable());
        break;
    case eAnimParamType_Sequence:
        pTrack = aznew CSequenceTrack;
        break;
    case eAnimParamType_Capture:
        pTrack = aznew CCaptureTrack;
        break;
    case eAnimParamType_CommentText:
        pTrack = aznew CCommentTrack;
        break;
    case eAnimParamType_ScreenFader:
        pTrack = aznew CScreenFaderTrack;
        break;
    case eAnimParamType_Goto:
        pTrack = aznew CGotoTrack;
        break;
    case eAnimParamType_TimeRanges:
        pTrack = aznew CTimeRangesTrack;
        break;
    case eAnimParamType_Float:
        pTrack = CreateTrackInternalFloat(trackType);
        break;

    default:
        // Create standard tracks
        switch (valueType)
        {
        case eAnimValue_Float:
            pTrack = CreateTrackInternalFloat(trackType);
            break;
        case eAnimValue_RGB:
        case eAnimValue_Vector:
            pTrack = CreateTrackInternalVector(trackType, paramType, valueType);
            break;
        case eAnimValue_Quat:
            pTrack = CreateTrackInternalQuat(trackType, paramType);
            break;
        case eAnimValue_Bool:
            pTrack = aznew CBoolTrack;
            break;
        case eAnimValue_Select:
            pTrack = aznew CSelectTrack;
            break;
        case eAnimValue_Vector4:
            pTrack = CreateTrackInternalVector4(paramType);
            break;
        case eAnimValue_CharacterAnim:
            pTrack = aznew CCharacterTrack;
            break;
        }
    }

    if (pTrack)
    {
        pTrack->SetParameterType(paramType);
        AddTrack(pTrack);
    }

    return pTrack;
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CAnimNode::CreateTrack(const CAnimParamType& paramType)
{
    IAnimTrack* pTrack = CreateTrackInternal(paramType, DEFAULT_TRACK_TYPE, eAnimValue_Unknown);
    InitializeTrackDefaultValue(pTrack, paramType);
    return pTrack;
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::SerializeAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Animation, 0, "CAnimNode");

    if (bLoading)
    {
        // Delete all tracks.
        stl::free_container(m_tracks);

        CAnimNode::SParamInfo info;
        // Loading.
        int paramTypeVersion = 0;
        xmlNode->getAttr("paramIdVersion", paramTypeVersion);
        CAnimParamType paramType;
        int num = xmlNode->getChildCount();
        for (int i = 0; i < num; i++)
        {
            XmlNodeRef trackNode = xmlNode->getChild(i);
            paramType.Serialize(trackNode, bLoading, paramTypeVersion);

            MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Animation, 0, CMovieSystem::GetParamTypeName(paramType));

            if (paramType.GetType() == eAnimParamType_Music)
            {
                // skip loading eAnimParamType_Music - it's deprecated
                continue;
            }

            if (paramTypeVersion == 0) // for old version with sound and animation param ids swapped
            {
                const int APARAM_ANIMATION_OLD = eAnimParamType_Sound;
                const int APARAM_SOUND_OLD = eAnimParamType_Animation;
                if (paramType.GetType() == APARAM_ANIMATION_OLD)
                {
                    paramType = eAnimParamType_Animation;
                }
                else if (paramType.GetType() == APARAM_SOUND_OLD)
                {
                    paramType = eAnimParamType_Sound;
                }
            }

            int curveType = eAnimCurveType_Unknown;
            trackNode->getAttr("Type", curveType);
            if (curveType == eAnimCurveType_Unknown)
            {
                if (paramTypeVersion == 0)
                {
                    //////////////////////////////////////////////////////////////////////////
                    // Backward compatibility code
                    //////////////////////////////////////////////////////////////////////////
                    // Legacy animation track.
                    // Collapse parameter ID to the single type
                    if (paramType.GetType() >= eAnimParamType_Sound && paramType.GetType() <= eAnimParamType_Sound + 2)
                    {
                        paramType = eAnimParamType_Sound;
                    }
                    if (paramType.GetType() >= eAnimParamType_Animation && paramType.GetType() <= eAnimParamType_Animation + 2)
                    {
                        paramType = eAnimParamType_Animation;
                    }
                    if (paramType.GetType() >= APARAM_CHARACTER4 && paramType.GetType() <= APARAM_CHARACTER10)
                    {
                        paramType = eAnimParamType_Animation;
                    }

                    // Old tracks always used TCB tracks.
                    // Backward compatibility to the CryEngine2 for track type (will make TCB controller)
                    curveType = eAnimCurveType_TCBVector;
                }
            }

            if (paramTypeVersion <= 1)
            {
                // In old versions goto tracks were identified by a curve id
                if (curveType == OLD_ACURVE_GOTO)
                {
                    paramType = eAnimParamType_Goto;
                    curveType = eAnimCurveType_Unknown;
                }
            }

            if (paramTypeVersion <= 3 && paramType.GetType() >= OLD_APARAM_USER)
            {
                // APARAM_USER 100 => 100000
                paramType = paramType.GetType() + (eAnimParamType_User - OLD_APARAM_USER);
            }

            if (paramTypeVersion <= 4)
            {
                // In old versions there was special code for particles
                // that is now handles by generic entity node code
                switch (paramType.GetType())
                {
                case OLD_APARAM_PARTICLE_COUNT_SCALE:
                    paramType = CAnimParamType("ScriptTable:Properties/CountScale");
                    break;
                case OLD_APARAM_PARTICLE_PULSE_PERIOD:
                    paramType = CAnimParamType("ScriptTable:Properties/PulsePeriod");
                    break;
                case OLD_APARAM_PARTICLE_SCALE:
                    paramType = CAnimParamType("ScriptTable:Properties/Scale");
                    break;
                case OLD_APARAM_PARTICLE_SPEED_SCALE:
                    paramType = CAnimParamType("ScriptTable:Properties/SpeedScale");
                    break;
                case OLD_APARAM_PARTICLE_STRENGTH:
                    paramType = CAnimParamType("ScriptTable:Properties/Strength");
                    break;
                }
            }

            if (paramTypeVersion <= 5 && !(GetSequence()->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet))
            {
                // In old versions there was special code for lights that is now handled
                // by generic entity node code if this is not a light animation set sequence
                switch (paramType.GetType())
                {
                case eAnimParamType_LightDiffuse:
                    paramType = CAnimParamType("ScriptTable:Properties/Color/clrDiffuse");
                    break;
                case eAnimParamType_LightRadius:
                    paramType = CAnimParamType("ScriptTable:Properties/Radius");
                    break;
                case eAnimParamType_LightDiffuseMult:
                    paramType = CAnimParamType("ScriptTable:Properties/Color/fDiffuseMultiplier");
                    break;
                case eAnimParamType_LightHDRDynamic:
                    paramType = CAnimParamType("ScriptTable:Properties/Color/fHDRDynamic");
                    break;
                case eAnimParamType_LightSpecularMult:
                    paramType = CAnimParamType("ScriptTable:Properties/Color/fSpecularMultiplier");
                    break;
                case eAnimParamType_LightSpecPercentage:
                    paramType = CAnimParamType("ScriptTable:Properties/Color/fSpecularPercentage");
                    break;
                }
            }

            if (paramTypeVersion <= 7 && paramType.GetType() == eAnimParamType_Physics)
            {
                paramType = eAnimParamType_PhysicsDriven;
            }

            int valueType = eAnimValue_Unknown;
            trackNode->getAttr("ValueType", valueType);

            IAnimTrack* pTrack = CreateTrackInternal(paramType, (EAnimCurveType)curveType, (EAnimValue)valueType);
            bool trackRemoved = false;
            if (pTrack)
            {
                if (!pTrack->Serialize(trackNode, bLoading, bLoadEmptyTracks))
                {
                    // Boolean tracks must always be loaded even if empty.
                    if (pTrack->GetValueType() != eAnimValue_Bool)
                    {
                        RemoveTrack(pTrack);
                        trackRemoved = true;
                    }
                }
            }

            if (!trackRemoved && gEnv->IsEditor())
            {
                InitializeTrackDefaultValue(pTrack, paramType);
            }
        }
    }
    else
    {
        // Saving.
        xmlNode->setAttr("paramIdVersion", CAnimParamType::kParamTypeVersion);
        for (unsigned int i = 0; i < m_tracks.size(); i++)
        {
            IAnimTrack* pTrack = m_tracks[i].get();
            if (pTrack)
            {
                CAnimParamType paramType = m_tracks[i]->GetParameterType();
                XmlNodeRef trackNode = xmlNode->newChild("Track");
                paramType.Serialize(trackNode, bLoading);
                int nTrackType = pTrack->GetCurveType();
                trackNode->setAttr("Type", nTrackType);
                pTrack->Serialize(trackNode, bLoading);
                int valueType = pTrack->GetValueType();
                trackNode->setAttr("ValueType", valueType);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::SetTimeRange(Range timeRange)
{
    for (unsigned int i = 0; i < m_tracks.size(); i++)
    {
        if (m_tracks[i])
        {
            m_tracks[i]->SetTimeRange(timeRange);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// AZ::Serialization requires a default constructor
CAnimNode::CAnimNode()
    : CAnimNode(0, eAnimNodeType_Invalid)
{
}

//////////////////////////////////////////////////////////////////////////
// explicit copy constructor is required to prevent compiler's generated copy constructor
// from calling AZStd::mutex's private copy constructor
CAnimNode::CAnimNode(const CAnimNode& other)
    : m_refCount(0)
    , m_id(0)                                   // don't copy id - these should be unique
    , m_parentNodeId(other.m_parentNodeId)
    , m_nodeType(other.m_nodeType)
    , m_pOwner(other.m_pOwner)
    , m_pSequence(other.m_pSequence)
    , m_flags(other.m_flags)
    , m_pParentNode(other.m_pParentNode)
    , m_nLoadedParentNodeId(other.m_nLoadedParentNodeId)
{
    // m_bIgnoreSetParam not copied
}

//////////////////////////////////////////////////////////////////////////
CAnimNode::CAnimNode(const int id, EAnimNodeType nodeType)
    : m_refCount(0)
    , m_id(id)
    , m_parentNodeId(0)
    , m_nodeType(nodeType)
{
    m_pOwner = 0;
    m_pSequence = 0;
    m_flags = 0;
    m_bIgnoreSetParam = false;
    m_pParentNode = 0;
    m_nLoadedParentNodeId = 0;
}

//////////////////////////////////////////////////////////////////////////
CAnimNode::~CAnimNode()
{
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::SetFlags(int flags)
{
    m_flags = flags;
}

//////////////////////////////////////////////////////////////////////////
int CAnimNode::GetFlags() const
{
    return m_flags;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::AreFlagsSetOnNodeOrAnyParent(EAnimNodeFlags flagsToCheck) const
{
    if (m_pParentNode)
    {
        // recurse up parent chain until we find the flagsToCheck set or get to the top of the chain
        return ((GetFlags() & flagsToCheck) != 0) || m_pParentNode->AreFlagsSetOnNodeOrAnyParent(flagsToCheck);
    }

    // top of parent chain
    return ((GetFlags() & flagsToCheck) != 0);
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::Animate(SAnimContext& ec)
{
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::IsParamValid(const CAnimParamType& paramType) const
{
    SParamInfo info;

    if (GetParamInfoFromType(paramType, info))
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::SetParamValue(float time, CAnimParamType param, float value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    IAnimTrack* pTrack = GetTrackForParameter(param);
    if (pTrack && pTrack->GetValueType() == eAnimValue_Float)
    {
        // Float track.
        bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
        pTrack->SetValue(time, value, bDefault);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::SetParamValue(float time, CAnimParamType param, const Vec3& value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    CCompoundSplineTrack* pTrack = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
    if (pTrack && pTrack->GetValueType() == eAnimValue_Vector)
    {
        // Vec3 track.
        bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
        pTrack->SetValue(time, value, bDefault);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::SetParamValue(float time, CAnimParamType param, const Vec4& value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    CCompoundSplineTrack* pTrack = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
    if (pTrack && pTrack->GetValueType() == eAnimValue_Vector4)
    {
        // Vec4 track.
        bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
        pTrack->SetValue(time, value, bDefault);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::GetParamValue(float time, CAnimParamType param, float& value)
{
    IAnimTrack* pTrack = GetTrackForParameter(param);
    if (pTrack && pTrack->GetValueType() == eAnimValue_Float && pTrack->GetNumKeys() > 0)
    {
        // Float track.
        pTrack->GetValue(time, value);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::GetParamValue(float time, CAnimParamType param, Vec3& value)
{
    CCompoundSplineTrack* pTrack = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
    if (pTrack && pTrack->GetValueType() == eAnimValue_Vector && pTrack->GetNumKeys() > 0)
    {
        // Vec3 track.
        pTrack->GetValue(time, value);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimNode::GetParamValue(float time, CAnimParamType param, Vec4& value)
{
    CCompoundSplineTrack* pTrack = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(param));
    if (pTrack && pTrack->GetValueType() == eAnimValue_Vector4 && pTrack->GetNumKeys() > 0)
    {
        // Vec4 track.
        pTrack->GetValue(time, value);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
void CAnimNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    if (bLoading)
    {
        xmlNode->getAttr("Id", m_id);
        const char* name = xmlNode->getAttr("Name");
        int flags;
        if (xmlNode->getAttr("Flags", flags))
        {
            // Don't load expanded or selected flags
            flags = flags & ~(eAnimNodeFlags_Expanded | eAnimNodeFlags_EntitySelected);
            SetFlags(flags);
        }

        SetName(name);

        m_nLoadedParentNodeId = 0;
        xmlNode->getAttr("ParentNode", m_nLoadedParentNodeId);
    }
    else
    {
        m_nLoadedParentNodeId = 0;
        xmlNode->setAttr("Id", m_id);

        EAnimNodeType nodeType = GetType();
        GetMovieSystem()->SerializeNodeType(nodeType, xmlNode, bLoading, IAnimSequence::kSequenceVersion, m_flags);

        xmlNode->setAttr("Name", GetName());

        // Don't store expanded or selected flags
        int flags = GetFlags() & ~(eAnimNodeFlags_Expanded | eAnimNodeFlags_EntitySelected);
        xmlNode->setAttr("Flags", flags);

        if (m_pParentNode)
        {
            xmlNode->setAttr("ParentNode", static_cast<CAnimNode*>(m_pParentNode)->GetId());
        }
    }

    SerializeAnims(xmlNode, bLoading, bLoadEmptyTracks);
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::InitPostLoad(IAnimSequence* sequence)
{
    m_pSequence = sequence;
    m_pParentNode = ((CAnimSequence*)m_pSequence)->FindNodeById(m_parentNodeId);

    // fix up animNode pointers and time ranges on tracks, then sort them
    for (unsigned int i = 0; i < m_tracks.size(); i++)
    {
        RegisterTrack(m_tracks[i].get());
        m_tracks[i].get()->InitPostLoad(sequence);
    }
    SortTracks();
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::SetNodeOwner(IAnimNodeOwner* pOwner)
{
    m_pOwner = pOwner;

    if (pOwner)
    {
        pOwner->OnNodeAnimated(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::PostLoad()
{
    if (m_nLoadedParentNodeId)
    {
        IAnimNode* pParentNode = ((CAnimSequence*)m_pSequence)->FindNodeById(m_nLoadedParentNodeId);
        m_pParentNode = pParentNode;
        m_parentNodeId = m_nLoadedParentNodeId; // adding as a temporary fix while we support both serialization methods
        m_nLoadedParentNodeId = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
Matrix34 CAnimNode::GetReferenceMatrix() const
{
    static Matrix34 tm(IDENTITY);
    return tm;
}

IAnimTrack* CAnimNode::CreateTrackInternalFloat(int trackType) const
{
    return aznew C2DSplineTrack;
}

IAnimTrack* CAnimNode::CreateTrackInternalVector(EAnimCurveType trackType, const CAnimParamType& paramType, const EAnimValue animValue) const
{
    CAnimParamType subTrackParamTypes[MAX_SUBTRACKS];
    for (unsigned int i = 0; i < MAX_SUBTRACKS; ++i)
    {
        subTrackParamTypes[i] = eAnimParamType_Float;
    }

    if (paramType == eAnimParamType_Position)
    {
        subTrackParamTypes[0] = eAnimParamType_PositionX;
        subTrackParamTypes[1] = eAnimParamType_PositionY;
        subTrackParamTypes[2] = eAnimParamType_PositionZ;
    }
    else if (paramType == eAnimParamType_Scale)
    {
        subTrackParamTypes[0] = eAnimParamType_ScaleX;
        subTrackParamTypes[1] = eAnimParamType_ScaleY;
        subTrackParamTypes[2] = eAnimParamType_ScaleZ;
    }
    else if (paramType == eAnimParamType_Rotation)
    {
        subTrackParamTypes[0] = eAnimParamType_RotationX;
        subTrackParamTypes[1] = eAnimParamType_RotationY;
        subTrackParamTypes[2] = eAnimParamType_RotationZ;
        IAnimTrack* pTrack = aznew CCompoundSplineTrack(3, eAnimValue_Quat, subTrackParamTypes);
        return pTrack;
    }
    else if (paramType == eAnimParamType_DepthOfField)
    {
        subTrackParamTypes[0] = eAnimParamType_FocusDistance;
        subTrackParamTypes[1] = eAnimParamType_FocusRange;
        subTrackParamTypes[2] = eAnimParamType_BlurAmount;
        IAnimTrack* pTrack = aznew CCompoundSplineTrack(3, eAnimValue_Vector, subTrackParamTypes);
        pTrack->SetSubTrackName(0, "FocusDist");
        pTrack->SetSubTrackName(1, "FocusRange");
        pTrack->SetSubTrackName(2, "BlurAmount");
        return pTrack;
    }
    else if (animValue == eAnimValue_RGB || paramType == eAnimParamType_LightDiffuse ||
             paramType == eAnimParamType_MaterialDiffuse || paramType == eAnimParamType_MaterialSpecular
             || paramType == eAnimParamType_MaterialEmissive)
    {
        subTrackParamTypes[0] = eAnimParamType_ColorR;
        subTrackParamTypes[1] = eAnimParamType_ColorG;
        subTrackParamTypes[2] = eAnimParamType_ColorB;
        IAnimTrack* pTrack = aznew CCompoundSplineTrack(3, eAnimValue_RGB, subTrackParamTypes);
        pTrack->SetSubTrackName(0, "Red");
        pTrack->SetSubTrackName(1, "Green");
        pTrack->SetSubTrackName(2, "Blue");
        return pTrack;
    }

    return aznew CCompoundSplineTrack(3, eAnimValue_Vector, subTrackParamTypes);
}

IAnimTrack* CAnimNode::CreateTrackInternalQuat(EAnimCurveType trackType, const CAnimParamType& paramType) const
{
    CAnimParamType subTrackParamTypes[MAX_SUBTRACKS];
    if (paramType == eAnimParamType_Rotation)
    {
        subTrackParamTypes[0] = eAnimParamType_RotationX;
        subTrackParamTypes[1] = eAnimParamType_RotationY;
        subTrackParamTypes[2] = eAnimParamType_RotationZ;
    }
    else
    {
        // Unknown param type
        assert(0);
    }

    return aznew CCompoundSplineTrack(3, eAnimValue_Quat, subTrackParamTypes);
}

IAnimTrack* CAnimNode::CreateTrackInternalVector4(const CAnimParamType& paramType) const
{
    IAnimTrack* pTrack;

    CAnimParamType subTrackParamTypes[MAX_SUBTRACKS];

    // set up track subtypes
    if (paramType == eAnimParamType_TransformNoise
        || paramType == eAnimParamType_ShakeMultiplier)
    {
        subTrackParamTypes[0] = eAnimParamType_ShakeAmpAMult;
        subTrackParamTypes[1] = eAnimParamType_ShakeAmpBMult;
        subTrackParamTypes[2] = eAnimParamType_ShakeFreqAMult;
        subTrackParamTypes[3] = eAnimParamType_ShakeFreqBMult;
    }
    else
    {
        // default to a Vector4 of floats
        for (unsigned int i = 0; i < MAX_SUBTRACKS; ++i)
        {
            subTrackParamTypes[i] = eAnimParamType_Float;
        }
    }

    // create track
    pTrack = aznew CCompoundSplineTrack(4, eAnimValue_Vector4, subTrackParamTypes);

    // label subtypes
    if (paramType == eAnimParamType_TransformNoise)
    {
        pTrack->SetSubTrackName(0, "Pos Noise Amp");
        pTrack->SetSubTrackName(1, "Pos Noise Freq");
        pTrack->SetSubTrackName(2, "Rot Noise Amp");
        pTrack->SetSubTrackName(3, "Rot Noise Freq");
    }
    else if (paramType == eAnimParamType_ShakeMultiplier)
    {
        pTrack->SetSubTrackName(0, "Amplitude A");
        pTrack->SetSubTrackName(1, "Amplitude B");
        pTrack->SetSubTrackName(2, "Frequency A");
        pTrack->SetSubTrackName(3, "Frequency B");
    }

    return pTrack;
}

void CAnimNode::TimeChanged(float newTime)
{
    // if the newTime is on a sound key, then reset sounds so sound will playback on next call to Animate()
    if (IsTimeOnSoundKey(newTime))
    {
        ResetSounds();
    }
}

bool CAnimNode::IsTimeOnSoundKey(float queryTime) const
{
    bool retIsTimeOnSoundKey = false;
    const float tolerance = 0.0333f;        // one frame at 30 fps

    int trackCount = NumTracks();
    for (int trackIndex = 0; trackIndex < trackCount; trackIndex++)
    {
        CAnimParamType paramType = m_tracks[trackIndex]->GetParameterType();
        IAnimTrack* pTrack = m_tracks[trackIndex].get();
        if ((paramType.GetType() != eAnimParamType_Sound)
            || (pTrack->HasKeys() == false && pTrack->GetParameterType() != eAnimParamType_Visibility)
            || (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
        {
            continue;
        }

        // if we're here, pTrack points to a eAnimParamType_Sound track
        ISoundKey oSoundKey;
        int const nSoundKey = static_cast<CSoundTrack*>(pTrack)->GetActiveKey(queryTime, &oSoundKey);
        if (nSoundKey >= 0)
        {
            retIsTimeOnSoundKey = AZ::IsClose(queryTime, oSoundKey.time, tolerance);
            if (retIsTimeOnSoundKey)
            {
                break;      // no need to search further, we have a hit
            }
        }
    }

    return retIsTimeOnSoundKey;
}

//////////////////////////////////////////////////////////////////////////
void CAnimNode::AnimateSound(std::vector<SSoundInfo>& nodeSoundInfo, SAnimContext& ec, IAnimTrack* pTrack, size_t numAudioTracks)
{
    bool const bMute = gEnv->IsEditor() && (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Muted);

    if (!bMute && ec.time >= 0.0f)
    {
        ISoundKey oSoundKey;
        int const nSoundKey = static_cast<CSoundTrack*>(pTrack)->GetActiveKey(ec.time, &oSoundKey);
        SSoundInfo& rSoundInfo = nodeSoundInfo[numAudioTracks - 1];

        if (nSoundKey >= 0)
        {
            float const fSoundKeyTime = (ec.time - oSoundKey.time);

            if (rSoundInfo.nSoundKeyStart < nSoundKey && fSoundKeyTime < oSoundKey.fDuration)
            {
                ApplyAudioKey(oSoundKey.sStartTrigger.c_str());
            }

            if (rSoundInfo.nSoundKeyStart > nSoundKey)
            {
                rSoundInfo.nSoundKeyStop = nSoundKey;
            }

            rSoundInfo.nSoundKeyStart = nSoundKey;

            if (fSoundKeyTime >= oSoundKey.fDuration)
            {
                if (rSoundInfo.nSoundKeyStop < nSoundKey)
                {
                    rSoundInfo.nSoundKeyStop = nSoundKey;

                    if (oSoundKey.sStopTrigger.empty())
                    {
                        ApplyAudioKey(oSoundKey.sStartTrigger.c_str(), false);
                    }
                    else
                    {
                        ApplyAudioKey(oSoundKey.sStopTrigger.c_str());
                    }
                }
            }
            else
            {
                rSoundInfo.nSoundKeyStop = -1;
            }
        }
        else
        {
            rSoundInfo.Reset();
        }
    }
}

void CAnimNode::SetParent(IAnimNode* parent)
{ 
    m_pParentNode = parent; 
    if (parent)
    {
        m_parentNodeId = static_cast<CAnimNode*>(m_pParentNode)->GetId();
    }
    else
    {
        m_parentNodeId = 0;
    }
}
//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimNode::HasDirectorAsParent() const
{
    IAnimNode* pParent = GetParent();
    while (pParent)
    {
        if (pParent->GetType() == eAnimNodeType_Director)
        {
            return pParent;
        }
        // There are some invalid data.
        if (pParent->GetParent() == pParent)
        {
            pParent->SetParent(NULL);
            return NULL;
        }
        pParent = pParent->GetParent();
    }
    return NULL;
}

void CAnimNode::UpdateDynamicParams()
{
    if (gEnv->IsEditor())
    {
        // UpdateDynamicParams is called as the result of an editor event that is fired when a material is loaded,
        // which could happen from multiple threads. Lock to avoid a crash iterating over the lua stack
        AZStd::lock_guard<AZStd::mutex> lock(m_updateDynamicParamsLock);

        UpdateDynamicParamsInternal();
    }
    else
    {
        UpdateDynamicParamsInternal();
    }
}
