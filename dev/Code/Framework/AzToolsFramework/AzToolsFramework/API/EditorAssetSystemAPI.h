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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Azcore/Asset/AssetManagerBus.h>

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        class AssetJobsInfoResponse;
        class AssetJobsInfoRequest;

        //! This struct is used for responses and requests about source assets
        struct SourceFileInfo
        {
            AZ_TYPE_INFO(SourceFileInfo, "{163A7399-0481-4315-BB94-5C07AC3C4B1B}")

            SourceFileInfo() = default;
            SourceFileInfo(const SourceFileInfo& other) = default;
            SourceFileInfo& operator=(const SourceFileInfo& other) = default;

            SourceFileInfo(SourceFileInfo&& other)
            {
                *this = AZStd::move(other);
            }

            SourceFileInfo& operator=(SourceFileInfo&& other)
            {
                if (this != &other)
                {
                    m_watchFolder = AZStd::move(other.m_watchFolder);
                    m_relativePath = AZStd::move(other.m_relativePath);
                    m_sourceGuid = other.m_sourceGuid;
                }
                return *this;
            }

            static void Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
                if (serialize)
                {
                    serialize->Class<SourceFileInfo>()
                        ->Version(1)
                        ->Field("watchFolder", &SourceFileInfo::m_watchFolder)
                        ->Field("relativePath", &SourceFileInfo::m_relativePath)
                        ->Field("sourceGuid", &SourceFileInfo::m_sourceGuid);
                }
            }

            //! The folder that's monitored for changes to the source file.
            AZStd::string m_watchFolder;

            //! The relative path to the source file. This is relative to the watch folder.
            AZStd::string m_relativePath;

            //! The unique identifier for the source file.
            AZ::Uuid m_sourceGuid = AZ::Uuid::CreateNull();
        };

        //! A bus to talk to the asset system as a tool or editor
        //! This contains things that only tools or editors should be given access to
        //! If you want to talk to it as if a game engine component or runtime component
        //! \ref AssetSystemBus.h
        //! in the common header location.
        class AssetSystemRequest
            : public AZ::EBusTraits
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // single bus

            using MutexType = AZStd::recursive_mutex;

            virtual ~AssetSystemRequest() = default;

            //! Retrieve the absolute folder path to the current game's source assets (the ones that go into source control)
            //! This may include the current mod path, if a mod is being edited by the editor
            virtual const char* GetAbsoluteDevGameFolderPath() = 0;

            //! Retrieve the absolute folder path to the current developer root ('dev'), which contains source artifacts
            //! and is generally checked into source control.
            virtual const char* GetAbsoluteDevRootFolderPath() = 0;
        
            /// Convert a full source path like "c:\\dev\gamename\\blah\\test.tga" into a relative product path.
            /// asset paths never mention their alias and are relative to the asset cache root
            virtual bool GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string& fullPath, AZStd::string& relativeProductPath) = 0;

            /// Convert a relative asset path like "blah/test.tga" to a full source path path.
            /// Once the asset processor has finished building, this function is capable of handling even when the extension changes
            /// or when the source is in a different folder or in a different location (such as inside gems)
            virtual bool GetFullSourcePathFromRelativeProductPath(const AZStd::string& relPath, AZStd::string& fullSourcePath) = 0;

            //! Send out queued events
            virtual void UpdateQueuedEvents() = 0;

            //! Retrieve the watch folder and relative path for the source asset
            virtual bool GetSourceAssetInfoById(const AZ::Uuid& guid, AZStd::string& watchFolder, AZStd::string& relativePath) = 0;

            //! Retrieve information about the source file from the source path.
            virtual bool GetSourceFileInfoByPath(SourceFileInfo& result, const char* sourcePath) = 0;
        };
        

        //! AssetSystemBusTraits
        //! This bus is for events that concern individual assets and is addressed by file extension
        class AssetSystemNotifications
            : public AZ::EBusTraits
        {
        public:
            typedef AZStd::recursive_mutex MutexType;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multiple listeners
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const bool EnableEventQueue = true; // enabled queued events, asset messages come from any thread

            virtual ~AssetSystemNotifications() = default;

            //! Called by the AssetProcessor when a source of an asset has been modified.
            virtual void SourceFileChanged(AZStd::string /*assetId*/, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/) {}
            //! Called by the AssetProcessor when a source of an asset has been removed.
            virtual void SourceFileRemoved(AZStd::string /*assetId*/, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/) {}
            //! This will be used by the asset processor to notify whenever a source file fails to process.
            virtual void SourceFileFailed(AZStd::string /*assetId*/, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/) {}
        };

        //! This enum have all the different job states
        //! Please note that these job status are written to the database, so it is very important that any new status should be added at the end else the database might get corrupted
        enum class JobStatus
        {
            Any = -1, //used exclusively by the database to indicate any state query, no job should ever actually be in this state, we start in Queued and progress from there
            Queued,  // its in the queue and will be built shortly
            InProgress,  // its being compiled right now.
            Failed,
            Failed_InvalidSourceNameExceedsMaxLimit, // use this enum to indicate that the job failed because the source file name length exceeds the maximum length allowed 
            Completed, // built successfully (no failure occurred)
            Missing //indicate that the job is not present for example if the source file is not there, or if job key is not there
        };

        inline const char* JobStatusString(JobStatus status)
        {
            switch(status)
            {
                case JobStatus::Any: return "Any";
                case JobStatus::Queued: return "Queued";
                case JobStatus::InProgress: return "InProgress";
                case JobStatus::Failed: return "Failed";
                case JobStatus::Failed_InvalidSourceNameExceedsMaxLimit: return "Failed_InvalidSourceNameExceedsMaxLimit";
                case JobStatus::Completed: return "Completed";
                case JobStatus::Missing: return "Missing";
            }
            return nullptr;
        }


        //! This struct is used for responses and requests about Asset Processor Jobs
        struct JobInfo
        {
            AZ_TYPE_INFO(JobInfo, "{276C9DE3-0C81-4721-91FE-F7C961D28DA8}")
            JobInfo()
            {
                m_jobRunKey = rand();
            }
            JobInfo(const JobInfo& other) = default;
            JobInfo& operator=(const JobInfo& other) = default;

            JobInfo(JobInfo&& other)
            {
                *this = AZStd::move(other);
            }

            JobInfo& operator=(JobInfo&& other)
            {
                if (this != &other)
                {
                    m_sourceFile = AZStd::move(other.m_sourceFile);
                    m_watchFolder = AZStd::move(other.m_watchFolder);
                    m_platform = AZStd::move(other.m_platform);
                    m_builderGuid = AZStd::move(other.m_builderGuid);
                    m_jobKey = AZStd::move(other.m_jobKey);
                    m_status = other.m_status;
                    m_jobRunKey = other.m_jobRunKey;
                    m_firstFailLogTime = other.m_firstFailLogTime;
                    m_firstFailLogFile = AZStd::move(other.m_firstFailLogFile);
                    m_lastFailLogTime = other.m_lastFailLogTime;
                    m_lastFailLogFile = AZStd::move(other.m_lastFailLogFile);
                    m_lastLogTime = other.m_lastLogTime;
                    m_lastLogFile = AZStd::move(other.m_lastLogFile);
                    m_jobID = other.m_jobID;
                }
                return *this;
            }

            AZ::u32 GetHash() const
            {
                AZ::Crc32 crc(m_sourceFile.c_str());
                crc.Add(m_platform.c_str());
                crc.Add(m_jobKey.c_str());
                crc.Add(m_builderGuid.ToString<AZStd::string>().c_str());
                return crc;
            }

            static void Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
                if (serialize)
                {
                    serialize->Class<JobInfo>()
                        ->Version(3)
                        ->Field("sourceFile", &JobInfo::m_sourceFile)
                        ->Field("platform", &JobInfo::m_platform)
                        ->Field("builderUuid", &JobInfo::m_builderGuid)
                        ->Field("jobKey", &JobInfo::m_jobKey)
                        ->Field("jobRunKey", &JobInfo::m_jobRunKey)
                        ->Field("status", &JobInfo::m_status)
                        ->Field("firstFailLogTime", &JobInfo::m_firstFailLogTime)
                        ->Field("firstFailLogFile", &JobInfo::m_firstFailLogFile)
                        ->Field("lastFailLogTime", &JobInfo::m_lastFailLogTime)
                        ->Field("lastFailLogFile", &JobInfo::m_lastFailLogFile)
                        ->Field("lastLogTime", &JobInfo::m_lastLogTime)
                        ->Field("lastLogFile", &JobInfo::m_lastLogFile)
                        ->Field("jobID", &JobInfo::m_jobID)
                        ->Field("watchFolder", &JobInfo::m_watchFolder);
                }
            }

            //! the file from which this job was originally spawned.  Is just the relative source file name ("whatever/something.tif", not an absolute path)
            AZStd::string m_sourceFile;

            //! the watchfolder for the file from which this job was originally spawned.
            AZStd::string m_watchFolder;

            //! which platform this is for.  Will be something like "pc" or "android"
            AZStd::string m_platform;

            //! The uuid of the builder
            AZ::Uuid m_builderGuid = AZ::Uuid::CreateNull();

            //! Job Key is arbitrarily defined by the builder.  Used to differentiate between different jobs emitted for the same input file, for the same platform, for the same builder.
            //! for example, you might want to split a particularly complicated and time consuming job into multiple sub-jobs.  In which case they'd all have the same input file,
            //! the same platform, the same builder UUID (since its the UUID of the builder itself)
            //! but would have different job keys.
            AZStd::string m_jobKey;
            
            //random int made to identify this attempt to process this job
            AZ::u64 m_jobRunKey = 0;

            //current status
            JobStatus m_status = JobStatus::Queued;

            //logging
            AZ::s64 m_firstFailLogTime = 0;
            AZStd::string m_firstFailLogFile;
            AZ::s64 m_lastFailLogTime = 0;
            AZStd::string m_lastFailLogFile;
            AZ::s64 m_lastLogTime = 0;
            AZStd::string m_lastLogFile;

            AZ::s64 m_jobID = 0; // this is the actual database row.   Client is unlikely to need this.
        };

        typedef AZStd::vector<JobInfo> JobInfoContainer; 

        //! This Ebus will be used to retrieve all the job related information from AP
        class AssetSystemJobRequest
            : public AZ::EBusTraits
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus

            virtual ~AssetSystemJobRequest() = default;

            /// Retrieve Jobs information for the given source file, setting escalteJobs to true will escalate all queued jobs 
            virtual AZ::Outcome<JobInfoContainer> GetAssetJobsInfo(const AZStd::string& sourcePath, const bool escalateJobs) = 0;

            /// Retrieve Jobs information for the given assetId, setting escalteJobs to true will escalate all queued jobs 
            virtual AZ::Outcome<JobInfoContainer> GetAssetJobsInfoByAssetID(const AZ::Data::AssetId& assetId, const bool escalateJobs) = 0;

            /// Retrieve Jobs information for the given jobKey 
            virtual AZ::Outcome<JobInfoContainer> GetAssetJobsInfoByJobKey(const AZStd::string& jobKey, const bool escalateJobs) = 0;

            /// Retrieve Job Status for the given jobKey.
            /// If no jobs are present, return missing,
            /// else, if any matching jobs have failed, it will return failed
            /// else, if any of the matching jobs are queued, it will return queued
            /// else, if any matching jobs are in progress, will return inprogress
            /// else it will return the completed job status.
            virtual AZ::Outcome<JobStatus> GetAssetJobsStatusByJobKey(const AZStd::string& jobKey, const bool escalateJobs) = 0;

            /// Retrieve the actual log content for a particular job.   you can retrieve the run key from the above info function.
            virtual AZ::Outcome<AZStd::string> GetJobLog(AZ::u64 jobrunkey) = 0;
        };

    } // namespace AssetSystem
    using AssetSystemBus = AZ::EBus<AssetSystem::AssetSystemNotifications>;
    using AssetSystemRequestBus = AZ::EBus<AssetSystem::AssetSystemRequest>;
    using AssetSystemJobRequestBus = AZ::EBus<AssetSystem::AssetSystemJobRequest>;
} // namespace AzToolsFramework