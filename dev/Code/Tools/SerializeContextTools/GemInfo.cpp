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

#include <AzCore/Debug/Trace.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/ModuleManager.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <GemRegistry/IGemRegistry.h>
#include <GemInfo.h>

namespace AZ
{
    namespace SerializeContextTools
    {
        bool GemInfo::LoadGemInformation(const char* appRoot, const char* gameRoot)
        {
            AZ::ModuleManagerRequests::LoadModuleOutcome gemRegistryResult = AZ::Failure(AZStd::string("Failed to connect to ModuleManagerRequestBus"));
            AZ::ModuleManagerRequestBus::BroadcastResult(gemRegistryResult, &AZ::ModuleManagerRequestBus::Events::LoadDynamicModule, "GemRegistry", AZ::ModuleInitializationSteps::Load, false);
            if (!gemRegistryResult.IsSuccess())
            {
                AZ_Error("GemInfo", false, "Could not load the GemRegistry module - %s", gemRegistryResult.GetError().c_str());
                return false;
            }

            // Use shared_ptr aliasing ctor to use the refcount/deleter from the moduledata pointer, but we only need to store the dynamic module handle.
            AZStd::shared_ptr<AZ::DynamicModuleHandle> registryModule = AZStd::shared_ptr<DynamicModuleHandle>(gemRegistryResult.GetValue(), gemRegistryResult.GetValue()->GetDynamicModuleHandle());
            auto CreateGemRegistry = registryModule->GetFunction<Gems::RegistryCreatorFunction>(GEMS_REGISTRY_CREATOR_FUNCTION_NAME);
            auto DestroyGemRegistry = registryModule->GetFunction<Gems::RegistryDestroyerFunction>(GEMS_REGISTRY_DESTROYER_FUNCTION_NAME);
            if (!CreateGemRegistry || !DestroyGemRegistry)
            {
                AZ_Error("GemInfo", false, "Failed to load GemRegistry functions.");
                return false;
            }

            auto destroyGemRegistryCallback = [&DestroyGemRegistry](Gems::IGemRegistry* registry)
            {
                DestroyGemRegistry(registry);
            };
            AZStd::unique_ptr<Gems::IGemRegistry, decltype(destroyGemRegistryCallback)> gemRegistry(CreateGemRegistry(), destroyGemRegistryCallback);
            if (!gemRegistry)
            {
                AZ_Error("GemInfo", false, "Failed to create Gems::GemRegistry.");
                return false;
            }

            AZStd::string gemsRoot;
            AzFramework::StringFunc::Path::ConstructFull(appRoot, "Gems", gemsRoot);
            ComponentValidationResult gemRegistrySearchPathResult = gemRegistry->AddSearchPath(Gems::SearchPath(gemsRoot), true);
            if (!gemRegistrySearchPathResult)
            {
                AZ_Error("Convert", false, "Failed to load gem information at '%s' - %s.",
                    gemsRoot.c_str(), gemRegistrySearchPathResult.GetError().c_str());
                return false;
            }

            auto destroyProjectSettingsCallback = [&gemRegistry](Gems::IProjectSettings* settings)
            {
                gemRegistry->DestroyProjectSettings(settings);
            };
            AZStd::unique_ptr<Gems::IProjectSettings, decltype(destroyProjectSettingsCallback)> projectSettings(
                gemRegistry->CreateProjectSettings(), destroyProjectSettingsCallback);
            if (!projectSettings)
            {
                AZ_Error("Gems", false, "Failed to create Gems::ProjectSettings.");
                return false;
            }

            if (!projectSettings->Initialize(appRoot, gameRoot + strlen(appRoot)))
            {
                AZ_Error("Gems", false, "Failed to initialize Gems::ProjectSettings.");
                return false;
            }

            auto loadProjectOutcome = gemRegistry->LoadProject(*projectSettings, true);
            if (!loadProjectOutcome.IsSuccess())
            {
                AZ_Error("Gems", false, "Failed to load Gems project: %s", loadProjectOutcome.GetError().c_str());
                return false;
            }

            for (auto& gem : projectSettings->GetGems())
            {
                Gems::IGemDescriptionConstPtr gemDescriptor = gemRegistry->GetGemDescription(gem.second);
                for (auto& module : gemDescriptor->GetModules())
                {
                    m_moduleToGemFolderLookup.emplace(module->m_fileName, gemDescriptor->GetAbsolutePath());
                    if (m_gemFolders.find(gemDescriptor->GetAbsolutePath()) == m_gemFolders.end())
                    {
                        m_gemFolders.insert(gemDescriptor->GetAbsolutePath());
                    }
                }
            }

            return true;
        }

        AZStd::string_view GemInfo::LocateGemPath(const Uuid& moduleClassId) const
        {
            if (m_moduleToGemFolderLookup.empty())
            {
                AZ_Assert(false, "GemInfo::LocateGemPath called before gem information has been loaded.");
                return AZStd::string_view{};
            }

            AZStd::string_view result;
            auto moduleCallback = [this, &result, &moduleClassId](const ModuleData& moduleData) -> bool
            {
                if (moduleData.GetModule() && moduleClassId == azrtti_typeid(*moduleData.GetModule()))
                {
                    if (moduleData.GetDynamicModuleHandle())
                    {
                        const OSString& modulePath = moduleData.GetDynamicModuleHandle()->GetFilename();
                        AZStd::string fileName;
                        AzFramework::StringFunc::Path::GetFileName(modulePath.c_str(), fileName);
                        auto pathIt = m_moduleToGemFolderLookup.find(fileName);
                        if (pathIt != m_moduleToGemFolderLookup.end())
                        {
                            result = pathIt->second;
                        }
                    }
                    return false;
                }
                return true;
            };
            ModuleManagerRequestBus::Broadcast(&ModuleManagerRequestBus::Events::EnumerateModules, moduleCallback);
            return result;
        }

        const AZStd::unordered_set<AZStd::string>& GemInfo::GetGemFolders() const
        {
            return m_gemFolders;
        }
    } // namespace SerializeContextTools
} // namespace AZ