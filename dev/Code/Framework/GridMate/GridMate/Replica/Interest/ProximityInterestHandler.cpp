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

#include <GridMate/Replica/Interest/ProximityInterestHandler.h>

#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/Interpolators.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/RemoteProcedureCall.h>

#include <GridMate/Replica/Interest/InterestManager.h>

namespace GridMate
{
    class ProximityInterestChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(ProximityInterestChunk);

        // ReplicaChunk
        typedef AZStd::intrusive_ptr<ProximityInterestChunk> Ptr;
        bool IsReplicaMigratable() override { return false; }
        static const char* GetChunkName() { return "ProximityInterestChunk"; }
        bool IsBroadcast() { return true; }
        ///////////////////////////////////////////////////////////////////////////


        ProximityInterestChunk()
            : AddRuleRpc("AddRule")
            , RemoveRuleRpc("RemoveRule")
            , UpdateRuleRpc("UpdateRule")
            , AddRuleForPeerRpc("AddRuleForPeerRpc")
            , m_interestHandler(nullptr)
        {

        }

        void OnReplicaActivate(const ReplicaContext& rc) override
        {
            m_interestHandler = static_cast<ProximityInterestHandler*>(rc.m_rm->GetUserContext(AZ_CRC("ProximityInterestHandler", 0x3a90b3e4)));
            AZ_Assert(m_interestHandler, "No proximity interest handler in the user context");

            if (m_interestHandler)
            {
                m_interestHandler->OnNewRulesChunk(this, rc.m_peer);
            }
        }

        void OnReplicaDeactivate(const ReplicaContext& rc) override
        {
            if (rc.m_peer && m_interestHandler)
            {
                m_interestHandler->OnDeleteRulesChunk(this, rc.m_peer);
            }
        }

        bool AddRuleFn(RuleNetworkId netId, AZ::Aabb bbox, const RpcContext& ctx)
        {
            if (IsProxy())
            {
                auto rulePtr = m_interestHandler->CreateRule(ctx.m_sourcePeer);
                rulePtr->Set(bbox);
                m_rules.insert(AZStd::make_pair(netId, rulePtr));
            }

            return true;
        }

        bool RemoveRuleFn(RuleNetworkId netId, const RpcContext&)
        {
            if (IsProxy())
            {
                m_rules.erase(netId);
            }

            return true;
        }

        bool UpdateRuleFn(RuleNetworkId netId, AZ::Aabb bbox, const RpcContext&)
        {
            if (IsProxy())
            {
                auto it = m_rules.find(netId);
                if (it != m_rules.end())
                {
                    it->second->Set(bbox);
                }
            }

            return true;
        }

        bool AddRuleForPeerFn(RuleNetworkId netId, PeerId peerId, AZ::Aabb bbox, const RpcContext&)
        {
            ProximityInterestChunk* peerChunk = m_interestHandler->FindRulesChunkByPeerId(peerId);
            if (peerChunk)
            {
                auto it = peerChunk->m_rules.find(netId);
                if (it == peerChunk->m_rules.end())
                {
                    auto rulePtr = m_interestHandler->CreateRule(peerId);
                    peerChunk->m_rules.insert(AZStd::make_pair(netId, rulePtr));
                    rulePtr->Set(bbox);
                }
            }
            return false;
        }

        Rpc<RpcArg<RuleNetworkId>, RpcArg<AZ::Aabb>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::AddRuleFn> AddRuleRpc;
        Rpc<RpcArg<RuleNetworkId>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::RemoveRuleFn> RemoveRuleRpc;
        Rpc<RpcArg<RuleNetworkId>, RpcArg<AZ::Aabb>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::UpdateRuleFn> UpdateRuleRpc;

        Rpc<RpcArg<RuleNetworkId>, RpcArg<PeerId>, RpcArg<AZ::Aabb>>::BindInterface<ProximityInterestChunk, &ProximityInterestChunk::AddRuleForPeerFn> AddRuleForPeerRpc;

