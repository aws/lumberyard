
#include "CloudGemDefectReporter_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "CloudGemDefectReporterSystemComponent.h"

#include <IConsole.h>
#include <ISystem.h>
#include <CrySystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AZCore/JSON/stringbuffer.h>
#include <AZCore/JSON/writer.h>

namespace CloudGemDefectReporter
{
    static void ConsoleCommandFpsDrop(ICVar* CVar)
    {
        bool enabled = CVar->GetIVal() != 0;
        CloudGemDefectReporterFPSDropReportingRequestBus::Broadcast(&CloudGemDefectReporterFPSDropReportingRequestBus::Events::Enable, enabled);
    }

    static void ConsoleCommandCaptureDefectReport(IConsoleCmdArgs* pCmdArgs)
    {
        bool immedate = false;
        
        if (pCmdArgs->GetArgCount() == 2)
        {
            if (AzFramework::StringFunc::Equal(pCmdArgs->GetArg(1), "immediate"))
            {
                immedate = true;
            }
        }
        CloudGemDefectReporterRequestBus::Broadcast(&CloudGemDefectReporterRequestBus::Events::TriggerDefectReport, immedate);
    }

    static void ConsoleCommandTriggerUserDefectReportEditing(IConsoleCmdArgs* pCmdArgs)
    {
        CloudGemDefectReporterRequestBus::Broadcast(&CloudGemDefectReporterRequestBus::Events::TriggerUserReportEditing);
    }

    // Behavior context reflection of CloudGemDefectReporterUINotifications
    class CloudGemDefectReporterUINotificationsBehaviorHandler :
        public CloudGemDefectReporterUINotificationBus::Handler,
        public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(CloudGemDefectReporterUINotificationsBehaviorHandler
            , "{AE8DCB83-1EC7-4927-A4FE-ACBA15FC24AA}"
            , AZ::SystemAllocator
            , OnOpenDefectReportEditorUI
            , OnNewReportTriggered
            , OnNewReportReady
            , OnReportsUpdated
            , OnDefectReportPostStatus
            , OnDefectReportPostError
            , OnClientConfigurationAvailable
            , OnSubmittingReport
            , OnReachSoftCap

        );

        void OnOpenDefectReportEditorUI() override
        {
            Call(FN_OnOpenDefectReportEditorUI);
        }

        void OnNewReportTriggered()
        {
            Call(FN_OnNewReportTriggered);
        }

        void OnNewReportReady()
        {
            Call(FN_OnNewReportReady);
        }

        void OnReportsUpdated(int totalAvailableReports, int totalPending)
        {
            Call(FN_OnReportsUpdated, totalAvailableReports, totalPending);
        }

        void OnDefectReportPostStatus(int currentReport, int totalReports) override
        {
            Call(FN_OnDefectReportPostStatus, currentReport, totalReports);
        }

        void OnDefectReportPostError(const AZStd::string& error) override
        {
            Call(FN_OnDefectReportPostError, error);
        }

        void OnClientConfigurationAvailable(const ServiceAPI::ClientConfiguration& clientConfiguration) override
        {
            Call(FN_OnClientConfigurationAvailable, clientConfiguration);
        }

        void OnSubmittingReport(const bool& status) override
        {
            Call(FN_OnSubmittingReport, status);
        }

