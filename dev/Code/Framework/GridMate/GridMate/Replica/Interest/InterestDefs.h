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
#ifndef GM_REPLICA_INTERESTDEFS_H
#define GM_REPLICA_INTERESTDEFS_H

#include <AzCore/base.h>
#include <GridMate/Replica/ReplicaCommon.h>
#include <GridMate/Containers/vector.h>
#include <GridMate/Containers/unordered_set.h>
#include <GridMate/Containers/unordered_map.h>

namespace GridMate
{
    /**
    * Bitmask used internally in InterestManager to check which handler is responsible for a given interest match
    */
    using InterestHandlerSlot = AZ::u32;

    /**
    * Rule identifier (unique within the session)
    */
    using RuleNetworkId = AZ::u64;
    ///////////////////////////////////////////////////////////////////////////


    /**
    *  InterestQueryResult: a structure to gather new matches from handlers. Passed to handler within matching context when handler's Match method is invoked.
    *  User must fill the structure with changes that handler recalculated.
    */
    using InterestPeerSet = unordered_set<PeerId>;
    using InterestMatchResult = unordered_map<ReplicaId, InterestPeerSet>;
    ///////////////////////////////////////////////////////////////////////////

    /**
    *  Base class for interest rules
    */
    class InterestRule
    {
    public:
        explicit InterestRule(PeerId peerId, RuleNetworkId netId)
            : m_peerId(peerId)
            , m_netId(netId)
        {}
            
        PeerId GetPeerId() const { return m_peerId; }
        RuleNetworkId GetNetworkId() { return m_netId; }

    protected:
        PeerId m_peerId; ///< the peer this rule is bound to
        RuleNetworkId m_netId; ///< network id
    };
    ///////////////////////////////////////////////////////////////////////////


    /**
    *  Base class for interest attributes
    */
    class InterestAttribute
    {
    public:
        explicit InterestAttribute(ReplicaId replicaId)
            : m_replicaId(replicaId)
        {}

        ReplicaId GetReplicaId() const { return m_replicaId; }

    protected:
        ReplicaId m_replicaId; ///< Replica id this attribute is bound to
    };
    ///////////////////////////////////////////////////////////////////////////
} // GridMate

#endif // GM_REPLICA_INTERESTDEFS_H
