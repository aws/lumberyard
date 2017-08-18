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

// Description : Manages and stores the multiple head mounted devices.


#include "StdAfx.h"
#include "../System.h"
#include "IHMDDevice.h"
#include "./NULL/HMDNullDevice.h"
#include "Cinemizer/CinemizerDevice.h"
#include "Oculus/OculusResources.h"
#include "HMDManager.h"

int CHMDManager::ms_sys_CurrentHMDType = 0;

#ifndef EXCLUDE_CINEMIZER_SDK
CCinemizerDevice* CHMDManager::ms_pCinemizerDevice = nullptr;
#endif //#ifndef EXCLUDE_CINEMIZER_SDK

IHMDDevice* CSystem::GetHMDDevice() const
{
    return CHMDManager::GetDevice();
}

bool CSystem::HasHMDDevice(unsigned int type) const
{
    return CHMDManager::HasHMDDevice((IHMDDevice::DeviceType)type);
}

void CHMDManager::Init()
{
#ifndef EXCLUDE_CINEMIZER_SDK
    assert(!ms_pCinemizerDevice);
    static char s_CinemizerResourceStorage[sizeof(CCinemizerDevice)] = {0};
    if (!ms_pCinemizerDevice)
    {
        ms_pCinemizerDevice = new(s_CinemizerResourceStorage) CCinemizerDevice();
    }
#endif //EXCLUDE_CINEMIZER_SDK

#ifndef EXCLUDE_OCULUS_SDK
    OculusResources::Init();
#endif //#ifndef EXCLUDE_OCULUS_SDK
}

void CHMDManager::Shutdown()
{
#ifndef EXCLUDE_CINEMIZER_SDK

#if !defined(_RELEASE)
    if (ms_pCinemizerDevice && ms_pCinemizerDevice->GetRefCount() != 1)
    {
        __debugbreak();
    }
#endif

    assert(ms_pCinemizerDevice);
    if (ms_pCinemizerDevice)
    {
        ms_pCinemizerDevice->~CCinemizerDevice();
        ms_pCinemizerDevice = 0;
    }
#endif //EXCLUDE_CINEMIZER_SDK

#ifndef EXCLUDE_OCULUS_SDK
    OculusResources::Shutdown();
#endif //#ifndef EXCLUDE_OCULUS_SDK
}

IHMDDevice* CHMDManager::GetDevice()
{
    return GetDevice((IHMDDevice::DeviceType)ms_sys_CurrentHMDType);
}

IHMDDevice* CHMDManager::GetDevice(IHMDDevice::DeviceType type)
{
    switch (type)
    {
    case IHMDDevice::OculusRift:
    {
#ifndef EXCLUDE_OCULUS_SDK
        IHMDDevice* pDevice = OculusResources::GetAccess().GetWrappedDevice();
        if (pDevice)
        {
            return pDevice;
        }
#endif
        return HMDNullDevice::CreateInstance();
    }
    break;

    case IHMDDevice::Cinemizer:
    {
#ifndef EXCLUDE_CINEMIZER_SDK
        return ms_pCinemizerDevice;
#endif //EXCLUDE_CINEMIZER_SDK
        return HMDNullDevice::CreateInstance();
    }
    break;

    default:
    {
        return HMDNullDevice::CreateInstance();
    }
    break;
    }
    return HMDNullDevice::CreateInstance();
}

bool CHMDManager::HasHMDDevice(IHMDDevice::DeviceType type)
{
    IHMDDevice* pDevice = GetDevice(type);

    return pDevice && pDevice->IsUsable();
}

void CHMDManager::InitCVars()
{
    REGISTER_CVAR2("sys_CurrentHMDType", &ms_sys_CurrentHMDType, 0,
        VF_NULL, "Chooses which Head Mounted Device to use:\n 0 - Null Device (default)\n 1 -Oculus Rift\n 2 - Cinemizer\n");

#ifndef EXCLUDE_CINEMIZER_SDK
    CCinemizerDevice::InitCVars();
#endif //#ifndef EXCLUDE_CINEMIZER_SDK

#ifndef EXCLUDE_OCULUS_SDK
    OculusDevice::InitCVars();
#endif //#ifndef EXCLUDE_CINEMIZER_SDK
}
