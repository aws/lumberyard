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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYCVARS_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYCVARS_H
#pragma once

//////////////////////////////////////////////////////////////////////////
// Console variables local to EntitySystem.
//////////////////////////////////////////////////////////////////////////
struct CVar
{
    static ICVar* pDebug;
    static ICVar* pCharacterIK;
    static ICVar* pProfileEntities;
    //  static ICVar *pUpdateInvisibleCharacter;
    //  static ICVar *pUpdateBonePositions;
    static ICVar* pUpdateScript;
    static ICVar* pUpdateTimer;
    static ICVar* pUpdateCamera;
    static ICVar* pUpdatePhysics;
    static ICVar* pUpdateAI;
    static ICVar* pUpdateEntities;
    static ICVar* pUpdateCollision;
    static ICVar* pUpdateCollisionScript;
    static ICVar* pUpdateContainer;
    static ICVar* pUpdateCoocooEgg;
    static ICVar* pPiercingCamera;
    static ICVar* pVisCheckForUpdate;
    static ICVar* pEntityBBoxes;
    static ICVar* pEntityHelpers;
    static ICVar* pMinImpulseVel;
    static ICVar* pImpulseScale;
    static ICVar* pMaxImpulseAdjMass;
    static ICVar* pDebrisLifetimeScale;
    static ICVar* pSplashThreshold;
    static ICVar* pSplashTimeout;
    static ICVar* pHitCharacters;
    static ICVar* pHitDeadBodies;
    static ICVar* pCharZOffsetSpeed;
    static ICVar* pLogCollisions;
    static ICVar* pNotSeenTimeout;
    static ICVar* pDebugNotSeenTimeout;
    static ICVar* pDrawAreas;
    static ICVar* pDrawAreaGrid;
    static ICVar* pDrawAreaDebug;
    static ICVar* pDrawAudioComponentZRay;
    static ICVar* pMotionBlur;
    static ICVar* pSysSpecLight;

    static int   es_UsePhysVisibilityChecks;
    static float es_MaxPhysDist;
    static float es_MaxPhysDistInvisible;
    static float es_MaxPhysDistCloth;
    static float es_FarPhysTimeout;
    static int   es_SortUpdatesByClass;

    // debug only
    static ICVar* pEnableFullScriptSave;

    static int es_DebugTimers;
    static int es_DebugFindEntity;
    static int es_DebugEvents;
    static int es_debugEntityLifetime;
    static int es_DisableTriggers;
    static int es_DrawProximityTriggers;

    static int es_DebugEntityUsage;
    static const char* es_DebugEntityUsageFilter;

    // Entity pool usage
    static int es_EnablePoolUse;
    static int es_DebugPool;
    static int es_TestPoolSignatures;

    static const char* es_DebugPoolFilter;

    static int es_LayerSaveLoadSerialization;
    static int es_SaveLoadUseLUANoSaveFlag;
    static int es_ClearPoolBookmarksOnLayerUnload;
    static int es_LayerDebugInfo;

    static ICVar* pUpdateType;

    static float es_EntityUpdatePosDelta;

    static int es_debugDrawEntityIDs;

    // Initialize console variables.
    static void Init(struct IConsole* pConsole);

    // Dump Entities
    static void DumpEntities(IConsoleCmdArgs*);
    static void DumpEntityClassesInUse(IConsoleCmdArgs*);

    // Recompile area grid
    static void CompileAreaGrid(IConsoleCmdArgs*);

    static void DumpEntityBookmarks(IConsoleCmdArgs*);

    static void EnableDebugAnimText(IConsoleCmdArgs* args);
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYCVARS_H
