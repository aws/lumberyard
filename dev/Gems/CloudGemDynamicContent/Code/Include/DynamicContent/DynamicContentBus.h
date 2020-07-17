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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace CloudCanvas
{
    namespace DynamicContent
    {

        class IDynamicContentTransferManager
            : public AZ::ComponentBus
        {
        public:

            virtual bool RequestManifest(const char* manifestName) { return false; }
            virtual bool RequestFileStatus(const char* fileName, const char* outputFile) { return false; }
            //! Request status for a list of bundles including hash, size, presigned url
            //! RequestVec is a list of strings corresponding to bucket keys
            virtual bool UpdateFileStatusList(const AZStd::vector<AZStd::string>& requestVec, bool autoDownload = false) { return false; }
            //! Update status for a single file including hash, size, presigned url
            virtual bool UpdateFileStatus(const char* fileName, bool autoDownload = false) { return false; }
            //! Request the download of a file for which the Presigned URL is already known (Was retrieved through RequestFileStatusList)
            //! fileName is a bucket key matching that used in RequestFileStatusList
            //! forceDownload indicates whether we want to re-download the file if it's not updated
            virtual bool RequestDownload(const AZStd::string& fileName, bool forceDownload) { return false; }
            virtual bool ClearAllContent() { return false; }
            virtual bool RemovePak(const char* fileName) { return false; }
            virtual bool LoadManifest(const AZStd::string& manifestName) { return false; }
            virtual bool LoadPak(const AZStd::string& fileName) { return false; }
            virtual bool HasUpdatingRequests() { return false; }
            virtual bool DeletePak(const AZStd::string& fileName) { return false; }
            virtual bool DeleteDownloadedContent() { return false; }
            virtual bool IsUpdated(const char* fileName) { return false; }
            using MutexType = AZStd::recursive_mutex;
            // Retrieve the list of paks which the user may request to download 
            virtual AZStd::vector<AZStd::string> GetDownloadablePaks() { return {}; }
            virtual int GetPakStatus(const char* fileName) { return 0; }
            virtual AZStd::string GetPakStatusString(const char* fileName) { return ""; }
            virtual void HandleWebCommunicatorUpdate(const AZStd::string& messageData) {}
        };

        using DynamicContentRequestBus = AZ::EBus<IDynamicContentTransferManager>;

        // Update bus to listen for data updates from the DynamicContent Gem
        class DynamicContentUpdate
            : public AZ::ComponentBus
        {
        public:
            virtual void NewContentReady(const AZStd::string& outputFile) {}
            virtual void NewPakContentReady(const AZStd::string& pakFileName) {}
            virtual void FileStatusFailed(const AZStd::string& outputFile, const AZStd::string& keyName) {}
            virtual void DownloadSucceeded(const AZStd::string& fileName, const AZStd::string& keyName, bool updated) {}

            // Download was attempted and failed
            virtual void DownloadFailed(const AZStd::string& outputFile, const AZStd::string& keyName) {}

            // Broadcast when we've detected a change in file status on the back end from the WebCommunicator (New file made public, public file made private)
            virtual void FileStatusChanged(const AZStd::string& fileName, const AZStd::string& fileStatus) {}

            virtual void DownloadReady(const AZStd::string& fileName, AZ::u64 fileSize) {}

            // Fired when when our tick bus checks and finds no remaining pak entries to mount in the UPDATING state
            virtual void RequestsCompleted() {}
            // Called when we've received data.  This is just the delta, not the running total of the data received.
            virtual void OnDataReceived(const AZStd::string& fileName, uint32_t receivedData, uint32_t totalSize) {}
        };

        class DynamicContentUpdateTraits
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            //////////////////////////////////////////////////////////////////////////
            using MutexType = AZStd::recursive_mutex;
        };

        using DynamicContentUpdateBus = AZ::EBus<DynamicContentUpdate, DynamicContentUpdateTraits>;
    }
}
