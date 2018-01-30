// Copyright 2017 FragLab Ltd. All rights reserved.

#pragma once

#include "MarshalerWithDefault.h"

namespace FragLab
{
    namespace Marshalers
    {
        template<AZ::u64 Value>
        struct NumBitsCalculator
        {
            enum { value = 1U + NumBitsCalculator<(Value >> 1ULL)>::value };
        };

        template<>
        struct NumBitsCalculator<0>
        {
            enum { value = 0U };
        };

        template<typename DataType, DataType MinValue, DataType MaxValue, typename SerializedType = AZ::u64, AZStd::size_t NumBits = NumBitsCalculator<MaxValue - MinValue>::value>
        class IntMarshaler
        {
        public:
            void Marshal(GridMate::WriteBuffer& wb, const DataType& value) const
            {
                AZ_Assert(value >= MinValue && value <= MaxValue, "%lld out of range [%lld...%lld]", static_cast<AZ::s64>(value), static_cast<AZ::s64>(MinValue), static_cast<AZ::s64>(MaxValue));
                SerializedType res = static_cast<SerializedType>(value) - static_cast<SerializedType>(MinValue);
                wb.WriteRaw(&res, GridMate::PackedSize(0, NumBits));
            }

            void Unmarshal(DataType& value, GridMate::ReadBuffer& rb) const
            {
                SerializedType res = 0;
                rb.ReadRawBits(&res, GridMate::PackedSize(0, NumBits));
                value = static_cast<DataType>(res + static_cast<SerializedType>(MinValue));
            }

            AZStd::size_t GetNumBits() const
            {
                return NumBits;
            }
        };

        template<typename DataType, DataType MinValue, DataType ExpectedMaxValue, typename SerializedType = AZStd::size_t, SerializedType ExpectedNumBits = NumBitsCalculator<ExpectedMaxValue - MinValue + DataType(1)>::value>
        class AdaptiveIntMarshaler
        {
            static_assert(ExpectedNumBits <= CHAR_BIT * sizeof(DataType), "ExpectedNumBits must be less or equal CHAR_BIT(8) * sizeof(DataType)");

        public:
            void Marshal(GridMate::WriteBuffer& wb, const DataType& value) const
            {
                static const SerializedType maxValue0 = (SerializedType(1) << ExpectedNumBits) - SerializedType(1);
                static const SerializedType maxValue1 = (SerializedType(1) << (CHAR_BIT - ExpectedNumBits % CHAR_BIT)) - SerializedType(1);
                static const SerializedType maxValue2 = (SerializedType(1) << CHAR_BIT) - SerializedType(1);

                SerializedType serValue = static_cast<SerializedType>(value - MinValue);
                if (serValue < maxValue0)
                {
                    wb.WriteRaw(&serValue, GridMate::PackedSize(0, ExpectedNumBits));
                }
                else
                {
                    wb.WriteRaw(&maxValue0, GridMate::PackedSize(0, ExpectedNumBits));
                    serValue -= maxValue0;

                    if (serValue < maxValue1)
                    {
                        wb.WriteRaw(&serValue, GridMate::PackedSize(0, CHAR_BIT - ExpectedNumBits % CHAR_BIT));
                    }
                    else
                    {
                        wb.WriteRaw(&maxValue1, GridMate::PackedSize(0, CHAR_BIT - ExpectedNumBits % CHAR_BIT));
                        serValue -= maxValue1;

                        if (serValue < maxValue2)
                        {
                            wb.WriteRaw(&serValue, GridMate::PackedSize(1));
                        }
                        else
                        {
                            wb.WriteRaw(&maxValue2, GridMate::PackedSize(1));
                            serValue -= maxValue2;

                            AZ_Assert(serValue < (1U << (CHAR_BIT * 2)), "Value (%lld) is too large for marshaling!", static_cast<AZ::s64>(value));
                            wb.WriteRaw(&serValue, GridMate::PackedSize(2));
                        }
                    }
                }
            }

            void Unmarshal(DataType& value, GridMate::ReadBuffer& rb) const
            {
                static const SerializedType maxValue0 = (SerializedType(1) << ExpectedNumBits) - SerializedType(1);
                static const SerializedType maxValue1 = (SerializedType(1) << (CHAR_BIT - ExpectedNumBits % CHAR_BIT)) - SerializedType(1);
                static const SerializedType maxValue2 = (SerializedType(1) << CHAR_BIT) - SerializedType(1);

                SerializedType res = 0;
                rb.ReadRawBits(&res, GridMate::PackedSize(0, ExpectedNumBits));
                if (res == maxValue0)
                {
                    SerializedType res2 = 0;
                    rb.ReadRawBits(&res2, GridMate::PackedSize(0, CHAR_BIT - ExpectedNumBits % CHAR_BIT));
                    res += res2;

                    if (res2 == maxValue1)
                    {
                        res2 = 0;
                        rb.ReadRaw(&res2, GridMate::PackedSize(1));
                        res += res2;

                        if (res2 == maxValue2)
                        {
                            res2 = 0;
                            rb.ReadRaw(&res2, GridMate::PackedSize(2));
                            res += res2;
                        }
                    }
                }

                value = static_cast<DataType>(res) + MinValue;
            }
        };

        template<typename DataType, DataType MinValue, DataType MaxValue, DataType DefaultValue = DataType(0), typename SerializedType = AZ::u64>
        class IntWithDefaultMarshaler : public MarshalerWithDefault<DataType, IntMarshaler<DataType, MinValue, MaxValue, SerializedType>>
        {
            using BaseMarshaler = MarshalerWithDefault<DataType, IntMarshaler<DataType, MinValue, MaxValue, SerializedType>>;

        public:
            IntWithDefaultMarshaler() : BaseMarshaler(DefaultValue) {}
        };

        template<typename DataType, DataType MinValue, DataType ExpectedMaxValue, DataType DefaultValue = DataType(0), typename SerializedType = AZStd::size_t>
        class AdaptiveIntWithDefaultMarshaler : public MarshalerWithDefault<DataType, AdaptiveIntMarshaler<DataType, MinValue, ExpectedMaxValue, SerializedType>>
        {
            using BaseMarshaler = MarshalerWithDefault<DataType, AdaptiveIntMarshaler<DataType, MinValue, ExpectedMaxValue, SerializedType>>;

        public:
            AdaptiveIntWithDefaultMarshaler() : BaseMarshaler(DefaultValue) {}
        };

        template<typename EnumType, EnumType FirstValue = EnumType(0), EnumType LastValue = static_cast<EnumType>(static_cast<typename AZStd::underlying_type<EnumType>::type>(EnumType::Count) - 1), typename UnderlyingType = typename AZStd::underlying_type<EnumType>::type>
        class EnumMarshaler : public IntMarshaler<EnumType, FirstValue, LastValue, UnderlyingType, NumBitsCalculator<static_cast<UnderlyingType>(LastValue) - static_cast<UnderlyingType>(FirstValue)>::value>
        {
        };
    }                                    // namespace Marshalers
}                                        // namespace FragLab
