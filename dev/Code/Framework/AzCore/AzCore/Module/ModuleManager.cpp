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

#include <AzCore/Module/ModuleManager.h>

#include <AzCore/Module/Module.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/ScriptContext.h>

#include <AzCore/std/smart_ptr/make_shared.h>

namespace
{
    static const char* s_moduleLoggingScope = "Module";
}

namespace AZ
{
    bool ShouldUseSystemComponent(const ComponentDescriptor& descriptor, const AZStd::vector<Crc32>& requiredTags, const SerializeContext& serialize)
    {
        const SerializeContext::ClassData* classData = serialize.FindClassData(descriptor.GetUuid());
        AZ_Warning(s_moduleLoggingScope, classData, "Component type %s not reflected to SerializeContext!", descriptor.GetName());
        if (classData)
        {
            if (Attribute* attribute = FindAttribute(Edit::Attributes::SystemComponentTags, classData->m_attributes))
            {
                // If the required tags are empty, it is assumed all components with the attribute are required
                if (requiredTags.empty())
                {
                    return true;
                }

                // Read the tags
                AZStd::vector<AZ::Crc32> tags;
                AZ::AttributeReader reader(nullptr, attribute);
                AZ::Crc32 tag;
                if (reader.Read<AZ::Crc32>(tag))
                {
                    tags.emplace_back(tag);
                }
                else
                {
                    AZ_Verify(reader.Read<AZStd::vector<AZ::Crc32>>(tags), "Attribute \"AZ::Edit::Attributes::SystemComponentTags\" must be of type AZ::Crc32 or AZStd::vector<AZ::Crc32>");
                }

                // Match tags to required tags
                for (const AZ::Crc32& requiredTag : requiredTags)
                {
                    if (AZStd::find(tags.begin(), tags.end(), requiredTag) != tags.end())
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void DynamicModuleDescriptor::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            // Bail early to avoid double reflection
            bool isReflected = serializeContext->FindClassData(azrtti_typeid<DynamicModuleDescriptor>()) != nullptr;
            if (isReflected != context->IsRemovingReflection())
            {
                return;
            }

            serializeContext->Class<DynamicModuleDescriptor>()
                ->Field("dynamicLibraryPath", &DynamicModuleDescriptor::m_dynamicLibraryPath)
                ;

            if (EditContext* ec = serializeContext->GetEditContext())
            {
                ec->Class<DynamicModuleDescriptor>(
                    "Dynamic Module descriptor", "Describes a dynamic module (DLL) used by the application")
                    ->DataElement(Edit::UIHandlers::Default, &DynamicModuleDescriptor::m_dynamicLibraryPath, "Dynamic library path", "Path to DLL.")
                    ;
            }
        }
    }

    //=========================================================================
    // ModuleEntity
    //=========================================================================
    void ModuleEntity::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
        {
            serialize->Class<ModuleEntity, Entity>()
                ->Field("moduleClassId", &ModuleEntity::m_moduleClassId)
                ;
        }
    }

    //=========================================================================
    // SetState
    //=========================================================================
    void ModuleEntity::SetState(Entity::State state)
    {
        if (state == Entity::ES_DEACTIVATING)
        {
            EntityBus::Event(m_id, &EntityBus::Events::OnEntityDeactivated, m_id);
            EntitySystemBus::Broadcast(&EntitySystemBus::Events::OnEntityDeactivated, m_id);
        }

        m_state = state;

        if (state == Entity::ES_ACTIVE)
        {
            EntityBus::Event(m_id, &EntityBus::Events::OnEntityActivated, m_id);
            EntitySystemBus::Broadcast(&EntitySystemBus::Events::OnEntityActivated, m_id);
        }
    }

    //=========================================================================
    // GetDebugName
    //=========================================================================
    const char* ModuleDataImpl::GetDebugName() const
    {
        // If module is from DLL, return DLL name.
        if (m_dynamicHandle)
        {
            const char* filepath = m_dynamicHandle->GetFilename().c_str();
            const char* lastSlash = strrchr(filepath, '/');
            return lastSlash ? lastSlash + 1 : filepath;
        }
        // If Module has its own RTTI info, return that name
        else if (m_module && !azrtti_istypeof<Module>(m_module))
        {
            return m_module->RTTI_GetTypeName();
        }
        else
        {
            return "module";
        }
    }

    //=========================================================================
    // ~ModuleDataImpl
    //=========================================================================
    ModuleDataImpl::~ModuleDataImpl()
    {
        m_moduleEntity.reset();

        // If the AZ::Module came from a DLL, destroy it
        // using the DLL's \ref DestroyModuleClassFunction.
        if (m_module && m_dynamicHandle)
        {
            auto destroyFunc = m_dynamicHandle->GetFunction<DestroyModuleClassFunction>(DestroyModuleClassFunctionName);
            AZ_Assert(destroyFunc, "Unable to locate '%s' entry point in module at \"%s\".",
                DestroyModuleClassFunctionName, m_dynamicHandle->GetFilename().c_str());
            if (destroyFunc)
            {
                destroyFunc(m_module);
                m_module = nullptr;
            }
        }

        // If the AZ::Module came from a static LIB, delete it directly.
        if (m_module)
        {
            delete m_module;
            m_module = nullptr;
        }

        // If dynamic, unload DLL and destroy handle.
        if (m_dynamicHandle)
        {
            m_dynamicHandle->Unload();
            m_dynamicHandle.reset();
        }
    }

    static_assert(!AZStd::has_trivial_copy<ModuleDataImpl>::value, "Compiler believes ModuleData is trivially copyable, despite having non-trivially copyable members.\n");

    //=========================================================================
    // Reflect
    //=========================================================================
    void ModuleManager::Reflect(ReflectContext* context)
    {
        DynamicModuleDescriptor::Reflect(context);
        ModuleEntity::Reflect(context);
    }

    //=========================================================================
    // ModuleManager
    //=========================================================================
    ModuleManager::ModuleManager()
    {
        ModuleManagerRequestBus::Handler::BusConnect();
        Internal::ModuleManagerInternalRequestBus::Handler::BusConnect();

        // Once the system entity activates, we can activate our entities
        EntityBus::Handler::BusConnect(SystemEntityId);
    }

    //=========================================================================
    // ~ModuleManager
    //=========================================================================
    ModuleManager::~ModuleManager()
    {
        ModuleManagerRequestBus::Handler::BusDisconnect();

        UnloadModules();

#if defined(AZ_ENABLE_TRACING)
        AZStd::string moduleHandlesOpen;
        for (const auto& weakModulePair : m_notOwnedModules)
        {
            AZ_Assert(!weakModulePair.second.expired(), "Internal error: Module was unloaded, but not removed from weak modules list!");
            auto modulePtr = weakModulePair.second.lock();
            moduleHandlesOpen += modulePtr->GetDebugName();
            moduleHandlesOpen += "\n";
        }
        AZ_Assert(m_notOwnedModules.empty(), "ModuleManager being destroyed, but module handles are still open:\n%s", moduleHandlesOpen.c_str());
#endif // AZ_ENABLE_TRACING

        // Clear the weak modules list
        {
            UnownedModulesMap emptyMap;
            m_notOwnedModules.swap(emptyMap);
        }

        // Disconnect from this last so that unloads still fire events back
        Internal::ModuleManagerInternalRequestBus::Handler::BusDisconnect();
    }

    //=========================================================================
    // UnloadModules
    //=========================================================================
    void ModuleManager::UnloadModules()
    {
        // For all modules that we created an entity for, set them to "Deactivating"
        for (auto& moduleData : m_ownedModules)
        {
            if (moduleData->m_moduleEntity)
            {
                moduleData->m_moduleEntity->SetState(Entity::ES_DEACTIVATING);
            }
        }

        // For all system components, deactivate
        for (auto componentIt = m_systemComponents.rbegin(); componentIt != m_systemComponents.rend(); ++componentIt)
        {
            ModuleEntity::DeactivateComponent(**componentIt);
        }

        // For all modules that we created an entity for, set them to "Init" (meaning not Activated)
        for (auto& moduleData : m_ownedModules)
        {
            if (moduleData->m_moduleEntity)
            {
                moduleData->m_moduleEntity->SetState(Entity::ES_INIT);
            }
        }

        // Because everything is unique_ptr, we don't need to explicitly delete anything
        // Shutdown in reverse order of initialization, just in case the order matters.
        while (!m_ownedModules.empty())
        {
            m_ownedModules.pop_back(); // ~ModuleData() handles shutdown logic
        }
        m_ownedModules.set_capacity(0);
    }

    //=========================================================================
    // AddModuleEntity
    //=========================================================================
    void ModuleManager::AddModuleEntity(ModuleEntity* moduleEntity)
    {
        for (auto& moduleData : m_ownedModules)
        {
            if (moduleEntity->m_moduleClassId == azrtti_typeid(moduleData->m_module))
            {
                AZ_Assert(!moduleData->m_moduleEntity, "Adding module entity twice!");
                moduleData->m_moduleEntity.reset(moduleEntity);
                return;
            }
        }
    }

    //=========================================================================
    // EnumerateModules
    //=========================================================================
    void ModuleManager::EnumerateModules(EnumerateModulesCallback perModuleCallback)
    {
        for (auto& moduleData : m_ownedModules)
        {
            if (!perModuleCallback(*moduleData))
            {
                return;
            }
        }
    }

    //=========================================================================
    // PreProcessModule
    // Preprocess a dynamic module name.  This is where we will apply any legacy/upgrade
    // of modules if necessary (and provide a warning to update the configuration)
    //=========================================================================
    AZ::OSString ModuleManager::PreProcessModule(const AZ::OSString& moduleName)
    {
        // This is the list of modules that have been deprecated as of version 1.10
        // Update this list accordingly.  
        static const char* legacyModules[] = { "LmbrCentral",
                                               "LmbrCentralEditor" };
        // List of modules (Gems) that represents the upgrade from the legacyModules 
        static const char* legacyUpgradeModules[] = { "Gem.LmbrCentral.ff06785f7145416b9d46fde39098cb0c.v0.1.0",
                                                      "Gem.LmbrCentral.Editor.ff06785f7145416b9d46fde39098cb0c.v0.1.0" };

        // Process out just the module name from a potential path or subpath
        OSString preprocessedName;
        OSString prefix = "";

        auto pathSeparatorIndex = moduleName.find_last_of("/\\");
        if (pathSeparatorIndex != moduleName.npos)
        {
            preprocessedName = moduleName.substr(pathSeparatorIndex + 1);
            prefix = moduleName.substr(0, pathSeparatorIndex + 1);
        }
        else
        {
            preprocessedName = moduleName;
        }

        // Update the module name if it matches an (upgradable) legacy module
        static size_t legacyModuleCount = AZ_ARRAY_SIZE(legacyModules);
        AZ_Assert(legacyModuleCount == AZ_ARRAY_SIZE(legacyUpgradeModules), "Legacy Module update list is not in sync.");

        for (size_t moduleIndex = 0; moduleIndex < legacyModuleCount; moduleIndex++)
        {
            const char* legacyModule = legacyModules[moduleIndex];
            if (preprocessedName.compare(legacyModule) == 0)
            {
                preprocessedName = legacyUpgradeModules[moduleIndex];
                break;
            }
        }

        AZ::OSString finalPath = prefix + preprocessedName;

        // Let the application process the path
        ComponentApplicationBus::Broadcast(&ComponentApplicationBus::Events::ResolveModulePath, finalPath);

        return finalPath;
    }

    //=========================================================================
    // LoadDynamicModule
    //=========================================================================
    ModuleManager::LoadModuleOutcome ModuleManager::LoadDynamicModule(const char* modulePath, ModuleInitializationSteps lastStepToPerform, bool maintainReference)
    {
        // Check that module isn't already loaded
        AZ::OSString preprocessedModulePath = PreProcessModule(AZ::OSString(modulePath));

        // This pointer will be populated either by creating a new instance, or pulling from an old one
        AZStd::shared_ptr<ModuleDataImpl> moduleDataPtr;

        // If the module is already known, capture a reference to it
        auto moduleIt = m_notOwnedModules.find(preprocessedModulePath);
        if (moduleIt != m_notOwnedModules.end())
        {
            AZ_Assert(!moduleIt->second.expired(), "Internal error: module should have been removed from weak modules list.");
            moduleDataPtr = moduleIt->second.lock();
        }
        else
        {
            // Create shared pointer, with a deleter that will remove the module reference from m_notOwnedModules
            moduleDataPtr = AZStd::shared_ptr<ModuleDataImpl>(
                aznew ModuleDataImpl(),
                [preprocessedModulePath](ModuleDataImpl* toDelete)
                {
                    using ModManIntReqBus = Internal::ModuleManagerInternalRequestBus;
                    ModManIntReqBus::Broadcast(&ModManIntReqBus::Events::ClearModuleReferences, preprocessedModulePath);
                    delete toDelete;
                }
            );

            // Create weak pointer to the module data
            m_notOwnedModules.emplace(preprocessedModulePath, moduleDataPtr);
        }

        // Cache an iterator to the module in the owned modules list. This will be used later to either:
        // 1. Remove the module if the load fails
        // 2. If the module was not found and it should be owned, add it.
        auto ownedModuleIt = AZStd::find(m_ownedModules.begin(), m_ownedModules.end(), moduleDataPtr);

        // If we need to hold a reference and we don't already have one, save it
        if (maintainReference && ownedModuleIt == m_ownedModules.end())
        {
            m_ownedModules.emplace_back(moduleDataPtr);
            ownedModuleIt = AZStd::prior(m_ownedModules.end());
        }

        // If the module is already "loaded enough," just return it
        if (moduleDataPtr->m_lastCompletedStep >= lastStepToPerform)
        {
            return AZ::Success(AZStd::static_pointer_cast<ModuleData>(moduleDataPtr));
        }

        using PhaseOutcome = AZ::Outcome<void, AZStd::string>;
        // This will serve as the list of steps to perform when loading a module
        AZStd::vector<AZStd::pair<ModuleInitializationSteps, AZStd::function<PhaseOutcome()>>> phases = {
            // N/A
            {
                ModuleInitializationSteps::None,
                []() -> PhaseOutcome
                {
                    return AZ::Success();
                }
            },

            // Load the dynamic module
            {
                ModuleInitializationSteps::Load,
                [&moduleDataPtr, &preprocessedModulePath]() -> PhaseOutcome
                {
                    // Create handle
                    moduleDataPtr->m_dynamicHandle = DynamicModuleHandle::Create(preprocessedModulePath.c_str());
                    if (!moduleDataPtr->m_dynamicHandle)
                    {
                        return AZ::Failure(AZStd::string::format("Failed to create AZ::DynamicModuleHandle at path \"%s\".", preprocessedModulePath.c_str()));
                    }

                    // Load DLL from disk
                    if (!moduleDataPtr->m_dynamicHandle->Load(true))
                    {
                        return AZ::Failure(AZStd::string::format("Failed to load dynamic library at path \"%s\".",
                            moduleDataPtr->m_dynamicHandle->GetFilename().c_str()));
                    }

                    return AZ::Success();
                }
            },

            // Create the module class
            {
                ModuleInitializationSteps::CreateClass,
                [&moduleDataPtr]() -> PhaseOutcome
                {
                    const char* moduleName = moduleDataPtr->GetDebugName();
                    // Find function that creates AZ::Module class.
                    // It's acceptable for a library not to have this function.
                    auto createModuleFunction = moduleDataPtr->m_dynamicHandle->GetFunction<CreateModuleClassFunction>(CreateModuleClassFunctionName);
                    if (!createModuleFunction)
                    {
                        // It's an error if the library is missing a CreateModuleClass() function.
                        return AZ::Failure(AZStd::string::format("'%s' entry point in module \"%s\" failed to create AZ::Module.",
                            CreateModuleClassFunctionName,
                            moduleName));
                    }

                    // Create AZ::Module class
                    moduleDataPtr->m_module = createModuleFunction();
                    if (!moduleDataPtr->m_module)
                    {
                        // It's an error if the library has a CreateModuleClass() function that returns nothing.
                        return AZ::Failure(AZStd::string::format("'%s' entry point in module \"%s\" failed to create AZ::Module.",
                            CreateModuleClassFunctionName,
                            moduleName));
                    }

                    return AZ::Success();
                }
            },

            // Register the module descriptors
            {
                ModuleInitializationSteps::RegisterComponentDescriptors,
                [&moduleDataPtr]() -> PhaseOutcome
                {
                    moduleDataPtr->m_module->RegisterComponentDescriptors();
                    return AZ::Success();
                }
            },

            // Activate the module's entity
            {
                ModuleInitializationSteps::ActivateEntity,
                [this, &moduleDataPtr]() -> PhaseOutcome
                {
                    Entity* systemEntity = nullptr;
                    ComponentApplicationBus::BroadcastResult(systemEntity, &ComponentApplicationBus::Events::FindEntity, SystemEntityId);

                    // Only initialize components if the system entity is up and activated. Otherwise, it'll be inited later.
                    if (systemEntity && systemEntity->GetState() == Entity::ES_ACTIVE)
                    {
                        // Initialize the entity (explicitly do not sort, because the dependencies are probably already met.
                        AZStd::vector<AZStd::shared_ptr<ModuleDataImpl>> modulesToInit = { moduleDataPtr };
                        ActivateEntities(modulesToInit);
                    }

                    return AZ::Success();
                }
            },
        };

        // Iterate over each pair and execute it
        for (const auto& phasePair : phases)
        {
            // If this step is already complete, skip it
            if (moduleDataPtr->m_lastCompletedStep >= phasePair.first)
            {
                continue;
            }

            PhaseOutcome phaseResult = phasePair.second();
            if (!phaseResult.IsSuccess())
            {
                // Remove all references to the module from the owned and unowned list
                if (ownedModuleIt != m_ownedModules.end())
                {
                    m_ownedModules.erase(ownedModuleIt);
                }
                moduleDataPtr.reset();
                m_notOwnedModules.erase(preprocessedModulePath);

                return AZ::Failure(AZStd::move(phaseResult.GetError()));
            }

            // Update the state of the module
            moduleDataPtr->m_lastCompletedStep = phasePair.first;

            // If this was the last step to perform, then we're done.
            if (phasePair.first == lastStepToPerform)
            {
                break;
            }
        }

        return AZ::Success(AZStd::static_pointer_cast<ModuleData>(moduleDataPtr));
    }

    //=========================================================================
    // LoadDynamicModules
    //=========================================================================
    ModuleManager::LoadModulesResult ModuleManager::LoadDynamicModules(const ModuleDescriptorList& modules, ModuleInitializationSteps lastStepToPerform, bool maintainReferences)
    {
        LoadModulesResult results;

        // Load DLLs specified in the application descriptor
        for (const auto& moduleDescriptor : modules)
        {
            LoadModuleOutcome result = LoadDynamicModule(moduleDescriptor.m_dynamicLibraryPath.c_str(), lastStepToPerform, maintainReferences);
            results.emplace_back(AZStd::move(result));
        }

        return results;
    }

    //=========================================================================
    // LoadStaticModules
    //=========================================================================
    ModuleManager::LoadModulesResult ModuleManager::LoadStaticModules(CreateStaticModulesCallback staticModulesCb, ModuleInitializationSteps lastStepToPerform)
    {
        ModuleManager::LoadModulesResult results;

        // Load static modules
        if (staticModulesCb)
        {
            // Get the System Entity
            Entity* systemEntity = nullptr;
            ComponentApplicationBus::BroadcastResult(systemEntity, &ComponentApplicationBus::Events::FindEntity, SystemEntityId);

            AZStd::vector<Module*> staticModules;
            staticModulesCb(staticModules);

            for (Module* module : staticModules)
            {
                if (!module)
                {
                    AZ_Warning(s_moduleLoggingScope, false, "Nullptr module somehow inserted during call to static module creation function. Ignoring...");
                    continue;
                }

                auto moduleData = AZStd::make_shared<ModuleDataImpl>();
                moduleData->m_module = module;
                moduleData->m_lastCompletedStep = ModuleInitializationSteps::CreateClass;

                if (lastStepToPerform >= ModuleInitializationSteps::RegisterComponentDescriptors)
                {
                    moduleData->m_module->RegisterComponentDescriptors();
                    moduleData->m_lastCompletedStep = ModuleInitializationSteps::RegisterComponentDescriptors;
                }

                if (lastStepToPerform >= ModuleInitializationSteps::ActivateEntity)
                {
                    // Only initialize components if the system entity is up and activated. Otherwise, it'll be inited later.
                    if (systemEntity && systemEntity->GetState() == Entity::ES_ACTIVE)
                    {
                        // Initialize the entity (explicitly do not sort, because the dependencies are probably already met.
                        AZStd::vector<AZStd::shared_ptr<ModuleDataImpl>> modulesToInit = { moduleData };
                        ActivateEntities(modulesToInit);
                    }

                    moduleData->m_lastCompletedStep = ModuleInitializationSteps::ActivateEntity;
                }

                // Success! Move ModuleData into a permanent location
                m_ownedModules.emplace_back(moduleData);

                // Store the result to be returned
                results.emplace_back(AZ::Success(AZStd::static_pointer_cast<ModuleData>(moduleData)));
            }
        }

        return results;
    }

    //=========================================================================
    // IsModuleLoaded
    //=========================================================================
    bool ModuleManager::IsModuleLoaded(const char* modulePath)
    {
        AZ::OSString preprocessedModulePath = PreProcessModule(AZ::OSString(modulePath));
        return m_notOwnedModules.find(preprocessedModulePath) != m_notOwnedModules.end();
    }

    //=========================================================================
    // ClearModuleReferences
    //=========================================================================
    void ModuleManager::ClearModuleReferences(const AZ::OSString & normalizedModulePath)
    {
        auto moduleIt = m_notOwnedModules.find(normalizedModulePath);
        AZ_Assert(moduleIt != m_notOwnedModules.end(), "Internal error: Removing already removed module.");
        m_notOwnedModules.erase(moduleIt);
    }

    //=========================================================================
    // OnEntityActivated
    //=========================================================================
    void ModuleManager::OnEntityActivated(const AZ::EntityId& entityId)
    {
        AZ_Assert(entityId == SystemEntityId, "OnEntityActivated called for wrong entity id (expected SystemEntityId)!");
        EntityBus::Handler::BusDisconnect(entityId);

        ActivateEntities(m_ownedModules);
    }

    //=========================================================================
    // ActivateEntities
    //=========================================================================
    void ModuleManager::ActivateEntities(const AZStd::vector<AZStd::shared_ptr<ModuleDataImpl>>& modulesToInit)
    {
        // Grab the system entity
        Entity* systemEntity = nullptr;
        ComponentApplicationBus::BroadcastResult(systemEntity, &ComponentApplicationBus::Events::FindEntity, SystemEntityId);

        SerializeContext* serialize = nullptr;
        ComponentApplicationBus::BroadcastResult(serialize, &ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serialize, "Internal error: serialize context not found");

        // Will store components that need activation
        Entity::ComponentArrayType componentsToActivate;

        // Init all modules
        for (auto& moduleData : modulesToInit)
        {
            const char* moduleName = moduleData->GetDebugName();

            // Create module entity if it wasn't deserialized
            if (!moduleData->m_moduleEntity)
            {
                moduleData->m_moduleEntity = AZStd::make_unique<ModuleEntity>(azrtti_typeid(moduleData->m_module), moduleName);
            }

            // Default to the Required System Components List
            ComponentTypeList requiredComponents = moduleData->m_module->GetRequiredSystemComponents();

            // Sort through components registered, pull ones required in this context
            for (const auto& descriptor : moduleData->m_module->GetComponentDescriptors())
            {
                if (ShouldUseSystemComponent(*descriptor, m_systemComponentTags, *serialize))
                {
                    requiredComponents.emplace_back(descriptor->GetUuid());
                }
            }

            // Populate with required system components
            for (const Uuid& componentTypeId : requiredComponents)
            {
                // Add the component to the entity (if it's not already there OR on the System Entity)
                Component* component = moduleData->m_moduleEntity->FindComponent(componentTypeId);
                if (!component && !(systemEntity && systemEntity->FindComponent(componentTypeId)))
                {
                    component = moduleData->m_moduleEntity->CreateComponent(componentTypeId);
                    AZ_Warning(s_moduleLoggingScope, component != nullptr, "Failed to find Required System Component of type %s in module %s. Did you register the descriptor?", componentTypeId.ToString<AZStd::string>().c_str(), moduleName);
                }

                // This can be nullptr if the component was on the System Entity or if the component was somehow not found
                if (component)
                {
                    // Store in list to topo sort later
                    componentsToActivate.emplace_back(component);
                }
            }

            // Init the module entity, set it to Activating state
            moduleData->m_moduleEntity->Init();
            // Remove from Component Application so that we can delete it, not the app. It gets added during Init()
            ComponentApplicationBus::Broadcast(&ComponentApplicationBus::Events::RemoveEntity, moduleData->m_moduleEntity.get());
            // Set the entity to think it's activating
            moduleData->m_moduleEntity->SetState(Entity::ES_ACTIVATING);
        }

        // Add all components that are currently activated so that dependencies may be fulfilled
        componentsToActivate.insert(componentsToActivate.begin(), m_systemComponents.begin(), m_systemComponents.end());

        // Get all the components from the System Entity, to include for sorting purposes
        // This is so that components in modules may have dependencies on components on the system entity
        if (systemEntity)
        {
            const Entity::ComponentArrayType& systemEntityComponents = systemEntity->GetComponents();
            componentsToActivate.insert(componentsToActivate.begin(), systemEntityComponents.begin(), systemEntityComponents.end());
        }

        // Topo sort components, activate them
        Entity::DependencySortResult result = ModuleEntity::DependencySort(componentsToActivate);
        switch (result)
        {
        case Entity::DSR_MISSING_REQUIRED:
            AZ_Error(s_moduleLoggingScope, false, "Module Entities have missing required services and cannot be activated.");
            return;
        case Entity::DSR_CYCLIC_DEPENDENCY:
            AZ_Error(s_moduleLoggingScope, false, "Module Entities' components order have cyclic dependency and cannot be activated.");
            return;
        case Entity::DSR_OK:
            break;
        }

        for (auto componentIt = componentsToActivate.begin(); componentIt != componentsToActivate.end(); )
        {
            Component* component = *componentIt;

            // Remove the system entity and already activated components, we don't need to activate or store those
            if (component->GetEntityId() == SystemEntityId ||
                AZStd::find(m_systemComponents.begin(), m_systemComponents.end(), component) != m_systemComponents.end())
            {
                componentIt = componentsToActivate.erase(componentIt);
            }
            else
            {
                ++componentIt;
            }
        }

        // Activate the entities in the appropriate order
        for (Component* component : componentsToActivate)
        {
            ModuleEntity::ActivateComponent(*component);
        }

        // Done activating; set state to active
        for (auto& moduleData : modulesToInit)
        {
            if (moduleData->m_moduleEntity)
            {
                moduleData->m_moduleEntity->SetState(Entity::ES_ACTIVE);
            }
            moduleData->m_lastCompletedStep = ModuleInitializationSteps::ActivateEntity;
        }

        // Save the activated components for deactivation later
        m_systemComponents.insert(m_systemComponents.end(), componentsToActivate.begin(), componentsToActivate.end());
    }
} // namespace AZ
#endif // #ifndef AZ_UNITY_BUILD
