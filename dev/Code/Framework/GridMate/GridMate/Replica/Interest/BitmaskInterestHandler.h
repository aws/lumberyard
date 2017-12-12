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

#ifndef GM_REPLICA_BITMASKINTERESTHANDLER_H
#define GM_REPLICA_BITMASKINTERESTHANDLER_H

#include <GridMate/Replica/Interest/RulesHandler.h>

#include <GridMate/Containers/vector.h>
#include <GridMate/Containers/unordered_set.h>
#include <AzCore/std/containers/array.h>


namespace GridMate
{
    class BitmaskInterestHandler;
    class BitmaskInterestChunk;
    using InterestBitmask = AZ::u32;
    class BitmaskInterestChunk;

    /*
     * Base interest
    */
    class BitmaskInterest
    {
        friend class BitmaskInterestHandler;

    public:
        InterestBitmask Get() const { return m_bits; }

    protected:
        explicit BitmaskInterest(BitmaskInterestHandler* handler);

        BitmaskInterestHandler* m_handler;
        InterestBitmask m_bits;
    };
    ///////////////////////////////////////////////////////////////////////////


    /*
    * Bitmask rule
    */
    class BitmaskInterestRule 
        : public InterestRule
        , public BitmaskInterest
    {
        friend class BitmaskInterestHandler;

    public:
        using Ptr = AZStd::intrusive_ptr<BitmaskInterestRule>;

        GM_CLASS_ALLOCATOR(BitmaskInterestRule);

        void Set(InterestBitmask newBitmask);

    private:

        // Intrusive ptr 
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        unsigned int m_refCount = 0;
        AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
        AZ_FORCE_INLINE void release() { --m_refCount; if (!m_refCount) Destroy(); }
        AZ_FORCE_INLINE bool IsDeleted() const { return m_refCount == 0; }
        ///////////////////////////////////////////////////////////////////////////

        BitmaskInterestRule(BitmaskInterestHandler* handler, PeerId peerId, RuleNetworkId netId)
            : InterestRule(peerId, netId)
            , BitmaskInterest(handler)
        {}

        void Destroy();
    };
    ///////////////////////////////////////////////////////////////////////////


    /*
    * Bitmask attribute
    */
    class BitmaskInterestAttribute
        : public InterestAttribute
        , public BitmaskInterest
    {
        friend class BitmaskInterestHandler;
        template<class T> friend class InterestPtr;

    public:
        using Ptr = AZStd::intrusive_ptr<BitmaskInterestAttribute>;

        GM_CLASS_ALLOCATOR(BitmaskInterestAttribute);

        void Set(InterestBitmask newBitmask);

    private:

        // Intrusive ptr 
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        unsigned int m_refCount = 0;
        AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
        AZ_FORCE_INLINE void release() { Destroy(); }
        AZ_FORCE_INLINE bool IsDeleted() const { return m_refCount == 0; }
        ///////////////////////////////////////////////////////////////////////////

        BitmaskInterestAttribute(BitmaskInterestHandler* handler, ReplicaId repId)
            : InterestAttribute(repId)
            , BitmaskInterest(handler)
        {}

        void Destroy();
    };
    ///////////////////////////////////////////////////////////////////////////


    /*
    * Rules handler
    */
    class BitmaskInterestHandler 
        : public BaseRulesHandler
    {
        friend class BitmaskInterestRule;
        friend class BitmaskInterestAttribute;
        friend class BitmaskInterestChunk;

    public:

        GM_CLASS_ALLOCATOR(BitmaskInterestHandler);

        BitmaskInterestHandler();

        // Creates new bitmask rule and binds it to the peer
        BitmaskInterestRule::Ptr CreateRule(PeerId peerId);

        // Creates new bitmask attribute and binds it to the replica
        BitmaskInterestAttribute::Ptr CreateAttribute(ReplicaId replicaId);

        // Calculates rules and attributes matches
        void Update() override;

        // Returns last recalculated results
        const InterestMatchResult& GetLastResult() override;

        InterestManager* GetManager() override { return m_im; }
    private:

        // BaseRulesHandler
        void OnRulesHandlerRegistered(InterestManager* manager) override;
        void OnRulesHandlerUnregistered(InterestManager* manager) override;

        void DestroyRule(BitmaskInterestRule* rule);
        void FreeRule(BitmaskInterestRule* rule);
        void UpdateRule(BitmaskInterestRule* rule);

        void DestroyAttribute(BitmaskInterestAttribute* attrib);
        void FreeAttribute(BitmaskInterestAttribute* attrib);
        void UpdateAttribute(BitmaskInterestAttribute* attrib);


        void OnNewRulesChunk(BitmaskInterestChunk* chunk, ReplicaPeer* peer);
        void OnDeleteRulesChunk(BitmaskInterestChunk* chunk, ReplicaPeer* peer);

        RuleNetworkId GetNewRuleNetId();

        BitmaskInterestChunk* FindRulesChunkByPeerId(PeerId peerId);

        typedef unordered_set<BitmaskInterestAttribute*> AttributeSet;
        typedef unordered_set<BitmaskInterestRule*> RuleSet;
        static const size_t k_numGroups = sizeof(InterestBitmask) * CHAR_BIT;

        InterestManager* m_im;
        ReplicaManager* m_rm;
        
        AZ::u32 m_lastRuleNetId;

        unordered_map<PeerId, BitmaskInterestChunk*> m_peerChunks;

        RuleSet m_localRules;

        AttributeSet m_dirtyAttributes;
        RuleSet m_dirtyRules;

        AZStd::array<AttributeSet, k_numGroups> m_attrGroups;
        AZStd::array<RuleSet, k_numGroups> m_ruleGroups;

        InterestMatchResult m_resultCache;

        BitmaskInterestChunk* m_rulesReplica;

        AttributeSet m_attrs;
        RuleSet m_rules;
    };
    ///////////////////////////////////////////////////////////////////////////
}

#endif