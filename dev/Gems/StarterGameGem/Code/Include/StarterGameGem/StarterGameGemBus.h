
#pragma once

#include <AzCore/EBus/EBus.h>

namespace StarterGameGem
{
    class StarterGameGemRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using StarterGameGemRequestBus = AZ::EBus<StarterGameGemRequests>;
} // namespace StarterGameGem
