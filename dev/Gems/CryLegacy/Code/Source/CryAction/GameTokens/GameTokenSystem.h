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

#ifndef CRYINCLUDE_CRYACTION_GAMETOKENS_GAMETOKENSYSTEM_H
#define CRYINCLUDE_CRYACTION_GAMETOKENS_GAMETOKENSYSTEM_H
#pragma once

#include "IGameTokens.h"
#include "StlUtils.h"

class CGameToken;

//////////////////////////////////////////////////////////////////////////
// Game Token Manager implementation class.
//////////////////////////////////////////////////////////////////////////
class CGameTokenSystem
    : public IGameTokenSystem
{
    friend class CGameTokenIterator;
public:
    CGameTokenSystem();
    ~CGameTokenSystem();

    //////////////////////////////////////////////////////////////////////////
    // IGameTokenSystem
    //////////////////////////////////////////////////////////////////////////
    virtual IGameToken* SetOrCreateToken(const char* sTokenName, const TFlowInputData& defaultValue);
    virtual void DeleteToken(IGameToken* pToken);
    virtual IGameToken* FindToken(const char* sTokenName);
    virtual void RenameToken(IGameToken* pToken, const char* sNewName);

    virtual IGameTokenIt* GetGameTokenIterator();

    virtual void RegisterListener(const char* sGameToken, IGameTokenEventListener* pListener, bool bForceCreate, bool bImmediateCallback);
    virtual void UnregisterListener(const char* sGameToken, IGameTokenEventListener* pListener);

    virtual void RegisterListener(IGameTokenEventListener* pListener);
    virtual void UnregisterListener(IGameTokenEventListener* pListener);

    virtual void Reset();
    virtual void Unload();
    virtual void Serialize(TSerialize ser);

    virtual void DebugDraw();

    virtual void LoadLibs(const char* sFileSpec);
    virtual void RemoveLibrary(const char* sPrefix);

    virtual void SerializeSaveLevelToLevel(const char** ppGameTokensList, uint32 numTokensToSave);
    virtual void SerializeReadLevelToLevel();

    virtual void GetMemoryStatistics(ICrySizer* s);

    //////////////////////////////////////////////////////////////////////////

    CGameToken* GetToken(const char* sTokenName);
    void Notify(EGameTokenEvent event, CGameToken* pToken);
    void DumpAllTokens();

private:
    bool _InternalLoadLibrary(const char* filename, const char* tag);

    // Use Hash map for speed.
    typedef AZStd::unordered_map<const char*, CGameToken*, stl::hash_string_caseless<const char*>, stl::equality_string_caseless<const char*> > GameTokensMap;

    typedef std::vector<IGameTokenEventListener*> Listeners;
    Listeners m_listeners;

    // Loaded libraries.
    std::vector<string> m_libraries;

    GameTokensMap* m_pGameTokensMap; // A pointer so it can be fully unloaded on level unload
    class CScriptBind_GameToken* m_pScriptBind;
    XmlNodeRef m_levelToLevelSave;

#ifdef _GAMETOKENSDEBUGINFO
    typedef std::set<string> TDebugListMap;
    TDebugListMap m_debugList;
    struct SDebugHistoryEntry
    {
        string tokenName;
        string value;
        CTimeValue timeChanged;
    };

    enum
    {
        DBG_HISTORYSIZE = 25
    };
    uint32 m_historyStart;  // start of the circular list in m_debugHistory.
    uint32 m_historyEnd;
    uint32 m_oldNumHistoryLines; // just to control dynamic changes without using a callback
    std::vector<SDebugHistoryEntry> m_debugHistory; // works like a circular list
    static int m_CVarShowDebugInfo;
    static int m_CVarPosX;
    static int m_CVarPosY;
    static int m_CVarNumHistoricLines;
    static ICVar* m_pCVarFilter;


    void DrawToken(const char* pTokenName, const char* pTokenValue, const CTimeValue& timeChanged, int line);
    void ClearDebugHistory();
    int GetHistoryBufferSize() { return min(int(DBG_HISTORYSIZE), m_CVarNumHistoricLines); }
    const char* GetTokenDebugString(CGameToken* pToken);


public:
    virtual void AddTokenToDebugList(const char* pToken);
    virtual void RemoveTokenFromDebugList(const char* pToken);

#endif
};

#endif // CRYINCLUDE_CRYACTION_GAMETOKENS_GAMETOKENSYSTEM_H
