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
#ifndef AZCORE_STREAMER_UTIL_H
#define AZCORE_STREAMER_UTIL_H

#include <AzCore/IO/Streamer.h>
#include <AzCore/std/functional.h>
// need for user synchronization, move to a separate .h
#include <AzCore/std/parallel/semaphore.h>

namespace AZ
{
    namespace IO
    {
        /**
         * SyncRequestCallback allows you to add request and wait till it's done (by blocking your calling thread).
         * example:
         *      SyncRequestCallback doneCB;
         *      GetStreamer()->ReadAsync/ReadAsyncDeferred(stream,0,sizeof(toRead),&toRead,doneCB);
         *      doneCB.Wait();
         */
        class SyncRequestCallback
        {
        public:
            SyncRequestCallback()
                : m_bytesTrasfered(0)
                , m_state(Request::StateType::ST_COMPLETED)
                , m_numToComplete(0)
            {
                m_wrapper = AZStd::bind(&SyncRequestCallback::operator(), this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, AZStd::placeholders::_4);
            }

            void operator()(RequestHandle request, Streamer::SizeType bytesTransfered, void* buffer, Request::StateType state)
            {
                (void)request;
                (void)buffer;
                m_bytesTrasfered = bytesTransfered;

                if (m_state != state)
                {
                    m_state = state;
                }

                m_wait.release();
            }

            operator const Request::RequestDoneCB & ()
            {
                m_numToComplete++;
                return m_wrapper;
            }

            /**
             * Wait in Auto mode
             * IMPORTANT: this code will rely on the fact that (Request::RequestDoneCB) is called ONLY when we pass it to Read or Write
             * request. If this is not true, use WaitManual and specify how many requests do you expect.
             */
            void Wait()
            {
                while (m_numToComplete)
                {
                    m_wait.acquire();
                    --m_numToComplete;
                }
            }

            /// Wait multiple requests.
            void WaitManual(int numToComplete = 1)
            {
                while (numToComplete)
                {
                    m_wait.acquire();
                    --numToComplete;
                }
            }

            Streamer::SizeType         m_bytesTrasfered;    ///< Number of bytes transfered in the LAST request. (read or write)
            Request::StateType m_state;             ///< Request state \ref Streamer::RequestStateType. \note If m_bytesTrasfered doesn't match the request num of bytes state ST_ERROR_FAILED_IN_OPEPATION is set.

        protected:
            SyncRequestCallback(const SyncRequestCallback&);     // because of the mutex this you can not copy or assign
            SyncRequestCallback& operator=(const SyncRequestCallback&);

            AZStd::semaphore        m_wait;
            unsigned int            m_numToComplete;
            Request::RequestDoneCB m_wrapper;
        };
    }
}

#endif // AZCORE_STREAMER_UTIL_H
#pragma once
