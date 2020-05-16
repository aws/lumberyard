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

#include "Utils_UnixLike.h"

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/Component/ComponentApplicationBus.h>


namespace AssetProcessor
{
    bool GetExternalProjectEnv(AZStd::vector<AZStd::string>& environment)
    {
        bool isProjectExternal = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(isProjectExternal, &AzFramework::ApplicationRequests::IsEngineExternal);

        if (isProjectExternal)
        {
            AZStd::string appLibPath;
            {
                AZStd::string appRoot;
                AzFramework::ApplicationRequests::Bus::BroadcastResult(appRoot, &AzFramework::ApplicationRequests::GetAppRoot);

                AZStd::string binFolder;
                AZ::ComponentApplicationBus::BroadcastResult(binFolder, &AZ::ComponentApplicationBus::Events::GetBinFolder);

                AzFramework::StringFunc::Path::Join(appRoot.c_str(), binFolder.c_str(), appLibPath);
            }

            for (auto envVar : {"LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "DYLD_FALLBACK_LIBRARY_PATH"})
            {
                environment.push_back(AZStd::string::format("%s=%s:%s", envVar, appLibPath.c_str(), envVar));
            }
        }

        return !environment.empty();
    }
} // namespace AssetProcessor