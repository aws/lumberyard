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
#include "DeploymentTool_precompiled.h"
#include "NetworkUtils.h"


namespace DeployTool
{
    bool GetAllHostIPAddrs(AZStd::vector<AZStd::string>& ipAddrs)
    {
        const char* localhost = "127.0.0.1";

        char hostBuffer[NI_MAXHOST] = { 0 };
        if (gethostname(hostBuffer, NI_MAXHOST) != 0)
        {
            return false;
        }

        // getaddrinfo doesn't return localhost like getifaddrs does, so make sure it's added to the list
        ipAddrs.push_back(localhost);

        struct addrinfo hints;
        memset(&hints, 0, sizeof(struct addrinfo));

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;

        struct addrinfo* result = nullptr;
        if (getaddrinfo(hostBuffer, nullptr, &hints, &result) == 0)
        {
            for (struct addrinfo* addr = result; addr != NULL; addr = addr->ai_next)
            {
                sockaddr_in* sockAddr = reinterpret_cast<sockaddr_in*>(addr->ai_addr);

                char ipAddrBuffer[INET_ADDRSTRLEN] = { 0 };
                const char* ret = inet_ntop(AF_INET, &(sockAddr->sin_addr), ipAddrBuffer, INET_ADDRSTRLEN);
                if (ret != nullptr)
                {
                    ipAddrs.push_back(ipAddrBuffer);
                }
            }
        }

        freeaddrinfo(result);

        return true;
    }
}