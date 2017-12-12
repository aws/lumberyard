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
#ifndef AZCORE_SOCKET_AZSOCKET_FWD_H
#define AZCORE_SOCKET_AZSOCKET_FWD_H

#include <AzCore/base.h>

#if !defined(AZ_PLATFORM_ANDROID) && !defined(AZ_PLATFORM_APPLE) && !defined(AZ_PLATFORM_LINUX) \
 && !defined(AZ_PLATFORM_PS4) && !defined(AZ_PLATFORM_WINDOWS) && !defined(AZ_PLATFORM_XBONE) // ACCEPTED_USE
#   error Platform not supported!
#endif

#define SOCKET_ERROR (-1)
#define AZ_SOCKET_INVALID (-1)

struct sockaddr;
struct sockaddr_in;

// Type wrappers for sockets
typedef sockaddr    AZSOCKADDR;
typedef sockaddr_in AZSOCKADDR_IN;
typedef AZ::s32     AZSOCKET;

namespace AZ
{
    namespace AzSock
    {
        enum class AzSockError : AZ::s32;
        enum class AZSocketOption : AZ::s32;
        class AzSocketAddress;
    }; // namespace AzSock
}; // namespace AZ
#endif // AZCORE_SOCKET_AZSOCKET_FWD_H
