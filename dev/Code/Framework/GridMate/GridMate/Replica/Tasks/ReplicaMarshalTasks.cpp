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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Debug/Profiler.h>

#include <GridMate/Replica/SystemReplicas.h>
#include <GridMate/Replica/Tasks/ReplicaMarshalTasks.h>
#include <GridMate/Replica/ReplicaDrillerEvents.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/ReplicaStatus.h>
#include <GridMate/Replica/ReplicaUtils.h>

namespace GridMate
{
    //-----------------------------------------------------------------------------
    // MarshalTaskContext
    //-----------------------------------------------------------------------------
    struct MarshalTaskContext
    {
        MarshalTaskContext(const PrepareDataResult& pdr) : m_pdr(pdr) {}

        PrepareDataResult m_pdr;
    };
    //-----------------------------------------------------------------------------
    // ReplicaMarshalTaskBase
    //-----------------------------------------------------------------------------
    ReplicaMarshalTaskBase::ReplicaMarshalTaskBase(ReplicaPtr replica)
    {
        m_replica = replica;
        AZ_Assert(m_replica, "No replica given to marshaling task");
        m_replica->RegisterMarshalingTask(this);
    }
    //-----------------------------------------------------------------------------
    ReplicaMarshalTaskBase::~ReplicaMarshalTaskBase()
    {
        m_replica->UnregisterMarshalingTask(this);
    }
    //-----------------------------------------------------------------------------
    void ReplicaMarshalTaskBase::MarshalNewReplica(Replica* replica, ReservedIds cmdId, WriteBuffer& outBuffer)
    {
        outBuffer.Write(cmdId);
        outBuffer.Write(replica->IsSyncStage());
        outBuffer.Write(replica->IsMigratable());
        outBuffer.Write(replica->m_createTime);
        outBuffer.Write(AZStd::static_pointer_cast<ReplicaStatus>(m_replica->m_replicaStatus)->m_ownerSeq.Get());
    }
    //-----------------------------------------------------------------------------
    ReplicaPeer* ReplicaMarshalTaskBase::GetUpstreamHop()
    {
        return m_replica->m_upstreamHop;
    }
    //-----------------------------------------------------------------------------
    bool ReplicaMarshalTaskBase::CanUpstream() const
    {
        return m_replica->m_upstreamHop && !m_replica->m_upstreamHop->IsOrphan() && !AZStd::static_pointer_cast<ReplicaStatus>(m_replica->m_replicaStatus)->IsUpstreamSuspended();
    }
    //-----------------------------------------------------------------------------
    void ReplicaMarshalTaskBase::ResetMarshalState()
    {
        m_replica->m_flags &= ~(Replica::Rep_New | Replica::Rep_ChangedOwner | Replica::Rep_SuspendDownstream);
    }
    //-----------------------------------------------------------------------------
    void ReplicaMarshalTaskBase::OnSendReplicaBegin()
    {
        EBUS_EVENT(Debug::ReplicaDrillerBus, OnSendReplicaBegin, m_replica.get());
    }
    //-----------------------------------------------------------------------------
    void ReplicaMarshalTaskBase::OnSendReplicaEnd(ReplicaPeer* to, const void* data, size_t len)
    {
        EBUS_EVENT(Debug::ReplicaDrillerBus, OnSendReplicaEnd, m_replica.get(), data, len);
        to->m_sentBytes += static_cast<int>(len);
    }
    //-----------------------------------------------------------------------------
    PrepareDataResult ReplicaMarshalTaskBase::PrepareData(ReplicaPtr replica, EndianType endianType)
    {
        return replica->PrepareData(endianType);
    }
    //-----------------------------------------------------------------------------
    // ReplicaMarshalNewTask
    // Marshaling whole replica including constructor data
    //-----------------------------------------------------------------------------
    class ReplicaMarshalNewTask
        : public ReplicaMarshalTaskBase
    {
    public:
        ReplicaMarshalNewTask(ReservedIds CmdId, ReplicaPtr replica, ReplicaPeer* peer)
            : ReplicaMarshalTaskBase(replica)
            , m_peer(peer)
            , m_cmdId(CmdId)
        {
            AZ_Assert(m_peer, "No peer given");
        }
        //-----------------------------------------------------------------------------
        TaskStatus Run(const RunContext& context) override
        {
            if (m_peer->IsOrphan())
            {
                return TaskStatus::Done;
            }

            OnSendReplicaBegin();
            WriteBuffer& buffer = m_peer->GetReliableOutBuffer();
            size_t bufferOffsetStart = buffer.Size();
            MarshalNewReplica(m_replica.get(), m_cmdId, buffer);

            AZ::u32 flags;
            switch (m_cmdId)
            {
            case Cmd_NewProxy:
                // Optimization for NewProxy task.
                flags = ReplicaMarshalFlags::NewProxy | ReplicaMarshalFlags::IncludeCtorData;
                break;
            case Cmd_NewOwner:
                // NewOwner task needs a full sync to avoid corner case de-sync cases.
                flags = ReplicaMarshalFlags::FullSync | ReplicaMarshalFlags::IncludeCtorData;
                break;
            default:
                flags = ReplicaMarshalFlags::FullSync | ReplicaMarshalFlags::IncludeCtorData;
                break;
            }

            MarshalContext mc(flags,
                &buffer,
                ReplicaContext(context.m_replicaManager, context.m_replicaManager->GetTime(), m_peer));
            m_replica->Marshal(mc);
            size_t bufferOffsetEnd = buffer.Size();

            OnSendReplicaEnd(m_peer, buffer.Get() + bufferOffsetStart, bufferOffsetEnd - bufferOffsetStart);

            return TaskStatus::Done;
        }

    private:
        ReplicaPeer* m_peer;
        ReservedIds  m_cmdId;
    };

