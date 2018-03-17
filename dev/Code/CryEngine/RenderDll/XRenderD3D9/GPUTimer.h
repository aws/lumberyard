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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_GPUTIMER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_GPUTIMER_H
#pragma once

class CSimpleGPUTimer
{
public:
    CSimpleGPUTimer();
    ~CSimpleGPUTimer();

    static void EnableTiming();
    static void DisableTiming();

    static void AllowTiming();
    static void DisallowTiming();

    static bool IsTimingEnabled() { return s_bTimingEnabled; }
    static bool IsTimingAllowed() { return s_bTimingAllowed; }

    void Start(const char* name);
    void Stop();
    void UpdateTime();

    float GetTime() { return m_time; }
    float GetSmoothedTime() { return m_smoothedTime; }

    bool IsStarted() const { return m_bStarted; }
    bool HasPendingQueries() const { return m_bWaiting; }

    void Release();
    bool Init();

private:
    static bool s_bTimingEnabled;
    static bool s_bTimingAllowed;

    void RecordSlice(AZ::u64 timeStart, AZ::u64 timeStop, AZ::u64 frequency);

    float  m_time;
    float  m_smoothedTime;

    bool   m_bInitialized;
    bool   m_bStarted;
    bool   m_bWaiting;
    AZStd::string m_Name;

#if GPUTIMER_H_TRAIT_DEFINEQUERIES
    ID3D11Query* m_pQueryStart, * m_pQueryStop, * m_pQueryFreq;
#endif
};

#endif
