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
#pragma once

#include <AzCore/base.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/queue.h>

namespace AZ
{
    namespace IO
    {
        class StreamerContext
        {
        public:
            ~StreamerContext();

            //! Gets a new file request, either by creating a new instance or
            //! picking one from the recycle bin. This version should only be used
            //! by nodes on the streaming stack as it's not thread safe, but faster.
            //! The scheduler will automatically recycle these requests.
            FileRequest* GetNewInternalRequest();

            //! Gets a new file request, either by creating a new instance or
            //! picking one from the recycle bin. This version is for use by 
            //! any system outside the stream stack and is thread safe. The owner
            //! needs to manually recycle these requests once they're done.
            FileRequest* GetNewExternalRequest();

            //! Marks a request as completed so the main thread in Streamer can close it out.
            void MarkRequestAsCompleted(FileRequest* request);
            //! Adds an old request to the recycle bin so it can be reused later.
            void RecycleRequest(FileRequest* request);

            //! Returns true if there are any requests marked as completed, but not yet finalized. Requests are
            //! finalized by calling FinalizeCompletedRequests.
            bool HasRequestsToFinalize();
            //! Does the FinalizeRequest callback where appropriate and does some bookkeeping to finalize requests.
            //! @completedRequestLinks Container where completed request of type "RequesLink" will be added to.
            //! @return True if any requests were finalized, otherwise false.
            // The completedRequestLinks are a temporary addition until Device can be fully deprecated in favor of the stream stack.
            bool FinalizeCompletedRequests(AZStd::vector<FileRequest*>& completedRequestLinks);

            //! Causes the main thread for streamer to wake up and process any pending requests. If the thread
            //! is already awake, nothing happens.
            void WakeUpMainStreamThread();
            AZStd::mutex& GetThreadSleepLock();
            AZStd::condition_variable& GetThreadSleepCondition();

        private:
            static constexpr size_t s_initialRecycleBinSize = 64;

            FileRequest* GetNewRequest(AZStd::vector<FileRequest*>& recycleBin, FileRequest::Usage usage);
            void RecycleRequest(FileRequest* request, AZStd::vector<FileRequest*>& recycleBin);

            AZStd::mutex m_externalRecycleBinGuard;
            AZStd::vector<FileRequest*> m_externalRecycleBin;
            AZStd::vector<FileRequest*> m_internalRecycleBin;
            
            // The completion is guarded so other threads can perform async IO and safely mark requests as completed.
            AZStd::recursive_mutex m_completedGuard;
            AZStd::queue<FileRequest*> m_completed;
            
            AZStd::mutex m_threadSleepLock;
            AZStd::condition_variable m_threadSleepCondition;
        };
    } // namespace IO
} // namespace AZ