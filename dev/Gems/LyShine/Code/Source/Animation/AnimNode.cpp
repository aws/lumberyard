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

#include "LyShine_precompiled.h"
#include "AnimNode.h"
#include "AnimTrack.h"
#include "AnimSequence.h"
#include "AnimSplineTrack.h"
#include "BoolTrack.h"
#include "CompoundSplineTrack.h"

#include <AzCore/std/sort.h>
#include <I3DEngine.h>
#include <ctime>


//////////////////////////////////////////////////////////////////////////
// Old deprecated IDs
//////////////////////////////////////////////////////////////////////////
#define APARAM_CHARACTER4 (eUiAnimParamType_User + 0x10)
#define APARAM_CHARACTER5 (eUiAnimParamType_User + 0x11)
#define APARAM_CHARACTER6 (eUiAnimParamType_User + 0x12)
#define APARAM_CHARACTER7 (eUiAnimParamType_User + 0x13)
#define APARAM_CHARACTER8 (eUiAnimParamType_User + 0x14)
#define APARAM_CHARACTER9 (eUiAnimParamType_User + 0x15)
#define APARAM_CHARACTER10 (eUiAnimParamType_User + 0x16)

#define APARAM_EXPRESSION4 (eUiAnimParamType_User + 0x20)
#define APARAM_EXPRESSION5 (eUiAnimParamType_User + 0x21)
#define APARAM_EXPRESSION6 (eUiAnimParamType_User + 0x22)
#define APARAM_EXPRESSION7 (eUiAnimParamType_User + 0x23)
#define APARAM_EXPRESSION8 (eUiAnimParamType_User + 0x24)
#define APARAM_EXPRESSION9 (eUiAnimParamType_User + 0x25)
#define APARAM_EXPRESSION10 (eUiAnimParamType_User + 0x26)

//////////////////////////////////////////////////////////////////////////
static const EUiAnimCurveType DEFAULT_TRACK_TYPE = eUiAnimCurveType_BezierFloat;

// Old serialization values that are no longer
// defined in IUiAnimationSystem.h, but needed for conversion:
static const int OLD_APARAM_USER = 100;
static const int OLD_ACURVE_GOTO = 21;
static const int OLD_APARAM_PARTICLE_COUNT_SCALE = 95;
static const int OLD_APARAM_PARTICLE_PULSE_PERIOD = 96;
static const int OLD_APARAM_PARTICLE_SCALE = 97;
static const int OLD_APARAM_PARTICLE_SPEED_SCALE = 98;
static const int OLD_APARAM_PARTICLE_STRENGTH = 99;

