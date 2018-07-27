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
#pragma once

#include <platform.h> // Many CryCommon files require that this is included first.

#include <algorithm>
#include <memory>
#include <vector>
#include <map>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <array>
#include <string>

#include <ProjectDefines.h>
#include <StlUtils.h>
#include <Cry_Math.h>
#include <Cry_Camera.h>
#include <CryArray.h>
#include <ISystem.h>
#include <INetwork.h>
#include <IScriptSystem.h>
#include <IEntitySystem.h>
#include <IEntityHelper.h>
#include <ICryPak.h>
#include <IConsole.h>
#include <ITimer.h>
#include <ILog.h>
#include <IRemoteControl.h>
#include <CryAction.h>
#include <IGameFramework.h>
#include <IActorSystem.h>
#include <IAnimatedCharacter.h>
#include <IGame.h>
#include <IItem.h>
#include <IItemSystem.h>
#include <IViewSystem.h>
#include <IVehicleSystem.h>
#include <IFlowSystem.h>
#include <IGameplayRecorder.h>
#include <GameWarning.h>
#include <GameUtils.h>
#include <Cry_Geo.h>
#include <ISerialize.h>
#include <IAIAction.h>
#include <IPhysics.h>
#include <I3DEngine.h>
#include <IAgent.h>
#include <IAIActorProxy.h>
#include <IAIGroupProxy.h>
#include <IEntity.h>
#include <CryFile.h>
#include <IXml.h>
#include <IRenderAuxGeom.h>
#include <Random.h>
#include <XMLUtils.h>
#include <VehicleSystem.h>
#include <FlowSystem/FlowSystem.h>

#ifdef CRYAISYSTEM_VERBOSITY
# define AIWarningID gEnv->pAISystem->Warning
# define AIErrorID gEnv->pAISystem->Error
# define AILogProgressID gEnv->pAISystem->LogProgress
# define AILogEventID gEnv->pAISystem->LogEvent
# define AILogCommentID gEnv->pAISystem->LogComment
#else
# define AIInitLog (void)
# define AIWarning (void)
# define AIError (void)
# define AILogProgress (void)
# define AILogEvent (void)
# define AILogComment (void)
# define AILogAlways (void)
# define AILogLoading (void)
# define AIWarningID (void)
# define AIErrorID (void)
# define AILogProgressID (void)
# define AILogEventID (void)
# define AILogCommentID (void)
#endif

#define AI_STRIP_OUT_LEGACY_LOOK_TARGET_CODE

/// This frees the memory allocation for a vector (or similar), rather than just erasing the contents
template<typename T>
void ClearVectorMemory(T& container)
{
    T().swap(container);
}

// adding some headers here can improve build times... but at the cost of the compiler not registering
// changes to these headers if you compile files individually.
#include "AILog.h"
#include "CAISystem.h"
#include "ActorLookUp.h"
#include "Environment.h"
#include "CodeCoverageTracker.h"

inline bool IsClientActor(EntityId id)
{
    IActor* pActor = CCryAction::GetCryAction()->GetClientActor();
    if (pActor && pActor->GetEntity()->GetId() == id)
    {
        return true;
    }
    return false;
}

template<typename T>
bool inline GetAttr(const XmlNodeRef& node, const char* key, T& val)
{
    return node->getAttr(key, val);
}

bool inline GetTimeAttr(const XmlNodeRef& node, const char* key, time_t& val)
{
    const char* pVal = node->getAttr(key);
    if (!pVal)
    {
        return false;
    }
    val = GameUtils::stringToTime(pVal);
    return true;
}

template<>
bool inline GetAttr(const XmlNodeRef& node, const char* key, string& val)
{
    const char* pVal = node->getAttr(key);
    if (!pVal)
    {
        return false;
    }
    val = pVal;
    return true;
}

class CAISystem;
ILINE CAISystem* GetAISystem()
{
    extern CAISystem* g_pAISystem;
    return g_pAISystem;
}

//====================================================================
// SetAABBCornerPoints
//====================================================================
inline void SetAABBCornerPoints(const AABB& b, Vec3* pts)
{
    pts[0].Set(b.min.x, b.min.y, b.min.z);
    pts[1].Set(b.max.x, b.min.y, b.min.z);
    pts[2].Set(b.max.x, b.max.y, b.min.z);
    pts[3].Set(b.min.x, b.max.y, b.min.z);

    pts[4].Set(b.min.x, b.min.y, b.max.z);
    pts[5].Set(b.max.x, b.min.y, b.max.z);
    pts[6].Set(b.max.x, b.max.y, b.max.z);
    pts[7].Set(b.min.x, b.max.y, b.max.z);
}


