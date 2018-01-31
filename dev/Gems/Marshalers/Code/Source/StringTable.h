// Copyright 2017 FragLab Ltd. All rights reserved.

#pragma once

#include <Marshalers/StringTableRequestBus.h>
#include <Marshalers/ContainerMarshaler.h>

#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Session/Session.h>
#include <ISystem.h>

namespace FragLab
{
    namespace Marshalers
    {
        class CStringTable
            : public StringTableRequestBus::Handler
            , public GridMate::ReplicaMgrCallbackBus::Handler
            , public GridMate::SessionEventBus::Handler
            , public CrySystemEventBus::Handler
            , public ISystemEventListener
        {
            static constexpr AZ::u8 kNumRemotePeerIds = 32;
            static constexpr AZ::u8 kUndefinedPeerId = 255;

            struct SElement
            {
                AZStd::string value;
                AZStd::bitset<kNumRemotePeerIds> sentToPeers;
            };

            using StringElements = AZStd::unordered_map<AZ::u32, SElement>;
            using RemotePears = AZStd::unordered_map<GridMate::PeerId, AZ::u8>;

        public:
            CStringTable();
            ~CStringTable();

            // StringTableRequestBus
            AZ::u32 MarshalString(const char* szValue) override;
            const char* UnmarshalString(AZ::u32 id) const override;

            // GridMate::ReplicaMgrCallbackBus
            void OnPeerRemoved(GridMate::PeerId peerId, GridMate::ReplicaManager* pMgr) override;
            void OnBeforeSendBuffer(GridMate::ReplicaPeer* pPeer, GridMate::Carrier* pCarrier, unsigned char commChannel) override;
            void UnmarshalStringTable(GridMate::PeerId peerId, GridMate::ReadBuffer& rb) override;

            // SessionEventBus
            void OnSessionCreated(GridMate::GridSession* session) override;
            void OnSessionDelete(GridMate::GridSession* session) override;

            // CrySystemEventBus
            virtual void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;

            // ISystemEventListener
            virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

        private:
            AZ::u8 GetPeerIndex(GridMate::PeerId peerId);

        private:
            AZStd::bitset<kNumRemotePeerIds> m_usedIds;
            AZStd::bitset<kNumRemotePeerIds> m_dirtyPeers;
            RemotePears m_remotePeers;

            StringElements m_netStrings;
            GridMate::WriteBufferDynamic m_outBuffer;

            ContainerMarshaler<AZStd::string, 30> m_tableStringMarshaler;
        };
    } // namespace Marshalers
}     // namespace FragLab
