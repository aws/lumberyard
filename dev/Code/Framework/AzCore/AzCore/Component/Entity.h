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

/** @file
 * Header file for the Entity class.
 * In Lumberyard's component entity system, an entity is an addressable container for  
 * a group of components. The entity represents the functionality and properties of an 
 * object within your game.
 */

#ifndef AZCORE_COMPONENT_ENTITY_H
#define AZCORE_COMPONENT_ENTITY_H

#include <AzCore/Component/Component.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/std/string/string.h>

namespace AZ
{
    class Transform;
    class TransformInterface;

    /**
     * An addressable container for a group of components. 
     * An entity creates, initializes, activates, and deactivates its components.  
     * An entity has an ID and, optionally, a name.  
     */
    class Entity
    {
    public:

        /**
         * Specifies that this class should use AZ::SystemAllocator for memory
         * management by default.
         */
        AZ_CLASS_ALLOCATOR(Entity, SystemAllocator, 0);

        /**
         * Adds run-time type information to this class.
         */
        AZ_RTTI(AZ::Entity, "{75651658-8663-478D-9090-2432DFCAFA44}");

        /**
         * The type of array that contains the entity's components. 
         * Used when iterating over components.
         */
        typedef AZStd::vector<Component*>   ComponentArrayType;

        /**
         * The state of the entity and its components.
         * @note An entity is only initialized once. It can be activated and deactivated multiple times.
         */
        enum State : u8
        {
            ES_CONSTRUCTED,         ///< The entity was constructed but is not initialized or active. This is the default state after an entity is created.
            ES_INITIALIZING,        ///< The entity is initializing itself and its components. This state is the transition between ES_CONSTRUCTED and ES_INIT. 
            ES_INIT,                ///< The entity and its components are initialized. You can add and remove components from the entity when it is in this state.
            ES_ACTIVATING,          ///< The entity is activating itself and its components. This state is the transition between ES_INIT and ES_ACTIVE. 
            ES_ACTIVE,              ///< The entity and its components are active and fully operational. You cannot add or remove components from the entity unless you first deactivate the entity.
            ES_DEACTIVATING,        ///< The entity is deactivating itself and its components. This state is the transition between ES_ACTIVE and ES_INIT. 
        };

        /**
         * Represents whether an entity can be activated. 
         * An entity cannot be activated unless all component dependency requirements are met, and 
         * components are sorted so that each can be activated before the components that depend on it.
         */
        enum class DependencySortResult
        {
            Success = 0,                ///< All component dependency requirements are met. The entity can be activated.
            MissingRequiredService,     ///< One or more components that provide required services are not in the list of components to activate.
            HasCyclicDependency,        ///< A cycle in component service dependencies was detected.
            HasIncompatibleServices,    ///< A component is incompatible with a service provided by another component.
            DescriptorNotRegistered,    ///< A component descriptor was not registered with the AZ::ComponentApplication.
            MissingDescriptor,          ///< Cannot find a component's ComponentDescriptor

            // Deprecated values
            DSR_OK = Success,
            DSR_MISSING_REQUIRED = MissingRequiredService,
            DSR_CYCLIC_DEPENDENCY = HasCyclicDependency,
        };

        /**
         * Constructs an entity and automatically generates an entity ID. 
         * @param name (Optional) A name for the entity. The entity ID is used for addressing and identification, 
         * but a name is useful for debugging.
         */
        Entity(const char* name = nullptr);

        /**
         * Constructs an entity with the entity ID that you specify.
         * @param id An ID for the entity.
         * @param name (Optional) A name for the entity. The entity ID is used for addressing and identification, 
         * but a name is useful for debugging.
         */
        Entity(const EntityId& id, const char* name = nullptr);

        /**
         * Destroys an entity and its components.
         * Do not destroy an entity when it is in a transition state. 
         * If the entity is in a transition state, this function asserts.
         */
        virtual ~Entity();

        /**
         * Gets the ID of the entity.
         * @return The ID of the entity. 
         */
        EntityId GetId() const { return m_id; }

        /**
         * Gets the name of the entity.
         * @return The name of the entity. 
         */
        const AZStd::string& GetName() const { return m_name; }

        /**
         * Sets the name of the entity.
         * @param name A name for the entity.
         */
        void SetName(AZStd::string name) { m_name = name; OnNameChanged(); }

        /**
         * Gets the state of the entity.
         * @return The state of the entity. For example, the entity has been initialized, the 
         * entity is active, and so on.  
         */
        State GetState() const   { return m_state; }

