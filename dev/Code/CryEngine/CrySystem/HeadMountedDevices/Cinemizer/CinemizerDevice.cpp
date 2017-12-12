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


#include "StdAfx.h"

#ifndef EXCLUDE_CINEMIZER_SDK

#include "../../../../SDKs/Cinemizer/TrackerSDK/include/TrackerCinemizer.h"

#include "CinemizerDevice.h"

namespace
{
    static CCinemizerDevice* pThis = nullptr;
};

int CCinemizerDevice::ms_sys_EnableCinemizerMouse = 0;

namespace CinemizerHelpers
{
    // This is needed because CryEngine stores quaternions as x, y, z, w, while Cinemizer stores quaternions as w, x, y, z.
    struct InrealQuat
    {
        float w;
        Vec3 v;
    };

    struct TrackerFrame
    {
        Vec3 Acc, Gyr, Mag;
        CinemizerHelpers::InrealQuat Rot;
        long long FrameNumber;
    };

    static ::Quat GetRotation(::Quat RotTracker, float fRollAdjustValue = 0, bool UseRollAdjust = true)
    {
        static const ::Quat rotTrackerToCinemizer(+0.38975754f, -0.66388851f, -0.46485052f, -0.43723896f);

        if (UseRollAdjust)
        {
            ::Quat YRot;

            YRot.SetRotationY(fRollAdjustValue);

            return RotTracker * rotTrackerToCinemizer * YRot;
        }
        return RotTracker * rotTrackerToCinemizer;
    }
};

CCinemizerDevice::CCinemizerDevice()
    : m_refCount(1)
    , m_devInfo()
    , m_hmdInfo()
    , m_senInfo()
    , m_hTrackerDLL(NULL)
    , m_pfTrackerInit(nullptr)
    , m_pfTrackerRelease(nullptr)
    , m_pfGetFrame(nullptr)
    , m_pfGetMouseSpeed(nullptr)
    , m_pfSetMouseSpeed(nullptr)
    , m_OriginalMouseSpeed(0)
    , m_bInitialized(false)
    , m_bMouseEnabled(false)
{
    COMPILE_TIME_ASSERT(sizeof(Vec3) == sizeof(Inreal::Vec3) /*&&"Size mismatch between Vec3 and Cinemizer's Vec3!"*/);
    COMPILE_TIME_ASSERT(sizeof(CinemizerHelpers::TrackerFrame) == sizeof(Inreal::TrackerFrame) /*,"Size mismatch between InrealWrapper::Frame and Cinemizer's Inreal::Frame!"*/);

    memset(&m_devInfo, 0, sizeof(m_devInfo));
    memset(&m_hmdInfo, 0, sizeof(m_hmdInfo));
    memset(&m_senInfo, 0, sizeof(m_senInfo));

    CreateDevices();

    pThis = this;
}

CCinemizerDevice::~CCinemizerDevice()
{
    pThis = nullptr;

    if (m_hTrackerDLL)
    {
        if (m_bInitialized && m_pfTrackerRelease)
        {
            if (!m_pfSetMouseSpeed(m_OriginalMouseSpeed))
            {
                gEnv->pLog->LogWarning("Failed to reset Cinemizer's mouse speed.");
            }
            m_pfTrackerRelease();
        }
        FreeLibrary(m_hTrackerDLL);
    }
}

void CCinemizerDevice::AddRef()
{
    CryInterlockedIncrement(&m_refCount);
}

void CCinemizerDevice::Release()
{
    long refCount = CryInterlockedDecrement(&m_refCount);
#if !defined(_RELEASE)
    IF (refCount < 0, 0)
    {
        __debugbreak();
    }
#endif
    IF (refCount == 0, 0)
    {
        delete this;
    }
}

Quat CCinemizerDevice::GetOrientation() const
{
    if (m_bInitialized)
    {
        CinemizerHelpers::TrackerFrame trackerFrame;
        m_pfGetFrame(trackerFrame);

        // This is needed because CryEngine stores quaternions as x, y, z, w, while Cinemizer stores quaternions as w, x, y, z.
        ::Quat qTrackframeRotation;

        qTrackframeRotation.w = trackerFrame.Rot.w;
        qTrackframeRotation.v.x = trackerFrame.Rot.v.x;
        qTrackframeRotation.v.y = trackerFrame.Rot.v.y;
        qTrackframeRotation.v.z = trackerFrame.Rot.v.z;

        //There might be a better way to switch the axis.
        // For now this is working, though.
        Ang3 sAngles(CinemizerHelpers::GetRotation(qTrackframeRotation));
        std::swap(sAngles.z, sAngles.y);

        return ::Quat(sAngles);
    }
    else
    {
        gEnv->pLog->LogError("Trying to use an uninitialized Cinemizer device.");
        return ::Quat(1.0f, 0.0f, 0.0f, 0.0f);
    }
}

