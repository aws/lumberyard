
#pragma once

#include <AzCore/EBus/EBus.h>

namespace NullVR
{
    class NullVRRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using NullVRRequestBus = AZ::EBus<NullVRRequests>;
} // namespace NullVR
