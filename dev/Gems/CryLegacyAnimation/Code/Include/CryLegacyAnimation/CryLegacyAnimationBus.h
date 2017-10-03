
#pragma once

#include <AzCore/EBus/EBus.h>

namespace CryLegacyAnimation
{
    class CryLegacyAnimationRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using CryLegacyAnimationRequestBus = AZ::EBus<CryLegacyAnimationRequests>;
} // namespace CryLegacyAnimation
