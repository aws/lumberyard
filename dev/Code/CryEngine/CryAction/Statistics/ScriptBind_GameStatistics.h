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

#ifndef CRYINCLUDE_CRYACTION_STATISTICS_SCRIPTBIND_GAMESTATISTICS_H
#define CRYINCLUDE_CRYACTION_STATISTICS_SCRIPTBIND_GAMESTATISTICS_H
#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

#define STATISTICS_CHECK_BINDING 1

struct IStatsTracker;
class CGameStatistics;

class CScriptBind_GameStatistics
    : public CScriptableBase
{
public:
    CScriptBind_GameStatistics(CGameStatistics* pGS);
    virtual ~CScriptBind_GameStatistics ();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
#if STATISTICS_CHECK_BINDING
        pSizer->AddObject(m_boundTrackers);
#endif
    }

    //! <code>GameStatistics.PushGameScope(scopeID)</code>
    //!     <param name="scopeID">identifier of the scope to be placed on top of the stack.</param>
    //! <description>Pushes a scope on top of the stack.</description>
    virtual int PushGameScope(IFunctionHandler* pH, int scopeID);

    //! <code>GameStatistics.PopGameScope( [checkScopeId] )</code>
    //!     <param name="checkScopeId">(optional)</param>
    //! <description>Removes the scope from the top of the stack.</description>
    virtual int PopGameScope(IFunctionHandler* pH);

    //! <code>GameStatistics.CurrentScope()</code>
    //! <description>Returns the ID of current scope, -1 if stack is empty.</description>
    virtual int CurrentScope(IFunctionHandler* pH);



    //! <code>GameStatistics.AddGameElement( scopeID, elementID, locatorID, locatorValue [, table])</code>
    //! <description>Adds a game element to specified scope.</description>
    virtual int AddGameElement(IFunctionHandler* pH);

    //! <code>GameStatistics.RemoveGameElement( scopeID, elementID, locatorID, locatorValue )</code>
    //! <description>Removes element from the specified scope if data parameters match.</description>
    virtual int RemoveGameElement(IFunctionHandler* pH);

    //! <code>GameStatistics.Event()</code>
    virtual int Event(IFunctionHandler* pH);

    //! <code>GameStatistics.StateValue( )</code>
    virtual int StateValue(IFunctionHandler* pH);

    //! <code>GameStatistics.BindTracker( name, tracker )</code>
    bool BindTracker(IScriptTable* pT, const char* name, IStatsTracker* tracker);

    //! <code>GameStatistics.UnbindTracker( name, tracker )</code>
    bool UnbindTracker(IScriptTable* pT, const char* name, IStatsTracker* tracker);

private:
    void RegisterMethods();
    void RegisterGlobals();
    static IStatsTracker* GetTracker(IFunctionHandler* pH);
    static IGameStatistics* GetStats(IFunctionHandler* pH);

    //CScriptableBase m_StatsTracker; //[FINDME][SergeyR]: ctor is protected
    CGameStatistics* m_pGS;
    IScriptSystem* m_pSS;

#if STATISTICS_CHECK_BINDING
    std::set<IStatsTracker*> m_boundTrackers;
#endif
};

#endif // CRYINCLUDE_CRYACTION_STATISTICS_SCRIPTBIND_GAMESTATISTICS_H