ILINE float LinStep(float a, float b, float x)
{
    float w = (b - a);
    if (w != 0.0f)
    {
        x = (x - a) / w;
        return min(1.0f, max(x, 0.0f));
    }
    return 0.0f;
}

//===================================================================
// HasPointInRange
// (To be replaced)
//===================================================================
ILINE bool  HasPointInRange(const std::vector<Vec3>& list, const Vec3& pos, float range)
{
    float   r(sqr(range));
    for (uint32 i = 0; i < list.size(); ++i)
    {
        if (Distance::Point_PointSq(list[i], pos) < r)
        {
            return true;
        }
    }
    return false;
}

#if defined(WIN64)
#define CRY_INTEGRATE_DX12
#endif

#ifdef ASSERT_ON_ANIM_CHECKS
#   define ANIM_ASSERT_TRACE(condition, parenthese_message) CRY_ASSERT_TRACE(condition, parenthese_message)
#   define ANIM_ASSERT(condition) CRY_ASSERT(condition)
#else
#   define ANIM_ASSERT_TRACE(condition, parenthese_message)
#   define ANIM_ASSERT(condition)
#endif

#define LOG_ASSET_PROBLEM(condition, parenthese_message)                   \
    do                                                                     \
    {                                                                      \
        gEnv->pLog->LogWarning("Anim asset error: %s failed", #condition); \
        gEnv->pLog->LogWarning parenthese_message;                         \
    } while (0)

#define ANIM_ASSET_CHECK_TRACE(condition, parenthese_message)     \
    do                                                            \
    {                                                             \
        bool anim_asset_ok = (condition);                         \
        if (!anim_asset_ok)                                       \
        {                                                         \
            ANIM_ASSERT_TRACE(anim_asset_ok, parenthese_message); \
            LOG_ASSET_PROBLEM(condition, parenthese_message);     \
        }                                                         \
    } while (0)

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Casting/lossy_cast.h>

#include <ICryAnimation.h>
#include <AnimationBase.h>

// maximum number of LODs per one geometric model (CryGeometry)
enum
{
    g_nMaxGeomLodLevels = 6
};

#include "EntityCVars.h"

#if !defined(_RELEASE)
#define INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE
#endif // !_RELEASE

//////////////////////////////////////////////////////////////////////////
class CEntitySystem;
extern CEntitySystem* g_pIEntitySystem;
ILINE CEntitySystem* GetIEntitySystem() { return g_pIEntitySystem; }
namespace LegacyCryEntitySystem
{
    IEntitySystem* InitEntitySystem();
}
//////////////////////////////////////////////////////////////////////////

class CEntity;

//////////////////////////////////////////////////////////////////////////
//! Reports a Game Warning to validator with WARNING severity.
inline void EntityWarning(const char* format, ...) PRINTF_PARAMS(1, 2);
inline void EntityWarning(const char* format, ...)
{
    if (!format)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    gEnv->pSystem->WarningV(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, 0, NULL, format, args);
    va_end(args);
}

inline void EntityFileWarning(const char* file, const char* format, ...) PRINTF_PARAMS(2, 3);
inline void EntityFileWarning(const char* file, const char* format, ...)
{
    if (!format)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    gEnv->pSystem->WarningV(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, 0, file, format, args);
    va_end(args);
}


//////////////////////////////////////////////////////////////////////////
// Proxy creation functions.
//////////////////////////////////////////////////////////////////////////
struct IEntityScript;
extern IComponentPtr CreateScriptComponent(IEntity* pEntity, IEntityScript* pScript, SEntitySpawnParams* pSpawnParams);

#define ENTITY_PROFILER FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_ENTITY, g_bProfilerEnabled);
#define ENTITY_PROFILER_NAME(str) FRAME_PROFILER_FAST(str, GetISystem(), PROFILE_ENTITY, g_bProfilerEnabled);

#ifdef _RELEASE
#define ScriptWarning(...) ((void)0)
#else
void ScriptWarning(const char*, ...) PRINTF_PARAMS(1, 2);
inline void ScriptWarning(const char* format, ...)
{
    IF(!format, 0)
    {
        return;
    }

    char buffer[MAX_WARNING_LENGTH];
    va_list args;
    va_start(args, format);
    int count = azvsnprintf(buffer, sizeof(buffer), format, args);
    if (count == -1 || count >= sizeof(buffer))
    {
        buffer[sizeof(buffer) - 1] = '\0';
    }
    va_end(args);
    CryWarning(VALIDATOR_MODULE_SCRIPTSYSTEM, VALIDATOR_WARNING, buffer);
}
#endif

#ifndef OUT
# define OUT
#endif

#ifndef IN
# define IN
#endif
