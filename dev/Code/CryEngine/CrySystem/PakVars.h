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

#ifndef CRYINCLUDE_CRYSYSTEM_PAKVARS_H
#define CRYINCLUDE_CRYSYSTEM_PAKVARS_H
#pragma once

#ifdef _RELEASE
#define BUILD_VALUE(nonReleaseVal, releaseVal) releaseVal
#else // _RELEASE
#define BUILD_VALUE(nonReleaseVal, releaseVal) nonReleaseVal
#endif // _RELEASE

enum EPakPriority
{
    ePakPriorityFileFirst = 0,
    ePakPriorityPakFirst  = 1,
    ePakPriorityPakOnly   = 2,
    ePakPriorityFileFirstModsOnly = 3,
};

//Android and IOS are still using some loose files
#if defined(ANDROID) || defined(IOS) || defined(APPLETV)
    #define PAK_PRIORITY_DEFAULT BUILD_VALUE(ePakPriorityFileFirst, ePakPriorityFileFirstModsOnly)
    #define STREAM_CACHE_DEFAULT 0
    #define FRONTEND_SHADER_CACHE_DEFAULT 0
#elif defined(CONSOLE)
    #define PAK_PRIORITY_DEFAULT ePakPriorityPakOnly
    #define STREAM_CACHE_DEFAULT 1
    #define FRONTEND_SHADER_CACHE_DEFAULT 1
#else // PC
    #define PAK_PRIORITY_DEFAULT BUILD_VALUE(ePakPriorityFileFirst, ePakPriorityFileFirstModsOnly)
    #define STREAM_CACHE_DEFAULT 0
    #define FRONTEND_SHADER_CACHE_DEFAULT 0
#endif

// variables that control behavior of CryPak/StreamEngine subsystems
struct PakVars
{
    int nReadSlice;
    int nLogMissingFiles;
    int nSaveTotalResourceList;
    int nSaveFastloadResourceList;
    int nSaveMenuCommonResourceList;
    int nSaveLevelResourceList;
    int nValidateFileHashes;
    int nUncachedStreamReads;
    int nInMemoryPerPakSizeLimit;
    int nTotalInMemoryPakSizeLimit;
    int nLoadCache;
    int nLoadModePaks;
    int nStreamCache;
    int nPriority;
    int nMessageInvalidFileAccess;
    int nLogInvalidFileAccess;
    int nLoadFrontendShaderCache;
    int nDisableNonLevelRelatedPaks;
    int nWarnOnPakAccessFails;
    int nSetLogLevel;
#ifndef _RELEASE
    int nLogAllFileAccess;
#endif

    PakVars()
    {
        nReadSlice = 0;
        nLogMissingFiles = 0;
        nSaveTotalResourceList = 0;
        nSaveFastloadResourceList = 0;
        nSaveMenuCommonResourceList = 0;
        nSaveLevelResourceList = 0;
        //The default log level is Aws::Utils::Logging::LogLevel::Warn when CloudCanvasCommon gem initializes the API
        nSetLogLevel = 3;

        //??????????
        nValidateFileHashes = BUILD_VALUE(1, 0);
#if defined(_DEBUG)
        nValidateFileHashes = 1;
#endif

        nUncachedStreamReads = 1;
        // Limits in MB
        nInMemoryPerPakSizeLimit = 6;
        nTotalInMemoryPakSizeLimit = 30;
        // Load in memory paks from _FastLoad folder
        nLoadCache = 0;
        // Load menucommon/gamemodeswitch paks
        nLoadModePaks = 0;

        nStreamCache = STREAM_CACHE_DEFAULT;
        // Which file location to favor (loose vs. pak files)

        //????????
        nPriority = PAK_PRIORITY_DEFAULT;
#if defined(_RELEASE)
        nPriority  = ePakPriorityPakOnly; // Only read from pak files by default
#else
        nPriority  = ePakPriorityFileFirst;
#endif

        nMessageInvalidFileAccess = 0;

        //????????
        nLogInvalidFileAccess = BUILD_VALUE(1, 0);
#ifndef _RELEASE
        nLogInvalidFileAccess = 1;
#else
        nLogInvalidFileAccess = 0;
#endif

        nLoadFrontendShaderCache = FRONTEND_SHADER_CACHE_DEFAULT;
        nDisableNonLevelRelatedPaks = 1;
        // Whether to treat failed pak access as a warning or log message
        nWarnOnPakAccessFails = 1;

#ifndef _RELEASE
        nLogAllFileAccess = 0;
#endif
    }
};


#endif // CRYINCLUDE_CRYSYSTEM_PAKVARS_H