    //-----------------------------------------------------------------------------
    // ReplicaMarshalNewProxyTask
    //-----------------------------------------------------------------------------
    class ReplicaMarshalNewProxyTask
        : public ReplicaMarshalNewTask
    {
    public:
        GM_CLASS_ALLOCATOR(ReplicaMarshalNewProxyTask);

        ReplicaMarshalNewProxyTask(ReplicaPtr replica, ReplicaPeer* peer)
            : ReplicaMarshalNewTask(Cmd_NewProxy, replica, peer)
        {};
    };

    //-----------------------------------------------------------------------------
    // ReplicaMarshalNewOwnerTask
    //-----------------------------------------------------------------------------
    class ReplicaMarshalNewOwnerTask
        : public ReplicaMarshalNewTask
    {
    public:
        GM_CLASS_ALLOCATOR(ReplicaMarshalNewOwnerTask);

        ReplicaMarshalNewOwnerTask(ReplicaPtr replica, ReplicaPeer* peer)
            : ReplicaMarshalNewTask(Cmd_NewOwner, replica, peer)
        {};
    };

    //-----------------------------------------------------------------------------
    // ReplicaMarshalUpstreamTask
    //-----------------------------------------------------------------------------
    class ReplicaMarshalUpstreamTask
        : public ReplicaMarshalTaskBase
    {
    public:
        GM_CLASS_ALLOCATOR(ReplicaMarshalUpstreamTask);

        enum ReliablilityFlags
        {
            FlagNone       = 0,
            FlagReliable   = 1 << 0,
            FlagUnreliable = 1 << 1
        };
        //-----------------------------------------------------------------------------
        ReplicaMarshalUpstreamTask(ReplicaPtr replica, AZ::u8 reliabilityFlags)
            : ReplicaMarshalTaskBase(replica)
            , m_reliabilityFlags(reliabilityFlags)
        {
        }
        //-----------------------------------------------------------------------------
        TaskStatus Run(const RunContext& context) override
        {
            (void)context;
            if (m_reliabilityFlags & FlagReliable)
            {
                MarshalUpstream(ReplicaMarshalFlags::Reliable, GetUpstreamHop()->GetReliableOutBuffer(), context);
            }

            if (m_reliabilityFlags & FlagUnreliable)
            {
                MarshalUpstream(ReplicaMarshalFlags::None, GetUpstreamHop()->GetUnreliableOutBuffer(), context);
            }

            return TaskStatus::Done;
        }

    private:
        //-----------------------------------------------------------------------------
        void MarshalUpstream(AZ::u32 baseFlags, WriteBuffer& buffer, const RunContext& context)
        {
            OnSendReplicaBegin();
            size_t bufferOffsetStart = buffer.Size();
            MarshalContext mc(baseFlags,
                &buffer,
                ReplicaContext(context.m_replicaManager, context.m_replicaManager->GetTime(), GetUpstreamHop()));

            m_replica->Marshal(mc);
            size_t bufferOffsetEnd = buffer.Size();
            OnSendReplicaEnd(GetUpstreamHop(), buffer.Get() + bufferOffsetStart, bufferOffsetEnd - bufferOffsetStart);
        }

        AZ::u8 m_reliabilityFlags;
    };

