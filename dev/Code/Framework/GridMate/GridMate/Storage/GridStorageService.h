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
#ifndef GM_STORAGE_H
#define GM_STORAGE_H

#include <GridMate/GridMate.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    namespace IO
    {
        class GenericStream;
    }   // namespace IO
}   // namespace AZ

namespace GridMate
{
    class ILocalMember;

    enum GridStorageType
    {
        StorageType_Global,
        StorageType_User,
        StorageType_SaveData,
    };

    enum GridStorageOpResult
    {
        StorageOp_Completed,
        StorageOp_Failed,
        StorageOp_Canceled,
        StorageOp_Pending,
    };

    struct GridStorageFileInfo
    {
        GridStorageFileInfo(const char* path, size_t length)
            : m_path(path)
            , m_length(length) {}

        const char* m_path;
        size_t      m_length;
    };

    struct GridStorageServiceDesc
    {
    };

    namespace Internal
    {
        //-------------------------------------------------------------------------
        //-------------------------------------------------------------------------
        class GridStorageCallbacks
            : public GridMateEBusTraits
        {
        public:
            typedef GridStorageCallbacks* BusIdType;

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            static const bool EnableEventQueue = true;

            virtual ~GridStorageCallbacks() {}

            virtual bool Enumerate(const GridStorageFileInfo& info, GridStorageType storage, ILocalMember* requestor) = 0;
            virtual void OnEnumerationCompleted(GridStorageOpResult result, GridStorageType storage, ILocalMember* requestor) = 0;

            virtual void OnLoadCompleted(GridStorageOpResult result, const void* data, AZ::u64 dataSize, const char* path, GridStorageType storage, ILocalMember* requestor) = 0;
            virtual void OnSaveCompleted(GridStorageOpResult result, const char* path, GridStorageType storage, ILocalMember* requestor) = 0;
            virtual void OnDeleteCompleted(GridStorageOpResult result, const char* path, GridStorageType storage, ILocalMember* requestor) = 0;
            virtual void OnQueryExistsCompleted(GridStorageOpResult result, bool exists, const char* path, GridStorageType storage, ILocalMember* requestor) = 0;
            virtual void OnQueryAvailableSpaceCompleted(GridStorageOpResult result, AZ::u64 availableSpace, GridStorageType storage, ILocalMember* requestor) = 0;
        };
        //-------------------------------------------------------------------------
        typedef AZ::EBus<GridStorageCallbacks> GridStorageCallbackBus;
        //-------------------------------------------------------------------------
    }   // namespace Internal

    /**
     * GridStorageCallbackHandler
     */
    class GridStorageCallbackHandler
        : public Internal::GridStorageCallbackBus::Handler
    {
    public:
        void BusConnect() { Internal::GridStorageCallbackBus::Handler::BusConnect(this); }

        virtual bool Enumerate(const GridStorageFileInfo& info, GridStorageType storage, ILocalMember* requestor) = 0;
        virtual void OnEnumerationCompleted(GridStorageOpResult result, GridStorageType storage, ILocalMember* requestor) = 0;
        virtual void OnLoadCompleted(GridStorageOpResult result, const void* data, AZ::u64 dataSize, const char* path, GridStorageType storage, ILocalMember* requestor) = 0;
        virtual void OnSaveCompleted(GridStorageOpResult result, const char* path, GridStorageType storage, ILocalMember* requestor) = 0;
        virtual void OnDeleteCompleted(GridStorageOpResult result, const char* path, GridStorageType storage, ILocalMember* requestor) = 0;
        virtual void OnQueryExistsCompleted(GridStorageOpResult result, bool exists, const char* path, GridStorageType storage, ILocalMember* requestor) = 0;
        virtual void OnQueryAvailableSpaceCompleted(GridStorageOpResult result, AZ::u64 availableSpace, GridStorageType storage, ILocalMember* requestor) = 0;
    };

    /**
     * GridStorage
     * Interface to Gridmate's online storage functionality.
     * All operations are async and results are always reported from the service's update thread
     */
    class GridStorageService
    {
    public:
        virtual ~GridStorageService()   {}

        virtual void Init(IGridMate* gridmate) = 0;
        virtual void Shutdown() = 0;
        virtual void Update() = 0;
        virtual bool IsReady() const = 0;

        virtual void Enumerate(const char* path, GridStorageType storage, ILocalMember* requestor, GridStorageCallbackHandler* handler) = 0;
        virtual void Load(const char* path, GridStorageType storage, ILocalMember* requestor, GridStorageCallbackHandler* handler) = 0;
        virtual void Save(const char* path, GridStorageType storage, const void* data, AZ::u64 dataSize, ILocalMember* requestor, GridStorageCallbackHandler* handler) = 0;
        virtual void Delete(const char* path, GridStorageType storage, ILocalMember* requestor, GridStorageCallbackHandler* handler) = 0;
        virtual void QueryExists(const char* path, GridStorageType storage, ILocalMember* requestor, GridStorageCallbackHandler* handler) = 0;
        virtual void QueryAvailableSpace(GridStorageType storage, ILocalMember* requestor, GridStorageCallbackHandler* handler) = 0;
    };
} // namespace GridMate

#endif  // GM_STORAGE_H
#pragma once