CCinemizerDevice* CCinemizerDevice::CreateInstance()
{
    return new CCinemizerDevice();
}

void CCinemizerDevice::InitCVars()
{
    REGISTER_CVAR2_CB("sys_EnableCinemizerMouse", &ms_sys_EnableCinemizerMouse, 0,
        VF_NULL, "Enables or disables Cinemizer mouse:\n 0 - Disables Cinemizer mouse Input (default)\n 1 -Enables Cinemizer mouse Input\n", OnMouseEnabledChangeCallback);
}

/*bool HasHMDDevice() const { }*/

void CCinemizerDevice::CreateDevices()
{
    assert(!m_hTrackerDLL);
    m_hTrackerDLL = LoadLibraryA("Tracker.dll");

    if (m_hTrackerDLL)
    {
        void* pProcAddress(nullptr);

        pProcAddress = GetProcAddress(m_hTrackerDLL, "Tracker_Init");
        m_pfTrackerInit = (TDPFRTBoolAVoid)pProcAddress;

        if (!pProcAddress)
        {
            gEnv->pLog->LogError("Failed to retrieve Cinemizer's Tracker_Init function from tracker dll.");
            return;
        }

        pProcAddress = GetProcAddress(m_hTrackerDLL, "Tracker_Release");
        m_pfTrackerRelease = (TDPFRTBoolAVoid)pProcAddress;

        if (!pProcAddress)
        {
            gEnv->pLog->LogError("Failed to retrieve Cinemizer's Tracker_Release function from tracker dll.");
            return;
        }

        pProcAddress = GetProcAddress(m_hTrackerDLL, "Tracker_GetFrame");
        m_pfGetFrame = (TDPFRTBoolARTrackerFrame)pProcAddress;

        if (!pProcAddress)
        {
            gEnv->pLog->LogError("Failed to retrieve Cinemizer's Tracker_GetFrame function from tracker dll.");
            return;
        }

        pProcAddress = GetProcAddress(m_hTrackerDLL, "Tracker_GetMouseSpeed");
        m_pfGetMouseSpeed = (TDPFRTBoolARByteDword)pProcAddress;

        if (!pProcAddress)
        {
            gEnv->pLog->LogError("Failed to retrieve Cinemizer's Tracker_GetMouseSpeed function from tracker dll.");
            return;
        }

        pProcAddress = GetProcAddress(m_hTrackerDLL, "Tracker_SetMouseSpeed");
        m_pfSetMouseSpeed = (TDPFRTBoolAByte)pProcAddress;

        if (!pProcAddress)
        {
            gEnv->pLog->LogError("Failed to retrieve Cinemizer's Tracker_SetMouseSpeed function from tracker dll.");
            return;
        }

        m_bInitialized = (m_pfTrackerInit() != FALSE);
        if (!m_bInitialized)
        {
            gEnv->pLog->LogError("Failed to initialize tracker Cinemizer device.");
            return;
        }

        // The second parameter is the wait time. By default it's 100 in the DLL source.
        if (!m_pfGetMouseSpeed(m_OriginalMouseSpeed, 100))
        {
            //gEnv->pLog->LogWarning("Failed to retrieve cinemizer's mouse speed.");
        }

        m_bMouseEnabled = ms_sys_EnableCinemizerMouse != 0;
        if (!m_bMouseEnabled)
        {
            if (!m_pfSetMouseSpeed(0))
            {
                //gEnv->pLog->LogWarning("Failed to set cinemizer's mouse speed (to 0).");
            }
        }
    }
    else
    {
        gEnv->pLog->LogError("Failed to load Cinemizer's Tracker dll.");
    }
}

void CCinemizerDevice::OnMouseEnabledChange(bool bEnable)
{
    if (bEnable)
    {
        if (m_bMouseEnabled)
        {
            return;
        }
        m_bMouseEnabled = true;

        if (!m_pfSetMouseSpeed(m_OriginalMouseSpeed))
        {
            gEnv->pLog->LogWarning("Failed to reset Cinemizer's mouse speed.");
        }
    }
    else
    {
        if (!m_bMouseEnabled)
        {
            return;
        }
        m_bMouseEnabled = false;

        if (!m_pfGetMouseSpeed(m_OriginalMouseSpeed, 100))
        {
            gEnv->pLog->LogWarning("Failed to retrieve cinemizer's mouse speed.");
        }

        if (!m_pfSetMouseSpeed(0))
        {
            gEnv->pLog->LogWarning("Failed to set cinemizer's mouse speed (to 0).");
        }
    }
}

void CCinemizerDevice::OnMouseEnabledChangeCallback(ICVar* pCVar)
{
    assert(pThis);
    if (!pThis)
    {
        return;
    }

    assert(pCVar);
    if (!pCVar)
    {
        return;
    }

    pThis->OnMouseEnabledChange(pCVar->GetIVal() != 0);
}

#endif //#ifndef EXCLUDE_CINEMIZER_SDK
