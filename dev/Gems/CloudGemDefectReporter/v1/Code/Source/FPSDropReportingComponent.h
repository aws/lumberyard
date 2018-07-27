#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/set.h>
#include <CloudGemDefectReporter/CloudGemDefectReporterBus.h>

namespace CloudGemDefectReporter
{
    class FPSDropReportingComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public CloudGemDefectReporterFPSDropReportingRequestBus::Handler
        , public CloudGemDefectReporterNotificationBus::Handler
    {
    public:
        FPSDropReportingComponent();
        AZ_COMPONENT(FPSDropReportingComponent, "{9e9e3d63-1a0c-47de-bbb2-c1f3e768443b}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:        
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////        

        ////////////////////////////////////////////////////////////////////////
        // CloudGemDefectReporterFPSDropReportingRequestBus interface implementation

        // set max number of FPS drop reports that can be queued
        virtual void SetMaxQueuedFPSDropReports(int max) override;

        // get max number of FPS drop reports that can be queued
        virtual int GetMaxQueuedFPSDropReports() override;

        // set threshold for frame time drop in seconds to trigger FPS drop defect report 
        virtual void SetFrameTimeDropThresholdInSeconds(float threshold) override;

        // get threshold for frame time drop in seconds to trigger FPS drop defect report 
        virtual float GetFrameTimeDropThresholdInSeconds() override;

        // Enable/Disable auto FPS drop reporting
        virtual void Enable(bool enable) override;

        // Get enabled flag
        virtual bool IsEnabled() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CloudGemDefectReporterFPSDropReportingRequestBus interface implementation

        // notify the handler that the report has been uploaded in case the handler needs to do
        // any manual cleanup of report artifacts
        virtual void OnDefectReportUploaded(int reportID) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        bool DetectFPSDrop(float deltaTimeInSecond);

    private:
        bool m_enabled{false};
        float m_frameTimeDropThreasholdInSeconds{1};
        float m_lastframeTimeInSeconds{-1};
        int m_maxQueuedFPSDropReports{3};
        AZStd::set<int> m_queuedReportIdSet;
    };
}
