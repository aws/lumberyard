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

#include <AzCore/Platform/Platform.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/time.h>
#include <AzCore/Math/Sha1.h>

#ifdef AZ_PLATFORM_WINDOWS
#include <tchar.h>
#endif

#if defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_LINUX)
#include <sys/types.h>
#include <unistd.h>
#endif

namespace AZ
{
    namespace Platform
    {
        static MachineId s_machineId;

        ProcessId GetCurrentProcessId()
        {
            ProcessId result = 0;
#if defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_LINUX) 
            result = ::getpid();
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(Platform_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif AZ_TRAIT_USE_WINDOWS_PROCESSID
            result = ::GetCurrentProcessId();
#else
#   error Platform not supported!
#endif
            return result;
        }

        MachineId GetLocalMachineId()
        {
            if (s_machineId == 0)
            {
                s_machineId = 1;
#if defined(AZ_PLATFORM_WINDOWS)
                HKEY key;
                long ret;
                // Query the machine guid generated at install time by windows, which is generated from
                // hardware signatures. This guid is not enough, because images (AWS) may duplicate this,
                // so include the hostname/username as well
                TCHAR machineInfo[MAX_COMPUTERNAME_LENGTH + 1024] = { 0 };
                ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_QUERY_VALUE, &key);
                if (ret == ERROR_SUCCESS)
                {
                    DWORD dataType = REG_SZ;
                    DWORD dataSize = sizeof(machineInfo);
                    ret = RegQueryValueEx(key, "MachineGuid", 0, &dataType, (LPBYTE)machineInfo, &dataSize);
                    RegCloseKey(key);
                }
                else
                {
                    AZ_Error("System", false, "Failed to open HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography\\MachineGuid!")
                }

                TCHAR* hostname = machineInfo + _tcslen(machineInfo);
                // salt the guid time with ComputerName/UserName
                DWORD  bufCharCount = DWORD(sizeof(machineInfo) - (hostname - machineInfo));
                if (!::GetComputerName(hostname, &bufCharCount))
                {
                    AZ_Error("System", false, "GetComputerName filed with code %d", GetLastError());
                }

                TCHAR* username = hostname + _tcslen(hostname);
                bufCharCount = DWORD(sizeof(machineInfo) - (username - machineInfo));
                if( !GetUserName( username, &bufCharCount ) ) 
                {
                    AZ_Error("System",false,"GetUserName filed with code %d",GetLastError());
                }

                Sha1 hash;
                AZ::u32 digest[5] = { 0 };
                hash.ProcessBytes(machineInfo, _tcslen(machineInfo) * sizeof(TCHAR));
                hash.GetDigest(digest);
                s_machineId = digest[0];
#elif defined(AZ_PLATFORM_APPLE_OSX)
                // Fetch a machine guid from the OS. Apple says this is guaranteed to be unique enough for
                // licensing purposes (so that should be good enough for us here), but that
                // the hardware ids which generate this can be modified (it most likely includes MAC addr)
                uuid_t hostId = { 0 };
                timespec wait;
                wait.tv_sec = 0;
                wait.tv_nsec = 5000;
                int result = ::gethostuuid(hostId, &wait);
                AZ_Error("System", result == 0, "gethostuuid() failed with code %d", result);  
                Sha1 hash;
                AZ::u32 digest[5] = { 0 };
                hash.ProcessBytes(&hostId, sizeof(hostId));
                hash.GetDigest(digest);
                s_machineId = digest[0];
#else
                // In specialized server situations, SetLocalMachineId() should be used, with whatever criteria works best in that environment
                // A proper implementation for each supported system will be needed instead of this temporary measure to avoid collision in the small scale.
                // On a larger scale, the odds of two people getting in here at the same millisecond will go up drastically, and we'll have the same issue again,
                // though far less reproducible, for duplicated EntityId's across a network.
                s_machineId = static_cast<AZ::u32>(AZStd::GetTimeUTCMilliSecond() & 0xffffffff);
#endif
                if (s_machineId == 0)
                {
                    s_machineId = 1;
                    AZ_Warning("System", false, "0 machine ID is reserved!");
                }
            }
            return s_machineId;
        }

        void SetLocalMachineId(AZ::u32 machineId)
        {
            AZ_Assert(machineId != 0, "0 machine ID is reserved!");
            if (s_machineId != 0)
            {
                s_machineId = machineId;
            }
        }
    } // namespace Platform
}
