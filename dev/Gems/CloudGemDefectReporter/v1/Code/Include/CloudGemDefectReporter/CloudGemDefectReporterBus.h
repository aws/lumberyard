/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
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
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

#include "DefectReporterDataStructures.h"

namespace CloudGemDefectReporter
{
    namespace ServiceAPI
    {
        struct ClientConfiguration;
    }

    // Request EBus for the defect reporter gem
    class CloudGemDefectReporterRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        static const bool EnableEventQueue = true;
        //////////////////////////////////////////////////////////////////////////

        // entry point for the defect reporter when you want it to gather data from the handlers.
        //     bool immediate - determine if the report dialog shows immediately or the report data 
        //     is queued for later annotation
        virtual int TriggerDefectReport(bool immediate) = 0;
        
        // called by a handler, and returns a unique ID for the handler and registers the intention
        // of this handler to report data at some point.MUST BE CALLED BEFORE ReportData
        // This should be called in the main thread if possible as ordering issues can occur if it is queued
        //    int reportID - the ID of the report this data belongs to, passed in from 
        //    CollectDefectReporterData.
        virtual int GetHandlerID(int reportID) = 0;
        
        // Called by a handler when it has gathered necessary data and is ready to add it to the
        // report. Note that this must always be called with QueueBroadcast as many reports
        // will come from different threads and ordering issues can arise if it's not queued
        //    int reportID - the report ID which this data will be assigned to
        //    int handlerID - the ID of this handler passed back by GetHandlerID
        //    vector<MetricDesc> metric - all the metrics attributes to record for the report
        //    vector<AttachmentDesc> attachmentPaths - descriptions of all the attachments to be
        //      sent with the report
        virtual void ReportData(int reportID, 
            int handlerID,
            AZStd::vector<MetricDesc> metrics,
            AZStd::vector<AttachmentDesc> attachments) = 0;
        
        // request the defect reporter UI be raised to allow the user to edit and annotate the 
        // current report queue.
        virtual void TriggerUserReportEditing() = 0;
        
        // gets currently available reports
        // available reports are the set of reports that have complete information
        // QueueBroadcast to be sure you the correct current state where all reports
        // would be resolved by the time the main thread is called
        virtual AZStd::vector<int> GetAvailableReportIDs() = 0;

        // gets a specified report
        virtual DefectReport GetReport(int reportID) = 0;

        // generally called by the UI to send an updated version of a defect report in to the 
        // report queue, could called by other code if desired. diffs will be detected and updated
        // in the queued report
        virtual void UpdateReport(DefectReport report) = 0;

        // add an annotation to a report
        virtual void AddAnnotation(int reportID, AZStd::string annotation) = 0;

        // gets the input record of a specific event
        virtual AZStd::string GetInputRecord(AZStd::string processedEventName) = 0;

        // gets a specified report
        virtual void GetClientConfiguration() = 0;

        // add defect type to a report
        virtual void AddCustomField(int reportID, AZStd::string key, AZStd::string value) = 0;

        // generally called by the UI to remove a specific queued report        
        virtual void RemoveReport(int reportID) = 0;

        // upload reports in the list provided, typically by the UI to report which
        // reports were edited by the user
        virtual void PostReports(AZStd::vector<int> reportIDs) = 0;
        
        // remove reports in the list provided
        virtual void FlushReports(AZStd::vector<int> reportIDs) = 0;

        // notify when an attachment has been uploaded to allow it to be added
        // for deletion if needed or other processing
        virtual void AttachmentUploadComplete(AZStd::string attachmentPath, bool autoDelete) = 0;

        // save completed reports to disk to allow recovery of report after crash
        virtual void BackupCompletedReports() = 0;

