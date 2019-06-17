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

#include <AzCore/JSON/document.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace CloudCanvas
{
    namespace DynamicContent
    {

        class DynamicContentFileInfo
        {
        public:

            using ManifestList = AZStd::vector<AZStd::string>;
            
            DynamicContentFileInfo(const rapidjson::Value& initData);
            DynamicContentFileInfo(const AZStd::string& bucketPath, const AZStd::string& outputFile);

            ~DynamicContentFileInfo();

            enum class RequestType
            {
                MANIFEST = 0, // Top level manifest requests
                PAK = 1, // Paks requested due to other manifests - may contain non top level manifests
            };

            enum FileStatus
            {
                UNKNOWN = -100,
                SIGNATURE_FAILED = -4,
                FILE_STATUS_FAILED = -3,
                PAK_MOUNT_FAILED = -2,
                DOWNLOAD_FAILED = -1,
                UNINITIALIZED = 0,
                WAITING_FOR_USER = 1,
                INITIALIZED = 2,
                UPDATING = 3,
                READY = 4,
                MOUNTED = 5,
            };

            const AZStd::string& GetKeyName() const { return m_keyName; }
            const AZStd::string& GetManifestHash() const { return m_manifestHash; }
            const AZStd::string& GetLocalFolder() const { return m_localFolder; }
            const AZStd::string& GetBucketPrefix() const { return m_bucketPrefix; }
            const AZStd::string& GetCacheDir() const { return m_cacheDir; }
            const AZStd::string& GetLocalHash() const { return m_localHash; }
            const AZStd::string& GetBucketHash() const { return m_bucketHash; }
            const AZStd::string& GetOutputDir() const { return m_outputDir; }
            const AZStd::string& GetFullLocalFileName() const { return m_localFileName; }
            const AZStd::vector<unsigned char>& GetSignature() const { return m_signature; }

            void SetLocalHash(const char* localHash) { m_localHash = localHash; }
            void SetBucketHash(const char* bucketHash) { m_bucketHash = bucketHash; }

            void SetLocalHash(AZStd::string&& localHash);
            void SetBucketHash(AZStd::string&& bucketHash);

            RequestType GetRequestType() const { return m_requestType; }
            bool IsManifestRequest() const { return m_requestType == RequestType::MANIFEST; }

            AZStd::string GetLocalKey() const;

            FileStatus GetStatus() const { return m_status; }
            void SetStatus(FileStatus newStatus);

            bool IsMounted() const { return m_status == FileStatus::MOUNTED; }
            bool IsUpdating() const { return m_status == FileStatus::UPDATING; }

            const AZStd::string& GetManifestPath() const { return m_manifestPath; }
            void SetManifestPath(const AZStd::string& manifestPath) { m_manifestPath = manifestPath; }

            AZStd::string GetFileName() const;
            AZStd::string MakeBucketHashName() const;

            // OutputDir + FileName - eg "@user@/DynamicContent/Paks/DynamicContentTest.shared.pak"
            AZStd::string GetAliasedFilePath() const;

            void AddManifest(const AZStd::string& manifestPath);
            const ManifestList& GetManifestList() const { return m_manifestList; }

            int GetOpenRetryCount() const { return m_openCount; }
            void IncrementOpenRetryCount() { ++m_openCount; }

            void SetSignature(const AZStd::string& signatureStr);

            bool IsUserRequested() const { return m_userRequested; }

            static const char* GetStatusString(FileStatus requestStatus);
        private:
            void ResolveLocalFileName();

            int m_openCount{ 0 };

            // Raw data from manifest
            AZStd::string m_keyName;
            AZStd::string m_manifestHash;
            AZStd::string m_localFolder;
            AZStd::string m_bucketPrefix;
            AZStd::string m_cacheDir;
            AZStd::string m_localHash;
            AZStd::string m_bucketHash;
            AZStd::string m_outputDir;
            AZStd::string m_isManifest;

            // Full resolved path
            AZStd::string m_localFileName;

            // Full path to parent manifest this file came from
            AZStd::string m_manifestPath;

            // Expected digital signature
            AZStd::vector<unsigned char> m_signature;

            // List of manifests which were expected in this .pak download - The Dynamic Content system will want to try
            // to load these after the download is completed
            ManifestList m_manifestList;

            AZStd::atomic<FileStatus> m_status{ FileStatus::UNINITIALIZED };
            RequestType m_requestType{ RequestType::PAK };

            bool m_userRequested{ false };
        };
    }
}