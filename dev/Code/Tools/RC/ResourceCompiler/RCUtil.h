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

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include "ConvertContext.h"

namespace ResourceCompilerUtil
{
    const int UseDefaultPort = 0;

    //! Attempts to connect to the asset processor using the specified port.  Returns false and logs an error on failure
    bool ConnectToAssetProcessor(int port, const char* identifier, const char* projectName)
    {
        using namespace AzFramework;
        using namespace AzFramework::AssetSystem;

        if (port > UseDefaultPort)
        {
            RCLog("Setting AP connection port to %d", port);
            AssetSystemRequestBus::Broadcast(&AssetSystemRequestBus::Events::SetAssetProcessorPort, port);
        }

        AssetSystemRequestBus::Broadcast(&AssetSystemRequestBus::Events::SetProjectName, projectName);

        AssetSystemRequestBus::Broadcast(&AssetSystemRequestBus::Events::Connect, identifier);

        RequestPing requestPing;
        ResponsePing responsePing;

        if (!SendRequest(requestPing, responsePing))
        {
            RCLogError("Failed to connect to AssetProcessor");
            return false;
        }

        return true;
    }
}
