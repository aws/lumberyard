
#pragma once

#include <AzCore/Component/Component.h>

#include <CloudGemLeaderboard/CloudGemLeaderboardBus.h>

namespace CloudGemLeaderboard
{
    class CloudGemLeaderboardSystemComponent
        : public AZ::Component
        , protected CloudGemLeaderboardRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CloudGemLeaderboardSystemComponent, "{DB7BFDC7-BDC0-4443-9A50-9DF48B6CE0FE}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemLeaderboardRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
