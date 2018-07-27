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
#include "CalculationStopper.h"

#ifdef DEBUG_STOPPER
bool CCalculationStopper::m_neverStop = false;
#endif

#ifdef STOPPER_CAN_USE_COUNTER
bool CCalculationStopper::m_useCounter;
#endif

#ifdef CALIBRATE_STOPPER
std::map< string, std::pair<unsigned, float> > CCalculationStopper::m_mapCallRate;
#endif

//===================================================================
// CCalculationStopper
//===================================================================
CCalculationStopper::CCalculationStopper(const char* szName, float fCalculationTime, float fCallsPerSecond)

#ifdef STOPPER_CAN_USE_COUNTER
    : m_stopCounter(0)
    ,  m_fCallsPerSecond(fCallsPerSecond)
#endif

{
    m_endTime = gEnv->pTimer->GetAsyncTime() + CTimeValue(fCalculationTime);

#ifdef STOPPER_CAN_USE_COUNTER
    if (m_useCounter)
    {
        m_stopCounter = (unsigned)(dt * fCallsPerSecond);
        if (m_stopCounter < 1)
        {
            m_stopCounter = 1;
        }
    }
#endif

#ifdef CALIBRATE_STOPPER
    m_name = name;
    m_calls = 0;
    m_dt = dt;
#endif
}

bool CCalculationStopper::ShouldCalculationStop() const
{
#ifdef DEBUG_STOPPER
    if (m_neverStop)
    {
        return false;
    }
#endif

#ifdef STOPPER_CAN_USE_COUNTER
    if (m_useCounter)
    {
        if (m_stopCounter > 0)
        {
            --m_stopCounter;
        }
        return m_stopCounter == 0;
    }
    else
#endif

    {
#ifdef CALIBRATE_STOPPER
        ++m_calls;
        if (gEnv->pTimer->GetAsyncTime() > m_endTime)
        {
            std::pair<unsigned, float>& record = m_mapCallRate[m_name];
            record.first += m_calls;
            record.second += m_dt;
            return true;
        }
        else
        {
            return false;
        }
#else
        return gEnv->pTimer->GetAsyncTime() > m_endTime;
#endif
    }
}

float CCalculationStopper::GetSecondsRemaining() const
{
#ifdef STOPPER_CAN_USE_COUNTER
    if (m_useCounter)
    {
        return m_stopCounter / m_fCallsPerSecond;
    }
    else
#endif
    {
        return (m_endTime - gEnv->pTimer->GetAsyncTime()).GetSeconds();
    }
}