// Copyright 2017 FragLab Ltd. All rights reserved.

#pragma once

#include "StringTableRequestBus.h"

#include <GridMate/Serialize/Buffer.h>

namespace FragLab
{
    namespace Marshalers
    {
        template<class StringType>
        class StringMarshalerImpl
        {
        public:
            void Marshal(GridMate::WriteBuffer& wb, const StringType& value) const
            {
                AZ::u32 crc = 0;
                EBUS_EVENT_RESULT(crc, StringTableRequestBus, MarshalString, value.c_str());
                wb.Write(crc);
            }

            void Unmarshal(StringType& value, GridMate::ReadBuffer& rb) const
            {
                AZ::u32 crc = 0;
                rb.Read(crc);
                EBUS_EVENT_RESULT(value, StringTableRequestBus, UnmarshalString, crc);
            }
        };

        using StringMarshaler = StringMarshalerImpl<AZStd::string>;
    }
}
