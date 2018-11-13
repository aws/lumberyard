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

#include <ifaddrs.h>
#include <netdb.h>


namespace DeployTool
{
    bool GetAllHostIPAddrs(AZStd::vector<AZStd::string>& ipAddrs)
    {
        // Ideally we should be doing what windows is doing but there seems to be a substantial
        // delay on the refresh of the IP addresses returned from getaddrinfo after a network
        // configuration change on macOS, getifaddrs doesn’t appear to have this side effect.
        struct ifaddrs* result = nullptr;
        if (getifaddrs(&result) == -1)
        {
            return false;
        }

        for (struct ifaddrs* addr = result; addr != NULL; addr = addr->ifa_next)
        {
            if (addr->ifa_addr != nullptr)
            {
                if (addr->ifa_addr->sa_family == AF_INET)
                {
                    char ipAddrBuffer[INET_ADDRSTRLEN] = { 0 };
                    int ret = getnameinfo(addr->ifa_addr, sizeof(struct sockaddr_in), ipAddrBuffer, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
                    if (ret == 0)
                    {
                        ipAddrs.push_back(ipAddrBuffer);
                    }
                }
            }
        }

        freeifaddrs(result);

        return true;
    }
}