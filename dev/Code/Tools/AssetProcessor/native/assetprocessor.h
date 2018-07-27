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

#include <QPair>
#include <QMetaType>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Crc.h>
#include <QString>
#include <QList>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzCore/Math/Crc.h>

namespace AssetProcessor
{
    const char* const DebugChannel = "Debug"; //Use this channel name if you want to write the message to the log file only.
    const char* const ConsoleChannel = "AssetProcessor";// Use this channel name if you want to write the message to both the console and the log file.
    const char* const FENCE_FILE_EXTENSION = "fence"; //fence file extension
    const char* const AutoFailReasonKey = "failreason"; // the key to look in for auto-fail reason.
    const char* const AutoFailLogFile = "faillogfile"; // if this is provided, this is a complete log of the failure and will be added after the failreason.
    const char* const AutoFailOmitFromDatabaseKey = "failreason_omitFromDatabase"; // if set in your job info hash, your job will not be tracked by the database.
    const unsigned int g_RetriesForFenceFile = 5; // number of retries for fencing
    // Even though AP can handle files with path length greater than window's legacy path length limit, we have some 3rdparty sdk's
    // which do not handle this case ,therefore we will make AP fail any jobs whose either source file or output file name exceeds the windows legacy path length limit
#define AP_MAX_PATH_LEN 260

    extern AZ::s64 GetThreadLocalJobId();
    extern void SetThreadLocalJobId(AZ::s64 jobId);

    //! a shared convenience typedef for requests that have come over the network
    //! The first element is the connection id it came from and the second element is the serial number
    //! which can be used to send a response.
    typedef QPair<quint32, quint32> NetworkRequestID;

    //! a shared convenience typedef for Escalating Jobs
    //! The first element is the jobRunKey of the job and the second element is the escalation
    typedef QList<QPair<AZ::s64, int> > JobIdEscalationList;

    enum AssetScanningStatus
    {
        Unknown,
        Started,
        InProgress,
        Completed,
        Stopped
    };

    //! This enum stores all the different job escalation values
    enum JobEscalation
    {
        ProcessAssetRequestSyncEscalation = 200,
        ProcessAssetRequestStatusEscalation = 150,
        AssetJobRequestEscalation = 100,
        Default = 0
    };
    //! This enum stores all the different asset processor status values
    enum AssetProcessorStatus
    {
        Initializing_Gems,
        Initializing_Builders,
        Scanning_Started,
        Analyzing_Jobs,
        Processing_Jobs,
    };

    enum AssetCatalogStatus
    {
        RequiresSaving, 
        UpToDate
    };

    //! AssetProcessorStatusEntry stores all the necessary information related to AssetProcessorStatus
    struct AssetProcessorStatusEntry
    {
        AssetProcessorStatus m_status;
        unsigned int m_count = 0;
        QString m_extraInfo; //this can be used to send any other info like name etc

        explicit AssetProcessorStatusEntry(AssetProcessorStatus status, unsigned int count = 0, QString extraInfo = QString())
            : m_status(status)
            , m_count(count)
            , m_extraInfo(extraInfo)
        {
        }

        AssetProcessorStatusEntry() = default;
    };

    struct AssetRecognizer;

    //! JobEntry is an internal structure that is used to uniquely identify a specific job and keeps track of it as it flows through the AP system
    //! It prevents us from having to copy the entire of JobDetails, which is a very heavy structure.
    //! In general, communication ABOUT jobs will have the JobEntry as the key
    class JobEntry
    {
    public:
        // note that QStrings are ref-counted copy-on-write, so a move operation will not be beneficial unless this struct gains considerable heap allocated fields.

        QString m_databaseSourceName;                           //! DATABASE "SourceName" Column, which includes the 'output prefix' if present, used for keying
        QString m_watchFolderPath;                              //! contains the absolute path to the watch folder that the file was found in.
        QString m_pathRelativeToWatchFolder;                    //! contains the relative path (from the above watch folder) that the file was found in.
        AZ::Uuid m_builderGuid = AZ::Uuid::CreateNull();        //! the builder that will perform the job
        AssetBuilderSDK::PlatformInfo m_platformInfo;
        AZ::Uuid m_sourceFileUUID = AZ::Uuid::CreateNull(); ///< The actual UUID of the source being processed
        QString m_jobKey;     // JobKey is used when a single input file, for a single platform, for a single builder outputs many separate jobs
        AZ::u32 m_computedFingerprint = 0;     // what the fingerprint was at the time of job creation.
        qint64 m_computedFingerprintTimeStamp = 0; // stores the number of milliseconds since the universal coordinated time when the fingerprint was computed.
        AZ::u64 m_jobRunKey = 0;
        bool m_checkExclusiveLock = false;      ///< indicates whether we need to check the input file for exclusive lock before we process this job
        bool m_addToDatabase = true; ///< If false, this is just a UI job, and should not affect the database.

        QString GetAbsoluteSourcePath() const
        {
            if (!m_watchFolderPath.isEmpty())
            {
                return m_watchFolderPath + "/" + m_pathRelativeToWatchFolder;
            }
            return m_pathRelativeToWatchFolder;
        }

        AZ::u32 GetHash() const
        {
            AZ::Crc32 crc(m_databaseSourceName.toUtf8().constData());
            crc.Add(m_platformInfo.m_identifier.c_str());
            crc.Add(m_jobKey.toUtf8().constData());
            crc.Add(m_builderGuid.ToString<AZStd::string>().c_str());
            return crc;
        }