//////////////////////////////////////////////////////////////////////////
// CUiAnimNode.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::Activate(bool bActivate)
{
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimNode::GetTrackCount() const
{
    return m_tracks.size();
}

const char* CUiAnimNode::GetParamName(const CUiAnimParamType& paramType) const
{
    SParamInfo info;
    if (GetParamInfoFromType(paramType, info))
    {
        return info.name;
    }

    return "Unknown";
}

EUiAnimValue CUiAnimNode::GetParamValueType(const CUiAnimParamType& paramType) const
{
    SParamInfo info;
    if (GetParamInfoFromType(paramType, info))
    {
        return info.valueType;
    }

    return eUiAnimValue_Unknown;
}

IUiAnimNode::ESupportedParamFlags CUiAnimNode::GetParamFlags(const CUiAnimParamType& paramType) const
{
    SParamInfo info;
    if (GetParamInfoFromType(paramType, info))
    {
        return info.flags;
    }

    return IUiAnimNode::ESupportedParamFlags(0);
}

IUiAnimTrack* CUiAnimNode::GetTrackForParameter(const CUiAnimParamType& paramType) const
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

IUiAnimTrack* CUiAnimNode::GetTrackForParameter(const CUiAnimParamType& paramType, uint32 index) const
{
    SParamInfo paramInfo;
    GetParamInfoFromType(paramType, paramInfo);

    if ((paramInfo.flags & IUiAnimNode::eSupportedParamFlags_MultipleTracks) == 0)
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

uint32 CUiAnimNode::GetTrackParamIndex(const IUiAnimTrack* pTrack) const
{
    assert(pTrack);
    uint32 index = 0;
    CUiAnimParamType paramType = pTrack->GetParameterType();

    SParamInfo paramInfo;
    GetParamInfoFromType(paramType, paramInfo);

    if ((paramInfo.flags & IUiAnimNode::eSupportedParamFlags_MultipleTracks) == 0)
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
    assert(!"CUiAnimNode::GetTrackParamIndex() called with an invalid argument!");
    return 0;
}

IUiAnimTrack* CUiAnimNode::GetTrackByIndex(int nIndex) const
{
    if (nIndex >= (int)m_tracks.size())
    {
        assert("nIndex>=m_tracks.size()" && false);
        return NULL;
    }
    return m_tracks[nIndex].get();
}

void CUiAnimNode::SetTrack(const CUiAnimParamType& paramType, IUiAnimTrack* pTrack)
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
bool CUiAnimNode::TrackOrder(const AZStd::intrusive_ptr<IUiAnimTrack>& left, const AZStd::intrusive_ptr<IUiAnimTrack>& right)
{
    return left->GetParameterType() < right->GetParameterType();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::AddTrack(IUiAnimTrack* pTrack)
{
    pTrack->SetTimeRange(GetSequence()->GetTimeRange());
    m_tracks.push_back(AZStd::intrusive_ptr<IUiAnimTrack>(pTrack));
    AZStd::allocator allocator;
    AZStd::stable_sort(m_tracks.begin(), m_tracks.end(), TrackOrder, allocator);
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::RemoveTrack(IUiAnimTrack* pTrack)
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
void CUiAnimNode::Reflect(AZ::SerializeContext* serializeContext)
{
    // we do not currently serialize node type because all nodes are the same type (AzEntityNode)

    serializeContext->Class<CUiAnimNode>()
        ->Version(1)
        ->Field("ID", &CUiAnimNode::m_id)
        ->Field("Parent", &CUiAnimNode::m_parentNodeId)
        ->Field("Name", &CUiAnimNode::m_name)
        ->Field("Flags", &CUiAnimNode::m_flags)
        ->Field("Tracks", &CUiAnimNode::m_tracks);
}

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* CUiAnimNode::CreateTrackInternal(const CUiAnimParamType& paramType, EUiAnimCurveType trackType, EUiAnimValue valueType)
{
    if (valueType == eUiAnimValue_Unknown)
    {
        SParamInfo info;

        // Try to get info from paramType, else we can't determine the track data type
        if (!GetParamInfoFromType(paramType, info))
        {
            return 0;
        }

        valueType = info.valueType;
    }

    IUiAnimTrack* pTrack = NULL;

    switch (paramType.GetType())
    {
    // Create sub-classed tracks
    case eUiAnimParamType_Float:
        pTrack = CreateTrackInternalFloat(trackType);
        break;

    default:
        // Create standard tracks
        switch (valueType)
        {
        case eUiAnimValue_Float:
            pTrack = CreateTrackInternalFloat(trackType);
            break;
        case eUiAnimValue_RGB:
        case eUiAnimValue_Vector:
            pTrack = CreateTrackInternalVector(trackType, paramType, valueType);
            break;
        case eUiAnimValue_Quat:
            pTrack = CreateTrackInternalQuat(trackType, paramType);
            break;
        case eUiAnimValue_Bool:
            pTrack = aznew UiBoolTrack;
            break;
        case eUiAnimValue_Vector2:
            pTrack = CreateTrackInternalVector2(paramType);
            break;
        case eUiAnimValue_Vector3:
            pTrack = CreateTrackInternalVector3(paramType);
            break;
        case eUiAnimValue_Vector4:
            pTrack = CreateTrackInternalVector4(paramType);
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
IUiAnimTrack* CUiAnimNode::CreateTrack(const CUiAnimParamType& paramType)
{
    IUiAnimTrack* pTrack = CreateTrackInternal(paramType, DEFAULT_TRACK_TYPE, eUiAnimValue_Unknown);
    return pTrack;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::SerializeUiAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Animation, 0, "CUiAnimNode");

    if (bLoading)
    {
        // Delete all tracks.
        stl::free_container(m_tracks);

        CUiAnimNode::SParamInfo info;
        // Loading.
        int paramTypeVersion = 0;
        xmlNode->getAttr("paramIdVersion", paramTypeVersion);
        CUiAnimParamType paramType;
        int num = xmlNode->getChildCount();
        for (int i = 0; i < num; i++)
        {
            XmlNodeRef trackNode = xmlNode->getChild(i);
            paramType.Serialize(GetUiAnimationSystem(), trackNode, bLoading, paramTypeVersion);

            MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Animation, 0, UiAnimationSystem::GetParamTypeName(paramType));

            int curveType = eUiAnimCurveType_Unknown;
            trackNode->getAttr("Type", curveType);

            int valueType = eUiAnimValue_Unknown;
            trackNode->getAttr("ValueType", valueType);

            IUiAnimTrack* pTrack = CreateTrackInternal(paramType, (EUiAnimCurveType)curveType, (EUiAnimValue)valueType);
            if (pTrack)
            {
                UiAnimParamData paramData;
                paramData.Serialize(GetUiAnimationSystem(), trackNode, bLoading);
                pTrack->SetParamData(paramData);

                if (!pTrack->Serialize(GetUiAnimationSystem(), trackNode, bLoading, bLoadEmptyTracks))
                {
                    // Boolean tracks must always be loaded even if empty.
                    if (pTrack->GetValueType() != eUiAnimValue_Bool)
                    {
                        RemoveTrack(pTrack);
                    }
                }
            }
        }
    }
    else
    {
        // Saving.
        xmlNode->setAttr("paramIdVersion", CUiAnimParamType::kParamTypeVersion);
        for (unsigned int i = 0; i < m_tracks.size(); i++)
        {
            IUiAnimTrack* pTrack = m_tracks[i].get();
            if (pTrack)
            {
                CUiAnimParamType paramType = pTrack->GetParameterType();
                XmlNodeRef trackNode = xmlNode->newChild("Track");
                paramType.Serialize(GetUiAnimationSystem(), trackNode, bLoading);

                UiAnimParamData paramData = pTrack->GetParamData();
                paramData.Serialize(GetUiAnimationSystem(), trackNode, bLoading);

                int nTrackType = pTrack->GetCurveType();
                trackNode->setAttr("Type", nTrackType);
                pTrack->Serialize(GetUiAnimationSystem(), trackNode, bLoading);
                int valueType = pTrack->GetValueType();
                trackNode->setAttr("ValueType", valueType);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::SetTimeRange(Range timeRange)
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
CUiAnimNode::CUiAnimNode(const int id)
    : m_refCount(0)
    , m_id(id)
    , m_parentNodeId(0)
{
    m_pOwner = 0;
    m_pSequence = 0;
    m_flags = 0;
    m_bIgnoreSetParam = false;
    m_pParentNode = 0;
    m_nLoadedParentNodeId = 0;
}

//////////////////////////////////////////////////////////////////////////
// AZ::Serialization requires a default constructor
CUiAnimNode::CUiAnimNode()
    : CUiAnimNode(0)
{
}

//////////////////////////////////////////////////////////////////////////
CUiAnimNode::~CUiAnimNode()
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::SetFlags(int flags)
{
    m_flags = flags;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimNode::GetFlags() const
{
    return m_flags;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::Animate(SUiAnimContext& ec)
{
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::IsParamValid(const CUiAnimParamType& paramType) const
{
    SParamInfo info;

    if (GetParamInfoFromType(paramType, info))
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::SetParamValue(float time, CUiAnimParamType param, float value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    IUiAnimTrack* pTrack = GetTrackForParameter(param);
    if (pTrack && pTrack->GetValueType() == eUiAnimValue_Float)
    {
        // Float track.
        bool bDefault = !(GetUiAnimationSystem()->IsRecording() && (m_flags & eUiAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
        pTrack->SetValue(time, value, bDefault);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::SetParamValue(float time, CUiAnimParamType param, const Vec3& value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    UiCompoundSplineTrack* pTrack = static_cast<UiCompoundSplineTrack*>(GetTrackForParameter(param));
    if (pTrack && pTrack->GetValueType() == eUiAnimValue_Vector)
    {
        // Vec3 track.
        bool bDefault = !(GetUiAnimationSystem()->IsRecording() && (m_flags & eUiAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
        pTrack->SetValue(time, value, bDefault);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::SetParamValue(float time, CUiAnimParamType param, const Vec4& value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    UiCompoundSplineTrack* pTrack = static_cast<UiCompoundSplineTrack*>(GetTrackForParameter(param));
    if (pTrack && pTrack->GetValueType() == eUiAnimValue_Vector4)
    {
        // Vec4 track.
        bool bDefault = !(GetUiAnimationSystem()->IsRecording() && (m_flags & eUiAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded
        pTrack->SetValue(time, value, bDefault);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::GetParamValue(float time, CUiAnimParamType param, float& value)
{
    IUiAnimTrack* pTrack = GetTrackForParameter(param);
    if (pTrack && pTrack->GetValueType() == eUiAnimValue_Float && pTrack->GetNumKeys() > 0)
    {
        // Float track.
        pTrack->GetValue(time, value);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::GetParamValue(float time, CUiAnimParamType param, Vec3& value)
{
    UiCompoundSplineTrack* pTrack = static_cast<UiCompoundSplineTrack*>(GetTrackForParameter(param));
    if (pTrack && pTrack->GetValueType() == eUiAnimValue_Vector && pTrack->GetNumKeys() > 0)
    {
        // Vec3 track.
        pTrack->GetValue(time, value);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimNode::GetParamValue(float time, CUiAnimParamType param, Vec4& value)
{
    UiCompoundSplineTrack* pTrack = static_cast<UiCompoundSplineTrack*>(GetTrackForParameter(param));
    if (pTrack && pTrack->GetValueType() == eUiAnimValue_Vector4 && pTrack->GetNumKeys() > 0)
    {
        // Vec4 track.
        pTrack->GetValue(time, value);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    if (bLoading)
    {
        xmlNode->getAttr("Id", m_id);
        const char* name = xmlNode->getAttr("Name");
        int flags;
        if (xmlNode->getAttr("Flags", flags))
        {
            // Don't load expanded or selected flags
            flags = flags & ~(eUiAnimNodeFlags_Expanded | eUiAnimNodeFlags_EntitySelected);
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

        EUiAnimNodeType nodeType = GetType();
        static_cast<UiAnimationSystem*>(GetUiAnimationSystem())->SerializeNodeType(nodeType, xmlNode, bLoading, IUiAnimSequence::kSequenceVersion, m_flags);

        xmlNode->setAttr("Name", GetName());

        // Don't store expanded or selected flags
        int flags = GetFlags() & ~(eUiAnimNodeFlags_Expanded | eUiAnimNodeFlags_EntitySelected);
        xmlNode->setAttr("Flags", flags);

        if (m_pParentNode)
        {
            xmlNode->setAttr("ParentNode", static_cast<CUiAnimNode*>(m_pParentNode)->GetId());
        }
    }

    SerializeUiAnims(xmlNode, bLoading, bLoadEmptyTracks);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::InitPostLoad(IUiAnimSequence* pSequence, bool remapIds, LyShine::EntityIdMap* entityIdMap)
{
    m_pSequence = pSequence;
    m_pParentNode = ((CUiAnimSequence*)m_pSequence)->FindNodeById(m_parentNodeId);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::SetNodeOwner(IUiAnimNodeOwner* pOwner)
{
    m_pOwner = pOwner;

    if (pOwner)
    {
        pOwner->OnNodeUiAnimated(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::PostLoad()
{
    if (m_nLoadedParentNodeId)
    {
        IUiAnimNode* pParentNode = ((CUiAnimSequence*)m_pSequence)->FindNodeById(m_nLoadedParentNodeId);
        m_pParentNode = pParentNode;
        m_parentNodeId = m_nLoadedParentNodeId; // adding as a temporary fix while we support both serialization methods
        m_nLoadedParentNodeId = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
Matrix34 CUiAnimNode::GetReferenceMatrix() const
{
    static Matrix34 tm(IDENTITY);
    return tm;
}

IUiAnimTrack* CUiAnimNode::CreateTrackInternalFloat(int trackType) const
{
    // Don't need trackType any more
    return aznew C2DSplineTrack;
}

IUiAnimTrack* CUiAnimNode::CreateTrackInternalVector(EUiAnimCurveType trackType, const CUiAnimParamType& paramType, const EUiAnimValue animValue) const
{
    // Don't need trackType any more

    CUiAnimParamType subTrackParamTypes[MAX_SUBTRACKS];
    for (unsigned int i = 0; i < MAX_SUBTRACKS; ++i)
    {
        subTrackParamTypes[i] = eUiAnimParamType_AzComponentField;
    }

    return aznew UiCompoundSplineTrack(3, eUiAnimValue_Vector, subTrackParamTypes);
}

IUiAnimTrack* CUiAnimNode::CreateTrackInternalQuat(EUiAnimCurveType trackType, const CUiAnimParamType& paramType) const
{
    // UI_ANIMATION_REVISIT - my may want quat support at some point
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* CUiAnimNode::CreateTrackInternalVector2(const CUiAnimParamType& paramType) const
{
    IUiAnimTrack* pTrack;

    CUiAnimParamType subTrackParamTypes[MAX_SUBTRACKS];
    for (unsigned int i = 0; i < MAX_SUBTRACKS; ++i)
    {
        subTrackParamTypes[i] = eUiAnimParamType_Float;
    }

    pTrack = aznew UiCompoundSplineTrack(2, eUiAnimValue_Vector2, subTrackParamTypes);

    return pTrack;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* CUiAnimNode::CreateTrackInternalVector3(const CUiAnimParamType& paramType) const
{
    IUiAnimTrack* pTrack;

    CUiAnimParamType subTrackParamTypes[MAX_SUBTRACKS];
    for (unsigned int i = 0; i < MAX_SUBTRACKS; ++i)
    {
        subTrackParamTypes[i] = eUiAnimParamType_Float;
    }

    pTrack = aznew UiCompoundSplineTrack(3, eUiAnimValue_Vector3, subTrackParamTypes);

    return pTrack;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* CUiAnimNode::CreateTrackInternalVector4(const CUiAnimParamType& paramType) const
{
    IUiAnimTrack* pTrack;

    CUiAnimParamType subTrackParamTypes[MAX_SUBTRACKS];
    for (unsigned int i = 0; i < MAX_SUBTRACKS; ++i)
    {
        subTrackParamTypes[i] = eUiAnimParamType_Float;
    }

    pTrack = aznew UiCompoundSplineTrack(4, eUiAnimValue_Vector4, subTrackParamTypes);

    return pTrack;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimNode::SetParent(IUiAnimNode* pParent)
{
    m_pParentNode = pParent;
    if (pParent)
    {
        m_parentNodeId = static_cast<CUiAnimNode*>(m_pParentNode)->GetId();
    }
    else
    {
        m_parentNodeId = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimNode::HasDirectorAsParent() const
{
    IUiAnimNode* pParent = GetParent();
    while (pParent)
    {
        if (pParent->GetType() == eUiAnimNodeType_Director)
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
