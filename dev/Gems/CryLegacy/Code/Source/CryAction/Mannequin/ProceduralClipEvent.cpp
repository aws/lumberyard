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

#include "CryLegacy_precompiled.h"

#include "ICryMannequin.h"

#include <Mannequin/Serialization.h>

struct SProceduralClipEventParams
    : public IProceduralParams
{
    virtual void Serialize(Serialization::IArchive& ar)
    {
        ar(eventName, "EventName", "Event Name");
    }

    virtual void GetExtraDebugInfo(StringWrapper& extraInfoOut) const override
    {
        extraInfoOut = eventName.c_str();
    }

    SProcDataCRC eventName;
};

class CProceduralClipEvent
    : public TProceduralClip<SProceduralClipEventParams>
{
public:
    CProceduralClipEvent()
    {
    }

    virtual void OnEnter(float blendTime, float duration, const SProceduralClipEventParams& params)
    {
        SendActionEvent(params.eventName.crc);
    }

    virtual void OnExit(float blendTime) {}

    virtual void Update(float timePassed) {}
};

REGISTER_PROCEDURAL_CLIP(CProceduralClipEvent, "ActionEvent");

//------

// Holds the arguments that are passed into the lua function.
struct SProceduralLuaCallbackParams
    : public SProceduralParams
{
    float arg0;
    float arg1;
    float arg2;
    float arg3;
};

class CProceduralClipLuaCallback
    : public TProceduralClip<SProceduralLuaCallbackParams>
{
public:
    CProceduralClipLuaCallback()
    { }

    ~CProceduralClipLuaCallback()
    {
        SAFE_RELEASE(m_scriptTable);
    }

    virtual void OnEnter(float blendTime, float duration, const SProceduralLuaCallbackParams& params)
    {
        if (!m_entity)
        {
            CryLogAlways("Procedural clip Lua callback does not support AZ Entity.");
            return;
        }

        m_scriptTable = m_entity->GetScriptTable();
        if (m_scriptTable)
        {
            m_scriptTable->AddRef();

            if (m_scriptTable->HaveValue(params.dataString.c_str()))
            {
                const SProceduralLuaCallbackParams& callbackParams = static_cast<const SProceduralLuaCallbackParams&>(params);
                Script::CallMethod(m_scriptTable, params.dataString.c_str(), params.dataString2.c_str(), callbackParams.arg0, callbackParams.arg1, callbackParams.arg2, callbackParams.arg3);
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "No method \"%s\" found in the script table. ", params.dataString.c_str());
            }
        }
        else if (gEnv->IsEditor())
        {
            CryLogAlways("LuaCallback has no script table. Ignore this message if you are in Mannequin Editor.");
        }
    }

    virtual void OnExit(float blendTime) {}

    virtual void Update(float timePassed) {}

private:
    SmartScriptTable m_scriptTable;
};

REGISTER_PROCEDURAL_CLIP(CProceduralClipLuaCallback, "LUACallback");