    //-----------------------------------------------------------------------------
    // ReplicaMarshalZombieToPeerTask
    //-----------------------------------------------------------------------------
    class ReplicaMarshalZombieToPeerTask
        : public ReplicaMarshalTaskBase
    {
    public:
        GM_CLASS_ALLOCATOR(ReplicaMarshalZombieToPeerTask);

        ReplicaMarshalZombieToPeerTask(ReplicaPtr replica, ReplicaPeer* peer)
            : ReplicaMarshalTaskBase(replica)
            , m_peer(peer)
        {}
        //-----------------------------------------------------------------------------
        TaskStatus Run(const RunContext& context) override
        {
            (void) context;
            if (m_peer->IsOrphan())
            {
                return TaskStatus::Done;
            }

            OnSendReplicaBegin();
            WriteBuffer& buffer = m_peer->GetReliableOutBuffer();
            size_t bufferOffsetStart = buffer.Size();

            MarshalContext mc(ReplicaMarshalFlags::Reliable | ReplicaMarshalFlags::Authoritative | ReplicaMarshalFlags::IncludeDatasets,
                &buffer,
                ReplicaContext(context.m_replicaManager, context.m_replicaManager->GetTime(), m_peer));
            m_replica->Marshal(mc);
            size_t bufferOffsetEnd = buffer.Size();
            OnSendReplicaEnd(m_peer, buffer.Get() + bufferOffsetStart, bufferOffsetEnd - bufferOffsetStart);
            m_peer->GetReliableOutBuffer().Write(Cmd_DestroyProxy);
            m_peer->GetReliableOutBuffer().Write(m_replica->GetRepId());
            return TaskStatus::Done;
        }
    private:
        ReplicaPeer* m_peer;
    };

    //-----------------------------------------------------------------------------
    // ReplicaMarshalZombieTask
    //-----------------------------------------------------------------------------
    ReplicaMarshalZombieTask::ReplicaMarshalZombieTask(ReplicaPtr replica)
        : ReplicaMarshalTaskBase(replica)
    {
    }
    //-----------------------------------------------------------------------------
    ReplicaTask::TaskStatus ReplicaMarshalZombieTask::Run(const RunContext& context)
    {
        if (m_replica->IsMaster() || context.m_replicaManager->IsSyncHost())
        {
            m_replica->PrepareData(context.m_replicaManager->GetGridMate()->GetDefaultEndianType(), 
                // A zombie task occurs right before replica gets removed, by design it needs to set all properties one last time.
                ReplicaMarshalFlags::ForceDirty);
            for (auto& dst : m_replica->m_targets)
            {
                // if this target is new (meaning we never marshaled replica there yet)
                // shouldn't send zombie update because replica was destroyed before target peer knew about it
                if (!dst.IsNew())
                {
                    WaitReplicaTask<ReplicaMarshalZombieToPeerTask>(context, m_replica, dst.GetPeer());
                }
            }

            m_replica->m_flags &= Replica::Rep_Traits;
            m_replica->SetRepId(Invalid_Cmd_Or_Id);
        }

        return TaskStatus::Done;
    }

    //-----------------------------------------------------------------------------
    // ReplicaMarshalUpdateTask
    //-----------------------------------------------------------------------------
    class ReplicaMarshalUpdateTask
        : public ReplicaMarshalTaskBase
    {
    public:
        GM_CLASS_ALLOCATOR(ReplicaMarshalUpdateTask);
        //-----------------------------------------------------------------------------
        ReplicaMarshalUpdateTask(ReplicaPtr replica, ReplicaPeer* peer, const MarshalTaskContext& taskContext)
            : ReplicaMarshalTaskBase(replica)
            , m_peer(peer)
            , m_taskContext(taskContext)
        {
        }
        //-----------------------------------------------------------------------------
        TaskStatus Run(const RunContext& context) override
        {
            if (m_peer->IsOrphan())
            {
                return TaskStatus::Done;
            }

            if (m_taskContext.m_pdr.m_isDownstreamReliableDirty)
            {
                OnSendReplicaBegin();
                WriteBuffer& buffer = m_peer->GetReliableOutBuffer();
                size_t bufferOffsetStart = buffer.Size();

                AZ::u32 flags =
                    ReplicaMarshalFlags::Authoritative |
                    ReplicaMarshalFlags::Reliable |
                    ReplicaMarshalFlags::IncludeDatasets;
                MarshalContext mc(flags,
                    &buffer,
                    ReplicaContext(context.m_replicaManager, context.m_replicaManager->GetTime(), m_peer));
                m_replica->Marshal(mc);
                size_t bufferOffsetEnd = buffer.Size();
                OnSendReplicaEnd(m_peer, buffer.Get() + bufferOffsetStart, bufferOffsetEnd - bufferOffsetStart);
            }

            if (m_taskContext.m_pdr.m_isDownstreamUnreliableDirty)
            {
                OnSendReplicaBegin();
                WriteBuffer& buffer = m_peer->GetUnreliableOutBuffer();
                size_t bufferOffsetStart = buffer.Size();

                AZ::u32 flags =
                    ReplicaMarshalFlags::Authoritative |
                    ReplicaMarshalFlags::IncludeDatasets;
                MarshalContext mc(flags,
                    &buffer,
                    ReplicaContext(context.m_replicaManager, context.m_replicaManager->GetTime(), m_peer));
                m_replica->Marshal(mc);
                size_t bufferOffsetEnd = buffer.Size();
                OnSendReplicaEnd(m_peer, buffer.Get() + bufferOffsetStart, bufferOffsetEnd - bufferOffsetStart);
            }

            return TaskStatus::Done;
        }

    private:
        ReplicaPeer* m_peer;
        MarshalTaskContext m_taskContext;
    };

