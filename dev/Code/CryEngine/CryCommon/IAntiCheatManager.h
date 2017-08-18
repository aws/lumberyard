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

#ifndef CRYINCLUDE_CRYCOMMON_IANTICHEATMANAGER_H
#define CRYINCLUDE_CRYCOMMON_IANTICHEATMANAGER_H
#pragma once


#if !defined(_RELEASE) && !PROFILE_PERFORMANCE_NET_COMPATIBLE
#define DEV_CHEAT_HANDLING
#endif


typedef int32 TCheatType;
typedef int32 TCheatAssetGroup;
typedef int32 TAntiCheatVarIdx;

struct IAntiCheatManager
{
    // <interfuscator:shuffle>
    virtual ~IAntiCheatManager(){}
    virtual int RetrieveHashMethod() = 0;
    virtual int GetAssetGroupCount() = 0;
    virtual XmlNodeRef GetFileProbeConfiguration() = 0;

    virtual void FlagActivity(TCheatType type, ChannelId channelId, const char* message) = 0;
    virtual void FlagActivity(TCheatType type, ChannelId channelId) = 0;
    virtual void FlagActivity(TCheatType type, ChannelId channelId, float param1) = 0;
    virtual void FlagActivity(TCheatType type, ChannelId channelId, float param1, float param2) = 0;    // Please update MAX_FLAG_ACTIVITY_PARAMS if you add more parameters
    virtual void FlagActivity(TCheatType type, ChannelId channelId, float param1, const char* message) = 0;
    virtual void FlagActivity(TCheatType type, ChannelId channelId, float param1, float param2, const char* message) = 0;


    virtual TCheatType FindCheatType(const char* name) = 0;
    virtual TCheatAssetGroup FindAssetTypeByExtension(const char* ext) = 0;
    virtual TCheatAssetGroup FindAssetTypeByWeight() = 0;
    virtual void InitSession() = 0;
    virtual void OnSessionEnd() = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IANTICHEATMANAGER_H
