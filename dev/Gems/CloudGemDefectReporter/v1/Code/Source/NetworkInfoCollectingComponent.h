#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <CloudGemDefectReporter/CloudGemDefectReporterBus.h>

#include <CrySystemBus.h>

namespace CloudGemDefectReporter
{
    class NetworkInfoCollectingComponent
        : public AZ::Component
        , protected CloudGemDefectReporterNotificationBus::Handler
        , protected CrySystemEventBus::Handler
    {
    public:
        NetworkInfoCollectingComponent();
        AZ_COMPONENT(NetworkInfoCollectingComponent, "{78d8b29c-9457-44fc-a5c8-20b5006b4843}");

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

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEventBus interface implementation
        virtual void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& params) override;
        ////////////////////////////////////////////////////////////////////////
    private:
        bool ReadServerDomainNames();
        const char* GetServerDomainNamesConfigPath() const;

        AZStd::vector<AZStd::string> m_serverDomainNames;
    };
}
