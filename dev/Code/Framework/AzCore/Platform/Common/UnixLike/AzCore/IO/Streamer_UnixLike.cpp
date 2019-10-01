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

#include <AzCore/IO/Device.h>

namespace AZ
{
    namespace IO
    {
        namespace Platform
        {
            bool GetDeviceSpecsFromPath(DeviceSpecifications& specs, const char* fullpath)
            {
                #pragma message("AZ::IO::Platform::GetDeviceSpecsFromPath (UnixLike): Device and path info, not fully implemented - defaults Device::Fixed")

                specs.m_deviceType = DeviceSpecifications::DeviceType::Fixed;
                specs.m_deviceName = "default";

                return true;
            }
        }
    }
}
