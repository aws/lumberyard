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
#include "WoodpeckerApplication.h"

#include <Woodpecker/Telemetry/TelemetryComponent.h>
#include <Woodpecker/Telemetry/TelemetryBus.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzToolsFramework/UI/LegacyFramework/Core/IPCComponent.h>

#ifndef AZ_PLATFORM_WINDOWS
int __argc;
char **__argv;
#endif

namespace Woodpecker
{
    BaseApplication::BaseApplication(int &argc, char **argv)
        : LegacyFramework::Application()
    {
        __argc = argc;
        __argv = argv;
        AZ::UserSettingsFileLocatorBus::Handler::BusConnect();
    }

    BaseApplication::~BaseApplication()
    {
        AZ::UserSettingsFileLocatorBus::Handler::BusDisconnect();
    }

    void BaseApplication::RegisterCoreComponents()
    {
        LegacyFramework::Application::RegisterCoreComponents();

        RegisterComponentDescriptor(Telemetry::TelemetryComponent::CreateDescriptor());
        RegisterComponentDescriptor(LegacyFramework::IPCComponent::CreateDescriptor());

        RegisterComponentDescriptor(AZ::UserSettingsComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::TargetManagementComponent::CreateDescriptor());
    }

    void BaseApplication::CreateSystemComponents()
    {
        LegacyFramework::Application::CreateSystemComponents();

        // AssetCatalogComponent was moved to the Application Entity to fulfil service requirements.
        EnsureComponentRemoved(AzFramework::AssetCatalogComponent::RTTI_Type());
    }

    void BaseApplication::CreateApplicationComponents()
    {
        LegacyFramework::Application::CreateApplicationComponents();
        EnsureComponentCreated(Telemetry::TelemetryComponent::RTTI_Type());
        EnsureComponentCreated(AzFramework::TargetManagementComponent::RTTI_Type());
        EnsureComponentCreated(LegacyFramework::IPCComponent::RTTI_Type());

        // Check for user settings components already added (added by the app descriptor
        AZStd::array<bool, AZ::UserSettings::CT_MAX> userSettingsAdded;
        userSettingsAdded.fill(false);
        for (const auto& component : m_applicationEntity->GetComponents())
        {
            if (const auto userSettings = azrtti_cast<AZ::UserSettingsComponent*>(component))
            {
                userSettingsAdded[userSettings->GetProviderId()] = true;
            }
        }

        // For each provider not already added, add it.
        for (AZ::u32 providerId = 0; providerId < userSettingsAdded.size(); ++providerId)
        {
            if (!userSettingsAdded[providerId])
            {
                // Don't need to add one for global, that's added by someone else
                m_applicationEntity->AddComponent(aznew AZ::UserSettingsComponent(providerId));
            }
        }
    }

    void BaseApplication::OnApplicationEntityActivated()
    {
        const int k_processIntervalInSecs = 2;
        const bool doSDKInitShutdown = true;
        EBUS_EVENT(Telemetry::TelemetryEventsBus, Initialize, "LumberyardIDE", k_processIntervalInSecs, doSDKInitShutdown);

        bool launched = LaunchDiscoveryService();

        AZ_Warning("EditorApplication", launched, "Could not launch GridHub; Only replay is available.");
        (void)launched;
    }

    void BaseApplication::ReflectSerializeDeprecated()
    {
        LegacyFramework::Application::ReflectSerializeDeprecated();

        // A "convenient" function to place all high level (no other place to put) deprecated reflections.
        // IMPORTANT: Please to time stamp each deprecation so we can remove after some time (we know all
        // old data had been converted)
        GetSerializeContext()->ClassDeprecate("EditorDataComponent", "{15E08C28-B98E-49ac-B60C-CCC6EF2E46B4}"); // 11/12/2012
    }

    bool BaseApplication::LaunchDiscoveryService()
    {
#ifdef WIN32
        WCHAR exeName[MAX_PATH] = { 0 };
        ::GetModuleFileName(::GetModuleHandle(nullptr), exeName, MAX_PATH);
        WCHAR drive[MAX_PATH] = { 0 };
        WCHAR dir[MAX_PATH] = { 0 };
        WCHAR discoveryServiceExe[MAX_PATH] = { 0 };
        WCHAR workingDir[MAX_PATH] = { 0 };

        _wsplitpath_s(exeName, drive, static_cast<size_t>(AZ_ARRAY_SIZE(drive)), dir, static_cast<size_t>(AZ_ARRAY_SIZE(dir)), nullptr, 0, nullptr, 0);
        _wmakepath_s(discoveryServiceExe, drive, dir, L"GridHub", L"exe");
        _wmakepath_s(workingDir, drive, dir, nullptr, nullptr);

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));

        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_MINIMIZE;

        PROCESS_INFORMATION pi;

        return ::CreateProcess(discoveryServiceExe, L"-fail_silently", nullptr, nullptr, FALSE, 0, nullptr, workingDir, &si, &pi) == TRUE;
#else
        return false;
#endif
    }

    AZStd::string BaseApplication::ResolveFilePath(AZ::u32 providerId)
    {
        AZStd::string userStoragePath;
        FrameworkApplicationMessages::Bus::BroadcastResult(userStoragePath, &FrameworkApplicationMessages::GetApplicationGlobalStoragePath);

        if (userStoragePath.size() == 0)
        {
            FrameworkApplicationMessages::Bus::BroadcastResult(userStoragePath, &FrameworkApplicationMessages::GetApplicationDirectory);
        }

        AZStd::string appName;
        FrameworkApplicationMessages::Bus::BroadcastResult(appName, &FrameworkApplicationMessages::GetApplicationName);

        AzFramework::StringFunc::Path::Join(userStoragePath.c_str(), appName.c_str(), userStoragePath);
        AZ::IO::SystemFile::CreateDir(userStoragePath.c_str());

        AZStd::string fileName;
        switch (providerId)
        {
        case AZ::UserSettings::CT_LOCAL:
            fileName = AZStd::string::format("%s_UserSettings.xml", appName.c_str());
            break;
        case AZ::UserSettings::CT_GLOBAL:
            fileName = "GlobalUserSettings.xml";
            break;
        }

        AzFramework::StringFunc::Path::Join(userStoragePath.c_str(), fileName.c_str(), userStoragePath);
        return userStoragePath;
    }
}