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
#include "StdAfx.h"

/*!
 * Between CryEngine and GameCore, we have three forms of RMIs supported through CryNetwork that
 * the shim must maintain support for:
 *  - GameObjectExtension RMIs (aka Legacy CryEngine)
 *  - Actor system RMIs (GameCore components)
 *  - Script/Lua RMIs
 *
 * This shim does in fact support all three, albeit in an ad-hoc manner. There's no expectation
 * for new features in the above systems, so the shim should not need to change.
 * Moving forward (post-shim), we will be using replicas directly, with replica chunks owned
 * by formal components, with all messages sent as native GridMate RPCs.
 *
 * All RMIs are packaged in buffers and RPC'd across. Legacy and Actor RMIs make use of static
 * RMI representatives, aka "reps", to serialize and interpret parameter buffers.
 * Script RMIs are handled through the CryEngine ScriptRMI system, which serializes to/from
 * lua tables.
 *
 * All RMI buffers use a flexible structure that makes use of in-place storage, spilling over
 * to heap-allocated space if the payload exceeds 128 bytes, as defined in GridMateRMI.h as
 * kInvocationBufferBaseSize.
 * Invocation wrappers that own this storage are allocated for each RMI invocation, however
 * pooling is a relatively trivial option if we find the allocation count is too high.
 *
 * Note: All invocations coming from the game/engine are added to a queue, which maintains
 * order across all RMI flavors. The root network layer (NetworkGridMate) is responsible for
 * flushing the queue after each update.
 */

#include "../NetworkGridMate.h"
#include "../NetworkGridmateDebug.h"

#include "../Replicas/EntityReplica.h"
#include "../Replicas/GameContextReplica.h"

#include "GridMateRMI.h"

#include <IGameObject.h>

//#pragma optimize("",off)
//#pragma inline_depth(0)

namespace GridMate
{
    namespace RMI
    {
        //-----------------------------------------------------------------------------
        uint32 s_actorRMIRepId = 0;
        std::vector<IActorRMIRep*> s_actorRMIReps;

        typedef std::tuple<EntityId,
            LegacyInvocationWrapper::Ptr,
            ActorInvocationWrapper::Ptr,
            ScriptInvocationWrapper::Ptr> QueuedRMI;

        std::vector<QueuedRMI> m_queuedRMIs;

        void InvokeLegacyInternal(EntityId entityId, const LegacyInvocationWrapper::Ptr& invocation);
        void InvokeActorInternal(EntityId entityId, const ActorInvocationWrapper::Ptr& invocation);
        void InvokeScriptInternal(const ScriptInvocationWrapper::Ptr& invocation);

        //-----------------------------------------------------------------------------
        static void ValidateRMI(ChannelId targetChannelFilter, WhereType where)
        {
            WhereType clientFlags = where & eRMI_ClientsMask;
            if (clientFlags != 0)
            {
                CRY_ASSERT_MESSAGE((where & eRMI_ToServer) == 0, "You cannot have both client and server flags set for an RMI!");
                CRY_ASSERT_MESSAGE((clientFlags & - clientFlags) == clientFlags, "Only one target client option can be set for an RMI!");
                if ((clientFlags & (eRMI_ToClientChannel | eRMI_ToOtherClients | eRMI_ToOtherRemoteClients)) != 0)
                {
                    CRY_ASSERT_MESSAGE(targetChannelFilter != kInvalidChannelId, "RMIs sent using eRMI_ToClientChannel, eRMI_ToOtherClients or eRMI_ToOtherRemoteClients require a valid channel id filter!");
                }
            }
        }

        //-----------------------------------------------------------------------------
        void FlushQueue()
        {
            std::vector<QueuedRMI> queuedRMIs(std::move(m_queuedRMIs));

            for (const QueuedRMI& rmi : queuedRMIs)
            {
                if (std::get<1>(rmi))
                {
                    InvokeLegacyInternal(std::get<0>(rmi), std::get<1>(rmi));
                }
                else if (std::get<2>(rmi))
                {
                    InvokeActorInternal(std::get<0>(rmi), std::get<2>(rmi));
                }
                else if (std::get<3>(rmi))
                {
                    InvokeScriptInternal(std::get<3>(rmi));
                }
            }
        }

