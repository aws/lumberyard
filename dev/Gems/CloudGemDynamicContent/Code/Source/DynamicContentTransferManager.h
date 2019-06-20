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

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/mutex.h>
#include <DynamicContentFileInfo.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/TickBus.h>

#include <FileTransferSupport/FileTransferSupport.h>

#include <DynamicContent/DynamicContentBus.h>
#include <PresignedURL/PresignedURLBus.h>

#include <chrono>

namespace CloudCanvas
{
    namespace DynamicContent
    {


        class DynamicContentTransferManager : 
            public DynamicContentRequestBus::Handler, 
            public AZ::TickBus::Handler,
            public AZ::Component,
            public PresignedURLResultBus::Handler
        {
        public:

            AZ_COMPONENT(DynamicContentTransferManager, "{3B15EBE9-3F45-41BF-A94D-9C1E079D30A2}");

            using DynamicFileInfoPtr = AZStd::shared_ptr<DynamicContentFileInfo>;
            using SignatureHashVec = AZStd::vector<unsigned char>;

            DynamicContentTransferManager();
            virtual ~DynamicContentTransferManager();

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void Reflect(AZ::ReflectContext* reflection);

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            // Locally we save our requests as a spaced (illegal in bucket/keys) bucket key string
            static AZStd::string GetRequestString(const AZStd::string& bucketName, const AZStd::string& keyName);

            // DynamicContentBus handlers
            virtual bool RequestManifest(const char* manifestName) override;

            // Convenience call for a single request
            virtual bool RequestFileStatus(const char* fileName, const char* writeFile) override;

            // Clear (And unmount) all pak records from Dynamic Content
            virtual bool ClearAllContent() override;

            // Remove (and unmount) a single pak record
            virtual bool RemovePak(const char* fileName) override;

            // Attempt to load an existing manifest on disk along with all content it describes
            // This allows you to load up dynamic content which has previously been downloading without checking
            // for updates or making any calls to aws
            virtual bool LoadManifest(const AZStd::string& manifestName) override;

            // Load a specific pak into the dynamic content system
            virtual bool LoadPak(const AZStd::string& manifestName) override;

            // Are there any requests still left in the UPDATING state
            virtual bool HasUpdatingRequests() override;

            // Delete a specific pak from the dynamic content system
            virtual bool DeletePak(const AZStd::string& manifestName) override;
            // Delete Downloaded Paks in the Pak folder from disk
            virtual bool DeleteDownloadedContent() override;
            // Get the list of Paks the Dynamic Content system is aware of which the user may request
            // These are entries which were discovered in the manifest data that has been loaded labeled "userRequested"
            // Which aren't currently found on disk or in flight
            virtual AZStd::vector<AZStd::string> GetDownloadablePaks() override;
            virtual int GetPakStatus(const char* fileName) override;
            virtual AZStd::string GetPakStatusString(const char* fileName) override;
            // Handle a JSON status update string
            // Intended for messages from CloudGemWebCommunicator
            virtual void HandleWebCommunicatorUpdate(const AZStd::string& messageData) override;

            bool RequestFileStatus(const char* fileName, const char* writeFile, bool manifestRequest);
            bool RequestFileStatus(FileTransferSupport::FileRequestMap& requestVec, bool manifestRequest);

            virtual void GotPresignedURLResult(const AZStd::string& fileRequest, int responseCode, const AZStd::string& resultString, const AZStd::string& outputFile) override;

            void OnDownloadSuccess(DynamicFileInfoPtr requestPtr);
            void OnDownloadFailure(DynamicFileInfoPtr requestPtr);

            DynamicFileInfoPtr GetLocalEntryFromBucketKey(const char* bucketKey);

            void SetFileInfo(DynamicFileInfoPtr fileInfo);
            DynamicFileInfoPtr GetFileInfo(const char* localFileName) const;

            void ManifestUpdated(const AZStd::string& manifestPath, const AZStd::string& bucketName);
           
            static AZStd::string GetDefaultWriteFolderAlias();
            static AZStd::string GetUserManifestFolder();
            static AZStd::string GetUserPakFolder();
            static AZStd::string GetBasePakFolder();
            static AZStd::string GetContentRequestFunction();
            static bool IsManifestPak(const AZStd::string& fileRequest);
            static AZStd::string GetPakNameForManifest(const AZStd::string& fileRequest);
            static AZStd::string GetManifestNameForPak(const AZStd::string& fileRequest);
            static SignatureHashVec GetMD5Hash(const AZStd::string& filePath);
            static size_t GetDecodedSize(const AZStd::string& base64String);

        protected:
            //////////////////////////////////////////////////////////////////////////
            // TickBus
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            //////////////////////////////////////////////////////////////////////////

        private:
            DynamicContentTransferManager(const DynamicContentTransferManager&) = delete;
            bool ShouldLoadManifestEntry(const rapidjson::Value& thisFileEntry) const;
            void ParseManifestFileList(const rapidjson::Value& docFileList, const AZStd::string& manifestPath);
            void UpdateManifest(const AZStd::string& manifestName, const AZStd::string& outputFile);
            bool LoadManifestData(const AZStd::string& manifestPath);
            int CheckFileList(const char* bucketName);
            bool IsManifestEntry(const rapidjson::Value& thisFileEntry) const;
            bool IsPakPending(const rapidjson::Value& thisFileEntry) const;
            AZStd::string GetPakPath(const rapidjson::Value& thisFileEntry) const;
            DynamicFileInfoPtr GetPakEntry(const rapidjson::Value& thisFileEntry) const;
            // To reduce the number of lookups this condenses the above 3 calls into one call - If this entry is from a pak which is currently pending, return the pak's
            // entry
            DynamicContentTransferManager::DynamicFileInfoPtr GetPendingPakEntry(const rapidjson::Value& thisFileEntry) const;

            // Hold onto the request while it's out
            void AddPresignedURLRequest(const AZStd::string& requestURL, DynamicFileInfoPtr fileInfo);

            // We only need to hold onto this as long as the request is out - when it's back just clear and hand us back the record in one step
            DynamicFileInfoPtr GetAndRemovePresignedRequest(const AZStd::string& requestURL);

            bool CanRequestFile(DynamicFileInfoPtr) const;

            void AppendPakInfo(const rapidjson::Value& docFileList);

            void AddPendingPak(DynamicFileInfoPtr pakInfo);
            void RemovePendingPak(DynamicFileInfoPtr pakInfo);
            void UpdatePakFilesToMount();
            void SetPakReady(DynamicFileInfoPtr pakInfo);

            void NewContentReady(DynamicFileInfoPtr pakInfo);
            void CheckPendingManifests(DynamicFileInfoPtr fileInfo);
            void OnFileStatusFailed(DynamicFileInfoPtr pakInfo);
            void RequestsCompleted();

            // Performed on the TickBus update or called when attempting to immediately update
            void CheckUpdates();

            // Check if this is a user requested file, if so:
            // If the file is already local, performs a state updated to Initialized and returns false
            // If the file is not local, returns true
            // If not userRequested returns false
            bool WaitsForUserRequest(DynamicFileInfoPtr pakInfo);

            AZStd::string GetDefaultPublicKeyPath() const;
            int ValidateSignature(DynamicFileInfoPtr pakInfo) const;
            int ValidateSignatureOpenSSL(const AZStd::string& checkString, const AZStd::vector<unsigned char>& signatureBuf) const;
            int ValidateEncodedSignature(const AZStd::string& checkString, const AZStd::string& signatureString) const;

            mutable AZStd::mutex m_completedDownloadMutex;

            mutable AZStd::mutex m_fileListMutex;
            mutable AZStd::mutex m_pakFileMountMutex;
            AZStd::mutex m_presignedURLMutex;

            AZStd::unordered_map<AZStd::string, DynamicFileInfoPtr> m_fileList;
            AZStd::deque<DynamicFileInfoPtr> m_pakFilesToMount;
            // For some cases - particularly our upload/list routine - we want to hash by the data we expect in our bucket
            // to more quickly process our listObjects return
            AZStd::unordered_map<AZStd::string, DynamicFileInfoPtr> m_bucketKeyToFileInfo;
            AZStd::unordered_map<AZStd::string, DynamicFileInfoPtr> m_presignedURLToFileInfo;

            static AZ::EntityId m_moduleEntity;
        };
    }
}