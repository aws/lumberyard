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
#include "StdAfx.h"
#include "GemManager.h"

#include "CryLibrary.h"
#include "CryExtension/ICryFactoryRegistry.h"
#include <CryExtension/CryCreateClassInstance.h>

#include <AzCore/std/utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/regex.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

#ifdef AZ_MONOLITHIC_BUILD
extern "C" DLL_EXPORT Gems::IGemRegistry * CreateGemRegistry();
extern "C" DLL_EXPORT void DestroyGemRegistry(Gems::IGemRegistry* reg);
#else//AZ_MONOLITHIC_BUILD
Gems::RegistryCreatorFunction CreateGemRegistry;
Gems::RegistryDestroyerFunction DestroyGemRegistry;
#endif//AZ_MONOLITHIC_BUILD

bool GemManager::Initialize(const SSystemInitParams& initParams)
{
    // Load the Gems file pak
    gEnv->pCryPak->OpenPack("@root@", "gems/gems.pak");

    if (!InitRegistry())
    {
        return false;
    }

    if (!LoadGems(initParams))
    {
        return false;
    }

    if (!InstantiateGems())
    {
        return false;
    }

    return true;
}

GemManager::~GemManager()
{
    for (auto&& gemDesc : m_gemPtrs)
    {
        GetISystem()->GetISystemEventDispatcher()->RemoveListener(gemDesc.second.get());
    }
    m_gemPtrs.clear();

    if (m_registry)
    {
        m_registry->DestroyProjectSettings(m_projectSettings);
        DestroyGemRegistry(m_registry);
    }
}

const IGem* GemManager::GetGem(const CryClassID& classID) const
{
    for (auto && desc : m_gemPtrs)
    {
        if (IGemPtr gem = desc.second)
        {
            // Check if Gem is of ID given or if it implements the interface given
            if (gem->GetFactory()->GetClassID() == classID || gem->GetFactory()->ClassSupports(classID))
            {
                return gem.get();
            }
        }
    }

    return nullptr;
}

bool GemManager::IsGemEnabled(const AZ::Uuid& id, const AZStd::vector<AZStd::string>& versionConstraints) const
{
    AZ_Assert(m_projectSettings, "Project settings must be initialized before checking to see if a gem is enabled.");
    return m_projectSettings->IsGemEnabled(id, versionConstraints);
}

bool GemManager::InitRegistry()
{
#ifndef AZ_MONOLITHIC_BUILD
    AZ::ModuleManagerRequests::LoadModuleOutcome result = AZ::Failure(AZStd::string("Failed to connect to ModuleManagerRequestBus"));
    AZ::ModuleManagerRequestBus::BroadcastResult(result, &AZ::ModuleManagerRequestBus::Events::LoadDynamicModule, "GemRegistry", AZ::ModuleInitializationSteps::Load, false);
    if (result.IsSuccess())
    {
        // Use shared_ptr aliasing ctor to use the refcount/deleter from the moduledata pointer, but we only need to store the dynamic module handle.
        m_registryModule = AZStd::shared_ptr<AZ::DynamicModuleHandle>(result.GetValue(), result.GetValue()->GetDynamicModuleHandle());

    CreateGemRegistry = m_registryModule->GetFunction<Gems::RegistryCreatorFunction>(GEMS_REGISTRY_CREATOR_FUNCTION_NAME);
    DestroyGemRegistry = m_registryModule->GetFunction<Gems::RegistryDestroyerFunction>(GEMS_REGISTRY_DESTROYER_FUNCTION_NAME);
    if (!CreateGemRegistry || !DestroyGemRegistry)
    {
        AZ_Error("Gems", false, "Failed to load GemRegistry functions.");
        return false;
    }
    }
    else
    {
        AZ_Error("Gems", false, "Failed to load GemRegistry module.");
        return false;
    }
#endif//AZ_MONOLITHIC_BUILD

    m_registry = CreateGemRegistry();
    if (!m_registry)
    {
        AZ_Error("Gems", false, "Failed to create Gems::GemRegistry.");
        return false;
    }

    m_registry->AddSearchPath({ "@root@", "Gems" }, false);

    return true;
}

