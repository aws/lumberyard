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
#include <IConfig.h>
#include <SceneConfig.h>
#include <SceneAPI/SceneCore/Import/Utilities/FileFinder.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>

// Hack to force loading of FbxSceneBuilder library at load time
// Must be DLL_EXPORT to ensure it's not stripped by the linker
AZ_DLL_EXPORT void* DoNotCall()
{
    AZ_Assert(false, "This function should never be called. It exists only to force the loading of the FbxSceneBuilder.");
    return new AZ::SceneAPI::FbxSceneSystem();
}

namespace AZ
{
    namespace RC
    {
        SceneConfig::SceneConfig()
        {
            LoadSceneLibrary("SceneCore");
            LoadSceneLibrary("SceneData");
            LoadSceneLibrary("FbxSceneBuilder");
        }

        SceneConfig::~SceneConfig()
        {
            // Explicitly uninitialize all modules. Because we loaded them twice, we need to explicitly uninit them.
            for (AZStd::unique_ptr<AZ::DynamicModuleHandle>& module : m_modules)
            {
                auto uninit = module->GetFunction<UninitializeDynamicModuleFunction>(UninitializeDynamicModuleFunctionName);
                if (uninit)
                {
                    (*uninit)();
                }
            }
        }

        const char* SceneConfig::GetManifestFileExtension() const
        {
            return AZ::SceneAPI::Import::Utilities::FileFinder::GetManifestExtension();
        }

        void SceneConfig::ReflectSceneModules(SerializeContext* context)
        {
            for (AZStd::unique_ptr<AZ::DynamicModuleHandle>& module : m_modules)
            {
                AZ_TraceContext("Scene Module", module->GetFilename());
                AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Reflecting scene module to context.\n");
                using ReflectFunc = void(*)(SerializeContext*);

                ReflectFunc reflect = module->GetFunction<ReflectFunc>("Reflect");
                if (reflect)
                {
                    (*reflect)(context);
                }
            }
        }

        void SceneConfig::ActivateSceneModules()
        {
            for (AZStd::unique_ptr<AZ::DynamicModuleHandle>& module : m_modules)
            {
                AZ_TraceContext("Scene Module", module->GetFilename());
                AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Activating scene module.\n");
                using ActivateFunc = void(*)();

                ActivateFunc activate = module->GetFunction<ActivateFunc>("Activate");
                if (activate)
                {
                    (*activate)();
                }
            }
        }

        void SceneConfig::DeactivateSceneModules()
        {
            for (AZStd::unique_ptr<AZ::DynamicModuleHandle>& module : m_modules)
            {
                AZ_TraceContext("Scene Module", module->GetFilename());
                AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Deactivating scene module.\n");
                using DeactivateFunc = void(*)();

                DeactivateFunc deactivate = module->GetFunction<DeactivateFunc>("Deactivate");
                if (deactivate)
                {
                    (*deactivate)();
                }
            }
        }

        void SceneConfig::LoadSceneLibrary(const char* name)
        {
            AZStd::unique_ptr<DynamicModuleHandle> module = AZ::DynamicModuleHandle::Create(name);
            AZ_Assert(module, "Failed to initialize library '%s'", name);
            if (!module)
            {
                return;
            }
            module->Load(false);

            // Explicitly initialize all modules. Because we're loading them twice (link time, and now-time), we need to explicitly uninit them.
            auto init = module->GetFunction<InitializeDynamicModuleFunction>(InitializeDynamicModuleFunctionName);
            if (init)
            {
                (*init)(AZ::Environment::GetInstance());
            }

            m_modules.push_back(AZStd::move(module));
        }

        size_t SceneConfig::GetErrorCount() const
        {
            return traceHook.GetErrorCount();
        }
    } // namespace RC
} // namespace AZ