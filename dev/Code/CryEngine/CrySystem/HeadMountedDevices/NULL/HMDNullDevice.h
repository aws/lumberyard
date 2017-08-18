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

// Description : Common interface for head mounted devices.


#ifndef CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_NULL_HMDNULLDEVICE_H
#define CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_NULL_HMDNULLDEVICE_H
#pragma once


#include <IHMDDevice.h>


class HMDNullDevice
    : public IHMDDevice
{
public:
    // IHMDDevice interface
    virtual void AddRef() {}
    virtual void Release() {}

    virtual void InitD3D(const D3DInitParams& params) override {}
    virtual void InitEyes(const D3DEyeTextures& params) override {}

    virtual DeviceType GetType() const { return Null; }
    virtual bool IsUsable() const { return false; }

    virtual void GetDeviceInfo(DeviceInfo& info) const { memset(&info, 0, sizeof(info)); }
    virtual void GetDisplayInfo(DisplayInfo& info) const { memset(&info, 0, sizeof(info)); }
    virtual void GetSensorInfo(SensorInfo& info) const { memset(&info, 0, sizeof(info)); }

    virtual void ResetSensor() {}

    virtual Quat GetOrientation() const { return Quat(1, 0, 0, 0); }
    virtual Quat GetCurrentOrientation() const { return Quat(1, 0, 0, 0); }

    virtual void EnablePrediction(float delta, bool enablePrediction, bool enablePredictionFilter) {}
    virtual Quat GetPredictedOrientation() const { return Quat(1, 0, 0, 0); }

    virtual void TickLatencyTester() {}

    virtual FrameParams BeginFrame() override { return FrameParams(); }
    virtual void EndFrame() override {}
    virtual EyeParams BeginEye(Eye eye, float znear) override { return EyeParams(); }
    virtual void EndEye() override {}

    virtual void RT_BeginFrame() override {}
    virtual void RT_EndFrame() override {}
    virtual void RT_SetEyePose(uint32 eye, void* pose) override {}

public:
    static HMDNullDevice* CreateInstance()
    {
        static HMDNullDevice s_inst;
        return &s_inst;
    }

private:
    HMDNullDevice() {}
    virtual ~HMDNullDevice() {}
};


#endif // CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_NULL_HMDNULLDEVICE_H
