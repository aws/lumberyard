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

#ifndef CRYINCLUDE_CRYCOMMON_ILIVECREATEMANAGER_H
#define CRYINCLUDE_CRYCOMMON_ILIVECREATEMANAGER_H
#pragma once


#include "ILiveCreateCommon.h"

struct IRemoteCommand;

namespace LiveCreate
{
    struct IPlatformHandler;

    //-----------------------------------------------------------------------------

    /// PC side Live Create host info
    struct IHostInfo
    {
    protected:
        virtual ~IHostInfo() {};

    public:
        // Host target name
        virtual const char* GetTargetName() const = 0;

        // Get resolved network address (NULL for non existing hosts)
        virtual const char* GetAddress() const = 0;

        // Get platform name
        virtual const char* GetPlatformName() const = 0;

        // Are we connected to this host ?
        virtual bool IsConnected() const = 0;

        // Is the connection to this host confirmed ? (remote side has responded to our messages)
        virtual bool IsReady() const = 0;

        // Get the platform related interface (never NULL)
        virtual IPlatformHandler* GetPlatformInterface() = 0;

        // Connect to remote manager
        virtual bool Connect() = 0;

        // Disconnect from remote manager
        virtual void Disconnect() = 0;

        // Add reference to object (refcounted interface)
        virtual void AddRef() = 0;

        // Release reference to object (refcounted interface)
        virtual void Release() = 0;
    };

    //-----------------------------------------------------------------------------

    /// PC side LiveCreate manager
    struct IManager
    {
    public:
        virtual ~IManager() {};

    public:
        // This is a hint that the LiveCreate is the null implementation that does nothing
        // useful when we dont want to ship CryLiveCreate.dll and the system falls back to LiveCreate::CNullManager
        virtual bool IsNullImplementation() const = 0;

        // Can we send any data to LiveCreate hosts?
        virtual bool CanSend() const = 0;

        // Is the manager enabled ?s
        virtual bool IsEnabled() const =  0;

        // Enable/Disable manager
        virtual void SetEnabled(bool bIsEnabled) = 0;

        // Is the logging of debug messages enabled
        virtual bool IsLogEnabled() const = 0;

        // Enable/Disable logging of debug messages
        virtual void SetLogEnabled(bool bIsEnabled) = 0;

        // Register a new listener
        virtual bool RegisterListener(IManagerListenerEx* pListener) = 0;

        // Unregister a listener
        virtual bool UnregisterListener(IManagerListenerEx* pListener) = 0;

        // Get number of host connections
        virtual uint GetNumHosts() const = 0;

        // Get host interface
        virtual IHostInfo* GetHost(const uint index) const = 0;

        // Get number of registered platforms
        virtual uint GetNumPlatforms() const = 0;

        // Get platform factory at given index
        virtual IPlatformHandlerFactory* GetPlatformFactory(const uint index) const = 0;

        // Create a host interface from valid platform interface and a valid address
        virtual IHostInfo* CreateHost(IPlatformHandler* pPlatform, const char* szValidIpAddress) = 0;

        // Remove host from manager
        virtual bool RemoveHost(IHostInfo* pHost) = 0;

        // Update manager
        virtual void Update() = 0;

        // Send remote command to connected hosts
        virtual bool SendCommand(const IRemoteCommand& command) = 0;

        // Log LiveCreate message
        virtual void LogMessage(ELogMessageType aType, const char* pMessage) = 0;
    };
}

#endif // CRYINCLUDE_CRYCOMMON_ILIVECREATEMANAGER_H
