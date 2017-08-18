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

#ifndef CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_OCULUS_OCULUSRESOURCES_H
#define CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_OCULUS_OCULUSRESOURCES_H
#pragma once



#ifndef EXCLUDE_OCULUS_SDK

#include "OculusConfig.h"
#include "OculusDevice.h"


class OculusResources
{
public:
    static OculusResources& GetAccess();

    static void Init();
    static void Shutdown();

    OculusDevice* GetWrappedDevice() { return m_pDeviceWrapped; }

private:
    OculusResources();
    ~OculusResources();

private:
    static OculusResources* ms_pRes;

private:
    OculusDevice* m_pDeviceWrapped;
};


#endif // #ifndef EXCLUDE_OCULUS_SDK


#endif // CRYINCLUDE_CRYSYSTEM_HEADMOUNTEDDEVICES_OCULUS_OCULUSRESOURCES_H
