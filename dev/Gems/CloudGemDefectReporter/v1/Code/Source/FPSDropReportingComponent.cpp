
#include "CloudGemDefectReporter_precompiled.h"

#include <FPSDropReportingComponent.h>
#include "DefectReportManager.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <ISystem.h>
#include <ITimer.h>

namespace CloudGemDefectReporter
{

    FPSDropReportingComponent::FPSDropReportingComponent()
    {

    }

    void FPSDropReportingComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<FPSDropReportingComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<FPSDropReportingComponent>("FPSDropReportingComponent", "Triggers defect reports collection when detecting FPS drop")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<CloudGemDefectReporterFPSDropReportingRequestBus>("CloudGemDefectReporterFPSDropReportingRequestBus")
                ->Event("SetMaxQueuedFPSDropReports", &CloudGemDefectReporterFPSDropReportingRequestBus::Events::SetMaxQueuedFPSDropReports)
                ->Event("GetMaxQueuedFPSDropReports", &CloudGemDefectReporterFPSDropReportingRequestBus::Events::GetMaxQueuedFPSDropReports)
                ->Event("SetFrameTimeDropThresholdInSeconds", &CloudGemDefectReporterFPSDropReportingRequestBus::Events::SetFrameTimeDropThresholdInSeconds)
                ->Event("GetFrameTimeDropThresholdInSeconds", &CloudGemDefectReporterFPSDropReportingRequestBus::Events::GetFrameTimeDropThresholdInSeconds)
                ->Event("Enable", &CloudGemDefectReporterFPSDropReportingRequestBus::Events::Enable)
                ->Event("IsEnabled", &CloudGemDefectReporterFPSDropReportingRequestBus::Events::IsEnabled);
        }
    }

    void FPSDropReportingComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemDefectReporterFPSDropReportingService"));
    }


    void FPSDropReportingComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void FPSDropReportingComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void FPSDropReportingComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("FPSDropReportingComponent"));
    }

    void FPSDropReportingComponent::Init()
    {

    }

    void FPSDropReportingComponent::Activate()
    {   
        CloudGemDefectReporterFPSDropReportingRequestBus::Handler::BusConnect();
        CloudGemDefectReporterNotificationBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void FPSDropReportingComponent::Deactivate()
    {
        CloudGemDefectReporterFPSDropReportingRequestBus::Handler::BusDisconnect();
        CloudGemDefectReporterNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    bool FPSDropReportingComponent::DetectFPSDrop(float frameTimeInSeconds)
    {
        if (m_enabled == false ||
            m_lastframeTimeInSeconds == -1 ||
            m_queuedReportIdSet.size() >= m_maxQueuedFPSDropReports)
        {            
            return false;
        }

        if (frameTimeInSeconds - m_lastframeTimeInSeconds >= m_frameTimeDropThreasholdInSeconds)
        {            
            return true;
        }
        else
        {
            return false;
        }        
    }

    void FPSDropReportingComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        float frameTimeInSeconds = gEnv->pTimer->GetRealFrameTime();

        if (DetectFPSDrop(frameTimeInSeconds))
        {
            int reportId = CloudGemDefectReporter::INVALID_ID;
            EBUS_EVENT_RESULT(reportId, CloudGemDefectReporter::CloudGemDefectReporterRequestBus, TriggerDefectReport, true);

            int handlerId = CloudGemDefectReporter::INVALID_ID;
            CloudGemDefectReporterRequestBus::BroadcastResult(handlerId, &CloudGemDefectReporterRequestBus::Events::GetHandlerID, reportId);

            AZStd::string message = AZStd::string::format("Auto generated FPS drop defect report. Frame time dropped from %f to %f seconds.", m_lastframeTimeInSeconds, frameTimeInSeconds);

            AZStd::vector<MetricDesc> metrics;
            {
                MetricDesc metricDesc;
                metricDesc.m_key = "fpsdrop";
                metricDesc.m_data = message;

                metrics.emplace_back(AZStd::move(metricDesc));
            }

            CloudGemDefectReporterRequestBus::QueueBroadcast(&CloudGemDefectReporterRequestBus::Events::ReportData, reportId, handlerId, metrics, AZStd::vector<AttachmentDesc>());
            CloudGemDefectReporterRequestBus::QueueBroadcast(&CloudGemDefectReporterRequestBus::Events::AddAnnotation, reportId, message);

            m_queuedReportIdSet.insert(reportId);
        }

        m_lastframeTimeInSeconds = frameTimeInSeconds;
    }
    
    void FPSDropReportingComponent::Enable(bool enable)
    {
        m_enabled = enable;
    }

    bool FPSDropReportingComponent::IsEnabled()
    {
        return m_enabled;
    }

    void FPSDropReportingComponent::OnDefectReportUploaded(int reportID)
    {
        m_queuedReportIdSet.erase(reportID);
    }

    void FPSDropReportingComponent::SetMaxQueuedFPSDropReports(int max)
    {
        m_maxQueuedFPSDropReports = max;
    }

    int FPSDropReportingComponent::GetMaxQueuedFPSDropReports()
    {
        return m_maxQueuedFPSDropReports;
    }

    void FPSDropReportingComponent::SetFrameTimeDropThresholdInSeconds(float threshold)
    {
        m_frameTimeDropThreasholdInSeconds = threshold;
    }

    float FPSDropReportingComponent::GetFrameTimeDropThresholdInSeconds()
    {
        return m_frameTimeDropThreasholdInSeconds;
    }
}