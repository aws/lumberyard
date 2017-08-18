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

#ifndef CRYINCLUDE_EDITOR_AI_SCRIPTBIND_AI_H
#define CRYINCLUDE_EDITOR_AI_SCRIPTBIND_AI_H
#pragma once



#include "ScriptHelpers.h"

struct IFunctionHandler;


// These numerical values are deprecated; use the strings instead
enum EAIPathType
{
    AIPATH_DEFAULT,
    AIPATH_HUMAN,
    AIPATH_HUMAN_COVER,
    AIPATH_CAR,
    AIPATH_TANK,
    AIPATH_BOAT,
    AIPATH_HELI,
    AIPATH_3D,
    AIPATH_SCOUT,
    AIPATH_TROOPER,
    AIPATH_HUNTER,
};


class CScriptBind_AI
    : public CScriptableBase
{
public:

    CScriptBind_AI(ISystem* pSystem);

    // Assign AgentPathfindingProperties to the given path type
    //
    int  AssignPFPropertiesToPathType(IFunctionHandler* pH);
    void AssignPFPropertiesToPathType(const string& sPathType, AgentPathfindingProperties& properties);

private:
    static const char* GetPathTypeName(EAIPathType pathType);
};

#endif // CRYINCLUDE_EDITOR_AI_SCRIPTBIND_AI_H
