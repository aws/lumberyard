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

#ifndef CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_OCULUS_OCULUSDEVICE_H
#define CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_OCULUS_OCULUSDEVICE_H
#pragma once


#ifndef EXCLUDE_OCULUS_SDK

#include <IHMDDevice.h>
#include "OculusConfig.h"


class OculusDevice
    : public IHMDDevice
{
public:
    // IHMDDevice interface
    virtual void AddRef() override;
    virtual void Release() override;

    virtual void InitD3D(const D3DInitParams& params) override;
    virtual void InitEyes(const D3DEyeTextures& params) override;

    virtual IHMDDevice::DeviceType GetType() const override { return OculusRift; }
    virtual bool IsUsable() const override { return m_ovrDevice != nullptr; }


    virtual void GetDeviceInfo(DeviceInfo& info) const override { info = m_devInfo; }
    virtual void GetDisplayInfo(DisplayInfo& info) const override { info = m_hmdInfo; }
    virtual void GetSensorInfo(SensorInfo& info) const override { info = m_senInfo; }

    virtual void ResetSensor() override;

    virtual Quat GetOrientation() const override;
    virtual Quat GetCurrentOrientation() const override;

    virtual void EnablePrediction(float delta, bool enablePrediction, bool enablePredictionFilter) override;
    virtual Quat GetPredictedOrientation() const override;

    virtual void TickLatencyTester() override;

    virtual FrameParams BeginFrame() override;
    virtual void EndFrame() override;
    virtual EyeParams BeginEye(Eye eye, float znear) override;
    virtual void EndEye() override;

    virtual void RT_BeginFrame() override;
    virtual void RT_EndFrame() override;
    virtual void RT_SetEyePose(uint32 eye, void* pose) override;

public:
    static OculusDevice* CreateInstance();
    static void InitCVars();
    static void RenderDeviceInfo();

    OculusDevice();
    int GetRefCount() const { return m_refCount; }

    bool HasHMDDevice() const { return m_ovrDevice != 0; }

protected:
    virtual ~OculusDevice();

    void CacheDisplayInfo();

protected:
    static int ms_sys_hmd_reset_sensor;
    const float c_ovrPixelDensity {
        1.0f
    };

protected:
    volatile int m_refCount;
    unsigned int m_plugInState;
    DeviceInfo m_devInfo;
    DisplayInfo m_hmdInfo;
    SensorInfo m_senInfo;
    ovrHmd m_ovrDevice {
        nullptr
    };
    ovrPosef m_eyePoses[2];
    ovrEyeRenderDesc m_eyeDesc[2];
    ovrD3D11Texture m_eyeTextures[2];
    mutable ovrQuatf m_lastRotation;
};

#endif // #ifndef EXCLUDE_OCULUS_SDK


#endif // CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_OCULUS_OCULUSDEVICE_H
