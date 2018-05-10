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

/**
 * @file
 * Header file for buses that dispatch and receive events from an entity context. 
 * Entity contexts are collections of entities. Examples of entity contexts are 
 * the editor context, game context, a custom context, and so on.
 */

#ifndef AZFRAMEWORK_ENTITYCONTEXTBUS_H
#define AZFRAMEWORK_ENTITYCONTEXTBUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/std/hash.h>

namespace AZ
{
    class Entity;
    class EntityId;
}

namespace AzFramework
{
    class EntityContext;

    /**
     * Unique ID for an entity context. 
     */
    using EntityContextId = AZ::Uuid;

    /**
     * Identifies an asynchronous slice instantiation request. 
     * This can be used to get the results of a slice instantiation request.
     */
    class SliceInstantiationTicket
    {
    public:
        /**
         * Enables this class to be identified across modules and serialized into
         * different contexts.
         */
        AZ_TYPE_INFO(SliceInstantiationTicket, "{E6E7C0C5-07C9-44BB-A38C-930431948667}");

        /**
         * Creates a slice instantiation ticket.
         * @param contextId The ID of the entity context to create the slice in.
         * @param requestId An ID for the slice instantiation ticket.
         */
        SliceInstantiationTicket(EntityContextId contextId = 0, AZ::u64 requestId = 0)
            : m_contextId(contextId)
            , m_requestId(requestId)
        {}

        /**
         * Overloads the equality operator to indicate that two slice instantiation  
         * tickets are equal if they have the same entity context ID and request ID.
         * @param rhs The slice instantiation ticket you want to compare to the 
         * current ticket.
         * @return Returns true if the entity context ID and the request ID of the   
         * slice instantiation tickets are equal.
         */
        inline bool operator==(const SliceInstantiationTicket& rhs) const
        {
            return rhs.m_contextId == m_contextId &&
                   rhs.m_requestId == m_requestId;
        }

        /**
         * Overloads the inequality operator to indicate that two slice instantiation
         * tickets are not equal do not have the same entity context ID and request ID.
         * @param rhs The slice instantiation ticket you want to compare to the
         * current ticket.
         * @return Returns true if the entity context ID and the request ID of the
         * slice instantiation tickets are not equal.
         */
        inline bool operator!=(const SliceInstantiationTicket& rhs) const
        {
            return !((*this) == rhs);
        }

        /**
         * Overloads the boolean operator to indicate that a slice instantiation  
         * ticket is true if its request ID is valid.
         * @return Returns true if the request ID is not equal to zero.
         */
        inline operator bool() const
        {
            return m_requestId != 0;
        }

        /**
         * The ID of the entity context to create the slice in.
         */
        EntityContextId m_contextId;

        /**
         * An ID for the slice instantiation ticket.
         */
        AZ::u64 m_requestId;
    };

    /**
     * Interface for AzFramework::EntityContextRequestBus, which is
     * the EBus that makes requests to a given entity context. 
     * If you want to make requests to a specific entity context, such  
     * as the game entity context, use the interface specific to that  
     * context. If you want to make requests to multiple types of entity   
     * contexts, use this interface.
     */
    class EntityContextRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        /**
         * Overrides the default AZ::EBusAddressPolicy so that the EBus has 
         * multiple addresses. Events that are addressed to an ID are received
         * by all handlers that are connected to that ID.
         */
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        /**
         * Specifies that events are addressed by entity context ID.
         */
        typedef EntityContextId BusIdType;
        //////////////////////////////////////////////////////////////////////////
        
        /**
         * Retrieves the root slice of the entity context. 
         * Each entity context has a root slice, which contains all the 
         * entities within the context.
         * @return A pointer to the root slice.
         */
        virtual AZ::SliceComponent* GetRootSlice() = 0;

        /**
         * Creates an entity and adds it to the root slice of the entity context.
         * This operation does not activate the entity by default.
         * @param name A name for the entity.
         * @return A pointer to a new entity. 
         * This operation succeeds unless the system is completely out of memory.
         */
        virtual AZ::Entity* CreateEntity(const char* name) = 0;

