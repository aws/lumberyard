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

// Description : CVars for the FlowSystem

#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWSYSTEMCVARS_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWSYSTEMCVARS_H
#pragma once

struct ICVar;

struct CFlowSystemCVars
{
    int m_enableUpdates;     // Enable/Disable the whole FlowSystem
    int m_profile;                   // Profile the FlowSystem
    int m_abortOnLoadError;  // Abort execution when an error in FlowGraph loading is encountered
    int m_inspectorLog;      // Log inspector to console
    int m_noDebugText;       // Don't show debug text from HUD nodes (default 0)

    int m_enableFlowgraphNodeDebugging;
    int m_debugNextStep;

    static __inline CFlowSystemCVars& Get()
    {
        assert (s_pThis != 0);
        return *s_pThis;
    }

private:
    friend class CFlowSystem; // Our only creator

    CFlowSystemCVars(); // singleton stuff
    ~CFlowSystemCVars();
    CFlowSystemCVars(const CFlowSystemCVars&);
    CFlowSystemCVars& operator= (const CFlowSystemCVars&);

    static CFlowSystemCVars* s_pThis;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWSYSTEMCVARS_H
