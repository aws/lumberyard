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

// Description : Binding of network functions into script


#ifndef CRYINCLUDE_CRYACTION_NETWORK_SCRIPTBIND_NETWORK_H
#define CRYINCLUDE_CRYACTION_NETWORK_SCRIPTBIND_NETWORK_H
#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

class CGameContext;
class CCryAction;

class CScriptBind_Network
    : public CScriptableBase
{
public:
    CScriptBind_Network(ISystem* pSystem, CCryAction* pFW);
    virtual ~CScriptBind_Network();

    void Release() { delete this; };

    //! <code>Network.Expose()</code>
    int Expose(IFunctionHandler* pFH);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
private:
    void RegisterGlobals();
    void RegisterMethods();

    ISystem* m_pSystem;
    CCryAction* m_pFW;
};

#endif // CRYINCLUDE_CRYACTION_NETWORK_SCRIPTBIND_NETWORK_H
