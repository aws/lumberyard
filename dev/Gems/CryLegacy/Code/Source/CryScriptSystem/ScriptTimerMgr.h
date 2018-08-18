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

// Description : interface for the CScriptTimerMgr class.


#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTTIMERMGR_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTTIMERMGR_H

#pragma once
#include <map>

#include <IScriptSystem.h>
#include <TimeValue.h>
#include <ISerialize.h>

//////////////////////////////////////////////////////////////////////////
struct ScriptTimer
{
    int nTimerID;
    int64 nMillis;
    int64 nEndTime;
    int64 nStartTime;
    char sFuncName[128];
    HSCRIPTFUNCTION pScriptFunction;
    SmartScriptTable pUserData;
    bool bUpdateDuringPause;

    ScriptTimer()
        : nTimerID(0)
        , nMillis(0)
        , nStartTime(0)
        , nEndTime(0)
        , bUpdateDuringPause(0)
        , pScriptFunction(0) { sFuncName[0] = 0; }
    void GetMemoryUsage(ICrySizer* pSizer) const { /*nothing*/}
};

typedef std::map<int, ScriptTimer*> ScriptTimerMap;
typedef ScriptTimerMap::iterator  ScriptTimerMapItor;

//////////////////////////////////////////////////////////////////////////
class CScriptTimerMgr
{
public:
    CScriptTimerMgr(IScriptSystem* pScriptSystem);
    virtual ~CScriptTimerMgr();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void Serialize(TSerialize& ser);

    // Create a new timer and put it in the list of managed timers.
    int     AddTimer(ScriptTimer& timer);
    void    RemoveTimer(int nTimerID);
    void    Update(int64 nCurrentTime);
    void    Reset();
    void    Pause(bool bPause);
private:
    void DeleteTimer(ScriptTimer* pTimer);

    ScriptTimerMap m_mapTimers;
    ScriptTimerMap m_mapTempTimers;
    IScriptSystem* m_pScriptSystem;
    int     m_nLastTimerID;
    int     m_nTimerIDCurrentlyBeingCalled;
    bool    m_bPause;
};

#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTTIMERMGR_H
