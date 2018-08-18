#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <CloudGemDefectReporter/CloudGemDefectReporterBus.h>
#include <CloudGemDefectReporter/DefectReporterDataStructures.h>

namespace CloudGemDefectReporter
{
    class DxDiagCollectingComponent
        : public AZ::Component
        , protected CloudGemDefectReporterNotificationBus::Handler
    {
    public:
        DxDiagCollectingComponent();
        AZ_COMPONENT(DxDiagCollectingComponent, "{80372dae-06cc-11e8-ba89-0ed5f89f718b}");

        static void Reflect(AZ::ReflectContext* context);

    protected:        
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ////////////////////////////////////////////////////////////////////////
        // CloudGemDefectReporterNotificationBus::Handler interface implementation
        virtual void OnCollectDefectReporterData(int reportID) override;
        ////////////////////////////////////////////////////////////////////////    
    private:
        const char* GetDxDiagFileDir() const;
        AZStd::string GetDxDiagFilePath() const;
        bool CreateDxDiagDirIfNotExists();
        AZStd::string SaveDxDiagFile(const FileBuffer& dxDiagBuffer);
    };
}
