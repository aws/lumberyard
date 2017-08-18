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

// Description : Script Binding for MusicLogic


#include "StdAfx.h"
#include "ScriptBind_MusicLogic.h"
#include <IMusicSystem.h>

//////////////////////////////////////////////////////////////////////////
CScriptBind_MusicLogic::CScriptBind_MusicLogic(IScriptSystem* const pScriptSystem, ISystem* const pSystem)
{
    m_pSS = pScriptSystem;
    m_pMusicLogic = gEnv->pMusicSystem->GetMusicLogic();

    CScriptableBase::Init(pScriptSystem, pSystem);
    SetGlobalName("MusicLogic");

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_MusicLogic::

    RegisterMethods();
    RegisterGlobals();
}

//////////////////////////////////////////////////////////////////////////
CScriptBind_MusicLogic::~CScriptBind_MusicLogic()
{
}

//////////////////////////////////////////////////////////////////////////
void CScriptBind_MusicLogic::RegisterMethods()
{
    SCRIPT_REG_TEMPLFUNC(SetEvent, "eMusicLogicEvent, fValue");
    SCRIPT_REG_TEMPLFUNC(SendEvent, "pEventName");
    SCRIPT_REG_TEMPLFUNC(StartLogic, "sNamePreset");
    SCRIPT_REG_TEMPLFUNC(StopLogic, "");
}

//////////////////////////////////////////////////////////////////////////
void CScriptBind_MusicLogic::RegisterGlobals()
{
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_ENTER_VEHICLE", Audio::eMUSICLOGICEVENT_VEHICLE_ENTER);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_LEAVE_VEHICLE", Audio::eMUSICLOGICEVENT_VEHICLE_LEAVE);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_MOUNT_WEAPON", Audio::eMUSICLOGICEVENT_WEAPON_MOUNT);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_UNMOUNT_WEAPON", Audio::eMUSICLOGICEVENT_WEAPON_UNMOUNT);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_SNIPERMODE_ENTER", Audio::eMUSICLOGICEVENT_SNIPERMODE_ENTER);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_SNIPERMODE_LEAVE", Audio::eMUSICLOGICEVENT_SNIPERMODE_LEAVE);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_CLOAKMODE_ENTER", Audio::eMUSICLOGICEVENT_CLOAKMODE_ENTER);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_CLOAKMODE_LEAVE", Audio::eMUSICLOGICEVENT_CLOAKMODE_LEAVE);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_ENEMY_SPOTTED", Audio::eMUSICLOGICEVENT_ENEMY_SPOTTED);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_ENEMY_KILLED", Audio::eMUSICLOGICEVENT_ENEMY_KILLED);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_ENEMY_HEADSHOT", Audio::eMUSICLOGICEVENT_ENEMY_HEADSHOT);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_ENEMY_OVERRUN", Audio::eMUSICLOGICEVENT_ENEMY_OVERRUN);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_PLAYER_WOUNDED", Audio::eMUSICLOGICEVENT_PLAYER_WOUNDED);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_PLAYER_KILLED", Audio::eMUSICLOGICEVENT_PLAYER_KILLED);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_PLAYER_SPOTTED", Audio::eMUSICLOGICEVENT_PLAYER_SPOTTED);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_PLAYER_TURRET_ATTACK", Audio::eMUSICLOGICEVENT_PLAYER_TURRET_ATTACK);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_PLAYER_SWIM_ENTER", Audio::eMUSICLOGICEVENT_PLAYER_SWIM_ENTER);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_PLAYER_SWIM_LEAVE", Audio::eMUSICLOGICEVENT_PLAYER_SWIM_LEAVE);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_EXPLOSION", Audio::eMUSICLOGICEVENT_EXPLOSION);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_FACTORY_CAPTURED", Audio::eMUSICLOGICEVENT_FACTORY_CAPTURED);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_FACTORY_LOST", Audio::eMUSICLOGICEVENT_FACTORY_LOST);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_FACTORY_RECAPTURED", Audio::eMUSICLOGICEVENT_FACTORY_RECAPTURED);
    m_pSS->SetGlobalValue("MUSICLOGICEVENT_VEHICLE_CREATED", Audio::eMUSICLOGICEVENT_VEHICLE_CREATED);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_MusicLogic::SetEvent(IFunctionHandler* pH, int eMusicLogicEvent)
{
    if (m_pMusicLogic != nullptr)
    {
        switch (eMusicLogicEvent)
        {
        case Audio::eMUSICLOGICEVENT_VEHICLE_ENTER:
        case Audio::eMUSICLOGICEVENT_VEHICLE_LEAVE:
        case Audio::eMUSICLOGICEVENT_WEAPON_MOUNT:
        case Audio::eMUSICLOGICEVENT_WEAPON_UNMOUNT:
        case Audio::eMUSICLOGICEVENT_ENEMY_SPOTTED:
        case Audio::eMUSICLOGICEVENT_ENEMY_KILLED:
        case Audio::eMUSICLOGICEVENT_ENEMY_HEADSHOT:
        case Audio::eMUSICLOGICEVENT_ENEMY_OVERRUN:
        case Audio::eMUSICLOGICEVENT_PLAYER_WOUNDED:
            m_pMusicLogic->SetEvent(static_cast<Audio::EMusicLogicEvents>(eMusicLogicEvent));
            break;
        case Audio::eMUSICLOGICEVENT_MAX:
            break;
        default:
            break;
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_MusicLogic::StartLogic(IFunctionHandler* pH)
{
    int const nParamCount = pH->GetParamCount();

    if (nParamCount < 1)
    {
        m_pSS->RaiseError("MusicLogic.StartLogic wrong number of arguments!");

        return pH->EndFunction(-1);
    }

    if (m_pMusicLogic != nullptr)
    {
        char const* sNamePreset = nullptr;

        if (!pH->GetParam(1, sNamePreset))
        {
            m_pSS->RaiseError("MusicLogic.StartLogic: first parameter is not a string!");
            return pH->EndFunction(-1);
        }

        m_pMusicLogic->Start(sNamePreset);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_MusicLogic::StopLogic(IFunctionHandler* pH)
{
    if (m_pMusicLogic != nullptr)
    {
        m_pMusicLogic->Stop();
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_MusicLogic::SendEvent(IFunctionHandler* pH, const char* pEventName)
{
    if (m_pMusicLogic != nullptr)
    {
        m_pMusicLogic->SendEvent(m_pMusicLogic->GetEventId(pEventName));
    }

    return pH->EndFunction();
}
