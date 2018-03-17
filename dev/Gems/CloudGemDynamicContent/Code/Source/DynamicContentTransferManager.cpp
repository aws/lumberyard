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
#include <DynamicContentTransferManager.h>
#include <DynamicContent/DynamicContentBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Module/ModuleManager.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/IO/SystemFile.h>

#include <PakVars.h>

#include <AzCore/std/parallel/lock.h>

#include <PresignedURL/PresignedURLBus.h>

#include <LmbrAWS/ILmbrSDKSupport.h>

#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
#include <AWS/ServiceAPI/CloudGemDynamicContentClientComponent.h>
using namespace ::CloudGemDynamicContent::ServiceAPI;
#endif

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#if defined(OPENSSL_ENABLED)
#include <openssl/pem.h>
#include <openssl/sha.h>
#endif
#include <Base64.h>
static const int fileRequestRetryMax = 10000; // Let's not request the same file again too often
static const int fileOpenRetryMax = 10; // How many times (once per second) will we attempt to open a file before assuming it failed

static const bool requireSignatures = true;

static const char* baseManifestFile = "default.json";
static const char* baseManifestFolder = "DynamicContent/Manifests/";
static const char* basePakFolder = "DynamicContent/Paks/";
static const char* certificateFolder = "DynamicContent/Certificates/";
static const char* manifestPakExtension = ".manifest.pak";
static const char* manifestExtension = ".json";
static const char* publicCertName = "DynamicContent.pub.pem";
static const char* allPlatformsName = "shared";


namespace CloudCanvas
{
    namespace DynamicContent
    {
        AZ::EntityId DynamicContentTransferManager::m_moduleEntity;

        DynamicContentTransferManager::DynamicContentTransferManager() 
        {

        }

        DynamicContentTransferManager::~DynamicContentTransferManager()
        {

        }

        // Update Bus
        class BehaviorDynamicContentComponentNotificationBusHandler : public DynamicContentUpdateBus::Handler, public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(BehaviorDynamicContentComponentNotificationBusHandler, "{9CE84D29-3DEF-4021-B899-6D971E39F75D}", AZ::SystemAllocator,
                NewContentReady, NewPakContentReady, RequestsCompleted);

            void NewContentReady(const AZStd::string& outputFile) override
            {
                Call(FN_NewContentReady, outputFile);
            }

            void NewPakContentReady(const AZStd::string& outputFile) override
            {
                Call(FN_NewPakContentReady, outputFile);
            }            
            
            void RequestsCompleted() override
            {
                Call(FN_RequestsCompleted);
            }
        };

