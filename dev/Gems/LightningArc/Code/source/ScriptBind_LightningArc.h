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
#ifndef _ENVIRONMENT_SCRIPTBIND_LIGHTNINGARC_H_
#define _ENVIRONMENT_SCRIPTBIND_LIGHTNINGARC_H_
#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

class CLightningArc;

class CScriptBind_LightningArc
    : public CScriptableBase
{
public:
    CScriptBind_LightningArc(ISystem* pSystem);

    void AttachTo(CLightningArc* pLightingArc);

    int TriggerSpark(IFunctionHandler* pFunction);
    int Enable(IFunctionHandler* pFunction, bool enable);
    int ReadLuaParameters(IFunctionHandler* pFunction);
};

#endif//_ENVIRONMENT_SCRIPTBIND_LIGHTNINGARC_H_
