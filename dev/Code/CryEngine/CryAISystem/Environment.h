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

// Description : Simple global struct for accessing major module pointers
//
// Notes:        auto_ptrs to handle init/delete would be great,
//               but exposing autoptrs to the rest of the code needs thought


#ifndef CRYINCLUDE_CRYAISYSTEM_ENVIRONMENT_H
#define CRYINCLUDE_CRYAISYSTEM_ENVIRONMENT_H
#pragma once

#include "Configuration.h"
#include "AIConsoleVariables.h"
#include "AISignalCRCs.h"
#include <RayCastQueue.h>
#include <IntersectionTestQueue.h>

//////////////////////////////////////////////////////////////////////////
// AI system environment.
// For internal use only!
// Contain pointers to commonly needed modules for simple, efficient lookup
// Need a clear point in execution when these will all be valid
//////////////////////////////////////////////////////////////////////////
class ActorLookUp;
struct IGoalOpFactory;
class CObjectContainer;
class CCodeCoverageTracker;
class CCodeCoverageManager;
class CCodeCoverageGUI;
class CStatsManager;
class CTacticalPointSystem;
class CTargetTrackManager;
struct IAIDebugRenderer;
class CPipeManager;
class CGraph;
namespace MNM
{
    class PathfinderNavigationSystemUser;
}
class CMNMPathfinder;
class CNavigation;
class CAIActionManager;
class CSmartObjectManager;
class CPerceptionManager;
class CCommunicationManager;
class CCoverSystem;
class CSelectionTreeManager;
namespace BehaviorTree
{
    class BehaviorTreeManager;
    class GraftManager;
}
class CVisionMap;
class CFactionMap;
class CGroupManager;
class CollisionAvoidanceSystem;
class CAIObjectManager;
class WalkabilityCacheManager;
class NavigationSystem;
namespace AIActionSequence {
    class SequenceManager;
}
class ClusterDetector;

#ifdef CRYAISYSTEM_DEBUG
class CAIRecorder;
struct IAIBubblesSystem;
#endif //CRYAISYSTEM_DEBUG

struct SAIEnvironment
{
    AIConsoleVars CVars;
    AISIGNALS_CRC SignalCRCs;

    SConfiguration configuration;

    ActorLookUp* pActorLookUp;
    WalkabilityCacheManager* pWalkabilityCacheManager;
    IGoalOpFactory* pGoalOpFactory;
    CObjectContainer* pObjectContainer;

#if !defined(_RELEASE)
    CCodeCoverageTracker* pCodeCoverageTracker;
    CCodeCoverageManager* pCodeCoverageManager;
    CCodeCoverageGUI* pCodeCoverageGUI;
#endif

    CStatsManager* pStatsManager;
    CTacticalPointSystem* pTacticalPointSystem;
    CTargetTrackManager* pTargetTrackManager;
    CAIObjectManager* pAIObjectManager;
    CPipeManager* pPipeManager;
    CGraph* pGraph; // superseded by NavigationSystem - remove when all links are cut
    MNM::PathfinderNavigationSystemUser* pPathfinderNavigationSystemUser;
    CMNMPathfinder* pMNMPathfinder; // superseded by NavigationSystem - remove when all links are cut
    CNavigation* pNavigation; // superseded by NavigationSystem - remove when all links are cut
    CAIActionManager* pAIActionManager;
    CSmartObjectManager* pSmartObjectManager;

    CPerceptionManager* pPerceptionManager;

    CCommunicationManager* pCommunicationManager;
    CCoverSystem* pCoverSystem;
    NavigationSystem* pNavigationSystem;
    CSelectionTreeManager* pSelectionTreeManager;
    BehaviorTree::BehaviorTreeManager* pBehaviorTreeManager;
    BehaviorTree::GraftManager* pGraftManager;
    CVisionMap* pVisionMap;
    CFactionMap* pFactionMap;
    CGroupManager* pGroupManager;
    CollisionAvoidanceSystem* pCollisionAvoidanceSystem;
    struct IMovementSystem* pMovementSystem;
    AIActionSequence::SequenceManager* pSequenceManager;
    ClusterDetector* pClusterDetector;

#ifdef CRYAISYSTEM_DEBUG
    IAIBubblesSystem* pBubblesSystem;
#endif

    typedef RayCastQueue<41> GlobalRayCaster;
    GlobalRayCaster* pRayCaster;

    typedef IntersectionTestQueue<43> GlobalIntersectionTester;
    GlobalIntersectionTester* pIntersectionTester;

    //more cache friendly
    IPhysicalWorld* pWorld;//TODO use this more, or eliminate it.


    SAIEnvironment();
    ~SAIEnvironment();


    SC_API IAIDebugRenderer* GetDebugRenderer();
    SC_API IAIDebugRenderer* GetNetworkDebugRenderer();
    void SetDebugRenderer(IAIDebugRenderer* pAIDebugRenderer);
    void SetNetworkDebugRenderer(IAIDebugRenderer* pAIDebugRenderer);

#ifdef CRYAISYSTEM_DEBUG
    CAIRecorder* GetAIRecorder();
    void SetAIRecorder(CAIRecorder* pAIRecorder);
#endif //CRYAISYSTEM_DEBUG

    void ShutDown();

    void GetMemoryUsage(ICrySizer* pSizer) const {}
private:
    IAIDebugRenderer* pDebugRenderer;
    IAIDebugRenderer* pNetworkDebugRenderer;

#ifdef CRYAISYSTEM_DEBUG
    CAIRecorder* pRecorder;
#endif //CRYAISYSTEM_DEBUG
};

SC_API extern SAIEnvironment gAIEnv;

#endif // CRYINCLUDE_CRYAISYSTEM_ENVIRONMENT_H
