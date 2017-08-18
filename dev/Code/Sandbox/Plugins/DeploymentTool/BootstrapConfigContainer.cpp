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

#include "stdafx.h"
#include "BootstrapConfigContainer.h"

const char* BootstrapConfigContainer::s_localFilePath = "bootstrap.cfg";
const char* BootstrapConfigContainer::s_gameFolderKey = "sys_game_folder";
const char* BootstrapConfigContainer::s_remoteFileSystemKey = "remote_filesystem";
const char* BootstrapConfigContainer::s_remoteIPKey = "remote_ip";
const char* BootstrapConfigContainer::s_remotePortKey = "remote_port";
const char* BootstrapConfigContainer::s_connectToRemoteKey = "connect_to_remote";
const char* BootstrapConfigContainer::s_waitForConnectKey = "wait_for_connect";
const char* BootstrapConfigContainer::s_androidConnectToRemoteKey = "android_connect_to_remote";

BootstrapConfigContainer::BootstrapConfigContainer()
    : ConfigFileContainer(s_localFilePath)
{
}

BootstrapConfigContainer::~BootstrapConfigContainer()
{
}

StringOutcome BootstrapConfigContainer::ConfigureForVFSUsage(const AZStd::string& remoteIpAddress, const AZStd::string& remoteIpPort)
{
    SetRemoteFileSystem(true);
    SetRemoteIP(remoteIpAddress);
    SetAndroidConnectToRemote(true);
    SetConnectToRemote(true);
    SetWaitForConnect(false);
    SetRemotePort(remoteIpPort);

    return WriteContents();
}

StringOutcome BootstrapConfigContainer::Reset()
{
    SetRemoteFileSystem(false);
    SetAndroidConnectToRemote(false);
    SetConnectToRemote(false);

    return WriteContents();
}

AZStd::string BootstrapConfigContainer::GetRemoteIP() const
{
    return GetString(s_remoteIPKey);
}

AZStd::string BootstrapConfigContainer::GetGameFolder() const
{
    return GetString(s_gameFolderKey);
}

bool BootstrapConfigContainer::GetRemoteFileSystem() const
{
    return GetBool(s_remoteFileSystemKey);
}

AZStd::string BootstrapConfigContainer::GetRemotePort() const
{
    return GetString(s_remotePortKey);
}

bool BootstrapConfigContainer::GetConnectToRemote() const
{
    return GetBool(s_connectToRemoteKey);
}

bool BootstrapConfigContainer::GetWaitForConnect() const
{
    return GetBool(s_waitForConnectKey);
}

bool BootstrapConfigContainer::GetAndroidConnectToRemote() const
{
    return GetBool(s_androidConnectToRemoteKey);
}

AZStd::string BootstrapConfigContainer::GetRemoteIPIncludingComments() const
{
    return GetStringIncludingComments(s_remoteIPKey);
}

AZStd::string BootstrapConfigContainer::GetRemotePortIncludingComments() const
{
    return GetStringIncludingComments(s_remotePortKey);
}

void BootstrapConfigContainer::SetRemoteIP(const AZStd::string& newIp)
{
    SetString(s_remoteIPKey, newIp);
}

void BootstrapConfigContainer::SetGameFolder(const AZStd::string& gameFolder)
{
    SetString(s_gameFolderKey, gameFolder);
}

void BootstrapConfigContainer::SetRemoteFileSystem(bool remoteFileSystem)
{
    SetBool(s_remoteFileSystemKey, remoteFileSystem);
}

void BootstrapConfigContainer::SetRemotePort(const AZStd::string& newPort)
{
    SetString(s_remotePortKey, newPort);
}

void BootstrapConfigContainer::SetConnectToRemote(bool connectToRemote)
{
    SetBool(s_connectToRemoteKey, connectToRemote);
}

void BootstrapConfigContainer::SetWaitForConnect(bool waitForConnect)
{
    SetBool(s_waitForConnectKey, waitForConnect);
}

void BootstrapConfigContainer::SetAndroidConnectToRemote(bool androidConnectToRemote)
{
    SetBool(s_androidConnectToRemoteKey, androidConnectToRemote);
}