        //-----------------------------------------------------------------------------
        void EmptyQueue()
        {
            m_queuedRMIs.clear();
        }

        //-----------------------------------------------------------------------------
        inline bool ActorRMICompareId(IActorRMIRep* rep, uint32 id)
        {
            return rep->GetUniqueId() < id;
        }

        //-----------------------------------------------------------------------------
        IActorRMIRep* FindActorRMIRep(uint32 repId)
        {
            auto foundAt = std::lower_bound(s_actorRMIReps.begin(), s_actorRMIReps.end(), repId, ActorRMICompareId);
            if (foundAt != s_actorRMIReps.end() && (*foundAt)->GetUniqueId() == repId)
            {
                return *foundAt;
            }

            return nullptr;
        }

        //-----------------------------------------------------------------------------
        IRMIRep* FindLegacyRMIRep(EntityId entityId, uint32 repId, IGameObjectExtension** extension /*= nullptr*/)
        {
            if (IGameObject* gameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(entityId))
            {
                if (IGameObjectExtension* owningExtension = gameObject->GetExtensionWithRMIRep(repId))
                {
                    if (extension)
                    {
                        *extension = owningExtension;
                    }

                    return owningExtension->GetRMIRep(repId);
                }
            }

            return nullptr;
        }

        //-----------------------------------------------------------------------------
        ChannelId GetEntityOwnerChannelId(EntityId entityId)
        {
            IActor* actor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entityId);
            if (actor)
            {
                return actor->GetChannelId();
            }
            else
            {
                GM_DEBUG_TRACE("Cannot retrieve channelId for entity %u. Only actors have valid channel id.", entityId);
                return kInvalidChannelId;
            }
        }

        //-----------------------------------------------------------------------------
        bool ShouldInvokeLocally(ChannelId sentFromChannelId, EntityId targetEntityId, ChannelId targetChannelFilter, WhereType whereMask)
        {
            const ChannelId localChannelId = Network::Get().GetLocalChannelId();
            const EntityId localActorId = gEnv->pGame->GetIGameFramework()->GetClientActorId();

            if (!!(whereMask & eRMI_ToServer))
            {
                if (gEnv->bServer)
                {
                    return true;
                }
            }

            if (!!(whereMask & eRMI_NoLocalCalls))
            {
                if (localChannelId == sentFromChannelId)
                {
                    return false;
                }
            }

            if (!!(whereMask & eRMI_ToOwningClient))
            {
                ChannelId ownerChannelId = GetEntityOwnerChannelId(targetEntityId);
                if (gEnv->IsClient() && ownerChannelId == localChannelId)
                {
                    return true;
                }
            }

            if (!!(whereMask & eRMI_ToOtherClients))
            {
                if (gEnv->IsClient() && localChannelId != targetChannelFilter)
                {
                    return true;
                }
            }

            if (!!(whereMask & eRMI_ToAllClients))
            {
                if (gEnv->IsClient())
                {
                    return true;
                }
            }

            if (!!(whereMask & eRMI_ToRemoteClients))
            {
                if (localChannelId != sentFromChannelId)
                {
                    return true;
                }
            }

            if (!!(whereMask & eRMI_ToOtherRemoteClients))
            {
                if (localChannelId != sentFromChannelId && localChannelId != targetChannelFilter)
                {
                    return true;
                }
            }

            if (!!(whereMask & eRMI_ToClientChannel))
            {
                if (localChannelId == targetChannelFilter)
                {
                    return true;
                }
            }

            return false;
        }

        //-----------------------------------------------------------------------------
        bool ShouldDispatch(ChannelId sentFromChannelId, EntityId targetEntityId, WhereType whereMask)
        {
            const ChannelId localChannelId = Network::Get().GetLocalChannelId();
            if (!gEnv->bServer &&
                localChannelId == sentFromChannelId &&
                !!(whereMask & eRMI_ToServer))
            {
                return true;
            }

            return !!(whereMask & eRMI_ClientsMask);
        }

