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

#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Network/AssetProcessorConnection.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include <Windows.h>
#include <Psapi.h>
#endif // defined(AZ_PLATFORM_WINDOWS)

namespace AzFramework
{
    namespace AssetSystem
    {
        void AllowAssetProcessorToForeground()
        {
#if defined(AZ_PLATFORM_WINDOWS)
            // Make sure that all of the asset processors can bring their window to the front
            // Hacky to put it here, but not really any better place.
            DWORD bytesReturned;

            // There's no straightforward way to get the exact number of running processes,
            // So we use 2^13 processes as a generous upper bound that shouldn't be hit.
            DWORD processIds[8 * 1024];

            if (EnumProcesses(processIds, sizeof(processIds), &bytesReturned))
            {
                const DWORD numProcesses = bytesReturned / sizeof(DWORD);
                for (DWORD processIndex = 0; processIndex < numProcesses; ++processIndex)
                {
                    DWORD processId = processIds[processIndex];
                    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION |
                        PROCESS_VM_READ,
                        FALSE, processId);

                    // Get the process name.
                    if (NULL != processHandle)
                    {
                        HMODULE moduleHandle;
                        DWORD bytesNeededForAllProcessModules;

                        // Get the first module, because that will be the executable
                        if (EnumProcessModules(processHandle, &moduleHandle, sizeof(moduleHandle), &bytesNeededForAllProcessModules))
                        {
                            TCHAR processName[4096] = TEXT("<unknown>");
                            if (GetModuleBaseName(processHandle, moduleHandle, processName, AZ_ARRAY_SIZE(processName)) > 0)
                            {
                                if (azstricmp(processName, "AssetProcessor") == 0)
                                {
                                    AllowSetForegroundWindow(processId);
                                }
                            }
                        }
                    }
                }
            }
#endif // #if defined(AZ_PLATFORM_WINDOWS)
        }

        // Do this here because including Windows.h causes problems with SetPort being a define
        void AssetSystemComponent::ShowAssetProcessor()
        {
            AllowAssetProcessorToForeground();

            ShowAssetProcessorRequest request;
            SendRequest(request);
        }

        void AssetSystemComponent::ShowInAssetProcessor(const AZStd::string& assetPath)
        {
            AllowAssetProcessorToForeground();

            ShowAssetInAssetProcessorRequest request;
            request.m_assetPath = assetPath;
            SendRequest(request);
        }
    }
}


