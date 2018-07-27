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

// Description : Signal entities based on configurable timers


#ifndef CRYINCLUDE_CRYACTION_SIGNALTIMERS_SIGNALTIMERS_H
#define CRYINCLUDE_CRYACTION_SIGNALTIMERS_SIGNALTIMERS_H
#pragma once

class CPersonalSignalTimer;

class CSignalTimer
{
public:

    /*$1- Singleton Stuff ----------------------------------------------------*/
    static CSignalTimer&  ref();
    static bool           Create();
    static void           Shutdown();
    void                  Reset();

    /*$1- IEditorSetGameModeListener -----------------------------------------*/
    void  OnEditorSetGameMode(bool bGameMode);
    void  OnProxyReset(EntityId IdEntity);

    /*$1- Basics -------------------------------------------------------------*/
    void  Init();
    bool  Update(float fElapsedTime);

    /*$1- Utils --------------------------------------------------------------*/
    bool  EnablePersonalManager(EntityId IdEntity, const char* sSignal);
    bool  DisablePersonalSignalTimer(EntityId IdEntity, const char* sSignal);
    bool  ResetPersonalTimer(EntityId IdEntity, const char* sSignal);
    bool  EnableAllPersonalManagers(EntityId IdEntity);
    bool  DisablePersonalSignalTimers(EntityId IdEntity);
    bool  ResetPersonalTimers(EntityId IdEntity);
    bool  SetTurnRate(EntityId IdEntity, const char* sSignal, float fTime, float fTimeMax = -1.0f);
    void  SetDebug(bool bDebug);
    bool  GetDebug() const;

protected:

    /*$1- Creation and destruction via singleton -----------------------------*/
    CSignalTimer();
    virtual ~CSignalTimer();

    /*$1- Utils --------------------------------------------------------------*/
    CPersonalSignalTimer*   GetPersonalSignalTimer(EntityId IdEntity, const char* sSignal) const;
    CPersonalSignalTimer*   CreatePersonalSignalTimer(EntityId IdEntity, const char* sSignal);

private:

    /*$1- Members ------------------------------------------------------------*/
    bool                                m_bInit;
    bool                                m_bDebug;
    static CSignalTimer*                m_pInstance;
    std::vector<CPersonalSignalTimer*> m_vecPersonalSignalTimers;
};
#endif // CRYINCLUDE_CRYACTION_SIGNALTIMERS_SIGNALTIMERS_H
