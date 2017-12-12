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
    const char* const AutoFailOmitFromDatabaseKey = "failreason_omitFromDatabase"; // if set in your job info hash, your job will not be tracked by the database.
    const unsigned int g_RetriesForFenceFile = 5; // number of retries for fencing
    // Even though AP can handle files with path length greater than window's legacy path length limit, we have some 3rdparty sdk's
    // which do not handle this case ,therefore we will make AP fail any jobs whose either source file or output file name exceeds the windows legacy path length limit
#define AP_MAX_PATH_LEN 260

#if defined(AZ_PLATFORM_APPLE_IOS)
    const char* const CURRENT_PLATFORM = "ios";
#elif defined(AZ_PLATFORM_APPLE_OSX)
    const char* const CURRENT_PLATFORM = "osx_gl";
#elif defined(AZ_PLATFORM_ANDROID)
    const char* const CURRENT_PLATFORM = "es3";
#elif defined(AZ_PLATFORM_WINDOWS)
    const char* const CURRENT_PLATFORM = "pc";
#endif

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

        QString m_relativePathToFile;     ///< contains relative path (relative to watch folder), used to identify input file and database name
        QString m_absolutePathToFile;     ///< contains the actual absolute path to the file, including watch folder.
        AZ::Uuid m_sourceFileUUID = AZ::Uuid::CreateNull(); ///< The actual UUID of the source being processed
        AZ::Uuid m_builderGuid = AZ::Uuid::CreateNull();     ///< the builder that will perform the job
        QString m_platform;     ///< the platform that the job will operate for
        QString m_jobKey;    ///< JobKey is used when a single input file, for a single platform, for a single builder outputs many separate jobs
        AZ::u32 m_computedFingerprint = 0;     ///< what the fingerprint was at the time of job creation.
        qint64 m_computedFingerprintTimeStamp = 0; ///< stores the number of milliseconds since the universal coordinated time when the fingerprint was computed.
        AZ::u64 m_jobRunKey = 0;
        bool m_checkExclusiveLock = false;      ///< indicates whether we need to check the input file for exclusive lock before we process this job
        bool m_addToDatabase = true; ///< If false, this is just a UI job, and should not affect the database.

        AZ::u32 GetHash() const
        {
            AZ::Crc32 crc(m_relativePathToFile.toUtf8().constData());
            crc.Add(m_platform.toUtf8().constData());
            crc.Add(m_jobKey.toUtf8().constData());
            crc.Add(m_builderGuid.ToString<AZStd::string>().c_str());
            return crc;
        }

        JobEntry()
        {
        }

        JobEntry(const JobEntry& other) = default;

        JobEntry(const QString& absolutePathToFile, const QString& relativePathToFile, const AZ::Uuid& builderGuid, const QString& platform, const QString& jobKey, AZ::u32 computedFingerprint, AZ::u64 jobRunKey, const AZ::Uuid &sourceUuid, bool addToDatabase=true)
            : m_absolutePathToFile(absolutePathToFile)
            , m_relativePathToFile(relativePathToFile)
            , m_builderGuid(builderGuid)
            , m_platform(platform)
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
        QString m_watchFolder; // the watch folder the file was found in
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
