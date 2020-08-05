/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/std/string/conversions.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Color.h>
#include <string>


namespace AZ
{
    namespace ConsoleTypeHelpers
    {
        inline AZStd::string ConvertString(const std::string& value)
        {
            return AZStd::string(value.c_str(), value.size());
        }


        inline std::string ConvertString(const AZStd::string& value)
        {
            return std::string(value.c_str(), value.size());
        }


        template <typename TYPE>
        inline AZStd::string ValueToString(const TYPE& value)
        {
            return AZStd::to_string(value);
        }

        template <>
        inline AZStd::string ValueToString(const bool& value)
        {
            return value ? "true" : "false";
        }

        template <>
        inline AZStd::string ValueToString(const char& value)
        {
            return AZStd::string(1, value);
        }

        template <>
        inline AZStd::string ValueToString<std::string>(const std::string& value)
        {
            return ConvertString(value);
        }

        template <>
        inline AZStd::string ValueToString<AZStd::string>(const AZStd::string& value)
        {
            return value;
        }

        template <>
        inline AZStd::string ValueToString<AZ::Vector2>(const AZ::Vector2& value)
        {
            return AZStd::string::format("%0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()));
        }

        template <>
        inline AZStd::string ValueToString<AZ::Vector3>(const AZ::Vector3& value)
        {
            return AZStd::string::format("%0.2f %0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()));
        }

        template <>
        inline AZStd::string ValueToString<AZ::Vector4>(const AZ::Vector4& value)
        {
            return AZStd::string::format("%0.2f %0.2f %0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()), static_cast<float>(value.GetW()));
        }

        template <>
        inline AZStd::string ValueToString<AZ::Quaternion>(const AZ::Quaternion& value)
        {
            return AZStd::string::format("%0.2f %0.2f %0.2f %0.2f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()), static_cast<float>(value.GetW()));
        }

        template <>
        inline AZStd::string ValueToString<AZ::Color>(const AZ::Color& value)
        {
            return AZStd::string::format("%0.2f %0.2f %0.2f %0.2f", static_cast<float>(value.GetR()), static_cast<float>(value.GetG()), static_cast<float>(value.GetB()), static_cast<float>(value.GetA()));
        }

        template <typename TYPE>
        bool StringSetToValue(TYPE& outValue, const StringSet& arguments);

        template <>
        inline bool StringSetToValue<bool>(bool& outValue, const StringSet& arguments)
        {
            if (!arguments.empty())
            {
                AZStd::string lower = arguments.front();
                AZStd::to_lower(lower.begin(), lower.end());

                if ((lower == "false") || (lower == "0"))
                {
                    outValue = false;
                    return true;
                }
                else if ((lower == "true") || (lower == "1"))
                {
                    outValue = true;
                    return true;
                }
                else
                {
                    AZ_Warning("Az Console", false, "Invalid input for boolean variable");
                }
            }
            return false;
        }

        template <>
        inline bool StringSetToValue<char>(char& outValue, const StringSet& arguments)
        {
            if (!arguments.empty())
            {
                const AZStd::string& frontArg = arguments.front();
                outValue = frontArg[0];
            }
            else
            {
                // An empty string is actually a null character '\0'
                outValue = '\0';
            }
            return true;
        }
                
        // Note that string-stream doesn't behave nicely with char and uint8_t..
        // So instead we string-stream to a very large type that does work, and downcast if the result is within the limits of the target type
        template <typename TYPE, typename MAX_TYPE>
        inline bool StringSetToIntegralValue(TYPE& outValue, const StringSet& arguments, const char* typeName, const char* errorString)
        {
            AZ_UNUSED(typeName);
            AZ_UNUSED(errorString);

            if (!arguments.empty())
            {
                char* endPtr = nullptr;
                MAX_TYPE value = static_cast<MAX_TYPE>(strtoll(arguments.front().c_str(), &endPtr, 0));

                if (endPtr == arguments.front().c_str())
                {
                    AZ_Warning("Az Console", false, "stringstream failed to convert value %s to %s type\n", arguments.front().c_str(), typeName);
                    return false;
                }

                if ((value >= std::numeric_limits<TYPE>::min()) && (value <= std::numeric_limits<TYPE>::max()))
                {
                    outValue = static_cast<TYPE>(value);
                    return true;
                }
                else
                {
                    AZ_Warning("Az Console", false, errorString, static_cast<MAX_TYPE>(outValue));
                }
            }

            return false;
        }

#define INTEGRAL_NUMERIC_HANDLER(TYPE, MAX_TYPE, FMT_STRING) \
        template <> \
        inline bool StringSetToValue<TYPE>(TYPE& outValue, const StringSet& arguments) \
        { \
            return StringSetToIntegralValue<TYPE, MAX_TYPE>(outValue, arguments, #TYPE, "attempted to assign out of range value %" #FMT_STRING "\n"); \
        }

        INTEGRAL_NUMERIC_HANDLER(int8_t,  int64_t, lld);
        INTEGRAL_NUMERIC_HANDLER(int16_t, int64_t, lld);
        INTEGRAL_NUMERIC_HANDLER(int32_t, int64_t, lld);
        INTEGRAL_NUMERIC_HANDLER(int64_t, int64_t, lld);

        INTEGRAL_NUMERIC_HANDLER(uint8_t,  uint64_t, llu);
        INTEGRAL_NUMERIC_HANDLER(uint16_t, uint64_t, llu);
        INTEGRAL_NUMERIC_HANDLER(uint32_t, uint64_t, llu);
        INTEGRAL_NUMERIC_HANDLER(uint64_t, uint64_t, llu);
#undef INTEGRAL_NUMERIC_HANDLER

        template <>
        inline bool StringSetToValue<float>(float& outValue, const StringSet& arguments)
        {
            if (!arguments.empty())
            {
                char* endPtr = nullptr;
                const float converted = static_cast<float>(strtod(arguments.front().c_str(), &endPtr));
                if (endPtr == arguments.front().c_str())
                {
                    AZ_Warning("Az Console", false, "Invalid input for float variable");
                    return false;
                }
                outValue = converted;
                return true;
            }
            return false;
        }

        template <>
        inline bool StringSetToValue<double>(double& outValue, const StringSet& arguments)
        {
            if (!arguments.empty())
            {
                char* endPtr = nullptr;
                const double converted = strtod(arguments.front().c_str(), &endPtr);
                if (endPtr == arguments.front().c_str())
                {
                    AZ_Warning("Az Console", false, "Invalid input for double variable");
                    return false;
                }
                outValue = converted;
                return true;
            }
            return false;
        }

        template <>
        inline bool StringSetToValue<std::string>(std::string& outValue, const StringSet& arguments)
        {
            if (!arguments.empty())
            {
                AZStd::string tempValue;
                StringFunc::Join(tempValue, arguments.begin(), arguments.end(), " ");
                outValue = ConvertString(tempValue);
                return true;
            }
            return false;
        }

        template <>
        inline bool StringSetToValue<AZStd::string>(AZStd::string& outValue, const StringSet& arguments)
        {
            if (!arguments.empty())
            {
                outValue.clear();
                StringFunc::Join(outValue, arguments.begin(), arguments.end(), " ");
                return true;
            }
            return false;
        }

        template <typename TYPE, uint32_t ELEMENT_COUNT>
        inline bool StringSetToVectorValue(TYPE& outValue, const StringSet& arguments, const char* typeName)
        {
            AZ_UNUSED(typeName);

            if (arguments.size() < ELEMENT_COUNT)
            {
                AZ_Warning("Az Console", false, "Not enough arguments provided to %s StringSetToValue()", typeName);
                return false;
            }

            for (uint32_t i = 0; i < ELEMENT_COUNT; ++i)
            {
                outValue.SetElement(i, static_cast<float>(strtod(arguments[i].c_str(), nullptr)));
            }

            return true;
        }

        inline bool StringSetToRgbaValue(AZ::Color& outColor, const StringSet& arguments)
        {
            const uint32_t ArgumentCount = 4;

            if (arguments.size() < ArgumentCount)
            {
                AZ_Warning("Az Console", false, "Not enough arguments provided to AZ::Color StringSetToValue()");
                return false;
            }

            using ColorSetter = void (AZ::Color::*)(u8);
            ColorSetter rgbaSetters[] = { &AZ::Color::SetR8, &AZ::Color::SetG8, &AZ::Color::SetB8, &AZ::Color::SetA8 };

            for (uint32_t i = 0; i < ArgumentCount; ++i)
            {
                ((outColor).*(rgbaSetters[i]))((static_cast<u8>(strtoll(arguments[i].c_str(), nullptr, 0))));
            }

            return true;
        }

        template <>
        inline bool StringSetToValue<AZ::Vector2>(AZ::Vector2& outValue, const StringSet& arguments)
        {
            return StringSetToVectorValue<AZ::Vector2, 2>(outValue, arguments, "AZ::Vector2");
        }

        template <>
        inline bool StringSetToValue<AZ::Vector3>(AZ::Vector3& outValue, const StringSet& arguments)
        {
            return StringSetToVectorValue<AZ::Vector3, 3>(outValue, arguments, "AZ::Vector3");
        }

        template <>
        inline bool StringSetToValue<AZ::Vector4>(AZ::Vector4& outValue, const StringSet& arguments)
        {
            return StringSetToVectorValue<AZ::Vector4, 4>(outValue, arguments, "AZ::Vector4");
        }

        template <>
        inline bool StringSetToValue<AZ::Quaternion>(AZ::Quaternion& outValue, const StringSet& arguments)
        {
            return StringSetToVectorValue<AZ::Quaternion, 4>(outValue, arguments, "AZ::Quaternion");
        }

        template <>
        inline bool StringSetToValue<AZ::Color>(AZ::Color& outValue, const StringSet& arguments)
        {
            const bool decimal = AZStd::any_of(
                AZStd::cbegin(arguments), AZStd::cend(arguments), [](const AZStd::string argument)
                {
                    return argument.find(".") != AZStd::string::npos;
                });

            if (decimal)
            {
                return StringSetToVectorValue<AZ::Color, 4>(outValue, arguments, "AZ::Color");
            }
            else
            {
                return StringSetToRgbaValue(outValue, arguments);
            }
        }

        template <typename _TYPE>
        inline bool StringToValue(_TYPE& outValue, const AZStd::string& string)
        {
            StringSet arguments;
            StringFunc::Tokenize(string, arguments, " ");
            return StringSetToValue(outValue, arguments);
        }
    }
}