        //-----------------------------------------------------------------------------
        void InvokeActor(EntityId entityId, uint8 actorExtensionId, ChannelId targetChannelFilter, IActorRMIRep& rep)
        {
            ValidateRMI(targetChannelFilter, rep.GetWhere());

            enum
            {
                kRMIParamsMaxSize = 32 * 1024
            };
            char paramsStorage[ kRMIParamsMaxSize ];
            WriteBufferType writeBuffer(EndianType::BigEndian, paramsStorage, sizeof(paramsStorage));

            // Serialize params structure to a temporary GridMate buffer.
            Network::Get().GetLegacySerializeProvider()->AcquireSerializer(writeBuffer, [&](ISerialize* serializer)
            {
                rep.SerializeParams(serializer);
            });

            GM_ASSERT_TRACE(writeBuffer.Size() < kRMIParamsMaxSize, "Overran params buffer.");

            // Dispatch via Gridmate RPCs. This wrapper is ref-counted, and owns a copy
            // of the params buffer.
            ActorInvocationWrapper::Ptr invocation = new ActorInvocationWrapper(
                    Network::Get().GetLocalChannelId(),
                    actorExtensionId,
                    rep.GetUniqueId(),
                    targetChannelFilter,
                    rep.GetWhere(),
                    writeBuffer.Get(),
                    writeBuffer.Size());

            QueuedRMI queuedRMI;
            std::get<0>(queuedRMI) = entityId;
            std::get<2>(queuedRMI) = invocation;
            m_queuedRMIs.push_back(queuedRMI);
        }

        //-----------------------------------------------------------------------------
        void LocalDispatchActor(const ActorInvocationWrapper::Ptr& invocation,
            IActorRMIRep& rep,
            EntityId entityId)
        {
            ReadBufferType readBuffer = invocation->m_paramsBuffer.GetReadBuffer();
            Network::Get().GetLegacySerializeProvider()->AcquireDeserializer(readBuffer, [&](ISerialize* serializer)
            {
                rep.SerializeParams(serializer);
            });
            rep.Invoke(entityId, invocation->m_actorExtensionId);
        }

        //-----------------------------------------------------------------------------
        void InvokeActorInternal(EntityId entityId, const ActorInvocationWrapper::Ptr& invocation)
        {
            IActorRMIRep* rep = FindActorRMIRep(invocation->m_repId);
            GM_ASSERT_TRACE(rep, "Unable to locate RMI rep with id %u.", invocation->m_repId);

            if (!rep)
            {
                return;
            }

            const uint8 actorExtensionId = invocation->m_actorExtensionId;
            const ChannelId targetChannelFilter = invocation->m_targetChannelFilter;
            const WhereType whereMask = rep->GetWhere();

            GM_DEBUG_TRACE_LEVEL(2, "Invoking actor RMI %s for entity/extension %u/%u, where: 0x%u",
                rep->GetDebugName(), entityId, actorExtensionId, whereMask);

            const ChannelId localChannelId = Network::Get().GetLocalChannelId();

            const bool dispatch = ShouldDispatch(localChannelId,
                    entityId,
                    whereMask);

            const bool invokeLocally = ShouldInvokeLocally(localChannelId,
                    entityId,
                    targetChannelFilter,
                    whereMask);

            // If the RMI only needs to execute on this machine, just invoke locally and bail.
            if (!dispatch && invokeLocally)
            {
                if (gEnv->IsClient())
                {
                    LocalDispatchActor(invocation, *rep, entityId);
                }

                GM_DEBUG_TRACE_LEVEL(3, "Locally handled actor RMI for entity/extension %u/%u, where: 0x%u",
                    entityId, actorExtensionId, whereMask);

                return;
            }

            EntityReplica* replica = Network::Get().FindEntityReplica(entityId);

            if (replica)
            {
                GM_DEBUG_TRACE_LEVEL(3, "Dispatching actor RMI %s for entity/extension %u/%u, where: 0x%u",
                    rep->GetDebugName(), entityId, actorExtensionId, whereMask);

                EBUS_EVENT(NetworkSystemEventBus, ActorRMISent, entityId, *rep, invocation->m_paramsBuffer.GetSize());

                if ((invocation->m_where & eRMI_ToServer) == eRMI_ToServer)
                {
                    replica->RPCHandleActorServerRMI(invocation);
                }
                else
                {
                    replica->RPCHandleActorClientRMI(invocation);
                }
            }
            else
            {
                // Support offline invocation.
                if (invokeLocally)
                {
                    LocalDispatchActor(invocation, *rep, entityId);
                }
            }
        }

