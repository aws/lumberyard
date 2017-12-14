
#pragma once

#include <AzCore/EBus/EBus.h>

namespace CloudGemSpeechRecognition
{
    class CloudGemSpeechRecognitionRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using CloudGemSpeechRecognitionRequestBus = AZ::EBus<CloudGemSpeechRecognitionRequests>;
} // namespace CloudGemSpeechRecognition
