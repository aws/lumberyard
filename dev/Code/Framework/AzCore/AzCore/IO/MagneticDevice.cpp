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

#include <AzCore/IO/MagneticDevice.h>
#include <AzCore/IO/StreamerDrillerBus.h>
#include <AzCore/IO/VirtualStream.h>

namespace AZ
{
    namespace IO
    {

        //=========================================================================
        // MagneticDevice
        // [8/29/2012]
        //=========================================================================
        MagneticDevice::MagneticDevice(const AZStd::string& name, FileIOBase* ioBase, unsigned int cacheBlockSize, unsigned int numCacheBlocks, const AZStd::thread_desc* threadDesc, int threadSleepTimeMS)
            : Device(name, ioBase, threadDesc, threadSleepTimeMS)
        {
            m_type = DT_MAGNETIC_DRIVE;

            m_sectorSize = 512;
            m_eccBlockSize = 32 * 1024;
            m_avgSeekTime = AZStd::chrono::microseconds(8000);
            m_readBytesPerMicrosecond = 50.0 * 1024.0 * 1024.0 / 1000000.0 /* to microsecond */;
            m_writeBytesPerMicrosecond = 25.0 * 1024.0 * 1024.0 / 1000000.0 /* to microsecond */;
            AZ_Assert((cacheBlockSize % m_sectorSize) == 0, "Cache block size %d must be multiple of the sector %d size!", cacheBlockSize, m_sectorSize);
            CreateCache(cacheBlockSize, numCacheBlocks);
        }


        //=========================================================================
        // ScheduleRequests
        // [8/29/2012]
        //=========================================================================
        void MagneticDevice::ScheduleRequests()
        {
            if (m_pending.size() <= 1)
            {
                // if we have one request and it's in the cache
                if (!m_pending.empty() && m_pending.front().m_operation == Request::OperationType::OT_READ && m_readCache && m_readCache->IsInCache(&m_pending.front()))
                {
                    Request* req = &m_pending.front();
                    EBUS_DBG_EVENT(StreamerDrillerBus, OnReadCacheHit, req->m_stream, req->GetStreamByteOffset(), req->m_byteSize, req->m_debugName);
                    m_pending.pop_front();
                    CompleteRequest(req);
                }
                return;
            }

            //
            //////////////////////////////////////////////////////////////////////////
            // estimate completion for all requests
            AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();

            for (Request::ListType::iterator iter = m_pending.begin(); iter != m_pending.end(); )
            {
                Request& r = *iter;
                if ((r.m_operation == Request::OperationType::OT_READ) && m_readCache && m_readCache->IsInCache(&r))  // check read cache
                {
                    EBUS_DBG_EVENT(StreamerDrillerBus, OnReadCacheHit, r.m_stream, r.GetStreamByteOffset(), r.m_byteSize, r.m_debugName);
                    iter = m_pending.erase(iter);
                    CompleteRequest(&r);
                    continue;
                }
            
                SizeType byteOffset = r.GetStreamByteOffset() + r.m_bytesProcessedStart;
                SizeType byteSize = r.m_byteSize - (r.m_bytesProcessedStart + r.m_bytesProcessedEnd);
                if (r.m_stream == m_currentStream)
                {
                    SizeType seekOffset = byteOffset > m_currentStreamOffset ? byteOffset - m_currentStreamOffset : SizeType(-1);
                    if (seekOffset > m_eccBlockSize * 2)  // if we are within the ECC block size no seek
                    {
                        // half the seek time if we are within the stream
                        r.m_estimatedCompletion = now + m_avgSeekTime / AZStd::sys_time_t(2);
                    }
                }
                else
                {
                    r.m_estimatedCompletion = now + m_avgSeekTime;
                }

                if (r.m_operation == Request::OperationType::OT_READ)
                {
                    r.m_estimatedCompletion += AZStd::chrono::microseconds(AZStd::sys_time_t(double(byteSize) / m_readBytesPerMicrosecond));
                }
                else
                {
                    if (byteOffset <= r.m_stream->GetLength())  // with the current file (request at the end will be == to position)
                    {
                        r.m_estimatedCompletion += AZStd::chrono::microseconds(AZStd::sys_time_t(double(byteSize) / m_writeBytesPerMicrosecond));
                    }
                    else
                    {
                        r.m_estimatedCompletion += AZStd::chrono::hours(1); /// don't even consider it for write until we are there
                    }
                }

                ++iter;
            }

            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // sort on deadline and then priority
            m_pending.sort(&Request::Sort);
            //////////////////////////////////////////////////////////////////////////
        }
    } // IO
} // AZ

#endif // #ifndef AZ_UNITY_BUILD
