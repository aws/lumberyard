
#pragma once

#include <AzCore/EBus/EBus.h>

namespace CloudGemLex
{
    class CloudGemLexRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using CloudGemLexRequestBus = AZ::EBus<CloudGemLexRequests>;
} // namespace CloudGemLex
