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
#ifndef GM_REPLICA_REPLICATARGET_H
#define GM_REPLICA_REPLICATARGET_H

#include <GridMate/Replica/ReplicaCommon.h>

#include <AzCore/std/containers/intrusive_list.h>

namespace GridMate
{
    class ReplicaPeer;

    /**
    *  ReplicaTarget: keeps replica's marshaling target (peer) and related meta data,
    *  Replica itself keeps an intrusive list of targets it needs to be forwarded to
    *  Peers keep all their associated replica targets as well
    *  Once target is removed from replica it is automatically removed from the corresponding peer and vice versa
    *  Once replica is destroyed - all its targets are automatically removed from peers, same goes for peers
    */
    class ReplicaTarget
    {
        friend class InterestManager;

    public:
        static ReplicaTarget* AddReplicaTarget(ReplicaPeer* peer, Replica* replica);

        void SetNew(bool isNew);
        bool IsNew() const;
        bool IsRemoved() const;

        // Returns ReplicaPeer associated with given replica
        ReplicaPeer* GetPeer() const;

        // Destroys current target. Target will be removed both from peer and replica
        void Destroy();

        // Intrusive hooks to keep this target node both in replica and peer
        AZStd::intrusive_list_node<ReplicaTarget> m_replicaHook;
        AZStd::intrusive_list_node<ReplicaTarget> m_peerHook;

    private:
        GM_CLASS_ALLOCATOR(ReplicaTarget);

        ReplicaTarget();
        ~ReplicaTarget();
        ReplicaTarget(const ReplicaTarget&) AZ_DELETE_METHOD;
        ReplicaTarget& operator=(const ReplicaTarget&) AZ_DELETE_METHOD;

        enum TargetStatus
        {
            TargetNone       =      0,
            TargetNew        = 1 << 0, // it's a newly added target
            TargetRemoved    = 1 << 1, // target was removed
        };

        ReplicaPeer* m_peer; // need to hold peer ptr for marshaling until marshaling is fully moved under peers, it is safe to keep raw ptr as this node will be auto-destroyed when its peer goes away
        AZ::u32 m_flags;

        AZ::u32 m_slotMask;
    };


    /**
    *  Intrusive list for replica targets, destroys targets when cleared
    *  Cannot use AZStd::intrusive_list directly because it does not support auto-unlinking of nodes
    */
    template<class T, class Hook>
    class ReplicaTargetAutoDestroyList
        : private AZStd::intrusive_list<T, Hook>
    {
        typedef typename AZStd::intrusive_list<T, Hook> BaseListType;

    public:

        using BaseListType::begin;
        using BaseListType::end;
        using BaseListType::push_back;

        AZ_FORCE_INLINE ~ReplicaTargetAutoDestroyList()
        {
            clear();
        }

        AZ_FORCE_INLINE void clear()
        {
            while (BaseListType::begin() != BaseListType::end())
            {
                BaseListType::begin()->Destroy();
            }
        }
    };

    typedef ReplicaTargetAutoDestroyList<ReplicaTarget, AZStd::list_member_hook<ReplicaTarget, & ReplicaTarget::m_replicaHook> > ReplicaTargetList;
    typedef ReplicaTargetAutoDestroyList<ReplicaTarget, AZStd::list_member_hook<ReplicaTarget, & ReplicaTarget::m_peerHook> > PeerTargetList;
}

#endif // GM_REPLICA_REPLICATARGET_H
