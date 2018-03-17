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

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>

#include <AzCore/Casting/lossy_cast.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Math/Crc.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Platform/Platform.h>

#include <AzCore/Debug/Profiler.h>


#if defined(AZ_PLATFORM_WINDOWS)
#   include <AzCore/IPC/SharedMemory.h>
#   include <AzCore/PlatformIncl.h>
#else
#   include <AzCore/std/parallel/spin_mutex.h>
#   include <AzCore/std/parallel/lock.h>
#endif // AZ_PLATFORM_WINDOWS

namespace AZ
{
    //////////////////////////////////////////////////////////////////////////
    // Globals
#if defined(AZ_PLATFORM_WINDOWS)
    /**
     * TODO: We can store the last ID generated on this computer into the registry
     * (starting for the first time stamp), this way not waits, etc. will be needed.
     * We are NOT doing this, because need of unique entity ID might be gone soon.
     */
    class TimeIntervalStamper
    {
    public:
        TimeIntervalStamper(AZ::u64 timeInterval)
            : m_time(nullptr)
            , m_timeInterval(timeInterval)
        {
            AZ_Assert(m_timeInterval > 0, "You can't have 0 time interval!");
            SharedMemory::CreateResult createResult = m_sharedData.Create("AZCoreIdStorage", sizeof(AZ::u64), true);
            (void)createResult;
            AZ_Assert(createResult != SharedMemory::CreateFailed, "Failed to create shared memory block for Id's!");
            bool isReady = m_sharedData.Map();
            (void)isReady;
            AZ_Assert(isReady, "Failed to map the shared memory data!");
            m_time = reinterpret_cast<AZ::u64*>(m_sharedData.Data());
        }

        ~TimeIntervalStamper()
        {
            // make sure we wait for time generated it time period, otherwise if
            // we restart instantly we can generate the same ID.
            AZ::u64 intervalUTC = AZStd::GetTimeUTCMilliSecond() / m_timeInterval;
            lock();
            AZ::u64 lastStamp = *m_time;
            unlock();
            if (intervalUTC < lastStamp)
            {
                // wait for the time
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds((lastStamp - intervalUTC) * m_timeInterval));
            }
        }

        //////////////////////////////////////////////////////////////////////////
        /// Naming is conforming with AZStd::lock_guard/unique_lock/etc.
        void    lock()
        {
            m_sharedData.lock();
            if (m_sharedData.IsLockAbandoned())
            {
                *m_time = 0; // reset the time
            }
        }
        bool    try_lock()
        {
            bool isLocked = m_sharedData.try_lock();
            if (isLocked && m_sharedData.IsLockAbandoned())
            {
                *m_time = 0; // reset the time
            }
            return m_sharedData.try_lock();
        }
        void    unlock()                    { m_sharedData.unlock(); }
        //////////////////////////////////////////////////////////////////////////

        AZ::u64  GetTimeStamp()
        {
            AZ::u64 utcTime = AZStd::GetTimeUTCMilliSecond();
            AZ::u64 intervalUTC = utcTime / m_timeInterval;
            if (intervalUTC <= *m_time) // if we generated more than one ID for the current time.
            {
                *m_time += 1; // get next interval
            }
            else
            {
                *m_time = intervalUTC;
            }

            return *m_time;
        }
    private:
        SharedMemory        m_sharedData;
        AZ::u64*            m_time;
        AZ::u64             m_timeInterval; ///< Interval in milliseconds in which we store a time stamp
    };
#else // !AZ_PLATFORM_WINDOWS
    static const char* kTimeVariableName = "TimeIntervalStamp";
    static AZ::EnvironmentVariable<AZ::u64> g_sharedTimeStorage;
    
    class TimeIntervalStamper
    {
    public:
        TimeIntervalStamper(AZ::u64 timeInterval)
            : m_time(nullptr)
            , m_timeInterval(timeInterval)
        {
            AZ_Assert(m_timeInterval > 0, "You can't have 0 time interval!");
            
            const bool isConstruct = true;
            const bool isTransferOwnership = false;

            g_sharedTimeStorage = Environment::CreateVariableEx<AZ::u64>(kTimeVariableName, isConstruct, isTransferOwnership);
            m_time = &g_sharedTimeStorage.Get();

            if (g_sharedTimeStorage.IsOwner())
            {
                *m_time = 0;
            }

        }

        ~TimeIntervalStamper()
        {
            // make sure we wait for time generated it time period, otherwise if
            // we restart instantly we can generate the same ID.
            AZ::u64 intervalUTC = AZStd::GetTimeUTCMilliSecond() / m_timeInterval;
            lock();
            AZ::u64 lastStamp = *m_time;
            unlock();
            if (intervalUTC < lastStamp)
            {
                // wait for the time
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds((lastStamp - intervalUTC) * m_timeInterval));
            }

            g_sharedTimeStorage = nullptr;
            m_time = nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        /// Naming is conforming with AZStd::lock_guard/unique_lock/etc.
        void    lock()                      { m_mutex.lock(); }
        bool    try_lock()                  { return m_mutex.try_lock(); }
        void    unlock()                    { m_mutex.unlock(); }
        //////////////////////////////////////////////////////////////////////////

        AZ::u64  GetTimeStamp()
        {
            AZ::u64 utcTime = AZStd::GetTimeUTCMilliSecond();
            AZ::u64 intervalUTC = utcTime / m_timeInterval;
            if (intervalUTC <= *m_time) // if we generated more than one ID for the current time.
            {
                *m_time += 1; // get next interval
            }
            else
            {
                *m_time = intervalUTC;
            }

            return *m_time;
        }

    private:
        AZStd::spin_mutex       m_mutex;
        AZ::u64*                m_time;
        AZ::u64                 m_timeInterval; ///< Interval in milliseconds in which we store a time stamp
    };
