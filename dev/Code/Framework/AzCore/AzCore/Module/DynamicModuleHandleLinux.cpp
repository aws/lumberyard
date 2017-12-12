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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Module/DynamicModuleHandle.h>

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE_OSX)
#include <AzCore/Memory/OSAllocator.h>
#include <dlfcn.h>

#if defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_LINUX)
const char* modulePrefix = "lib";
const char* moduleExtension = ".so";
#elif defined(AZ_PLATFORM_APPLE_OSX)
const char* modulePrefix = "lib";
const char* moduleExtension = ".dylib";

#include <mach-o/dyld.h>
#include <libgen.h>
#endif

namespace AZ
{
    class DynamicModuleHandleLinux
        : public DynamicModuleHandle
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicModuleHandleLinux, OSAllocator, 0)

        DynamicModuleHandleLinux(const char* fullFileName)
            : DynamicModuleHandle(fullFileName)
            , m_handle(nullptr)
        {
            AZ::OSString path;
            AZ::OSString fileName;
            AZ::OSString::size_type finalSlash = m_fileName.find_last_of("/");
            if (finalSlash != AZ::OSString::npos)
            {
                // Path up to and including final slash
                path = m_fileName.substr(0, finalSlash + 1);
                // Everything after the final slash
                // If m_fileName ends in /, the end result is path/lib.dylib, which just fails to load.
                fileName = m_fileName.substr(finalSlash + 1);
            }
            else
            {
                // If no slash found, assume empty path, only file name
                path = "";
#ifdef AZ_PLATFORM_APPLE
                uint32_t bufsize = 1024;
                char exePath[bufsize];
                if (_NSGetExecutablePath(exePath, &bufsize) == 0)
                {
                    path = dirname(exePath);
                    path += "/";
                }
#endif
                fileName = m_fileName;
            }

            if (fileName.substr(0, 3) != modulePrefix)
            {
                fileName = modulePrefix + fileName;
            }

            if (fileName.substr(fileName.length() - 3, 3) != moduleExtension)
            {
                fileName = fileName + moduleExtension;
            }

            m_fileName = path + fileName;
        }

        ~DynamicModuleHandleLinux() override
        {
            Unload();
        }

        LoadStatus LoadModule() override
        {
            AZ::Debug::Trace::Printf("Module", "Attempting to load module:%s\n", m_fileName.c_str());
            bool alreadyOpen = false;
            // Android 19 does not have RTLD_NOLOAD but it should be OK since only the Editor expects to reopen modules
#ifndef AZ_PLATFORM_ANDROID
            alreadyOpen = NULL != dlopen(m_fileName.c_str(), RTLD_NOLOAD);
#endif
            m_handle = dlopen (m_fileName.c_str(), RTLD_NOW);
            if(m_handle)
            {
                if (alreadyOpen)
                {
                    AZ::Debug::Trace::Printf("Module", "Success! System already had it opened.\n");
                    // We want to return LoadSuccess and not AlreadyLoaded
                    // because the system may have already loaded the DLL (because
                    // it is a dependency of another library we have loaded) but
                    // we have not run our initialization routine on it. Returning
                    // AlreadyLoaded stops us from running the initialization
                    // routine that results in a cascade of failures and potential
                    // crash.
                    return LoadStatus::LoadSuccess;
                }
                else
                {
                    AZ::Debug::Trace::Printf("Module", "Success!\n");
                    return LoadStatus::LoadSuccess;
                }
            }
            else
            {
                AZ::Debug::Trace::Printf("Module", "Failed with error:\n%s\n", dlerror());
                return LoadStatus::LoadFailure;
            }
        }

        bool UnloadModule() override
        {
            bool result = false;
            if (m_handle)
            {
                result = dlclose(m_handle) == 0 ? true : false;
                m_handle = 0;
            }
            return result;
        }

        bool IsLoaded() const override
        {
            return m_handle ? true : false;
        }

        void* GetFunctionAddress(const char* functionName) const override
        {
            return dlsym(m_handle, functionName);
        }

        void* m_handle;
    };

    // Implement the module creation function
    AZStd::unique_ptr<DynamicModuleHandle> DynamicModuleHandle::Create(const char* fullFileName)
    {
        return AZStd::unique_ptr<DynamicModuleHandle>(aznew DynamicModuleHandleLinux(fullFileName));
    }
} // namespace AZ

#endif //defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID)

#endif // #ifndef AZ_UNITY_BUILD
