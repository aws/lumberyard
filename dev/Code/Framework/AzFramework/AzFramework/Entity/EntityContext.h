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
#ifndef AZFRAMEWORK_ENTITYCONTEXT_H
#define AZFRAMEWORK_ENTITYCONTEXT_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/IdUtils.h>

#include "EntityContextBus.h"

namespace AZ
{
    class SliceComponent;
    class ReflectContext;
}

namespace AzFramework
{
    class EntityContext;

    /**
     * Provides services for a group of entities under the umbrella of a given context.
     *
     * e.g. Edit-time entities and runtime entities would belong to separate contexts,
     * each with their own root slice.
     *
     * A context owns a root slice entity, which can be serialized in or out. Interfaces are
     * provided for creating entities owned by the context, and instantiating slices within
     * the context.
     *
     * Entity contexts are not required to use entities, but provide a package for managing
     * independent slice hierarchies (i.e. a level, a world, etc).
     */
    class EntityContext
        : public EntityIdContextQueryBus::MultiHandler
        , public AZ::Data::AssetBus::MultiHandler
        , public AZ::EntityBus::MultiHandler 
        , public EntityContextRequestBus::Handler
    {
    public:

        using EntityList = AZStd::vector<AZ::Entity*>;
        using EntityIdList = AZStd::vector<AZ::EntityId>;

        AZ_TYPE_INFO(EntityContext, "{4F98A6B9-C7B5-450E-8A8A-30EEFC411EF5}");

        EntityContext(AZ::SerializeContext* serializeContext = nullptr);
        EntityContext(const EntityContextId& contextId, AZ::SerializeContext* serializeContext = nullptr);
        virtual ~EntityContext();

        void InitContext();
        void DestroyContext();

        /// \return the context's Id, which is used to listen on a given context's request or event bus.
        const EntityContextId& GetContextId() const { return m_contextId; }

        /// Get ids of all entities made from the root slice including those from the slices the root references.
        /// \return a vector of EntityId. Empty if the root slice hasn't been instantiated.
        EntityIdList GetRootSliceEntityIds();

        /// Instantiate a slice asset in the context. Listen for the OnSliceInstantiated() / OnSliceInstantiationFailed()
        /// events for details about the resulting entities.
        /// \param asset slice asset to instantiate.
        /// \param customIdMapper optional Id map callback to allow caller to customize entity Id generation.
        /// \param assetLoadFilterCB optional asset load filter callback. This is only necessary when heavily customizing asset loading, as it can allow deferral of dependent asset loading.
        /// \param return slice instantiation ticket.
        virtual SliceInstantiationTicket InstantiateSlice(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper = nullptr, const AZ::Data::AssetFilterCB& assetLoadFilter = nullptr);

        /// Cancels the asynchronous instantiation of a slice.
        virtual void CancelSliceInstantiation(const SliceInstantiationTicket& ticket);

        /** 
         * Clones an existing slice instance in the context. New instance is immediately returned. 
         * This function doesn't automatically add new instance to any entity context, callers are responsible for that.
         * \param sourceInstance The source instance to be cloned
         * \param sourceToCloneEntityIdMap [out] The map between source entity ids and clone entity ids
         * \return new slice address. A null slice address will be returned if cloning fails (.first==nullptr, .second==nullptr).
         */
        virtual AZ::SliceComponent::SliceInstanceAddress CloneSliceInstance(AZ::SliceComponent::SliceInstanceAddress sourceInstance, AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap);

        /// Load the root slice from a stream.
        /// \return whether or not the root slice was successfully loaded from the provided stream.
        /// \param stream - the source stream from which to load
        /// \param remapIds - if true, entity Ids will be remapped post-load
        /// \param idRemapTable - if remapIds is true, the provided table is filled with a map of original ids to new ids.
        /// \param loadFlags - any ObjectStream::LoadFlags.
        virtual bool LoadFromStream(AZ::IO::GenericStream& stream, bool remapIds, AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable = nullptr, const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor());

        /// Initialize this entity context with a newly loaded root slice
        /// \return whether or not the root slice is valid.
        /// \param rootEntity - the rootEntity which has been loaded
        /// \param remapIds - if true, entity Ids will be remapped post-load
        /// \param idRemapTable - if remapIds is true, the provided table is filled with a map of original ids to new ids.
        virtual bool HandleLoadedRootSliceEntity(AZ::Entity* rootEntity, bool remapIds, AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable = nullptr);

        /// Properly process new metadata entities created during slice streaming
        /// Because the streaming process bypasses slice creation, the entity context has to make sure they're handled properly.
        /// \param slice - The slice that was streamed in
        virtual void HandleNewMetadataEntitiesCreated(AZ::SliceComponent& /*slice*/) {};

        //////////////////////////////////////////////////////////////////////////
        // EntityContextRequestBus
        AZ::SliceComponent* GetRootSlice() override;
        AZ::Data::AssetId CurrentlyInstantiatingSlice() override;
        AZ::Entity* CreateEntity(const char* name) override;
        void AddEntity(AZ::Entity* entity) override;
        void ActivateEntity(AZ::EntityId entityId) override;
        void DeactivateEntity(AZ::EntityId entityId) override;
        bool DestroyEntity(AZ::Entity* entity) override;
        bool DestroyEntityById(AZ::EntityId entityId) override;
        AZ::Entity* CloneEntity(const AZ::Entity& sourceEntity) override;
        void ResetContext() override;

        const AZ::SliceComponent::EntityIdToEntityIdMap& GetLoadedEntityIdMap() override;
        AZ::EntityId FindLoadedEntityIdMapping(const AZ::EntityId& staticId) const override;
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);