        /**
         * Sets the ID of the entity.
         * You can only change the ID of the entity when the entity has been constructed but is
         * not yet active or initialized.
         * @param id The ID of the entity.
         */
        void SetId(const EntityId& id);

        /**
         * Initializes the entity and its components.
         * This function is called only once in an entity's lifetime, whereas an entity  
         * can be activated and deactivated multiple times.
         * This function calls each component's Init function and provides its entity ID 
         * to each component.
         */
        virtual void Init();

        /**
         * Activates the entity and its components.
         * This function can be called multiple times throughout the lifetime of an 
         * entity. Before activating the components, this function verifies that all 
         * component dependency requirements are met, and that components are sorted 
         * so that each can be activated before the components that depend on it. 
         * If these requirements are met, this function calls the Activate function 
         * of each component.
         */
        virtual void Activate();

        /**
         * Deactivates the entity and its components.
         * This function can be called multiple times throughout the lifetime of an
         * entity. This function calls the Deactivate function of each component.
         */
        virtual void Deactivate();

        /**
         * Creates a component and attaches the component to the entity. 
         * You cannot add a component to an entity when the entity is 
         * active or in a transition state. After the component is attached 
         * to the entity, the entity owns the component. If you destroy the 
         * entity, the component is destroyed along with the entity.
         * To release ownership without destroying the component, use RemoveComponent().
         * @return A pointer to the component. Returns a null pointer if
         * the component could not be created.
         */
        template<class ComponentType, typename... Args>
        ComponentType* CreateComponent(Args&&... args);

        /**
         * Creates a component and attaches the component to the entity.
         * You cannot add a component to an entity when the entity is
         * active or in a transition state. After the component is attached
         * to the entity, the entity owns the component. If you destroy the
         * entity, the component is destroyed along with the entity. 
         * To release ownership without destroying the component, use RemoveComponent().
         * @param componentTypeId The UUID of the component type.
         * @return A pointer to the component. Returns a null pointer if 
         * the component could not be created.
         */
        Component* CreateComponent(const Uuid& componentTypeId);

        /// @cond EXCLUDE_DOCS 
        /**
         * @deprecated In tools, use AzToolsFramework::EntityCompositionRequestBus 
         * to ensure component requirements are met. 
         */
        template<class ComponentType>
        ComponentType* CreateComponentIfReady()
        {
            return static_cast<ComponentType*>(CreateComponentIfReady(AzTypeInfo<ComponentType>::Uuid()));
        }
        /// @endcond

        /// @cond EXCLUDE_DOCS 
        /**
         * @deprecated In tools, use AzToolsFramework::EntityCompositionRequestBus 
         * to ensure component requirements are met.
         */
        Component* CreateComponentIfReady(const Uuid& componentTypeId);
        /// @endcond

        /**
         * Attaches an existing component to the entity.        
         * You cannot attach a component to an entity when the entity is active
         * or in a transition state. After the component is attached 
         * to the entity, the entity owns the component. If you destroy the 
         * entity, the component is destroyed along with the entity. 
         * To release ownership without destroying the component, use RemoveComponent().
         * The component can be attached to only one entity at a time.
         * If the component is already attached to an entity, this code asserts.
         * @param component A pointer to the component to attach to the entity.
         * @return True if the component was successfully attached to the entity. Otherwise, false.
         */
        bool AddComponent(Component* component);

        /// @cond EXCLUDE_DOCS 
        /**
         * @deprecated In tools, use AzToolsFramework::EntityCompositionRequestBus 
         * to ensure component requirements are met.
         */
        bool IsComponentReadyToAdd(const Component* component, ComponentDescriptor::DependencyArrayType* servicesNeededToBeAdded = nullptr, ComponentArrayType* incompatibleComponents = nullptr)
        {
            return IsComponentReadyToAdd(component->RTTI_GetType(), component, servicesNeededToBeAdded, incompatibleComponents);
        }
        /// @endcond

        /// @cond EXCLUDE_DOCS 
        /**
         * @deprecated In tools, use AzToolsFramework::EntityCompositionRequestBus 
         * to ensure component requirements are met.
         */
        bool IsComponentReadyToAdd(const Uuid& componentTypeId, ComponentDescriptor::DependencyArrayType* servicesNeededToBeAdded = nullptr, ComponentArrayType* incompatibleComponents = nullptr)
        {
            return IsComponentReadyToAdd(componentTypeId, nullptr, servicesNeededToBeAdded, incompatibleComponents);
        }
        /// @endcond

