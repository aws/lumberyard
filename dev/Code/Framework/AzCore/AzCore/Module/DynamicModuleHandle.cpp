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
#include <AzCore/Module/Environment.h>

namespace AZ
{
    DynamicModuleHandle::DynamicModuleHandle(const char* fileFullName)
        : m_fileName(fileFullName)
        , m_requiresUninitializeFunction(false)
    {
    }

    bool DynamicModuleHandle::Load(bool isInitializeFunctionRequired)
    {
        Unload();

        LoadStatus status = LoadModule();
        switch (status) 
        {
            case LoadStatus::LoadFailure:
                m_requiresUninitializeFunction = false;
                return false;
            case LoadStatus::AlreadyLoaded:
                return true;
            case LoadStatus::LoadSuccess:
                break;
        }

        // Call module's initialize function.
        auto initFunc = GetFunction<InitializeDynamicModuleFunction>(InitializeDynamicModuleFunctionName);
        if (initFunc)
        {
            m_requiresUninitializeFunction = true;

            initFunc(AZ::Environment::GetInstance());
        }
        else if (isInitializeFunctionRequired)
        {
            AZ_Error("Module", false, "Unable to locate required entry point '%s' within module '%s'.",
                InitializeDynamicModuleFunctionName, m_fileName.c_str());
            Unload();
            return false;
        }

        return true;
    }

    bool DynamicModuleHandle::Unload()
    {
        if (!IsLoaded())
        {
            return false;
        }

        // Call module's uninitialize function
        if (m_requiresUninitializeFunction)
        {
            m_requiresUninitializeFunction = false;

            auto uninitFunc = GetFunction<UninitializeDynamicModuleFunction>(UninitializeDynamicModuleFunctionName);
            AZ_Error("Module", uninitFunc, "Unable to locate required entry point '%s' within module '%s'.",
                UninitializeDynamicModuleFunctionName, m_fileName.c_str());
            if (uninitFunc)
            {
                uninitFunc();
            }
        }

        if (!UnloadModule())
        {
            AZ_Error("Module", false, "Failed to unload module at \"%s\".", m_fileName.c_str());
            return false;
        }

        return true;
    }

#if !defined(AZ_HAS_DLL_SUPPORT)
    // Default create which fails for unsupported platforms
    AZStd::unique_ptr<DynamicModuleHandle> DynamicModuleHandle::Create(const char*)
    {
        AZ_Assert(false, "Dynamic modules are not supported on this platform");
        return nullptr;
    }
#endif
} // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD