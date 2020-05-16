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
#include <AzCore/std/string/string_view.h>
#include <AzCore/Math/Crc.h> // ensure that AZ_CRC is available to all users of this header

#include <AzFramework/Asset/AssetSystemTypes.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>

namespace AzFramework
{
    namespace AssetSystem
    {
        //! AssetSystemInfoBusTraits
        //! This bus is for events that occur in the asset system in general, and has no address
        class AssetSystemInfoNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;   // single bus

            virtual ~AssetSystemInfoNotifications() = default;

            //! Notifies listeners that the Asset Processor has claimed a file in the cache for updating.
            //! The absolute path is provided. This call will be followed by AssetFileReleased.
            virtual void AssetFileClaimed(const AZStd::string& /*assetPath*/) {}
            //! Notifies listeners that the Asset Processor has released a file in the cache it previously 
            // exclusively claimed with AssetFileClaim. The absolute path is provided.
            virtual void AssetFileReleased(const AZStd::string& /*assetPath*/) {}
            //! Notifies listeners the compilation of an asset has started.
            virtual void AssetCompilationStarted(const AZStd::string& /*assetPath*/) {}
            //! Notifies listeners the compilation of an asset has succeeded.
            virtual void AssetCompilationSuccess(const AZStd::string& /*assetPath*/) {}
            //! Notifies listeners the compilation of an asset has failed.
            virtual void AssetCompilationFailed(const AZStd::string& /*assetPath*/) {}
            //! Returns the number of assets in queue for processing.
            virtual void CountOfAssetsInQueue(const int& /*count*/) {}
            //! Notifies listeners an error has occurred in the asset system
            virtual void OnError(AssetSystemErrors /*error*/) {}
        };

        //! AssetSystemRequestBusTraits
        //! This bus is for making requests to the asset system
        class AssetSystemRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;   // single listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;   // single bus
            
            using MutexType = AZStd::recursive_mutex;
            static const bool LocklessDispatch = true; // no reason to block other threads when a thread is waiting for a response from AP.

            virtual ~AssetSystemRequests() = default;

            //! Configure the underlying socket connection
            virtual bool ConfigureSocketConnection(const AZStd::string& branch, const AZStd::string& platform, const AZStd::string& identifier, const AZStd::string& projectName) = 0;
            //! Configure the underlying socket connection and try to connect to the Asset processor            
            //! \param identifier A name for the connection that will show up in the asset processor dialog
            //! \return Whether the connection was established
            virtual bool Connect(const char* identifier) = 0;
            //! Configure the underlying socket connection and try to connect to the Asset processor            
            //! \param identifier A name for the connection that will show up in the asset processor dialog
            //! \param timeout Time in seconds which the function waits while trying to create a connection.
            //! \return Whether the connection was established
            virtual bool ConnectWithTimeout(const char* identifier, AZStd::chrono::duration<float> timeout) = 0;
            //! Disconnect from the underlying socket connection to the Asset processor if connected, otherwise this function does nothing.
            virtual bool Disconnect() = 0;
            
            
            /** CompileAssetSync
             * Compile an asset synchronously.  This will only return after compilation, and also escalates it so that it builds immediately.
             * Note that the asset path will be heuristically matched like a search term, so things missing an extension or things that
             * are just a folder name will cause all assets which match that search term to be escalated and compiled.
             * PERFORMANCE WARNING: Only use the FlushIO version if you have just written an asset file you wish to immediately compile,
             *     potentially before the operating system's disk IO queue is finished writing it.
             *     It will force a flush of the OS file monitoring queue before considering the request.
            **/
            virtual AssetStatus CompileAssetSync(const AZStd::string& assetPath) = 0;
            virtual AssetStatus CompileAssetSync_FlushIO(const AZStd::string& assetPath) = 0;

            virtual AssetStatus CompileAssetSyncById(const AZ::Data::AssetId& assetId) = 0;
            virtual AssetStatus CompileAssetSyncById_FlushIO(const AZ::Data::AssetId& assetId) = 0;
            
            /** GetAssetStatusByUuid
            * Retrieve the status of an asset synchronously and  also escalate it so that it builds sooner than others that are not
            * escalated.  If possible, prefer this function over the string-based version below.
            * PERFORMANCE WARNING: Only use the FlushIO version if you have just written an asset file you wish to immediately compile,
            *     potentially before the operating system's disk IO queue is finished writing it.
            *     It will force a flush of the OS file monitoring queue before considering the request.
            **/
            virtual AssetStatus GetAssetStatusById(const AZ::Data::AssetId& assetId) = 0;
            virtual AssetStatus GetAssetStatusById_FlushIO(const AZ::Data::AssetId& assetId) = 0;

            /** GetAssetStatus
             * Retrieve the status of an asset synchronously and  also escalate it so that it builds sooner than others that are not escalated.
             * @param assetPath - a relpath to a product in the cache, or a relpath to a source file, or a full path to either
             * PERFORMANCE WARNING: Only use the FlushIO version if you have just written an asset file you wish to immediately query the status of,
             *     potentially before the operating system's disk IO queue is finished writing it.
             *     It will force a flush of the OS file monitoring queue before considering the request.
            **/
            virtual AssetStatus GetAssetStatus(const AZStd::string& assetPath) = 0;
            virtual AssetStatus GetAssetStatus_FlushIO(const AZStd::string& assetPath) = 0;

