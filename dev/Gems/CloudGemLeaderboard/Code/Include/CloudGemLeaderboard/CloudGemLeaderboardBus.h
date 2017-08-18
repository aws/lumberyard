
#pragma once

#include <AzCore/EBus/EBus.h>

namespace CloudGemLeaderboard
{
    class CloudGemLeaderboardRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using CloudGemLeaderboardRequestBus = AZ::EBus<CloudGemLeaderboardRequests>;
} // namespace CloudGemLeaderboard