    protected:

        //////////////////////////////////////////////////////////////////////////
        // EntityIdContextQueryBus
        EntityContextId GetOwningContextId() override { return m_contextId; }
        AZ::SliceComponent::SliceInstanceAddress GetOwningSlice() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetBus
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EntityBus
        void OnEntityDestruction(const AZ::EntityId& entityId) override;
        //////////////////////////////////////////////////////////////////////////

        void CreateRootSlice();

        AZ::SliceComponent::SliceInstanceAddress GetOwningSliceForEntity(AZ::EntityId entityId) const;

        /**
        * A performant way to destroy all entities under the root slice, including both loose entities and entities
        * in slices. This is useful when resetting the context. 
        */
        void DestroyRootSliceEntities();

        void HandleEntitiesAdded(const EntityList& entities);
        void HandleEntityRemoved(const AZ::EntityId& id);

        /// Entity context derived implementations can conduct specialized actions when internal events occur, such as adds/removals/resets.
        virtual void OnContextEntitiesAdded(const EntityList& /*entities*/) {}
        virtual void OnContextEntityRemoved(const AZ::EntityId& /*id*/) {}
        virtual void OnRootSliceCreated() {}
        virtual void OnRootSlicePreDestruction() {}
        virtual void PrepareForContextReset() { m_contextIsResetting = true; }
        virtual void OnContextReset() { m_contextIsResetting = false; }

        /// Used to validate that the entities in an instantiated slice are valid entities for this context
        /// For example they could be non-UI entities being instantiated in a UI context
        virtual bool ValidateEntitiesAreValidForContext(const EntityList& entities);

        /// Determine if the entity with the given ID is owned by this Entity Context
        /// \param entityId An entity ID to check
        /// \return true if this context owns the entity with the given id.
        bool IsOwnedByThisContext(const AZ::EntityId& entityId);

        /// Helper function to send OnSliceInstantiationFailed events.
        static void DispatchOnSliceInstantiationFailed(const SliceInstantiationTicket& ticket, const AZ::Data::AssetId& assetId, bool canceled);


        AZ::SerializeContext*                       m_serializeContext;
        EntityContextId                             m_contextId;            ///< Id of the context, used to address bus messages
        AZ::Data::Asset<AZ::SliceAsset>             m_rootAsset;            ///< Stores root entity and slice instance.
        EntityContextEventBus::BusPtr               m_eventBusPtr;          ///< Pre-bound event bus for the context.
        AZ::u64                                     m_nextSliceTicket;      ///< Monotic tickets for slice instantiation requests.
        AZ::SliceComponent::EntityIdToEntityIdMap   m_loadedEntityIdMap;    ///< Stores map from entity Ids loaded from stream, to remapped entity Ids, if remapping was performed.
        AZ::Data::AssetId                           m_instantiatingAssetId; ///< When a slice is instantiating, the associated asset ID is cached here.

        /// Tracking of pending slice instantiations, each being the requested asset and the associated request's ticket.
        struct InstantiatingSliceInfo
        {
            InstantiatingSliceInfo(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const SliceInstantiationTicket& ticket, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customMapper)
                : m_asset(asset)
                , m_ticket(ticket)
                , m_customMapper(customMapper)
            {
            }

            AZ::Data::Asset<AZ::Data::AssetData>            m_asset;
            SliceInstantiationTicket                        m_ticket;
            AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper   m_customMapper;
        };

        // Slices queued for instantation. AZStd::list is used for its stable iterators since elements are deleted during traversal in Entitycontext::OnAssetReady
        AZStd::list<InstantiatingSliceInfo> m_queuedSliceInstantiations;
        // Tracks if the context is currently being reset.
        // This allows systems to skip steps during teardown that will be handled in bulk by the reset.
        bool m_contextIsResetting = false;
    };
} // namespace AzFramework

#endif // AZFRAMEWORK_ENTITYCONTEXT_H
