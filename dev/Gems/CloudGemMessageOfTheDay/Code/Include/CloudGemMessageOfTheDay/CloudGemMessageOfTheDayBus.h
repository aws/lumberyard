
#pragma once

#include <AzCore/EBus/EBus.h>

namespace CloudGemMessageOfTheDay
{
    class CloudGemMessageOfTheDayRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using CloudGemMessageOfTheDayRequestBus = AZ::EBus<CloudGemMessageOfTheDayRequests>;
} // namespace CloudGemMessageOfTheDay
