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
#pragma once

//! Simple stub timer that exposes a single simple interface for setting the current time.
class StubTimer
    : public ITimer
{
public:
    // Stub methods
    void SetTime(float seconds)
    {
        m_frameStartTime.SetSeconds(seconds);
    }
    //~Stub methods

    StubTimer(float frameTime)
        : m_frameTime(frameTime)
        , m_frameRate(1.0f / frameTime)
        , m_frameStartTime(0.0f)
    {
    }
    virtual ~StubTimer() {};

    // ITimer
    void ResetTimer() override {}
    void UpdateOnFrameStart() override {}
    float GetCurrTime(ETimer which = ETIMER_GAME) const override
    {
        // return the same as the frame start time
        return m_frameStartTime.GetSeconds();
    }
    const CTimeValue& GetFrameStartTime(ETimer which = ETIMER_GAME) const override
    {
        return m_frameStartTime;
    }
    CTimeValue GetAsyncTime() const override
    {
        return m_frameStartTime;
    }
    float GetAsyncCurTime() override
    {
        return m_frameStartTime.GetSeconds();
    }
    float GetFrameTime(ETimer which = ETIMER_GAME) const override
    {
        return m_frameTime;
    }
    float GetRealFrameTime() const override
    {
        return m_frameTime;
    }
    float GetTimeScale() const override
    {
        return 1.0f;
    }
    float GetTimeScale(uint32 channel) const override
    {
        return 1.0f;
    }
    void ClearTimeScales() override {}
    void SetTimeScale(float s, uint32 channel = 0) override {}
    void EnableTimer(bool bEnable) override {}
    bool IsTimerEnabled() const override
    {
        return true;
    }
    float GetFrameRate() override
    {
        return m_frameRate;
    }
    float GetProfileFrameBlending(float* pfBlendTime = 0, int* piBlendMode = 0) override
    {
        return 0.0f;
    }
    void Serialize(TSerialize ser) override {}
    bool PauseTimer(ETimer which, bool bPause) override { return false; }
    bool IsTimerPaused(ETimer which) override { return false; }
    bool SetTimer(ETimer which, float timeInSeconds) override { return false; }
    void SecondsToDateUTC(time_t time, struct tm& outDateUTC) override {}
    time_t DateToSecondsUTC(struct tm& timePtr) override
    {
        return 0;
    }
    float TicksToSeconds(int64 ticks) override
    {
        return 0.0f;
    }
    int64 GetTicksPerSecond() override
    {
        return 0;
    }
    ITimer* CreateNewTimer() override
    {
        return nullptr;
    }
    void EnableFixedTimeMode(bool enable, float timeStep) override {}
    // ~ITimer

private:
    CTimeValue m_frameStartTime;
    float m_frameTime;
    float m_frameRate;
};