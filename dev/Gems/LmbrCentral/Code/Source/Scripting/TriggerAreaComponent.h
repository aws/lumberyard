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

#include <AzCore/std/containers/vector.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzFramework/Network/NetBindable.h>
#include <GridMate/Replica/ReplicaCommon.h>
#include <GridMate/Replica/ReplicaChunk.h>

#include <IProximityTriggerSystem.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <LmbrCentral/Scripting/TriggerAreaComponentBus.h>
struct SProximityElement;

namespace LmbrCentral
{
    class TriggerAreaReplicaChunkDesc;
    class TriggerAreaReplicaChunk;

    /**
     * TriggerArea Component
     *
     * Simple component that registers a proximity area with the ProximityTriggerSystem.
     *
     * Entities linked to the component by the designer will have the configured actions applied when
     * the trigger area is interacted with by eligible entities in the world.
     */
    class TriggerAreaComponent
        : public AZ::Component
        , ProximityTriggerEventBus::Handler
        , public AzFramework::NetBindable
        , public AZ::TickBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , private TriggerAreaRequestsBus::Handler
        , private ShapeComponentNotificationsBus::Handler
        , private TagComponentNotificationsBus::MultiHandler
    {
        friend TriggerAreaReplicaChunk;
        friend TriggerAreaReplicaChunkDesc;
    public:
        AZ_COMPONENT(TriggerAreaComponent, "{E3DF5790-F0AD-43AE-9FB2-0A37F873DECB}", AzFramework::NetBindable);

        friend class EditorTriggerAreaComponent;

        TriggerAreaComponent();
        ~TriggerAreaComponent() override {}

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TriggerAreaRequestsBus::Handler
        void AddRequiredTag(const Tag& requiredTag) override;
        void RemoveRequiredTag(const Tag& requiredTag) override;

        void AddExcludedTag(const Tag& excludedTag) override;
        void RemoveExcludedTag(const Tag& excludedTag) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& parentLocalTM, const AZ::Transform& parentWorldTM) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ProximityTriggerEventBus::Handler
        void OnTriggerEnter(AZ::EntityId entityId) override;
        void OnTriggerExit(AZ::EntityId entityId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // NetBindable
        GridMate::ReplicaChunkPtr GetNetworkBinding() override;
        void SetNetworkBinding(GridMate::ReplicaChunkPtr chunk) override;
        void UnbindFromNetwork() override;

        bool OnEntityEnterAreaRPC(AZ::u64 staticEntityId, const GridMate::RpcContext& rpcContext);
        bool OnEntityExitAreaRPC(AZ::u64 staticEntityId, const GridMate::RpcContext& rpcContext);
        // End of NetBindable
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShapeComponentNotificationsBus::Handler
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TagComponentNotificationsBus::MultiHandler
        void OnTagAdded(const Tag&) override;
        void OnTagRemoved(const Tag&) override;
        //////////////////////////////////////////////////////////////////////////

        AZ::Crc32 OnActivatedByComboBoxChanged() { return AZ::Edit::PropertyRefreshLevels::EntireTree; }

        /**
        * \brief Does a narrow pass check to find out if the indicated point is inside the shape attached to this entity
        * \param position 3D vector indicating the position to be checked
        * \return boolean value indicating whether the position is inside the shape or not
        */
        bool NarrowPassCheck(const AZ::Vector3& position) const;

    private:

        void CreateProximityTrigger();
        void RemoveProximityTrigger();

        bool OnTriggerEnterImpl(AZ::EntityId entityId);
        bool OnTriggerExitImpl(AZ::EntityId entityId);

        void UpdateTriggerArea();

        /**
         * The category of entities able to interact with the trigger area.
         */
        enum class ActivationEntityType
        {
            AllEntities,
            SpecificEntities,
        };

        //! Adds a required tag without Reevaluating trigger (used for building game entities)
        bool AddRequiredTagInternal(const Tag& requiredTag);
        bool AddExcludedTagInternal(const Tag& excludedTag);

        /// Called when a valid entity has entered the trigger area.
        void HandleEnter();

        //! Enumerates the results of the filtering pass done on entities
        enum class FilteringResult
        {
            // Entity Passes all filters
            Pass,
            // Entity failed a filter that can change (eg. tags did not match)
            FailWithPossibilityOfChange,
            // Entity failed a filter that cannot change (eg. not one of the specified entities)
            FailWithoutPossibilityOfChange
        };

        /// Checks the supplied entity Id against the m_activationEntityType filter.
        FilteringResult EntityPassesFilters(AZ::EntityId id) const;

        /// Debug-draw the trigger.
        void DebugDraw() const;

        bool IsNetworkControlled() const;
        void EnforceProximityTriggerState();

        /// Reevaluates tags for all entities in the Area
        void ReevaluateTagsAllEntities();

        /// Handles changes in tags on any entities that are inside the Trigger volume
        void HandleTagChange(const AZ::EntityId& entityId);

        ActivationEntityType            m_activationEntityType;     ///< The types of entities able to interact with the trigger area.
        bool                            m_triggerOnce;              ///< Only trigger once.
        AZStd::vector<AZ::EntityId>     m_specificInteractEntities; ///< Specific list of entities that can interact with the trigger, if m_activationEntityType is SpecificEntities.
        AZStd::vector<AZ::EntityId>     m_entitiesInside;           ///< Tracking of valid entities currently inside the trigger.
        AZStd::vector<AZ::EntityId>     m_entitiesInsideExcludedByTags;           ///< Tracking of valid entities currently spatially inside the trigger but are logically evicted
        SProximityElement*              m_proximityTrigger;         ///< The proximity trigger instance per the ProximityTriggerSystem.

        GridMate::ReplicaChunkPtr       m_replicaChunk;

        //! The tags that are required to trigger this Area
        AZStd::vector<LmbrCentral::Tag> m_requiredTags;

        //! The tags that exclude an entity from triggering this Area
        AZStd::vector<LmbrCentral::Tag> m_excludedTags;

        bool UseSpecificEntityList() const { return m_activationEntityType == ActivationEntityType::SpecificEntities; }

        //////////////////////////////////////////////////////////////////////////
        // Component descriptor

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ProximityTriggerService", 0x561f262c));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        }

        /// Determines if a given entity is controlled by a player (legacy check = has a net channel).
        static bool IsPlayer(AZ::EntityId entityId);

        /// Determines if a given entity is controlled by the local player (legacy check = has our net channel).
        static bool IsLocalPlayer(AZ::EntityId entityId);

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //! Caches the AABB currently being used for triggering
        AABB m_cachedAABB;
    };
} // namespace LmbrCentral
