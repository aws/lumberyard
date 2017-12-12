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

#ifndef CRYINCLUDE_CRYCOMMON_ILIVECREATEHOST_H
#define CRYINCLUDE_CRYCOMMON_ILIVECREATEHOST_H
#pragma once

#include "ILiveCreateCommon.h"

namespace LiveCreate
{
    // The default port for LiveCreate host
    static const uint16 kDefaultHostListenPort = 30601;

    // The default port for UDP target discovery service
    static const uint16 kDefaultDiscoverySerivceListenPost = 30602;

    // The default port for TCP file transfer service
    static const uint16 kDefaultFileTransferServicePort = 30603;

    // Host information (send over the network based host discovery system)
    struct CHostInfoPacket
    {
        string platformName;
        string hostName;
        string buildExecutable;
        string buildDirectory;
        string gameFolder;
        string rootFolder;
        string currentLevel;
        uint8 bAllowsLiveCreate;
        uint8 bHasLiveCreateConnection;
        uint16 screenWidth;
        uint16 screenHeight;

        template<class T>
        void Serialize(T& stream)
        {
            stream << platformName;
            stream << hostName;
            stream << gameFolder;
            stream << buildExecutable;
            stream << buildDirectory;
            stream << rootFolder;
            stream << currentLevel;
            stream << bAllowsLiveCreate;
            stream << bHasLiveCreateConnection;
            stream << screenWidth;
            stream << screenHeight;
        }
    };

    // Host side (console) LiveCreate interface
    struct IHost
    {
    public:
        virtual ~IHost() {};

    public:
        // Initialize host at given listening port
        virtual bool Initialize(const uint16 listeningPort = kDefaultHostListenPort,
            const uint16 discoveryServiceListeningPort = kDefaultDiscoverySerivceListenPost,
            const uint16 fileTransferServiceListeningPort = kDefaultFileTransferServicePort) = 0;

        // Close the interface and disconnect all clients
        virtual void Close() = 0;

        // Enter a "loading mode" in which commands are not executed
        virtual void SuppressCommandExecution() = 0;

        // Exit a "loading mode" and restore command execution
        virtual void ResumeCommandExecution() = 0;

        // Execute pending commands
        virtual void ExecuteCommands() = 0;

        // Draw overlay information
        virtual void DrawOverlay() = 0;
    };
}

#endif // CRYINCLUDE_CRYCOMMON_ILIVECREATEHOST_H