            /** Request that a particular asset be escalated to the top of the build queue, by uuid
             *  This is an async request - the return value only indicates whether it was sent, not whether it escalated or was found.
             *  Note that the Uuid of an asset is the Uuid of its source file.  If you have an AssetId field, this is the m_uuid part
             *  inside the AssetId, since that refers to the source file that produced the asset.
             *  @param assetUuid - the uuid to look up.
             * note that this request always flushes IO (on the AssetProcessor side), but you don't pay for it in the caller
             * process since it is a fire-and-forget message.  This means its the fastest possible way to reliably escalate an asset by UUID
            **/ 
            virtual bool EscalateAssetByUuid(const AZ::Uuid& assetUuid) = 0;
            
            /** EscalateAssetBySearchTerm
              * Request that a particular asset be escalated to the top of the build queue, by "search term" (ie, source file name)
              * This is an async request - the return value only indicates whether it was sent, not whether it escalated or was found.
              * Search terms can be:
              *     Source File Names
              *     fragments of source file names
              *     Folder Names or pieces of folder names
              *     Product file names
              *     fragments of product file names
              * The asset processor will find the closest match and escalate it.  So for example if you request escalation on
              * "mything.dds" and no such SOURCE FILE exists, it may match mything.fbx heuristically after giving up on the dds.
              * If possible, use the above EscalateAsset with the Uuid, which does not require a heuristic match, or use
              * the source file name (relative or absolute) as the input, instead of trying to work with product names.
              *  @param searchTerm - see above
              *
              * note that this request always flushes IO (on the AssetProcessor side), but you don't pay for it in the caller
              * process since it is a fire-and-forget message.  This means its the fastest possible way to reliably escalate an asset by name
             **/
            virtual bool EscalateAssetBySearchTerm(AZStd::string_view searchTerm) = 0;

            /** Returns the number of unresolved AssetId and path references for the given asset.
             * These are product assets that the given asset refers to which are not yet known by the Asset Processor.
             * This API can be used to determine if a given asset can safely be loaded and have its asset references resolve successfully.
             * @param assetId - Asset to lookup
             * @param unresolvedAssetIdReferences - number of AssetId-based references which are unresolved
             * @param unresolvedPathReferences - number of path-based references which are unresolved.  This count excludes wildcard references which are never resolved
            **/
            virtual void GetUnresolvedProductReferences(AZ::Data::AssetId assetId, AZ::u32& unresolvedAssetIdReferences, AZ::u32& unresolvedPathReferences) = 0;

            //! Show the AssetProcessor App
            virtual void ShowAssetProcessor() = 0;
            //! Show an asset in the AssetProcessor App
            virtual void ShowInAssetProcessor(const AZStd::string& assetPath) = 0;
            //! Sets the asset processor port to use when connecting
            virtual void SetAssetProcessorPort(AZ::u16 port) = 0;
            //! Sets the asset processor IP to use when connecting
            virtual void SetAssetProcessorIP(const AZStd::string& ip) = 0;
            //! Sets the branchtoken that will be used for negotiating with the AssetProcessor
            virtual void SetBranchToken(const AZStd::string& branchtoken) = 0;
            //! Sets the (game) project name that will be used for negotiating with the AssetProcessor
            virtual void SetProjectName(const AZStd::string& projectName) = 0;

            //! Compute the ping time between this client and the Asset Processor that's actually handling our requests (proxy relaying is included in the time)
            virtual float GetAssetProcessorPingTimeMilliseconds() = 0;
            //! Saves the catalog synchronously
            virtual bool SaveCatalog() = 0;
        };

        //! AssetSystemNegotiationBusTraits
        //! This bus is for events that occur during negotiation
        class AssetSystemConnectionNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;   // single bus

            virtual ~AssetSystemConnectionNotifications() = default;

            //! Notifies listeners negotiation with Asset Processor failed
            virtual void NegotiationFailed() {};

            //! Notifies listeners that connection to the Asset Processor failed
            virtual void ConnectionFailed() {};
        };

        namespace ConnectionIdentifiers
        {
            static const char* Editor = "EDITOR";
            static const char* Game = "GAME";
        }

        //! AssetSystemStatusBusTraits
        //! This bus is for AssetSystem status change
        class AssetSystemStatus
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;   // single bus

            virtual ~AssetSystemStatus() = default;

            //! Notifies listeners Asset System turns available
            virtual void AssetSystemAvailable() {}
            //! Notifies listeners Asset System turns not available
            virtual void AssetSystemUnavailable() {}
        };

    } // namespace AssetSystem

    /**
    * AssetSystemBus removed - if you have a system which was using it
    * use AssetCatalogEventBus for asset updates
    */
    using AssetSystemInfoBus = AZ::EBus<AssetSystem::AssetSystemInfoNotifications>;
    using AssetSystemRequestBus = AZ::EBus<AssetSystem::AssetSystemRequests>;
    using AssetSystemConnectionNotificationsBus = AZ::EBus<AssetSystem::AssetSystemConnectionNotifications>;
    using AssetSystemStatusBus = AZ::EBus<AssetSystem::AssetSystemStatus>;
} // namespace AzFramework
