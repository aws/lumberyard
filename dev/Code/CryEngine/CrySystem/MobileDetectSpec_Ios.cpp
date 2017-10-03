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

#include "StdAfx.h"
#include <AzCore/std/string/string.h>

#include "MobileDetectSpec.h"
#include "SystemUtilsApple.h"

namespace MobileSysInspect
{
    void LoadDeviceSpecMapping()
    {
        Internal::LoadDeviceSpecMapping_impl("@assets@/config/gpu/ios_models.xml");
    }

    // Returns true if device is found in map
    bool GetAutoDetectedSpecName(AZStd::string &buffer)
    {
        AZStd::string name = SystemUtilsApple::GetMachineName();
        buffer = device_spec_map[AZStd::hash<AZStd::string>()(name)];
        return !buffer.empty();
    }

    const float GetDeviceRamInGB()
    {
        // not supported on this platform
        return 0.0f;
    }
}
