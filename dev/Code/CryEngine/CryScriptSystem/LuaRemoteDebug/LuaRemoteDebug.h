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

#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_LUAREMOTEDEBUG_LUAREMOTEDEBUG_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_LUAREMOTEDEBUG_LUAREMOTEDEBUG_H
#pragma once

#ifdef _RELEASE
#define LUA_REMOTE_DEBUG_ENABLED 0
#else
#define LUA_REMOTE_DEBUG_ENABLED 1
#endif

#if LUA_REMOTE_DEBUG_ENABLED

#include <INotificationNetwork.h>
#include <IScriptSystem.h>
#include "SerializationHelper.h"
#include "../ScriptSystem.h"

struct lua_State;
struct lua_Debug;
class CScriptSystem;

#define LUA_REMOTE_DEBUG_HOST_VERSION 7
#define LUA_REMOTE_DEBUG_CHANNEL "LuaDebug"

struct LuaBreakPoint
{
    int nLine;
    string sSourceFile;

    LuaBreakPoint() { nLine = -1; }
    LuaBreakPoint(const LuaBreakPoint& b)
    {
        nLine = b.nLine;
        sSourceFile = b.sSourceFile;
    }
    bool operator ==(const LuaBreakPoint& b) { return nLine == b.nLine && sSourceFile == b.sSourceFile; }
    bool operator !=(const LuaBreakPoint& b) { return nLine != b.nLine || sSourceFile != b.sSourceFile; }
};
typedef std::vector<LuaBreakPoint> LuaBreakPoints;

class CLuaRemoteDebug
    : public INotificationNetworkListener
{
public:
    enum EPacketType
    {
        ePT_Version,
        ePT_LogMessage,
        ePT_Break,
        ePT_Continue,
        ePT_StepOver,
        ePT_StepInto,
        ePT_StepOut,
        ePT_SetBreakpoints,
        ePT_ExecutionLocation,
        ePT_LuaVariables,
        ePT_BinaryFileDetected,
        ePT_GameFolder,
        ePT_FileMD5,
        ePT_FileContents,
        ePT_EnableCppCallstack,
        ePT_ScriptError,
    };

    enum EPlatform
    {
        eP_Unknown,
        eP_Win32,
        eP_Win64,
    };

    CLuaRemoteDebug(CScriptSystem* pScriptSystem);

    // Debug hook callback.
    void OnDebugHook(lua_State* L, lua_Debug* ar);
    void OnScriptError(lua_State* L, lua_Debug* ar, const char* errorStr);

    virtual void OnNotificationNetworkReceive(const void* pBuffer, size_t length);

    bool IsClientConnected() { return m_clientVersion != 0; }

private:
    void HaltExecutionLoop(lua_State* L, lua_Debug* ar, const char* errorStr = NULL);
    void ReceiveVersion(CSerializationHelper& buffer);
    void ReceiveSetBreakpoints(CSerializationHelper& buffer);
    void ReceiveFileMD5Request(CSerializationHelper& buffer);
    void ReceiveFileContentsRequest(CSerializationHelper& buffer);
    void ReceiveEnableCppCallstack(CSerializationHelper& buffer);
    void SendVersion();
    void SendLuaState(lua_Debug* ar);
    void SendVariables();
    void SendBinaryFileDetected();
    void SendGameFolder();
    void SendScriptError(const char* errorStr);
    void SendBuffer();
    void BreakOnNextLuaCall();
    void Continue();
    void StepOver();
    void StepInto();
    void StepOut();
    void SendLogMessage(const char* format, ...);
    bool HaveBreakPointAt(const char* sSourceFile, int nLine);
    void AddBreakPoint(const char* sSourceFile, int nLine);
    void RemoveBreakPoint(const char* sSourceFile, int nLine);
    string GetLineFromFile(const char* sFilename, int nLine);
    void SerializeLuaValue(const ScriptAnyValue& scriptValue, CSerializationHelper& buffer, int maxDepth);
    void SerializeLuaTable(IScriptTable* pTable, CSerializationHelper& buffer, int maxDepth);
    void GetLuaCallStack(std::vector<SLuaStackEntry>& callstack);

    CScriptSystem* m_pScriptSystem;
    CSerializationHelper m_sendBuffer;
    BreakState m_breakState;
    int m_nCallLevelUntilBreak;
    bool m_bForceBreak;
    bool m_bExecutionStopped;
    LuaBreakPoints m_breakPoints;
    lua_Debug* m_pHaltedLuaDebug;
    bool m_bBinaryLuaDetected;
    int m_clientVersion;
    bool m_enableCppCallstack;
};

#endif //LUA_REMOTE_DEBUG_ENABLED

#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_LUAREMOTEDEBUG_LUAREMOTEDEBUG_H
