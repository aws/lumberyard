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

#include <AzCore/IO/OpticalDevice.h>
#include <AzCore/IO/StreamerDrillerBus.h>
#include <AzCore/IO/VirtualStream.h>

namespace AZ
{
    namespace IO
    {
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Optical Device
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //=========================================================================
        // OpticalDevice
        // [8/29/2012]
        //=========================================================================
        OpticalDevice::OpticalDevice(const AZStd::string& name, FileIOBase* ioBase, unsigned int cacheBlockSize, unsigned int numCacheBlocks, const AZStd::thread_desc* threadDesc, int threadSleepTimeMS)
            : Device(name, ioBase, threadDesc, threadSleepTimeMS)
        {
            m_type = DT_OPTICAL_MEDIA;
            m_layerSwitch = AZStd::chrono::milliseconds::zero();
            m_layerOffsetLBA = SizeType(-1); // no layers
            m_sectorSize = 2048;
            m_eccBlockSize = 32 * 1024;
            AZ_Assert((cacheBlockSize % m_sectorSize) == 0, "Cache block size %d must be multiple of the sector %d size!", cacheBlockSize, m_sectorSize);
            CreateCache(cacheBlockSize, numCacheBlocks);
        }

        //=========================================================================
        // EstimateSeek
        // [8/29/2012]
        //=========================================================================
        inline AZStd::chrono::microseconds OpticalDevice::EstimateSeek(SizeType currentLBA, SizeType targetLBA) const
        {
            AZStd::chrono::microseconds est(0);
            if ((currentLBA < m_layerOffsetLBA && targetLBA >= m_layerOffsetLBA))
            {
                est += m_layerSwitch;
                currentLBA += m_layerOffsetLBA;
            }
            else if (currentLBA > m_layerOffsetLBA && targetLBA < m_layerOffsetLBA)
            {
                est += m_layerSwitch;
                targetLBA += m_layerOffsetLBA;
            }

            SizeType offsetLBA;
            //bool isForward;
            if (currentLBA < targetLBA)
            {
                offsetLBA = targetLBA - currentLBA;
                //isForward = true;
            }
            else
            {
                offsetLBA = currentLBA - targetLBA;
                //isForward = false;
            }

            if (offsetLBA > 32)
            {
                est += AZStd::chrono::milliseconds(100);
            }


            return est;
        }

        //=========================================================================
        // EstimateRead
        // [8/29/2012]
        //=========================================================================
        inline AZStd::chrono::microseconds OpticalDevice::EstimateRead(SizeType currentLBA, SizeType targetLBA, SizeType byteSize) const
        {
            AZStd::chrono::microseconds est(0);
            if (currentLBA != targetLBA)
            {
                est += EstimateSeek(currentLBA, targetLBA);
            }

            double bytesPerMicroSecondRead;
            bytesPerMicroSecondRead = 5.0 * 1024.0 * 1024.0 / 1000000.0 /* to microsecond */;

            est += AZStd::chrono::microseconds(AZStd::sys_time_t(double(byteSize) / bytesPerMicroSecondRead));
            return est;
        }

        //=========================================================================
        // ScheduleRequests
        // [8/29/2012]
        //=========================================================================
        void OpticalDevice::ScheduleRequests()
        {
            if (m_pending.size() <= 1)
            {
                // if we have one request and it's in the cache
                if (!m_pending.empty() && m_readCache && m_readCache->IsInCache(&m_pending.front()))
                {
                    Request* req = &m_pending.front();
                    EBUS_DBG_EVENT(StreamerDrillerBus, OnReadCacheHit, req->m_stream, req->GetStreamByteOffset(), req->m_byteSize, req->m_debugName);
                    m_pending.pop_front();
                    CompleteRequest(req);
                }
                return;
            }

            //////////////////////////////////////////////////////////////////////////
            // estimate completion for all requests
            AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
            SizeType currentLBA, targetLBA;
            if (m_isStreamOffsetSupport)
            {
                currentLBA = m_currentStream ? (m_currentStreamOffset / m_eccBlockSize) : 0;
                for (Request::ListType::iterator iter = m_pending.begin(); iter != m_pending.end(); )
                {
                    Request& r = *iter;
                    if (m_readCache && m_readCache->IsInCache(&r))  // check read cache
                    {
                        EBUS_DBG_EVENT(StreamerDrillerBus, OnReadCacheHit, r.m_stream, r.GetStreamByteOffset(), r.m_byteSize, r.m_debugName);
                        iter = m_pending.erase(iter);
                        CompleteRequest(&r);
                        continue;
                    }
                    else
                    {
                        SizeType byteOffset = r.GetStreamByteOffset() + r.m_bytesProcessedStart;
                        SizeType byteSize = r.m_byteSize - (r.m_bytesProcessedStart + r.m_bytesProcessedEnd);
                        targetLBA = (byteOffset / m_eccBlockSize);

                        r.m_estimatedCompletion = now + EstimateRead(currentLBA, targetLBA, byteSize);

                        ++iter;
                    }
                }
            }
            else
            {
                unsigned int avgSeekMs = 100;
                currentLBA = m_currentStream ? (m_currentStreamOffset / m_sectorSize) : 0;
                for (Request::ListType::iterator iter = m_pending.begin(); iter != m_pending.end(); )
                {
                    Request& r = *iter;
                    if (m_readCache && m_readCache->IsInCache(&r))  // check read cache
                    {
                        EBUS_DBG_EVENT(StreamerDrillerBus, OnReadCacheHit, r.m_stream, r.GetStreamByteOffset(), r.m_byteSize, r.m_debugName);
                        iter = m_pending.erase(iter);
                        CompleteRequest(&r);
                        continue;
                    }
                    else
                    {
                        SizeType byteSize = r.m_byteSize - (r.m_bytesProcessedStart + r.m_bytesProcessedEnd);
                        if (r.m_stream == m_currentStream)  // if we are with in the same stream we can compute relative LBAs
                        {
                            SizeType byteOffset = r.GetStreamByteOffset() + r.m_bytesProcessedStart;
                            targetLBA = byteOffset / m_sectorSize;
                            r.m_estimatedCompletion = now + EstimateRead(currentLBA, targetLBA, byteSize);
                        }
                        else
                        {
                            r.m_estimatedCompletion = now + AZStd::chrono::milliseconds(avgSeekMs) + EstimateRead(m_layerOffsetLBA / 2, m_layerOffsetLBA / 2, byteSize);
                        }
                        ++iter;
                    }
                }
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
