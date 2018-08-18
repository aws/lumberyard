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

#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_SCRIPT_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_SCRIPT_H
#pragma once


#include <IScriptSystem.h>

/*!
*   <description>This class implements script-functions for exposing the scripting system functionalities.</description>
*   <remarks>After initialization of the script-object it will be globally accessable through scripts using the namespace "Script".</remarks>
*/
class CScriptBind_Script
    : public CScriptableBase
{
public:
    CScriptBind_Script(IScriptSystem* pScriptSystem, ISystem* pSystem);
    virtual ~CScriptBind_Script();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    //! <code>Script.LoadScript(scriptName)</code>
    //! <description>Loads the script.</description>
    //!     <param name="scriptName">name of the script to load</param>
    int LoadScript(IFunctionHandler* pH);

    //! <code>Script.ReloadScripts()</code>
    //! <description>Reloads all the scripts.</description>
    int ReloadScripts(IFunctionHandler* pH);

    //! <code>Script.ReloadScript()</code>
    //! <description>Reload the script.</description>
    int ReloadScript(IFunctionHandler* pH);

    //! <code>Script.ReloadEntityScript( className )</code>
    //!     <param name="className">Name of the entity script.</param>
    //! <description>Reloads the specified entity script.</description>
    int ReloadEntityScript(IFunctionHandler* pH, const char* className);

    //! <code>Script.UnloadScript()</code>
    //! <description>Unloads the script.</description>
    int UnloadScript(IFunctionHandler* pH);

    //! <code>Script.DumpLoadedScripts()</code>
    //! <description>Dumps all the loaded scripts.</description>
    int DumpLoadedScripts(IFunctionHandler* pH);

    //! <code>Script.SetTimer( nMilliseconds, luaFunction [, userData [, bUpdateDuringPause]] )</code>
    //! <description>
    //!    Set a general script timer, when timer expires will call back a specified lua function.
    //!    Lua function will accept 1 or 2 parameters,
    //!    if userData is specified luaFunction must be:
    //!      <pre>LuaCallback = function( userData,nTimerId )
    //!         -- function body
    //!      end;</pre>
    //!    if userData is not specified luaFunction must be:
    //!      <pre>LuaCallback = function( nTimerId )
    //!         -- function body
    //!      end;</pre>
    //! </description>
    //!     <param name="nMilliseconds">Delay of trigger in milliseconds.</param>
    //!     <param name="luaFunction">.</param>
    //!     <param name="userData">(optional) Any user defined table. If specified will be passed as a first argument of the callback function.</param>
    //!     <param name="bUpdateDuringPause">(optional) will be updated and trigger even if in pause mode.</param>
    //! <returns>ID assigned to this timer or nil if not specified.</returns>
    int SetTimer(IFunctionHandler* pH, int nMilliseconds, HSCRIPTFUNCTION hFunc);


    //! <code>Script.SetTimerForFunction( nMilliseconds, luaFunction [, userData [, bUpdateDuringPause]] )</code>
    //! <description>
    //!    Set a general script timer, when timer expires will call back a specified lua function.
    //!    Lua function will accept 1 or 2 parameters,
    //!    if userData is specified luaFunction must be:
    //!      <pre>LuaCallback = function( userData,nTimerId )
    //!         -- function body
    //!      end;</pre>
    //!    if userData is not specified luaFunction must be:
    //!      <pre>LuaCallback = function( nTimerId )
    //!         -- function body
    //!      end;</pre>
    //!.</description>
    //!     <param name="nMilliseconds">Delay of trigger in milliseconds.</param>
    //!     <param name="luaFunction">.</param>
    //!     <param name="userData"> (optional) Any user defined table. If specified it will be passed as a first argument of the callback function.</param>
    //!     <param name="bUpdateDuringPause"> (optional) will be updated and trigger even if in pause mode.</param>
    //! <returns>ID assigned to this timer or nil if not specified.</returns>
    int SetTimerForFunction(IFunctionHandler* pH, int nMilliseconds, const char* sFunctionName);

    //! <code>Script.KillTimer( nTimerId )</code>
    //! <description>Stops a timer set by the Script.SetTimer function.</description>
    //!     <param name="nTimerId">ID of the timer returned by the Script.SetTimer function.</param>
    int KillTimer(IFunctionHandler* pH, ScriptHandle nTimerId);
};

#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_SCRIPT_H
