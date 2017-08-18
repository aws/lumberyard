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
 * Header file for buses that dispatch notification events concerning the AZ::Entity class. 
 * Buses enable entities and components to communicate with each other and with external 
 * systems.
 */

#ifndef AZCORE_ENTITY_BUS_H
#define AZCORE_ENTITY_BUS_H

#include <AzCore/std/string/string.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

namespace AZ
{
    /**
     * Interface for the AZ::EntitySystemBus, which is the EBus that dispatches   
     * notification events about every entity in the system.
     */
    class EntitySystemEvents
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~EntitySystemEvents() {}


        /**
         * Signals that an entity was activated.
         * This event is dispatched after the activation of the entity is complete. 
         * @param id The ID of the activated entity.
         */
        virtual void OnEntityActivated(const AZ::EntityId&) {}
        
        /**
         * Signals that an entity is being deactivated.
         * This event is dispatched immediately before the entity is deactivated.
         * @param id The ID of the deactivated entity.
         */
        virtual void OnEntityDeactivated(const AZ::EntityId&) {}
        
        /**
         * Signals that the name of an entity changed.
         * @param id The ID of the entity. 
         * @param name The new name of the entity.
         */
        virtual void OnEntityNameChanged(const AZ::EntityId&, const AZStd::string& /*name*/) {}
    };


    /**
     * The EBus for systemwide entity notification events. 
     * The events are defined in the AZ::EntitySystemEvents class.
     */
    typedef AZ::EBus<EntitySystemEvents> EntitySystemBus;

    /**
     * Interface for the AZ::EntityBus, which is the EBus for notification 
     * events dispatched by a specific entity.
     */
    class EntityEvents
        : public ComponentBus
    {
    private:
        template<class Bus>
        struct EntityEventsConnectionPolicy
            : public EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, const typename Bus::BusIdType& id = 0)
            {
                EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, id);

                Entity* entity = nullptr;
                EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, id);
                if (entity && (entity->GetState() == Entity::ES_ACTIVE))
                {
                    handler->OnEntityActivated(id);
                }
            }
        };
        
    public:

        /**
         * With this connection policy, an AZ::EntityEvents::OnEntityActivated event is  
         * immediately dispatched if the entity is active when a handler connects to the bus.
         */
        template<class Bus>
        using ConnectionPolicy = EntityEventsConnectionPolicy<Bus>;

        /**
         * Destroys the instance of the class.
         */
        virtual ~EntityEvents() {}

        /**
         * Signals that an entity was activated. 
         * This event is dispatched after the activation of the entity is complete.  
         * It is also dispatched immediately if the entity is already active 
         * when a handler connects to the bus.
         * @param EntityId The ID of the entity that was activated.
         */
        virtual void OnEntityActivated(const AZ::EntityId&)              {}

        /**
         * Signals that an entity is being deactivated.
         * This event is dispatched immediately before the entity is deactivated.
         * @param EntityId The ID of the entity that is being deactivated.
         */
        virtual void OnEntityDeactivated(const AZ::EntityId&)             {}

        /**
         * Signals that the name of an entity changed.
         * @param name The new name of the entity.
         */
        virtual void OnEntityNameChanged(const AZStd::string& name) { (void)name; }
    };

    /**
     * The EBus for notification events dispatched by a specific entity.
     * The events are defined in the AZ::EntityEvents class.
     */
    typedef AZ::EBus<EntityEvents>  EntityBus;
}

#endif // AZCORE_ENTITY_BUS_H
#pragma once
