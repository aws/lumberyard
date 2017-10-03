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
#include <AzCore/Android/JNI/JNI.h>
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/std/string/string.h>

#include "MobileDetectSpec.h"

namespace MobileSysInspect
{
    void LoadDeviceSpecMapping()
    {
        Internal::LoadDeviceSpecMapping_impl("@assets@/config/gpu/android_models.xml");
    }

    // Returns true if device is found in map
    bool GetAutoDetectedSpecName(AZStd::string &buffer)
    {
        static constexpr const char* s_javaFieldName = "MODEL";
        AZ::Android::JNI::Object obj("android/os/Build");
        obj.RegisterStaticField(s_javaFieldName, "Ljava/lang/String;");
        AZStd::string name = obj.GetStaticStringField(s_javaFieldName);
        buffer = device_spec_map[AZStd::hash<AZStd::string>()(name)];
        AZ_Printf("Mobile", "Model name for this device is %s", name.c_str());
        return !buffer.empty();
    }

    const float GetDeviceRamInGB()
    {
        static constexpr const char* s_javaFuntionNameGetDeviceRamInGB = "GetDeviceRamInGB";
        AZ::Android::JNI::Object obj("com/amazon/lumberyard/AndroidDeviceManager");
        obj.RegisterStaticMethod(s_javaFuntionNameGetDeviceRamInGB, "()F");
        return obj.InvokeStaticFloatMethod(s_javaFuntionNameGetDeviceRamInGB);
    }
}