        void OnReachSoftCap() override
        {
            Call(FN_OnReachSoftCap);
        }
    };

    // Behavior context reflection of CloudGemDefectReporterNotifications
    class CloudGemDefectReporterNotificationsBehaviorHandler :
        public CloudGemDefectReporterNotificationBus::Handler,
        public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(CloudGemDefectReporterNotificationsBehaviorHandler
            , "{B8503186-ABC9-48AB-B7C0-77E847EA4962}"
            , AZ::SystemAllocator
            , OnCollectDefectReporterData
            , OnDefectReportUploaded
        );

        void OnCollectDefectReporterData(int reportID) override
        {
            Call(FN_OnCollectDefectReporterData, reportID);
        }

        virtual void OnDefectReportUploaded(int reportID) override
        {
            Call(FN_OnDefectReportUploaded, reportID);
        }
    };

    void AttachmentDesc::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AttachmentDesc>()
                ->Field("name", &AttachmentDesc::m_name)
                ->Field("type", &AttachmentDesc::m_type)
                ->Field("path", &AttachmentDesc::m_path)
                ->Field("extension", &AttachmentDesc::m_extension)
                ->Field("autoDelete", &AttachmentDesc::m_autoDelete)
                ->Version(1);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AttachmentDesc>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("name", BehaviorValueGetter(&AttachmentDesc::m_name), nullptr)
                ->Property("type", BehaviorValueGetter(&AttachmentDesc::m_type), nullptr)
                ->Property("path", BehaviorValueGetter(&AttachmentDesc::m_path), nullptr)
                ->Property("extension", BehaviorValueGetter(&AttachmentDesc::m_extension), nullptr)
                ->Property("autoDelete", BehaviorValueGetter(&AttachmentDesc::m_autoDelete), nullptr)
                ;
        }
    }

    void MetricDesc::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<MetricDesc>()
                ->Field("key", &MetricDesc::m_key)
                ->Field("data", &MetricDesc::m_data)
                ->Version(1);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MetricDesc>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("key", BehaviorValueGetter(&MetricDesc::m_key), nullptr)
                ->Property("data", BehaviorValueGetter(&MetricDesc::m_data), nullptr)
                ;
        }
    }

    void DefectReport::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DefectReport>()
                ->Field("reportID", &DefectReport::m_reportID)
                ->Field("metrics", &DefectReport::m_metrics)
                ->Field("attachments", &DefectReport::m_attachments)
                ->Version(1);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DefectReport>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("reportID", BehaviorValueGetter(&DefectReport::m_reportID), nullptr)
                ->Property("metrics", BehaviorValueGetter(&DefectReport::m_metrics), nullptr)
                ->Property("attachments", BehaviorValueGetter(&DefectReport::m_attachments), nullptr)
                ;
        }
    }


    void CloudGemDefectReporterSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AttachmentDesc::Reflect(context);
        MetricDesc::Reflect(context);
        DefectReport::Reflect(context);
        DefectReportManager::ReflectDataStructures(context);

        ServiceAPI::CustomField::Reflect(context);
        ServiceAPI::ClientConfiguration::Reflect(context);
        
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemDefectReporterSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemDefectReporterSystemComponent>("CloudGemDefectReporter", "Allows the user to create defect reports that automatically capture data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
        
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<CloudGemDefectReporterNotificationBus>("CloudGemDefectReporterNotificationBus")
                ->Handler<CloudGemDefectReporterNotificationsBehaviorHandler>();

            behaviorContext->EBus<CloudGemDefectReporterUINotificationBus>("CloudGemDefectReporterUINotificationBus")
                ->Handler<CloudGemDefectReporterUINotificationsBehaviorHandler>();
        
            behaviorContext->EBus<CloudGemDefectReporterRequestBus>("CloudGemDefectReporterRequestBus")
                ->Event("TriggerDefectReport", &CloudGemDefectReporterRequestBus::Events::TriggerDefectReport)
                ->Event("GetHandlerID", &CloudGemDefectReporterRequestBus::Events::GetHandlerID)
                ->Event("ReportData", &CloudGemDefectReporterRequestBus::Events::ReportData)
                ->Event("TriggerUserReportEditing", &CloudGemDefectReporterRequestBus::Events::TriggerUserReportEditing)
                ->Event("GetAvailableReportIDs", &CloudGemDefectReporterRequestBus::Events::GetAvailableReportIDs)
                ->Event("GetReport", &CloudGemDefectReporterRequestBus::Events::GetReport)
                ->Event("UpdateReport", &CloudGemDefectReporterRequestBus::Events::UpdateReport)
                ->Event("AddAnnotation", &CloudGemDefectReporterRequestBus::Events::AddAnnotation)
                ->Event("AddCustomField", &CloudGemDefectReporterRequestBus::Events::AddCustomField)
                ->Event("RemoveReport", &CloudGemDefectReporterRequestBus::Events::RemoveReport)
                ->Event("PostReports", &CloudGemDefectReporterRequestBus::Events::PostReports)
                ->Event("FlushReports", &CloudGemDefectReporterRequestBus::Events::FlushReports)
                ->Event("GetInputRecord", &CloudGemDefectReporterRequestBus::Events::GetInputRecord)
                ->Event("GetClientConfiguration", &CloudGemDefectReporterRequestBus::Events::GetClientConfiguration)
                ->Event("IsSubmittingReport", &CloudGemDefectReporterRequestBus::Events::IsSubmittingReport);
        }
    }

    void CloudGemDefectReporterSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemDefectReporterService"));
    }

    void CloudGemDefectReporterSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemDefectReporterService"));
    }

    void CloudGemDefectReporterSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudGemDefectReporterSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemDefectReporterSystemComponent::Init()
    {
    }

    void CloudGemDefectReporterSystemComponent::Activate()
    {
        CloudGemDefectReporterRequestBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
        CloudGemDefectReporterNotificationBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void CloudGemDefectReporterSystemComponent::Deactivate()
    {
        CloudGemDefectReporterRequestBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        CloudGemDefectReporterNotificationBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void CloudGemDefectReporterSystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& params)
    {
        system.GetIConsole()->RegisterInt("cc_defect_report_capture_fps_drop", 1, VF_NULL, "Automatically generate defect report when FPS drop", ConsoleCommandFpsDrop);
        system.GetIConsole()->AddCommand("cc_defect_report_capture", ConsoleCommandCaptureDefectReport);
        system.GetIConsole()->AddCommand("cc_defect_report_trigger_ui", ConsoleCommandTriggerUserDefectReportEditing);

        // asset aliases haven't been initialized until here so we can't attempt to load until these are resolved
        m_reportManager.LoadBackedUpReports();
    }

    int CloudGemDefectReporterSystemComponent::TriggerDefectReport(bool immediate)
    {
        if (CheckSoftCap())
        {
            return 0;
        }

        int currentReportID = m_reportManager.StartNewReport();
        CloudGemDefectReporterNotificationBus::Broadcast(&CloudGemDefectReporterNotificationBus::Events::OnCollectDefectReporterData, currentReportID);

        int availableReports = m_reportManager.CountCompleteReports();
        int pendingReports = max(m_reportManager.CountPendingReports(), 1);
        CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnReportsUpdated, availableReports, pendingReports);
        CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnNewReportTriggered);

        return currentReportID;
    }

    // Check whether the number of the pre-signed post calls to submit all the attachments exceeds the soft cap limitation
    bool CloudGemDefectReporterSystemComponent::CheckSoftCap()
    {
        int numAttachments = m_reportManager.CountAttachments();
        if (numAttachments >= s_maxNumAttachmentsToSubmit)
        {
            CloudGemDefectReporterUINotificationBus::Broadcast(&CloudGemDefectReporterUINotificationBus::Events::OnReachSoftCap);
            return true;
        }
        
        return false;
    }

    int CloudGemDefectReporterSystemComponent::GetHandlerID(int reportID)
    {
        return m_reportManager.GetNextHandlerID(reportID);
    }

    void CloudGemDefectReporterSystemComponent::ReportData(int reportID,
        int handlerID,
        AZStd::vector<MetricDesc> metrics,
        AZStd::vector<AttachmentDesc> attachments)
    {
        m_reportManager.ReportData(reportID, handlerID, metrics, attachments);
        // every time we get a complete report, save the reports to disk
        if (m_reportManager.IsReportComplete(reportID))
        {
            CheckSoftCap();
            CloudGemDefectReporterRequestBus::QueueBroadcast(&CloudGemDefectReporterRequestBus::Events::BackupCompletedReports);
        }
    }

    void CloudGemDefectReporterSystemComponent::TriggerUserReportEditing()
    {
        CloudGemDefectReporterUINotificationBus::Broadcast(&CloudGemDefectReporterUINotificationBus::Events::OnOpenDefectReportEditorUI);
    }

    AZStd::vector<int> CloudGemDefectReporterSystemComponent::GetAvailableReportIDs()
    {
        return m_reportManager.GetAvailiableReportIDs();
    }

    CloudGemDefectReporter::DefectReport CloudGemDefectReporterSystemComponent::GetReport(int reportID)
    {
        return m_reportManager.GetReport(reportID);
    }

    void CloudGemDefectReporterSystemComponent::UpdateReport(DefectReport report)
    {
        m_reportManager.UpdateReport(report);
        CloudGemDefectReporterRequestBus::QueueBroadcast(&CloudGemDefectReporterRequestBus::Events::BackupCompletedReports);
    }

    void  CloudGemDefectReporterSystemComponent::AddAnnotation(int reportID, AZStd::string annotation)
    {
        m_reportManager.AddAnnotation(reportID, annotation);
        CloudGemDefectReporterRequestBus::QueueBroadcast(&CloudGemDefectReporterRequestBus::Events::BackupCompletedReports);
    }
    
    void  CloudGemDefectReporterSystemComponent::GetClientConfiguration()
    {
        m_reportManager.GetClientConfiguration();
    }

    AZStd::string CloudGemDefectReporterSystemComponent::GetInputRecord(AZStd::string processedEventName)
    {
        AZ::EditableInputRecords outResults;
        EBUS_EVENT(AZ::GlobalInputRecordRequestBus, GatherEditableInputRecords, outResults);

        auto foundIter = AZStd::find_if(outResults.begin(), outResults.end(), [processedEventName](const AZ::EditableInputRecord& inputRecord) { return inputRecord.m_eventGroup == Input::ProcessedEventName(processedEventName.c_str()); });
        if (foundIter != outResults.end())
        {
            return foundIter->m_inputName;
        }

        return "";
    }

    void  CloudGemDefectReporterSystemComponent::AddCustomField(int reportID, AZStd::string key, AZStd::string value)
    {
        m_reportManager.AddCustomField(reportID, key, value);
    }

    void CloudGemDefectReporterSystemComponent::RemoveReport(int reportID)
    {
        m_reportManager.RemoveReport(reportID);
        int availableReports = m_reportManager.CountCompleteReports();
        int pendingReports = m_reportManager.CountPendingReports();
        CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnReportsUpdated, availableReports, pendingReports);
    }

    void CloudGemDefectReporterSystemComponent::PostReports(AZStd::vector<int> reportIDs)
    {
        m_reportManager.PostReports(reportIDs);
    }

    void CloudGemDefectReporterSystemComponent::FlushReports(AZStd::vector<int> reportIDs)
    {
        m_reportManager.FlushReports(reportIDs);
        int availableReports = m_reportManager.CountCompleteReports();
        int pendingReports = m_reportManager.CountPendingReports();
        CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnReportsUpdated, availableReports, pendingReports);
    }

    void CloudGemDefectReporterSystemComponent::BackupCompletedReports()
    {
        m_reportManager.BackupCompletedReports();
    }
    void CloudGemDefectReporterSystemComponent::OnSystemTick()
    {
        CloudGemDefectReporterRequestBus::ExecuteQueuedEvents();
        int availableReports = m_reportManager.CountCompleteReports();
        int pendingReports = m_reportManager.CountPendingReports();

        if (availableReports != m_prevAvailableReports || pendingReports != m_prevPendingReports)
        {
            if (availableReports > m_prevAvailableReports)
            {
                CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnNewReportReady);
            }

            m_prevAvailableReports = availableReports;
            m_prevPendingReports = pendingReports;
            CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnReportsUpdated, availableReports, pendingReports);
        }
        CloudGemDefectReporterUINotificationBus::ExecuteQueuedEvents();

        if (pendingScreenShots.size() > 0)
		{
            ISystem* system = nullptr;
            CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);

            if (system)
            {
                IConsole* console = system->GetIConsole();
                ICVar* ssVar = console->GetCVar("e_ScreenShot");
                if (ssVar && ssVar->GetIVal() == 0)
                {
                    AZStd::string ssFilename;
                    ICVar* ssFilenameVar = console->GetCVar("e_ScreenShotFileName");

                    updateScreenShotSettings();

                    if (ssFilenameVar && ssVar)
                    {
                        ssFilename = pendingScreenShots[0];
                        ssFilenameVar->Set(ssFilename.c_str());
                        const char* captureScreenShot = "1";
                        ssVar->ForceSet(captureScreenShot); // standard hi res screen shot, enums defined in 3DEngineRender.cpp so not accessible here
                        pendingScreenShots.erase(pendingScreenShots.begin());
                    }
                }
            }
        }
    }


    void CloudGemDefectReporterSystemComponent::OnCollectDefectReporterData(int reportID)
    {
        int handlerID = CloudGemDefectReporter::INVALID_ID;
        CloudGemDefectReporterRequestBus::BroadcastResult(handlerID, &CloudGemDefectReporterRequestBus::Events::GetHandlerID, reportID);

        ISystem* system = nullptr;
        CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);

        AZStd::string ssFilename;

        if (system)
        {
            IConsole* console = system->GetIConsole();
            ICVar* ssFilenameVar = console->GetCVar("e_ScreenShotFileName");
            ICVar* ssVar = console->GetCVar("e_ScreenShot");

            updateScreenShotSettings();

            if (ssFilenameVar && ssVar)
            {
                ssFilename = "DefectReporter";
                ssFilename += AZ_CORRECT_FILESYSTEM_SEPARATOR;
                ssFilename += AZStd::string::format("defectScreenShot%s", AZ::Uuid::Create().ToString<AZStd::string>().c_str());

                if (ssVar->GetIVal() == 1)
                {
                    pendingScreenShots.push_back(ssFilename);
                }
                else
                {
                    ssFilenameVar->Set(ssFilename.c_str());
                    const char* captureScreenShot = "1";
                    ssVar->ForceSet(captureScreenShot);  // standard hi res screen shot, enums defined in 3DEngineRender.cpp so not accessible here
                }
                // the screenshot file name cvar requires only the relative path so add the rest of the path after
                ssFilename = AZStd::string::format("@user@/ScreenShots/%s.%s", ssFilename.c_str(), "jpg");
            }
        }

        AttachmentDesc attachmentDesc;
        attachmentDesc.m_name = "screenshot";
        attachmentDesc.m_autoDelete = false;
        attachmentDesc.m_type = "image/jpeg";
        attachmentDesc.m_extension = "jpg";

        attachmentDesc.m_path = ssFilename;

        DefectReport::AttachmentList attachments;
        attachments.push_back(attachmentDesc);

        CloudGemDefectReporterRequestBus::Broadcast(&CloudGemDefectReporterRequestBus::Events::ReportData,
            reportID,
            handlerID,
            DefectReport::MetricsList{},
            attachments);

    }

    void CloudGemDefectReporterSystemComponent::updateScreenShotSettings() {
        ISystem* system = nullptr;
        CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);

        int ssWidth = 0;
        int ssHeight = 0;

        if (system)
        {
            IConsole* console = system->GetIConsole();
            // get the current dimensions of the screen shot. Then, if we are able to get the 
            // current dimensions of the actual screen set the screen shot size to that
            // as it defaults to a strange value
            ICVar* ssWidthVar = console->GetCVar("e_ScreenShotWidth");
            if (ssWidthVar)
            {
                ssWidth = ssWidthVar->GetIVal();

                ICVar* screenWidthVar = console->GetCVar("r_width");
                if (screenWidthVar)
                {
                    ssWidth = screenWidthVar->GetIVal();
                    ssWidthVar->Set(ssWidth);
                }
            }
            ICVar* ssHeightVar = console->GetCVar("e_ScreenShotHeight");
            if (ssHeightVar)
            {
                ssHeight = ssHeightVar->GetIVal();
                ICVar* screenHeightVar = console->GetCVar("r_height");
                if (screenHeightVar)
                {
                    ssHeight = screenHeightVar->GetIVal();
                    ssHeightVar->Set(ssHeight);
                }
            }
            ICVar* ssFormatVar = console->GetCVar("e_ScreenShotFileFormat");
            if (ssFormatVar)
            {
                ssFormatVar->Set("jpg");
            }
        }
    }

    void CloudGemDefectReporterSystemComponent::IsSubmittingReport(bool status)
    {
        EBUS_EVENT(CloudGemDefectReporterUINotificationBus, OnSubmittingReport, status);
    }
}

