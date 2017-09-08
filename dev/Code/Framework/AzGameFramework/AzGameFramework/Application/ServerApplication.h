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

#pragma once

#include <AzCore/base.h>
#include "GameApplication.h"

namespace AzGameFramework
{
    class ServerApplication
        : public GameApplication
    {
    public:

        AZ_CLASS_ALLOCATOR(ServerApplication, AZ::SystemAllocator, 0);

        static void GetGameDescriptorPath(char(&outConfigFilename)[AZ_MAX_PATH_LEN], const char* gameName)
        {
            azstrcpy(outConfigFilename, AZ_MAX_PATH_LEN, gameName);
            azstrcat(outConfigFilename, AZ_MAX_PATH_LEN, "/config/server.xml");
            AZStd::to_lower(outConfigFilename, outConfigFilename + strlen(outConfigFilename));
        }
    };
} // namespace AzGameFramework
