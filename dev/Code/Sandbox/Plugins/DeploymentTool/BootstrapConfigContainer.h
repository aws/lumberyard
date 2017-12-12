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

#include "ConfigFileContainer.h"

// interface to read/modify bootstrap.cfg
class BootstrapConfigContainer : private ConfigFileContainer
{
public:
    BootstrapConfigContainer();
    BootstrapConfigContainer(const BootstrapConfigContainer& rhs) = delete;
    BootstrapConfigContainer& operator=(const BootstrapConfigContainer& rhs) = delete;
    BootstrapConfigContainer& operator=(BootstrapConfigContainer&& rhs) = delete;
    ~BootstrapConfigContainer();

    using ConfigFileContainer::ReadContents;
    using ConfigFileContainer::WriteContents;

    StringOutcome ConfigureForVFSUsage(const AZStd::string& remoteIpAddress, const AZStd::string& remoteIpPort);
    StringOutcome Reset();

    AZStd::string GetGameFolder() const;
    bool GetRemoteFileSystem() const;
    AZStd::string GetRemoteIP() const;
    AZStd::string GetRemotePort() const;
    bool GetConnectToRemote() const;
    bool GetWaitForConnect() const;
    bool GetAndroidConnectToRemote() const;

    AZStd::string GetRemoteIPIncludingComments() const;
    AZStd::string GetRemotePortIncludingComments() const;

    void SetGameFolder(const AZStd::string& gameFolder);
    void SetRemoteFileSystem(bool remoteFileSystem);
    void SetRemoteIP(const AZStd::string& newIp);
    void SetRemotePort(const AZStd::string& newPort);
    void SetConnectToRemote(bool connectToRemote);
    void SetWaitForConnect(bool waitForConnect);
    void SetAndroidConnectToRemote(bool androidConnectToRemote);

private:
    static const char* s_localFilePath;
    static const char* s_gameFolderKey;
    static const char* s_remoteFileSystemKey;
    static const char* s_remoteIPKey;
    static const char* s_remotePortKey;
    static const char* s_connectToRemoteKey;
    static const char* s_waitForConnectKey;
    static const char* s_androidConnectToRemoteKey;
};

