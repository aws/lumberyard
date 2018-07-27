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

#ifndef CRYINCLUDE_CRYACTION_CRYACTIONCVARS_H
#define CRYINCLUDE_CRYACTION_CRYACTIONCVARS_H
#pragma once

#include "IConsole.h"

class CCryActionCVars
{
public:

    float playerInteractorRadius;  //Controls CInteractor action radius
    int     debugItemMemStats;              //Displays item mem stats

    int     g_debug_stats;
    int     g_statisticsMode;
    int     useCurrentUserNameAsDefault;

#if !defined(_RELEASE)
    int     g_userNeverAutoSignsIn;
#endif

#ifdef AI_LOG_SIGNALS
    int aiLogSignals;
    float aiMaxSignalDuration;
#endif
    int aiFlowNodeAlertnessCheck;

    // Disable HUD debug text
    int     cl_DisableHUDText;

    int g_gameplayAnalyst;
    int g_multiplayerEnableVehicles;

    // Cooperative Animation System
    int     co_coopAnimDebug;
    int     co_usenewcoopanimsystem;
    int     co_slideWhileStreaming;
    // ~Cooperative Animation System

    int g_syncClassRegistry;

    int g_allowSaveLoadInEditor;
    int g_saveLoadBasicEntityOptimization;
    int g_debugSaveLoadMemory;
    int g_saveLoadUseExportedEntityList;
    int g_useXMLCPBinForSaveLoad;
    int g_XMLCPBGenerateXmlDebugFiles;
    int g_XMLCPBAddExtraDebugInfoToXmlDebugFiles;
    int g_XMLCPBSizeReportThreshold;
    int g_XMLCPBUseExtraZLibCompression;
    int g_XMLCPBBlockQueueLimit;
    int g_saveLoadExtendedLog;

    int g_debugDialogBuffers;

    int g_allowDisconnectIfUpdateFails;

    int g_useSinglePosition;
    int g_handleEvents;
    int g_disableInputKeyFlowNodeInDevMode;

    int g_disableSequencePlayback;

    int g_enableMergedMeshRuntimeAreas;

    // AI stances
    ICVar* ag_defaultAIStance;

    //Feature Test CVARS
    ICVar* ft_runFeatureTest;
    //~Feature Test CVARS

    static ILINE CCryActionCVars& Get()
    {
        CRY_ASSERT(s_pThis);
        return *s_pThis;
    }

private:
    friend class CCryAction; // Our only creator

    CCryActionCVars(); // singleton stuff
    ~CCryActionCVars();

    static CCryActionCVars* s_pThis;

    static void DumpEntitySerializationData(IConsoleCmdArgs* pArgs);
    static void DumpClassRegistry(IConsoleCmdArgs* pArgs);
};

#endif // CRYINCLUDE_CRYACTION_CRYACTIONCVARS_H
