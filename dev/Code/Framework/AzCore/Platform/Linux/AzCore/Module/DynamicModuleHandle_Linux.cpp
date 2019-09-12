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

#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzCore/std/string/osstring.h>
#include <dlfcn.h>
#include <libgen.h>

namespace AZ
{
    namespace Platform
    {
        void GetModulePath(AZ::OSString& path)
        {
            char exePath[AZ_MAX_PATH_LEN] = { 0 };
            const char* modulePath = ::getenv("MODULE_PATH");
            if (!modulePath)
            {
                modulePath = ".";
                int len = ::readlink("/proc/self/exe", exePath, AZ_MAX_PATH_LEN);
                if (len != -1)
                {
                    exePath[len] = 0;
                    modulePath = ::dirname(exePath);
                }
            }
            path = modulePath;
            path += '/';
        }

        void* OpenModule(const AZ::OSString& fileName, bool& alreadyOpen)
        {
            void* handle = dlopen(fileName.c_str(), RTLD_NOLOAD);
            alreadyOpen = (handle != nullptr);
            if (!alreadyOpen)
            {
                handle = dlopen(fileName.c_str(), RTLD_NOW);
            }
            return handle;
        }
    }
}