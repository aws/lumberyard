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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Serialization/IdUtils.h>

namespace AZ
{
    /**
     * Slice component manages entities on the project graph (Slice is a node). SliceComponent with nested dependencies is an edit
     * only concept. We have a runtime version with dependencies so that we can accelerate production by live tweaking
     * of game objects. Otherwise all slices should be exported as list of entities (flat structure). There is an exception with the
     * "dynamic" slices, which should still be a flat list of entities that you "clone" for dynamic reuse.
     */
    class SliceComponent
        : public Component
        , public Data::AssetBus::MultiHandler
    {
        friend class SliceComponentSerializationEvents;
        friend class SliceAssetHandler;
    public:
        AZ_COMPONENT(SliceComponent, "{AFD304E4-1773-47C8-855A-8B622398934F}", Data::AssetEvents);

        class SliceReference;
        class SliceInstance;

        using EntityList = AZStd::vector<Entity*>;
        using EntityIdToEntityIdMap = AZStd::unordered_map<EntityId, EntityId>;
        using SliceInstanceAddress = AZStd::pair<SliceReference*, SliceInstance*>;
        using SliceInstanceToSliceInstanceMap = AZStd::unordered_map<SliceInstanceAddress, SliceInstanceAddress>;
        using EntityIdSet = AZStd::unordered_set<AZ::EntityId>;
        using SliceInstanceId = AZ::Uuid;

        /// @deprecated Use SliceReference.
        using PrefabReference = SliceReference;

        /// @deprecated Use SliceInstance.
        using PrefabInstance = SliceInstance;

        /// @deprecated Use SliceInstanceAddress.
        using PrefabInstanceAddress = SliceInstanceAddress;

        /// @deprecated Use SliceInstanceId.
        using PrefabInstanceId = SliceInstanceId;

        /**
         * For each entity, flags which may affect slice data inheritance.
         *
         * For example, if an entity has a component field flagged with ForceOverride,
         * that field never inherits the value from its the corresponding entity in the reference slice.
         *
         * Data flags affect how inheritance works, but the flags themselves are not inherited.
         * Data flags live at a particular level in an entity's slice ancestry.
         * For this reason, data flags are not stored within an entity or its components
         * (all of which are inherited by slice instances).
         *
         * See @ref DataPatch::Flags, @ref DataPatch::Flag, @ref DataPatch::FlagsMap for more.
         *
         * Care is taken to prune this datastructure so that no empty entries are stored.
         */
        class DataFlagsPerEntity
        {
        public:
            AZ_CLASS_ALLOCATOR(DataFlagsPerEntity, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(DataFlagsPerEntity, "{57FE7B9E-B2AF-4F6F-9F8D-87F671E91C99}");
            static void Reflect(ReflectContext* context);

            /// Function used to check whether a given entity ID is allowed.
            using IsValidEntityFunction = AZStd::function<bool(EntityId)>;

            DataFlagsPerEntity(const IsValidEntityFunction& isValidEntityFunction = nullptr);

            void CopyDataFlagsFrom(const DataFlagsPerEntity& rhs);
            void CopyDataFlagsFrom(DataFlagsPerEntity&& rhs);

            /// Return all data flags for this entity.
            /// Addresses are relative the entity.
            const DataPatch::FlagsMap& GetEntityDataFlags(EntityId entityId) const;

            /// Set all data flags for this entity.
            /// Addresses should be relative the entity.
            /// @return True if flags are set. False if IsValidEntity fails.
            bool SetEntityDataFlags(EntityId entityId, const DataPatch::FlagsMap& dataFlags);

            /// Clear all data flags for this entity.
            /// @return True if flags are cleared. False if IsValidEntity fails.
            bool ClearEntityDataFlags(EntityId entityId);

            /// Get the data flags set at a particular address within this entity.
            DataPatch::Flags GetEntityDataFlagsAtAddress(EntityId entityId, const DataPatch::AddressType& dataAddress) const;

            /// Set the data flags at a particular address within this entity.
            /// @return True if flags are set. False if IsValidEntity fails.
            bool SetEntityDataFlagsAtAddress(EntityId entityId, const DataPatch::AddressType& dataAddress, DataPatch::Flags flags);

            /// @return True if entityId passes the IsValidEntityFunction.
            /// Always returns true if no IsValidEntityFunction is set.
            bool IsValidEntity(EntityId entityId) const;

            /// Discard any entries for entities or addresses that no longer exist
            void Cleanup(const EntityList& validEntities);

        private:
            AZStd::unordered_map<EntityId, DataPatch::FlagsMap> m_entityToDataFlags;
            IsValidEntityFunction m_isValidEntityFunction;

            // Prevent copying whole class, since user probably doesn't want to change the IsValidEntityFunction.
            DataFlagsPerEntity(const DataFlagsPerEntity&) = delete;
            DataFlagsPerEntity& operator=(const DataFlagsPerEntity&) = delete;

        };
        using EntityDataFlagsMap = AZStd::unordered_map<EntityId, DataPatch::FlagsMap>;

        /**
        * Represents an ancestor of a live entity within a slice.
        * GetInstanceEntityAncestry can be used to retrieve all ancestral entities for a given entity,
        * or in other words, retrieve the corresponding entity at each level of the slice data hierarchy.
        */
        struct Ancestor
        {
            Ancestor()
                : m_entity(nullptr)
                , m_sliceAddress(nullptr, nullptr)
            {
            }
            Ancestor(AZ::Entity* entity, const SliceInstanceAddress& sliceAddress)
                : m_entity(entity)
                , m_sliceAddress(sliceAddress)
            {
            }
            AZ::Entity* m_entity;
            SliceInstanceAddress m_sliceAddress;
        };

        using EntityAncestorList = AZStd::vector<Ancestor>;

        SliceComponent();
        ~SliceComponent() override;

        /**
         * Container for instantiated entities (which are not serialized), but build from the source object and deltas \ref DataPatch.
         */
        struct InstantiatedContainer
        {
            AZ_CLASS_ALLOCATOR(InstantiatedContainer, SystemAllocator, 0);
            AZ_TYPE_INFO(InstantiatedContainer, "{05038EF7-9EF7-40D8-A29B-503D85B85AF8}");

            ~InstantiatedContainer();

            void DeleteEntities();

            /**
            * Non-Destructively clears all data.
            * @note Only call this when using this type as a temporary object to store pointers to data owned elsewhere.
            */
            void ClearAndReleaseOwnership();

            EntityList m_entities;
            EntityList m_metadataEntities;
        };

        /**
         * Stores information required to restore an entity back into an internal slice instance, which 
         * includes the address of the reference and owning instance at the time of capture.
         * EntityRestoreInfo must be retrieved via SliceReference::GetEntityRestoreInfo(). It can then be
         * provided to SliceComponent::RestoreEntity() to restore the entity, at which point the owning
         * reference and instance will be re-created if needed.
         */
        struct EntityRestoreInfo
        {
            AZ_TYPE_INFO(EntityRestoreInfo, "{AF2BE53F-C212-4BA4-880C-E1859BE75EA9}");

            EntityRestoreInfo()
            {}

            EntityRestoreInfo(const Data::Asset<SliceAsset>& asset, const SliceInstanceId& instanceId, const EntityId& ancestorId, const DataPatch::FlagsMap& dataFlags)
                : m_assetId(asset.GetId())
                , m_instanceId(instanceId)
                , m_ancestorId(ancestorId)
                , m_dataFlags(dataFlags)
            {}

            inline operator bool() const
            {
                return m_assetId.IsValid();
            }

            Data::AssetId       m_assetId;
            SliceInstanceId    m_instanceId;
            EntityId            m_ancestorId;
            DataPatch::FlagsMap m_dataFlags;
        };

        /**
         * Represents a slice instance in the current slice. For example if you refer to a base slice "lamppost"
         * you can have multiple instances of that slice (all of them with custom overrides) in the current slice.
         */
        class SliceInstance
        {
            friend class SliceComponent;
        public:

            AZ_TYPE_INFO(SliceInstance, "{E6F11FB3-E9BF-43BA-BD78-2A19F51D0ED3}");

            SliceInstance(const SliceInstanceId& id = SliceInstanceId::CreateRandom());
            SliceInstance(SliceInstance&& rhs);
            SliceInstance(const SliceInstance& rhs);
            ~SliceInstance();

            /// Returns pointer to the instantiated entities. Null if the slice was not instantiated.
            const InstantiatedContainer* GetInstantiated() const
            {
                return m_instantiated;
            }

            /// Returns a reference to the data patch, it's always available even when the \ref DataPatch contain no data to apply (no deltas)
            const DataPatch& GetDataPatch() const
            {
                return m_dataPatch;
            }

            /// Returns a reference to the data flags per entity.
            const DataFlagsPerEntity& GetDataFlags() const
            {
                return m_dataFlags;
            }

            /// Returns a reference to the data flags per entity.
            DataFlagsPerEntity& GetDataFlags()
            {
                return m_dataFlags;
            }

            /// Returns the EntityID of the from the base entity ID, to the new Entity ID that we will instantiate with this instance.
            /// @Note: This map may contain mappings to inactive entities.
            const EntityIdToEntityIdMap& GetEntityIdMap() const
            {
                return m_baseToNewEntityIdMap;
            }

            /// Returns the reverse of \ref GetEntityIdMap. The reverse table is build on demand.
            /// @Note: This map may contain mappings to inactive entities.
            const EntityIdToEntityIdMap& GetEntityIdToBaseMap() const
            {
                if (m_entityIdToBaseCache.empty())
                {
                    BuildReverseLookUp();
                }
                return m_entityIdToBaseCache;
            }

            /// Returns the instance's unique Id.
            const SliceInstanceId& GetId() const
            {
                return m_instanceId;
            }

            // Instances are stored in a set - AZStd::hash<> requirement.
            operator size_t() const
            {
                return m_instanceId.GetHash();
            }

            // Returns the metadata entity for this instance.
            Entity* GetMetadataEntity() const;

        protected:

            void SetId(const SliceInstanceId& id)
            {
                m_instanceId = id;
            }

            /// Returns data flags whose addresses align with those in the data patch.
            DataPatch::FlagsMap GetDataFlagsForPatching() const;

            static DataFlagsPerEntity::IsValidEntityFunction GenerateValidEntityFunction(const SliceInstance*);

            // The lookup is built lazily when accessing the map, but constness is desirable
            // in the higher level APIs.
            void BuildReverseLookUp() const;
            InstantiatedContainer* m_instantiated; ///< Runtime only list of instantiated objects (serialization stores the delta \ref m_dataPatch only)
            mutable EntityIdToEntityIdMap m_entityIdToBaseCache; ///< reverse lookup to \ref m_baseToNewEntityIdMap, this is build on demand

            EntityIdToEntityIdMap m_baseToNewEntityIdMap; ///< Map of old entityId to new
            DataPatch m_dataPatch; ///< Stored data patch which will take us from the dependent slice to the instantiated entities. Addresses are relative to the InstantiatedContainer.
            DataFlagsPerEntity m_dataFlags; ///< For each entity, flags which may affect slice data inheritance. Addresses are relative to the entity, not the InstantiatedContainer.

            SliceInstanceId m_instanceId; ///< Unique Id of the instance.

            /// Pointer to the clone metadata entity associated with the asset used to instantiate this class.
            /// Data is owned the m_instantiated member of this class.
            Entity* m_metadataEntity;
        };

        /**
         * Reference to a dependent slice. Each dependent slice can have one or more
         * instances in the current slice.
         */
        class SliceReference
        {
            friend class SliceComponent;
        public:
            using SliceInstances = AZStd::unordered_set<SliceInstance>;

            /// @deprecated Use SliceInstances
            using PrefabInstances = SliceInstances;

            AZ_TYPE_INFO(SliceReference, "{F181B80D-44F0-4093-BB0D-C638A9A734BE}");

            SliceReference();

            /**
             * Create a new instance of the slice (with new IDs for every entity).
             * \param customMapper Used to generate runtime entity ids for the new instance. By default runtime ids are randomly generated.
             */
            SliceInstance* CreateInstance(const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customMapper = nullptr);

            /** 
             * Clones an existing slice instance
             * \param instance Source slice instance to be cloned
             * \param sourceToCloneEntityIdMap [out] The map between source entity ids and clone entity ids
             * \return A clone of \ref instance
             */
            SliceInstance* CloneInstance(SliceInstance* instance, EntityIdToEntityIdMap& sourceToCloneEntityIdMap);

            /// Locates an instance by Id.
            SliceInstance* FindInstance(const SliceInstanceId& instanceId);

            /// Removes an instance of the slice.
            bool RemoveInstance(SliceInstance* instance);

            /// Remove an entity if belongs to some of the instances
            bool RemoveEntity(EntityId entityId, bool isDeleteEntity, SliceInstance* instance = nullptr);

            /// Retrieves the list of instances for this reference.
            const SliceInstances& GetInstances() const;

            /// Retrieves the underlying asset pointer.
            const Data::Asset<SliceAsset>& GetSliceAsset() const { return m_asset; }

            /// Retrieves the SliceComponent this reference belongs to.
            SliceComponent* GetSliceComponent() const { return m_component; }

            /// Checks if instances are instantiated
            bool IsInstantiated() const;

            /// Retrieves the specified entity's chain of ancestors and their associated assets along the slice data hierarchy.
            /// \param instanceEntityId - Must be Id of an entity within a live instance.
            /// \param ancestors - Output list of ancestors, up to maxLevels.
            /// \param maxLevels - Maximum cascade levels to explore.
            bool GetInstanceEntityAncestry(const AZ::EntityId& instanceEntityId, EntityAncestorList& ancestors, u32 maxLevels = 8) const;

            /// Compute data patch for all instances
            void ComputeDataPatch();

        protected:

            /// Creates a new Id'd instance slot internally, but does not instantiate it.
            SliceInstance* CreateEmptyInstance(const SliceInstanceId& instanceId = SliceInstanceId::CreateRandom());

            /// Instantiate all instances (by default we just hold the deltas - data patch), the Slice component controls the instantiate state
            bool Instantiate(const AZ::ObjectStream::FilterDescriptor& filterDesc);

            void UnInstantiate();

            void InstantiateInstance(SliceInstance& instance, const AZ::ObjectStream::FilterDescriptor& filterDesc);

            void AddInstanceToEntityInfoMap(SliceInstance& instance);

            void RemoveInstanceFromEntityInfoMap(SliceInstance& instance);

            bool m_isInstantiated;

            SliceComponent* m_component = nullptr;

            SliceInstances m_instances; ///< Instances of the slice in our slice reference

            Data::Asset<SliceAsset> m_asset; ///< Asset reference to a dependent slice reference
        };

        using SliceList = AZStd::list<SliceReference>; // we use list as we need stable iterators

        /**
         * Map from entity ID to entity and slice address (if it comes from a slice).
         */
        struct EntityInfo
        {
            EntityInfo()
                : m_entity(nullptr)
                , m_sliceAddress(nullptr, nullptr)
            {}

            EntityInfo(Entity* entity, const SliceInstanceAddress& sliceAddress)
                : m_entity(entity)
                , m_sliceAddress(sliceAddress)
            {}

            Entity* m_entity; ///< Pointer to the entity
            SliceInstanceAddress m_sliceAddress; ///< Address of the slice
        };
        using EntityInfoMap = AZStd::unordered_map<EntityId, EntityInfo>;

        SerializeContext* GetSerializeContext() const
        {
            return m_serializeContext;
        }

        void SetSerializeContext(SerializeContext* context)
        {
            m_serializeContext = context;
        }

        /// Connect to asset bus for dependencies.
        void ListenForAssetChanges();

        /// This will listen for child asset changes and instruct all other slices in the slice
        /// hierarchy to do the same.
        void ListenForDependentAssetChanges();

        /// Returns list of the entities that are "new" for the current slice (not based on an existing slice)
        const EntityList& GetNewEntities() const;

        /**
         * Returns all entities including the ones based on instances, you need to provide container as we don't
         * keep all entities in one list (we can change that if we need it easily).
         * If entities are not instantiated (the ones based on slices) \ref Instantiate will be called
         * \returns true if entities list has been populated (even if empty), and false if instantiation failed.
         */
        bool GetEntities(EntityList& entities);

        /**
        * Adds the IDs of all non-metadata entities, including the ones based on instances, to the provided set.
        * @param entities An entity ID set to add the IDs to
        */
        bool GetEntityIds(EntityIdSet& entities);

        /**
         * Returns the count of all instantiated entities, including the ones based on instances.
         * If the slice has not been instantiated then 0 is returned.
         */
        size_t GetInstantiatedEntityCount() const;

        /**
        * Adds ID of every metadata entity that's part of this slice, including those based on instances, to the
        * given set.
        * @param entities An entity ID set to add the IDs to
        */
        bool GetMetadataEntityIds(EntityIdSet& entities);

        /// Returns list of all slices and their instantiated entities (for this slice node)
        const SliceList& GetSlices() const;

        SliceReference* GetSlice(const Data::Asset<SliceAsset>& sliceAsset);
        SliceReference* GetSlice(const Data::AssetId& sliceAssetId);

        /**
         * Adds a dependent slice, and instantiate the slices if needed.
         * \param sliceAsset slice asset.
         * \param customMapper optional entity runtime id mapper.
         * \returns null if adding slice failed.
         */
        SliceInstanceAddress AddSlice(const Data::Asset<SliceAsset>& sliceAsset, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customMapper = nullptr);
        /// Adds a slice reference (moves it) along with its instance information.
        SliceReference* AddSlice(SliceReference& sliceReference);
        /// Adds a slice (moves it) from another already generated reference/instance pair.
        SliceInstanceAddress AddSliceInstance(SliceReference* sliceReference, SliceInstance* sliceInstance);

        /// Remove an entire slice asset reference and all its instances.
        bool RemoveSlice(const Data::Asset<SliceAsset>& sliceAsset);
        bool RemoveSlice(const SliceReference* slice);

        /// Removes the slice instance, if this is last instance the SliceReference will be removed too.
        bool RemoveSliceInstance(SliceInstance* instance);

        /// Adds entity to the current slice and takes ownership over it (you should not manage/delete it)
        void AddEntity(Entity* entity);

        /**
        * Find an entity in the slice by entity Id.
        */
        AZ::Entity* FindEntity(EntityId entityId);

        /*
         * Removes and/or deletes an entity from the current slice by pointer. We delete by default it because we own the entities.
         * \param entity entity pointer to be removed, since this entity should already exists this function will NOT instantiate,
         * as this should have already be done to have a valid pointer.
         * \param isDeleteEntity true by default as we own all entities, pass false to just remove the entity and get ownership of it.
         * \param isRemoveEmptyInstance if this entity is the last in an instance, it removes the instance (default)
         * \returns true if entity was removed, otherwise false
         */
        bool RemoveEntity(Entity* entity, bool isDeleteEntity = true, bool isRemoveEmptyInstance = true);

        /// Same as \ref RemoveEntity but by using entityId
        bool RemoveEntity(EntityId entityId, bool isDeleteEntity = true, bool isRemoveEmptyInstance = true);

        /**
         * Returns the slice information about an entity, if it belongs to the component and is in a slice.
         * \param entity pointer to instantiated entity
         * \returns pair of SliceReference and SliceInstance if the entity is in a "child" slice, otherwise null,null. This is true even if entity
         * belongs to the slice (but it's not in a "child" slice)
         */
        SliceInstanceAddress FindSlice(Entity* entity);
        SliceInstanceAddress FindSlice(EntityId entityId);
        
        /**
         * Extracts data required to restore the specified entity.
         * \return false if the entity is not part of an internal instance, and doesn't require any special restore operations.
         */
        bool GetEntityRestoreInfo(const AZ::EntityId entityId, EntityRestoreInfo& info);

        /**
         * Adds an entity back to an existing instance.
         * This create a reference for the specified asset if it doesn't already exist.
         * If the reference exists, but the instance with the specified id does not, one will be created.
         * Ownership of the entity is transferred to the instance.
         * \param asset/reference the instance belongs to.
         * \param Id of the instance the entity will be added to.
         * \param entity to add to the instance.
         * \param ancestorId must be the original ancestor's entity Id in order to relate the entity to its source in the asset.
         */
        SliceInstanceAddress RestoreEntity(AZ::Entity* entity, const EntityRestoreInfo& restoreInfo);

        /**
         * Sets the asset that owns this component, which allows us to listen for asset changes.
         */
        void SetMyAsset(SliceAsset* asset)
        {
            m_myAsset = asset;
        }

        /**
        * Gets the asset that owns this component.
        */
        const SliceAsset* GetMyAsset() const
        {
            return m_myAsset;
        }

        /** 
        * Appends the metadata entities belonging only directly to each instance in the slice to the given list. Metadata entities belonging
        * to any instances within each instance are omitted.
        */
        bool GetInstanceMetadataEntities(EntityList& outMetadataEntities);

        /**
        * Appends all metadata entities belonging to instances owned by this slice, including those in nested instances to the end of the given list.
        */
        bool GetAllInstanceMetadataEntities(EntityList& outMetadataEntities);

        /**
        * Gets all metadata entities belonging to this slice. This includes instance metadata components and the slice's own metadata component.
        * Because the contents of the result could come from multiple objects, the calling function must provide its own container.
        * Returns true if the operation succeeded (Even if the resulting container is empty)
        */
        bool GetAllMetadataEntities(EntityList& outMetadataEntities);

        /**
        * Gets the metadata entity belonging to this slice
        */
        AZ::Entity* GetMetadataEntity();

        typedef AZStd::unordered_set<Data::AssetId> AssetIdSet;

        /**
         * Gathers referenced slice assets for this slice (slice assets this contains, slice assets this depends on)
         * \param idSet the container that contains the referenced assets after this function is called
         * \param recurse whether to recurse. true by default so you get ALL referenced assets, false will return only directly-referenced assets
         */
        void GetReferencedSliceAssets(AssetIdSet& idSet, bool recurse = true);

        /** 
         * Clones the slice, its references, and its instances. Entity Ids are not regenerated
         * during this process. This utility is currently used to clone slice asset data
         * for data-push without modifying the existing asset in-memory.
         * \param serializeContext SerializeContext to use for cloning
         * \param sourceToCloneSliceInstanceMap [out] (optional) The map between source SliceInstances and cloned SliceInstances.
         * \return A clone of this SliceComponent
         */
        SliceComponent* Clone(AZ::SerializeContext& serializeContext, SliceInstanceToSliceInstanceMap* sourceToCloneSliceInstanceMap = nullptr) const;

        /**
         * Set whether the slice can be instantiated despite missing dependent slices (allowed by default).
         */
        void AllowPartialInstantiation(bool allow)
        {
            m_allowPartialInstantiation = allow;
        }

        /**
         * Returns whether or not the slice allows partial instantiation.
         */
        bool IsAllowPartialInstantiation() const
        {
            return m_allowPartialInstantiation;
        }

        /**
         * Returns whether or not this is a dynamic slice to be exported for runtime instantiation.
         */
        bool IsDynamic() const
        {
            return m_isDynamic;
        }

        /**
         * Designates this slice as dynamic (can be instantiated at runtime).
         */
        void SetIsDynamic(bool isDynamic)
        {
            m_isDynamic = isDynamic;
        }

        /**
        * Instantiate entities for this slice, otherwise only the data are stored.
        */
        bool Instantiate();
        bool IsInstantiated() const;
        /**
         * Generate new entity Ids and remap references
         * \param previousToNewIdMap Output map of previous entityIds to newly generated entityIds
         */
        void GenerateNewEntityIds(EntityIdToEntityIdMap* previousToNewIdMap = nullptr);

        /**
        * Newly created slices and legacy slices won't have required metadata components. This will check to see if
        * necessary components to the function of the metadata entities are present and
        * \param instance Source slice instance
        */
        void InitMetadata();

    protected:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
        // Workaround for VS2013 - Delete the copy constructor and make it private
        // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
        SliceComponent(const SliceComponent&) = delete;
#endif

        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Asset Bus
        void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        /// \ref ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        /// \ref ComponentDescriptor::GetDependentServices
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);
        /// \ref ComponentDescriptor::Reflect
        static void Reflect(ReflectContext* reflection);

        /// Prepare component entity for save, if this is a leaf entity, data patch will be build.
        void PrepareSave();

        /// Populate the entity info map. This will re-populate it even if already populated.
        void BuildEntityInfoMap();

        /**
        * During instance instantiation, entities from root slices may be removed by data patches. We need to remove these
        * from the metadata associations in our newly cloned instance metadata entities.
        */
        void CleanMetadataAssociations();
        
        /// Returns the entity info map (and builds it if necessary).
        EntityInfoMap& GetEntityInfoMap();

        /// Returns the reference associated with the specified asset Id. If none is present, one will be created.
        SliceReference* AddOrGetSliceReference(const Data::Asset<SliceAsset>& sliceAsset);

        /// Removes a slice reference (and all instances) by iterator.
        void RemoveSliceReference(SliceComponent::SliceList::iterator sliceReferenceIt);

        /// Utility function to apply a EntityIdToEntityIdMap to a EntityIdToEntityIdMap (the mapping will override values in the destination)
        static void ApplyEntityMapId(EntityIdToEntityIdMap& destination, const EntityIdToEntityIdMap& mapping);

        /// Utility function to add an assetId to the cycle checker vector
        void PushInstantiateCycle(const AZ::Data::AssetId& assetId);

        /// Utility function to check if the given assetId would cause an instantiation cycle, and if so
        /// output the chain of slices that causes the cycle.
        bool CheckContainsInstantiateCycle(const AZ::Data::AssetId& assetId);
        
        /// Utility function to pop an assetId to the cycle checker vector (also checks to make sure its at the tail of it and clears it)
        void PopInstantiateCycle(const AZ::Data::AssetId& assetId);

        SliceAsset* m_myAsset;   ///< Pointer to the asset we belong to, note this is just a reference stored by the handler, we don't need Asset<SliceAsset> as it's not a reference to another asset.

        SerializeContext* m_serializeContext;

        EntityInfoMap m_entityInfoMap; ///< Build on demand usually for accelerate tools look ups.

        EntityList m_entities;  ///< Entities that are new (not based on a slice).

        SliceList m_slices; ///< List of base slices and their instances in the world

        AZ::Entity m_metadataEntity; ///< Entity for attaching slice metadata components

        AZStd::atomic<bool> m_slicesAreInstantiated; ///< Instantiate state of the base slices (they should be instantiated or not)

        bool m_allowPartialInstantiation; ///< Instantiation is still allowed even if dependencies are missing.

        bool m_isDynamic;   ///< Dynamic slices are available for instantiation at runtime.

        AZ::Data::AssetFilterCB m_assetLoadFilterCB; ///< Asset load filter callback to apply for internal loads during data patching.
        AZ::u32 m_filterFlags; ///< Asset load filter flags to apply for internal loads during data patching

        AZStd::recursive_mutex m_instantiateMutex; ///< Used to prevent multiple threads from trying to instantiate the slices at once
        
        typedef AZStd::vector<AZ::Data::AssetId> AssetIdVector;

        // note that this vector provides a "global" view of instantiation occurring and is only accessed while m_instantiateMutex is locked.
        // in addition, it only temporarily contains data - during instantiation it is the stack of assetIds that is in the current branch
        // of instantiation, and when we finish instantiation and return from the recursive call it is emptied and the memory is freed, so
        // there should be no risk of leaking it or having it survive beyond the allocator's existence.
        static AssetIdVector m_instantiateCycleChecker; ///< Used to prevent cyclic dependencies
    };

    /// @deprecated Use SliceComponent.
    using PrefabComponent = SliceComponent;
} // namespace AZ
