
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

#include "CloudGemDefectReporter_precompiled.h"

#include "DefectReportManager.h"
#include <PostDefectReportsJob.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>

namespace CloudGemDefectReporter
{
    ReportWrapper::ReportWrapper(const DefectReport& report) :
        DefectReport(report.GetReportID(), report.GetMetrics(), report.GetAttachments())
    {
         // this constructor is used to load from serialized reports, so the report must be complete
        // therefore we just need to make sure the IsReportComplete function reports true
        // but don't need to worry about handler ID's as they are only used to keep track of
        // incomplete report elements
        m_dataReceived[0] = true;
    }

    void ReportWrapper::RegisterHandlerID(int id)
    {
        auto handleSlotIter = m_dataReceived.find(id);
        if (handleSlotIter != m_dataReceived.end())
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: Attempt to register handler ID already registered: %d", id);
        }
        else
        {
            m_dataReceived.emplace(id, false);
        }
    }

    void ReportWrapper::MarkHandlerReceived(int id)
    {
        auto handleSlotIter = m_dataReceived.find(id);
        if (handleSlotIter != m_dataReceived.end())
        {
            handleSlotIter->second = true;
        }
        else
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: Attempt to mark handler ID that doesn't exist: %d", id);
        }
    }

    bool ReportWrapper::IsReportComplete() const
    {
        return AZStd::all_of(m_dataReceived.begin(), m_dataReceived.end(),
            [](const AZStd::pair<int, bool> t) {return t.second == true; });
    }

    void ReportWrapper::ClearHandlers()
    {
        m_dataReceived.clear();
    }

    void ReportWrapper::AddReportData(int handlerID, AZStd::vector<MetricDesc> metrics, AZStd::vector<AttachmentDesc> attachments)
    {
        MarkHandlerReceived(handlerID);
        m_metrics.insert(m_metrics.end(), metrics.begin(), metrics.end());
        m_attachments.insert(m_attachments.end(), attachments.begin(), attachments.end());
    }

    static const char*  ANNOTATION_KEY = "annotation";

    void ReportWrapper::AddAnnotation(const AZStd::string& annotation)
    {
        // if annotation exists, update it
        auto foundIter = AZStd::find_if(m_metrics.begin(), m_metrics.end(), [](const MetricDesc& metric){ return metric.m_key == ANNOTATION_KEY; });
        if (foundIter == m_metrics.end())
        {
            MetricDesc annotationMetric;
            annotationMetric.m_key = ANNOTATION_KEY;
            annotationMetric.m_data = annotation;
            m_metrics.push_back(annotationMetric);
        }
        else
        {
            foundIter->m_data = annotation;
        }
    }

    void ReportWrapper::AddCustomField(const AZStd::string& key, const AZStd::string& value)
    {
        // if annotation exists, update it
        auto foundIter = AZStd::find_if(m_metrics.begin(), m_metrics.end(), [key](const MetricDesc& metric) { return metric.m_key == key; });
        if (foundIter == m_metrics.end())
        {
            MetricDesc typeMetric;
            typeMetric.m_key = key;
            typeMetric.m_data = value;
            m_metrics.push_back(typeMetric);
        }
        else
        {
            foundIter->m_data = value;
        }
    }

    void ReportWrapper::ClearReportData()
    {
        m_metrics.clear();
        m_attachments.clear();
    }
    
    DefectReportManager::DefectReportManager()
    {
    }

    DefectReportManager::~DefectReportManager()
    {

    }

    int DefectReportManager::StartNewReport()
    {
        int currentReportID = m_nextReportID;
        ++m_nextReportID;

        ReportWrapper report(currentReportID);
        m_reports.emplace(currentReportID, report);

        return currentReportID;
    }

    int DefectReportManager::GetNextHandlerID(int reportID)
    {
        auto reportIter = m_reports.find(reportID);
        if (reportIter != m_reports.end())
        {
            int handlerID = reportIter->second.GetNextHandlerID();
            reportIter->second.RegisterHandlerID(handlerID);
            return handlerID;
        }
        else
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: Requested report ID that doesn't exist (%d) in GetNextHandlerID", reportID);
            return INVALID_ID;
        }
    }

    void DefectReportManager::ReportData(int reportID, 
        int handlerID, 
        AZStd::vector<MetricDesc> metrics, 
        AZStd::vector<AttachmentDesc> attachments)
    {
        auto reportIter = m_reports.find(reportID);
        if (reportIter != m_reports.end())
        {
            reportIter->second.AddReportData(handlerID, metrics, attachments);
        }
        else
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: Attempt to report data to a report ID that doesn't exist (%d) in ReportData", reportID);
        }
    }

    bool DefectReportManager::IsReportComplete(int reportID)
    {
        auto reportIter = m_reports.find(reportID);
        if (reportIter != m_reports.end())
        {
            return reportIter->second.IsReportComplete();
        }
        else
        {
            // no need to warn, the report may just not be registered yet
            return false;
        }
    }

    int DefectReportManager::CountCompleteReports() const
    {
        return static_cast<int>(AZStd::count_if(m_reports.begin(), m_reports.end(),
            [](const AZStd::pair<int, ReportWrapper> t) {return t.second.IsReportComplete() == true; }));
    }

    int DefectReportManager::CountPendingReports() const
    {
        return m_reports.size() - CountCompleteReports();
    }

    int DefectReportManager::CountAttachments() const
    {
        int numAttachments = 0;
        for (auto& report : m_reports)
        {
            numAttachments += report.second.GetAttachments().size();
        }

        return numAttachments;
    }

    AZStd::vector<int> DefectReportManager::GetAvailiableReportIDs() const
    {
        AZStd::vector<int> availableReportIDs;
        for (auto iter(m_reports.begin()); 
            m_reports.end() != (iter = AZStd::find_if(iter, m_reports.end(), [](const AZStd::pair<int, ReportWrapper> t) {return t.second.IsReportComplete() == true;}));
            ++iter)
        {
            availableReportIDs.push_back(iter->first);
        }

        return availableReportIDs;
    }

    DefectReport DefectReportManager::GetReport(int reportID) const
    {
        auto reportIter = m_reports.find(reportID);
        if (reportIter != m_reports.end())
        {
            return reportIter->second;
        }
        else
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: Attempt get a report ID that doesn't exist (%d) in GetReport", reportID);
            return DefectReport{ INVALID_ID };
        }
    }



    void DefectReportManager::UpdateReport(const DefectReport& report)
    {
        auto reportIter = m_reports.find(report.GetReportID());
        if (reportIter != m_reports.end())
        {
            reportIter->second.ClearReportData();
            reportIter->second.AddReportData(report.GetReportID(), report.GetMetrics(), report.GetAttachments());
        }
        else
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: Attempt update a report ID that doesn't exist (%d) in UpdateReport", report.GetReportID());
        }

    }

    void DefectReportManager::AddAnnotation(int reportID, const AZStd::string& annotation)
    {
        auto reportIter = m_reports.find(reportID);
        if (reportIter != m_reports.end())
        {
            reportIter->second.AddAnnotation(annotation);
        }
        else
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: Attempt annotate a report ID that doesn't exist (%d) in AddAnnotation", reportID);
        }
    }

    void DefectReportManager::AddCustomField(int reportID, const AZStd::string& key, const AZStd::string& value)
    {
        auto reportIter = m_reports.find(reportID);
        if (reportIter != m_reports.end())
        {
            reportIter->second.AddCustomField(key, value);
        }
        else
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: Attempt annotate a report ID that doesn't exist (%d) in AddAnnotation", reportID);
        }
    }

    void DefectReportManager::GetClientConfiguration()
    {
        AZStd::string postUrl;
        auto job = ServiceAPI::GetClientconfigurationRequestJob::Create(
            [postUrl](ServiceAPI::GetClientconfigurationRequestJob* job) mutable
        {
            EBUS_EVENT(CloudGemDefectReporter::CloudGemDefectReporterUINotificationBus, OnClientConfigurationAvailable, job->result);         
        },
        [](ServiceAPI::GetClientconfigurationRequestJob* job)
        {
            AZ_Warning("CloudCanvas", false, "Failed to load the custom fields");
        }
        );

        job->Start();
    }

    void DefectReportManager::RemoveReport(int reportID)
    {
        auto reportIter = m_reports.find(reportID);
        if (reportIter != m_reports.end())
        {
            m_reports.erase(reportIter);
        }
        else
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: Attempt remove a report ID that doesn't exist (%d) in RemoveReport", reportID);
        }
    }

    void DefectReportManager::PostReports(const AZStd::vector<int>& reportIDs)
    {
        AZStd::vector<ReportWrapper> reports;
        for (int reportId : reportIDs)
        {
            auto it = m_reports.find(reportId);
            if (it == m_reports.end())
            {
                AZ_Warning("DefectReportManager", false, "DefectReportManager: Attempt post a report ID that doesn't exist (%d) in PostReports", reportId);
                continue;
            }

            reports.emplace_back(it->second);
        }

        if (reports.size() == 0)
        {
            return;
        }
        
        AZ::JobContext* jobContext{ nullptr };
        EBUS_EVENT_RESULT(jobContext, CloudGemFramework::CloudGemFrameworkRequestBus, GetDefaultJobContext);

        auto job = aznew PostDefectReportsJob(jobContext, reports);
        job->Start();
    }

    void DefectReportManager::FlushReports(const AZStd::vector<int>& reportIDs)
    {
        AZStd::for_each(reportIDs.begin(), reportIDs.end(), [this](int reportID) {
            auto reportIter = m_reports.find(reportID);
            if (reportIter != m_reports.end())
            {
                m_reports.erase(reportIter);
            }
            else
            {
                AZ_Warning("DefectReportManager", false, "DefectReportManager: Attempt remove a report ID that doesn't exist (%d) in FlushReports", reportID);
            }
        });

        RemoveBackedUpReports();
    }

    // This class is what's actually serialized as it will contain only data needed to restore the DefectReportManager
    // to a state that it can be used. It's just a snapshot of all the complete reports when BackupCompletedReports
    // is invoked. The DefectReportManager contains lots of extra info that's not needed to load the complete reports.
    // Only complete reports are saved because this data is only used to restore reports that might have been lost
    // if a crash occurred or the user quit before sending the reports. Partial reports wouldn't make sense as they
    // would never be completed, so they aren't saved out.
    struct DefectReportSerializeData
    {
        AZ_TYPE_INFO(DefectReportSerializeData, "{77FDD5A9-A084-4B9D-93FE-AD599568BD9D}");
        AZ_CLASS_ALLOCATOR(DefectReportSerializeData, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context)
        {
            auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<DefectReportSerializeData>()
                    ->Field("reports", &DefectReportSerializeData::m_reports)
                    ->Version(1);
            }
        }

        AZStd::vector<DefectReport> m_reports;
    };

    static const char* BACKUP_FILE_PATH = "/DefectReporterBackup/defect_reporter_backup.xml";
    static const char* BACKUP_FILE_LOCTION_ALIAS = "@user@";
    AZStd::string DefectReportManager::GetBackupFileName()
    {
        if (m_backupFileName.empty())
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO)
            {
                const char* aliasLoc = fileIO->GetAlias(BACKUP_FILE_LOCTION_ALIAS);
                if (aliasLoc)
                {
                    AZStd::string retVal(aliasLoc);
                    retVal += BACKUP_FILE_PATH;
                    m_backupFileName = retVal;
                    return retVal;
                }
                else
                {
                    return{};
                }
            }
            else
            {
                return{};
            }
        }
        else
        {
            return m_backupFileName;
        }
    }

    void DefectReportManager::BackupCompletedReports()
    {
        DefectReportSerializeData serializeData;

        AZStd::string filePath = GetBackupFileName();
        if (filePath.empty())
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: File system unavailble so report backup skipped");
            return;
        }

        if (!m_reports.empty())
        {
            for (auto iter(m_reports.begin());
                m_reports.end() != (iter = AZStd::find_if(iter, m_reports.end(), [](const AZStd::pair<int, ReportWrapper> t) {return t.second.IsReportComplete() == true; }));
                ++iter)
            {
                // it's not useful to persist the handler data because the reports that are saved are complete
                // and handlers ID's are there to track if reports are complete
                // also, there's no need to keep the report ID's as they will be re-assigned when loaded
                
                DefectReport unwrappedReport(0, iter->second.GetMetrics(), iter->second.GetAttachments());
                serializeData.m_reports.push_back(unwrappedReport);
            }

            AZStd::string backupPath(filePath + ".restore");
            auto fileBase = AZ::IO::FileIOBase::GetInstance();
            if (fileBase)
            {
                // make a backup of the backup, just in case the serialized files gets messed up
                fileBase->Copy(filePath.c_str(), backupPath.c_str());
            
                // remove the existing backup
                RemoveBackedUpReports();
            }


            if (!AZ::Utils::SaveObjectToFile(filePath, AZ::DataStream::StreamType::ST_XML, &serializeData))
            {
                AZ_Warning("DefectReportManager", false, "DefectReportManager: Unable to back up reports");
                return;
            }

            // if all went well, delete the restored file
            if (fileBase)
            {
                fileBase->Remove(backupPath.c_str());
            }
        }
    }

    void DefectReportManager::LoadBackedUpReports()
    {
        AZStd::string filePath = GetBackupFileName();
        if (filePath.empty())
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: File system unavailble so report backup loading skipped");
            return;
        }

        auto fileBase = AZ::IO::FileIOBase::GetInstance();
        if (fileBase)
        {
            if (!fileBase->Exists(filePath.c_str()))
            {
                // no file to load, so just bail
                return;
            }
        }
        else
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: FileIOBase doesn't exist, can't check if back up file exists");
            return;
        }

        DefectReportSerializeData serializeData;

        if (!AZ::Utils::LoadObjectFromFileInPlace(filePath, serializeData))
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: Unable to load backed up report");
            return;
        }

        m_reports.clear();

        // we don't care what the report ID's were before, they are just for organization so we can renumber them sequentially
        // now to avoid having to deal with "holes" in the ID's.
        m_nextReportID = serializeData.m_reports.size();

        for (size_t reportID = 0; reportID < m_nextReportID; ++reportID)
        {
            serializeData.m_reports[reportID].SetReportID(reportID);
            ReportWrapper wrappedReport(serializeData.m_reports[reportID]);
            m_reports[reportID] = serializeData.m_reports[reportID];
        }
    }

    void DefectReportManager::RemoveBackedUpReports()
    {
        AZStd::string filePath = GetBackupFileName();
        if (filePath.empty())
        {
            AZ_Warning("DefectReportManager", false, "DefectReportManager: File system unavailble so unable to remove any existing report backup file");
            return;
        }

        auto fileBase = AZ::IO::FileIOBase::GetInstance();
        if (fileBase)
        {
            fileBase->Remove(filePath.c_str());
        }
    }

    void DefectReportManager::ReflectDataStructures(AZ::ReflectContext* context)
    {
        DefectReportSerializeData::Reflect(context);
    }

}