        JobEntry() = default;
        JobEntry(const JobEntry& other) = default;
        
        JobEntry& operator=(const JobEntry& other) = default;

        // vs2013 compat:  No auto-generation of move operations
        JobEntry(JobEntry&& other)
        {
            if (this != &other)
            {
                *this = std::move(other);
            }
        }
        
        JobEntry& operator=(JobEntry&& other)
        {
            if (this != &other)
            {
                m_watchFolderPath = std::move(other.m_watchFolderPath);
                m_databaseSourceName = std::move(other.m_databaseSourceName);
                m_pathRelativeToWatchFolder = std::move(other.m_pathRelativeToWatchFolder);
                m_builderGuid = std::move(other.m_builderGuid);
                m_platformInfo = std::move(other.m_platformInfo);
                m_sourceFileUUID = std::move(other.m_sourceFileUUID);
                m_jobKey = std::move(other.m_jobKey);
                m_computedFingerprint = std::move(other.m_computedFingerprint);
                m_computedFingerprintTimeStamp = std::move(other.m_computedFingerprintTimeStamp);
                m_jobRunKey = std::move(other.m_jobRunKey);
                m_checkExclusiveLock = std::move(other.m_checkExclusiveLock);
                m_addToDatabase = std::move(other.m_addToDatabase);
            }
            return *this;
        }

        JobEntry(QString watchFolderPath, QString relativePathToFile, QString databaseSourceName, const AZ::Uuid& builderGuid, const AssetBuilderSDK::PlatformInfo& platformInfo, QString jobKey, AZ::u32 computedFingerprint, AZ::u64 jobRunKey, const AZ::Uuid &sourceUuid, bool addToDatabase = true)
            : m_watchFolderPath(watchFolderPath)
            , m_pathRelativeToWatchFolder(relativePathToFile)
            , m_databaseSourceName(databaseSourceName)
            , m_builderGuid(builderGuid)
            , m_platformInfo(platformInfo)
            , m_jobKey(jobKey)
            , m_computedFingerprint(computedFingerprint)
            , m_jobRunKey(jobRunKey)
            , m_addToDatabase(addToDatabase)
            , m_sourceFileUUID(sourceUuid)
        {
        }
    };

    //! This is an internal structure that hold all the information related to source file Dependency
    struct SourceFileDependencyInternal
    {
        AZStd::string m_sourceWatchFolder;
        AZStd::string m_relativeSourcePath;
        AZ::Uuid m_sourceUUID;
        AZ::Uuid m_builderId;
        AZStd::string m_relativeSourceFileDependencyPath;
        AssetBuilderSDK::SourceFileDependency m_sourceFileDependency;

        AZStd::string ToString() const
        {
            return AZStd::string::format(" %s %s %s", m_sourceUUID.ToString<AZStd::string>().c_str(), m_builderId.ToString<AZStd::string>().c_str(), m_relativeSourceFileDependencyPath.c_str());
        }
    };

    //! JobDetails is an internal structure that is used to store job related information by the Asset Processor
    //! Its heavy, since it contains the parameter map and the builder desc so is expensive to copy and in general only used to create jobs
    //! After which, the Job Entry is used to track and identify jobs.
    class JobDetails
    {
    public:
        JobEntry m_jobEntry;
        QString m_extraInformationForFingerprinting;
        QString m_destinationPath; // the final folder that will be where your products are placed if you give relative path names
        // destinationPath will be a cache folder.  If you tell it to emit something like "blah.dds"
        // it will put it in (destinationPath)/blah.dds for example

        AZStd::vector<SourceFileDependencyInternal> m_sourceFileDependencyList;
        AZStd::vector<AZStd::string> m_fingerprintFilesList;

        bool m_critical = false;
        int m_priority = -1;

        AssetBuilderSDK::AssetBuilderDesc   m_assetBuilderDesc;
        AssetBuilderSDK::JobParameterMap    m_jobParam;

        // autoFail makes jobs which are added to the list and will automatically fail, and are used
        // to make sure that a "failure" shows up on the list so that the user can click to inspect the job and see why
        // it has failed instead of having a job fail mysteriously or be hard to find out why.
        // it is currently the only way for the job to be marked as a failure because of data integrity reasons after the builder
        // has already succeeded in actually making the asset data.
        // if you set a job to "auto fail" it will check the m_jobParam map for a AZ_CRC(AutoFailReasonKey) and use that, if present, for fail information
        bool m_autoFail = false;

        // indicates a succeeded createJob and clears out any autoFail failures
        bool m_autoSucceed = false;

        JobDetails() = default;
    };
} // namespace AssetProcessor

Q_DECLARE_METATYPE(AssetBuilderSDK::ProcessJobResponse)
Q_DECLARE_METATYPE(AssetProcessor::JobEntry)
Q_DECLARE_METATYPE(AssetProcessor::AssetProcessorStatusEntry)
Q_DECLARE_METATYPE(AssetProcessor::JobDetails)
Q_DECLARE_METATYPE(AssetProcessor::NetworkRequestID)
Q_DECLARE_METATYPE(AssetProcessor::AssetScanningStatus)
Q_DECLARE_METATYPE(AssetProcessor::AssetCatalogStatus)
