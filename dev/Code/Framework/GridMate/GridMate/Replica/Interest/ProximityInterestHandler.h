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

#ifndef GM_REPLICA_PROXIMITYINTERESTHANDLER_H
#define GM_REPLICA_PROXIMITYINTERESTHANDLER_H

#include <GridMate/Replica/Interest/RulesHandler.h>
#include <AzCore/Math/Aabb.h>


namespace GridMate
{
    class ProximityInterestHandler;
    class ProximityInterestChunk;

    /*
    * Base interest
    */
    class ProximityInterest
    {
        friend class ProximityInterestHandler;

    public:
        const AZ::Aabb& Get() const { return m_bbox; }

    protected:
        explicit ProximityInterest(ProximityInterestHandler* handler);

        ProximityInterestHandler* m_handler;
        AZ::Aabb m_bbox;
    };
    ///////////////////////////////////////////////////////////////////////////


    /*
    * Proximity rule
    */
    class ProximityInterestRule
        : public InterestRule
        , public ProximityInterest
    {
        friend class ProximityInterestHandler;

    public:
        using Ptr = AZStd::intrusive_ptr<ProximityInterestRule>;

        GM_CLASS_ALLOCATOR(ProximityInterestRule);

        void Set(const AZ::Aabb& bbox);

    private:

        // Intrusive ptr 
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        unsigned int m_refCount = 0;
        AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
        AZ_FORCE_INLINE void release() { --m_refCount; if (!m_refCount) Destroy(); }
        AZ_FORCE_INLINE bool IsDeleted() const { return m_refCount == 0; }
        ///////////////////////////////////////////////////////////////////////////

        ProximityInterestRule(ProximityInterestHandler* handler, PeerId peerId, RuleNetworkId netId)
            : InterestRule(peerId, netId)
            , ProximityInterest(handler)
        {}

        void Destroy();
    };
    ///////////////////////////////////////////////////////////////////////////


    /*
    * Proximity attribute
    */
    class ProximityInterestAttribute
        : public InterestAttribute
        , public ProximityInterest
    {
        friend class ProximityInterestHandler;
        template<class T> friend class InterestPtr;

    public:
        using Ptr = AZStd::intrusive_ptr<ProximityInterestAttribute>;

        GM_CLASS_ALLOCATOR(ProximityInterestAttribute);

        void Set(const AZ::Aabb& bbox);

    private:

        // Intrusive ptr 
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        unsigned int m_refCount = 0;
        AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
        AZ_FORCE_INLINE void release() { Destroy(); }
        AZ_FORCE_INLINE bool IsDeleted() const { return m_refCount == 0; }
        ///////////////////////////////////////////////////////////////////////////

        ProximityInterestAttribute(ProximityInterestHandler* handler, ReplicaId repId)
            : InterestAttribute(repId)
            , ProximityInterest(handler)
        {}

        void Destroy();
    };
    ///////////////////////////////////////////////////////////////////////////


    /*
    * Rules handler
    */
    class ProximityInterestHandler
        : public BaseRulesHandler
    {
        friend class ProximityInterestRule;
        friend class ProximityInterestAttribute;
        friend class ProximityInterestChunk;

    public:

        typedef unordered_set<ProximityInterestAttribute*> AttributeSet;
        typedef unordered_set<ProximityInterestRule*> RuleSet;

        GM_CLASS_ALLOCATOR(ProximityInterestHandler);

        ProximityInterestHandler();

        // Creates new proximity rule and binds it to the peer
        ProximityInterestRule::Ptr CreateRule(PeerId peerId);

        // Creates new proximity attribute and binds it to the replica
        ProximityInterestAttribute::Ptr CreateAttribute(ReplicaId replicaId);

        // Calculates rules and attributes matches
        void Update() override;

        // Returns last recalculated results
        const InterestMatchResult& GetLastResult() override;

        // Returns manager its bound with
        InterestManager* GetManager() override { return m_im; }

        const RuleSet& GetLocalRules() { return m_localRules; }
    private:

        // BaseRulesHandler
        void OnRulesHandlerRegistered(InterestManager* manager) override;
        void OnRulesHandlerUnregistered(InterestManager* manager) override;

        void DestroyRule(ProximityInterestRule* rule);
        void FreeRule(ProximityInterestRule* rule);
        void UpdateRule(ProximityInterestRule* rule);

        void DestroyAttribute(ProximityInterestAttribute* attrib);
        void FreeAttribute(ProximityInterestAttribute* attrib);
        void UpdateAttribute(ProximityInterestAttribute* attrib);


        void OnNewRulesChunk(ProximityInterestChunk* chunk, ReplicaPeer* peer);
        void OnDeleteRulesChunk(ProximityInterestChunk* chunk, ReplicaPeer* peer);

        RuleNetworkId GetNewRuleNetId();

        ProximityInterestChunk* FindRulesChunkByPeerId(PeerId peerId);

        InterestManager* m_im;
        ReplicaManager* m_rm;

        AZ::u32 m_lastRuleNetId;

        unordered_map<PeerId, ProximityInterestChunk*> m_peerChunks;
        RuleSet m_localRules;

        AttributeSet m_attributes;
        RuleSet m_rules;

        ProximityInterestChunk* m_rulesReplica;

        InterestMatchResult m_resultCache;
    };
    ///////////////////////////////////////////////////////////////////////////
}

#endif // GM_REPLICA_PROXIMITYINTERESTHANDLER_H
