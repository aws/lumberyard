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

#include <AzCore/IO/StreamerDriller.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/IO/Streamer.h>
#include <AzCore/IO/Device.h>

namespace AZ
{
    namespace Debug
    {
        StreamerDriller::StreamerDriller(bool isTrackStreams)
            : m_isTrackStreams(isTrackStreams)
        {
            if (m_isTrackStreams)
            {
                BusConnect();
            }
        }

        StreamerDriller::~StreamerDriller()
        {
            if (BusIsConnected())
            {
                BusDisconnect();
            }
        }

        void StreamerDriller::Start(const Param* params, int numParams)
        {
            (void)params;
            (void)numParams;

            // streamer has not be created to need to capture the initial state
            if (!IO::Streamer::IsReady())
            {
                return;
            }

            if (m_isTrackStreams)
            {
                m_isTrackStreams = false; // disable tracking so we can report the initial state

                // if we track stream report the current state
                vector<IO::Device*>::type devices;
                devices.reserve(5);
                for (size_t i = 0; i < m_trackedStreams.size(); ++i)
                {
                    IO::GenericStream* stream = m_trackedStreams[i];

                    // register the device if needed
                    IO::Device* device = IO::GetStreamer()->FindDevice(stream->GetFilename());
                    if (device)
                    {
                        if (AZStd::find(devices.begin(), devices.end(), device) != devices.end())
                        {
                            devices.push_back(device);
                            OnDeviceMounted(device);
                        }
                    }

                    OnRegisterStream(stream, stream->GetModeFlags());
                }

                m_isTrackStreams = true; // not that we have reported the initial state, start tracking again
            }
            else
            {
                BusConnect();
            }
        }

        void StreamerDriller::Stop()
        {
            if (!m_isTrackStreams)
            {
                BusDisconnect();
            }
        }

