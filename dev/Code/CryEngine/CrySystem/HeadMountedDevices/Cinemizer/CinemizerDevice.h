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

// Description : Encapsulates the Cinemizer Head Mounted Device.


#ifndef CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_CINEMIZER_CINEMIZERDEVICE_H
#define CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_CINEMIZER_CINEMIZERDEVICE_H
#pragma once


#ifndef EXCLUDE_CINEMIZER_SDK

#include <IHMDDevice.h>


namespace CinemizerHelpers
{
    struct TrackerFrame;
}

class CCinemizerDevice
    : public IHMDDevice
{
public:
    CCinemizerDevice();
    ~CCinemizerDevice();

    // IHMDDevice interface
    virtual void AddRef();
    virtual void Release();

    virtual DeviceType GetType() const { return Cinemizer; }
    virtual bool IsUsable() const {return m_bInitialized; }

    virtual void GetDeviceInfo(DeviceInfo& info) const { info = m_devInfo; }
    virtual void GetDisplayInfo(DisplayInfo& info) const { info = m_hmdInfo; }
    virtual void GetSensorInfo(SensorInfo& info) const { info = m_senInfo; }

    virtual void ResetSensor(){}

    virtual Quat GetOrientation() const;
    virtual Vec3 GetAcceleration() const { return Vec3(0.0f, 0.0f, 0.0f); }
    virtual Vec3 GetAngularVelocity() const { return Vec3(0.0f, 0.0f, 0.0f); }

    virtual void EnablePrediction(float delta, bool enablePrediction, bool enablePredictionFilter) {}
    virtual Quat GetPredictedOrientation() const{return ::Quat(1.0f, 0.0f, 0.0f, 0.0f); }

    virtual void TickLatencyTester(){}
public:
    static CCinemizerDevice* CreateInstance();
    static void InitCVars();

    int GetRefCount() const { return m_refCount; }

    bool HasHMDDevice() const { return m_bInitialized; }
protected:
    void CreateDevices();
    void OnMouseEnabledChange(bool bEnable);
    static void OnMouseEnabledChangeCallback(ICVar* pCVar);

    volatile int m_refCount;

    HMODULE m_hTrackerDLL;

    typedef BOOL  (* TDPFRTBoolAVoid)();
    typedef BOOL  (* TDPFRTBoolARTrackerFrame)(CinemizerHelpers::TrackerFrame&);

    typedef BOOL  (* TDPFRTBoolARByteDword)(BYTE&, DWORD);
    typedef BOOL  (* TDPFRTBoolAByte)(BYTE);

    TDPFRTBoolAVoid                     m_pfTrackerInit;
    TDPFRTBoolAVoid                     m_pfTrackerRelease;
    TDPFRTBoolARTrackerFrame    m_pfGetFrame;

    TDPFRTBoolARByteDword           m_pfGetMouseSpeed;
    TDPFRTBoolAByte                     m_pfSetMouseSpeed;

    BYTE        m_OriginalMouseSpeed;

    static int ms_sys_EnableCinemizerMouse;

    bool    m_bInitialized;
    bool    m_bMouseEnabled;

    DeviceInfo m_devInfo;
    DisplayInfo m_hmdInfo;
    SensorInfo m_senInfo;
};

#endif //EXCLUDE_CINEMIZER_SDK

#endif // CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_CINEMIZER_CINEMIZERDEVICE_H
