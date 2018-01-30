// Copyright 2017 FragLab Ltd. All rights reserved.

#pragma once

#include "MarshalerWithDefault.h"

namespace FragLab
{
    namespace Marshalers
    {
        template<AZStd::size_t NumBits>
        class FloatMarshaler
        {
            static const AZ::u32 MaxIntValue = (1U << NumBits) - 1U;

        public:
            FloatMarshaler(float minValue, float maxValue) : m_minValue(minValue), m_maxValue(maxValue) {}

            void Marshal(GridMate::WriteBuffer& wb, const float& value) const
            {
                AZ_Assert(value >= m_minValue && value <= m_maxValue, "%f out of range [%f...%f]", value, m_minValue, m_maxValue);
                const float kRatio = MaxIntValue / (m_maxValue - m_minValue);

                AZ::u32 res = AZ::GetMin(static_cast<AZ::u32>((value - m_minValue) * kRatio + 0.5f), MaxIntValue);
                wb.WriteRaw(&res, GridMate::PackedSize(0, NumBits));
            }
            void Unmarshal(float& value, GridMate::ReadBuffer& rb) const
            {
                AZ::u32 res = 0;
                rb.ReadRawBits(&res, GridMate::PackedSize(0, NumBits));
                if (res < MaxIntValue)
                {
                    const float kInvRatio = (m_maxValue - m_minValue) / MaxIntValue;
                    value = res * kInvRatio + m_minValue;
                }
                else
                {
                    value = m_maxValue;
                }
            }

            float GetPrecision() const
            {
                return 0.5f * (m_maxValue - m_minValue) / MaxIntValue;
            }

        private:
            float m_minValue;
            float m_maxValue;
        };

        // Uses NumBits on interval [expectedMinValue, expectedMaxValue]
        // and 1 bit to indicate the value in/out of interval [expectedMinValue, expectedMaxValue]
        // Adds ExponentBits when the value out of interval [expectedMinValue, expectedMaxValue]
        // supportedMinValue = expectedMinValue * pow(10, 1 << ExponentBits)
        // supportedMaxValue = expectedMaxValue * pow(10, 1 << ExponentBits)
        // with precision = GetPrecision() * pow(10, 1 << ExponentBits)
        template<AZ::u32 NumBits, AZ::u32 ExponentBits = 3>
        class AdaptiveFloatMarshaler
        {
            static const AZ::u32 MaxIntValue = (1U << NumBits) - 1U;

        public:
            AdaptiveFloatMarshaler(float expectedMinValue, float expectedMaxValue) : m_minValue(expectedMinValue), m_maxValue(expectedMaxValue) {}

            void Marshal(GridMate::WriteBuffer& wb, const float& value) const
            {
                const float kRatio = MaxIntValue / (m_maxValue - m_minValue);
                if (value >= m_minValue && value <= m_maxValue)
                {
                    AZ::u32 res = AZ::GetMin(static_cast<AZ::u32>((value - m_minValue) * kRatio + 0.5f), MaxIntValue);
                    wb.WriteRaw(&res, GridMate::PackedSize(0, NumBits + 1));
                }
                else
                {
                    AZ::u32 res = 1U << NumBits;

                    const float scale = 0.1f;
                    float scaledValue = value * scale;
                    if (scaledValue < m_minValue || scaledValue > m_maxValue)
                    {
                        const AZ::u32 maxExponent = (1U << ExponentBits) - 1U;
                        // exponent = min(ceil(log10(value / m_minValue)), 1 << ExponentBits) - 1
                        AZ::u32 exponent = maxExponent;
                        for (AZ::u32 i = 1; i <= maxExponent; ++i)
                        {
                            scaledValue *= scale;
                            if (scaledValue >= m_minValue && scaledValue <= m_maxValue)
                            {
                                exponent = i;
                                break;
                            }
                        }

                        res |= (exponent << (NumBits + 1U));
                    }

                    res |= AZ::GetMin(static_cast<AZ::u32>((scaledValue - m_minValue) * kRatio + 0.5f), MaxIntValue);
                    wb.WriteRaw(&res, GridMate::PackedSize(0, NumBits + ExponentBits + 1U));
                }
            }
            void Unmarshal(float& value, GridMate::ReadBuffer& rb) const
            {
                const float kInvRatio = (m_maxValue - m_minValue) / MaxIntValue;

                AZ::u32 res = 0;
                rb.ReadRawBits(&res, GridMate::PackedSize(0, NumBits + 1));
                value = (res & MaxIntValue) * kInvRatio + m_minValue;

                if (res > MaxIntValue)
                {
                    int exponent = 0;
                    rb.ReadRawBits(&exponent, GridMate::PackedSize(0, ExponentBits));
                    do
                    {
                        value *= 10.0f;
                    }
                    while (--exponent >= 0);
                }
            }

            float GetPrecision(float value) const
            {
                float precision = 0.5f * (m_maxValue - m_minValue) / MaxIntValue;
                if (value < m_minValue)
                {
                    AZ::s32 exponent = AZ::GetMin(int_ceil(log10(value / m_minValue)), 1 << ExponentBits);
                    precision *= pow(10.0f, exponent);
                }
                else if (value > m_maxValue)
                {
                    AZ::s32 exponent = AZ::GetMin(int_ceil(log10(value / m_maxValue)), 1 << ExponentBits);
                    precision *= pow(10.0f, exponent);
                }

                return precision;
            }

        private:
            float m_minValue;
            float m_maxValue;
        };

        template<AZStd::size_t NumBits>
        class FloatWithDefaultMarshaler : public MarshalerWithDefault<float, FloatMarshaler<NumBits - 1>>
        {
            using BaseMarshaler = MarshalerWithDefault<float, FloatMarshaler<NumBits - 1>>;

        public:
            FloatWithDefaultMarshaler(float minValue, float maxValue, float defaultValue = 0.0f) : BaseMarshaler(minValue, maxValue, defaultValue) {}
        };

        template<AZ::u32 NumBits, AZ::u32 ExponentBits = 3>
        class AdaptiveFloatWithDefaultMarshaler : public MarshalerWithDefault<float, AdaptiveFloatMarshaler<NumBits - 1, ExponentBits>>
        {
            using BaseMarshaler = MarshalerWithDefault<float, AdaptiveFloatMarshaler<NumBits - 1, ExponentBits>>;

        public:
            AdaptiveFloatWithDefaultMarshaler(float expectedMinValue, float expectedMaxValue, float defaultValue = 0.0f) : BaseMarshaler(expectedMinValue, expectedMaxValue, defaultValue) {}
        };
    } // namespace Marshalers
}     // namespace FragLab
