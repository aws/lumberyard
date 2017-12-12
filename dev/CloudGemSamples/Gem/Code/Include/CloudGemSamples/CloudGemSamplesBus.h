
#pragma once

#include <AzCore/EBus/EBus.h>

namespace LYGame
{
    class CloudGemSamplesRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using CloudGemSamplesRequestBus = AZ::EBus<CloudGemSamplesRequests>;
} // namespace CloudGemSamples
