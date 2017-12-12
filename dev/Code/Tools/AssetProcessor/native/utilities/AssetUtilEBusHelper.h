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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <QHostAddress>

#include <QByteArray>

//This EBUS broadcasts the platform of the connection the AssetProcessor connected or disconnected with
class AssetProcessorPlaformBusTraits
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus

    virtual ~AssetProcessorPlaformBusTraits() {}
    //Informs that the AP got a connection for this platform.
    virtual void AssetProcessorPlatformConnected(const AZStd::string platform) {}
    //Informs that a connection got disconnected for this platform.
    virtual void AssetProcessorPlatformDisconnected(const AZStd::string platform) {}
};

using AssetProcessorPlatformBus = AZ::EBus<AssetProcessorPlaformBusTraits>;

class ApplicationServerBusTraits
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    typedef AZStd::recursive_mutex MutexType;

    virtual ~ApplicationServerBusTraits() {};

    //! Returns the port the server is set to listen on
    virtual int GetServerListeningPort() const = 0;
};

using ApplicationServerBus = AZ::EBus<ApplicationServerBusTraits>;

namespace AzFramework
{
    namespace AssetSystem
    {
        class BaseAssetProcessorMessage;
    }
}

namespace AssetProcessor
{
    // This bus sends messages to connected clients/proxies identified by their connection ID. The bus
    // is addressed by the connection ID as assigned by the ConnectionManager.
    class ConnectionBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        typedef unsigned int BusIdType;
        typedef AZStd::recursive_mutex MutexType;

        virtual ~ConnectionBusTraits() {}

        // Sends a message to the connection
        virtual size_t Send(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message) = 0;
        // Sends a raw buffer to the connection
        virtual size_t SendRaw(unsigned int type, unsigned int serial, const QByteArray& data) = 0;

        // Sends a message to the connection if the platform match
        virtual size_t SendPerPlatform(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message, const QString& platform) = 0;
        // Sends a raw buffer to the connection if the platform match
        virtual size_t SendRawPerPlatform(unsigned int type, unsigned int serial, const QByteArray& data, const QString& platform) = 0;
    };

    using ConnectionBus = AZ::EBus<ConnectionBusTraits>;

    class MessageInfoBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus
        typedef AZStd::recursive_mutex MutexType;

        virtual ~MessageInfoBusTraits() {}
        //Show a message window to the user
        virtual void NegotiationFailed() {}
        //Shows a message window to the user indicating that proxy settings are wrong
        virtual void ProxyConnectFailed() {}
    };

    using MessageInfoBus = AZ::EBus<MessageInfoBusTraits>;

    class IncomingConnectionInfoBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus
        // this is actually called from multiple threads.
        typedef AZStd::recursive_mutex MutexType;

        virtual ~IncomingConnectionInfoBusTraits() {}
        //Broadcasts the IP address of the incoming connection
        virtual void OnNewIncomingConnection(QHostAddress hostAddress) = 0;
    };

    using IncomingConnectionInfoBus = AZ::EBus<IncomingConnectionInfoBusTraits>;

    typedef AZStd::vector <AssetBuilderSDK::AssetBuilderDesc> BuilderInfoList;

    // This EBUS is used to retrieve asset builder Information
    class AssetBuilderInfoBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus

        virtual ~AssetBuilderInfoBusTraits() {}

        // For a given asset returns a list of all asset builder that are interested in it.
        virtual void GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList) {};
    };

    using  AssetBuilderInfoBus = AZ::EBus<AssetBuilderInfoBusTraits>;

    // This EBUS is used to broadcast information about the currently processing job 
    class ProcessingJobInfoBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus
        typedef AZStd::recursive_mutex MutexType;

        virtual ~ProcessingJobInfoBusTraits() {}

        // Will notify other systems which old product is just about to get removed from the cache 
        // before we copy the new product instead along. 
        virtual void BeginIgnoringCacheFileDelete(const AZStd::string /*productPath*/) {};
        // Will notify other systems which product we are trying to copy in the cache 
        // along with status of whether that copy succeeded or failed.
        virtual void StopIgnoringCacheFileDelete(const AZStd::string /*productPath*/, bool /*queueAgainForProcessing*/) {};
    };

    using ProcessingJobInfoBus = AZ::EBus<ProcessingJobInfoBusTraits>;

    // This EBUS is used to issue requests to the AssetCatalog
    class AssetRegistryRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus
        typedef AZStd::recursive_mutex MutexType;

        // This function will either return the registry version of the next registry save or of the current one, if it is in progress
        // It will not put another save registry event in the event pump if we are currently in the process of saving the registry
        virtual int SaveRegistry() = 0;
    };

    typedef AZ::EBus<AssetRegistryRequests> AssetRegistryRequestBus;

    // This EBUS issues notifications when the catalog begins and finishes saving the asset registry
    class AssetRegistryNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // single listener
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus
        typedef AZStd::recursive_mutex MutexType;

        // The asset catalog has finished saving the registry
        virtual void OnRegistrySaveComplete(int /*assetCatalogVersion*/, bool /*allCatalogsSaved*/) {}
    };

    using AssetRegistryNotificationBus = AZ::EBus<AssetRegistryNotifications>;
} // namespace AssetProcessor
