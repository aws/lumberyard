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

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE) // ACCEPTED_USE
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/PlatformIncl.h>

namespace
{
    const char* moduleExtension = ".dll";
}

namespace AZ
{
    class DynamicModuleHandleWindows
        : public DynamicModuleHandle
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicModuleHandleWindows, OSAllocator, 0)

        DynamicModuleHandleWindows(const char* fullFileName)
            : DynamicModuleHandle(fullFileName)
            , m_handle(nullptr)
        {
            // Ensure filename ends in ".dll"
            // Otherwise filenames like "gem.1.0.0" fail to load (.0 is assumed to be the extension).
            if (m_fileName.substr(m_fileName.length() - 4) != moduleExtension)
            {
                m_fileName = m_fileName + moduleExtension;
            }
        }

        ~DynamicModuleHandleWindows() override
        {
            Unload();
        }

        LoadStatus LoadModule() override
        {
            if (IsLoaded())
            {
                return LoadStatus::AlreadyLoaded;
            }

            bool alreadyLoaded = false;

#   ifdef _UNICODE
            wchar_t fileNameW[MAX_PATH];
            size_t numCharsConverted;
            errno_t wcharResult = mbstowcs_s(&numCharsConverted, fileNameW, m_fileName.c_str(), AZ_ARRAY_SIZE(fileNameW) - 1);
            if (wcharResult == 0)
            {
                // If module already open, return false, it was not loaded.
                alreadyLoaded = NULL != GetModuleHandleW(fileNameW);
                m_handle = LoadLibraryW(fileNameW);
            }
            else
            {
                AZ_Assert(false, "Failed to convert %s to wchar with error %d!", m_fileName.c_str(), wcharResult);
                m_handle = NULL;
                return LoadStatus::LoadFailure;
            }
#   else //!_UNICODE
            alreadyLoaded = NULL != GetModuleHandleA(m_fileName.c_str());
            m_handle = LoadLibraryA(m_fileName.c_str());
#   endif // !_UNICODE

            if (m_handle)
            {
                if (alreadyLoaded)
                {
                    return LoadStatus::AlreadyLoaded;
                }
                else
                {
                    return LoadStatus::LoadSuccess;
                }
            }
            else
            {
                return LoadStatus::LoadFailure;
            }
        }

        bool UnloadModule() override
        {
            bool result = false;
            if (m_handle)
            {
                result = FreeLibrary(m_handle) ? true : false;
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
            return reinterpret_cast<void*>(::GetProcAddress(m_handle, functionName));
        }

        HMODULE m_handle;
    };

    // Implement the module creation function
    AZStd::unique_ptr<DynamicModuleHandle> DynamicModuleHandle::Create(const char* fullFileName)
    {
        return AZStd::unique_ptr<DynamicModuleHandle>(aznew DynamicModuleHandleWindows(fullFileName));
    }
} // namespace AZ

#endif // AZ_PLATFORM_WINDOWS || AZ_PLATFORM_XBONE // ACCEPTED_USE

#endif // #ifndef AZ_UNITY_BUILD
