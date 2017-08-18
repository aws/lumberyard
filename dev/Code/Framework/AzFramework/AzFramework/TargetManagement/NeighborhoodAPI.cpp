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

#include "NeighborhoodAPI.h"
#include <GridMate/Session/LANSession.h>

namespace Neighborhood {
    //---------------------------------------------------------------------
    NeighborReplica::NeighborReplica()
        : m_capabilities("Capabilities")
        , m_owner("Owner")
        , m_persistentName("PersistentName")
        , m_displayName("DisplayName")
    {
    }
    //---------------------------------------------------------------------
    NeighborReplica::NeighborReplica(GridMate::MemberIDCompact owner, const char* persistentName, NeighborCaps capabilities)
        : m_capabilities("Capabilities", capabilities)
        , m_owner("Owner", owner)
        , m_persistentName("PersistentName", persistentName)
        , m_displayName("DisplayName")
    {
        /*
        Different instances have different constructor values inside driller, thus we have to forcefully mark these as non default.
        */
        m_capabilities.MarkNonDefaultValue();
        m_owner.MarkNonDefaultValue();
        m_persistentName.MarkNonDefaultValue();
        m_displayName.MarkNonDefaultValue();
    }
    //---------------------------------------------------------------------
    void NeighborReplica::UpdateChunk(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
    }
    //---------------------------------------------------------------------
    void NeighborReplica::OnReplicaActivate(const GridMate::ReplicaContext& /*rc*/)
    {
        // TODO: Should we send the message to ourselves as well (master)?
        if (IsProxy())
        {
            AZ_Assert(m_persistentName.Get().c_str(), "Received NeighborReplica with missing persistent name!");
            AZ_Assert(m_displayName.Get().c_str(), "Received NeighborReplica with missing display name!");
            EBUS_EVENT(NeighborhoodBus, OnNodeJoined, *this);
        }
    }
    //---------------------------------------------------------------------
    void NeighborReplica::OnReplicaDeactivate(const GridMate::ReplicaContext& /*rc*/)
    {
        // TODO: Should we send the message to ourselves as well (master)?
        if (IsProxy())
        {
            EBUS_EVENT(NeighborhoodBus, OnNodeLeft, *this);
        }
    }
    //---------------------------------------------------------------------
    void NeighborReplica::UpdateFromChunk(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
    }
    //---------------------------------------------------------------------
    bool NeighborReplica::IsReplicaMigratable()
    {
        return false;
    }
    //---------------------------------------------------------------------
    NeighborCaps NeighborReplica::GetCapabilities() const
    {
        return m_capabilities.Get();
    }
    //---------------------------------------------------------------------
    GridMate::MemberIDCompact NeighborReplica::GetTargetMemberId() const
    {
        return m_owner.Get();
    }
    //---------------------------------------------------------------------
    const char* NeighborReplica::GetPersistentName() const
    {
        return m_persistentName.Get().c_str();
    }
    //---------------------------------------------------------------------
    void NeighborReplica::SetDisplayName(const char* displayName)
    {
        m_displayName.Set(AZStd::string(displayName));
    }
    //---------------------------------------------------------------------
    const char* NeighborReplica::GetDisplayName() const
    {
        return m_displayName.Get().c_str();
    }
    //---------------------------------------------------------------------
}   // namespace Neighborhood
