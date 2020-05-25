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

#include "native/assetprocessor.h"
#include "native/utilities/assetUtils.h"

#include <QString>
#include <QByteArray>
#include <QHash>
#include <QObject>

#include <AzFramework/Asset/AssetSystemTypes.h>

namespace AzFramework
{
    namespace AssetSystem
    {
        class BaseAssetProcessorMessage;
    } // namespace AssetSystem
} // namespace AzFramework

namespace AssetProcessor
{
    //! AssetRequestHandler
    //!  this exists to handle requests from outside sources to compile assets.
    //!  or to get the status of groups of assets.
    class AssetRequestHandler
        : public QObject
    {
        using AssetStatus = AzFramework::AssetSystem::AssetStatus;
        using BaseAssetProcessorMessage = AzFramework::AssetSystem::BaseAssetProcessorMessage;
        Q_OBJECT
    public:

        AssetRequestHandler();
        //! Registration function for message handler
        void RegisterRequestHandler(unsigned int messageId, QObject* object);
        //! Deregistration function for message handler
        void DeRegisterRequestHandler(unsigned int messageId, QObject* object);
    protected:
        //! This function creates a fence file.
        //! It will return the fencefile path if it succeeds, otherwise it returns an empty string  
        virtual QString CreateFenceFile(unsigned int fenceId);
        //! This function delete a fence file.
        //! it will return true if it succeeds, otherwise it returns false.
        virtual bool DeleteFenceFile(QString fenceFileName);

Q_SIGNALS:
        //! Request that a compile group is created for all assets that match that platform and search term.
        //! emitting this signal will ultimately result in OnCompileGroupCreated and OnCompileGroupFinished being executed
        //! at some later time with the same groupID.
        void RequestCompileGroup(NetworkRequestID groupID, QString platform, QString searchTerm, AZ::Data::AssetId assetId, bool isStatusRequest);

        //! This request goes out to ask the system in general whether an asset can be found (as a product).
        void RequestAssetExists(NetworkRequestID groupID, QString platform, QString searchTerm, AZ::Data::AssetId assetId);

        void RequestEscalateAssetByUuid(QString platform, AZ::Uuid escalatedAssetUUID);
        void RequestEscalateAssetBySearchTerm(QString platform, QString escalatedSearchTerm);

    public Q_SLOTS:
        //! ProcessGetAssetStatus - someone on the network wants to know about the status of an asset.
        //! isStatusRequest will be TRUE if its a status request.  If its false it means its a compile request
        void ProcessAssetRequest(NetworkRequestID networkRequestId, BaseAssetProcessorMessage* message, QString platform, bool fencingFailed);

        //! OnCompileGroupCreated is invoked in response to asking for a compile group to be created.
        //! Its status will either be Unknown if no assets are queued or in flight that match that pattern
        //! or it will be Queued or Compiling if some were matched.
        //! If you get a Queued or Compiling, you will eventually get a OnCompileGroupFinished with the same group ID.
        void OnCompileGroupCreated(NetworkRequestID groupID, AssetStatus status);

        //! OnCompileGroupFinished is expected to be called when a compile group completes or fails.
        //! the status is expected to be either Compiled or Failed.
        void OnCompileGroupFinished(NetworkRequestID groupID, AssetStatus status);

        //! Called from the outside in response to a RequestAssetExists.
        void OnRequestAssetExistsResponse(NetworkRequestID groupID, bool exists);

        //! Just return how many in flight requests there are.
        int GetNumOutstandingAssetRequests() const;

        //!  This will get called for every asset related messages or messages that require fencing 
        void OnNewIncomingRequest(unsigned int connId, unsigned int serial, QByteArray payload, QString platform);

        //! This function will only get called when we have ensured that either the message does not require fencing or when we have detected the corresponding fence file for it. 
        virtual void RequestReady(NetworkRequestID networkRequestId, BaseAssetProcessorMessage* message, QString platform, bool fencingFailed = false);

        void OnFenceFileDetected(unsigned int fenceId);

        void DeleteFenceFile_Retry(unsigned int fenceId, QString fenceFileName, NetworkRequestID key, BaseAssetProcessorMessage* message, QString platform, int retriesRemaining);

    private:
        void DispatchAssetCompileResult(QString inputAssetPath, QString platform, AssetStatus status);

        void SendAssetStatus(NetworkRequestID groupID, unsigned int type, AssetStatus status);

        // Invokes the appropriate handler and returns true if the message should be deleted by the caller and false if the request handler is responsible for deleting the message
        bool InvokeHandler(AzFramework::AssetSystem::BaseAssetProcessorMessage* message, NetworkRequestID key, QString platform, bool fencingFailed = false);

        // we keep state about a request in this class:
        class AssetRequestLine
        {
        public:
            AssetRequestLine(QString platform, QString searchTerm, const AZ::Data::AssetId& assetId, bool isStatusRequest);
            bool IsStatusRequest() const;
            QString GetPlatform() const;
            QString GetSearchTerm() const;
            const AZ::Data::AssetId& GetAssetId() const;
            QString GetDisplayString() const;

        private:
            bool m_isStatusRequest;
            QString m_platform;
            QString m_searchTerm;
            AZ::Data::AssetId m_assetId;
        };

        // this map keeps track of whether a request was for a compile (FALSE), or a status (TRUE)
        QHash<NetworkRequestID, AssetRequestLine> m_pendingAssetRequests;

        QHash<unsigned int, QObject*> m_RequestHandlerMap;// map of messageid to request handler

        //! This is an internal struct that is used for storing all the necessary information for requests that require fencing
        struct RequestInfo
        {
            RequestInfo(NetworkRequestID requestId, BaseAssetProcessorMessage* message, QString platform)
                :m_requestId(requestId)
                , m_message(message)
                , m_platform(platform)
            {
            }

            NetworkRequestID m_requestId;
            BaseAssetProcessorMessage* m_message;
            QString m_platform;
        };
        QHash<unsigned int, RequestInfo> m_pendingFenceRequestMap;
        unsigned int m_fenceId = 0;
    };
} // namespace AssetProcessor
