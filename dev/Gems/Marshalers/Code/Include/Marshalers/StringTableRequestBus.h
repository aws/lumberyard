// Copyright 2017 FragLab Ltd. All rights reserved.

#pragma once

#include <AzCore/EBus/EBus.h>

namespace FragLab
{
    namespace Marshalers
    {
        class IStringTableRequestBus : public AZ::EBusTraits
        {
        public:
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            virtual AZ::u32 MarshalString(const char* szValue) = 0;
            virtual const char* UnmarshalString(AZ::u32 id) const = 0;
        };

        using StringTableRequestBus = AZ::EBus<IStringTableRequestBus>;
    }
}