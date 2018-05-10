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

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define UTILS_CPP_SECTION_1 1
#define UTILS_CPP_SECTION_2 2
#define UTILS_CPP_SECTION_3 3
#endif

#ifndef AZ_UNITY_BUILD

#include <GridMate/Carrier/Utils.h>
#include <GridMate/Carrier/Driver.h>

#if AZ_TRAIT_OS_USE_WINDOWS_SOCKETS
#   include <AzCore/PlatformIncl.h>
#   include <WinSock2.h>
#   include <Ws2tcpip.h>

#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION UTILS_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(Utils_cpp, AZ_RESTRICTED_PLATFORM)
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
#   include <sys/types.h>
#   include <ifaddrs.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <netdb.h>
#elif defined(AZ_PLATFORM_ANDROID)
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace GridMate
{
#ifdef AZ_PLATFORM_WINDOWS
    const char* inet_ntop(int af, const void* src, char* dst, socklen_t size);  // Implemented in SocketDriver.cpp
#endif

    //=========================================================================
    // GetMachineAddress
    // [10/19/2012]
    //=========================================================================
    string Utils::GetMachineAddress(int familyType)
    {
        string machineName;
        (void)familyType;
    #if AZ_TRAIT_OS_USE_WINDOWS_SOCKETS
        char name[MAX_PATH];
        int result = gethostname(name, sizeof(name));
        AZ_Error("GridMate", result == 0, "Failed in gethostname with result=%d, WSAGetLastError=%d!", result, WSAGetLastError());
        if (result)
        {
            return "";
        }
        addrinfo  hints = {0};
        addrinfo* addrInfo;
        hints.ai_family = (familyType == Driver::BSD_AF_INET6) ? AF_INET6 : AF_INET;
        hints.ai_flags = AI_CANONNAME;
        result = getaddrinfo(name, nullptr, &hints, &addrInfo);
        AZ_Error("GridMate", result == 0, "Failed in getaddrinfo with %d!", WSAGetLastError());
        if (addrInfo->ai_family == AF_INET6)
        {
            inet_ntop(addrInfo->ai_family, &reinterpret_cast<sockaddr_in6*>(addrInfo->ai_addr)->sin6_addr, name, AZ_ARRAY_SIZE(name));
        }
        else if (addrInfo->ai_family == AF_INET)
        {
            inet_ntop(addrInfo->ai_family, &reinterpret_cast<sockaddr_in*>(addrInfo->ai_addr)->sin_addr, name, AZ_ARRAY_SIZE(name));
        }
        else
        {
            AZ_Assert(false, "Invalid address family");
        }

        freeaddrinfo(addrInfo);
        machineName = name;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION UTILS_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(Utils_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
        struct ifaddrs* ifAddrStruct = nullptr;
        struct ifaddrs* ifa = nullptr;
        void* tmpAddrPtr = nullptr;
        int systemFamilyType = (familyType == Driver::BSD_AF_INET6) ? AF_INET6 : AF_INET;
        getifaddrs(&ifAddrStruct);

        for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr->sa_family == systemFamilyType)
            {
                if (systemFamilyType == AF_INET)
                {
                    tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
                    char addressBuffer[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
                    machineName = addressBuffer;
                }
                else if (systemFamilyType == AF_INET6)
                {
                    tmpAddrPtr = &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
                    char addressBuffer[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
                    machineName = addressBuffer;
                }
                break;
            }
        }
        if (ifAddrStruct != nullptr)
        {
            freeifaddrs(ifAddrStruct);
        }
    #elif defined(AZ_PLATFORM_ANDROID)
        struct RTMRequest
        {
            nlmsghdr m_msghdr;
            ifaddrmsg m_msg;
        } rtmRequest;

        int sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
        int family = familyType == Driver::BSD_AF_INET ? AF_INET : AF_INET6;

        memset(&rtmRequest, 0, sizeof(rtmRequest));
        rtmRequest.m_msghdr.nlmsg_len = NLMSG_LENGTH(sizeof(ifaddrmsg));
        rtmRequest.m_msghdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
        rtmRequest.m_msghdr.nlmsg_type = RTM_GETADDR;
        rtmRequest.m_msg.ifa_family = family;

        rtattr* rtaAttr = reinterpret_cast<rtattr*>(((char*)&rtmRequest) + NLMSG_ALIGN(rtmRequest.m_msghdr.nlmsg_len));
        rtaAttr->rta_len = RTA_LENGTH(familyType == Driver::BSD_AF_INET ? 4 : 16);

        int res = send(sock, &rtmRequest, rtmRequest.m_msghdr.nlmsg_len, 0);
        if (res < 0)
        {
            close(sock);
            AZ_Warning("GridMate", false, "Failed to send rtm request");
            return "";
        }

        const unsigned int k_bufSize = 4096;
        char* buf = static_cast<char*>(azmalloc(k_bufSize, 1, GridMateAllocatorMP));
        res = recv(sock, buf, k_bufSize, 0);
        if (res > 0 && static_cast<unsigned int>(res) < k_bufSize)
        {
            for (nlmsghdr* nlmp = reinterpret_cast<nlmsghdr*>(buf); static_cast<unsigned int>(res) > sizeof(*nlmp); )
            {
                int len = nlmp->nlmsg_len;
                int reqLen = len - sizeof(*nlmp);
                if (reqLen < 0 || len > res || !NLMSG_OK(nlmp, res))
                {
                    AZ_Warning("GridMate", false, "Failed to dispatch rtnetlink request");
                    break;
                }

                ifaddrmsg* rtmp = reinterpret_cast<ifaddrmsg*>(NLMSG_DATA(nlmp));
                rtattr* rtatp = reinterpret_cast<rtattr*>(IFA_RTA(rtmp));

                char address[INET6_ADDRSTRLEN] = {0};
                bool isLoopback = false;
                string devname;
                for (int rtattrlen = IFA_PAYLOAD(nlmp); RTA_OK(rtatp, rtattrlen); rtatp = RTA_NEXT(rtatp, rtattrlen))
                {
                    if (rtatp->rta_type == IFA_ADDRESS)
                    {
                        in_addr* addr = reinterpret_cast<in_addr*>(RTA_DATA(rtatp));
                        inet_ntop(family, addr, address, sizeof(address));

                        static const in6_addr loopbackAddr6[] = {IN6ADDR_LOOPBACK_INIT};
                        isLoopback |= family == AF_INET && *reinterpret_cast<AZ::u32*>(addr) == 0x0100007F;
                        isLoopback |= family == AF_INET6 && !memcmp(addr, loopbackAddr6, sizeof(loopbackAddr6));
                    }

                    if (rtatp->rta_type == IFA_LABEL)
                    {
                        devname = reinterpret_cast<const char*>RTA_DATA(rtatp);
                    }
                }

                if (!isLoopback
                    && *address
                    && (!strcmp(devname.c_str(), "eth0") || !strcmp(devname.c_str(), "wlan0")))
                {
                    machineName = address;
                    break;
                }

                res -= NLMSG_ALIGN(len);
                nlmp = reinterpret_cast<nlmsghdr*>(((char*)nlmp + NLMSG_ALIGN(len)));
            }
        }

        azfree(buf, GridMateAllocatorMP, k_bufSize);
        close(sock);
    #else
    #   error Platform not supported
    #endif
        return machineName;
    }

    //=========================================================================
    // GetBroadcastAddress
    // [7/12/2013]
    //=========================================================================
    const char* GridMate::Utils::GetBroadcastAddress(int familyType)
    {
        if (familyType == Driver::BSD_AF_INET6)
        {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION UTILS_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(Utils_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            return "FF02::1";
#endif
        }
        else if (familyType == Driver::BSD_AF_INET)
        {
            return "255.255.255.255";
        }
        else
        {
            return "";
        }
    }
} // namespace GridMate

#endif // #ifndef AZ_UNITY_BUILD
