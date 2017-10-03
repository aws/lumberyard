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

#include <AzFramework/Network/NetBindingSystemBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzFramework
{
    /**
    */
    class BindRequest
    {
    public:
        BindRequest()
            : m_bindTo(GridMate::InvalidReplicaId)
        {
        }

        GridMate::ReplicaId m_bindTo;
        AZ::EntityId m_desiredRuntimeEntityId;
        AZ::EntityId m_actualRuntimeEntityId;
        AZStd::chrono::system_clock::time_point m_requestTime;
    };

    typedef AZStd::unordered_map<AZ::EntityId, BindRequest> BindRequestContainerType;

    /**
    */
    class NetBindingSliceInstantiationHandler
        : public SliceInstantiationResultBus::Handler
    {
    public:
        void InstantiateEntities();
        bool IsInstantiated();
        bool IsBindingComplete();

        //////////////////////////////////////////////////////////////////////////
        // SliceInstantiationResultBus
        void OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId) override;
        //////////////////////////////////////////////////////////////////////////

        AZ::Data::AssetId m_sliceAssetId;
        BindRequestContainerType m_bindingQueue;
        SliceInstantiationTicket m_ticket;
    };

    /**
    * NetBindingSystemImpl works in conjunction with NetBindingComponent and
    * NetBindingComponentChunk to perform network binding for game entities.
    *
    * It is responsible for adding entity replicas to the network on the master side
    * and servicing entity spawn requests from the network on the proxy side, as
    * well as detecting network availability and triggering network binding/unbinding.
    *
    * The system is first activated on the host side when OnNetworkSessionActivated event
    * is received, and NetBindingSystemContextData is created.
    * The system becomes fully operational when the NetBindingSystemContextData is activated
    * and bound to the system, and remains operational as long as the NetBindingSystemContextData
    * remains valid.
    *
    * Level switching is tracked by a monotonically increasing context sequence number controlled
    * by the host. Spawn and bind operations are deferred until the correct sequence number
    * is reached. Spawning is always performed from the game thread.
    */
    class NetBindingSystemImpl
        : public NetBindingSystemBus::Handler
        , public NetBindingSystemEventsBus::Handler
        , public EntityContextEventBus::Handler
        , public AZ::TickBus::Handler
    {
        friend class NetBindingSystemContextData;

    public:
        NetBindingSystemImpl();
        ~NetBindingSystemImpl() override;

        static void Reflect(AZ::ReflectContext* context);

        virtual void Init();
        virtual void Shutdown();

        //////////////////////////////////////////////////////////////////////////
        // NetBindingSystemBus
        bool ShouldBindToNetwork() override;
        NetBindingContextSequence GetCurrentContextSequence() override;
        void AddReplicaMaster(AZ::Entity* entity, GridMate::ReplicaPtr replica) override;
        AZ::EntityId GetStaticIdFromEntityId(AZ::EntityId entity) override;
        AZ::EntityId GetEntityIdFromStaticId(AZ::EntityId staticEntityId) override;
        void SpawnEntityFromSlice(GridMate::ReplicaId bindTo, const NetBindingSliceContext& bindToContext) override;
        void SpawnEntityFromStream(AZ::IO::GenericStream& spawnData, AZ::EntityId useEntityId, GridMate::ReplicaId bindTo, NetBindingContextSequence addToContext) override;
        void OnNetworkSessionActivated(GridMate::GridSession* session) override;
        void OnNetworkSessionDeactivated(GridMate::GridSession* session) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EntityContextEventBus::Handler
        void OnEntityContextReset() override;
        void OnEntityContextLoadedFromStream(const AZ::SliceComponent::EntityList& contextEntities) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        //! Called by the NetBindingContext chunk when it is activated
        void OnContextDataActivated(GridMate::ReplicaChunkPtr contextData);

        //! Called by the NetBindingContext chunk when it is deactivated
        void OnContextDataDeactivated(GridMate::ReplicaChunkPtr contextData);

        //! Update the current binding context sequence
        virtual void UpdateContextSequence();

        //! Process pending spawn requests
        virtual void ProcessSpawnRequests();

        //! Process pending bind requests
        virtual void ProcessBindRequests();

        //! Performs final stage of entity spawning process
        virtual void BindAndActivate(AZ::Entity* entity, GridMate::ReplicaId replicaId, bool addToContext);

        //! Called on the host to spawn the net binding system replica
        virtual GridMate::Replica* CreateSystemReplica();

        AZ_FORCE_INLINE bool ReadyToAddReplica() const;

        class SpawnRequest
        {
        public:
            GridMate::ReplicaId m_bindTo;
            AZ::EntityId m_useEntityId;
            AZStd::vector<AZ::u8> m_spawnDataBuffer;
        };

        typedef AZStd::list<SpawnRequest> SpawnRequestContainerType;
        typedef AZStd::map<NetBindingContextSequence, SpawnRequestContainerType> SpawnRequestContextContainerType;

        typedef AZStd::unordered_map<AZ::EntityId, NetBindingSliceInstantiationHandler> SliceRequestContainerType;
        typedef AZStd::map<NetBindingContextSequence, SliceRequestContainerType> BindRequestContextContainerType;

        GridMate::GridSession* m_bindingSession;
        GridMate::ReplicaChunkPtr m_contextData;
        NetBindingContextSequence m_currentBindingContextSequence;
        SpawnRequestContextContainerType m_spawnRequests;
        BindRequestContextContainerType m_bindRequests;
        AZStd::list<AZStd::pair<AZ::EntityId, GridMate::ReplicaPtr>> m_addMasterRequests;

        /**
         * \brief override how root slice entities' replicas should be loaded
         *
         * We occasionally get GameContextBridge replica (that tells us what level to load) before we get
         * a replica that tells us that we are connecting to a network sessions, thus we may not figure out in time if we
         * need to load the root slice entities with NetBindingComponent as master replicas or proxy replicas.
         * This is a fix until proper order is established.
         *
         * \param isAuthoritative true if root slice entities with NetBindingComponents to be loaded authoritatively
         */
        void OverrideRootSliceLoadMode(bool isAuthoritative)
        {
            m_isAuthoritativeRootSliceLoad = isAuthoritative;
            m_overrideRootSliceLoadAuthoritative = true;
        }

    private:
        /**
         * \brief True if the root slice is to be loaded authoritatively
         */
        bool m_isAuthoritativeRootSliceLoad;
        /**
         * \brief True if root slice loading mode was overriden, otherwise the mode would be determined via m_bindingSession
         */
        bool m_overrideRootSliceLoadAuthoritative;
        /**
         * \brief A helper method to figure the mode of loading root slice entities' replicas
         * \return True if the root slice entities is to be loaded authoritatively
         */
        bool IsAuthoritateLoad() const;
    };
} // namespace AzFramework

