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

// Description : Dialog System ScriptBinding


#ifndef CRYINCLUDE_CRYACTION_DIALOGSYSTEM_SCRIPTBIND_DIALOGSYSTEM_H
#define CRYINCLUDE_CRYACTION_DIALOGSYSTEM_SCRIPTBIND_DIALOGSYSTEM_H
#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

class CDialogSystem;

class CScriptBind_DialogSystem
    : public CScriptableBase
{
public:
    CScriptBind_DialogSystem(ISystem* pSystem, CDialogSystem* pDS);
    virtual ~CScriptBind_DialogSystem();

    void Release() { delete this; };

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
private:
    void RegisterGlobals();
    void RegisterMethods();

    int CreateSession(IFunctionHandler* pH, const char* scriptID);
    int DeleteSession(IFunctionHandler* pH, int sessionID);
    int SetActor(IFunctionHandler* pH, int sessionID, int actorID, ScriptHandle entity);
    int Play(IFunctionHandler* pH, int sessionID);
    int Stop(IFunctionHandler* pH, int sessionID);
    int IsEntityInDialog(IFunctionHandler* pH, ScriptHandle entity);

private:
    ISystem*       m_pSystem;
    IEntitySystem* m_pEntitySystem;
    CDialogSystem* m_pDS;
};

#endif // CRYINCLUDE_CRYACTION_DIALOGSYSTEM_SCRIPTBIND_DIALOGSYSTEM_H
