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
#include <AzCore/Serialization/SerializeContext.h>
#include "EntityNode.h"
#include "EventTrack.h"
#include "AnimSplineTrack.h"
#include "BoolTrack.h"
#include "CharacterTrack.h"
#include "ISystem.h"
#include "SelectTrack.h"
#include "LookAtTrack.h"
#include "CompoundSplineTrack.h"
#include "Movie.h"
#include "PNoise3.h"
#include "AnimSequence.h"

#include <IAudioSystem.h>
#include <ILipSync.h>
#include <ICryAnimation.h>
#include <CryCharMorphParams.h>
#include <Cry_Camera.h>

// AI
#include <IAgent.h>
#include "IAIObject.h"
#include "IAIActor.h"
#include "IGameFramework.h"
#include "MannequinTrack.h"
#include "../CryAction/IActorSystem.h"
#include "../CryAction/ICryMannequinDefs.h"
#include "../CryAction/ICryMannequin.h"
#include "../CryAction/IAnimatedCharacter.h"

#include <IEntityHelper.h>
#include "Components/IComponentEntityNode.h"
#include "Components/IComponentAudio.h"
#include "Components/IComponentPhysics.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"


#define s_nodeParamsInitialized s_nodeParamsInitializedEnt
#define s_nodeParams s_nodeParamsEnt
#define AddSupportedParam AddSupportedParamEnt

static const float EPSILON = 0.01f;
static float movie_physicalentity_animation_lerp;

static const char* s_VariablePrefixes[] =
{
    "n", "i", "b", "f", "s", "ei", "es",
    "shader", "clr", "color", "vector",
    "snd", "sound", "dialog", "tex", "texture",
    "obj", "object", "file", "aibehavior",
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
    "aicharacter",
#endif
    "aipfpropertieslist", "aientityclasses", "aiterritory",
    "aiwave", "text", "equip", "reverbpreset", "eaxpreset",
    "aianchor", "soclass", "soclasses", "sostate", "sostates"
    "sopattern", "soaction", "sohelper", "sonavhelper",
    "soanimhelper", "soevent", "sotemplate", "customaction",
    "gametoken", "seq_", "mission_", "seqid_", "lightanimation_"
};

namespace BoneNames
{
    const char* Head = "Bip01 Head";
}

//////////////////////////////////////////////////////////////////////////
namespace
{
    const char* kScriptTablePrefix = "ScriptTable:";

    bool s_nodeParamsInitialized = false;
    std::vector<CAnimNode::SParamInfo> s_nodeParams;

    void AddSupportedParam(std::vector<CAnimNode::SParamInfo>& nodeParams, const char* sName, AnimParamType paramId, AnimValueType valueType, int flags = 0)
    {
        CAnimNode::SParamInfo param;
        param.name = sName;
        param.paramType = paramId;
        param.valueType = valueType;
        param.flags = (IAnimNode::ESupportedParamFlags)flags;
        nodeParams.push_back(param);
    }

    // Quat::IsEquivalent has numerical problems with very similar values
    bool CompareRotation(const Quat& q1, const Quat& q2, float epsilon)
    {
        return (fabs_tpl(q1.v.x - q2.v.x) <= epsilon)
               && (fabs_tpl(q1.v.y - q2.v.y) <= epsilon)
               && (fabs_tpl(q2.v.z - q2.v.z) <= epsilon)
               && (fabs_tpl(q1.w - q2.w) <= epsilon);
    }
};

//////////////////////////////////////////////////////////////////////////
CAnimEntityNode::CAnimEntityNode()
    : CAnimEntityNode(0, AnimNodeType::Entity)
{
}

