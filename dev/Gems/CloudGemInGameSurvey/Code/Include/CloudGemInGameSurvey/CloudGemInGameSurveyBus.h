
#pragma once

#include <AzCore/EBus/EBus.h>

namespace CloudGemInGameSurvey
{
    class CloudGemInGameSurveyRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions
    };
    using CloudGemInGameSurveyRequestBus = AZ::EBus<CloudGemInGameSurveyRequests>;
} // namespace CloudGemInGameSurvey