        void StreamerDriller::OnDeviceMounted(IO::Device* device)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnDeviceMounted", 0xc6bdd55e));
            m_output->Write(AZ_CRC("DeviceId", 0x383bcd03), device);
            m_output->Write(AZ_CRC("Name", 0x5e237e06), device->GetPhysicalName());
            m_output->EndTag(AZ_CRC("OnDeviceMounted", 0xc6bdd55e));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnDeviceUnmounted(IO::Device* device)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->Write(AZ_CRC("OnDeviceUnmounted", 0x7395545a), device);
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnRegisterStream(IO::GenericStream* stream, IO::OpenMode flags)
        {
            if (m_isTrackStreams)
            {
                m_trackedStreams.push_back(stream);
            }

            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnRegisterStream", 0x893513c1));
            m_output->Write(AZ_CRC("StreamId", 0x7597546f), stream);
            m_output->Write(AZ_CRC("DeviceId", 0x383bcd03), IO::GetStreamer()->FindDevice(stream->GetFilename()));
            m_output->Write(AZ_CRC("Name", 0x5e237e06), stream->GetFilename());
            m_output->Write(AZ_CRC("Flags", 0x0b0541ba), flags);
            m_output->Write(AZ_CRC("Size", 0xf7c0246a), (flags & (IO::OpenMode::ModeWrite | IO::OpenMode::ModeAppend)) != IO::OpenMode::Invalid ? static_cast<AZ::IO::SizeType>(-1) : stream->GetLength());
            m_output->Write(AZ_CRC("IsCompressed", 0xdd32876c), stream->IsCompressed());
            m_output->EndTag(AZ_CRC("OnRegisterStream", 0x893513c1));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnUnregisterStream(IO::GenericStream* stream)
        {
            if (m_isTrackStreams)
            {
                vector<IO::GenericStream*>::type::iterator it = AZStd::find(m_trackedStreams.begin(), m_trackedStreams.end(), stream);
                if (it != m_trackedStreams.end())
                {
                    m_trackedStreams.erase(it);
                }
            }

            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->Write(AZ_CRC("OnUnregisterStream", 0x3374d0cb), stream);
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnReadCacheHit(IO::GenericStream* stream, AZ::u64 offset, AZ::u64 size, const char* debugName)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnReadCacheHit", 0xd4535712));
            m_output->Write(AZ_CRC("StreamId", 0x7597546f), stream);
            m_output->Write(AZ_CRC("Size", 0xf7c0246a), size);
            m_output->Write(AZ_CRC("Offset", 0x590acad0), offset);
            if (debugName)
            {
                m_output->Write(AZ_CRC("DebugName", 0x6c3ea120), debugName);
            }
            m_output->EndTag(AZ_CRC("OnReadCacheHit", 0xd4535712));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnAddRequest(const AZStd::shared_ptr<IO::Request>& request)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnAddRequest", 0xee41c96e));
            m_output->Write(AZ_CRC("RequestId", 0x34e754a3), request.get());
            m_output->Write(AZ_CRC("Offset", 0x590acad0), request->GetStreamByteOffset());
            m_output->Write(AZ_CRC("Size", 0xf7c0246a), request->m_byteSize);
            m_output->Write(AZ_CRC("Deadline", 0xb74774f2), request->m_deadline.time_since_epoch().count());
            m_output->Write(AZ_CRC("Priority", 0x62a6dc27), (char)request->m_priority);
            m_output->Write(AZ_CRC("Operation", 0x1981a66d), (char)request->m_operation);
            if (request->m_debugName)
            {
                m_output->Write(AZ_CRC("DebugName", 0x6c3ea120), request->m_debugName);
            }
            m_output->WriteTimeMicrosecond(AZ_CRC("Timestamp", 0xa5d6e63e));
            m_output->EndTag(AZ_CRC("OnAddRequest", 0xee41c96e));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnCompleteRequest(const AZStd::shared_ptr<IO::Request>& request, IO::Request::StateType state)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnCompleteRequest", 0x7f6b66f7));
            m_output->Write(AZ_CRC("RequestId", 0x34e754a3), request.get());
            m_output->Write(AZ_CRC("State", 0xa393d2fb), state);
            m_output->WriteTimeMicrosecond(AZ_CRC("Timestamp", 0xa5d6e63e));
            m_output->EndTag(AZ_CRC("OnCompleteRequest", 0x7f6b66f7));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnCancelRequest(const AZStd::shared_ptr<IO::Request>& request)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->Write(AZ_CRC("OnCancelRequest", 0x89d4ea74), request.get());
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnRescheduleRequest(const AZStd::shared_ptr<IO::Request>& request, AZStd::chrono::system_clock::time_point newDeadline, IO::Request::PriorityType newPriority)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnRescheduleRequest", 0x883b3e85));
            m_output->Write(AZ_CRC("RequestId", 0x34e754a3), request.get());
            m_output->Write(AZ_CRC("NewDeadLine", 0x184cc661), newDeadline.time_since_epoch().count());
            m_output->Write(AZ_CRC("NewPriority", 0xcdad6eb4), (char)newPriority);
            m_output->EndTag(AZ_CRC("OnRescheduleRequest", 0x883b3e85));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnRead(IO::GenericStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnRead", 0xd7714b7b));
            m_output->Write(AZ_CRC("StreamId", 0x7597546f), stream);
            m_output->Write(AZ_CRC("Size", 0xf7c0246a), byteSize);
            m_output->Write(AZ_CRC("Offset", 0x590acad0), byteOffset);
            m_output->WriteTimeMicrosecond(AZ_CRC("Timestamp", 0xa5d6e63e));
            m_output->EndTag(AZ_CRC("OnRead", 0xd7714b7b));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnReadComplete(IO::GenericStream* stream, AZ::u64 bytesTransferred)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnReadComplete", 0x0efa014b));
            m_output->Write(AZ_CRC("StreamId", 0x7597546f), stream);
            m_output->Write(AZ_CRC("bytesTransferred", 0x35684b99), bytesTransferred);
            m_output->WriteTimeMicrosecond(AZ_CRC("Timestamp", 0xa5d6e63e));
            m_output->EndTag(AZ_CRC("OnRead", 0xd7714b7b));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnWrite(IO::GenericStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnWrite", 0x6925001a));
            m_output->Write(AZ_CRC("StreamId", 0x7597546f), stream);
            m_output->Write(AZ_CRC("Size", 0xf7c0246a), byteSize);
            m_output->Write(AZ_CRC("Offset", 0x590acad0), byteOffset);
            m_output->WriteTimeMicrosecond(AZ_CRC("Timestamp", 0xa5d6e63e));
            m_output->EndTag(AZ_CRC("OnWrite", 0x6925001a));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnWriteComplete(IO::GenericStream* stream, AZ::u64 bytesTransferred)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnWriteComplete", 0x6c5f7c79));
            m_output->Write(AZ_CRC("StreamId", 0x7597546f), stream);
            m_output->Write(AZ_CRC("bytesTransferred", 0x35684b99), bytesTransferred);
            m_output->WriteTimeMicrosecond(AZ_CRC("Timestamp", 0xa5d6e63e));
            m_output->EndTag(AZ_CRC("OnWriteComplete", 0x6c5f7c79));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnCompressorRead(IO::CompressorStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnCompressorRead", 0xbd093b22));
            m_output->Write(AZ_CRC("StreamId", 0x7597546f), stream);
            m_output->Write(AZ_CRC("Size", 0xf7c0246a), byteSize);
            m_output->Write(AZ_CRC("Offset", 0x590acad0), byteOffset);
            m_output->WriteTimeMicrosecond(AZ_CRC("Timestamp", 0xa5d6e63e));
            m_output->EndTag(AZ_CRC("OnCompressorRead", 0xbd093b22));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnCompressorReadComplete(IO::CompressorStream* stream, AZ::u64 bytesTransferred)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnCompressorReadComplete", 0x9c08d9cd));
            m_output->Write(AZ_CRC("StreamId", 0x7597546f), stream);
            m_output->Write(AZ_CRC("bytesTransferred", 0x35684b99), bytesTransferred);
            m_output->WriteTimeMicrosecond(AZ_CRC("Timestamp", 0xa5d6e63e));
            m_output->EndTag(AZ_CRC("OnCompressorReadComplete", 0x9c08d9cd));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnCompressorWrite(IO::CompressorStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnCompressorWrite", 0x7bf8913a));
            m_output->Write(AZ_CRC("StreamId", 0x7597546f), stream);
            m_output->Write(AZ_CRC("Size", 0xf7c0246a), byteSize);
            m_output->Write(AZ_CRC("Offset", 0x590acad0), byteOffset);
            m_output->WriteTimeMicrosecond(AZ_CRC("Timestamp", 0xa5d6e63e));
            m_output->EndTag(AZ_CRC("OnCompressorWrite", 0x7bf8913a));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }

        void StreamerDriller::OnCompressorWriteComplete(IO::CompressorStream* stream, AZ::u64 bytesTransferred)
        {
            if (m_output == NULL)
            {
                return;                     // we can have the driller running without the output (tracking mode)
            }
            m_output->BeginTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
            m_output->BeginTag(AZ_CRC("OnCompressorWriteComplete", 0x6816a8b4));
            m_output->Write(AZ_CRC("StreamId", 0x7597546f), stream);
            m_output->Write(AZ_CRC("bytesTransferred", 0x35684b99), bytesTransferred);
            m_output->WriteTimeMicrosecond(AZ_CRC("Timestamp", 0xa5d6e63e));
            m_output->EndTag(AZ_CRC("OnCompressorWriteComplete", 0x6816a8b4));
            m_output->EndTag(AZ_CRC("StreamerDriller", 0x2f27ed88));
        }
    } // namespace Debug
} // namespace AZ
