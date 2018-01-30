// Copyright 2017 FragLab Ltd. All rights reserved.

#pragma once

#include <GridMate/Serialize/Buffer.h>
#include <AzCore/Math/Internal/MathTypes.h>
#include <AzCore/Math/MathUtils.h>

namespace FragLab
{
    namespace Marshalers
    {
        template<typename DataType>
        inline bool IsEqual(const DataType& value1, const DataType& value2)
        {
            return value1 == value2;
        }

        template<>
        inline bool IsEqual(const float& value1, const float& value2)
        {
            return AZ::IsClose(value1, value2, AZ_FLT_EPSILON);
        }

        template<typename DataType, class BaseMarshaler>
        class MarshalerWithDefault : public BaseMarshaler
        {
        public:
            MarshalerWithDefault(DataType defaultValue = DataType(0)) : m_defaultValue(defaultValue) {}
            MarshalerWithDefault(DataType minValue, DataType maxValue, DataType defaultValue = DataType(0)) : BaseMarshaler(minValue, maxValue), m_defaultValue(defaultValue) {}

            void Marshal(GridMate::WriteBuffer& wb, const DataType& value) const
            {
                const bool isDefault = IsEqual(value, m_defaultValue);
                wb.WriteRawBit(isDefault);
                if (!isDefault)
                {
                    BaseMarshaler::Marshal(wb, value);
                }
            }
            void Unmarshal(DataType& value, GridMate::ReadBuffer& rb) const
            {
                bool isDefault = false;
                rb.ReadRawBit(isDefault);
                if (isDefault)
                {
                    value = m_defaultValue;
                }
                else
                {
                    BaseMarshaler::Unmarshal(value, rb);
                }
            }

        private:
            DataType m_defaultValue;
        };
    } // namespace Marshalers
}     // namespace FragLab