        /**
         * Adds an entity to the root slice of the entity context. 
         * This operation does not activate the entity by default.
         * Derived classes might choose to set the entity to another state.
         * @param entity A pointer to the entity to add.
         */
        virtual void AddEntity(AZ::Entity* entity) = 0;

        /**
         * Activates an entity that is owned by the entity context.
         * @param id The ID of the entity to activate.
         */
        virtual void ActivateEntity(AZ::EntityId entityId) = 0;

        /**
         * Deactivates an entity that is owned by the entity context.
         * @param id The ID of the entity to deactivate.
         */
        virtual void DeactivateEntity(AZ::EntityId entityId) = 0;

        /**
         * Removes an entity from the entity context's root slice and 
         * destroys the entity.
         * @param entity A pointer to the entity to destroy.
         * @return If the entity context does not own the entity,  
         * this returns false and does not destroy the entity.  
         */
        virtual bool DestroyEntity(AZ::Entity* entity) = 0;

        /**
         * Removes an entity from the entity context's root slice and 
         * destroys the entity.
         * @param entityId The ID of the entity to destroy.
         * @return If the entity context does not own the entity,
         * this returns false and does not destroy the entity.
         */
        virtual bool DestroyEntityById(AZ::EntityId entityId) = 0;

        /**
         * Creates a copy of the entity in the root slice of the entity context.
         * The cloned copy is assigned a unique entity ID.
         * @param sourceEntity A reference to the entity to clone.
         * @return A pointer to the cloned copy of the entity. This operation 
         * can fail if serialization data fails to interpret the source entity.
         */
        virtual AZ::Entity* CloneEntity(const AZ::Entity& sourceEntity) = 0;

        /**
         * Returns a mapping of stream-loaded entity IDs to remapped entity IDs, 
         * if remapping was performed.
         * @return A mapping of entity IDs loaded from a stream to remapped values. 
         * If the stream was loaded without remapping enabled, the map will be empty.
         */
        virtual const AZ::SliceComponent::EntityIdToEntityIdMap& GetLoadedEntityIdMap() = 0;

