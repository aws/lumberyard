// Copyright 2017 FragLab Ltd. All rights reserved.

#include "StdAfx.h"
#include "StringTable.h"

#include <Utils/Trace.h>
#include <INetwork.h>

namespace FragLab
{
    namespace Marshalers
    {
        CStringTable::CStringTable() : m_outBuffer(GridMate::EndianType::IgnoreEndian, 0)
        {
            CrySystemEventBus::Handler::BusConnect();
        }

        CStringTable::~CStringTable()
        {
            GridMate::ReplicaMgrCallbackBus::Handler::BusDisconnect();
            GridMate::SessionEventBus::Handler::BusDisconnect();
            CrySystemEventBus::Handler::BusDisconnect();

            if (gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
            {
                gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
            }
        }

        AZ::u32 CStringTable::MarshalString(const char* szValue)
        {
            GAssert(szValue, "szValue could not be nullptr");
            AZ::u32 crc = AZ::Crc32(szValue, strlen(szValue));
            if (crc)
            {
                m_dirtyPeers.set();
                auto it = m_netStrings.insert_key(crc);
                if (it.second)
                {
                    it.first->second.value = szValue;
                }
                else if (it.first->second.value != szValue)
                {
                    GError("2 strings '%s', '%s' have identical crc32: %u.", it.first->second.value.c_str(), szValue, crc);
                }
            }

            return crc;
        }

        const char* CStringTable::UnmarshalString(AZ::u32 crc) const
        {
            if (crc)
            {
                auto it_f = m_netStrings.find(crc);
                if (it_f != m_netStrings.end())
                {
                    return it_f->second.value.c_str();
                }

                GError("StringTable has no string with crc32: %u.", crc);
            }

            return "";
        }

        void CStringTable::OnPeerRemoved(GridMate::PeerId peerId, GridMate::ReplicaManager* pMgr)
        {
            const auto it_f = m_remotePeers.find(peerId);
            if (it_f != m_remotePeers.end())
            {
                AZ::u8 peerIndex = it_f->second;
                for (auto& elem : m_netStrings)
                {
                    elem.second.sentToPeers.reset(peerIndex);
                }

                m_usedIds.reset(peerIndex);
                m_remotePeers.erase(it_f);
            }
        }

        void CStringTable::OnBeforeSendBuffer(GridMate::ReplicaPeer* pPeer, GridMate::Carrier* pCarrier, unsigned char commChannel)
        {
            AZ::u8 peerIndex = GetPeerIndex(pPeer->GetId());
            if (peerIndex != kUndefinedPeerId && m_dirtyPeers[peerIndex])
            {
                AZ::u16 numStrings = 0;
                GridMate::WriteBuffer::Marker<AZ::u16> numStringsMarker;
                for (auto& elem : m_netStrings)
                {
                    if (!elem.second.sentToPeers[peerIndex])
                    {
                        elem.second.sentToPeers.set(peerIndex);
                        if (0 == numStrings++)
                        {
                            AZ::u32 timeStamp = pCarrier->GetTime();
                            m_outBuffer.Write(timeStamp);
                            m_outBuffer.Write(GridMate::Cmd_StringTable);
                            numStringsMarker = m_outBuffer.InsertMarker<AZ::u16>();
                        }

                        m_tableStringMarshaler.Marshal(m_outBuffer, elem.second.value);
                    }
                }

                if (numStrings > 0)
                {
                    numStringsMarker = numStrings;
                    pCarrier->Send(m_outBuffer.Get(), static_cast<unsigned int>(m_outBuffer.Size()), pPeer->GetConnectionId(), GridMate::Carrier::SEND_RELIABLE, GridMate::Carrier::PRIORITY_NORMAL, commChannel);
                    m_outBuffer.Clear();
                }

                m_dirtyPeers.reset(peerIndex);
            }
        }

        void CStringTable::UnmarshalStringTable(GridMate::PeerId peerId, GridMate::ReadBuffer& rb)
        {
            AZ::u8 peerIndex = GetPeerIndex(peerId);
            if (peerIndex != kUndefinedPeerId)
            {
                AZ::u16 numStrings = 0;
                rb.Read(numStrings);
                for (AZ::u16 i = 0; i < numStrings; ++i)
                {
                    AZStd::string szNewString;
                    m_tableStringMarshaler.Unmarshal(szNewString, rb);
                    AZ::u32 crc = AZ::Crc32(szNewString.c_str(), szNewString.size());
                    auto it = m_netStrings.insert_key(crc);
                    if (it.second)
                    {
                        it.first->second.value = szNewString;
                    }

                    it.first->second.sentToPeers.set(peerIndex);
                }
            }
        }

        AZ::u8 CStringTable::GetPeerIndex(GridMate::PeerId peerId)
        {
            if (GridMate::InvalidReplicaPeerId != peerId)
            {
                const auto it_f = m_remotePeers.find(peerId);
                if (it_f != m_remotePeers.end())
                {
                    return it_f->second;
                }

                for (AZ::u8 i = 0; i < kNumRemotePeerIds; ++i)
                {
                    if (!m_usedIds[i])
                    {
                        m_usedIds.set(i);
                        m_dirtyPeers.set(i);
                        m_remotePeers.insert(AZStd::make_pair(peerId, i));
                        return i;
                    }
                }

                GError("Failed to generate remote peer id; remotePeers: %zu.", m_remotePeers.size());
            }

            return kUndefinedPeerId;
        }

        void CStringTable::OnSessionCreated(GridMate::GridSession* session)
        {
            StringTableRequestBus::Handler::BusConnect();
        }

        void CStringTable::OnSessionDelete(GridMate::GridSession* session)
        {
            StringTableRequestBus::Handler::BusDisconnect();

            m_usedIds.reset();
            m_dirtyPeers.reset();
            m_remotePeers.clear();
            m_netStrings.clear();
        }

        void CStringTable::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
        {
            (void)systemInitParams;

            system.GetISystemEventDispatcher()->RegisterListener(this);
        }

        void CStringTable::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
        {
            switch (event)
            {
            case ESYSTEM_EVENT_GAME_POST_INIT:
            {
                GridMate::IGridMate* pGridMate = gEnv->pNetwork->GetGridMate();
                m_outBuffer.SetEndianType(pGridMate->GetDefaultEndianType());
                GridMate::ReplicaMgrCallbackBus::Handler::BusConnect(pGridMate);
                GridMate::SessionEventBus::Handler::BusConnect(pGridMate);
            }
            break;
            default:
                break;
            }
        }
    } // namespace Marshalers
}     // namespace FragLab
