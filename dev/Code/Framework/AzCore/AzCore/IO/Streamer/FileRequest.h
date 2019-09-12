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
#include <AzCore/IO/CompressionBus.h>
#include <AzCore/IO/StreamerRequest.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/chrono/clocks.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/string/string_view.h>

namespace AZStd
{
    class semaphore;
}

namespace AZ
{
    namespace IO
    {
        namespace Util
        {
            template<typename... Args>
            struct MaxSize
            {
                static constexpr size_t s_maxSize = 0;
            };

            template<typename T, typename... Args>
            struct MaxSize<T, Args...>
            {
                static constexpr size_t s_argSize = MaxSize<Args...>::s_maxSize;
                static constexpr size_t s_maxSize = sizeof(T) >= s_argSize ? sizeof(T) : s_argSize;
            };
        }

        class StreamStackEntry;

        class FileRequest
        {
        public:
            friend class StreamerContext;

            struct RequestLinkData
            {
                ~RequestLinkData();
                AZStd::shared_ptr<Request> m_request; //!< The original request before it entered the streaming stack.
                AZStd::semaphore* m_cancelSync{ nullptr }; //!< If set this semaphore will signaled upon completion.
            };

            struct ReadData
            {
                void* m_output; //!< Target output to write the read data to.
                const RequestPath* m_path; //!< The path to the file that contains the requested data.
                u64 m_offset; //!< The offset in bytes into the file.
                u64 m_size; //!< The number of bytes to read from the file.
            };

            struct CompressedReadData
            {
                CompressionInfo m_compressionInfo;
                void* m_output; //!< Target output to write the read data to.
                u64 m_readOffset; //!< The offset into the decompressed to start copying from.
                u64 m_readSize; //!< Number of bytes to read from the decompressed file.
            };

            struct CancelData
            {
                FileRequest* m_target; //!< The request that will be canceled.
            };
            
            AZ_CLASS_ALLOCATOR(FileRequest, SystemAllocator, 0);

            enum class Operation : u8
            {
                None, //!< No operation assigned.
                RequestLink, //!< Link to the original request that came from outside the stream stack.
                Read, //!< Request to read data.
                CompressedRead, //!< Request to read and decompress data.
                Wait, //!< Holds the progress of the operation chain until this request is explicitly completed.
                Cancel, //!< Cancels a request in the stream stack, if possible. 
                        // Note: it's not safe to cancel a request that has children as they wouldn't be notified about the parent
                        // no longer existing. Requests that have children should instead react to callback with Status::Canceled.
            };
            enum class Status
            {
                Pending, //!< The request has been registered but is waiting to be processed.
                Queued, //!< The request is queued somewhere on in the stack and waiting further processing.
                Processing, //!< The request is being processed.
                Completed, //!< The request has successfully completed.
                Canceled, //!< The request was canceled. This can still mean the request was partially completed.
                Failed //!< The request has failed to complete and can't be processed any further.
            };

            enum class Usage : u8
            {
                Internal,
                External
            };

            void CreateRequestLink(AZStd::shared_ptr<Request>&& m_request);
            void CreateRead(StreamStackEntry* owner, FileRequest* parent, void* output, const RequestPath& path, u64 offset, u64 size);
            void CreateCompressedRead(StreamStackEntry* owner, FileRequest* parent, const CompressionInfo& compressionInfo, void* output, u64 readOffset, u64 readSize);
            void CreateCompressedRead(StreamStackEntry* owner, FileRequest* parent, CompressionInfo&& compressionInfo, void* output, u64 readOffset, u64 readSize);
            void CreateWait(StreamStackEntry* owner, FileRequest* parent);
            void CreateCancel(FileRequest* target);

            RequestLinkData& GetRequestLinkData();
            ReadData& GetReadData();
            CompressedReadData& GetCompressedReadData();
            FileRequest* GetCanceledRequest();
            
            Status GetStatus() const;
            void SetStatus(Status newStatus);
            Operation GetOperationType() const;
            StreamStackEntry* GetOwner();
            FileRequest* GetParent();

            //! Determines if this request is part of the chain that contains the provided request.
            bool IsChildOf(FileRequest* parent) const;

            //! Set the estimated completion time for this request and it's immediate parent. The general approach
            //! to getting the final estimation is to bubble up the estimation, with ever entry in the stack adding
            //! it's own additional delay.
            void SetEstimatedCompletion(AZStd::chrono::system_clock::time_point time);
            AZStd::chrono::system_clock::time_point GetEstimatedCompletion() const;
            
        private:
            explicit FileRequest(Usage usage);
            ~FileRequest();

            void Reset();

            //! Data for the operation.
            //! Uses a poor mans version of std::variant.
            alignas(128) u8 m_payload[Util::MaxSize<RequestLinkData, ReadData, CompressedReadData, CancelData>::s_maxSize];
            //! Estimated time this request will complete. This is an estimation and depends no many
            //! factors which can cause it to change drastically from moment to moment.
            AZStd::chrono::system_clock::time_point m_estimatedCompletion;
            //! The file request that has a dependency on this one. This can be null if there are no
            //! other request depending on this one to complete.
            FileRequest* m_parent{ nullptr };
            //! The stack entry that created and issued this request. Once the request has completed
            //! this entry will be called to finalize the request. If the owner is null it means the
            //! request originated outside the stream stack and was the original request.
            StreamStackEntry* m_owner{ nullptr };
            AZStd::atomic<Status> m_status{ Status::Pending };
            //! The number of dependent file request that need to complete before this one is done.
            u16 m_dependencies{ 0 };
            //! The operation this request needs to execute. The data for the operation is stored in m_payload.
            Operation m_operation{ Operation::None };
            //! Internal request. If this is true the request is created inside the streaming stack and never
            //! leaves it. If true it will automatically be maintained by the scheduler, if false than it's
            //! up to the owner to recycle this request.
            Usage m_usage{ Usage::Internal };
        };
    } // namespace IO
} // namespace AZ