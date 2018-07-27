
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

#include <CloudGemDefectReporter/CloudGemDefectReporterBus.h>
#include <CloudGemDefectReporter/DefectReporterDataStructures.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>

#include "AWS/ServiceApi/CloudGemDefectReporterClientComponent.h"

namespace CloudGemDefectReporter
{
    /*namespace ServiceAPI
    {
        struct GetClientconfigurationRequest;
        struct CloudGemDefectReporterRequestBus;
    }*/

    class ReportWrapper : public DefectReport
    {
    public:
        ReportWrapper() :
            DefectReport()
        {}

        ReportWrapper(int reportID) :
            DefectReport(reportID)
        {}

        ReportWrapper(const DefectReport& report);

        int GetNextHandlerID() { return m_nextHandlerID++; }
        void RegisterHandlerID(int id);
        void MarkHandlerReceived(int id);
        bool IsReportComplete() const;
        void ClearHandlers();

        void AddReportData(int handlerID,
            AZStd::vector<MetricDesc> metrics,
            AZStd::vector<AttachmentDesc> attachments);
        void AddAnnotation(const AZStd::string& annotation);
        void AddCustomField(const AZStd::string& key, const AZStd::string& value);
        void ClearReportData();

    private:
        int m_nextHandlerID = 0;
        AZStd::map<int, bool> m_dataReceived;
    };

    class DefectReportManager
    {
    public:
        DefectReportManager();
        ~DefectReportManager();
        
        // adds new report and returns the ID
        int StartNewReport();
        
        // gets the next handler ID to use for this report
        int GetNextHandlerID(int reportID);

        // adds new data to the report
        void ReportData(int reportID,
            int handlerID,
            AZStd::vector<MetricDesc> metrics,
            AZStd::vector<AttachmentDesc> attachments);

        // test if any reports are available
        int CountCompleteReports() const;

        // get the number of reports pending and not yet available
        int CountPendingReports() const;

        int CountAttachments() const;

        // gets the report ID's of all the reports that are ready
        AZStd::vector<int> GetAvailiableReportIDs() const;

        // check if an individual report is ready
        bool IsReportComplete(int reportID);

        // get a report
        DefectReport GetReport(int reportID) const;

        // update a report
        void UpdateReport(const DefectReport& report);

        void AddAnnotation(int reportID, const AZStd::string& annotation);

        // get the custom client configuration
        void GetClientConfiguration();

        // add a new custom field to the metrics
        void AddCustomField(int reportID, const AZStd::string& key, const AZStd::string& value);

        // delete a report
        void RemoveReport(int reportID);

        // send available reports and attachments to AWS
        void PostReports(const AZStd::vector<int>& reportIDs);

        // remove the available reports (typically after posting)
        void FlushReports(const AZStd::vector<int>& reportIDs);

        // serialize the complete reports to disk so they can be loaded if there
        // is a crash or the user quits before processing reports
        void BackupCompletedReports();

        // this will initialize the reports with backup data. Note it will
        // remove any existing reports and change the report ID so should
        // really only be called on initialization
        void LoadBackedUpReports();

        // just deletes the cached file
        void RemoveBackedUpReports();

        // used during initialization, shouldn't be called otherwise
        static void ReflectDataStructures(AZ::ReflectContext* context);

    private:

        AZStd::string GetBackupFileName();

        AZStd::map<int, ReportWrapper> m_reports;
        int m_nextReportID = 0;
        AZStd::string m_backupFileName;
    };
}