        unordered_map<RuleNetworkId, ProximityInterestRule::Ptr> m_rules;
        ProximityInterestHandler* m_interestHandler;
    };
    ///////////////////////////////////////////////////////////////////////////


    /*
    * ProximityInterest
    */
    ProximityInterest::ProximityInterest(ProximityInterestHandler* handler)
        : m_handler(handler)
        , m_bbox(AZ::Aabb::CreateNull())
    {
        AZ_Assert(m_handler, "Invalid interest handler");
    }
    ///////////////////////////////////////////////////////////////////////////


    /*
    * ProximityInterestRule
    */
    void ProximityInterestRule::Set(const AZ::Aabb& bbox)
    {
        m_bbox = bbox;
        m_handler->UpdateRule(this);
    }

    void ProximityInterestRule::Destroy()
    {
        m_handler->DestroyRule(this);
    }
    ///////////////////////////////////////////////////////////////////////////


    /*
    * ProximityInterestAttribute
    */
    void ProximityInterestAttribute::Set(const AZ::Aabb& bbox)
    {
        m_bbox = bbox;
        m_handler->UpdateAttribute(this);
    }

    void ProximityInterestAttribute::Destroy()
    {
        m_handler->DestroyAttribute(this);
    }
    ///////////////////////////////////////////////////////////////////////////


    /*
    * ProximityInterestHandler
    */
    ProximityInterestHandler::ProximityInterestHandler()
        : m_im(nullptr)
        , m_rm(nullptr)
        , m_lastRuleNetId(0)
        , m_rulesReplica(nullptr)
    {
        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(ProximityInterestChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<ProximityInterestChunk>();
        }
    }

    ProximityInterestRule::Ptr ProximityInterestHandler::CreateRule(PeerId peerId)
    {
        ProximityInterestRule* rulePtr = aznew ProximityInterestRule(this, peerId, GetNewRuleNetId());
        if (peerId == m_rm->GetLocalPeerId())
        {
            m_rulesReplica->AddRuleRpc(rulePtr->GetNetworkId(), rulePtr->Get());
            m_localRules.insert(rulePtr);
        }

        return rulePtr;
    }

    void ProximityInterestHandler::FreeRule(ProximityInterestRule* rule)
    {
        //TODO: should be pool-allocated
        delete rule;
    }

    void ProximityInterestHandler::DestroyRule(ProximityInterestRule* rule)
    {
        if (m_rm && rule->GetPeerId() == m_rm->GetLocalPeerId())
        {
            m_rulesReplica->RemoveRuleRpc(rule->GetNetworkId());
        }

        rule->m_bbox = AZ::Aabb::CreateNull();
        m_rules.insert(rule);
        m_localRules.erase(rule);
    }

    void ProximityInterestHandler::UpdateRule(ProximityInterestRule* rule)
    {
        if (rule->GetPeerId() == m_rm->GetLocalPeerId())
        {
            m_rulesReplica->UpdateRuleRpc(rule->GetNetworkId(), rule->Get());
        }

        m_rules.insert(rule);
    }

    ProximityInterestAttribute::Ptr ProximityInterestHandler::CreateAttribute(ReplicaId replicaId)
    {
        return aznew ProximityInterestAttribute(this, replicaId);
    }

    void ProximityInterestHandler::FreeAttribute(ProximityInterestAttribute* attrib)
    {
        //TODO: should be pool-allocated
        delete attrib;
    }

    void ProximityInterestHandler::DestroyAttribute(ProximityInterestAttribute* attrib)
    {
        attrib->m_bbox = AZ::Aabb::CreateNull();
        m_attributes.insert(attrib);
    }

    void ProximityInterestHandler::UpdateAttribute(ProximityInterestAttribute* attrib)
    {
        m_attributes.insert(attrib);
    }

    void ProximityInterestHandler::OnNewRulesChunk(ProximityInterestChunk* chunk, ReplicaPeer* peer)
    {
        if (chunk != m_rulesReplica) // non-local
        {
            m_peerChunks.insert(AZStd::make_pair(peer->GetId(), chunk));

            for (auto& rule : m_localRules)
            {
                chunk->AddRuleForPeerRpc(rule->GetNetworkId(), rule->GetPeerId(), rule->Get());
            }
        }
    }

    void ProximityInterestHandler::OnDeleteRulesChunk(ProximityInterestChunk* chunk, ReplicaPeer* peer)
    {
        (void)chunk;
        m_peerChunks.erase(peer->GetId());
    }

    RuleNetworkId ProximityInterestHandler::GetNewRuleNetId()
    {
        ++m_lastRuleNetId;
        return m_rulesReplica->GetReplicaId() | (static_cast<AZ::u64>(m_lastRuleNetId) << 32);
    }

    ProximityInterestChunk* ProximityInterestHandler::FindRulesChunkByPeerId(PeerId peerId)
    {
        auto it = m_peerChunks.find(peerId);
        if (it == m_peerChunks.end())
        {
            return nullptr;
        }
        else
        {
            return it->second;
        }
    }

    const InterestMatchResult& ProximityInterestHandler::GetLastResult()
    {
        return m_resultCache;
    }

    void ProximityInterestHandler::Update()
    {
        m_resultCache.clear();

        //////////////////////////////////////////////
        // just recalculating the whole state for now
        for (auto attrIt = m_attributes.begin(); attrIt != m_attributes.end(); )
        {
            ProximityInterestAttribute* attr = *attrIt;

            auto resultIt = m_resultCache.insert(attr->GetReplicaId());
            for (auto ruleIt = m_rules.begin(); ruleIt != m_rules.end(); ++ruleIt)
            {
                ProximityInterestRule* rule = *ruleIt;
                if (rule->m_bbox.Overlaps(attr->m_bbox))
                {
                    resultIt.first->second.insert(rule->GetPeerId());
                }
            }

            if ((*attrIt)->IsDeleted())
            {
                attrIt = m_attributes.erase(attrIt);
                delete attr;
            }
            else
            {
                ++attrIt;
            }
        }

        for (auto ruleIt = m_rules.begin(); ruleIt != m_rules.end(); )
        {
            ProximityInterestRule* rule = *ruleIt;

            if (rule->IsDeleted())
            {
                ruleIt = m_rules.erase(ruleIt);
                delete rule;
            }
            else
            {
                ++ruleIt;
            }
        }
        //////////////////////////////////////////////
    }

    void ProximityInterestHandler::OnRulesHandlerRegistered(InterestManager* manager)
    {
        AZ_Assert(m_im == nullptr, "Handler is already registered with manager %p (%p)\n", m_im, manager);
        AZ_Assert(m_rulesReplica == nullptr, "Rules replica is already created\n");
        AZ_TracePrintf("GridMate", "Proximity interest handler is registered\n");
        m_im = manager;
        m_rm = m_im->GetReplicaManager();
        m_rm->RegisterUserContext(AZ_CRC("ProximityInterestHandler", 0x3a90b3e4), this);

        auto replica = Replica::CreateReplica("ProximityInterestHandlerRules");
        m_rulesReplica = CreateAndAttachReplicaChunk<ProximityInterestChunk>(replica);
        m_rm->AddMaster(replica);
    }

    void ProximityInterestHandler::OnRulesHandlerUnregistered(InterestManager* manager)
    {
        (void)manager;
        AZ_Assert(m_im == manager, "Handler was not registered with manager %p (%p)\n", manager, m_im);
        AZ_TracePrintf("GridMate", "Proximity interest handler is unregistered\n");
        m_rulesReplica = nullptr;
        m_im = nullptr;
        m_rm->UnregisterUserContext(AZ_CRC("ProximityInterestHandler", 0x3a90b3e4));
        m_rm = nullptr;

        for (auto& chunk : m_peerChunks)
        {
            chunk.second->m_interestHandler = nullptr;
        }
        m_peerChunks.clear();
        m_localRules.clear();

        for (ProximityInterestRule* rule : m_rules)
        {
            delete rule;
        }

        for (ProximityInterestAttribute* attr : m_attributes)
        {
            delete attr;
        }

        m_attributes.clear();
        m_rules.clear();

        m_resultCache.clear();
    }
    ///////////////////////////////////////////////////////////////////////////
}
