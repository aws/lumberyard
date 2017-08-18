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

// Description : Temp file holding code extracted from CAISystem.h/cpp


#ifndef CRYINCLUDE_CRYAISYSTEM_CALCULATIONSTOPPER_H
#define CRYINCLUDE_CRYAISYSTEM_CALCULATIONSTOPPER_H
#pragma once


//#define CALIBRATE_STOPPER
//#define DEBUG_STOPPER
//#define STOPPER_CAN_USE_COUNTER

/// Used to determine when a calculation should stop. Normally this would be after a certain time (dt). However,
/// if m_useCounter has been set then the times are internally converted to calls to ShouldCalculationStop using
/// callRate, which should have been estimated previously
class CCalculationStopper
{
public:
    /// name is used during calibration. fCalculationTime is the amount of time (in seconds).
    /// fCallsPerSecond is for when we are running in "counter" mode - the time
    /// gets converted into a number of calls to ShouldCalculationStop
    CCalculationStopper(const char* szName, float fCalculationTime, float fCallsPerSecond);
    bool ShouldCalculationStop() const;
    float GetSecondsRemaining() const;

#ifdef STOPPER_CAN_USE_COUNTER
    static bool m_useCounter;
#endif

private:
    CTimeValue m_endTime;

#ifdef STOPPER_CAN_USE_COUNTER
    mutable unsigned m_stopCounter; // if > 0 use this - stop when it's 0
    float m_fCallsPerSecond;
#endif // STOPPER_CAN_USE_COUNTER

#ifdef DEBUG_STOPPER
    static bool m_neverStop; // for debugging
#endif

#ifdef CALIBRATE_STOPPER
public:
    mutable unsigned m_calls;
    float m_dt;
    string m_name;
    /// the pair is calls and time (seconds)
    typedef std::map< string, std::pair<unsigned, float> > TMapCallRate;
    static TMapCallRate m_mapCallRate;
#endif
};


#endif // CRYINCLUDE_CRYAISYSTEM_CALCULATIONSTOPPER_H
