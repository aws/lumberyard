
#pragma once

#include <AzCore/EBus/EBus.h>

namespace LegacyTimeDemoRecorder
{
    class LegacyTimeDemoRecorderRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using LegacyTimeDemoRecorderRequestBus = AZ::EBus<LegacyTimeDemoRecorderRequests>;
} // namespace LegacyTimeDemoRecorder