#endif // !AZ_PLATFORM_WINDOWS

    class SerializeEntityFactory
        : public SerializeContext::IObjectFactory
    {
        void* Create(const char* name) override
        {
            (void)name;
            // Init with invalid entity as the serializer will load the values into them,
            // otherwise the user will have to set them.
            return aznewex(name) Entity(EntityId());
        }
        void Destroy(void* ptr) override
        {
            delete reinterpret_cast<Entity*>(ptr);
        }
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    class BehaviorEntityBusHandler : public EntityBus::Handler, public BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorEntityBusHandler, "{8DAE4CBE-BF6C-4469-9A9E-47E7DB9E21E3}", AZ::SystemAllocator, OnEntityActivated, OnEntityDeactivated);

        void OnEntityActivated(const AZ::EntityId& id) override
        {
            Call(FN_OnEntityActivated, id);
        }

        void OnEntityDeactivated(const AZ::EntityId& id) override
        {
            Call(FN_OnEntityDeactivated, id);
        }
    };

    //=========================================================================
    // Entity
    // [5/31/2012]
    //=========================================================================
    Entity::Entity(const char* name)
        : m_state(ES_CONSTRUCTED)
        , m_transform(nullptr)
        , m_isDependencyReady(false)
        , m_isRuntimeActiveByDefault(true)
    {
        m_id = MakeId();
        if (name)
        {
            m_name = name;
        }
        else
        {
            to_string(m_name, static_cast<u64>(m_id));
        }
    }

    //=========================================================================
    // Entity
    // [5/30/2012]
    //=========================================================================
    Entity::Entity(const EntityId& id, const char* name)
        : m_id(id)
        , m_state(ES_CONSTRUCTED)
        , m_transform(nullptr)
        , m_isDependencyReady(false)
        , m_isRuntimeActiveByDefault(true)
    {
        if (name)
        {
            m_name = name;
        }
        else
        {
            to_string(m_name, static_cast<u64>(m_id));
        }
    }

    //=========================================================================
    // ~Entity
    // [5/30/2012]
    //=========================================================================
    Entity::~Entity()
    {
        AZ_Assert(m_state != ES_ACTIVATING && m_state != ES_DEACTIVATING && m_state != ES_INITIALIZING, "Unsafe to delete an entity during its state transition.");
        if (m_state == ES_ACTIVE)
        {
            Deactivate();
        }

        if (m_state == ES_INIT)
        {
            EBUS_EVENT(EntitySystemBus, OnEntityDestruction, m_id);
            EBUS_EVENT_ID(m_id, EntityBus, OnEntityDestruction, m_id);

            EBUS_EVENT(ComponentApplicationBus, RemoveEntity, this);
        }

        for (ComponentArrayType::reverse_iterator it = m_components.rbegin(); it != m_components.rend(); ++it)
        {
            delete *it;
        }

        if (m_state == ES_INIT)
        {
            EBUS_EVENT(EntitySystemBus, OnEntityDestroyed, m_id);
            EBUS_EVENT_ID(m_id, EntityBus, OnEntityDestroyed, m_id);
        }
    }

    //=========================================================================
    // SetId
    // [3/11/2014]
    //=========================================================================
    void Entity::SetId(const EntityId& source)
    {
        AZ_Assert(source != SystemEntityId, "You may not set the ID of an entity to the system entity ID.");
        if (source == SystemEntityId)
        {
            return;
        }

        AZ_Assert(m_state == ES_CONSTRUCTED, "You may not alter the ID of an entity when it is active or initialized");

        if (m_state != ES_CONSTRUCTED)
        {
            return;
        }

        m_id = source;
    }

    //=========================================================================
    // Init
    // [5/30/2012]
    //=========================================================================
    void Entity::Init()
    {
        AZ_Assert(m_state == ES_CONSTRUCTED, "Component should be in Constructed state to be Initialized!");
        m_state = ES_INITIALIZING;

        bool result = true;
        EBUS_EVENT_RESULT(result, ComponentApplicationBus, AddEntity, this);
        (void)result;
        AZ_Assert(result, "Failed to add entity '%s' [0x%llx]! Did you already register an entity with this ID?", m_name.c_str(), m_id);

        for (ComponentArrayType::iterator it = m_components.begin(); it != m_components.end();)
        {
            Component* component = *it;
            if (component)
            {
                component->SetEntity(this);
                component->Init();
                ++it;
            }
            else
            {
                it = m_components.erase(it);
            }
        }

        m_state = ES_INIT;

        EBUS_EVENT_ID(m_id, EntityBus, OnEntityExists, m_id);
        EBUS_EVENT(EntitySystemBus, OnEntityInitialized, m_id);
    }

    //=========================================================================
    // Activate
    // [5/30/2012]
    //=========================================================================
    void Entity::Activate()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

        AZ_Assert(m_state == ES_INIT, "Entity should be in Init state to be Activated!");

        const DependencySortResult dependencySortResult = EvaluateDependencies();
        switch (dependencySortResult)
        {
        case DSR_MISSING_REQUIRED:
            AZ_Error("Entity", false, "Entity '%s' [0x%llx] has missing required services and cannot be activated.\n", m_name.c_str(), m_id);
            return;
        case DSR_CYCLIC_DEPENDENCY:
            AZ_Error("Entity", false, "Entity '%s' [0x%llx] components order have cyclic dependency and cannot be activated.\n", m_name.c_str(), m_id);
            return;
        case DSR_OK:
            break;
        }

        m_state = ES_ACTIVATING;

        for (ComponentArrayType::iterator it = m_components.begin(); it != m_components.end(); ++it)
        {
            ActivateComponent(**it);
        }

        // Cache the transform interface to the transform interface
        // Generally this pattern is not recommended unless for component event buses
        // As we have a guarantee (by design) that components can't change during active state)
        // Even though technically they can connect disconnect from the bus.
        m_transform = TransformBus::FindFirstHandler(m_id);

        m_state = ES_ACTIVE;

        EBUS_EVENT_ID(m_id, EntityBus, OnEntityActivated, m_id);
        EBUS_EVENT(EntitySystemBus, OnEntityActivated, m_id);
    }

    //=========================================================================
    // Deactivate
    // [5/30/2012]
    //=========================================================================
    void Entity::Deactivate()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

        EBUS_EVENT_ID(m_id, EntityBus, OnEntityDeactivated, m_id);
        EBUS_EVENT(EntitySystemBus, OnEntityDeactivated, m_id);

        AZ_Assert(m_state == ES_ACTIVE, "Component should be in Active state to br Deactivated!");
        m_state = ES_DEACTIVATING;

        for (ComponentArrayType::reverse_iterator it = m_components.rbegin(); it != m_components.rend(); ++it)
        {
            DeactivateComponent(**it);
        }

        m_transform = nullptr;
        m_state = ES_INIT;
    }

    //=========================================================================
    // EvaluateDependencies
    //=========================================================================
    Entity::DependencySortResult Entity::EvaluateDependencies()
    {
        DependencySortResult result = DSR_OK;

        if (!m_isDependencyReady)
        {
            result = DependencySort(m_components);
            m_isDependencyReady = (result == DSR_OK);
        }

        return result;
    }

    //=========================================================================
    // CreateComponent
    //=========================================================================
    void Entity::InvalidateDependencies()
    {
        m_isDependencyReady = false;
    }

    //=========================================================================
    // SetRuntimeActiveByDefault
    //=========================================================================
    void Entity::SetRuntimeActiveByDefault(bool activeByDefault)
    {
        m_isRuntimeActiveByDefault = activeByDefault;
    }

    //=========================================================================
    // IsRuntimeActiveByDefault
    //=========================================================================
    bool Entity::IsRuntimeActiveByDefault() const
    {
        return m_isRuntimeActiveByDefault;
    }

    //=========================================================================
    // CreateComponent
    //=========================================================================
    Component* Entity::CreateComponent(const Uuid& componentTypeId)
    {
        Component* component = nullptr;
        EBUS_EVENT_ID_RESULT(component, componentTypeId, ComponentDescriptorBus, CreateComponent);
        if (component)
        {
            if (!AddComponent(component))
            {
                delete component;
                component = nullptr;
            }
        }
#ifdef AZ_ENABLE_TRACING
        else
        {
            const char* name = nullptr;
            EBUS_EVENT_ID_RESULT(name, componentTypeId, ComponentDescriptorBus, GetName);
            AZ_Assert(false, "Failed to create component: %s", (name ? name : componentTypeId.ToString<AZStd::string>().c_str()));
        }
#endif // AZ_ENABLE_TRACING

        return component;
    }

    //=========================================================================
    // CreateComponent
    //=========================================================================
    Component* Entity::CreateComponentIfReady(const Uuid& componentTypeId)
    {
        if (IsComponentReadyToAdd(componentTypeId))
        {
            return CreateComponent(componentTypeId);
        }

        return nullptr;
    }

    //=========================================================================
    // AddComponent
    // [5/30/2012]
    //=========================================================================
    bool Entity::AddComponent(Component* component)
    {
        AZ_Assert(component != nullptr, "Invalid component!");
        AZ_Assert(CanAddRemoveComponents(), "Can't add component while the entity is active!");
        AZ_Assert(component->GetEntity() == nullptr, "Component is already added to entity %p [0x%llx]", component->m_entity, component->m_entity->GetId());
        if (!CanAddRemoveComponents())
        {
            return false;
        }
        if (AZStd::find(m_components.begin(), m_components.end(), component) != m_components.end())
        {
            return false;
        }

        if (component->GetId() != InvalidComponentId)
        {
            // Ensure we do not already have a component with this id
            // if so, set invalid id and SetEntity will generate a new one
            if (FindComponent(component->GetId()))
            {
                component->m_id = InvalidComponentId;
            }
        }
        component->SetEntity(this);
        m_components.push_back(component);

        if (m_state == ES_INIT)
        {
            component->Init();
        }
        
        InvalidateDependencies(); // We need to re-evaluate dependencies
        return true;
    }

    //=========================================================================
    // IsComponentReadyToAdd
    // [3/13/2014]
    //=========================================================================
    bool Entity::IsComponentReadyToAdd(const Uuid& componentTypeId, const Component* instance, ComponentDescriptor::DependencyArrayType* servicesNeededToBeAdded, ComponentArrayType* incompatibleComponents)
    {
        bool isReadyToAdd = true;

        ComponentDescriptor::DependencyArrayType provided;
        ComponentDescriptor::DependencyArrayType incompatible;
        ComponentDescriptor* componentDescriptor = nullptr;
        EBUS_EVENT_ID_RESULT(componentDescriptor, componentTypeId, ComponentDescriptorBus, GetDescriptor);
        if (!componentDescriptor)
        {
            return false;
        }

        componentDescriptor->GetIncompatibleServices(incompatible, instance);
        // Check for incompatible components "component" is incompatible with.
        if (!incompatible.empty())
        {
            for (auto componentIt = m_components.begin(); componentIt != m_components.end(); ++componentIt)
            {
                provided.clear();

                ComponentDescriptor* subComponentDescriptor = nullptr;
                EBUS_EVENT_ID_RESULT(subComponentDescriptor, (*componentIt)->RTTI_GetType(), ComponentDescriptorBus, GetDescriptor);
                AZ_Assert(subComponentDescriptor, "Component class %s descriptor is not created! It must be before you can use it!", (*componentIt)->RTTI_GetTypeName());
                subComponentDescriptor->GetProvidedServices(provided, (*componentIt));
                for (auto providedIt = provided.begin(); providedIt != provided.end(); ++providedIt)
                {
                    for (auto incompatibleIt = incompatible.begin(); incompatibleIt != incompatible.end(); ++incompatibleIt)
                    {
                        if (*providedIt == *incompatibleIt)
                        {
                            isReadyToAdd = false;
                            if (incompatibleComponents)
                            {
                                incompatibleComponents->push_back(*componentIt);
                            }
                        }
                    }
                }
            }
        }

        // Check for components incompatible with "component"
        provided.clear();
        componentDescriptor->GetProvidedServices(provided, instance);
        if (!provided.empty())
        {
            for (auto componentIt = m_components.begin(); componentIt != m_components.end(); ++componentIt)
            {
                incompatible.clear();
                ComponentDescriptor* subComponentDescriptor = nullptr;
                EBUS_EVENT_ID_RESULT(subComponentDescriptor, (*componentIt)->RTTI_GetType(), ComponentDescriptorBus, GetDescriptor);
                AZ_Assert(subComponentDescriptor, "Component class %s descriptor is not created! It must be before you can use it!", (*componentIt)->RTTI_GetTypeName());
                subComponentDescriptor->GetIncompatibleServices(incompatible, (*componentIt));
                for (auto incompatibleIt = incompatible.begin(); incompatibleIt != incompatible.end(); ++incompatibleIt)
                {
                    for (auto providedIt = provided.begin(); providedIt != provided.end(); ++providedIt)
                    {
                        if (*providedIt == *incompatibleIt)
                        {
                            isReadyToAdd = false;
                            if (incompatibleComponents)
                            {
                                // make sure we don't double add components (if we are incompatible with "component" and "component" is not compatible with them
                                if (AZStd::find(incompatibleComponents->begin(), incompatibleComponents->end(), *componentIt) == incompatibleComponents->end())
                                {
                                    incompatibleComponents->push_back(*componentIt);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Check if all required components are present
        ComponentDescriptor::DependencyArrayType required;
        componentDescriptor->GetRequiredServices(required, instance);
        if (!required.empty())
        {
            for (auto componentIt = m_components.begin(); componentIt != m_components.end(); ++componentIt)
            {
                provided.clear();
                ComponentDescriptor* subComponentDescriptor = nullptr;
                EBUS_EVENT_ID_RESULT(subComponentDescriptor, (*componentIt)->RTTI_GetType(), ComponentDescriptorBus, GetDescriptor);
                AZ_Assert(subComponentDescriptor, "Component class %s descriptor is not created! It must be before you can use it!", (*componentIt)->RTTI_GetTypeName());
                subComponentDescriptor->GetProvidedServices(provided, (*componentIt));
                for (auto providedIt = provided.begin(); providedIt != provided.end(); ++providedIt)
                {
                    for (auto requiredIt = required.begin(); requiredIt != required.end(); )
                    {
                        if (*providedIt == *requiredIt)
                        {
                            requiredIt = required.erase(requiredIt);
                        }
                        else
                        {
                            ++requiredIt;
                        }
                    }
                }
            }

            if (servicesNeededToBeAdded)
            {
                *servicesNeededToBeAdded = required;
            }

            if (!required.empty())
            {
                isReadyToAdd = false;
            }
        }

        return isReadyToAdd;
    }

    //=========================================================================
    // RemoveComponent
    // [5/30/2012]
    //=========================================================================
    bool Entity::RemoveComponent(Component* component)
    {
        AZ_Assert(component != nullptr, "Invalid component!");
        AZ_Assert(CanAddRemoveComponents(), "Can't remove component while the entity is active!");
        // If the component doesn't have an entity, it hasn't been Init()'d yet, and should be fine to remove.
        AZ_Assert(!component->GetEntity() || component->GetEntity() == this, "Component doesn't belong to this entity %p [0x%llx] it belongs to %p [0x%llx]", this, m_id, component->m_entity, component->m_entity->GetId());
        if (!CanAddRemoveComponents())
        {
            return false;
        }
        ComponentArrayType::iterator it = AZStd::find(m_components.begin(), m_components.end(), component);
        if (it == m_components.end())
        {
            return false;
        }

        m_components.erase(it);
        component->SetEntity(nullptr);

        InvalidateDependencies(); // We need to re-evaluate dependencies.
        return true;
    }

    bool Entity::SwapComponents(Component* componentToRemove, Component* componentToAdd)
    {
        AZ_Assert(componentToRemove && componentToAdd, "Invalid component");
        AZ_Assert(CanAddRemoveComponents(), "Can't remove component while the entity is active!");
        AZ_Assert(!componentToRemove->GetEntity() || componentToRemove->GetEntity() == this, "Component doesn't belong to this entity %p [0x%llx] it belongs to %p [0x%llx]", this, m_id, componentToRemove->m_entity, componentToRemove->m_entity->GetId());
        AZ_Assert(componentToAdd->GetEntity() == nullptr, "Component already belongs to this entity %p [0x%llx]", componentToAdd->m_entity, componentToAdd->m_entity->GetId());

        if (!CanAddRemoveComponents())
        {
            return false;
        }

        // Swap components as seamlessly as possible.
        // Do not disturb the vector and use the same ComponentID.
        auto it = AZStd::find(m_components.begin(), m_components.end(), componentToRemove);
        if (it == m_components.end())
        {
            return false;
        }

        ComponentId componentId = componentToRemove->GetId();
        componentToRemove->SetEntity(nullptr);

        *it = componentToAdd;

        componentToAdd->m_id = componentId;
        componentToAdd->SetEntity(this);

        // Initialize component if necessary.
        if (m_state == ES_INIT)
        {
            componentToAdd->Init();
        }

        InvalidateDependencies(); // We need to re-evaluate dependencies
        return true;
    }

    //=========================================================================
    // IsComponentReadyToRemove
    // [3/13/2014]
    //=========================================================================
    bool Entity::IsComponentReadyToRemove(Component* component, ComponentArrayType* componentsNeededToBeRemoved)
    {
        AZ_Assert(component, "You need to provide a valid component!");
        AZ_Assert(component->GetEntity() == this, "Component belongs to a different entity!");
        ComponentDescriptor::DependencyArrayType provided;
        ComponentDescriptor* componentDescriptor = nullptr;
        EBUS_EVENT_ID_RESULT(componentDescriptor, component->RTTI_GetType(), ComponentDescriptorBus, GetDescriptor);
        AZ_Assert(componentDescriptor, "Component class %s descriptor is not created! It must be before you can use it!", component->RTTI_GetTypeName());
        componentDescriptor->GetProvidedServices(provided, component);
        if (!provided.empty())
        {
            // first remove all the services that other components beside us provide (normally this should be none and we can remove it in the future)
            for (auto componentIt = m_components.begin(); componentIt != m_components.end(); ++componentIt)
            {
                if (component == *componentIt)
                {
                    continue;
                }
                ComponentDescriptor::DependencyArrayType subProvided;
                ComponentDescriptor* subComponentDescriptor = nullptr;
                EBUS_EVENT_ID_RESULT(subComponentDescriptor, (*componentIt)->RTTI_GetType(), ComponentDescriptorBus, GetDescriptor);
                AZ_Assert(componentDescriptor, "Component class %s descriptor is not created! It must be before you can use it!", (*componentIt)->RTTI_GetTypeName());
                subComponentDescriptor->GetProvidedServices(subProvided, (*componentIt));
                for (auto subProvidedIt = subProvided.begin(); subProvidedIt != subProvided.end(); ++subProvidedIt)
                {
                    for (auto providedIt = provided.begin(); providedIt != provided.end(); )
                    {
                        if (*providedIt == *subProvidedIt)
                        {
                            providedIt = provided.erase(providedIt);
                        }
                        else
                        {
                            ++providedIt;
                        }
                    }
                }
            }

            // find all components that depend on our services
            bool hasDependents = false;
            if (componentsNeededToBeRemoved)
            {
                componentsNeededToBeRemoved->clear();
            }
            for (auto componentIt = m_components.begin(); componentIt != m_components.end(); ++componentIt)
            {
                if (component == *componentIt)
                {
                    continue;
                }
                ComponentDescriptor::DependencyArrayType required;
                ComponentDescriptor* subComponentDescriptor = nullptr;
                EBUS_EVENT_ID_RESULT(subComponentDescriptor, (*componentIt)->RTTI_GetType(), ComponentDescriptorBus, GetDescriptor);
                AZ_Assert(subComponentDescriptor, "Component class %s descriptor is not created! It must be before you can use it!", (*componentIt)->RTTI_GetTypeName());
                subComponentDescriptor->GetRequiredServices(required, (*componentIt));
                for (auto requiredIt = required.begin(); requiredIt != required.end(); ++requiredIt)
                {
                    if (AZStd::find(provided.begin(), provided.end(), *requiredIt) != provided.end())
                    {
                        if (componentsNeededToBeRemoved)
                        {
                            componentsNeededToBeRemoved->push_back(*componentIt);
                        }
                        hasDependents = true;
                        break;
                    }
                }
            }
            return !hasDependents;
        }

        return true;
    }

    //=========================================================================
    // FindComponent
    // [5/30/2012]
    //=========================================================================
    Component* Entity::FindComponent(ComponentId id) const
    {
        size_t numComponents = m_components.size();
        for (size_t i = 0; i < numComponents; ++i)
        {
            Component* component = m_components[i];
            if (component->GetId() == id)
            {
                return component;
            }
        }
        return nullptr;
    }

    //=========================================================================
    // FindComponent
    // [5/30/2012]
    //=========================================================================
    Component* Entity::FindComponent(const Uuid& type) const
    {
        size_t numComponents = m_components.size();
        for (size_t i = 0; i < numComponents; ++i)
        {
            Component* component = m_components[i];
            if (component->RTTI_GetType() == type)
            {
                return component;
            }
        }
        return nullptr;
    }

    Entity::ComponentArrayType Entity::FindComponents(const Uuid& typeId) const
    {
        ComponentArrayType components;
        size_t numComponents = m_components.size();
        for (size_t i = 0; i < numComponents; ++i)
        {
            Component* component = m_components[i];
            if (typeId == component->RTTI_GetType())
            {
                components.push_back(component);
            }
        }
        return components;
    }

    //=========================================================================
    // OnNameChanged
    //=========================================================================
    void Entity::OnNameChanged() const
    {
        EBUS_EVENT_ID(GetId(), EntityBus, OnEntityNameChanged, m_name);
        EBUS_EVENT(EntitySystemBus, OnEntityNameChanged, GetId(), m_name);
    }

    //=========================================================================
    // CanAddRemoveComponents
    //=========================================================================
    bool Entity::CanAddRemoveComponents() const
    {
        switch (m_state)
        {
        case ES_CONSTRUCTED:
        case ES_INIT:
            return true;
        default:
            return false;
        }
    }

    //=========================================================================
    // ConvertOldData
    // [5/30/2012]
    //=========================================================================
    bool ConvertOldData(SerializeContext& context, SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 2)
        {
            // Convert from version 0/1, where EntityId was just a flat u64, and is now a reflected type.

            // Class<Entity>()->
            //    Version(1)->
            //    ...
            //    Field("Id",&Entity::m_id) where m_id was u64
            //    ...
            for (int i = 0; i < classElement.GetNumSubElements(); ++i)
            {
                AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(i);
                if (elementNode.GetName() == AZ_CRC("Id", 0xbf396750))
                {
                    u64 oldEntityId;
                    if (elementNode.GetData(oldEntityId))
                    {
                        // remove old u64 m_id
                        classElement.RemoveElement(i);

                        // add new EntityId m_id
                        int entityIdIdx = classElement.AddElement<EntityId>(context, "Id");

                        // New Entity::m_id is has the structure
                        // Class<EntityId>()->
                        //   Version(1)->
                        //   Field("id", &EntityId::m_id);

                        if (entityIdIdx != -1)
                        {
                            AZ::SerializeContext::DataElementNode& entityIdNode = classElement.GetSubElement(entityIdIdx);
                            int idIndex = entityIdNode.AddElement<u64>(context, "id");
                            if (idIndex != -1)
                            {
                                entityIdNode.GetSubElement(idIndex).SetData(context, oldEntityId);
                                return true;
                            }
                        }
                    }
                    return false;
                }
            }
        }

        return true;
    }

    //=========================================================================
    // EntityIdConverter
    //=========================================================================
    bool EntityIdConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 0)
        {
            // Version 0 was EntityRef, so convert the old field name to the new one.
            for (int i = 0; i < classElement.GetNumSubElements(); ++i)
            {
                AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(i);
                if (elementNode.GetName() == AZ_CRC("m_refId", 0xb7853eda))
                {
                    u64 oldEntityId;
                    if (elementNode.GetData(oldEntityId))
                    {
                        // Remove the old m_refId field.
                        classElement.RemoveElement(i);

                        // Add the new field, which is just called "id", and copy data.
                        int entityIdIdx = classElement.AddElement<AZ::u64>(context, "id");
                        classElement.GetSubElement(entityIdIdx).SetData(context, oldEntityId);
                        return true;
                    }

                    return false;
                }
            }
        }

        return true;
    }

    //=========================================================================
    // Reflect
    // [5/30/2012]
    //=========================================================================
    void Entity::Reflect(ReflectContext* reflection)
    {
        Component::ReflectInternal(reflection); // reflect base component class

        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Entity>(&Serialize::StaticInstance<SerializeEntityFactory>::s_instance)
                ->PersistentId([](const void* instance) -> u64 { return static_cast<u64>(reinterpret_cast<const Entity*>(instance)->GetId()); })
                ->Version(2, &ConvertOldData)
                ->Field("Id", &Entity::m_id)
                    ->Attribute(Edit::Attributes::IdGeneratorFunction, &Entity::MakeId)
                ->Field("Name", &Entity::m_name)
                ->Field("Components", &Entity::m_components) // Component serialization can result in IsDependencyReady getting modified, so serialize Components first.
                ->Field("IsDependencyReady", &Entity::m_isDependencyReady)
                ->Field("IsRuntimeActive", &Entity::m_isRuntimeActiveByDefault)
                ;

            serializeContext->Class<EntityId>()
                ->Version(1, &EntityIdConverter)
                ->Field("id", &EntityId::m_id);

            EditContext* ec = serializeContext->GetEditContext();
            if (ec)
            {
                ec->Class<Entity>("Entity", "Base entity class")->
                    DataElement(AZ::Edit::UIHandlers::Default, &Entity::m_id, "Id", "")->
                        Attribute(Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)->
                        Attribute(Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::NotPushable)->
                    DataElement(AZ::Edit::UIHandlers::Default, &Entity::m_isDependencyReady, "IsDependencyReady", "")->
                        Attribute(Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)->
                        Attribute(Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::NotPushable)->
                    DataElement(AZ::Edit::UIHandlers::Default, &Entity::m_isRuntimeActiveByDefault, "StartActive", "")->
                    DataElement("String", &Entity::m_name, "Name", "Unique name of the entity")->
                        Attribute(Edit::Attributes::ChangeNotify, &Entity::OnNameChanged)->
                    DataElement("Components", &Entity::m_components, "Components", "");

                ec->Class<EntityId>("EntityId", "Entity Unique Id");
            }
        }

        BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<EntityId>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Method("IsValid", &EntityId::IsValid)
                ->Method("ToString", &EntityId::ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ->Method("Equal", &EntityId::operator==)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ;

            behaviorContext->Constant("SystemEntityId", BehaviorConstant(SystemEntityId));

            behaviorContext->EBus<EntityBus>("EntityBus")
                ->Handler<BehaviorEntityBusHandler>()
                ;

            behaviorContext->Class<ComponentConfig>();
        }
    }

    //=========================================================================
    // DependencySort
    // [8/31/2012]
    //=========================================================================
    Entity::DependencySortResult
    Entity::DependencySort(ComponentArrayType& components)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Sorting algorithm that organizes the components in topological order.
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        if (components.size() <= 1)
        {
            return DSR_OK;
        }

        // Graph structure for topological sort
        using GraphNode = AZStd::pair<Component*, int>;
        using GraphNodes = AZStd::list<GraphNode>;
        AZStd::vector<AZStd::pair<GraphNodes::iterator, GraphNodes::iterator>> graphLinks;
        graphLinks.reserve(128);
        GraphNodes graphNodes;
        AZStd::vector<Component*> consumerComponents; /// List of components where order doesn't matter and they should be placed at the end of the sort
        consumerComponents.reserve(32);

        // map all provided services to the components
        AZStd::unordered_map<ComponentServiceType, GraphNodes::iterator> providedServiceToComponentMap(37);
        ComponentDescriptor::DependencyArrayType services;
        for (Component* component : components)
        {
            ComponentDescriptor* componentDescriptor = nullptr;
            EBUS_EVENT_ID_RESULT(componentDescriptor, component->RTTI_GetType(), ComponentDescriptorBus, GetDescriptor);
            AZ_Assert(componentDescriptor, "Component class %s descriptor is not created! It must be before you can use it!", component->RTTI_GetTypeName());
            services.clear();
            componentDescriptor->GetProvidedServices(services, component);

            if (services.empty())
            {
                // this component only consumes services, no need to topo sort it
                consumerComponents.push_back(component);
            }
            else
            {
                graphNodes.push_front(GraphNode(component, 0));
                GraphNodes::iterator currentNode = graphNodes.begin();

                for (ComponentServiceType service : services)
                {
                    providedServiceToComponentMap.insert(AZStd::make_pair(service, currentNode));
                }
            }
        }

        components.clear(); // clear input in preparation for sorted list

        // build graph connections - build directional links between graph nodes based on dependencies (dependent
        // and required services), from current node/component toward dependent node/component.
        for (auto nodeIt = graphNodes.begin(); nodeIt != graphNodes.end(); ++nodeIt)
        {
            ComponentDescriptor* componentDescriptor = nullptr;
            EBUS_EVENT_ID_RESULT(componentDescriptor, nodeIt->first->RTTI_GetType(), ComponentDescriptorBus, GetDescriptor);
            AZ_Assert(componentDescriptor, "Component class %s descriptor is not created! It must be before you can use it!", nodeIt->first->RTTI_GetTypeName());

            // process dependent services
            services.clear();
            componentDescriptor->GetDependentServices(services, nodeIt->first);
            for (ComponentServiceType service : services)
            {
                auto mapIter = providedServiceToComponentMap.find(service);
                if (mapIter != providedServiceToComponentMap.end())
                {
                    ++nodeIt->second; // increment dependency counter
                    graphLinks.push_back(AZStd::make_pair(nodeIt, mapIter->second));
                }
                // it's ok if dependent service is not found. Dependent only indicates that if present it would be initialized before the component itself.
            }

            // process required services
            services.clear();
            componentDescriptor->GetRequiredServices(services, nodeIt->first);
            for (ComponentServiceType service : services)
            {
                auto mapIter = providedServiceToComponentMap.find(service);
                if (mapIter != providedServiceToComponentMap.end())
                {
                    ++nodeIt->second; // increment dependency counter
                    graphLinks.push_back(AZStd::make_pair(nodeIt, mapIter->second));
                }
                else
                {
                    AZ_Error("Entity", false, "Component %s is missing required service 0x%08x!\n", nodeIt->first->RTTI_GetTypeName(), service);
                    return DSR_MISSING_REQUIRED;
                }
            }
        }

        // topological sort
        for (auto graphNodeIt = graphNodes.begin(); graphNodeIt != graphNodes.end(); )
        {
            if (graphNodeIt->second == 0) // check if this node's dependencies are met
            {
                // remove links
                for (auto linkIt = graphLinks.begin(); linkIt != graphLinks.end(); )
                {
                    AZ_Assert(linkIt->first != graphNodeIt, "If the use count is 0, we should not have any 'exit' links!");
                    if (linkIt->second == graphNodeIt)
                    {
                        if (--linkIt->first->second == 0) // decrement dependency count
                        {
                            // this dependent node is ready for processing, move it after us for processing
                            graphNodes.splice(AZStd::next(graphNodeIt), graphNodes, linkIt->first);
                        }

                        linkIt = graphLinks.erase(linkIt);
                    }
                    else
                    {
                        ++linkIt;
                    }
                }

                // move the component to the sorted list and remove from the graph
                components.push_back(graphNodeIt->first);
                graphNodeIt = graphNodes.erase(graphNodeIt);
            }
            else
            {
                ++graphNodeIt;
            }
        }

        // Add consumer components
        components.insert(components.end(), consumerComponents.begin(), consumerComponents.end());

        // If we have any nodes left on the graph, they form a cyclic dependency.
        if (!graphNodes.empty())
        {
            AZ_Error("Entity", false, "The following components have a cyclic dependency:\n");

            for (const GraphNode& node : graphNodes)
            {
                (void)node;
                AZ_Error("Entity", false, "Component: %s\n", node.first->RTTI_GetTypeName());
            }

            return DSR_CYCLIC_DEPENDENCY;
        }

        return DSR_OK;
    }

    //32 bit crc of machine ID, process ID, and process start time
    AZ::u32 Entity::GetProcessSignature()
    {
        static EnvironmentVariable<AZ::u32> processSignature = nullptr;

        struct ProcessInfo
        {
            AZ::Platform::MachineId m_machineId = 0;
            AZ::Platform::ProcessId m_processId = 0;
            AZ::u64 m_startTime = 0;
        };
        if (!processSignature)
        {
            ProcessInfo processInfo;
            processInfo.m_machineId = AZ::Platform::GetLocalMachineId();
            processInfo.m_processId = AZ::Platform::GetCurrentProcessId();
            processInfo.m_startTime = AZStd::GetTimeUTCMilliSecond();
            AZ::u32 signature = AZ::Crc32(&processInfo, sizeof(processInfo));
            processSignature = Environment::CreateVariable<AZ::u32>(AZ_CRC("MachineProcessSignature", 0x47681763), signature);
        }
        return *processSignature;
    }

    //=========================================================================
    // MakeId
    // Ids must be unique across a project at authoring time. Runtime doesn't matter
    // as much, especially since ids are regenerated at spawn time, and the network
    // layer re-maps ids as entities are spawned via replication.
    // Ids are of the following format:
    // | 32 bits of monotonic count | 32 bit crc of machine ID, process ID, and process start time |
    //=========================================================================
    EntityId Entity::MakeId()
    {
        static EnvironmentVariable<AZStd::atomic_uint> counter = nullptr; // this counter is per-process

        if (!counter)
        {
            counter = Environment::CreateVariable<AZStd::atomic_uint>(AZ_CRC("EntityIdMonotonicCounter", 0xbe691c64), 1);
        }

        AZ::u64 count = counter->fetch_add(1);
        EntityId eid = AZ::EntityId(count << 32 | GetProcessSignature());
        return eid;
    }
} // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD
