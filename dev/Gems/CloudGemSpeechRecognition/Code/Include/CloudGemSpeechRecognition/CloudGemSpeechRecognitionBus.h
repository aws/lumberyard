
#pragma once

#include <AzCore/EBus/EBus.h>

namespace CloudGemSpeechRecognition
{
    class SpeechRecognitionRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions

        virtual void BeginSpeechCapture() {};
        virtual void EndSpeechCaptureAndCallBot(const AZStd::string& botName,
            const AZStd::string& botAlias,
            const AZStd::string& userId,
            const AZStd::string& sessionAttributes) {};
    };
    using SpeechRecognitionRequestBus = AZ::EBus<SpeechRecognitionRequests>;
} // namespace CloudGemSpeechRecognition
