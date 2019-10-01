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

#include <AzCore/std/algorithm.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>

namespace AZ
{
    namespace IO
    {
        StreamerContext::~StreamerContext()
        {
            for (FileRequest* entry : m_internalRecycleBin)
            {
                delete entry;
            }

            AZStd::lock_guard<AZStd::mutex> guard(m_externalRecycleBinGuard);
            for (FileRequest* entry : m_externalRecycleBin)
            {
                delete entry;
            }
        }

        FileRequest* StreamerContext::GetNewInternalRequest()
        {
            return GetNewRequest(m_internalRecycleBin, FileRequest::Usage::Internal);
        }

        FileRequest* StreamerContext::GetNewExternalRequest()
        {
            AZStd::lock_guard<AZStd::mutex> guard(m_externalRecycleBinGuard);
            return GetNewRequest(m_externalRecycleBin, FileRequest::Usage::External);
        }

        void StreamerContext::MarkRequestAsCompleted(FileRequest* request)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> guard(m_completedGuard);
            AZ_Assert(AZStd::find(m_completed.get_container().begin(), m_completed.get_container().end(), request) == m_completed.get_container().end(),
                "Request that's already been marked for deletion has been added again.");
            m_completed.push(request);
        }

        void StreamerContext::RecycleRequest(FileRequest* request)
        {
            if (request->m_usage == FileRequest::Usage::Internal)
            {
                RecycleRequest(request, m_internalRecycleBin);
            }
            else
            {
                AZStd::lock_guard<AZStd::mutex> guard(m_externalRecycleBinGuard);
                RecycleRequest(request, m_externalRecycleBin);
            }
        }

        bool StreamerContext::HasRequestsToFinalize()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> guard(m_completedGuard);
            return !m_completed.empty();
        }

        bool StreamerContext::FinalizeCompletedRequests(AZStd::vector<FileRequest*>& completedRequestLinks)
        {
            while (true)
            {
                AZStd::queue<FileRequest*> completed;
                {
                    AZStd::lock_guard<AZStd::recursive_mutex> guard(m_completedGuard);
                    completed.swap(m_completed);
                }
                if (completed.empty())
                {
                    return false;
                }

                while (!completed.empty())
                {
                    FileRequest* top = completed.front();
                    if (top->m_owner)
                    {
                        top->m_owner->FinalizeRequest(top);
                    }
                    if (top->m_parent)
                    {
                        AZ_Assert(top->m_parent->m_dependencies > 0, 
                            "A file request with a parent has completed, but the request wasn't registered as a dependency with the parent.");
                        --top->m_parent->m_dependencies;

                        top->m_parent->SetStatus(top->GetStatus());

                        if (top->m_parent->m_dependencies == 0)
                        {
                            completed.push(top->m_parent);
                        }
                    }

                    if (top->GetOperationType() == FileRequest::Operation::RequestLink)
                    {
                        // Temporarily needed until Device has been fully deprecated in favor of the streamer stack.
                        top->SetStatus(FileRequest::Status::Completed);
                        completedRequestLinks.push_back(top);
                    }
                    else if (top->m_usage == FileRequest::Usage::Internal)
                    {
                        RecycleRequest(top);
                    }

                    completed.pop();
                }
            }
            return true;
        }

        void StreamerContext::WakeUpMainStreamThread()
        {
            m_threadSleepCondition.notify_one(); 
        }

        AZStd::mutex& StreamerContext::GetThreadSleepLock()
        {
            return m_threadSleepLock;
        }

        AZStd::condition_variable& StreamerContext::GetThreadSleepCondition()
        {
            return m_threadSleepCondition;
        }

        FileRequest* StreamerContext::GetNewRequest(AZStd::vector<FileRequest*>& recycleBin, FileRequest::Usage usage)
        {
            if (recycleBin.empty())
            {
                if (recycleBin.capacity() > 0)
                {
                    // There are no requests left in the recycle bin so create a new one. This will
                    // eventually end up in the recycle bin again.
                    return aznew FileRequest(usage);
                }
                else
                {
                    // Create a few request up front so the engine initialization doesn't
                    // constantly create new instances and resizes the recycle bin.
                    recycleBin.reserve(s_initialRecycleBinSize);
                    for (size_t i = 0; i < s_initialRecycleBinSize; ++i)
                    {
                        recycleBin.push_back(aznew FileRequest(usage));
                    }
                }
            }

            FileRequest* result = recycleBin.back();
            recycleBin.pop_back();
            return result;
        }

        void StreamerContext::RecycleRequest(FileRequest* request, AZStd::vector<FileRequest*>& recycleBin)
        {
            AZ_Assert(AZStd::find(recycleBin.begin(), recycleBin.end(), request) == recycleBin.end(),
                "Request that's already been recycled has been added again.");
            request->Reset();
            recycleBin.push_back(request);
        }
    } // namespace IO
} // namespace AZ