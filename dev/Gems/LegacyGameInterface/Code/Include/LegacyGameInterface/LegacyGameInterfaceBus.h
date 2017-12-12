
#pragma once

#include <AzCore/EBus/EBus.h>

namespace LegacyGameInterface
{
    class LegacyGameInterfaceRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using LegacyGameInterfaceRequestBus = AZ::EBus<LegacyGameInterfaceRequests>;
} // namespace LegacyGameInterface
