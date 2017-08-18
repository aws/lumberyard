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


#ifndef CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_HMDMANAGER_H
#define CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_HMDMANAGER_H
#pragma once

#include "IHMDDevice.h"


#ifndef EXCLUDE_CINEMIZER_SDK
class CCinemizerDevice;
#endif //EXCLUDE_CINEMIZER_SDK

class CHMDManager
{
public:
    static void Init();
    static void Shutdown();

    // Get HMD device associated with sys_CurrentHMDType.
    static IHMDDevice* GetDevice();

    // Get HMD device of specified type.
    static IHMDDevice* GetDevice(IHMDDevice::DeviceType type);

    // Test if HMD device type is available.
    static bool HasHMDDevice(IHMDDevice::DeviceType type);

    static void InitCVars();

private:
    CHMDManager(){};
    ~CHMDManager(){};

private:
    static int ms_sys_CurrentHMDType;

    #ifndef EXCLUDE_CINEMIZER_SDK
    static CCinemizerDevice* ms_pCinemizerDevice;
    #endif //EXCLUDE_CINEMIZER_SDK
};

#endif // CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_HMDMANAGER_H