        /**
         * Removes a component from the entity.
         * After the component is removed from the entity, you are responsible for destroying the component.
         * @param component A pointer to the component to remove from the entity.
         * @return True if the component was removed from the entity. False if the component could not be removed.
         */
        bool RemoveComponent(Component* component);
        
        /// @cond EXCLUDE_DOCS 
        /**
         * @deprecated In tools, use AzToolsFramework::EntityCompositionRequestBus 
         * to ensure component requirements are met.
         */
        bool IsComponentReadyToRemove(Component* component, ComponentArrayType* componentsNeededToBeRemoved = nullptr);
        /// @endcond

        /// @cond EXCLUDE_DOCS 
        /**
         * Replaces one of an entity's components with another component.
         * The entity takes ownership of the added component and relinquishes ownership of the removed component.
         * The added component is assigned the component ID of the removed component.
         *
         * You can only swap the components of an entity when the entity is in the ES_CONSTRUCTED or ES_INIT state.
         * @param componentToRemove The component to remove from the entity.
         * @param componentToAdd The component to add to the entity.
         * @return True if the components were swapped. False if the components could not be swapped.
         */
         bool SwapComponents(Component* componentToRemove, Component* componentToAdd);
         /// @endcond

        /**
         * Gets all components registered with the entity.
         * @return An array of all components registered with the entity.
         */
        const ComponentArrayType& GetComponents() const { return m_components; }

        /**
         * Finds a component by component ID.
         * @param id The ID of the component to find.
         * @return A pointer to the component with the specified component ID.
         * If a component with the specified ID cannot be found, the return value
         * is a null pointer.
         */
        Component*      FindComponent(ComponentId id) const;

        /**
         * Finds the first component of the requested component type.
         * @param typeId The type of component to find.
         * @return A pointer to the first component of the requested type. Returns
         * a null pointer if a component of the requested type cannot be found.
         */
        Component*      FindComponent(const Uuid& typeId) const;

        /**
         * Finds a component by component ID.
         * @param id The ID of the component to find.
         * @return A pointer to the component with the specified component ID.
         * If a component with the specified ID cannot be found or the component
         * type does not exist, the return value is a null pointer.
         */
        template<class ComponentType>
        inline ComponentType* FindComponent(ComponentId id) const
        {
            return azrtti_cast<ComponentType*>(FindComponent(id));
        }

        /**
         * Finds the first component of the requested component type.
         * @return A pointer to the first component of the requested type. Returns
         * a null pointer if a component of the requested type cannot be found.
         */
        template<class ComponentType>
        inline ComponentType* FindComponent() const
        {
            return azrtti_cast<ComponentType*>(FindComponent(AzTypeInfo<ComponentType>::Uuid()));
        }
 
        /**
        * Return a vector of all the components of the specified type in an entity.
         * @return a vector of all the components of the specified type.
         */
        ComponentArrayType FindComponents(const Uuid& typeId) const;

        /// Return a vector of all the components of the specified type in an entity.
        template<class ComponentType>
        inline AZStd::vector<ComponentType*> FindComponents() const
        {
            ComponentArrayType componentArray = FindComponents(azrtti_typeid<ComponentType>());
            AZStd::vector<ComponentType*> components(componentArray.size());
            AZStd::transform(componentArray.begin(), componentArray.end(), components.begin(), [](Component* component) { return static_cast<ComponentType*>(component); });
            return components;
        }

        /**
         * Indicates to the entity that dependencies among its components need
         * to be evaluated.
         * Dependencies will be evaluated the next time the entity is activated.
         */
        void            InvalidateDependencies();

        /**
         * Contains a failed DependencySortResult code
         * and a detailed message that can be presented to users.
         */
        struct FailedSortDetails
        {
            DependencySortResult m_code;
            AZStd::string m_message;
        };

        using DependencySortOutcome = AZ::Outcome<void, FailedSortDetails>;

        /**
         * Calls DependencySort() to sort an entity's components based on the dependencies
         * among components. If all dependencies are met, the required services can be
         * activated before the components that depend on them. An entity will not be
         * activated unless the sort succeeds.
         * @return A successful outcome is returned if the entity can
         * determine an order in which to activate its components.
         * Otherwise the failed outcome contains details on why the sort failed.
         */
        DependencySortOutcome EvaluateDependenciesGetDetails();

        /**
         * Same as EvaluateDependenciesGetDetails(), but if sort fails
         * only a code is returned, there is no detailed error message.
         */
        DependencySortResult EvaluateDependencies();

