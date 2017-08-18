
#pragma once

#include <AzCore/EBus/EBus.h>

namespace LyShineExamples
{
    class LyShineExamplesRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions

    };
    using LyShineExamplesRequestBus = AZ::EBus<LyShineExamplesRequests>;
} // namespace LyShineExamples