        //-----------------------------------------------------------------------------
        bool HandleActor(EntityId entityId, ActorInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
        {
            (void)rc;
            IActorRMIRep* rep = FindActorRMIRep(invocation->m_repId);

            GM_DEBUG_TRACE_LEVEL(2, "Handling actor RMI %s for entity/extension %u/%u, where: 0x%u",
                rep->GetDebugName(), entityId, invocation->m_actorExtensionId, invocation->m_where);

            if (rep)
            {
                const WhereType whereMask = invocation->m_where;
                const ChannelId sentFromChannel = invocation->m_sentFromChannel;
                const ChannelId targetChannelFilter = invocation->m_targetChannelFilter;

                const bool dispatch = ShouldDispatch(sentFromChannel,
                        entityId,
                        whereMask);

                const bool invokeLocally = ShouldInvokeLocally(sentFromChannel,
                        entityId,
                        targetChannelFilter,
                        whereMask);

                if (invokeLocally)
                {
                    LocalDispatchActor(invocation, *rep, entityId);

                    if (invocation->m_sentFromChannel != Network::Get().GetLocalChannelId())
                    {
                        EBUS_EVENT(NetworkSystemEventBus, ActorRMIReceived, entityId, *rep, invocation->m_paramsBuffer.GetSize());
                    }

                    GM_DEBUG_TRACE_LEVEL(3, "Dispatched to rep actor RMI %s for entity/extension %u/%u, where: 0x%u",
                        rep->GetDebugName(), entityId, invocation->m_actorExtensionId, invocation->m_where);
                }

                if (dispatch)
                {
                    GM_DEBUG_TRACE_LEVEL(3, "Passing on to clients actor RMI %s for entity/extension %u/%u, where: 0x%u",
                        rep->GetDebugName(), entityId, invocation->m_actorExtensionId, invocation->m_where);

                    // This RMI is to be forwarded on to clients.
                    return true;
                }
            }

            return false;
        }

        //-----------------------------------------------------------------------------
        void InvokeLegacy(IGameObject* gameObject, IRMIRep& rep, void* params, ChannelId targetChannelFilter, WhereType whereMask)
        {
            ValidateRMI(targetChannelFilter, whereMask);

            enum
            {
                kRMIParamsMaxSize = 16 * 1024
            };
            char paramsStorage[ kRMIParamsMaxSize ];
            WriteBufferType writeBuffer(EndianType::BigEndian, paramsStorage, sizeof(paramsStorage));

            // Serialize params structure to a temporary buffer.
            Network::Get().GetLegacySerializeProvider()->AcquireSerializer(writeBuffer, [&](ISerialize* serializer)
            {
                rep.SerializeParamsToBuffer(serializer, params);
            });

            // Dispatch via Gridmate RPCs. This wrapper is ref-counted, and owns a copy
            // of the params buffer.
            LegacyInvocationWrapper::Ptr invocation = new LegacyInvocationWrapper(
                    Network::Get().GetLocalChannelId(),
                    rep.GetUniqueId(),
                    targetChannelFilter,
                    whereMask,
                    writeBuffer.Get(),
                    writeBuffer.Size());

            QueuedRMI queuedRMI;
            std::get<0>(queuedRMI) = gameObject->GetEntityId();
            std::get<1>(queuedRMI) = invocation;
            m_queuedRMIs.push_back(queuedRMI);
        }

        //-----------------------------------------------------------------------------
        void LocalDispatchLegacy(const LegacyInvocationWrapper::Ptr& invocation, IRMIRep& rep, IGameObjectExtension* extension)
        {
            ReadBufferType readBuffer = invocation->m_paramsBuffer.GetReadBuffer();
            void* params = nullptr;
            Network::Get().GetLegacySerializeProvider()->AcquireDeserializer(readBuffer, [&](ISerialize* serializer)
            {
                params = rep.SerializeParamsFromBuffer(serializer);
            });
            rep.Invoke(extension, params);
        }

        //-----------------------------------------------------------------------------
        void InvokeLegacyInternal(EntityId entityId, const LegacyInvocationWrapper::Ptr& invocation)
        {
            IGameObject* gameObject = GetGameFramework()->GetGameObject(entityId);

            if (!gameObject)
            {
                GM_DEBUG_TRACE("Cannot invoke queued RMI because game object for entity %u could not be found.",
                    entityId);
                return;
            }

            IGameObjectExtension* extension = nullptr;
            IRMIRep* rep = FindLegacyRMIRep(entityId, invocation->m_repId, &extension);
            const WhereType whereMask = invocation->m_where;

            GM_ASSERT_TRACE(rep, "Unable to locate RMI rep with id %u.", invocation->m_repId);
            GM_ASSERT_TRACE(extension, "Unable to locate extension for RMI rep. Cannot invoke RMI.");

            if (extension)
            {
                const EntityId targetEntityId = gameObject->GetEntityId();

                // If the RMI only needs to execute on this machine, just invoke manually.
                if (whereMask == eRMI_ToServer && gEnv->bServer)
                {
                    LocalDispatchLegacy(invocation, *rep, extension);

                    return;
                }

                if (!gEnv->bMultiplayer)
                {
                    return;
                }

                Network& net = Network::Get();
                EntityReplica* replica = net.FindEntityReplica(targetEntityId);

                GM_ASSERT_TRACE(replica, "Unable to locate replica for entity %u. Cannot invoke RMI.",
                    targetEntityId);

                if (replica)
                {
                    EBUS_EVENT(NetworkSystemEventBus, LegacyRMISent, targetEntityId, *rep, invocation->m_paramsBuffer.GetSize());

                    if ((invocation->m_where & eRMI_ToServer) == eRMI_ToServer)
                    {
                        replica->RPCHandleLegacyServerRMI(invocation);
                    }
                    else
                    {
                        replica->RPCHandleLegacyClientRMI(invocation);
                    }
                }
                else if (ShouldInvokeLocally(kInvalidChannelId, kInvalidEntityId, kInvalidChannelId, whereMask))
                {
                    LocalDispatchLegacy(invocation, *rep, extension);
                }
            }
        }

        //-----------------------------------------------------------------------------
        bool HandleLegacy(EntityId entityId, LegacyInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
        {
            (void)rc;

            IGameObjectExtension* extension = nullptr;
            IRMIRep* rep = FindLegacyRMIRep(entityId, invocation->m_repId, &extension);

            const WhereType where = invocation.get()->m_where;

            if (rep)
            {
                bool invokeLocally = ShouldInvokeLocally(invocation->m_sentFromChannel, entityId, invocation->m_targetChannelFilter, invocation->m_where);
                if (invokeLocally)
                {
                    LocalDispatchLegacy(invocation, *rep, extension);

                    if (invocation->m_sentFromChannel != Network::Get().GetLocalChannelId())
                    {
                        EBUS_EVENT(NetworkSystemEventBus, LegacyRMIReceived, entityId, *rep, invocation->m_paramsBuffer.GetSize());
                    }
                }

                if (!rep->IsServerRMI() && !!(where & eRMI_ClientsMask))
                {
                    // This RMI is to be forwarded on to clients.
                    return true;
                }

                return false;
            }
            else
            {
                GM_ASSERT_TRACE(0, "Failed to locate RMI rep with id %u for entity %u",
                    invocation->m_repId, entityId);
            }

            return false;
        }

        //-----------------------------------------------------------------------------
        void InvokeScript(ISerializable* serializable, bool isServerRMI, ChannelId toChannelId, ChannelId avoidChannelId)
        {
            // Serialize contents.
            enum
            {
                kRMIDataMaxSize = 1024
            };
            char tempStorage[ kRMIDataMaxSize ];
            WriteBufferType writeBuffer(EndianType::BigEndian, tempStorage, sizeof(tempStorage));

            // Serialize params structure to a temporary GridMate buffer.
            Network::Get().GetLegacySerializeProvider()->AcquireSerializer(writeBuffer, [&](ISerialize* serializer)
            {
                serializable->SerializeWith(serializer);
            });

            ScriptInvocationWrapper::Ptr invocation = new ScriptInvocationWrapper(
                    isServerRMI,
                    toChannelId,
                    avoidChannelId,
                    writeBuffer.Get(), writeBuffer.Size());

            QueuedRMI queuedRMI;
            std::get<3>(queuedRMI) = invocation;
            m_queuedRMIs.push_back(queuedRMI);
        }

        //-----------------------------------------------------------------------------
        bool HandleScript(ScriptInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
        {
            (void)rc;

            if (gEnv && gEnv->pGame && gEnv->pGame->GetIGameFramework())
            {
                ReadBufferType readBuffer = invocation->m_serializedData.GetReadBuffer();
                EBUS_EVENT(NetworkSystemEventBus, ScriptRMIReceived, readBuffer.Size().GetSizeInBytesRoundUp());
                Network::Get().GetLegacySerializeProvider()->AcquireDeserializer(readBuffer, [&](ISerialize* serializer)
                {
                    gEnv->pGame->GetIGameFramework()->HandleGridMateScriptRMI(serializer, invocation->m_toChannelId, invocation->m_avoidChannelId);
                });

                if (invocation->m_toChannelId == Network::Get().GetLocalChannelId())
                {
                    return false;
                }
            }

            return true;
        }

        //-----------------------------------------------------------------------------
        void InvokeScriptInternal(const ScriptInvocationWrapper::Ptr& invocation)
        {
            EBUS_EVENT(NetworkSystemEventBus, ScriptRMISent, invocation->m_serializedData.GetSize());

            if (GameContextReplica* contextReplica = Network::Get().GetGameContextReplica())
            {
                if (invocation->m_isServerRMI)
                {
                    contextReplica->RPCHandleServerScriptRMI(invocation);
                }
                else
                {
                    contextReplica->RPCHandleClientScriptRMI(invocation);
                }
            }
            else
            {
                // Support offline invocation.
                HandleScript(invocation, GridMate::RpcContext());
            }
        }

        //-----------------------------------------------------------------------------
        void RegisterActorRMI(IActorRMIRep* rep)
        {
            GM_ASSERT_TRACE(0 == rep->GetUniqueId(), "Rep is already registered.");

            if (0 == rep->GetUniqueId())
            {
                rep->SetUniqueId(++s_actorRMIRepId);

                auto insertAt = std::lower_bound(s_actorRMIReps.begin(), s_actorRMIReps.end(), rep->GetUniqueId(), ActorRMICompareId);

                if (insertAt == s_actorRMIReps.end() || (*insertAt)->GetUniqueId() != rep->GetUniqueId())
                {
                    s_actorRMIReps.insert(insertAt, rep);
                }
            }
        }

        //-----------------------------------------------------------------------------
        void UnregisterActorRMI(IActorRMIRep* rep)
        {
            GM_ASSERT_TRACE(0 != rep->GetUniqueId(), "Rep is not registered.");

            auto removeAt = std::lower_bound(s_actorRMIReps.begin(), s_actorRMIReps.end(), rep->GetUniqueId(), ActorRMICompareId);
            if (removeAt != s_actorRMIReps.end() && (*removeAt)->GetUniqueId() == rep->GetUniqueId())
            {
                s_actorRMIReps.erase(removeAt);
            }
        }
    } // namespace RMI
} // namespace GridMate