        /**
         * Mark the entity to be activated by default. This is observed automatically by EntityContext,
         * and should be observed by any other custom systems that create and manage entities.
         * @param activeByDefault whether the entity should be active by default after creation.
         */
        void            SetRuntimeActiveByDefault(bool activeByDefault);

        /**
         * @return true if the entity is marked to activate by default upon creation.
         */
        bool            IsRuntimeActiveByDefault() const;

        /**
         * Reflects the entity into a variety of contexts (script, serialize, edit,
         * and so on).
         * @param reflection A pointer to the reflection context.
         */
        static void     Reflect(ReflectContext* reflection);

        /**
         * Generates a unique entity ID.
         * @return An entity ID.
         */
        static EntityId MakeId();

        /**
         * Gets the Process Signature of the local machine.
         * @return The Process Signature of the local machine.
         */
        static AZ::u32 GetProcessSignature();

        /// @cond EXCLUDE_DOCS 
        /**
         * @deprecated Use the TransformBus to communicate with the TransformInterface.
         */
        inline TransformInterface* GetTransform() const { return m_transform; }
        /// @endcond

        /**
        * Sorts an entity's components based on the dependencies between components.
        * If all dependencies are met, the required services can be activated
        * before the components that depend on them.
        * @param components An array of components attached to the entity.
        * @return A successful outcome is returned if the entity can
        * determine an order in which to activate its components.
        * Otherwise the outcome contains details on why the sort failed.
        */
        static DependencySortOutcome DependencySort(ComponentArrayType& components);

    protected:

        /// @cond EXCLUDE_DOCS 
        /**
         * @deprecated In tools, use AzToolsFramework::EntityCompositionRequestBus 
         * to ensure component requirements are met.
         */
        bool IsComponentReadyToAdd(const Uuid& componentTypeId, const Component* instance, ComponentDescriptor::DependencyArrayType* servicesNeededToBeAdded, ComponentArrayType* incompatibleComponents);
        /// @endcond

        /**
         * Signals to listeners that the entity's name has changed.
         */
        void OnNameChanged() const;

        /**
         * Finds whether the entity is in a state in which components can be added or removed.
         * Components can be added or removed when the entity is in the ES_CONSTRUCTED
         * or ES_INIT state.
         * @return True if the entity is in a state in which that components can be added or removed.
         * Otherwise, false.
         */
        bool CanAddRemoveComponents() const;

        // Helpers for child classes
        static void ActivateComponent(Component& component) { component.Activate(); }
        static void DeactivateComponent(Component& component) { component.Deactivate(); }

        /**
         * The ID that the system uses to identify and address the entity.
         * The serializer determines whether this is an entity ID or an entity
         * reference ID. 
         *
         * IMPORTANT: This must be the only EntityId member of the Entity class.
         */
        EntityId m_id;

        /**
         * An array of components attached to the entity.
         */
        ComponentArrayType m_components;

        
        /**
         * A cached pointer to the transform interface. 
         * We recommend using AZ::TransformBus and caching locally instead of accessing
         * the transform interface directly through this pointer.
         */
        TransformInterface* m_transform;
        
        /**
         * A user-friendly name for the entity. This makes error messages easier to read.
         */
        AZStd::string m_name;

        /**
         * The state of the entity.
         */
        State m_state;
        
        /**
         * Foundational entity properties/flags.
         * To keep AZ::Entity lightweight, one should resist the urge the add flags here unless they're extremely
         * common to AZ::Entity use cases, and inherently fundamental.
         * Furthermore, if more than 4 flags are needed, please consider using a more space-efficient container,
         * such as AZStd::bit_set<>. With just a couple flags, AZStd::bit_set's word-size of 32-bits will actually 
         * waste space.
         */
        bool m_isDependencyReady;           ///< Indicates the component dependencies have been evaluated and sorting was completed successfully.
        bool m_isRuntimeActiveByDefault;    ///< Indicates the entity should be activated on initial creation.
    };

    template<class ComponentType, typename... Args>
    inline ComponentType* Entity::CreateComponent(Args&&... args)
    {
        ComponentType* component = aznew ComponentType(AZStd::forward<Args>(args)...);
        AZ_Assert(component, "Failed to create component: %s", AzTypeInfo<ComponentType>::Name());
        if (component)
        {
            if (!AddComponent(component))
            {
                delete component;
                component = nullptr;
            }
        }
        return component;
    }
} // namespace AZ

#endif // AZCORE_COMPONENT_ENTITY_H
#pragma once
