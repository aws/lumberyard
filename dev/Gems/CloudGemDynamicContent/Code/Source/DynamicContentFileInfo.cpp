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

#include "CloudGemDynamicContent_precompiled.h"

#include <DynamicContentFileInfo.h>

#include <FileTransferSupport/FileTransferSupport.h>
#include <DynamicContentTransferManager.h>

#include <AzCore/std/string/string.h>

#include <Base64.h>

namespace CloudCanvas
{
    namespace DynamicContent
    {
        DynamicContentFileInfo::DynamicContentFileInfo(const rapidjson::Value& initData)
        {
            SetStatus(FileStatus::INITIALIZED);

            auto valueIter = initData.FindMember("keyName");

            if (valueIter != initData.MemberEnd() && valueIter->value.IsString())
            {
                m_keyName = valueIter->value.GetString();
            }

            valueIter = initData.FindMember("hash");

            if (valueIter != initData.MemberEnd() && valueIter->value.IsString())
            {
                m_manifestHash += valueIter->value.GetString();
            }

            valueIter = initData.FindMember("cacheRoot");

            if (valueIter != initData.MemberEnd() && valueIter->value.IsString())
            {
                m_cacheDir = valueIter->value.GetString();
                FileTransferSupport::MakeEndInSlash(m_cacheDir);
            }

            valueIter = initData.FindMember("outputRoot");

            if (valueIter != initData.MemberEnd() && valueIter->value.IsString())
            {
                m_outputDir = valueIter->value.GetString();
                FileTransferSupport::MakeEndInSlash(m_outputDir);
            }

            valueIter = initData.FindMember("localFolder");

            if (valueIter != initData.MemberEnd() && valueIter->value.IsString())
            {
                m_localFolder = valueIter->value.GetString();
                // Append the localFolder to the cache root
                m_cacheDir += m_localFolder;
                FileTransferSupport::MakeEndInSlash(m_cacheDir);

            }

            valueIter = initData.FindMember("bucketPrefix");

            if (valueIter != initData.MemberEnd() && valueIter->value.IsString())
            {
                m_bucketPrefix = valueIter->value.GetString();
                // Make sure our bucket prefix merges with our file correctly (both static-data someFile.csv and static-data/ someFile.csv end up at static-data/someFile.csv)
                FileTransferSupport::MakeEndInSlash(m_bucketPrefix);
            }

            valueIter = initData.FindMember("userRequested");

            if (valueIter != initData.MemberEnd() && valueIter->value.IsBool())
            {
                m_userRequested = valueIter->value.GetBool();

                if (m_userRequested)
                {
                    SetStatus(FileStatus::WAITING_FOR_USER);
                }
            }

            ResolveLocalFileName();
        }

        // The default construtor for a top level manifest request - we don't have JSON manifest data available from another manifest
        // to describe the request
        DynamicContentFileInfo::DynamicContentFileInfo(const AZStd::string& bucketPath, const AZStd::string& outputFile) :
            m_requestType(RequestType::MANIFEST)
        {
            SetStatus(FileStatus::INITIALIZED);

            auto pathPtr = bucketPath.find_last_of('/');
            if (pathPtr != AZStd::string::npos)
            {
                m_keyName = bucketPath.substr(pathPtr + 1);
                m_bucketPrefix = bucketPath.substr(0, pathPtr + 1);
            }
            else
            {
                m_keyName = bucketPath;
            }

            m_outputDir = DynamicContentTransferManager::GetDefaultWriteFolderAlias();

            pathPtr = outputFile.find_last_of('/');
            if (pathPtr != AZStd::string::npos)
            {
                m_keyName = outputFile.substr(pathPtr + 1);
                m_localFolder = outputFile.substr(0, pathPtr + 1);
            }
            else
            {
                m_keyName = outputFile;
            }

            ResolveLocalFileName();
        }

        DynamicContentFileInfo::~DynamicContentFileInfo()
        {
            if ( IsMounted())
            {
                AZ_TracePrintf("CloudCanvas", "Unmounting pak for %s.", GetFileName().c_str());
                gEnv->pCryPak->ClosePack(GetAliasedFilePath().c_str());
            }
            else
            {
                AZ_TracePrintf("CloudCanvas", "Not unmounting %s IsMounted is %d", GetFileName().c_str(), static_cast<int>(IsMounted()));
            }
        }

        AZStd::string DynamicContentFileInfo::GetFileName() const
        {
            AZStd::string returnString = m_localFolder;

            FileTransferSupport::MakeEndInSlash(returnString);
            returnString += m_keyName;
            return returnString;
        }

        AZStd::string DynamicContentFileInfo::GetAliasedFilePath() const
        {
            AZStd::string returnString = m_outputDir;

            FileTransferSupport::MakeEndInSlash(returnString);
            returnString += GetFileName();
            return returnString;
        }

        void DynamicContentFileInfo::ResolveLocalFileName() 
        {
            m_localFileName = FileTransferSupport::ResolvePath(m_outputDir.c_str());
            FileTransferSupport::MakeEndInSlash(m_localFileName);
            m_localFileName += GetFileName();
        }

        AZStd::string DynamicContentFileInfo::MakeBucketHashName() const
        {
            AZStd::string returnString{ m_bucketPrefix };
            FileTransferSupport::MakeEndInSlash(returnString);
            returnString += m_keyName;
            return returnString;
        }

        void DynamicContentFileInfo::SetLocalHash(AZStd::string&& localHash)
        {
            m_localHash = AZStd::move(localHash);
        }

        void DynamicContentFileInfo::SetBucketHash(AZStd::string&& bucketHash)
        {
            m_bucketHash = AZStd::move(bucketHash);
        }

        void DynamicContentFileInfo::AddManifest(const AZStd::string& manifestPath)
        {
            m_manifestList.push_back(manifestPath);
        }

        // Base64 encoded signature passed in - convert and store as true signature
        void DynamicContentFileInfo::SetSignature(const AZStd::string& signatureString)
        {
            m_signature.resize(DynamicContentTransferManager::GetDecodedSize(signatureString));
            if (m_signature.size())
            {
                Base64::decode_base64(reinterpret_cast<char*>(&m_signature[0]), signatureString.c_str(), signatureString.length(), false);
            }
        }

        const char* DynamicContentFileInfo::GetStatusString(FileStatus requestStatus)
        {
            switch (requestStatus)
            {
            case FileStatus::SIGNATURE_FAILED:
                return "SIGNATURE_FAILED";
            case FileStatus::FILE_STATUS_FAILED:
                return "FILE_STATUS_FAILED";
            case FileStatus::PAK_MOUNT_FAILED:
                return "MOUNT_FAILED";
            case FileStatus::DOWNLOAD_FAILED:
                return "DOWNLOAD_FAILED";
            case FileStatus::UNINITIALIZED:
                return "UNINITIALIZED";
            case FileStatus::WAITING_FOR_USER:
                return "WAITING_FOR_USER";
            case FileStatus::INITIALIZED:
                return "INITIALIZED";
            case FileStatus::UPDATING:
                return "UPDATING";
            case FileStatus::READY:
                return "READY";
            case FileStatus::MOUNTED:
                return "MOUNTED";
            case FileStatus::UNKNOWN:
                return "UNKNOWN";
            }
            return "UNKNOWN";
        }

        void DynamicContentFileInfo::SetStatus(FileStatus newStatus)
        {
            if (newStatus != m_status)
            {
                AZ_TracePrintf("CloudCanvas", "Setting entry %s to status %s (was %s)", GetFileName().c_str(), GetStatusString(newStatus), GetStatusString(m_status));
                m_status = newStatus; 
            }
        }
    }
}