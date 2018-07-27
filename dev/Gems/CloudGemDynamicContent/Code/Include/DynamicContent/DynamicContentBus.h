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

namespace CloudCanvas
{
    namespace DynamicContent
    {

        class IDynamicContentTransferManager
            : public AZ::ComponentBus
        {
        public:

            virtual bool RequestManifest(const char* manifestName) = 0;
            virtual bool RequestFileStatus(const char* fileName, const char* outputFile) = 0;
            virtual bool ClearAllContent() = 0;
            virtual bool RemovePak(const char* fileName) = 0;
            virtual bool LoadManifest(const AZStd::string& manifestName) = 0;
            virtual bool LoadPak(const AZStd::string& fileName) = 0;
            virtual bool HasUpdatingRequests() = 0;

            virtual bool DeletePak(const AZStd::string& fileName) = 0;
            virtual bool DeleteDownloadedContent() = 0;
            using MutexType = AZStd::recursive_mutex;
            // Retrieve the list of paks which the user may request to download 
            virtual AZStd::vector<AZStd::string> GetDownloadablePaks() = 0;
            virtual int GetPakStatus(const char* fileName) = 0;
            virtual AZStd::string GetPakStatusString(const char* fileName) = 0;
        };

        using DynamicContentRequestBus = AZ::EBus<IDynamicContentTransferManager>;

        // Update bus to listen for data updates from the DynamicContent Gem
        class DynamicContentUpdate
            : public AZ::ComponentBus
        {
        public:
            virtual void NewContentReady(const AZStd::string& outputFile) {}
            virtual void NewPakContentReady(const AZStd::string& pakFileName) {}
            virtual void FileStatusFailed(const AZStd::string& outputFile) {}

            // Download was attempted and failed
            virtual void DownloadFailed(const AZStd::string& outputFile) {}

            // Broadcast when we've detected a change in file status on the back end from the WebCommunicator (New file made public, public file made private)
            virtual void FileStatusChanged(const AZStd::string& fileName, const AZStd::string& fileStatus) {}

            // Fired when when our tick bus checks and finds no remaining pak entries to mount in the UPDATING state
            virtual void RequestsCompleted() {}
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
