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
#include "LmbrCentral_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/TickBus.h>

#include <MathConversion.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Network/NetBindingSystemBus.h>
#include <AzFramework/Network/NetworkContext.h>

#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Serialize/MarshalerTypes.h>
#include <GridMate/Serialize/MathMarshal.h>
#include <GridMate/Serialize/DataMarshal.h>
#include <GridMate/Serialize/ContainerMarshal.h>

#include "TriggerAreaComponent.h"
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <IRenderAuxGeom.h>
#include <IActorSystem.h>

namespace LmbrCentral
{
    // BehaviorContext TriggerAreaNotificationBus forwarder
    class BehaviorTriggerAreaNotificationBusHandler : public TriggerAreaNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorTriggerAreaNotificationBusHandler, "{D3C88952-AA9A-48AA-8C2C-9011020A6AEA}", AZ::SystemAllocator,
            OnTriggerAreaEntered, OnTriggerAreaExited);

        void OnTriggerAreaEntered(AZ::EntityId id)
        {
            Call(FN_OnTriggerAreaEntered, id);
        }

        void OnTriggerAreaExited(AZ::EntityId id)
        {
            Call(FN_OnTriggerAreaExited, id);
        }
    };
    
    // BehaviorContext TriggerAreaEntityNotificationBus forwarder
    class BehaviorTriggerAreaEntityNotificationBusHandler : public TriggerAreaEntityNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorTriggerAreaEntityNotificationBusHandler,"{4CCF51B5-DF0A-436F-97BD-9EF208D17CC8}", AZ::SystemAllocator,
            OnEntityEnteredTriggerArea, OnEntityExitedTriggerArea);

        void OnEntityEnteredTriggerArea(AZ::EntityId id)
        {
            Call(FN_OnEntityEnteredTriggerArea, id);
        }

        void OnEntityExitedTriggerArea(AZ::EntityId id)
        {
            Call(FN_OnEntityExitedTriggerArea, id);
        }
    };

    //=========================================================================
    // ReplicaChunk
    //=========================================================================
    class TriggerAreaReplicaChunk : public GridMate::ReplicaChunkBase
    {
    public:
        AZ_CLASS_ALLOCATOR(TriggerAreaReplicaChunk, AZ::SystemAllocator, 0);

        static const char* GetChunkName() { return "TriggerAreaChunk"; }

        TriggerAreaReplicaChunk()
            : OnAreaEnter("OnAreaEnter")
            , OnAreaExit("OnAreaExit")
        {
        }

        bool IsReplicaMigratable() override
        {
            return true;
        }                

        void OnReplicaChangeOwnership(const GridMate::ReplicaContext& /*rc*/) override
        {
            TriggerAreaComponent* triggerAreaComponent = static_cast<TriggerAreaComponent*>(GetHandler());

            if (triggerAreaComponent)
            {
                triggerAreaComponent->EnforceProximityTriggerState();
            }
        }

        GridMate::Rpc< GridMate::RpcArg<AZ::u64> >::BindInterface<TriggerAreaComponent,&TriggerAreaComponent::OnEntityEnterAreaRPC> OnAreaEnter;
        GridMate::Rpc< GridMate::RpcArg<AZ::u64> >::BindInterface<TriggerAreaComponent,&TriggerAreaComponent::OnEntityExitAreaRPC> OnAreaExit;

        // Only used to pass along the initial set, after that all of the enter/exits will take care of updating the list.
        AZStd::vector< AZ::u64 > m_initialEntitiesInArea;
    };

    class TriggerAreaReplicaChunkDesc : public AzFramework::ExternalChunkDescriptor<TriggerAreaReplicaChunk>
    {
    public:        

        GridMate::ReplicaChunkBase* CreateFromStream(GridMate::UnmarshalContext& context) override
        {
            TriggerAreaReplicaChunk* triggerAreaChunk = aznew TriggerAreaReplicaChunk;            

            AZStd::vector<AZ::u64> initialEntitiesInArea;

            context.m_iBuf->Read(initialEntitiesInArea);

            triggerAreaChunk->m_initialEntitiesInArea.reserve(initialEntitiesInArea.size());

            for (AZ::u64 entityId : initialEntitiesInArea)
            {
                triggerAreaChunk->m_initialEntitiesInArea.push_back(entityId);
            }

            return triggerAreaChunk;
        }

        void DiscardCtorStream(GridMate::UnmarshalContext& context) override
        {
            AZStd::vector< AZ::u64 > discard;
            context.m_iBuf->Read(discard);
        }

        void MarshalCtorData(GridMate::ReplicaChunkBase* chunk, GridMate::WriteBuffer& wb) override
        {
            TriggerAreaReplicaChunk* triggerAreaChunk = static_cast<TriggerAreaReplicaChunk*>(chunk);
            TriggerAreaComponent* triggerAreaComponent = static_cast<TriggerAreaComponent*>(chunk->GetHandler());

            if (triggerAreaComponent)
            {
                AZStd::vector< AZ::u64 > entitiesInside;
                entitiesInside.reserve(triggerAreaComponent->m_entitiesInside.size());

                for (AZ::EntityId entityId : triggerAreaComponent->m_entitiesInside)
                {
                    AZ::u64 writableId = static_cast<AZ::u64>(entityId);
                    entitiesInside.push_back(writableId);
                }

                wb.Write(entitiesInside);
            }
            else
            {
                wb.Write(triggerAreaChunk->m_initialEntitiesInArea);
            }
        }

        void DeleteReplicaChunk(GridMate::ReplicaChunkBase* replica)
        {
            delete replica;
        }
    };



    //=========================================================================
    // Reflect
    //=========================================================================
    void TriggerAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TriggerAreaComponent, AZ::Component, AzFramework::NetBindable>()
                ->Version(2)
                ->Field("TriggerOnce", &TriggerAreaComponent::m_triggerOnce)
                ->Field("ActivatedBy", &TriggerAreaComponent::m_activationEntityType)
                ->Field("SpecificInteractEntities", &TriggerAreaComponent::m_specificInteractEntities)
                ->Field("RequiredTags", &TriggerAreaComponent::m_requiredTags)
                ->Field("ExcludedTags", &TriggerAreaComponent::m_excludedTags);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<TriggerAreaRequestsBus>("TriggerAreaRequestsBus")
                ->Event("AddRequiredTag", &TriggerAreaRequestsBus::Events::AddRequiredTag)
                ->Event("RemoveRequiredTag", &TriggerAreaRequestsBus::Events::RemoveRequiredTag)
                ->Event("AddExcludedTag", &TriggerAreaRequestsBus::Events::AddExcludedTag)
                ->Event("RemoveExcludedTag", &TriggerAreaRequestsBus::Events::RemoveExcludedTag)
                ;

            behaviorContext->EBus<TriggerAreaNotificationBus>("TriggerAreaNotificationBus")
                ->Handler<BehaviorTriggerAreaNotificationBusHandler>();

            behaviorContext->EBus<TriggerAreaEntityNotificationBus>("TriggerAreaEntityNotificationBus")
                ->Handler<BehaviorTriggerAreaEntityNotificationBusHandler>();
        }

        AzFramework::NetworkContext* netContext = azrtti_cast<AzFramework::NetworkContext*>(context);

        if (netContext)
        {
            netContext->Class<TriggerAreaComponent>()
                ->Chunk<TriggerAreaReplicaChunk,TriggerAreaReplicaChunkDesc>()
                    ->RPC<TriggerAreaReplicaChunk, TriggerAreaComponent>("OnAreaEnter",&TriggerAreaReplicaChunk::OnAreaEnter)
                    ->RPC<TriggerAreaReplicaChunk, TriggerAreaComponent>("OnAreaExit",&TriggerAreaReplicaChunk::OnAreaExit);
        }
    }

    //=========================================================================
    // Constructor
    //=========================================================================
    TriggerAreaComponent::TriggerAreaComponent()
        : m_activationEntityType(ActivationEntityType::AllEntities)
        , m_triggerOnce(false)
        , m_proximityTrigger(nullptr)
        , m_replicaChunk(nullptr)
    {
    }

    //=========================================================================
    // Activate
    //=========================================================================
    void TriggerAreaComponent::Activate()
    {
        if (!IsNetworkControlled())
        {
            EnforceProximityTriggerState();
            TriggerAreaRequestsBus::Handler::BusConnect(GetEntityId());
        }
    }

    void TriggerAreaComponent::CreateProximityTrigger()
    {
        AZ_Error("TriggerAreaComponent", m_proximityTrigger == nullptr, "Creating two proximity triggers to TriggerAreaComponent on Entity(%s)", GetEntity()->GetName().c_str());

        if (m_proximityTrigger == nullptr)
        {
            AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(entityTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);

            // Create an entry in the proximity trigger system.
            EBUS_EVENT_RESULT(m_proximityTrigger, ProximityTriggerSystemRequestBus, CreateTrigger, AZStd::bind(&TriggerAreaComponent::NarrowPassCheck, this, AZStd::placeholders::_1));
            AZ_Assert(m_proximityTrigger, "Failed to create proximity trigger.");
            m_proximityTrigger->id = GetEntityId();

            UpdateTriggerArea();

            ProximityTriggerEventBus::Handler::BusConnect(GetEntityId());
            ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        }
    }

    //=========================================================================
    // UpdateTriggerArea
    //=========================================================================
    void TriggerAreaComponent::UpdateTriggerArea()
    {
        AZ::Aabb encompassingAABB;
        EBUS_EVENT_ID_RESULT(encompassingAABB, GetEntityId(), ShapeComponentRequestsBus, GetEncompassingAabb);
        m_cachedAABB = AZAabbToLyAABB(encompassingAABB);
        EBUS_EVENT(ProximityTriggerSystemRequestBus, MoveTrigger, m_proximityTrigger, m_cachedAABB, false);
    }

    //=========================================================================
    // Deactivate
    //=========================================================================
    void TriggerAreaComponent::Deactivate()
    {
        TriggerAreaRequestsBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        RemoveProximityTrigger();
    }


    bool TriggerAreaComponent::AddRequiredTagInternal(const Tag& requiredTag)
    {
        bool isTagExcluded = (m_excludedTags.end() != AZStd::find(m_excludedTags.begin(), m_excludedTags.end(), requiredTag));
        AZ_Warning("TriggerAreaComponent", !isTagExcluded, "Required tag is already Excluded");

        if (!isTagExcluded)
        {
            m_requiredTags.push_back(requiredTag);
        }

        return !isTagExcluded;
    }

    void TriggerAreaComponent::AddRequiredTag(const Tag& requiredTag)
    {
        if (AddRequiredTagInternal(requiredTag))
        {
            ReevaluateTagsAllEntities();
        }
    }
    
    void TriggerAreaComponent::RemoveRequiredTag(const Tag& requiredTag)
    {
        const auto& requiredTagIter = AZStd::find(m_requiredTags.begin(), m_requiredTags.end(), requiredTag);
        bool isTagRequired = (m_requiredTags.end() != requiredTagIter);
        AZ_Warning("TriggerAreaComponent", isTagRequired, "No such tag is required %i", requiredTag);

        if (isTagRequired)
        {
            m_requiredTags.erase(requiredTagIter);
            ReevaluateTagsAllEntities();
        }
    }

    bool TriggerAreaComponent::AddExcludedTagInternal(const Tag& excludedTag)
    {
        bool isTagRequired = (m_requiredTags.end() != AZStd::find(m_requiredTags.begin(), m_requiredTags.end(), excludedTag));

        AZ_Warning("TriggerAreaComponent", !isTagRequired, "Excluded tag is already Required");

        if (!isTagRequired)
        {
            m_excludedTags.push_back(excludedTag);
        }

        return !isTagRequired;
    }
    
    void TriggerAreaComponent::AddExcludedTag(const Tag& excludedTag)
    {
        if (AddExcludedTagInternal(excludedTag))
        {
            ReevaluateTagsAllEntities();
        }
    }

    void TriggerAreaComponent::RemoveExcludedTag(const Tag& excludedTag)
    {
        const auto& excludedTagIter = AZStd::find(m_excludedTags.begin(), m_excludedTags.end(), excludedTag);
        bool isTagExcluded = (m_excludedTags.end() != excludedTagIter);
        AZ_Warning("TriggerAreaComponent", isTagExcluded, "No such tag is excluded %i", excludedTag);

        if (isTagExcluded)
        {
            m_excludedTags.erase(excludedTagIter);
            ReevaluateTagsAllEntities();
        }
    }

    void TriggerAreaComponent::OnTick(float deltaTime, AZ::ScriptTimePoint)
    {
        TriggerAreaReplicaChunk* triggerAreaReplicaChunk = static_cast<TriggerAreaReplicaChunk*>(m_replicaChunk.get());

        for (AZ::u64 entityId : triggerAreaReplicaChunk->m_initialEntitiesInArea)
        {
            OnTriggerEnterImpl(AZ::EntityId(entityId));
        }

        triggerAreaReplicaChunk->m_initialEntitiesInArea.clear();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void TriggerAreaComponent::OnTransformChanged(const AZ::Transform& /*parentLocalTM*/, const AZ::Transform& /*parentWorldTM*/)
    {
        UpdateTriggerArea();
    }

    void TriggerAreaComponent::RemoveProximityTrigger()
    {
        if (m_proximityTrigger)
        {
            ProximityTriggerEventBus::Handler::BusDisconnect();
            ShapeComponentNotificationsBus::Handler::BusDisconnect();

            EBUS_EVENT(ProximityTriggerSystemRequestBus, RemoveTrigger, m_proximityTrigger);
            m_proximityTrigger = nullptr;

            m_entitiesInside.clear();
        }
    }

    //=========================================================================
    // OnTriggerEnter
    //=========================================================================
    void TriggerAreaComponent::OnTriggerEnter(AZ::EntityId entityId)
    {
        AZ_Error("TriggerAreaComponent", !IsNetworkControlled(), "OnTriggerEnter being called on a proxy TriggerAreaComponent for Entity(%s).", GetEntity()->GetName().c_str());

        if (!IsNetworkControlled())
        {
            bool result = OnTriggerEnterImpl(entityId);

            if (result && m_replicaChunk && m_replicaChunk->IsMaster())
            {
                static_cast<TriggerAreaReplicaChunk*>(m_replicaChunk.get())->OnAreaEnter(static_cast<AZ::u64>(entityId));
            }
        }
    }

    bool TriggerAreaComponent::OnTriggerEnterImpl(AZ::EntityId entityId)
    {
        bool retVal = false;

        FilteringResult result = EntityPassesFilters(entityId);
        if (result == FilteringResult::Pass)
        {
            retVal = true;

            if (m_entitiesInside.end() == AZStd::find(m_entitiesInside.begin(), m_entitiesInside.end(), entityId))
            {
                EBUS_EVENT_ID(GetEntityId(), TriggerAreaNotificationBus, OnTriggerAreaEntered, entityId);
                EBUS_EVENT_ID(entityId, TriggerAreaEntityNotificationBus, OnEntityEnteredTriggerArea, GetEntityId());

                HandleEnter();

                m_entitiesInside.push_back(entityId);
            }
        }

        if (result == FilteringResult::FailWithPossibilityOfChange || result == FilteringResult::Pass)
        {
            TagComponentNotificationsBus::MultiHandler::BusConnect(entityId);
        }

        return retVal;
    }    

    //=========================================================================
    // OnTriggerExit
    //=========================================================================
    void TriggerAreaComponent::OnTriggerExit(AZ::EntityId entityId)
    {
        AZ_Error("TriggerAreaComponent", !IsNetworkControlled(), "OnTriggerExit being called on a proxy TriggerAreaComponent for Entity(%s).", GetEntity()->GetName().c_str());

        if (!IsNetworkControlled())
        {
            bool result = OnTriggerExitImpl(entityId);

            if (result && m_replicaChunk && m_replicaChunk->IsMaster())
            {
                static_cast<TriggerAreaReplicaChunk*>(m_replicaChunk.get())->OnAreaExit(static_cast<AZ::u64>(entityId));
            }
        }
    }

    bool TriggerAreaComponent::OnTriggerExitImpl(AZ::EntityId entityId)
    {
        bool retVal = false;

        auto foundIter = AZStd::find(m_entitiesInside.begin(), m_entitiesInside.end(), entityId);
        if (foundIter != m_entitiesInside.end())
        {
            m_entitiesInside.erase(foundIter);

            retVal = true;

            EBUS_EVENT_ID(GetEntityId(), TriggerAreaNotificationBus, OnTriggerAreaExited, entityId);
            EBUS_EVENT_ID(entityId, TriggerAreaEntityNotificationBus, OnEntityExitedTriggerArea, GetEntityId());
        }

        auto excludedEntityIter = AZStd::find(m_entitiesInsideExcludedByTags.begin(), m_entitiesInsideExcludedByTags.end(), entityId);
        if (excludedEntityIter != m_entitiesInsideExcludedByTags.end())
        {
            m_entitiesInsideExcludedByTags.erase(excludedEntityIter);
        }

        TagComponentNotificationsBus::MultiHandler::BusDisconnect(entityId);

        return retVal;
    }

    //=========================================================================
    // DebugDraw
    //=========================================================================
    void TriggerAreaComponent::DebugDraw() const
    {
        gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(m_cachedAABB ,false ,m_entitiesInside.empty() ? Col_LightGray : Col_Green, eBBD_Faceted);
    }

    //=========================================================================
    // HandleEnter
    //=========================================================================
    void TriggerAreaComponent::HandleEnter()
    {
        if (m_entitiesInside.size() == 0) // First one in
        {
            if (m_triggerOnce)
            {
                Deactivate();
            }
        }
    }

    //=========================================================================
    // EntityPassesFilters
    //=========================================================================
    TriggerAreaComponent::FilteringResult TriggerAreaComponent::EntityPassesFilters(AZ::EntityId id) const
    {
        bool result = false;

        switch (m_activationEntityType)
        {
            case ActivationEntityType::AllEntities:
            {
                result = true;
                break;
            }
            case ActivationEntityType::SpecificEntities:
            {
                auto foundIter = AZStd::find(m_specificInteractEntities.begin(), m_specificInteractEntities.end(), id);
                result = foundIter != m_specificInteractEntities.end();
                break;
            }
        }
        
        if (!result)
        {
            return FilteringResult::FailWithoutPossibilityOfChange;
        }

        if (result)
        {
            for (const Tag& requiredTag : m_requiredTags)
            {
                result = false;
                EBUS_EVENT_ID_RESULT(result, id, TagComponentRequestBus, HasTag, requiredTag);
                if (!result)
                {
                    break;
                }
            }
        }

        if (result && LmbrCentral::TagComponentRequestBus::FindFirstHandler(id))
        {
            for (const Tag& requiredTag : m_excludedTags)
            {
                result = true;
                EBUS_EVENT_ID_RESULT(result, id, TagComponentRequestBus, HasTag, requiredTag);
                result = !result;
                if (!result)
                {
                    break;
                }
            }
        }

        if (!result)
        {
            return FilteringResult::FailWithPossibilityOfChange;
        }
        else
        {
            return FilteringResult::Pass;
        }
    }

    //=========================================================================
    // IsPlayer
    //=========================================================================
    bool TriggerAreaComponent::IsPlayer(AZ::EntityId entityId)
    {
        // We don't yet have a way to create player actors in AZ::Entity/Components,
        // so check legacy code paths.
        if (IsLegacyEntityId(entityId))
        {
            const /*Cry*/ EntityId id = GetLegacyEntityId(entityId);
            IActor* actor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(id);
            if (actor && actor->GetChannelId() != kInvalidChannelId)
            {
                return true;
            }
        }

        return false;
    }

    //=========================================================================
    // IsLocalPlayer
    //=========================================================================
    bool TriggerAreaComponent::IsLocalPlayer(AZ::EntityId entityId)
    {
        // We don't yet have a way to create player actors in AZ::Entity/Components,
        // so check legacy code paths.
        if (IsLegacyEntityId(entityId))
        {
            const /*Cry*/ EntityId id = GetLegacyEntityId(entityId);
            return (id == gEnv->pGame->GetClientActorId());
        }

        return false;
    }
    bool TriggerAreaComponent::IsNetworkControlled() const
    {
        return m_replicaChunk != nullptr && m_replicaChunk->IsActive() && m_replicaChunk->IsProxy();
    }

    GridMate::ReplicaChunkPtr TriggerAreaComponent::GetNetworkBinding()
    {
        TriggerAreaReplicaChunk* replicaChunk = GridMate::CreateReplicaChunk<TriggerAreaReplicaChunk>();
        replicaChunk->SetHandler(this);

        m_replicaChunk = replicaChunk;

        return m_replicaChunk;
    }

    void TriggerAreaComponent::SetNetworkBinding(GridMate::ReplicaChunkPtr chunk)
    {
        chunk->SetHandler(this);

        m_replicaChunk = chunk;

        EnforceProximityTriggerState();

        AZ::TickBus::Handler::BusConnect();
    }

    void TriggerAreaComponent::UnbindFromNetwork()
    {
        m_replicaChunk->SetHandler(nullptr);
        m_replicaChunk = nullptr;
        
        EnforceProximityTriggerState();
    }

    bool TriggerAreaComponent::OnEntityEnterAreaRPC(AZ::u64 entityId, const GridMate::RpcContext& rpcContext)
    {
        bool allowTrigger = false;        
        if (IsNetworkControlled())
        {
            OnTriggerEnterImpl(AZ::EntityId(entityId));
        }
        else if (!m_entitiesInside.empty())
        {
            allowTrigger = (m_entitiesInside.back() == AZ::EntityId(entityId));
        }

        return allowTrigger;
    }

    bool TriggerAreaComponent::OnEntityExitAreaRPC(AZ::u64 entityId, const GridMate::RpcContext& rpcContext)
    {
        if (IsNetworkControlled())
        {
            OnTriggerExitImpl(AZ::EntityId(entityId));
        }

        return true;
    }

    void TriggerAreaComponent::EnforceProximityTriggerState()
    {
        if (IsNetworkControlled())
        {
            if (m_proximityTrigger)
            {
                RemoveProximityTrigger();
            }
        }
        else if (m_proximityTrigger == nullptr)
        {
            CreateProximityTrigger();
        }
    }


    bool TriggerAreaComponent::NarrowPassCheck(const AZ::Vector3& position) const
    {
        bool isPointInside = false;
        EBUS_EVENT_ID_RESULT(isPointInside, GetEntityId(), ShapeComponentRequestsBus, IsPointInside, position);
        return isPointInside;
    }

    void TriggerAreaComponent::OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        (void)changeReason;
        UpdateTriggerArea();
    }

    void TriggerAreaComponent::ReevaluateTagsAllEntities()
    {
        for (const AZ::EntityId& entityInside : m_entitiesInside)
        {
            HandleTagChange(entityInside);
        }

        for (const AZ::EntityId& entityInside : m_entitiesInsideExcludedByTags)
        {
            HandleTagChange(entityInside);
        }
    }

    void TriggerAreaComponent::HandleTagChange(const AZ::EntityId& entityId)
    {
        FilteringResult result = EntityPassesFilters(entityId);
        if (result != FilteringResult::Pass)
        {
            OnTriggerExit(entityId);
            if (result == FilteringResult::FailWithPossibilityOfChange)
            {
                TagComponentNotificationsBus::MultiHandler::BusConnect(entityId);
                m_entitiesInsideExcludedByTags.push_back(entityId);
            }
        }
        else
        {
            OnTriggerEnter(entityId);
            auto foundIter = AZStd::find(m_entitiesInsideExcludedByTags.begin(), m_entitiesInsideExcludedByTags.end(), entityId);
            if (foundIter != m_entitiesInsideExcludedByTags.end())
            {
                m_entitiesInsideExcludedByTags.erase(foundIter);
            }
        }
    }

    void TriggerAreaComponent::OnTagAdded(const Tag& tagAdded)
    {
        AZ::EntityId tagAddedToEntity = *(TagComponentNotificationsBus::GetCurrentBusId());
        HandleTagChange(tagAddedToEntity);
    }
    
    void TriggerAreaComponent::OnTagRemoved(const Tag& tagRemoved)
    {
        AZ::EntityId tagRemovedFromEntity = *(TagComponentNotificationsBus::GetCurrentBusId());
        HandleTagChange(tagRemovedFromEntity);
    }

} // namespace LmbrCentral