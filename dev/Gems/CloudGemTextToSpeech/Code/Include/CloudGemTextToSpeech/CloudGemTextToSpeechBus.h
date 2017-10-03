
#pragma once

#include <AzCore/EBus/EBus.h>

namespace CloudGemTextToSpeech
{
    class CloudGemTextToSpeechRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using CloudGemTextToSpeechRequestBus = AZ::EBus<CloudGemTextToSpeechRequests>;
} // namespace CloudGemTextToSpeech