        /**
         * Clears the entity context by destroying all entities and slice instances 
         * that the entity context owns.
         */
        virtual void ResetContext() = 0;
    };

    /**
     * The EBus for requests to the entity context.
     * The events are defined in the AzFramework::EntityContextRequests class.
     * If you want to make requests to a specific entity context, such
     * as the game entity context, use the bus specific to that context.
     * If you want to make requests to multiple types of entity contexts,
     * use this bus.
     */
    using EntityContextRequestBus = AZ::EBus<EntityContextRequests>;

    /**
     * Interface for the AzFramework::EntityContextEventBus, which is the EBus
     * that dispatches notification events from the global entity context.
     * If you want to receive notification events from a specific entity context, 
     * such as the game entity context, use the interface specific to that context.
     * If you want to receive notification events from multiple types of entity 
     * contexts, use this interface.
     */
    class EntityContextEvents
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~EntityContextEvents() {}

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        /**
         * Overrides the default AZ::EBusAddressPolicy to specify that the EBus
         * has multiple addresses. Events that are addressed to an ID are received
         * by all handlers connected to that ID.
         */
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        /**
         * Specifies that events are addressed by entity context ID.
         */
        typedef EntityContextId BusIdType;
        //////////////////////////////////////////////////////////////////////////

        /**
         * Signals that an entity context was loaded from a stream.
         * @param contextEntities A reference to a list of entities that 
         * are owned by the entity context that was loaded.
         */
        virtual void OnEntityContextLoadedFromStream(const AZ::SliceComponent::EntityList& /*contextEntities*/) {}

        /**
         * Signals that the entity context was reset.
         */
        virtual void OnEntityContextReset() {}

        /**
         * Signals that the entity context created an entity.
         * @param entity A reference to the entity that was created.
         */
        virtual void OnEntityContextCreateEntity(AZ::Entity& /*entity*/) {}

        /**
         * Signals that the entity context is about to destroy an entity.
         * @param id A reference to the ID of the entity that will be destroyed.
         */
        virtual void OnEntityContextDestroyEntity(const AZ::EntityId& /*id*/) {}

        /**
         * Signals that a slice was successfully instantiated prior to 
         * entity registration.
         * @param sliceAssetId A reference to the slice asset ID.
         * @param sliceAddress A reference to the slice instance address.
         */
        virtual void OnSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/) {}

        /**
         * Signals that a slice was successfully instantiated after 
         * entity registration.
         * @param sliceAssetId A reference to the slice asset ID.
         * @param sliceAddress A reference to the slice instance address.
         */
        virtual void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/) {}

        /**
         * Signals that a slice could not be instantiated.
         * @param sliceAssetId A reference to the slice asset ID.
         */
        virtual void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/) {}
    };

    /**
     * The EBus for entity context events.
     * The events are defined in the AzFramework::EntityContextEvents class.
     * If you want to receive event notifications from a specific entity context, 
     * such as the game entity context, use the bus specific to that context.
     * If you want to receive event notifications from multiple types of entity 
     * contexts, use this bus.
     */
    using EntityContextEventBus = AZ::EBus<EntityContextEvents>;

    /**
     * Interface for AzFramework::EntityIdContextQueryBus, which is
     * the EBus that queries an entity about its context.
     */
    class EntityIdContextQueries
        : public AZ::ComponentBus
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~EntityIdContextQueries() {}

        /**
         * Gets the ID of the entity context that the entity belongs to.
         * @return The ID of the entity context that the entity belongs to.
         */
        virtual EntityContextId GetOwningContextId() = 0;

        /**
         * Gets the address of the slice instance that owns the entity.
         * @return The address of the slice instance that owns the entity. 
         * If the entity is not owned by any other slice instance, then  
         * the slice instance that owns the entity is the root slice.
         */
        virtual AZ::SliceComponent::SliceInstanceAddress GetOwningSlice() = 0;
    };

    /**
     * The EBus for querying an entity about its context.
     * The events are defined in the AzFramework::EntityIdContextQueries class.
     */
    using EntityIdContextQueryBus = AZ::EBus<EntityIdContextQueries>;

    /**
     * Interface for AzFramework::SliceInstantiationResultBus, which 
     * enables you to receive results regarding your slice instantiation 
     * requests.
     */
    class SliceInstantiationResults
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~SliceInstantiationResults() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        /**
         * Overrides the default AZ::EBusAddressPolicy to specify that the EBus
         * has multiple addresses. Components that request slice instantiation 
         * receive the results of the request at the EBus address that is 
         * associated with the request ID of the slice instantiation ticket.
         */
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        /**
         * Specifies that events are addressed by the request ID of the 
         * slice instantiation ticket.
         */
        typedef SliceInstantiationTicket BusIdType;
        //////////////////////////////////////////////////////////////////////////

        /**
         * Signals that a slice was successfully instantiated prior to
         * entity registration.
         * @param sliceAssetId A reference to the slice asset ID.
         * @param sliceAddress A reference to the slice instance address.
         */
        virtual void OnSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/) {}

        /**
         * Signals that a slice was successfully instantiated after
         * entity registration.
         * @param sliceAssetId A reference to the slice asset ID.
         * @param sliceAddress A reference to the slice instance address.
         */
        virtual void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/) {}

        /**
         * Signals that a slice could not be instantiated.
         * @param sliceAssetId A reference to the slice asset ID.
         */
        virtual void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/) {}
    };

    /**
     * The EBus that notifies you about the results of your slice instantiation requests.
     * The events are defined in the AzFramework::SliceInstantiationResults class.
     */
    using SliceInstantiationResultBus = AZ::EBus<SliceInstantiationResults>;
} // namespace AzFramework

namespace AZStd
{
    /**
     * Enables slice instantiation tickets to be keys in hashed data structures.
     */
    template<>
    struct hash<AzFramework::SliceInstantiationTicket>
    {
        inline size_t operator()(const AzFramework::SliceInstantiationTicket& value) const
        {
            AZ::Crc32 crc;
            crc.Add(&value.m_contextId, sizeof(value.m_contextId));
            crc.Add(&value.m_requestId, sizeof(value.m_requestId));
            return static_cast<size_t>(static_cast<AZ::u32>(crc));
        }
    };
}

#endif // AZFRAMEWORK_ENTITYCONTEXTBUS_H