bool GemManager::LoadGems(const SSystemInitParams& initParams)
{
    m_projectSettings = m_registry->CreateProjectSettings();

    // get the game name:

    if (!m_projectSettings->Initialize("", "@assets@"))
    {
        const char* assetPath = initParams.UseAssetCache() ? initParams.assetsPathCache : initParams.assetsPath;
        AZ_Error("Gems", false, "Error initializing Gems::ProjectSettings for project. Add gems.json to %s at enabled Gems", assetPath);
        return false;
    }

    auto loadProjectOutcome = m_registry->LoadProject(*m_projectSettings, false);
    if (!loadProjectOutcome.IsSuccess())
    {
        AZ_Error("Gems", false, "Failed to load Gems project: %s", loadProjectOutcome.GetError().c_str());
        return false;
    }

    AZStd::vector<Gems::IGemDescriptionConstPtr> gemDescs;

#if !defined(AZ_MONOLITHIC_BUILD)
    // List of Gem modules not loaded
    AZStd::vector<AZStd::string> missingModules;
#endif // AZ_MONOLITHIC_BUILD

    for (const auto& pair : m_projectSettings->GetGems())
    {
        auto desc = m_registry->GetGemDescription(pair.second);

        if (!desc)
        {
            Gems::ProjectGemSpecifier spec = pair.second;
            AZStd::string errorStr = AZStd::string::format(
                    "Failed to load Gem with ID %s and Version %s (from path %s).",
                    spec.m_id.ToString<AZStd::string>().c_str(), spec.m_version.ToString().c_str(), spec.m_path.c_str());

            if (Gems::IGemDescriptionConstPtr latestVersion = m_registry->GetLatestGem(pair.first))
            {
                errorStr += AZStd::string::format(" Found version %s, you may want to use that instead.", latestVersion->GetVersion().ToString().c_str());
            }

            AZ_Error("Gems", false, errorStr.c_str());
            return false;
        }

        gemDescs.push_back(desc);

#if !defined(AZ_MONOLITHIC_BUILD)
        Gems::ModuleDefinition::Type type = gEnv->IsEditor()
            ? Gems::ModuleDefinition::Type::EditorModule
            : Gems::ModuleDefinition::Type::GameModule;

        for (const auto& modulePtr : desc->GetModulesOfType(type))
        {
            missingModules.emplace_back(modulePtr->m_fileName);
        }
#endif // AZ_MONOLITHIC_BUILD
    }

    auto depOutcome = m_projectSettings->ValidateDependencies();
    if (!depOutcome.IsSuccess())
    {
        AZ_Error("Gems", false, depOutcome.GetError().c_str());
        return false;
    }

#if !defined(AZ_MONOLITHIC_BUILD)
    // Init all the modules with ModuleInitISystem in them
    EBUS_EVENT(AZ::ModuleManagerRequestBus, EnumerateModules, [&missingModules](const AZ::ModuleData& moduleData)
    {
        // If dynamic module handle...
        if (moduleData.GetDynamicModuleHandle())
        {
            const AZ::DynamicModuleHandle* handle = moduleData.GetDynamicModuleHandle();
            // Check if the variable inserted by GEM_REGISTER macro exists
            if (handle->GetFunction<int(*)()>("ThisModuleIsAGem"))
            {
                if (auto moduleInitFun = handle->GetFunction<void(*)(ISystem*, const char*)>("ModuleInitISystem"))
                {
                    moduleInitFun(GetISystem(), moduleData.GetDebugName());
                }
            }

            // Find the Gem for this module, and remove it from the missing list
            auto gemIt = AZStd::find_if(missingModules.begin(), missingModules.end(), [handle](AZStd::string gemModuleName) {
                return strstr(handle->GetFilename().c_str(), gemModuleName.c_str()) != nullptr;
            });
            if (gemIt != missingModules.end())
            {
                missingModules.erase(gemIt);
            }
        }

        return true;
    });

    AZStd::string_view binFolderName;
    AZ::ComponentApplicationBus::BroadcastResult(binFolderName, &AZ::ComponentApplicationRequests::GetBinFolder);

    for (const auto& missingModule : missingModules)
    {
        AZ_Error("Gems", false,
            "Module %s is missing. This occurs when the module needs to be built, or it was added to the current project's gems.json, but not the app descriptor.\n"
            "App descriptors for all projects can be updated by running lmbr from your dev/ root with the following arguments: \"lmbr projects populate-appdescriptors\".\n"
            "The lmbr executable can be found in '%s'.",
            missingModule.c_str(),
            binFolderName.data());
        return false;
    }
#endif // AZ_MONOLITHIC_BUILD

    for (const auto& desc : gemDescs)
    {
        if (!desc->GetModules().empty())
        {
            // Attempt to initialize old style Gems
            IGemPtr pModule;
            if (CryCreateClassInstance(desc->GetEngineModuleClass().c_str(), pModule))
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Gem %s uses deprecated IGem interface. If you are the owner of this Gem, please port to new AZ::Module interface.", desc->GetName().c_str());
                pModule->Initialize(*gEnv, initParams);
            }
        }
    }

    return true;
}

bool GemManager::InstantiateGems()
{
    const ICryFactoryRegistry* factoryRegistry = GetISystem()->GetCryFactoryRegistry();
    const CryInterfaceID igemiid = cryiidof<IGem>();

    // Get number of registered Gem classes
    size_t numFactories = 0;
    factoryRegistry->IterateFactories(igemiid, nullptr, numFactories);

    if (numFactories > 0)
    {
        // Get Factories for classes that implement IGem
        std::unique_ptr<ICryFactory*[]> factories(new ICryFactory*[numFactories]);
        factoryRegistry->IterateFactories(igemiid, factories.get(), numFactories);

        // Iterate over factories
        for (size_t i = 0; i < numFactories; ++i)
        {
            if (IGemPtr gemInst = cryinterface_cast<IGem>(factories[i]->CreateClassInstance()))
            {
                // Register Gem for system events
                GetISystem()->GetISystemEventDispatcher()->RegisterListener(gemInst.get());

                // Store pointer in map
                const CryGUID id = gemInst->GetClassID();
                if (m_gemPtrs.find(id) == m_gemPtrs.end())
                {
                    m_gemPtrs.insert(std::make_pair(id, gemInst));
                }
                else
                {
                    AZ_Error("Gems", false, "Gem has already been registered. Is the GUID provided to GEM_IMPLEMENT used elsewhere?");
                    return false;
                }
            }
        }
    }

    return true;
}