        // enable or disable the keyboard input
        virtual void IsSubmittingReport(bool status) = 0;
    };
    using CloudGemDefectReporterRequestBus = AZ::EBus<CloudGemDefectReporterRequests>;

    // Handler EBus for code that wishes to report info for the defect report
    // Report Data Request Mechanism
    //    Since the data reporting handler may need time to gather the needed data(for example it 
    //    needs to call an external program like dxdiag, or make an AWS request for information) 
    //    it's assumed that the data will be returned to the defect reporter with latency. As
    //    such, the initial request for data from the handler will return immediately, registering
    //    its intent to send data to the defect reporter. The handler must ask the defect reporter
    //    for an ID which will be returned from the initial request so that the defect reporter can
    //    keep track of the request. The ID will be sent along with the data from the handler to
    //    the defect reporter when it is available, and the data will be added to the appropriate 
    //    report.
    //
    // Sequence for a handler to report data
    //    1. Defect reporter creates a "report accumulator" that will be responsible for accepting
    //       data for the report as it comes in.
    //    2. Defect reporter sends a CollectDefectReporterData message to all handlers which
    //       includes the report ID.
    //    3. Handler calls GetHandlerID on defect reporter using the reportID and stores both ID's
    //       for later use. When GetHandlerID is called, the defect reporter "opens a slot" in the 
    //       report accumulator for the report so it knows to wait for the report before finalizing.
    //    4. Handler initiates data gathering.Please note this MUST happen after GetHandlerID so 
    //       the latent call can't beat the handler ID being registered or if data is recorded 
    //       immediately.
    //    5. Handler returns from CollectDefectReporterData.
    //    6. When handler has data ready, it calls ReportData on the DefectReporter with the report
    //       ID, the handler ID and the data appropriate for the request.The author of the handler
    //       should make sure there is a mechanism to match the report ID and handler ID to the
    //       correct data.Latent data gathering functions may want to send these as user values(or
    //       other metadata passing mechanism) if possible, or come up with their own queuing
    //       mechanism to match incoming data to the report and handler ID's.
    class CloudGemDefectReporterNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // request the handler start collecting defect data and report it back to the defect
        // reporter (see sequence above for a handler to report data)
        virtual void OnCollectDefectReporterData(int reportID) {}

        // notify the handler that the report has been uploaded in case the handler needs to do
        // any manual cleanup of report artifacts
        virtual void OnDefectReportUploaded(int reportID) {}
    };
    using CloudGemDefectReporterNotificationBus = AZ::EBus<CloudGemDefectReporterNotifications>;

    // UI handler EBus for processing defect reporter UI requests
    // This abstracts the defect reporter from the UI so the user can supply their own implementation
    class CloudGemDefectReporterUINotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        static const bool EnableEventQueue = true;
        //////////////////////////////////////////////////////////////////////////
    
        // request the user supplied UI open and allow report editing and annotation
        virtual void OnOpenDefectReportEditorUI() {}

        // called when the user has initiated a new report
        virtual void OnNewReportTriggered() {}

        // called when all the information in a report has been gathered
        virtual void OnNewReportReady() {}

        // called when the set of reports has been updated, allowing UI to respond
        virtual void OnReportsUpdated(int totalAvailableReports, int totalPending) {}

        // called when posting reports starts and tells how many steps there will be in the post
        // this allows a progress indicator to show the current steps
        // Also indicates that another post shouldn't start until OnDefectReportPostEnd is called
        virtual void OnDefectReportPostStart(int numberOfSteps) {}
        
        // called when the defect report post process has completed a step
        // step number is not sent as these are latent calls and it can't be tracked
        // ui should track and compare to the number of steps passed in to OnDefectReportPostStart
        // the report is complete when the final step has been posted
        virtual void OnDefectReportPostStep() {}
        
        //called if there is an error attempting to upload the reports
        virtual void OnDefectReportPostError(const AZStd::string& error) {}

        //called when the custom client configuration is available
        virtual void OnClientConfigurationAvailable(const ServiceAPI::ClientConfiguration& clientConfiguration) {}

        //called when the keyboard input is enabled or disabled
        virtual void OnSubmittingReport(const bool& status) {}

        //called when the soft cap is reached
        virtual void OnReachSoftCap() {}
    };
    using CloudGemDefectReporterUINotificationBus = AZ::EBus<CloudGemDefectReporterUINotifications>;

    // EBus for FPSDropReportingComponent
    class CloudGemDefectReporterFPSDropReportingRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // set max number of FPS drop reports that can be queued
        virtual void SetMaxQueuedFPSDropReports(int max) = 0;

        // get max number of FPS drop reports that can be queued
        virtual int GetMaxQueuedFPSDropReports() = 0;

        // set threshold for frame time drop in seconds to trigger FPS drop defect report 
        virtual void SetFrameTimeDropThresholdInSeconds(float threshold) = 0;

        // get threshold for frame time drop in seconds to trigger FPS drop defect report 
        virtual float GetFrameTimeDropThresholdInSeconds() = 0;

        // enable/disable auto FPS drop reporting
        virtual void Enable(bool enable) = 0;

        // get enabled flag
        virtual bool IsEnabled() = 0;
    };
    using CloudGemDefectReporterFPSDropReportingRequestBus = AZ::EBus<CloudGemDefectReporterFPSDropReportingRequests>;
} // namespace CloudGemDefectReporter
