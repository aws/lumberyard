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

//   Description : Script Binding for Item

#ifndef CRYINCLUDE_CRYAISYSTEM_BLACKBOARD_H
#define CRYINCLUDE_CRYAISYSTEM_BLACKBOARD_H
#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>
#include <IBlackBoard.h>

class CBlackBoard
    : public IBlackBoard
{
public:
    virtual ~CBlackBoard(){}

    CBlackBoard();

    virtual SmartScriptTable& GetForScript() { return m_BB; }
    virtual void                            SetFromScript(SmartScriptTable& sourceBB);
    virtual void                            Clear() { m_BB->Clear(); }

private:
    SmartScriptTable    m_BB;
};

#endif // CRYINCLUDE_CRYAISYSTEM_BLACKBOARD_H
