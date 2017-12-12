
#pragma once

#include <AzCore/EBus/EBus.h>

namespace Footsteps
{
    class FootstepsRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using FootstepsRequestBus = AZ::EBus<FootstepsRequests>;
} // namespace Footsteps