//////////////////////////////////////////////////////////////////////////
CAnimEntityNode::CAnimEntityNode(const int id, AnimNodeType nodeType)
    : CAnimNode(id, nodeType)
{
    m_EntityId = 0;
    m_entityGuid = 0;
    m_target = NULL;
    m_bInitialPhysicsStatus = false;

    m_pos(0, 0, 0);
    m_scale(1, 1, 1);
    m_rotate.SetIdentity();

    m_visible = true;

    m_time = 0.0f;

    m_lastEntityKey = -1;

    m_lookAtTarget = "";
    m_lookAtEntityId = 0;
    m_lookAtLocalPlayer = false;
    m_allowAdditionalTransforms = true;
    m_lookPose = "";

    m_proceduralFacialAnimationEnabledOld = true;

    m_entityGuidTarget = 0;
    m_EntityIdTarget = 0;
    m_entityGuidSource = 0;
    m_EntityIdSource = 0;

    #ifdef CHECK_FOR_TOO_MANY_ONPROPERTY_SCRIPT_CALLS
    m_OnPropertyCalls = 0;
    #endif

    m_vInterpPos(0, 0, 0);
    m_interpRot.SetIdentity();
    SetSkipInterpolatedCameraNode(false);

    m_iCurMannequinKey = -1;

    CAnimEntityNode::Initialize();

    SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::Initialize()
{
    if (!s_nodeParamsInitialized)
    {
        s_nodeParamsInitialized = true;
        s_nodeParams.reserve(14);
        AddSupportedParam(s_nodeParams, "Position", AnimParamType::Position, AnimValueType::Vector);
        AddSupportedParam(s_nodeParams, "Rotation", AnimParamType::Rotation, AnimValueType::Quat);
        AddSupportedParam(s_nodeParams, "Scale", AnimParamType::Scale, AnimValueType::Vector);
        AddSupportedParam(s_nodeParams, "Visibility", AnimParamType::Visibility, AnimValueType::Bool);
        AddSupportedParam(s_nodeParams, "Event", AnimParamType::Event, AnimValueType::Unknown);
        AddSupportedParam(s_nodeParams, "Sound", AnimParamType::Sound, AnimValueType::Unknown, eSupportedParamFlags_MultipleTracks);
        AddSupportedParam(s_nodeParams, "Animation", AnimParamType::Animation, AnimValueType::CharacterAnim, eSupportedParamFlags_MultipleTracks);
        AddSupportedParam(s_nodeParams, "Mannequin", AnimParamType::Mannequin, AnimValueType::Unknown, eSupportedParamFlags_MultipleTracks);
        AddSupportedParam(s_nodeParams, "LookAt", AnimParamType::LookAt, AnimValueType::Unknown);
        AddSupportedParam(s_nodeParams, "Noise", AnimParamType::TransformNoise, AnimValueType::Vector4);
        AddSupportedParam(s_nodeParams, "Physicalize", AnimParamType::Physicalize, AnimValueType::Bool);
        AddSupportedParam(s_nodeParams, "PhysicsDriven", AnimParamType::PhysicsDriven, AnimValueType::Bool);
        AddSupportedParam(s_nodeParams, "Procedural Eyes", AnimParamType::ProceduralEyes, AnimValueType::Bool);

        REGISTER_CVAR(movie_physicalentity_animation_lerp, 0.85f, 0, "Lerp value for animation-driven physical entities");
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::AddTrack(IAnimTrack* pTrack)
{
    const CAnimParamType& paramType = pTrack->GetParameterType();

    if (paramType == AnimParamType::PhysicsDriven)
    {
        CBoolTrack* pPhysicsDrivenTrack = static_cast<CBoolTrack*>(pTrack);
        pPhysicsDrivenTrack->SetDefaultValue(false);
    }
    else if (paramType == AnimParamType::ProceduralEyes)
    {
        CBoolTrack* pProceduralEyesTrack = static_cast<CBoolTrack*>(pTrack);
        pProceduralEyesTrack->SetDefaultValue(false);
    }

    CAnimNode::AddTrack(pTrack);
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::UpdateDynamicParamsInternal()
{
    m_entityScriptPropertiesParamInfos.clear();
    m_nameToScriptPropertyParamInfo.clear();

    // editor stores *all* properties of *every* entity used in an AnimEntityNode, including to-display names, full lua paths, string maps for fast access, etc.
    // in pure game mode we just need to store the properties that we know are going to be used in a track, so we can save a lot of memory.
    if (gEnv->IsEditor())
    {
        UpdateDynamicParams_Editor();
    }
    else
    {
        UpdateDynamicParams_PureGame();
    }
}


//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::UpdateDynamicParams_Editor()
{
    IEntity* pEntity = GetEntity();
    // on level reload in editor we may try to update dynamic parameters for an Entity is garbage and is soon to be removed,
    // leading to a crash if !pEntity->IsGarbage() is not checked.
    if (pEntity && !pEntity->IsGarbage())
    {
        IScriptTable* pScriptTable = pEntity->GetScriptTable();

        if (!pScriptTable)
        {
            return;
        }

        SmartScriptTable propertiesTable;
        if (pScriptTable->GetValue("Properties", propertiesTable))
        {
            FindDynamicPropertiesRec(propertiesTable, "Properties/", 0);
        }

        SmartScriptTable propertiesInstanceTable;
        if (pScriptTable->GetValue("PropertiesInstance", propertiesInstanceTable))
        {
            FindDynamicPropertiesRec(propertiesInstanceTable, "PropertiesInstance/", 0);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::UpdateDynamicParams_PureGame()
{
    IEntity* pEntity = GetEntity();
    if (!pEntity)
    {
        return;
    }

    IScriptTable* pEntityScriptTable = pEntity->GetScriptTable();
    if (!pEntityScriptTable)
    {
        return;
    }

    for (uint32 i = 0; i < m_tracks.size(); ++i)
    {
        AZStd::intrusive_ptr<IAnimTrack> pTrack = m_tracks[i];
        CAnimParamType paramType(pTrack->GetParameterType());
        if (paramType.GetType() != AnimParamType::ByString)
        {
            continue;
        }

        string paramName = paramType.GetName();
        string scriptTablePrefix = kScriptTablePrefix;
        if (scriptTablePrefix != paramName.Left(scriptTablePrefix.size()))
        {
            continue;
        }

        string path = paramName.Right(paramName.size() - scriptTablePrefix.size());

        string propertyName;
        SmartScriptTable propertyScriptTable;
        FindScriptTableForParameterRec(pEntityScriptTable, path, propertyName, propertyScriptTable);
        if (propertyScriptTable && !propertyName.empty())
        {
            SScriptPropertyParamInfo paramInfo;
            bool isUnknownTable = ObtainPropertyTypeInfo(propertyName.c_str(), propertyScriptTable, paramInfo);
            if (paramInfo.animNodeParamInfo.valueType == AnimValueType::Unknown)
            {
                return;
            }

            string strippedPath = path.Left(path.size() - propertyName.size());
            AddPropertyToParamInfoMap(propertyName.c_str(), strippedPath, paramInfo);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::FindScriptTableForParameterRec(IScriptTable* pScriptTable, const string& path, string& propertyName, SmartScriptTable& propertyScriptTable)
{
    size_t pos = path.find_first_of('/');
    if (pos == string::npos)
    {
        propertyName = path;
        propertyScriptTable = pScriptTable;
        return;
    }
    string tableName = path.Left(pos);
    pos++;
    string pathLeft = path.Right(path.size() - pos);

    ScriptAnyValue value;
    pScriptTable->GetValueAny(tableName.c_str(), value);
    assert(value.type == ANY_TTABLE);
    if (value.type == ANY_TTABLE)
    {
        FindScriptTableForParameterRec(value.table, pathLeft, propertyName, propertyScriptTable);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::FindDynamicPropertiesRec(IScriptTable* pScriptTable, const string& currentPath, unsigned int depth)
{
    // There might be infinite recursion in the tables, so we need to limit the max recursion depth
    const unsigned int kMaxRecursionDepth = 5;

    IScriptTable::Iterator iter = pScriptTable->BeginIteration();

    while (pScriptTable->MoveNext(iter))
    {
        if (!iter.sKey || iter.sKey[0] == '_')  // Skip properties that start with an underscore
        {
            continue;
        }

        SScriptPropertyParamInfo paramInfo;
        bool isUnknownTable = ObtainPropertyTypeInfo(iter.sKey, pScriptTable, paramInfo);

        if (isUnknownTable && depth < kMaxRecursionDepth)
        {
            FindDynamicPropertiesRec(paramInfo.scriptTable, currentPath + iter.sKey + "/", depth + 1);
            continue;
        }

        if (paramInfo.animNodeParamInfo.valueType != AnimValueType::Unknown)
        {
            AddPropertyToParamInfoMap(iter.sKey, currentPath, paramInfo);
        }
    }
    pScriptTable->EndIteration(iter);
}


//////////////////////////////////////////////////////////////////////////
// fills some fields in paramInfo with the appropriate value related to the property defined by pScriptTable and pKey.
// returns true if the property is a table that should be parsed, instead of an atomic type  (simple vectors are treated like atomic types)
bool CAnimEntityNode::ObtainPropertyTypeInfo(const char* pKey, IScriptTable* pScriptTable, SScriptPropertyParamInfo& paramInfo)
{
    ScriptAnyValue value;
    pScriptTable->GetValueAny(pKey, value);
    paramInfo.scriptTable = pScriptTable;
    paramInfo.isVectorTable = false;
    paramInfo.animNodeParamInfo.valueType = AnimValueType::Unknown;
    bool isUnknownTable = false;

    switch (value.type)
    {
    case ANY_TNUMBER:
    {
        const bool hasBoolPrefix = (strlen(pKey) > 1) && (pKey[0] == 'b')
            && (pKey[1] != tolower(pKey[1]));
        paramInfo.animNodeParamInfo.valueType = hasBoolPrefix ? AnimValueType::Bool : AnimValueType::Float;
    }
    break;
    case ANY_TVECTOR:
        paramInfo.animNodeParamInfo.valueType = AnimValueType::Vector;
        break;
    case ANY_TBOOLEAN:
        paramInfo.animNodeParamInfo.valueType = AnimValueType::Bool;
        break;
    case ANY_TTABLE:
        // Threat table as vector if it contains x, y & z
        paramInfo.scriptTable = value.table;
        if (value.table->HaveValue("x") && value.table->HaveValue("y") && value.table->HaveValue("z"))
        {
            paramInfo.animNodeParamInfo.valueType = AnimValueType::Vector;
            paramInfo.scriptTable = value.table;
            paramInfo.isVectorTable = true;
        }
        else
        {
            isUnknownTable = true;
        }
        break;
    }

    return isUnknownTable;
}


//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::AddPropertyToParamInfoMap(const char* pKey, const string& currentPath, SScriptPropertyParamInfo& paramInfo)
{
    // Strip variable name prefix
    const char* strippedName = pKey;
    while (*strippedName && (*strippedName == tolower(*strippedName) || *strippedName == '_'))
    {
        ++strippedName;
    }
    const size_t prefixLength = strippedName - pKey;

    // Check if stripped prefix is in list of valid variable prefixes
    bool foundPrefix = false;
    for (size_t i = 0; i < sizeof(s_VariablePrefixes) / sizeof(const char*); ++i)
    {
        if ((strlen(s_VariablePrefixes[i]) == prefixLength) &&
            (memcmp(pKey, s_VariablePrefixes[i], prefixLength) == 0))
        {
            foundPrefix = true;
            break;
        }
    }

    // Only use stripped name if prefix is in list, otherwise use full name
    strippedName = foundPrefix ? strippedName : pKey;

    // If it is a vector check if we need to create a color track
    if (paramInfo.animNodeParamInfo.valueType == AnimValueType::Vector)
    {
        if ((prefixLength == 3 && memcmp(pKey, "clr", 3) == 0)
            || (prefixLength == 5 && memcmp(pKey, "color", 5) == 0))
        {
            paramInfo.animNodeParamInfo.valueType = AnimValueType::RGB;
        }
    }

    paramInfo.variableName = pKey;
    paramInfo.displayName = currentPath + strippedName;
    paramInfo.animNodeParamInfo.name = &paramInfo.displayName[0];
    const string paramIdStr = kScriptTablePrefix + currentPath + pKey;
    paramInfo.animNodeParamInfo.paramType = CAnimParamType(paramIdStr);
    paramInfo.animNodeParamInfo.flags = eSupportedParamFlags_MultipleTracks;
    m_entityScriptPropertiesParamInfos.push_back(paramInfo);

    m_nameToScriptPropertyParamInfo[paramIdStr] = m_entityScriptPropertiesParamInfos.size() - 1;
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType)
{
    // Initialize new track to property value
    if (paramType.GetType() == AnimParamType::ByString && pTrack && (strncmp(paramType.GetName(), kScriptTablePrefix, strlen(kScriptTablePrefix)) == 0))
    {
        IEntity* pEntity = GetEntity();
        if (!pEntity)
        {
            return;
        }

        TScriptPropertyParamInfoMap::iterator findIter = m_nameToScriptPropertyParamInfo.find(paramType.GetName());
        if (findIter != m_nameToScriptPropertyParamInfo.end())
        {
            SScriptPropertyParamInfo& param = m_entityScriptPropertiesParamInfos[findIter->second];

            float fValue = 0.0f;
            Vec3 vecValue = Vec3(0.0f, 0.0f, 0.0f);
            bool boolValue = false;

            switch (pTrack->GetValueType())
            {
            case AnimValueType::Float:
                param.scriptTable->GetValue(param.variableName, fValue);
                pTrack->SetValue(0, fValue, true);
                break;
            case AnimValueType::Vector:
            // fall through
            case AnimValueType::RGB:
                if (param.isVectorTable)
                {
                    param.scriptTable->GetValue("x", vecValue.x);
                    param.scriptTable->GetValue("y", vecValue.y);
                    param.scriptTable->GetValue("z", vecValue.z);
                }
                else
                {
                    param.scriptTable->GetValue(param.variableName, vecValue);
                }

                if (pTrack->GetValueType() == AnimValueType::RGB)
                {
                    vecValue.x = clamp_tpl(vecValue.x, 0.0f, 1.0f);
                    vecValue.y = clamp_tpl(vecValue.y, 0.0f, 1.0f);
                    vecValue.z = clamp_tpl(vecValue.z, 0.0f, 1.0f);

                    vecValue *= 255.0f;
                }

                pTrack->SetValue(0, vecValue, true);
                break;
            case AnimValueType::Bool:
                param.scriptTable->GetValue(param.variableName, boolValue);
                pTrack->SetValue(0, boolValue, true);
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::CreateDefaultTracks()
{
    // Default tracks for Entities are controlled through the toolbar menu
    // in MannequinDialog.
}

//////////////////////////////////////////////////////////////////////////
CAnimEntityNode::~CAnimEntityNode()
{
    ReleaseSounds();
    if (m_characterTrackAnimator)
    {
        delete m_characterTrackAnimator;
        m_characterTrackAnimator = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
unsigned int CAnimEntityNode::GetParamCount() const
{
    return CAnimEntityNode::GetParamCountStatic() + m_entityScriptPropertiesParamInfos.size();
}

//////////////////////////////////////////////////////////////////////////
CAnimParamType CAnimEntityNode::GetParamType(unsigned int nIndex) const
{
    SParamInfo info;

    if (!CAnimEntityNode::GetParamInfoStatic(nIndex, info))
    {
        const uint scriptParamsOffset = (uint)s_nodeParams.size();
        const uint end = (uint)s_nodeParams.size() + (uint)m_entityScriptPropertiesParamInfos.size();

        if (nIndex >= scriptParamsOffset && nIndex < end)
        {
            return m_entityScriptPropertiesParamInfos[nIndex - scriptParamsOffset].animNodeParamInfo.paramType;
        }

        return AnimParamType::Invalid;
    }

    return info.paramType;
}

//////////////////////////////////////////////////////////////////////////
int CAnimEntityNode::GetParamCountStatic()
{
    return s_nodeParams.size();
}

//////////////////////////////////////////////////////////////////////////
bool CAnimEntityNode::GetParamInfoStatic(int nIndex, SParamInfo& info)
{
    if (nIndex >= 0 && nIndex < (int)s_nodeParams.size())
    {
        info = s_nodeParams[nIndex];
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimEntityNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    for (int i = 0; i < (int)s_nodeParams.size(); i++)
    {
        if (s_nodeParams[i].paramType == paramId)
        {
            info = s_nodeParams[i];
            return true;
        }
    }

    for (size_t i = 0; i < m_entityScriptPropertiesParamInfos.size(); ++i)
    {
        if (m_entityScriptPropertiesParamInfos[i].animNodeParamInfo.paramType == paramId)
        {
            info = m_entityScriptPropertiesParamInfos[i].animNodeParamInfo;
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
const char* CAnimEntityNode::GetParamName(const CAnimParamType& param) const
{
    SParamInfo info;
    if (GetParamInfoFromType(param, info))
    {
        return info.name;
    }

    const char* pName = param.GetName();
    if (param.GetType() == AnimParamType::ByString && pName && strncmp(pName, kScriptTablePrefix, strlen(kScriptTablePrefix)) == 0)
    {
        return pName + strlen(kScriptTablePrefix);
    }

    return "Unknown Entity Parameter";
}

//////////////////////////////////////////////////////////////////////////
IEntity* CAnimEntityNode::GetEntity()
{
    if (!m_EntityId)
    {
        m_EntityId = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuid);
    }
    return gEnv->pEntitySystem->GetEntity(m_EntityId);
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetEntityGuid(const EntityGUID& guid)
{
    m_entityGuid = guid;
    m_EntityId = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuid);
    UpdateDynamicParams();
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetEntityGuidTarget(const EntityGUID& guid)
{
    m_entityGuidTarget = guid;
    m_EntityIdTarget = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuidTarget);
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetEntityId(const int entityId)
{
    m_EntityId = entityId;
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetEntityGuidSource(const EntityGUID& guid)
{
    m_entityGuidSource = guid;
    m_EntityIdSource = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuidSource);
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::StillUpdate()
{
    IEntity* pEntity = GetEntity();
    if (!pEntity)
    {
        return;
    }

    int paramCount = NumTracks();
    for (int paramIndex = 0; paramIndex < paramCount; paramIndex++)
    {
        IAnimTrack* pTrack = m_tracks[paramIndex].get();

        switch (m_tracks[paramIndex]->GetParameterType().GetType())
        {
        case AnimParamType::LookAt:
        {
            SAnimContext ec;
            ec.time = m_time;

            CLookAtTrack* pSelTrack = (CLookAtTrack*)pTrack;
            AnimateLookAt(pSelTrack, ec);
        }
        break;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::EnableEntityPhysics(bool bEnable)
{
    if (IEntity* pEntity = GetEntity())
    {
        IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>();
        if (pPhysicsComponent && pPhysicsComponent->IsPhysicsEnabled() != bEnable)
        {
            pPhysicsComponent->EnablePhysics(bEnable);
            if (bEnable)
            {
                // Force xform update to move physics component. Physics ignores updates while physics are disabled.
                SEntityEvent event(ENTITY_EVENT_XFORM);
                event.nParam[0] = ENTITY_XFORM_POS | ENTITY_XFORM_ROT | ENTITY_XFORM_SCL;
                pEntity->SendEvent(event);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::Animate(SAnimContext& ec)
{
    IEntity* pEntity = GetEntity();
    if (!pEntity)
    {
        return;
    }

    pEntity->GetOrCreateComponent<IComponentEntityNode>();

    Vec3 pos = m_pos;
    Quat rotate = m_rotate;
    Vec3 scale = m_scale;
    Vec4 noiseParam(0.f, 0.f, 0.f, 0.f);

    int entityUpdateFlags = 0;
    bool bScaleModified = false;
    bool bApplyNoise = false;
    bool bScriptPropertyModified = false;
    bool bForceEntityActivation = false;

    IAnimTrack* pPosTrack = NULL;
    IAnimTrack* pRotTrack = NULL;
    IAnimTrack* sclTrack = NULL;

    PrecacheDynamic(ec.time);

    int nAnimCharacterLayer = 0, iAnimationTrack = 0;
    size_t nNumAudioTracks = 0;
    int trackCount = NumTracks();
    for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
    {
        CAnimParamType paramType = m_tracks[paramIndex]->GetParameterType();
        IAnimTrack* pTrack = m_tracks[paramIndex].get();
        // always evaluate Boolean or interpolated camera tracks, even if it has no keys. Boolean tracks may differ from their
        // non-animate state, and interpolated cameras may have animation depsite having no keys.
        bool alwaysEvaluateTrack = pTrack->GetValueType() == AnimValueType::Bool || IsSkipInterpolatedCameraNodeEnabled();
        if ((pTrack->HasKeys() == false && !alwaysEvaluateTrack)
            || (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled)
            || pTrack->IsMasked(ec.trackMask))
        {
            continue;
        }

        switch (paramType.GetType())
        {
        case AnimParamType::Position:
            pPosTrack = pTrack;
            if (!IsSkipInterpolatedCameraNodeEnabled())
            {
                pPosTrack->GetValue(ec.time, pos);
                if (!Vec3::IsEquivalent(pos, pEntity->GetPos(), 0.0001f))
                {
                    entityUpdateFlags |= eUpdateEntity_Position;
                }
            }
            else
            {
                if (!Vec3::IsEquivalent(pos, pEntity->GetPos(), 0.0001f))
                {
                    entityUpdateFlags |= eUpdateEntity_Position;
                    pos = m_vInterpPos;
                }
            }

            break;
        case AnimParamType::Rotation:
            pRotTrack = pTrack;
            if (!IsSkipInterpolatedCameraNodeEnabled())
            {
                pRotTrack->GetValue(ec.time, rotate);
                if (!CompareRotation(rotate, pEntity->GetRotation(), DEG2RAD(0.0001f)))
                {
                    entityUpdateFlags |= eUpdateEntity_Rotation;
                }
            }
            else
            {
                if (!CompareRotation(rotate, pEntity->GetRotation(), DEG2RAD(0.0001f)))
                {
                    entityUpdateFlags |= eUpdateEntity_Rotation;
                    rotate = m_interpRot;
                }
            }

            break;
        case AnimParamType::TransformNoise:
            static_cast<CCompoundSplineTrack*>(pTrack)->GetValue(ec.time, noiseParam);
            m_posNoise.m_amp = noiseParam.x;
            m_posNoise.m_freq = noiseParam.y;
            m_rotNoise.m_amp = noiseParam.z;
            m_rotNoise.m_freq = noiseParam.w;
            bApplyNoise = true;
            break;
        case AnimParamType::Scale:
            sclTrack = pTrack;
            sclTrack->GetValue(ec.time, scale);
            // Check whether the scale value is valid.
            if (scale.x < 0.01 || scale.y < 0.01 || scale.z < 0.01)
            {
                CryWarning(VALIDATOR_MODULE_MOVIE, VALIDATOR_WARNING,
                    "An EntityNode <%s> gets an invalid scale (%f,%f,%f) from a TrackView track, so ignored.",
                    (const char*)GetName(), scale.x, scale.y, scale.z);
                scale = m_scale;
            }
            if (!Vec3::IsEquivalent(scale, pEntity->GetScale(), 0.001f))
            {
                bScaleModified = true;
            }
            break;
        case AnimParamType::Event:
            if (!ec.bResetting)
            {
                CEventTrack* pEntityTrack = (CEventTrack*)pTrack;
                IEventKey key;
                int entityKey = pEntityTrack->GetActiveKey(ec.time, &key);

                // If key is different or if time is standing exactly on key time.
                if (entityKey != m_lastEntityKey || key.time == ec.time)
                {
                    m_lastEntityKey = entityKey;

                    if (entityKey >= 0)
                    {
                        const bool bNotTrigger = key.bNoTriggerInScrubbing && ec.bSingleFrame && key.time != ec.time;
                        if (!bNotTrigger)
                        {
                            const bool bRagollizeEvent = strcmp(key.event.c_str(), "Ragdollize") == 0;
                            if (!bRagollizeEvent || GetCMovieSystem()->IsPhysicsEventsEnabled())
                            {
                                ApplyEventKey(pEntityTrack, entityKey, key);
                            }

                            bool bHideOrUnHide = strcmp(key.event.c_str(), "Hide") == 0 || strcmp(key.event.c_str(), "UnHide") == 0;

                            if (m_pOwner && bHideOrUnHide)
                            {
                                m_pOwner->OnNodeVisibilityChanged(this, pEntity->IsHidden());
                            }
                        }
                    }
                }
            }
            break;
        case AnimParamType::Visibility:
            if (!ec.bResetting)
            {
                IAnimTrack* visTrack = pTrack;
                bool visible = m_visible;
                visTrack->GetValue(ec.time, visible);
                if (pEntity->IsHidden() == visible)
                {
                    pEntity->Hide(!visible);
                    if (visible)
                    {
                        IAnimTrack* pPhysicalizeTrack = GetTrackForParameter(AnimParamType::Physicalize);
                        if (pPhysicalizeTrack)
                        {
                            bool bUsePhysics = false;
                            pPhysicalizeTrack->GetValue(m_time, bUsePhysics);
                            EnableEntityPhysics(bUsePhysics);
                        }
                    }
                    m_visible = visible;
                    if (m_pOwner)
                    {
                        m_pOwner->OnNodeVisibilityChanged(this, pEntity->IsHidden());
                    }
                }
            }
            break;
        case AnimParamType::ProceduralEyes:
        {
            if (!ec.bResetting)
            {
                IAnimTrack* procEyeTrack = pTrack;
                bool useProcEyes = false;
                procEyeTrack->GetValue(ec.time, useProcEyes);
                EnableProceduralFacialAnimation(useProcEyes);
            }

            break;
        }
        case AnimParamType::Sound:
            ++nNumAudioTracks;
            if (nNumAudioTracks > m_SoundInfo.size())
            {
                m_SoundInfo.resize(nNumAudioTracks);
            }

            AnimateSound(m_SoundInfo, ec, pTrack, nNumAudioTracks);

            break;

        case AnimParamType::Mannequin:
            if (!ec.bResetting)
            {
                CMannequinTrack* pMannequinTrack = (CMannequinTrack*)pTrack;
                IMannequinKey manKey;
                int key = pMannequinTrack->GetActiveKey(ec.time, &manKey);
                if (key >= 0)
                {
                    int breakhere = 0;

                    //Only activate once
                    if (m_iCurMannequinKey != key)
                    {
                        m_iCurMannequinKey = key;


                        const uint32  valueName = CCrc32::ComputeLowercase(manKey.m_fragmentName.c_str());

                        IGameObject* pGameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(pEntity->GetId());
                        if (pGameObject)
                        {
                            IAnimatedCharacter* pAnimChar = (IAnimatedCharacter*) pGameObject->QueryExtension("AnimatedCharacter");
                            if (pAnimChar && pAnimChar->GetActionController())
                            {
                                const FragmentID fragID = pAnimChar->GetActionController()->GetContext().controllerDef.m_fragmentIDs.Find(valueName);

                                bool isValid = !(FRAGMENT_ID_INVALID == fragID);

                                // Find tags
                                string tagName = manKey.m_tags.c_str();
                                TagState tagState = TAG_STATE_EMPTY;
                                if (!tagName.empty())
                                {
                                    const CTagDefinition* tagDefinition = pAnimChar->GetActionController()->GetTagDefinition(fragID);
                                    tagName.append("+");
                                    while (tagName.find("+") != string::npos)
                                    {
                                        string::size_type strPos = tagName.find("+");

                                        string rest = tagName.substr(strPos + 1, tagName.size());
                                        string curTagName = tagName.substr(0, strPos);
                                        bool found = false;

                                        if (tagDefinition)
                                        {
                                            const int tagCRC = CCrc32::ComputeLowercase(curTagName);
                                            const TagID tagID = tagDefinition->Find(tagCRC);
                                            found = tagID != TAG_ID_INVALID;
                                            tagDefinition->Set(tagState, tagID, true);
                                        }

                                        if (!found)
                                        {
                                            const TagID tagID = pAnimChar->GetActionController()->GetContext().state.GetDef().Find(curTagName);
                                            if (tagID != TAG_ID_INVALID)
                                            {
                                                pAnimChar->GetActionController()->GetContext().state.Set(tagID, true);
                                            }
                                        }

                                        tagName = rest;
                                    }
                                }

                                IAction* pAction = new TAction<SAnimationContext>(manKey.m_priority, fragID, tagState, 0u, 0xffffffff);
                                pAnimChar->GetActionController()->Queue(*pAction);
                            }
                        }
                    }
                }
                else
                {
                    m_iCurMannequinKey = -1;
                }
            }
            break;

        //////////////////////////////////////////////////////////////////////////
        case AnimParamType::Animation:
            if (!ec.bResetting)
            {
                if (!m_characterTrackAnimator)
                {
                    m_characterTrackAnimator = new CCharacterTrackAnimator;
                }

                bForceEntityActivation = true;
                if (nAnimCharacterLayer < MAX_CHARACTER_TRACKS + ADDITIVE_LAYERS_OFFSET)
                {
                    int index = nAnimCharacterLayer;
                    CCharacterTrack* pCharTrack = (CCharacterTrack*)pTrack;
                    if (pCharTrack->GetAnimationLayerIndex() >= 0)   // If the track has an animation layer specified,
                    {
                        assert(pCharTrack->GetAnimationLayerIndex() < ISkeletonAnim::LayerCount);
                        index = pCharTrack->GetAnimationLayerIndex();  // use it instead.
                    }

                    m_characterTrackAnimator->AnimateTrack(pCharTrack, ec, index, iAnimationTrack);

                    if (nAnimCharacterLayer == 0)
                    {
                        nAnimCharacterLayer += ADDITIVE_LAYERS_OFFSET;
                    }
                    ++nAnimCharacterLayer;
                    ++iAnimationTrack;
                }
            }
            break;

        case AnimParamType::LookAt:
            if (!ec.bResetting)
            {
                CLookAtTrack* pSelTrack = (CLookAtTrack*)pTrack;
                AnimateLookAt(pSelTrack, ec);
            }
            break;

        case AnimParamType::ByString:
            if (!ec.bResetting)
            {
                bScriptPropertyModified = AnimateScriptTableProperty(pTrack, ec, paramType.GetName()) || bScriptPropertyModified;
            }
            break;
        }
    }

    if (bApplyNoise)
    {
        // Position noise
        if (m_posNoise.m_amp != 0)
        {
            pos += m_posNoise.Get(ec.time);
            if (pos != m_pos)
            {
                entityUpdateFlags |= eUpdateEntity_Position;
            }
        }

        // Rotation noise
        if (m_rotNoise.m_amp != 0)
        {
            Ang3 angles = Ang3::GetAnglesXYZ(Matrix33(rotate)) * 180.0f / gf_PI;
            Vec3 noiseVec = m_rotNoise.Get(ec.time);
            angles.x += noiseVec.x;
            angles.y += noiseVec.y;
            angles.z += noiseVec.z;
            rotate.SetRotationXYZ(angles * gf_PI / 180.0f);

            if (rotate != m_rotate)
            {
                entityUpdateFlags |= eUpdateEntity_Rotation;
            }
        }
    }

    if (bScriptPropertyModified)
    {
        if (pEntity->GetScriptTable()->HaveValue("OnPropertyChange"))
        {
            Script::CallMethod(pEntity->GetScriptTable(), "OnPropertyChange");
        }
#ifdef CHECK_FOR_TOO_MANY_ONPROPERTY_SCRIPT_CALLS
        m_OnPropertyCalls++;
#endif
    }

    // Update physics status if needed.
    IAnimTrack* pPhysicalizeTrack = GetTrackForParameter(AnimParamType::Physicalize);
    if (pPhysicalizeTrack)
    {
        bool bUsePhysics = false;
        pPhysicalizeTrack->GetValue(m_time, bUsePhysics);
        EnableEntityPhysics(bUsePhysics);
    }

    if (bForceEntityActivation)
    {
        const bool bIsCutScene = (GetSequence()->GetFlags() & IAnimSequence::eSeqFlags_CutScene) != 0;
        if (bIsCutScene)
        {
            // Activate entity to force CEntityObject::Update which calls StartAnimationProcessing for skeletal animations.
            // This solves problems in the first frame the entity becomes visible, because it won't be active.
            // Only do it in cut scenes, because it is too for all sequences.
            pEntity->Activate(true);
        }
    }

    // [*DavidR | 6/Oct/2010] Positioning an entity when ragdollized will not look good at all :)
    // Note: Articulated != ragdoll in some cases. And kinematic(mass 0) articulated entities could allow
    // repositioning, but positioning a kinematic articulated entity by TrackView isn't something we expect
    // to happen very much compared with regular ragdoll
    const bool bRagdoll = pEntity->GetPhysics() && (pEntity->GetPhysics()->GetType() == PE_ARTICULATED);
    if (!bRagdoll && (entityUpdateFlags || bScaleModified || bScriptPropertyModified || (m_target != NULL)))
    {
        m_pos = pos;
        m_rotate = rotate;
        m_scale = scale;

        m_bIgnoreSetParam = true; // Prevents feedback change of track.
        // no callback specified, so lets move the entity directly
        if (!m_pOwner)
        {
            if (entityUpdateFlags)
            {
                UpdateEntityPosRotVel(m_pos, m_rotate, ec.time == 0.f, (EUpdateEntityFlags)entityUpdateFlags, ec.time);
            }
            if (bScaleModified)
            {
                pEntity->SetScale(m_scale, ENTITY_XFORM_TRACKVIEW);
            }
        }
        m_bIgnoreSetParam = false; // Prevents feedback change of track.
    }

    m_time = ec.time;

    UpdateTargetCamera(pEntity, rotate);

    if (m_pOwner)
    {
        m_bIgnoreSetParam = true; // Prevents feedback change of track.
        m_pOwner->OnNodeAnimated(this);
        m_bIgnoreSetParam = false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::ReleaseSounds()
{
    // Audio: Stop all playing sounds
    IEntity* const pEntity = GetEntity();
    if (!pEntity)
    {
        return;
    }

    // Get or create sound proxy if necessary.
    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        pAudioComponent->StopAllTriggers();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::OnReset()
{
    m_EntityId = 0;
    m_lastEntityKey = -1;

    if (m_characterTrackAnimator)
    {
        m_characterTrackAnimator->OnReset(this);
    }

    m_lookAtTarget = "";
    m_lookAtEntityId = 0;
    m_allowAdditionalTransforms = true;
    m_lookPose = "";
    ReleaseSounds();
    UpdateDynamicParams();

}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::OnResetHard()
{
    OnReset();
    if (m_pOwner)
    {
        m_pOwner->OnNodeReset(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::PrepareAnimations()
{
    // Update durations of all character animations.
    IEntity* pEntity = GetEntity();
    if (!pEntity)
    {
        return;
    }

    ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
    if (!pCharacter)
    {
        return;
    }

    IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();

    int trackCount = NumTracks();
    for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
    {
        CAnimParamType trackType = m_tracks[paramIndex]->GetParameterType();
        IAnimTrack* pTrack = m_tracks[paramIndex].get();

        if (pAnimations)
        {
            switch (trackType.GetType())
            {
            case AnimParamType::Animation:
            {
                int numKeys = pTrack->GetNumKeys();
                for (int i = 0; i < numKeys; i++)
                {
                    ICharacterKey key;
                    pTrack->GetKey(i, &key);
                    int animId = pAnimations->GetAnimIDByName(key.m_animation.c_str());
                    if (animId >= 0)
                    {
                        float duration = pAnimations->GetDuration_sec(animId);
                        if (duration != key.m_duration)
                        {
                            key.m_duration = duration;
                            pTrack->SetKey(i, &key);
                        }
                    }
                }
            }
            break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::EnableProceduralFacialAnimation(bool enable)
{
    IEntity* pEntity = GetEntity();
    ICharacterInstance* pCharacter = (pEntity ? pEntity->GetCharacter(0) : 0);
    IFacialInstance* pFaceInstance = (pCharacter ? pCharacter->GetFacialInstance() : 0);
    if (pFaceInstance)
    {
        pFaceInstance->EnableProceduralFacialAnimation(enable);
    }
}


//////////////////////////////////////////////////////////////////////////
bool CAnimEntityNode::GetProceduralFacialAnimationEnabled()
{
    IEntity* pEntity = GetEntity();
    ICharacterInstance* pCharacter = (pEntity ? pEntity->GetCharacter(0) : 0);
    const IFacialInstance* pFaceInstance = (pCharacter ? pCharacter->GetFacialInstance() : 0);
    if (pFaceInstance)
    {
        return pFaceInstance->IsProceduralFacialAnimationEnabled();
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::Activate(bool bActivate)
{
    CAnimNode::Activate(bActivate);
    if (bActivate)
    {
#ifdef CHECK_FOR_TOO_MANY_ONPROPERTY_SCRIPT_CALLS
        m_OnPropertyCalls = 0;
#endif
        PrepareAnimations();
        m_proceduralFacialAnimationEnabledOld = GetProceduralFacialAnimationEnabled();
        EnableProceduralFacialAnimation(false);
    }
    else
    {
#ifdef CHECK_FOR_TOO_MANY_ONPROPERTY_SCRIPT_CALLS
        IEntity* pEntity = GetEntity();
        if (m_OnPropertyCalls > 30) // arbitrary amount
        {
            CryWarning(VALIDATOR_MODULE_MOVIE, VALIDATOR_ERROR, "Entity: %s. A trackview movie has called lua function 'OnPropertyChange' too many (%d) times .This is a performance issue. Adding Some custom management in the entity lua code will fix the issue", pEntity ? pEntity->GetName() : "<UNKNOWN", m_OnPropertyCalls);
        }
#endif

        // Release lock on preloaded Animations so no locks can leak
        m_animationCacher.ClearCache();

        EnableProceduralFacialAnimation(m_proceduralFacialAnimationEnabledOld);
    }

    m_lookAtTarget = "";
    m_lookAtEntityId = 0;
    m_allowAdditionalTransforms = true;
    m_lookPose = "";
};

//////////////////////////////////////////////////////////////////////////
ICharacterInstance* CAnimEntityNode::GetCharacterInstance()
{
    ICharacterInstance* retCharInstance = nullptr;

    IEntity* entity = GetEntity();
    if (entity)
    {
        retCharInstance = entity->GetCharacter(0);
    }

    return retCharInstance;
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::ResetSounds()
{
    for (int i = m_SoundInfo.size(); --i >= 0; )
    {
        m_SoundInfo[i].Reset();
    }
}

void CAnimEntityNode::OnStart()
{
    IAnimTrack* pPhysicalizeTrack = GetTrackForParameter(AnimParamType::Physicalize);
    if (pPhysicalizeTrack)
    {
        m_bInitialPhysicsStatus = false;
        if (IEntity* pEntity = GetEntity())
        {
            IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>();
            if (pPhysicsComponent)
            {
                m_bInitialPhysicsStatus = pPhysicsComponent->IsPhysicsEnabled();
            }
        }
        EnableEntityPhysics(false);
    }
    ResetSounds();
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::OnPause()
{
    ReleaseSounds();
    StopEntity();
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::OnStop()
{
    ReleaseSounds();
    StopEntity();
}

void CAnimEntityNode::OnLoop()
{
    ResetSounds();
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::UpdateEntityPosRotVel(const Vec3& targetPos, const Quat& targetRot, const bool initialState, const int flags, float fTime)
{
    IEntity* pEntity = GetEntity();
    if (!pEntity)
    {
        return;
    }

    IAnimTrack* pPhysicsDrivenTrack = GetTrackForParameter(AnimParamType::PhysicsDriven);
    bool bPhysicsDriven = false;
    if (pPhysicsDrivenTrack)
    {
        pPhysicsDrivenTrack->GetValue(m_time, bPhysicsDriven);
    }

    IPhysicalEntity* pPhysEnt = (initialState || pEntity->GetParent()) ? NULL : pEntity->GetPhysics();
    if (bPhysicsDriven && pPhysEnt && pPhysEnt->GetType() != PE_STATIC)
    {
        pe_status_dynamics dynamics;
        pPhysEnt->GetStatus(&dynamics);

        pe_status_pos psp;
        pPhysEnt->GetStatus(&psp);

        const float rTimeStep = 1.f / max(gEnv->pTimer->GetFrameTime(), 100e-3f);
        pe_action_set_velocity setVel;
        setVel.v = dynamics.v;
        setVel.w = dynamics.w;

        if (flags & (eUpdateEntity_Animation | eUpdateEntity_Position))
        {
            setVel.v = (targetPos - psp.pos) * (rTimeStep * movie_physicalentity_animation_lerp);
        }

        if (flags & (eUpdateEntity_Animation | eUpdateEntity_Rotation))
        {
            const Quat dq = targetRot * psp.q.GetInverted();
            setVel.w = dq.v * (2.f * rTimeStep * movie_physicalentity_animation_lerp); //This is an approximation
        }

        pPhysEnt->Action(&setVel);
    }
    else
    {
        if (flags & eUpdateEntity_Animation || ((flags & eUpdateEntity_Position) && (flags & eUpdateEntity_Rotation)))
        {
            const Vec3& scale = pEntity->GetScale();
            pEntity->SetPosRotScale(targetPos, targetRot, scale, ENTITY_XFORM_TRACKVIEW);
        }
        else
        {
            if (flags & eUpdateEntity_Position)
            {
                pEntity->SetPos(targetPos, ENTITY_XFORM_TRACKVIEW);
            }
            if (flags & eUpdateEntity_Rotation)
            {
                pEntity->SetRotation(targetRot, ENTITY_XFORM_TRACKVIEW);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetPos(float time, const Vec3& pos)
{
    m_pos = pos;

    bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded

    IAnimTrack* pPosTrack = GetTrackForParameter(AnimParamType::Position);
    if (pPosTrack)
    {
        pPosTrack->SetValue(time, pos, bDefault);
    }

    if (!bDefault)
    {
        GetCMovieSystem()->Callback(IMovieCallback::CBR_CHANGETRACK, this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetRotate(float time, const Quat& quat)
{
    m_rotate = quat;

    bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded

    IAnimTrack* rotTrack = GetTrackForParameter(AnimParamType::Rotation);
    if (rotTrack)
    {
        rotTrack->SetValue(time, m_rotate, bDefault);
    }

    if (!bDefault)
    {
        GetCMovieSystem()->Callback(IMovieCallback::CBR_CHANGETRACK, this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetScale(float time, const Vec3& scale)
{
    m_scale = scale;
    bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags & eAnimNodeFlags_EntitySelected)); // Only selected nodes can be recorded

    IAnimTrack* sclTrack = GetTrackForParameter(AnimParamType::Scale);
    if (sclTrack)
    {
        sclTrack->SetValue(time, scale, bDefault);
    }

    if (!bDefault)
    {
        GetCMovieSystem()->Callback(IMovieCallback::CBR_CHANGETRACK, this);
    }
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CAnimEntityNode::CreateTrack(const CAnimParamType& paramType)
{
    IAnimTrack* retTrack = CAnimNode::CreateTrack(paramType);

    if (retTrack)
    {
        if (paramType.GetType() == AnimParamType::Animation && !m_characterTrackAnimator)
        {
            m_characterTrackAnimator = new CCharacterTrackAnimator();
        }
    }

    return retTrack;
}


//////////////////////////////////////////////////////////////////////////
bool CAnimEntityNode::RemoveTrack(IAnimTrack* pTrack)
{
    if (pTrack && pTrack->GetParameterType().GetType() == AnimParamType::Animation && m_characterTrackAnimator)
    {
        delete m_characterTrackAnimator;
        m_characterTrackAnimator = nullptr;
    }

    return CAnimNode::RemoveTrack(pTrack);
}

//////////////////////////////////////////////////////////////////////////
Quat CAnimEntityNode::GetRotate(float time)
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

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::ApplyEventKey(CEventTrack* track, int keyIndex, IEventKey& key)
{
    IEntity* pEntity = GetEntity();
    if (!pEntity)
    {
        return;
    }

    if (!key.animation.empty()) // if there is an animation
    {
        // Start playing animation.
        ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
        if (pCharacter)
        {
            CryCharAnimationParams aparams;
            aparams.m_fTransTime  = 0.15f;

            aparams.m_fTransTime    = 0.0f;
            aparams.m_nFlags = CA_TRACK_VIEW_EXCLUSIVE;

            ISkeletonAnim* pSkeletonAnimation = pCharacter->GetISkeletonAnim();

            aparams.m_nLayerID = 0;
            pSkeletonAnimation->StartAnimation(key.animation.c_str(),  aparams);

            IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
            assert (pAnimations);
            //float duration = pAnimations->GetLength( key.animation );

            int animId = pAnimations->GetAnimIDByName(key.animation.c_str());
            if (animId >= 0)
            {
                float duration = pAnimations->GetDuration_sec(animId);
                if (duration != key.duration)
                {
                    key.duration = duration;
                    track->SetKey(keyIndex, &key);
                }
            }
        }
    }

    if (!key.event.empty()) // if there's an event
    {
        // Fire event on Entity.
        IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
        if (scriptComponent)
        {
            // Find event
            int type = -1;
            if (IEntityClass* pClass = pEntity->GetClass())
            {
                int count = pClass->GetEventCount();
                IEntityClass::SEventInfo info;
                for (int i = 0; i < count; ++i)
                {
                    info = pClass->GetEventInfo(i);
                    if (strcmp(key.event.c_str(), info.name) == 0)
                    {
                        type = info.type;
                        break;
                    }
                }
            }

            // Convert value to type
            switch (type)
            {
            case IEntityClass::EVT_INT:
            case IEntityClass::EVT_FLOAT:
            case IEntityClass::EVT_ENTITY:
                scriptComponent->CallEvent(key.event.c_str(), (float)atof(key.eventValue.c_str()));
                break;
            case IEntityClass::EVT_BOOL:
                scriptComponent->CallEvent(key.event.c_str(), atoi(key.eventValue.c_str()) != 0 ? true : false);
                break;
            case IEntityClass::EVT_STRING:
                scriptComponent->CallEvent(key.event.c_str(), key.eventValue.c_str());
                break;
            case IEntityClass::EVT_VECTOR:
            {
                Vec3 vTemp(0, 0, 0);
                float x = 0, y = 0, z = 0;
                int res = sscanf(key.eventValue.c_str(), "%f,%f,%f", &x, &y, &z);
                assert(res == 3);
                vTemp(x, y, z);
                scriptComponent->CallEvent(key.event.c_str(), vTemp);
            }
            break;
            case -1:
            default:
                scriptComponent->CallEvent(key.event.c_str());
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::ApplyAudioKey(char const* const sTriggerName, bool const bPlay /* = true */)
{
    IEntity* const pEntity = GetEntity();
    if (!pEntity)
    {
        return;
    }

    // Get or create sound proxy if necessary.
    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        Audio::TAudioControlID nAudioControlID = INVALID_AUDIO_CONTROL_ID;
        Audio::AudioSystemRequestBus::BroadcastResult(nAudioControlID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, sTriggerName);
        if (nAudioControlID != INVALID_AUDIO_CONTROL_ID)
        {
            if (bPlay)
            {
                pAudioComponent->ExecuteTrigger(nAudioControlID, eLSM_None);
            }
            else
            {
                pAudioComponent->StopTrigger(nAudioControlID);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::AnimateLookAt(CLookAtTrack* pTrack, SAnimContext& ec)
{
    IEntity* pEntity = GetEntity();
    if (!pEntity)
    {
        return;
    }

    ICharacterInstance* pCharacter = 0;

    IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
    if (!pAIActor)
    {
        pCharacter = pEntity->GetCharacter(0);
    }

    EntityId lookAtEntityId = 0;
    bool allowAdditionalTransforms = 0;
    string lookPose = "";

    ILookAtKey key;
    int nkey = pTrack->GetActiveKey(ec.time, &key);
    allowAdditionalTransforms = true;
    if (nkey >= 0)
    {
        lookPose = key.lookPose.c_str();
        if ((m_lookAtTarget[0] && !m_lookAtEntityId) || strcmp(key.szSelection.c_str(), m_lookAtTarget) != 0)
        {
            m_lookAtTarget = key.szSelection.c_str();
            IEntity* pTargetEntity = NULL;
            if (strcmp(key.szSelection.c_str(), "_LocalPlayer") != 0)
            {
                m_lookAtLocalPlayer = false;
                pTargetEntity = gEnv->pEntitySystem->FindEntityByName(key.szSelection.c_str());
            }
            else if (gEnv->pGame)
            {
                m_lookAtLocalPlayer = true;
                IActor* pLocalActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
                if (pLocalActor)
                {
                    pTargetEntity = pLocalActor->GetEntity();
                }
            }
            if (pTargetEntity)
            {
                lookAtEntityId = pTargetEntity->GetId();
            }
            else
            {
                lookAtEntityId = 0;
            }
        }
        else
        {
            lookAtEntityId = m_lookAtEntityId;
        }
    }
    else
    {
        lookAtEntityId = 0;
        m_lookAtTarget = "";
        allowAdditionalTransforms = true;
        lookPose = "";
    }

    if (m_lookAtEntityId != lookAtEntityId
        || m_allowAdditionalTransforms != allowAdditionalTransforms
        || m_lookPose != lookPose)
    {
        m_lookAtEntityId = lookAtEntityId;
        m_allowAdditionalTransforms = allowAdditionalTransforms;
        m_lookPose = lookPose;

        // We need to enable smoothing for the facial animations, since look ik can override them and cause snapping.
        IFacialInstance* pFacialInstance = (pCharacter ? pCharacter->GetFacialInstance() : 0);
        if (pFacialInstance)
        {
            pFacialInstance->TemporarilyEnableBoneRotationSmoothing();
        }
    }

    IEntity* pLookAtEntity = 0;
    if (m_lookAtEntityId)
    {
        pLookAtEntity = gEnv->pEntitySystem->GetEntity(m_lookAtEntityId);
    }

    if (pLookAtEntity)
    {
        Vec3 pos;

        // override _LocalPlayer position with camera position - looks a lot better
        if (m_lookAtLocalPlayer)
        {
            pos = gEnv->pRenderer->GetCamera().GetPosition();
        }
        else
        {
            pos = pLookAtEntity->GetWorldPos();
            ICharacterInstance* pLookAtChar = pLookAtEntity->GetCharacter(0);
            if (pLookAtChar)
            {
                // Try look at head bone.
                int16 nHeadBoneId = pLookAtChar->GetIDefaultSkeleton().GetJointIDByName(BoneNames::Head);
                if (nHeadBoneId >= 0)
                {
                    pos = pLookAtEntity->GetWorldTM().TransformPoint(pLookAtChar->GetISkeletonPose()->GetAbsJointByID(nHeadBoneId).t);
                    //  gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere( pos,0.2f,ColorB(255,0,0,255) );
                }
            }
        }

        if (pCharacter)
        {
            IAnimationPoseBlenderDir* pIPoseBlenderLook = pCharacter->GetISkeletonPose()->GetIPoseBlenderLook();
            if (pIPoseBlenderLook)
            {
                uint32 lookIKLayer = DEFAULT_LOOKIK_LAYER;
                if (pTrack->GetAnimationLayerIndex() >= 0)
                {
                    assert(pTrack->GetAnimationLayerIndex() < 16);
                    lookIKLayer = pTrack->GetAnimationLayerIndex();
                }
                pIPoseBlenderLook->SetState(true);
                pIPoseBlenderLook->SetLayer(lookIKLayer);
                pIPoseBlenderLook->SetTarget(pos);
                pIPoseBlenderLook->SetFadeoutAngle(DEG2RAD(120.0f));
                pIPoseBlenderLook->SetPolarCoordinatesSmoothTimeSeconds(key.smoothTime);

                CryCharAnimationParams Params;
                Params.m_nFlags = CA_TRACK_VIEW_EXCLUSIVE | CA_LOOP_ANIMATION;
                Params.m_nLayerID = lookIKLayer;
                Params.m_fTransTime = 1.0f;
                pCharacter->GetISkeletonAnim()->StartAnimation(key.lookPose.c_str(), Params);
            }
        }
    }
    else
    {
        if (pCharacter)
        {
            IAnimationPoseBlenderDir* pIPoseBlenderLook = pCharacter->GetISkeletonPose()->GetIPoseBlenderLook();
            if (pIPoseBlenderLook)
            {
                pIPoseBlenderLook->SetState(false);
            }
        }
        if (pAIActor)
        {
            pAIActor->ResetLookAt();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAnimEntityNode::AnimateScriptTableProperty(IAnimTrack* pTrack, SAnimContext& ec, const char* name)
{
    bool propertyChanged = false;
    IEntity* pEntity = GetEntity();
    if (!pEntity)
    {
        return propertyChanged;
    }

    TScriptPropertyParamInfoMap::iterator findIter = m_nameToScriptPropertyParamInfo.find(name);
    if (findIter != m_nameToScriptPropertyParamInfo.end())
    {
        SScriptPropertyParamInfo& param = m_entityScriptPropertiesParamInfos[findIter->second];

        float fValue;
        Vec3 vecValue;
        bool boolValue;
        float currfValue = 0.f;
        Vec3 currVecValue(0, 0, 0);
        bool currBoolValue = false;
        int currBoolIntValue = 0;

        switch (pTrack->GetValueType())
        {
        case AnimValueType::Float:
            pTrack->GetValue(ec.time, fValue);
            param.scriptTable->GetValue(param.variableName, currfValue);
            // this check actually fails much more often than it should. There is some kind of lack of precision in the trackview interpolation calculations, and often a value that should
            // be constant does small oscillations around the correct value. (0.49999, 5.00001, 0.49999, etc).
            // not a big issue as long as those changes dont trigger constant calls to OnPropertyChange, but maybe it could be worth it to see if is ok to use some range check like fcmp().
            if (currfValue != fValue)
            {
                param.scriptTable->SetValue(param.variableName, fValue);
                propertyChanged = true;
            }
            break;
        case AnimValueType::Vector:
        // fall through
        case AnimValueType::RGB:
            pTrack->GetValue(ec.time, vecValue);

            if (pTrack->GetValueType() == AnimValueType::RGB)
            {
                vecValue /= 255.0f;

                vecValue.x = clamp_tpl(vecValue.x, 0.0f, 1.0f);
                vecValue.y = clamp_tpl(vecValue.y, 0.0f, 1.0f);
                vecValue.z = clamp_tpl(vecValue.z, 0.0f, 1.0f);
            }

            if (param.isVectorTable)
            {
                param.scriptTable->GetValue("x", currVecValue.x) && param.scriptTable->GetValue("y", currVecValue.y) && param.scriptTable->GetValue("z", currVecValue.z);
                if (currVecValue != vecValue)
                {
                    param.scriptTable->SetValue("x", vecValue.x);
                    param.scriptTable->SetValue("y", vecValue.y);
                    param.scriptTable->SetValue("z", vecValue.z);
                    propertyChanged = true;
                }
            }
            else
            {
                param.scriptTable->GetValue(param.variableName, currVecValue);
                if (currVecValue != vecValue)
                {
                    param.scriptTable->SetValue(param.variableName, vecValue);
                    propertyChanged = true;
                }
            }
            break;
        case AnimValueType::Bool:
            pTrack->GetValue(ec.time, boolValue);
            if (param.scriptTable->GetValueType(param.variableName) == svtNumber)
            {
                int boolIntValue = boolValue ? 1 : 0;
                param.scriptTable->GetValue(param.variableName, currBoolIntValue);
                if (currBoolIntValue != boolIntValue)
                {
                    param.scriptTable->SetValue(param.variableName, boolIntValue);
                    propertyChanged = true;
                }
            }
            else
            {
                param.scriptTable->GetValue(param.variableName, currBoolValue);
                if (currBoolValue != boolValue)
                {
                    param.scriptTable->SetValue(param.variableName, boolValue);
                    propertyChanged = true;
                }
            }
            break;
        }

        if (propertyChanged)
        {
            // we give the lua code the chance to internally manage the change without needing a call to OnPropertyChange which is totally general and usually includes a recreation of all internal objects
            HSCRIPTFUNCTION func = 0;
            pEntity->GetScriptTable()->GetValue("OnPropertyAnimated", func);
            if (func)
            {
                bool changeTakenCareOf = false;
                Script::CallReturn(gEnv->pScriptSystem, func, pEntity->GetScriptTable(), param.variableName.c_str(), changeTakenCareOf);
                propertyChanged = !changeTakenCareOf;
                gEnv->pScriptSystem->ReleaseFunc(func);
            }

            // Broadcast the animated sript property change. Currently this is only used to allow the Editor to update the UI, so as an optimization, we only
            // broadcast if we're in the editor
            if (gEnv->IsEditor() && !gEnv->IsEditorGameMode())
            {
                SEntityEvent event(ENTITY_EVENT_SCRIPT_PROPERTY_ANIMATED);
                pEntity->SendEvent(event);
            }
        }
    }
    return propertyChanged;
}

//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
void CAnimEntityNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
    if (bLoading)
    {
        xmlNode->getAttr("EntityGUID", m_entityGuid);

        xmlNode->getAttr("EntityGUIDTarget", m_entityGuidTarget);
        xmlNode->getAttr("EntityGUIDSource", m_entityGuidSource);

        int paramTypeVersion = 0;
        xmlNode->getAttr("paramIdVersion", paramTypeVersion);
        if (paramTypeVersion <= 8)
        {
            // Entity Nodes were made renamable for paramIdVersion == 9 (circa 04/2016)
            SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
        }
    }
    else
    {
        xmlNode->setAttr("EntityGUID", m_entityGuid);

        if (m_entityGuidTarget)
        {
            xmlNode->setAttr("EntityGUIDTarget", m_entityGuidTarget);
        }
        if (m_entityGuidSource)
        {
            xmlNode->setAttr("EntityGUIDSource", m_entityGuidSource);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CAnimEntityNode, CAnimNode>()
        ->Version(1)
        ->Field("EntityGUID", &CAnimEntityNode::m_entityGuid)
        ->Field("EntityGUIDTarget", &CAnimEntityNode::m_entityGuidTarget)
        ->Field("EntityGUIDSource", &CAnimEntityNode::m_entityGuidSource);
}

void CAnimEntityNode::PrecacheStatic(float time)
{
    // Update durations of all character animations.
    IEntity* pEntity = GetEntity();
    if (pEntity)
    {
        static_cast<CAnimSequence*>(GetSequence())->PrecacheEntity(pEntity);
    }

    const uint trackCount = NumTracks();
    for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
    {
        CAnimParamType paramType = m_tracks[paramIndex]->GetParameterType();
        IAnimTrack* pTrack = m_tracks[paramIndex].get();

        if (paramType == AnimParamType::Sound)
        {
            // Pre-cache audio data from all tracks
            CSoundTrack const* const pSoundTrack = static_cast<CSoundTrack const* const>(pTrack);
            ISoundKey oKey;

            int const nKeysCount = pTrack->GetNumKeys();
            for (int nKeyIndex = 0; nKeyIndex < nKeysCount; ++nKeyIndex)
            {
                pSoundTrack->GetKey(nKeyIndex, &oKey);
            }
        }
    }
}

void CAnimEntityNode::PrecacheDynamic(float time)
{
    // Update durations of all character animations.
    IEntity* pEntity = GetEntity();
    if (!pEntity)
    {
        return;
    }

    ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
    if (!pCharacter)
    {
        return;
    }

    IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
    if (!pAnimations)
    {
        return;
    }

    int trackCount = NumTracks();
    for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
    {
        CAnimParamType trackType = m_tracks[paramIndex]->GetParameterType();
        IAnimTrack* pTrack = m_tracks[paramIndex].get();

        if (trackType == AnimParamType::Animation)
        {
            int numKeys = pTrack->GetNumKeys();
            for (int i = 0; i < numKeys; i++)
            {
                ICharacterKey key;
                pTrack->GetKey(i, &key);

                // always make sure that all animation keys in the time interval
                // [time ; time + ANIMATION_KEY_PRELOAD_INTERVAL]
                // have an extra reference so they are kept in memory
                if (key.time < time)
                {
                    int32 animID = pAnimations->GetAnimIDByName(key.m_animation.c_str());
                    if (animID >= 0)
                    {
                        uint32 animPathCRC = pAnimations->GetFilePathCRCByAnimID(animID);
                        m_animationCacher.RemoveFromCache(animPathCRC);
                    }
                    continue;
                }
                if (key.time < time + ANIMATION_KEY_PRELOAD_INTERVAL)
                {
                    int animId = pAnimations->GetAnimIDByName(key.m_animation.c_str());
                    if (animId >= 0)
                    {
                        uint32 animPathCRC = pAnimations->GetFilePathCRCByAnimID(animId);
                        m_animationCacher.AddToCache(animPathCRC);
                    }
                    continue;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
Vec3 CAnimEntityNode::Noise::Get(float time) const
{
    Vec3 noise;
    const float phase = time * m_freq;
    const Vec3 phase0 = Vec3(15.0f * m_freq, 55.1f * m_freq, 101.2f * m_freq);

    noise.x = gEnv->pSystem->GetNoiseGen()->Noise1D(phase + phase0.x) * m_amp;
    noise.y = gEnv->pSystem->GetNoiseGen()->Noise1D(phase + phase0.y) * m_amp;
    noise.z = gEnv->pSystem->GetNoiseGen()->Noise1D(phase + phase0.z) * m_amp;

    return noise;
}

Vec3 CAnimEntityNode::Adjust3DSoundOffset(bool bVoice, IEntity* pEntity, Vec3& oSoundPos) const
{
    ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
    Vec3 offset(0);
    if (pCharacter)
    {
        oSoundPos = pEntity->GetWorldTM() * pCharacter->GetAABB().GetCenter();
        if (bVoice == false)
        {
            offset = pCharacter->GetAABB().GetCenter();
        }
    }

    return offset;
}

void CAnimEntityNode::StopEntity()
{
    IEntity* pEntity = GetEntity();
    if (!pEntity)
    {
        return;
    }

    IPhysicalEntity* pPhysEntity = pEntity->GetParent() ? NULL : pEntity->GetPhysics();

    if (pPhysEntity)
    {
        int entityUpdateFlags = 0;

        int trackCount = NumTracks();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            IAnimTrack* pTrack = m_tracks[paramIndex].get();
            if (pTrack && pTrack->GetNumKeys() > 0)
            {
                CAnimParamType paramType = pTrack->GetParameterType();
                switch (paramType.GetType())
                {
                case AnimParamType::Position:
                    entityUpdateFlags |= eUpdateEntity_Position;
                    break;
                case AnimParamType::Rotation:
                    entityUpdateFlags |= eUpdateEntity_Rotation;
                    break;
                }
            }

            pe_status_dynamics dynamics;
            pPhysEntity->GetStatus(&dynamics);

            pe_action_set_velocity setVel;
            setVel.v = (entityUpdateFlags & eUpdateEntity_Position) ? Vec3(ZERO) : dynamics.v;
            setVel.w = (entityUpdateFlags & eUpdateEntity_Rotation) ? Vec3(ZERO) : dynamics.w;
            pPhysEntity->Action(&setVel);
        }
    }

    IAnimTrack* pPhysicalizeTrack = GetTrackForParameter(AnimParamType::Physicalize);
    if (pPhysicalizeTrack)
    {
        EnableEntityPhysics(m_bInitialPhysicsStatus);
    }
}

void CAnimEntityNode::UpdateTargetCamera(IEntity* pEntity, const Quat& rotation)
{
    // TODO: This only works if either the camera or the target are directly
    // referenced in TrackView. Would be better if we could move this to
    // the camera component in entity system or similar.

    IEntity* pEntityCamera = NULL;
    IEntity* pEntityTarget = NULL;

    if (m_entityGuidTarget)
    {
        pEntityCamera = pEntity;

        if (!m_EntityIdTarget)
        {
            m_EntityIdTarget = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuidTarget);
        }

        if (m_EntityIdTarget)
        {
            pEntityTarget = gEnv->pEntitySystem->GetEntity(m_EntityIdTarget);
        }
    }
    else if (m_entityGuidSource)
    {
        pEntityTarget = pEntity;

        if (!m_EntityIdSource)
        {
            m_EntityIdSource = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuidSource);
        }

        if (m_EntityIdSource)
        {
            pEntityCamera = gEnv->pEntitySystem->GetEntity(m_EntityIdSource);
        }
    }

    if (pEntityCamera && pEntityTarget)
    {
        Vec3 cameraPos = pEntityCamera->GetPos();

        if (pEntityCamera->GetParent())
        {
            cameraPos = pEntityCamera->GetParent()->GetWorldTM().TransformPoint(cameraPos);
        }

        Matrix33 rotationMatrix(IDENTITY);
        if (!rotation.IsIdentity())
        {
            Ang3 angles(rotation);
            rotationMatrix = Matrix33::CreateRotationY(angles.y);
        }

        const Vec3 targetPos = pEntityTarget->GetWorldPos();

        Matrix34 tm;
        if (targetPos == cameraPos)
        {
            tm.SetTranslation(cameraPos);
        }
        else
        {
            tm = Matrix34(Matrix33::CreateRotationVDir((targetPos - cameraPos).GetNormalized()), cameraPos) * rotationMatrix;
        }

        pEntityCamera->SetWorldTM(tm, ENTITY_XFORM_TRACKVIEW);
    }
}

#undef s_nodeParamsInitialized
#undef s_nodeParams
#undef AddSupportedParam

void CAnimationCacher::AddToCache(uint32 animPathCRC)
{
    uint32 numAnims = m_cachedAnims.size();
    for (uint32 i = 0; i < numAnims; ++i)
    {
        if (m_cachedAnims[i] == animPathCRC)
        {
            return;
        }
    }

    gEnv->pCharacterManager->CAF_AddRef(animPathCRC);
    m_cachedAnims.push_back(animPathCRC);
}

void CAnimationCacher::RemoveFromCache(uint32 animPathCRC)
{
    uint32 numAnims = m_cachedAnims.size();
    for (uint32 i = 0; i < numAnims; ++i)
    {
        if (m_cachedAnims[i] == animPathCRC)
        {
            gEnv->pCharacterManager->CAF_Release(animPathCRC);
            for (uint32 j = i; j < numAnims - 1; ++j)
            {
                m_cachedAnims[j] = m_cachedAnims[j + 1];
            }
            m_cachedAnims.resize(numAnims - 1);
            return;
        }
    }
}

void CAnimationCacher::ClearCache()
{
    uint32 numAnims = m_cachedAnims.size();
    for (uint32 i = 0; i < numAnims; ++i)
    {
        gEnv->pCharacterManager->CAF_Release(m_cachedAnims[i]);
    }
    m_cachedAnims.resize(0);
}
