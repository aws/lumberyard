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
#ifndef CRYINCLUDE_CRYINPUT_MOBILEINPUT_H
#define CRYINCLUDE_CRYINPUT_MOBILEINPUT_H
#pragma once

#include "BaseInput.h"

#include <unordered_map>

#include <AzFramework/API/ApplicationAPI.h>

class CMotionSensorInputDevice;

////////////////////////////////////////////////////////////////////////////////////////////////////
class CMobileInput
    : public CBaseInput
    , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
{
public:
    CMobileInput(ISystem* pSystem);
    ~CMobileInput() override;

    bool Init() override;
    void ShutDown() override;

    bool IsMotionSensorDataAvailable(EMotionSensorFlags sensorFlags) const override;
    bool AddMotionSensorEventListener(IMotionSensorEventListener* pListener,
        EMotionSensorFlags sensorFlags = eMSF_AllAvailable) override;
    void RemoveMotionSensorEventListener(IMotionSensorEventListener* pListener,
        EMotionSensorFlags sensorFlags = eMSF_AllAvailable) override;

    void ActivateMotionSensors(EMotionSensorFlags sensorFlags = eMSF_AllAvailable) override;
    void DeactivateMotionSensors(EMotionSensorFlags sensorFlags = eMSF_AllAvailable) override;

    void PauseMotionSensorInput() override;
    void UnpauseMotionSensorInput() override;

    void OnApplicationSuspended(Event /*lastEvent*/) override;
    void OnApplicationResumed(Event /*lastEvent*/) override;

    void SetMotionSensorUpdateInterval(float seconds, bool force = false) override;
    void SetMotionSensorFilter(IMotionSensorFilter* filter) override;
    const SMotionSensorData* GetMostRecentMotionSensorData() const override;

    void PostMotionSensorEvent(const SMotionSensorEvent& event, bool bForce = false) override;

    void StartTextInput(const Vec2& topLeft = ZERO, const Vec2& bottomRight = ZERO) override = 0;
    void StopTextInput() override = 0;
    bool IsScreenKeyboardShowing() const override = 0;
    bool IsScreenKeyboardSupported() const override = 0;

protected:
    void RefreshSensors();

private:
    std::unordered_map<IMotionSensorEventListener*, int> m_sensorListeners;
    int m_manuallyActivatedSensorFlags;
    int m_sensorPausedCount;
    CMotionSensorInputDevice* m_sensorDevice;
};

#endif // CRYINCLUDE_CRYINPUT_MOBILEINPUT_H
