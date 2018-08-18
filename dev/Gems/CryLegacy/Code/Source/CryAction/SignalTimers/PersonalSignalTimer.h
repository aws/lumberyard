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

// Description : Manager per-actor signal timers

#ifndef CRYINCLUDE_CRYACTION_SIGNALTIMERS_PERSONALSIGNALTIMER_H
#define CRYINCLUDE_CRYACTION_SIGNALTIMERS_PERSONALSIGNALTIMER_H
#pragma once

#include "AIProxy.h"

class CSignalTimer;

class CPersonalSignalTimer
    : public IAIProxyListener
{
public:
    // Base ----------------------------------------------------------
    CPersonalSignalTimer(CSignalTimer* pParent);
    virtual         ~CPersonalSignalTimer();
    bool            Init(EntityId Id, const char* sSignal);
    bool            Update(float fElapsedTime, uint32 uDebugOrder = 0);
    void            ForceReset(bool bAlsoEnable = true);
    void            OnProxyReset();

    // Utils ---------------------------------------------------------
    void            SetEnabled(bool bEnabled);
    void            SetRate(float fNewRateMin, float fNewRateMax);
    EntityId        GetEntityId() const;
    const string&   GetSignalString() const;

private:
    void            Reset(bool bAlsoEnable = true);
    void            SendSignal();
    IEntity*         GetEntity();
    IEntity const*   GetEntity() const;
    void            DebugDraw(uint32 uOrder) const;

    // IAIProxyListener
    void SetListener(bool bAdd);
    virtual void OnAIProxyEnabled(bool bEnabled);
    // ~IAIProxyListener

private:

    bool            m_bInit;
    CSignalTimer*   m_pParent;
    EntityId        m_EntityId;
    string          m_sSignal;
    float           m_fRateMin;
    float           m_fRateMax;
    float           m_fTimer;
    float           m_fTimerSinceLastReset;
    int             m_iSignalsSinceLastReset;
    bool            m_bEnabled;
    IFFont* m_pDefaultFont;
};
#endif // CRYINCLUDE_CRYACTION_SIGNALTIMERS_PERSONALSIGNALTIMER_H
