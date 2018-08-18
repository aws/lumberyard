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

// Description : Dialog System


#ifndef CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGSYSTEM_H
#define CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGSYSTEM_H
#pragma once

#include <SerializeFwd.h>

#include "DialogScript.h"
#include "IDialogSystem.h"
#include "ILevelSystem.h"
#include "DialogQueuesManager.h"

class CDialogSession;
class CDialogActorContext;

#ifndef _RELEASE
#define DEBUGINFO_DIALOGBUFFER
#endif

class CDialogSystem
    : public IDialogSystem
    , ILevelSystemListener
{
public:
    typedef int SessionID;
    typedef int ActorContextID;

    CDialogSystem();
    virtual ~CDialogSystem();

    void GetMemoryStatistics(ICrySizer* s);

    // Later go into IDialogSystem i/f
    virtual void Update(const float dt);
    virtual void Shutdown();
    virtual void Serialize(TSerialize ser);   // serializes load/save. After load serialization PostLoad needs to be called

    // IDialogSystem
    virtual bool Init();
    virtual void Reset(bool bUnload);
    virtual IDialogScriptIteratorPtr CreateScriptIterator();
    virtual bool ReloadScripts(const char* levelName = NULL);
    // ~IDialogSystem

    // ILevelSystemListener
    virtual void OnLevelNotFound(const char* levelName) {};
    virtual void OnLoadingStart(ILevelInfo* pLevel) {};
    virtual void OnLoadingLevelEntitiesStart(ILevelInfo* pLevelInfo) {}
    virtual void OnLoadingComplete(ILevel* pLevel);
    virtual void OnLoadingError(ILevelInfo* pLevel, const char* error) {};
    virtual void OnLoadingProgress(ILevelInfo* pLevel, int progressAmount) {};
    virtual void OnUnloadComplete(ILevel* pLevel) {};
    // ~ILevelSystemListener

    SessionID CreateSession(const string& scriptID);
    bool      DeleteSession(SessionID id);
    CDialogSession* GetSession(SessionID id) const;
    CDialogSession* GetActiveSession(SessionID id) const;
    CDialogActorContext* GetActiveSessionActorContext(ActorContextID id) const;
    const CDialogScript* GetScriptByID(const string& scriptID) const;

    virtual bool IsEntityInDialog(EntityId entityId) const;
    bool FindSessionAndActorForEntity(EntityId entityId, SessionID& outSessionID, CDialogScript::TActorID& outActorId) const;

    // called from CDialogSession
    bool AddSession(CDialogSession* pSession);
    bool RemoveSession(CDialogSession* pSession);

    // Debug dumping
    void Dump(int verbosity = 0);
    void DumpSessions();

    CDialogQueuesManager* GetDialogQueueManager() { return &m_dialogQueueManager; }

    static int sDiaLOGLevel;    // CVar ds_LogLevel
    static int sPrecacheSounds; // CVar ds_PrecacheSounds
    static int sAutoReloadScripts; // CVar to reload scripts when jumping into GameMode
    static int sLoadSoundSynchronously;
    static int sLoadExcelScripts; // CVar to load legacy Excel based Dialogs
    static int sWarnOnMissingLoc; // CVar ds_WarnOnMissingLoc
    static ICVar* ds_LevelNameOverride;


protected:
    void ReleaseScripts();
    void ReleaseSessions();
    void ReleasePendingDeletes();
    void RestoreSessions();
    CDialogSession* InternalCreateSession(const string& scriptID, SessionID sessionID);

protected:
    class CDialogScriptIterator;
    typedef std::map<SessionID, CDialogSession*> TDialogSessionMap;
    typedef std::vector<CDialogSession*> TDialogSessionVec;

    int               m_nextSessionID;
    TDialogScriptMap  m_dialogScriptMap;
    TDialogSessionMap m_allSessions;
    TDialogSessionVec m_activeSessions;
    TDialogSessionVec m_pendingDeleteSessions;
    std::vector<SessionID> m_restoreSessions;
    CDialogQueuesManager m_dialogQueueManager;
};

#endif // CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGSYSTEM_H