        //
        // Component Implementations
        //
        void DynamicContentTransferManager::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("DynamicContent"));
        }

        void DynamicContentTransferManager::Init()
        {

        }

        void DynamicContentTransferManager::Activate()
        {
            if (azrtti_istypeof<AZ::ModuleEntity>(GetEntity()))
            {
                m_moduleEntity = GetEntityId();
            }
            DynamicContentRequestBus::Handler::BusConnect(m_entity->GetId());
            CloudCanvas::PresignedURLResultBus::Handler::BusConnect(m_entity->GetId());
        }

        void DynamicContentTransferManager::Deactivate()
        {
            CloudCanvas::PresignedURLResultBus::Handler::BusDisconnect();
            DynamicContentRequestBus::Handler::BusDisconnect();
            if (azrtti_istypeof<AZ::ModuleEntity>(GetEntity()))
            {
                m_moduleEntity.SetInvalid();
            }
        }

        void DynamicContentTransferManager::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<DynamicContentTransferManager>()
                    ->Version(3);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<DynamicContentTransferManager>("DynamicContent", "CloudCanvas Dynamic Content Component")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Cloud Gem Framework")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/DynamicContent.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/DynamicContent.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"));
                }
            }

            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<DynamicContentRequestBus>("DynamicContentRequestBus")
                    ->Event("RequestManifest", &DynamicContentRequestBus::Events::RequestManifest)
                    ->Event("RequestFileStatus", &DynamicContentRequestBus::Events::RequestFileStatus)
                    ->Event("ClearAllContent", &DynamicContentRequestBus::Events::ClearAllContent)
                    ->Event("RemovePak", &DynamicContentRequestBus::Events::RemovePak)
                    ->Event("LoadManifest", &DynamicContentRequestBus::Events::LoadManifest)
                    ->Event("LoadPak", &DynamicContentRequestBus::Events::LoadPak)
                    ->Event("HasUpdatingRequests", &DynamicContentRequestBus::Events::HasUpdatingRequests)
                    ->Event("DeleteDownloadedContent", &DynamicContentRequestBus::Events::DeleteDownloadedContent)
                    ->Event("GetDownloadablePaks", &DynamicContentRequestBus::Events::GetDownloadablePaks)
                    ->Event("DeletePak", &DynamicContentRequestBus::Events::DeletePak)
                    ->Event("GetPakStatus", &DynamicContentRequestBus::Events::GetPakStatus)
                    ->Event("GetPakStatusString", &DynamicContentRequestBus::Events::GetPakStatusString)
                    ;

                behaviorContext->Class<DynamicContentTransferManager>("DynamicContent")
                    ->Property("ModuleEntity", BehaviorValueGetter(&DynamicContentTransferManager::m_moduleEntity), nullptr);

                behaviorContext->EBus<DynamicContentUpdateBus>("DynamicContentUpdateBus")
                    ->Handler<BehaviorDynamicContentComponentNotificationBusHandler>()
                    ;
            }
        }

        // Handles the result of a presigned url request
        void DynamicContentTransferManager::GotPresignedURLResult(const AZStd::string& requestURL, int responseCode, const AZStd::string& resultString, const AZStd::string& outputFile)
        {
            DynamicFileInfoPtr requestPtr = GetAndRemovePresignedRequest(requestURL);
            if(!requestPtr)
            {
                AZ_Warning("CloudCanvas", false,"Could not find URL entry for %s!", requestURL.c_str());
                return;
            }
            if (responseCode == LmbrAWS::HttpOKResponse())
            {
                AZ_TracePrintf("CloudCanvas", "Downloaded signed URL to %s", requestPtr->GetFullLocalFileName().c_str());
                OnDownloadSuccess(requestPtr);
            }
            else
            {
                AZ_Warning("CloudCanvas", false, "Signed download failed: %d : %s", responseCode, resultString.c_str());
                OnDownloadFailure(requestPtr);
            }
        }

        void DynamicContentTransferManager::OnDownloadSuccess(DynamicFileInfoPtr requestPtr)
        {
            if (!ValidateSignature(requestPtr))
            {
                requestPtr->SetStatus(DynamicContentFileInfo::FileStatus::SIGNATURE_FAILED);
                return;
            }
            SetPakReady(requestPtr);
        }

        void DynamicContentTransferManager::SetPakReady(DynamicFileInfoPtr requestPtr)
        {
            requestPtr->SetStatus(DynamicContentFileInfo::FileStatus::READY);
        }

        void DynamicContentTransferManager::OnDownloadFailure(DynamicFileInfoPtr requestPtr)
        {
            // Top level manifests should be removed from the pending list - they can be requested again
            if (requestPtr->IsManifestRequest())
            {
                RemovePendingPak(requestPtr);
            }
            else
            {
                requestPtr->SetStatus(DynamicContentFileInfo::FileStatus::DOWNLOAD_FAILED);
            }
        }

        AZStd::string DynamicContentTransferManager::GetRequestString(const AZStd::string& bucketName, const AZStd::string& keyName) 
        {
            return (bucketName + " " + keyName);
        }

        bool DynamicContentTransferManager::RequestManifest(const char* manifestName)
        {
            UpdateManifest(manifestName ? manifestName : baseManifestFile, {});
            return true;
        }

        bool DynamicContentTransferManager::IsManifestPak(const AZStd::string& manifestName)
        {
            return manifestName.find(manifestPakExtension) != AZStd::string::npos;
        }

        // Attempt to unmount and delete a single pak
        bool DynamicContentTransferManager::DeletePak(const AZStd::string& fileName)
        {
            AZ_TracePrintf("CloudCanvas", "Deleting downloaded content.");

            AZStd::string removeFile{ basePakFolder };
            removeFile += fileName;
            // Attempt to unmount everything and release any file locks
            RemovePak(removeFile.c_str());

            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (!fileIO)
            {
                AZ_Error("CloudCanvas", false, "No FileIoBase Instance");
                return false;
            }

            AZStd::string pakFolder = GetUserPakFolder();

            // Let's make really really sure we're looking in the right spot
            if (pakFolder.find("@user@") != 0)
            {
                AZ_Warning("CloudCanvas", false, "Invalid User folder : %s", pakFolder.c_str());
                return false;
            }

            if (pakFolder.length() && (pakFolder[pakFolder.length() - 1] != '/' && pakFolder[pakFolder.length() - 1] != '\\'))
            {
                pakFolder.append("/");
            }
            pakFolder.append(fileName);

            AZ::IO::Result destroyResult = fileIO->Remove(pakFolder.c_str());
            if (destroyResult != AZ::IO::ResultCode::Success)
            {
                AZ_Warning("CloudCanvas", false,"Failed to remove  %s with return code %d", pakFolder.c_str(), static_cast<int>(destroyResult));
            }

            return destroyResult == AZ::IO::ResultCode::Success;
        }

        // Attempt to destroy all downloaded content - will attempt to unmount everything first.
        bool DynamicContentTransferManager::DeleteDownloadedContent()
        {
            AZ_TracePrintf("CloudCanvas", "Deleting downloaded content.");
            // Attempt to unmount everything and release any file locks
            ClearAllContent();

            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (!fileIO)
            {
                AZ_Error("CloudCanvas", false, "No FileIoBase Instance");
                return false;
            }

            AZStd::string pakFolder = GetUserPakFolder();

            // Let's make really really sure we're looking in the right spot
            if (pakFolder.find("@user@") != 0)
            {
                AZ_Warning("CloudCanvas", false,"Invalid User folder : %s", pakFolder.c_str());
                return false;
            }

            if(pakFolder.length() && pakFolder[pakFolder.length() - 1] == '/' || pakFolder[pakFolder.length() - 1] == '\\')
            {
                pakFolder = pakFolder.substr(0, pakFolder.length() - 1);
            }
            AZ::IO::Result destroyResult = fileIO->DestroyPath(pakFolder.c_str());
            if (destroyResult != AZ::IO::ResultCode::Success)
            {
                AZ_Warning("CloudCanvas", false,"Failed to clean up %s with return code %d", pakFolder.c_str(), static_cast<int>(destroyResult));
            }

            return destroyResult == AZ::IO::ResultCode::Success;
        }

        // Top level manifests are placed in a pak of a specified name format - default.json is placed in default.<platform>.pak
        AZStd::string DynamicContentTransferManager::GetPakNameForManifest(const AZStd::string& manifestName) 
        {
            AZStd::string replaceName{ manifestName };
            auto replacePos = replaceName.find(manifestExtension);
            if(replacePos != AZStd::string::npos)
            { 
                return replaceName.substr(0, replacePos) + manifestPakExtension;
            }
            return manifestName;
        }

        // Top level manifests are placed in a pak of a specified name format - default.json is placed in default.<platform>.pak
        // This call deals with the directory structure as well - manifests are expected to be found in the manifests folder
        AZStd::string DynamicContentTransferManager::GetManifestNameForPak(const AZStd::string& pakName)
        {
            AZStd::string replaceName{ pakName };
            std::replace(replaceName.begin(), replaceName.end(), '\\', '/');
            auto lastSeparator = replaceName.find_last_of('/');
            if (lastSeparator != AZStd::string::npos)
            {
                replaceName.erase(0, lastSeparator + 1);
            }
            replaceName = baseManifestFolder + replaceName;
            auto replacePos = replaceName.find(manifestPakExtension);
            if (replacePos != AZStd::string::npos)
            {
                return replaceName.substr(0, replacePos) + manifestExtension;
            }
            return{};
        }

        // Manifest handling
        void DynamicContentTransferManager::UpdateManifest(const AZStd::string& manifestName, const AZStd::string& outputFile)
        {
            AZStd::string pakName{ GetPakNameForManifest(manifestName) };
            AZStd::string localFileName{ outputFile };

            if (!localFileName.length())
            {
                localFileName = GetBasePakFolder() + pakName;
            }

            AZStd::string writeFile = GetUserPakFolder() + pakName;
            DynamicFileInfoPtr manifestPtr = AZStd::make_shared<DynamicContentFileInfo>(pakName, localFileName);

            if (!CanRequestFile(manifestPtr))
            {
                return;
            }

            SetFileInfo(manifestPtr);
            AddPendingPak(manifestPtr);
            manifestPtr->AddManifest(baseManifestFolder + manifestName);
            RequestFileStatus(pakName.c_str(), writeFile.c_str(), true);
        }

        bool DynamicContentTransferManager::HasUpdatingRequests()
        {
            AZStd::lock_guard<AZStd::mutex> fileLock(m_pakFileMountMutex);
            for (auto thisElement : m_pakFilesToMount)
            {
                if (thisElement->IsUpdating())
                {
                    return true;
                }
            }
            return false;
        }

        AZStd::vector<AZStd::string> DynamicContentTransferManager::GetDownloadablePaks()
        {
            AZStd::lock_guard<AZStd::mutex> fileLock(m_fileListMutex);

            AZStd::vector<AZStd::string> returnList;
            for (auto thisPair : m_fileList)
            {
                auto thisEntry = thisPair.second;
                if (thisEntry->IsUserRequested() && thisEntry->GetStatus() == DynamicContentFileInfo::FileStatus::WAITING_FOR_USER)
                {
                    returnList.push_back(thisEntry->GetKeyName());
                }
            }
            return returnList;
        }

        bool DynamicContentTransferManager::LoadPak(const AZStd::string& fileName)
        {
            AZStd::string sanitizedName{ fileName };
            std::replace(sanitizedName.begin(), sanitizedName.end(), '\\', '/');

            AZStd::string userPath{ GetUserPakFolder() + sanitizedName };

            if (!AZ::IO::FileIOBase::GetInstance()->Exists(userPath.c_str()))
            {
                return false;
            }

            // Use base here like our normal download routine - the DynamicContentFileInfo takes care of the @user@ aliasing
            AZStd::string localFileName{ GetBasePakFolder() + sanitizedName };

            auto lastSeparator = sanitizedName.find_last_of('/');
            AZStd::string pakName;
            if (lastSeparator != AZStd::string::npos && lastSeparator < (sanitizedName.length() - 1))
            {
                pakName = sanitizedName.substr(lastSeparator + 1, AZStd::string::npos);
            }
            else
            {
                pakName = sanitizedName;
            }
            DynamicFileInfoPtr manifestPtr = AZStd::make_shared<DynamicContentFileInfo>(pakName, localFileName);
            SetFileInfo(manifestPtr);
            AddPendingPak(manifestPtr);
            SetPakReady(manifestPtr);
            return true;
        }

        // Load a manifest which has already been downloaded to the user - this is not necessary when calling RequestManifest
        // which first downloads a top level manifest and then goes through the load routine.
        // This will only result in requests to AWS if the loaded manifest finds missing content locally
        bool DynamicContentTransferManager::LoadManifest(const AZStd::string& manifestName)
        {
            AZStd::string pakName{ GetPakNameForManifest(manifestName) };
            AZStd::string userPath{ GetUserPakFolder() + pakName };

            if (!AZ::IO::FileIOBase::GetInstance()->Exists(userPath.c_str()))
            {
                return false;
            }

            AZStd::string localName{ GetBasePakFolder() + pakName };

            DynamicFileInfoPtr manifestPtr = AZStd::make_shared<DynamicContentFileInfo>(pakName, localName);

            SetFileInfo(manifestPtr);
            AddPendingPak(manifestPtr);
            manifestPtr->AddManifest(baseManifestFolder + manifestName);
            SetPakReady(manifestPtr);
            CheckUpdates();
            return true;
        }

        bool DynamicContentTransferManager::LoadManifestData(const AZStd::string& manifestPath)
        {
            if (!gEnv->pCryPak)
            {
                return false;
            }

            AZ::IO::HandleType pFile = gEnv->pCryPak->FOpen(manifestPath.c_str(), "rt");
            if (pFile == AZ::IO::InvalidHandle)
            {
                return false;
            }

            size_t fileSize = gEnv->pCryPak->FGetSize(pFile);
            if (fileSize > 0)
            {
                AZStd::string fileBuf;
                fileBuf.resize(fileSize);

                size_t read = gEnv->pCryPak->FRead(fileBuf.data(), fileSize, pFile);
                gEnv->pCryPak->FClose(pFile);

                rapidjson::Document parseDoc;
                parseDoc.Parse<rapidjson::kParseStopWhenDoneFlag>(fileBuf.data());

                if (parseDoc.HasParseError())
                {
                    return false;
                }

                const rapidjson::Value& pakFileList = parseDoc["Paks"];
                ParseManifestFileList(pakFileList,  manifestPath);

                const rapidjson::Value& docFileList = parseDoc["Files"];
                AppendPakInfo(docFileList);

                return true;
            }
            gEnv->pCryPak->FClose(pFile);
            return false;
        }

        bool DynamicContentTransferManager::IsManifestEntry(const rapidjson::Value& initData) const
        {
            auto valueIter = initData.FindMember("isManifest");

            if (valueIter != initData.MemberEnd() && valueIter->value.IsBool())
            {
                return valueIter->value.GetBool();
            }
            return false;
        }

        bool DynamicContentTransferManager::ShouldLoadManifestEntry(const rapidjson::Value& initData) const
        {
            auto valueIter = initData.FindMember("platformType");

            if (valueIter != initData.MemberEnd() && valueIter->value.IsString())
            {
                AZStd::string thisEntry{ valueIter->value.GetString() };

                if (!thisEntry.length() || thisEntry == allPlatformsName)
                {
                    // No platform specified, all platforms
                    return true;
                }

                AZStd::string myPlatform{ GetISystem()->GetAssetsPlatform() };

                return thisEntry == myPlatform;
            }
            return true;
        }

        bool DynamicContentTransferManager::CanRequestFile(DynamicFileInfoPtr newFile) const
        {
            AZStd::lock_guard<AZStd::mutex> fileLock(m_fileListMutex);
            auto oldIter = m_fileList.find(newFile->GetFileName());
            if (oldIter == m_fileList.end())
            {
                return true;
            }
            DynamicFileInfoPtr oldEntry = oldIter->second;

            if (oldEntry)
            {
                if (oldEntry->GetStatus() == DynamicContentFileInfo::FileStatus::UPDATING)
                {
                    AZ_TracePrintf("CloudCanvas", "Attempting to request a file (%s) which is currently being processed (Status %s) - rejecting", newFile->GetFileName().c_str(), DynamicContentFileInfo::GetStatusString(oldEntry->GetStatus()));
                    return false;
                }
                if (oldEntry->IsManifestRequest())
                {
                    // Top level manifests may be requested - we can't know if the file has been updated
                    AZ_TracePrintf("CloudCanvas", "Requesting a new version of top level manifest");
                    return true;
                }
                if (oldEntry->GetManifestHash() == newFile->GetManifestHash() && oldEntry->IsMounted())
                {
                    AZ_TracePrintf("CloudCanvas", "Attempting to request a matching file (%s - Status %s) - rejecting", newFile->GetFileName().c_str(), DynamicContentFileInfo::GetStatusString(oldEntry->GetStatus()));
                    return false;
                }
            }
            return true;
        }

        void DynamicContentTransferManager::SetFileInfo(DynamicFileInfoPtr thisEntry)
        {
            AZStd::lock_guard<AZStd::mutex> fileLock(m_fileListMutex);
            AZ_TracePrintf("CloudCanvas", "Adding dynamic file entry for %s (%s)", thisEntry->GetFileName().c_str(),thisEntry->GetFullLocalFileName().c_str());
            DynamicFileInfoPtr oldEntry = m_fileList[thisEntry->GetFileName()];
            m_fileList[thisEntry->GetFileName()] = thisEntry;
            m_bucketKeyToFileInfo[thisEntry->MakeBucketHashName()] = thisEntry;
        }

        DynamicContentTransferManager::DynamicFileInfoPtr DynamicContentTransferManager::GetFileInfo(const char* fileName) const
        {
            AZStd::lock_guard<AZStd::mutex> fileLock(m_fileListMutex);
            auto thisEntry = m_fileList.find(AZStd::string{ fileName });
            if (thisEntry == m_fileList.end())
            {
                AZ_TracePrintf("CloudCanvas", "Failed to find dynamic file entry for %s", fileName);
                return nullptr;
            }
            return thisEntry->second;
        }

        // This goes to lua, we convert to int
        int DynamicContentTransferManager::GetPakStatus(const char* fileName) 
        {
            AZStd::string pakName{ basePakFolder };
            pakName += fileName;
            DynamicFileInfoPtr fileInfo = GetFileInfo(pakName.c_str());
            if (!fileInfo)
            {
                return static_cast<int>(DynamicContentFileInfo::FileStatus::UNKNOWN);
            }

            return static_cast<int>(fileInfo->GetStatus());
        }

        AZStd::string DynamicContentTransferManager::GetPakStatusString(const char* fileName)
        {
            AZStd::string pakName{ basePakFolder };
            pakName += fileName;
            DynamicFileInfoPtr fileInfo = GetFileInfo(pakName.c_str());

            return DynamicContentFileInfo::GetStatusString(fileInfo ? fileInfo->GetStatus() : DynamicContentFileInfo::FileStatus::UNKNOWN);
        }

        DynamicContentTransferManager::DynamicFileInfoPtr DynamicContentTransferManager::GetPendingPakEntry(const rapidjson::Value& thisFileEntry) const
        {
            auto valueIter = thisFileEntry.FindMember("pakFile");

            if (valueIter != thisFileEntry.MemberEnd() && valueIter->value.IsString())
            {
                AZStd::string thisEntry{ valueIter->value.GetString() };

                DynamicFileInfoPtr pakEntry = GetFileInfo(thisEntry.c_str());
                if (pakEntry)
                {
                    if (AZStd::find(m_pakFilesToMount.begin(), m_pakFilesToMount.end(), pakEntry) != m_pakFilesToMount.end())
                    {
                        return pakEntry;
                    }
                }
            }
            return nullptr;
        }

        AZStd::string DynamicContentTransferManager::GetPakPath(const rapidjson::Value& thisFileEntry) const
        {
            auto valueIter = thisFileEntry.FindMember("pakFile");

            if (valueIter != thisFileEntry.MemberEnd() && valueIter->value.IsString())
            {
                AZStd::string thisEntry{ valueIter->value.GetString() };

                DynamicFileInfoPtr pakEntry = GetFileInfo(thisEntry.c_str());
                if (pakEntry)
                {
                    return pakEntry->GetFileName();
                }
            }
            return{};
        }

        DynamicContentTransferManager::DynamicFileInfoPtr  DynamicContentTransferManager::GetPakEntry(const rapidjson::Value& thisFileEntry) const
        {
            return GetFileInfo(GetPakPath(thisFileEntry).c_str());
        }

        bool DynamicContentTransferManager::IsPakPending(const rapidjson::Value& thisFileEntry) const
        {
            AZStd::lock_guard<AZStd::mutex> pakMutex(m_pakFileMountMutex);
            DynamicFileInfoPtr pakEntry = GetPakEntry(thisFileEntry);
            if (pakEntry)
            {
                return AZStd::find(m_pakFilesToMount.begin(), m_pakFilesToMount.end(), pakEntry) != m_pakFilesToMount.end();
            }
            return false;
        }

        void DynamicContentTransferManager::ParseManifestFileList(const rapidjson::Value& docFileList, const AZStd::string& manifestPath)
        {
            if (!docFileList.IsArray())
            {
                return;
            }

            for (rapidjson::SizeType fileCount = 0; fileCount < docFileList.Size(); ++fileCount)
            {
                const rapidjson::Value& thisFileEntry = docFileList[fileCount];

                if (!ShouldLoadManifestEntry(thisFileEntry))
                {
                    continue;
                }

                DynamicFileInfoPtr thisFileInfo = AZStd::make_shared<DynamicContentFileInfo>(thisFileEntry);
                thisFileInfo->SetManifestPath(manifestPath);

                if (!CanRequestFile(thisFileInfo))
                {
                    continue;
                }

                SetFileInfo(thisFileInfo);

                // User Request files are not automatically downloaded, but if they've already been downloaded they should be loaded automatically like any other entry
                if(WaitsForUserRequest(thisFileInfo))
                {
                    continue;
                }

                AddPendingPak(thisFileInfo);
            }
        }

        // Check if this is a user requested file, if so:
        // If the file is already local, performs a state updated to Initialized and returns false
        // If the file is not local, returns true
        // If not userRequested returns false
        bool DynamicContentTransferManager::WaitsForUserRequest(DynamicFileInfoPtr thisFileInfo)
        {
            if (thisFileInfo->IsUserRequested())
            {
                AZStd::string userPath{ GetUserPakFolder() + thisFileInfo->GetKeyName() };

                if (!AZ::IO::FileIOBase::GetInstance()->Exists(userPath.c_str()))
                {
                    return true;
                }
                thisFileInfo->SetStatus(DynamicContentFileInfo::FileStatus::INITIALIZED);
            }
            return false;
        }

        void DynamicContentTransferManager::AddPendingPak(DynamicFileInfoPtr thisCompletion)
        {
            AZStd::lock_guard<AZStd::mutex> pakMutex(m_pakFileMountMutex);
            m_pakFilesToMount.push_back(thisCompletion);
            AZ::TickBus::Handler::BusConnect();
        }

        void DynamicContentTransferManager::RemovePendingPak(DynamicFileInfoPtr thisPakInfo)
        {
            AZStd::lock_guard<AZStd::mutex> pakMutex(m_pakFileMountMutex);
            auto pakFileToMountIter = AZStd::find(m_pakFilesToMount.begin(), m_pakFilesToMount.end(), thisPakInfo);
            if (pakFileToMountIter != m_pakFilesToMount.end())
            {
                m_pakFilesToMount.erase(pakFileToMountIter);
            }
        }

        // We've queued up .paks we're going to be downloading - Let's take a single pass through the Manifest to note any manifests we expect to arrive
        // within those paks or any other data we don't want to have to dig up later
        void DynamicContentTransferManager::AppendPakInfo(const rapidjson::Value& docFileList)
        {
            if (!docFileList.IsArray())
            {
                return;
            }

            for (rapidjson::SizeType fileCount = 0; fileCount < docFileList.Size(); ++fileCount)
            {
                const rapidjson::Value& thisFileEntry = docFileList[fileCount];

                if (!ShouldLoadManifestEntry(thisFileEntry))
                {
                    continue;
                }
                if (!IsManifestEntry(thisFileEntry))
                {
                    continue;
                }
                if (!IsPakPending(thisFileEntry))
                {
                    continue;
                }
                DynamicFileInfoPtr thisFileInfo = AZStd::make_shared<DynamicContentFileInfo>(thisFileEntry);
                DynamicFileInfoPtr pakFileInfo = GetPendingPakEntry(thisFileEntry);
                if (!pakFileInfo)
                {
                    continue;
                }
                pakFileInfo->AddManifest(thisFileInfo->GetFileName());
            }
        }

        int DynamicContentTransferManager::CheckFileList(const char* bucketName)
        {
            AZStd::lock_guard<AZStd::mutex> fileLock(m_fileListMutex);
            FileTransferSupport::FileRequestMap requestMap;
            for (auto thisEntry : m_fileList)
            {
                DynamicFileInfoPtr filePtr = thisEntry.second;
                if (filePtr->GetStatus() != DynamicContentFileInfo::FileStatus::INITIALIZED)
                {
                    // Already processed
                    continue;
                }

                AZStd::string localStr(filePtr->GetFullLocalFileName());
                AZStd::string fileKey(filePtr->GetKeyName());

                const bool cDirectFileAcces = true; // We want to use the AZ::Io DirectInstance system to work properly on clients running in release
                filePtr->SetLocalHash(FileTransferSupport::CalculateMD5(localStr.c_str(), cDirectFileAcces));

                if (!filePtr->GetLocalHash().size() || filePtr->GetManifestHash() != filePtr->GetLocalHash())
                {
                    AZStd::string bucketKey = filePtr->GetBucketPrefix() + fileKey;
                    requestMap[bucketKey] = localStr;
                }
                else 
                {
                    filePtr->SetStatus(DynamicContentFileInfo::FileStatus::READY);
                }
            }
            if (requestMap.size())
            {
                RequestFileStatus(requestMap, false);
            }
            return requestMap.size();
        }

        AZStd::string DynamicContentTransferManager::GetDefaultWriteFolderAlias() 
        {
            return "@user@/";
        }

        AZStd::string DynamicContentTransferManager::GetBasePakFolder()
        {
            return basePakFolder;
        }

        AZStd::string DynamicContentTransferManager::GetUserPakFolder()  
        {
            return GetDefaultWriteFolderAlias() + GetBasePakFolder();
        }

        AZStd::string DynamicContentTransferManager::GetUserManifestFolder()
        {
            return GetDefaultWriteFolderAlias() + baseManifestFolder;
        }

        AZStd::string DynamicContentTransferManager::GetContentRequestFunction()
        {
            return "DynamicContent.ContentRequest";
        }

        AZStd::shared_ptr<DynamicContentFileInfo> DynamicContentTransferManager::GetLocalEntryFromBucketKey(const char* bucketKey)
        {
            auto bucketIter = m_bucketKeyToFileInfo.find(AZStd::string{ bucketKey });
            if (bucketIter != m_bucketKeyToFileInfo.end())
            {
                return bucketIter->second;
            }
            return{};
        }

        void DynamicContentTransferManager::ManifestUpdated(const AZStd::string& manifestPath, const AZStd::string& bucketName)
        {
            LoadManifestData(manifestPath.c_str());
            CheckFileList(bucketName.c_str());

            // It's possible that some or all pak files have already been downloaded
            UpdatePakFilesToMount();
        }

        void DynamicContentTransferManager::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            CheckUpdates();

            if (!HasUpdatingRequests())
            {
                RequestsCompleted();
            }
        }

        void DynamicContentTransferManager::CheckUpdates()
        {
            UpdatePakFilesToMount();
        }

        void DynamicContentTransferManager::CheckPendingManifests(DynamicFileInfoPtr fileInfo)
        {
            if (fileInfo == nullptr)
            {
                return;
            }
            for (auto thisManifest : fileInfo->GetManifestList())
            {
                ManifestUpdated(thisManifest.c_str(), {});
            }
        }

        void DynamicContentTransferManager::NewContentReady(DynamicFileInfoPtr fileInfo)
        {
            EBUS_EVENT(CloudCanvas::DynamicContent::DynamicContentUpdateBus, NewContentReady, fileInfo->GetFullLocalFileName());
        }

        void DynamicContentTransferManager::RequestsCompleted()
        {
            AZ::TickBus::Handler::BusDisconnect();
            EBUS_EVENT(CloudCanvas::DynamicContent::DynamicContentUpdateBus, RequestsCompleted);
        }

        void DynamicContentTransferManager::UpdatePakFilesToMount()
        {
            AZStd::vector<DynamicFileInfoPtr> paksMounted;
            DynamicFileInfoPtr failurePak; // A single pak will stop the mount process, don't need to keep trying after one fails
            {
                AZStd::lock_guard<AZStd::mutex> pakMutex(m_pakFileMountMutex);
                while (m_pakFilesToMount.size() &&
                    m_pakFilesToMount.front()->GetStatus() == DynamicContentFileInfo::FileStatus::READY)
                {
                    if (gEnv->pCryPak->GetPakPriority() == ePakPriorityFileFirst)
                    {
                        AZ_TracePrintf("CloudCanvas", "Warning - Dynamic Content downloaded but Pak priority is currently set to FileFirst.  Use the console command sys_PakPriority=1 if you wish to switch to prefer paks.");
                    }
                    const auto& pakFileToMount = m_pakFilesToMount.front();
                    const AZStd::string downloadedPath{ pakFileToMount->GetAliasedFilePath() };
                    const bool pakMounted = gEnv->pCryPak->OpenPack("@assets@", downloadedPath.c_str(), ICryPak::FLAGS_NO_LOWCASE);
                    if (!pakMounted)
                    {
                        pakFileToMount->IncrementOpenRetryCount();
                        if (pakFileToMount->GetOpenRetryCount() >= fileOpenRetryMax)
                        {
                            failurePak = pakFileToMount;
                        }
                        break;
                    }

                    pakFileToMount->SetStatus(DynamicContentFileInfo::FileStatus::MOUNTED);
                    paksMounted.push_back(pakFileToMount);
                    m_pakFilesToMount.pop_front();
                    EBUS_EVENT(CloudCanvas::DynamicContent::DynamicContentUpdateBus, NewPakContentReady, pakFileToMount->GetKeyName());
                }
            }

            // Handle this outside the loop because RemovePendingPak uses our m_pakFileMountMutex
            if (failurePak)
            {
                AZ_Warning("CloudCanvas", "Attempted to open %s %d times without success - removing", failurePak->GetAliasedFilePath().c_str(), failurePak->GetOpenRetryCount());
                RemovePendingPak(failurePak);
                failurePak->SetStatus(DynamicContentFileInfo::FileStatus::PAK_MOUNT_FAILED);
            }

            for (auto thisPakEntry : paksMounted)
            {
                CheckPendingManifests(thisPakEntry);
            }
        }

        bool DynamicContentTransferManager::RequestFileStatus(const char* fileName, const char* outputFile)
        {
            return RequestFileStatus(fileName, outputFile, false);
        }

        bool DynamicContentTransferManager::RequestFileStatus(const char* fileName, const char* outputFile, bool manifestRequest)
        {
            FileTransferSupport::FileRequestMap requestMap;
            AZStd::string writeFile{ outputFile };
            if (!writeFile.length())
            {
                writeFile = GetUserPakFolder() + fileName;
            }
            requestMap[AZStd::string(fileName)] = FileTransferSupport::ResolvePath(writeFile.c_str(), true);

            return RequestFileStatus(requestMap, manifestRequest);
        }

        void DynamicContentTransferManager::OnFileStatusFailed(DynamicFileInfoPtr requestPtr)
        {
            requestPtr->SetStatus(DynamicContentFileInfo::FileStatus::FILE_STATUS_FAILED);
            if (requestPtr->IsManifestRequest() || requestPtr->IsUserRequested())
            {
                // Top level manifests and user requested paks can be removed from the pending list - they have no dependencies
                RemovePendingPak(requestPtr);
            }
        }

        bool DynamicContentTransferManager::RequestFileStatus(FileTransferSupport::FileRequestMap& requestMap, bool manifestRequest)
        {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
            auto requestJob = PostClientContentRequestJob::Create([this](PostClientContentRequestJob* job)
            {
                auto resultList = job->result.ResultList;
                for (auto thisResult : resultList)
                {
                    AZStd::string fileURL = thisResult.PresignedURL;
                    AZStd::string fileName = thisResult.FileName;
                    AZStd::string fileStatus = thisResult.FileStatus;
                    AZStd::string signatureString = thisResult.Signature;

                    AZ_TracePrintf("CloudCanvas", "Request %s returned status %s URL %s", fileName.c_str(), fileStatus.c_str(), fileURL.c_str());
                    AZ_TracePrintf("CloudCanvas", "Request Signature: %s", signatureString.c_str());

                    DynamicFileInfoPtr requestPtr = GetLocalEntryFromBucketKey(fileName.c_str());

                    if (!requestPtr)
                    {
                        AZ_Warning("CloudCanvas", false,"Could not find request info for %s!", fileName.c_str());
                        return;
                    }

                    if (!fileURL.length())
                    {
                        OnFileStatusFailed(requestPtr);
                        return;
                    }
                    AddPresignedURLRequest(fileURL, requestPtr);

                    requestPtr->SetSignature(signatureString);
                    AZStd::string localFile = requestPtr->GetFullLocalFileName();
 
                    EBUS_EVENT(CloudCanvas::PresignedURLRequestBus, RequestDownloadSignedURL, fileURL, localFile, AZ::EntityId());
                }
            },
            [this](PostClientContentRequestJob* job)
            {
                AZStd::string requestString;
                if (job->parameters.request_content.FileList.size())
                {
                    requestString = job->parameters.request_content.FileList.front();
                }
                auto requestList = job->parameters.request_content.FileList;
                AZ_Warning("CloudCanvas", false, "Failed to retrieve status request list");
                for (auto thisRequest : requestList)
                {
                    DynamicFileInfoPtr pakEntry = GetLocalEntryFromBucketKey(thisRequest.c_str());
                    if (pakEntry)
                    {
                        OnFileStatusFailed(pakEntry);
                    }
                }
            }
            );

            FileTransferSupport::ValidateWritable(requestMap);

            for (auto thisFile : requestMap)
            {
                auto thisEntry = GetLocalEntryFromBucketKey(thisFile.first.c_str());
                if (thisEntry)
                {
                    thisEntry->SetStatus(DynamicContentFileInfo::FileStatus::UPDATING);
                   
                    if (thisEntry->IsUserRequested())
                    {
                        AddPendingPak(thisEntry);
                    }
                }
                requestJob->parameters.request_content.FileList.push_back(thisFile.first);
            }
            requestJob->Start();
#endif
            return true;
        }

        void DynamicContentTransferManager::AddPresignedURLRequest(const AZStd::string& requestURL, DynamicFileInfoPtr fileInfo)
        {
            AZStd::lock_guard<AZStd::mutex> pakMutex(m_presignedURLMutex);
            m_presignedURLToFileInfo[requestURL] = fileInfo;
        }

        DynamicContentTransferManager::DynamicFileInfoPtr DynamicContentTransferManager::GetAndRemovePresignedRequest(const AZStd::string& requestURL)
        {
            DynamicFileInfoPtr returnPtr;
            AZStd::lock_guard<AZStd::mutex> pakMutex(m_presignedURLMutex);
            auto fileInfoIter = m_presignedURLToFileInfo.find(requestURL);
            if (fileInfoIter != m_presignedURLToFileInfo.end())
            {
                returnPtr = fileInfoIter->second;
                m_presignedURLToFileInfo.erase(fileInfoIter);
            }
            return returnPtr;
        }
        bool DynamicContentTransferManager::ClearAllContent()
        {   
            AZStd::vector<DynamicFileInfoPtr> removedVec;
            {
                AZStd::lock_guard<AZStd::mutex> fileLock(m_fileListMutex);
                for(auto thisElement : m_fileList)
                { 
                    removedVec.push_back(thisElement.second);
                }
                m_fileList.clear();
                m_bucketKeyToFileInfo.clear();
            }
            for (auto removedInfo : removedVec)
            {
                RemovePendingPak(removedInfo);
            }
            {
                AZStd::lock_guard<AZStd::mutex> pakMutex(m_presignedURLMutex);
                m_presignedURLToFileInfo.clear();
            }
            return true;
        }

        bool DynamicContentTransferManager::RemovePak(const char* fileName)
        {
            DynamicFileInfoPtr returnPtr;
            {
                AZStd::lock_guard<AZStd::mutex> fileLock(m_fileListMutex);
                auto infoIter = m_fileList.find(fileName);
                if (infoIter == m_fileList.end())
                {
                    return false;
                }
                returnPtr = infoIter->second;
                m_fileList.erase(infoIter);
                m_bucketKeyToFileInfo.erase(returnPtr->MakeBucketHashName());
            }
            {
                AZStd::lock_guard<AZStd::mutex> pakLock(m_pakFileMountMutex);
                AZStd::remove(m_pakFilesToMount.begin(), m_pakFilesToMount.end(), returnPtr);
            }
            return true;
        }

        DynamicContentTransferManager::SignatureHashVec DynamicContentTransferManager::GetMD5Hash(const AZStd::string& filePath)
        {
            return FileTransferSupport::GetMD5Buffer(filePath.c_str());
        }

        AZStd::string DynamicContentTransferManager::GetDefaultPublicKeyPath() const
        {
            AZStd::string returnPath{ certificateFolder };
            returnPath += publicCertName;

            return returnPath;
        }

        int DynamicContentTransferManager::ValidateSignature(DynamicFileInfoPtr pakInfo) const
        {
            if (!requireSignatures && !pakInfo->GetSignature().size())
            {
                return true;
            }

            AZStd::string localStr(pakInfo->GetFullLocalFileName());
            AZStd::string localHash = FileTransferSupport::CalculateMD5(localStr.c_str());

            return ValidateSignatureOpenSSL(localHash, pakInfo->GetSignature());
        }

        int DynamicContentTransferManager::ValidateEncodedSignature(const AZStd::string& checkString, const AZStd::string& signatureString) const
        {
            AZStd::vector<unsigned char> sigVec;
            sigVec.resize(GetDecodedSize(signatureString));
            if (sigVec.size())
            {
                Base64::decode_base64(reinterpret_cast<char*>(&sigVec[0]), signatureString.c_str(), signatureString.length(), false);
            }
            return ValidateSignatureOpenSSL(checkString, sigVec);
        }

        // OpenSSL Signature Verification
        int DynamicContentTransferManager::ValidateSignatureOpenSSL(const AZStd::string& checkString, const AZStd::vector<unsigned char>& signatureBuf) const
        {
            int verifyResult = 1;

#if defined(OPENSSL_ENABLED)
            AZ_TracePrintf("CloudCanvas", "Attempting to validate signature of string %s", checkString.c_str());

            if (!checkString.size())
            {
                AZ_TracePrintf("CloudCanvas", "Failed to compute hash for signature check for %s", checkString.c_str());
                return 0;
            }

            AZStd::vector<unsigned char> keyBuf;
            AZ::IO::HandleType pFile = gEnv->pCryPak->FOpen(GetDefaultPublicKeyPath().c_str(), "rt");
            if (pFile == AZ::IO::InvalidHandle)
            {
                AZ_TracePrintf("CloudCanvas", "No public key file found at %s", GetDefaultPublicKeyPath().c_str());
                // Pass if we don't have a public key and weren't given a signature
                return signatureBuf.size() == 0;
            }

            if (!signatureBuf.size())
            {
                gEnv->pCryPak->FClose(pFile);
                AZ_TracePrintf("CloudCanvas", "No signature set for %s", checkString.c_str());
                return 0;
            }

            size_t fileSize = gEnv->pCryPak->FGetSize(pFile);
            if (!fileSize)
            {
                AZ_TracePrintf("CloudCanvas", "No public key data found at %s", GetDefaultPublicKeyPath().c_str());
                gEnv->pCryPak->FClose(pFile);
                // Pass if we don't have a public key
                return 1;
            }
            keyBuf.resize(fileSize);

            size_t read = gEnv->pCryPak->FRead(&keyBuf[0], fileSize, pFile);
            gEnv->pCryPak->FClose(pFile);

            BIO *bufio;
            bufio = BIO_new_mem_buf((void*)&keyBuf[0], keyBuf.size());
            EVP_PKEY* pubKey = PEM_read_bio_PUBKEY(bufio, nullptr, 0, nullptr);

            if (!pubKey)
            {
                AZ_Warning("CloudCanvas", false, "Failed to load public key for %s", checkString.c_str());
                return 0;
            }

            EVP_MD_CTX *mdctx = NULL;
            if (!(mdctx = EVP_MD_CTX_create())) 
            {
                AZ_Error("CloudCanvas", false, "Failed to create MD ctx");
                return 0;
            }
            
            int returnValue = EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, pubKey);
            if (returnValue != 1)
            {
                AZ_Error("CloudCanvas", false, "Failed DigestVerifyInit");
                return returnValue;
            }
            returnValue = EVP_DigestVerifyUpdate(mdctx, reinterpret_cast<const unsigned char*>(&checkString[0]), checkString.size());

            if (returnValue != 1)
            {
                AZ_Error("CloudCanvas", false,"Failed DigestVerifyInit");
                return returnValue;
            }
            verifyResult = EVP_DigestVerifyFinal(mdctx, &signatureBuf[0], signatureBuf.size());
            if(verifyResult == 1)
            { 
                AZ_TracePrintf("CloudCanvas", "Signature verified for %s", checkString.c_str());
            }
            else if (verifyResult == 0)
            {
                AZ_TracePrintf("CloudCanvas", "Signature didn't match for %s", checkString.c_str());
            }
            else
            {
                AZ_TracePrintf("CloudCanvas", "Signature verification error for %s: %d", checkString.c_str(), verifyResult);
            }
#else
            AZ_TracePrintf("CloudCanvas", "Can't validate signature - OpenSSL is not enabled for platform.");
#endif  // OPENSSL_ENABLED
            return verifyResult;
        }

        size_t DynamicContentTransferManager::GetDecodedSize(const AZStd::string& base64String)
        {
            size_t stringLength = base64String.length();
            if (!stringLength)
            {
                return stringLength;
            }

            size_t padding = 0;
            if (base64String[stringLength - 1] == '=')
            {
                ++padding;
            }
            if (stringLength > 1 && base64String[stringLength - 2] == '=')
            {
                ++padding;
            }

            return (stringLength * 3 / 4 - padding);
        }
    }
}