    //-----------------------------------------------------------------------------
    // ReplicaMarshalTask
    //-----------------------------------------------------------------------------
    ReplicaMarshalTask::ReplicaMarshalTask(ReplicaPtr replica)
        : ReplicaMarshalTaskBase(replica)
    {
    }
    //-----------------------------------------------------------------------------
    ReplicaTask::TaskStatus ReplicaMarshalTask::Run(const RunContext& context)
    {
        PrepareDataResult pdr = PrepareData(m_replica, context.m_replicaManager->GetGridMate()->GetDefaultEndianType());
        bool isDownstreamDirty = pdr.m_isDownstreamReliableDirty | pdr.m_isDownstreamUnreliableDirty;
        for (auto& dst : m_replica->m_targets)
        {
            if (m_replica->IsNewOwner())
            {
                WaitReplicaTask<ReplicaMarshalNewOwnerTask>(context, m_replica, dst.GetPeer());
            }
            else if (dst.IsNew())
            {
                WaitReplicaTask<ReplicaMarshalNewProxyTask>(context, m_replica, dst.GetPeer());
            }
            else if (dst.IsRemoved())
            {
                WaitReplicaTask<ReplicaMarshalZombieToPeerTask>(context, m_replica, dst.GetPeer());
            }
            else if (isDownstreamDirty)
            {
                WaitReplicaTask<ReplicaMarshalUpdateTask>(context, m_replica, dst.GetPeer(), MarshalTaskContext(pdr));
            }

            dst.SetNew(false);

            // If the unreliable buffer size is above the cutoff, flush it to avoid fragmentation.
            // Note that the reliable buffer is also flushed to maintain correct ordering.
            if (dst.GetPeer()->GetUnreliableOutBuffer().Size() > GM_REPLICA_MSG_CUTOFF)
            {
                dst.GetPeer()->SendBuffer(context.m_replicaManager->m_cfg.m_carrier, context.m_replicaManager->m_cfg.m_commChannel);
            }
        }

        if (CanUpstream() && (pdr.m_isUpstreamReliableDirty || pdr.m_isUpstreamUnreliableDirty))
        {
            AZ::u8 reliabilityFlags = 0;
            reliabilityFlags |= (pdr.m_isUpstreamReliableDirty ? ReplicaMarshalUpstreamTask::FlagReliable : 0);
            reliabilityFlags |= (pdr.m_isUpstreamUnreliableDirty ? ReplicaMarshalUpstreamTask::FlagUnreliable : 0);
            WaitReplicaTask<ReplicaMarshalUpstreamTask>(context, m_replica, reliabilityFlags);

            // If the unreliable buffer size is above the cutoff, flush it to avoid fragmentation.
            // Note that the reliable buffer is also flushed to maintain correct ordering.
            if (m_replica->m_upstreamHop->GetUnreliableOutBuffer().Size() > GM_REPLICA_MSG_CUTOFF)
            {
                m_replica->m_upstreamHop->SendBuffer(context.m_replicaManager->m_cfg.m_carrier, context.m_replicaManager->m_cfg.m_commChannel);
            }
        }

        ResetMarshalState();

        if (isDownstreamDirty)
        {
            m_replica->MarkRPCsAsRelayed();
        }

        for (auto it = m_replica->m_targets.begin(); it != m_replica->m_targets.end(); )
        {
            auto& target = *(it++);
            if (target.IsRemoved())
            {
                target.Destroy();
            }
        }

        // downstream/unreliable is normally used for dataset updates that will continue to be sent,
        // so keep the task in the queue.
        TaskStatus result = pdr.m_isDownstreamUnreliableDirty ? TaskStatus::Repeat : TaskStatus::Done;
        return result;
    }
} // namespace GridMate

#endif // AZ_UNITY_BUILD
