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
#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Module/ModuleManagerBus.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>

namespace AZ
{
    namespace Internal
    {
        /**
         * Internal bus for the ModuleManager to communicate with itself.
         */
        class ModuleManagerInternalRequests
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            //////////////////////////////////////////////////////////////////////////

            /**
             * Event for shared pointers' deleters to call to unload the module.
             * This is an ebus event (instead of a raw access) in case a module shared pointer outlives the ModuleManager.
             */
            virtual void ClearModuleReferences(const AZ::OSString& normalizedModulePath) = 0;
        };
        using ModuleManagerInternalRequestBus = AZ::EBus<ModuleManagerInternalRequests>;
    }

    /**
     * Checks if a component should be used as a System Component in a given context.
     *
     * \param descriptor    The descriptor for the component being checked
     * \param requiredTags  The tags the component must have one of for true to be returned
     * \param serialize     The serialize context to pull attributes for the component from
     *
     * \returns whether the component represented by descriptor has at least one tag that is present in required tags
     */
    bool ShouldUseSystemComponent(const ComponentDescriptor& descriptor, const AZStd::vector<Crc32>& requiredTags, const SerializeContext& serialize);

    /**
     * ModuleEntity is an Entity that carries a module class id along with it.
     * This we do so that when the System Entity Editor saves out an entity,
     * when it's loaded from the stream, we can use that id to associate the
     * entity with the appropriate module.
     */
    class ModuleEntity
        : public Entity
    {
        friend class ModuleManager;
    public:
        AZ_CLASS_ALLOCATOR(ModuleEntity, SystemAllocator, 0)
        AZ_RTTI(ModuleEntity, "{C5950488-35E0-4B55-B664-29A691A6482F}", Entity);

        static void Reflect(ReflectContext* context);

        ModuleEntity() = default;
        ModuleEntity(const AZ::Uuid& moduleClassId, const char* name = nullptr)
            : Entity(name)
            , m_moduleClassId(moduleClassId)
        { }

        // The typeof of the module class associated with this module,
        // so it can be associated with the correct module after loading
        AZ::Uuid m_moduleClassId;

    protected:
        // Allow manual setting of state
        void SetState(Entity::State state);
    };

    /**
     * Contains a static or dynamic AZ::Module.
     */
    struct ModuleDataImpl
        : public ModuleData
    {
        AZ_CLASS_ALLOCATOR(ModuleDataImpl, SystemAllocator, 0);

        ModuleDataImpl() = default;
        ~ModuleDataImpl() override;

        // no copy/move allowed
        ModuleDataImpl(const ModuleDataImpl&) = delete;
        ModuleDataImpl& operator=(const ModuleDataImpl&) = delete;
        ModuleDataImpl(ModuleDataImpl&& rhs) = delete;
        ModuleDataImpl& operator=(ModuleDataImpl&&) = delete;

        ////////////////////////////////////////////////////////////////////////
        // IModuleData
        DynamicModuleHandle* GetDynamicModuleHandle() const override { return m_dynamicHandle.get(); }
        Module* GetModule() const override { return m_module; }
        Entity* GetEntity() const override { return m_moduleEntity.get(); }
        const char* GetDebugName() const override;
        ////////////////////////////////////////////////////////////////////////

        /// Deals with loading and unloading the AZ::Module's DLL.
        /// This is null when the AZ::Module comes from a static LIB.
        AZStd::unique_ptr<DynamicModuleHandle> m_dynamicHandle;

        /// Handle to the module class within the module
        Module* m_module = nullptr;

        //! Entity that holds this module's provided system components
        AZStd::unique_ptr<ModuleEntity> m_moduleEntity;

        //! The last step this module completed
        ModuleInitializationSteps m_lastCompletedStep = ModuleInitializationSteps::None;
    };

    /*!
     * Handles reloading modules and their dependents at runtime
     */
    class ModuleManager
        : protected ModuleManagerRequestBus::Handler
        , protected EntityBus::Handler
        , protected Internal::ModuleManagerInternalRequestBus::Handler
    {
    public:
        static void Reflect(ReflectContext* context);

        ModuleManager();
        ~ModuleManager() override;

        // Destroy and unload all modules
        void UnloadModules();

        // To be called by the Component Application when it deserializes an entity
        void AddModuleEntity(ModuleEntity* moduleEntity);

        // To be called by the Component Application on startup
        template <typename CrcCollection>
        void SetSystemComponentTags(const CrcCollection& tags)
        {
            m_systemComponentTags.insert(m_systemComponentTags.end(), tags.begin(), tags.end());
        }

    protected:
        ////////////////////////////////////////////////////////////////////////
        // ModuleManagerRequestBus
        void EnumerateModules(EnumerateModulesCallback perModuleCallback) override;
        LoadModuleOutcome LoadDynamicModule(const char* modulePath, ModuleInitializationSteps lastStepToPerform, bool maintainReference) override;
        LoadModulesResult LoadDynamicModules(const ModuleDescriptorList& modules, ModuleInitializationSteps lastStepToPerform, bool maintainReferences) override;
        LoadModulesResult LoadStaticModules(CreateStaticModulesCallback staticModulesCb, ModuleInitializationSteps lastStepToPerform) override;
        bool IsModuleLoaded(const char* modulePath) override;

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ModuleManagerInternalRequestBus
        void ClearModuleReferences(const AZ::OSString& normalizedModulePath) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EntityBus
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        ////////////////////////////////////////////////////////////////////////

        // Helper function for initializing module entities
        void ActivateEntities(const AZStd::vector<AZStd::shared_ptr<ModuleDataImpl>>& modulesToInit);

        // Helper function to preprocess the module names to handle any special processing
        static AZ::OSString PreProcessModule(const AZ::OSString& moduleName);

        // Tags to look for when activating system components
        AZStd::vector<Crc32> m_systemComponentTags;

        /// System components we own and are responsible for shutting down
        AZ::Entity::ComponentArrayType m_systemComponents;

        /// The modules we own
        AZStd::vector<AZStd::shared_ptr<ModuleDataImpl>> m_ownedModules;

        using UnownedModulesMap = AZStd::unordered_map<AZ::OSString, AZStd::weak_ptr<ModuleDataImpl>>;
        /// The modules we don't own
        /// Is multimap to handle CRC collisions
        UnownedModulesMap m_notOwnedModules;
    };
} // namespace AZ
