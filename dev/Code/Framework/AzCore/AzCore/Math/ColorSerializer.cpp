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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/ColorSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonColorSerializer, SystemAllocator, 0);

    JsonSerializationResult::Result JsonColorSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Color>() == outputValueTypeId,
            "Unable to deserialize Color to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);

        Color* color = reinterpret_cast<Color*>(outputValue);
        AZ_Assert(color, "Output value for JsonColorSerializer can't be null.");

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
            return LoadFloatArray(*color, inputValue, LoadAlpha::IfAvailable, path, settings);
        case rapidjson::kObjectType:
            return LoadObject(*color, inputValue, path, settings);
        
        case rapidjson::kStringType: // fall through
        case rapidjson::kNumberType: // fall through
        case rapidjson::kNullType:   // fall through
        case rapidjson::kFalseType:  // fall through
        case rapidjson::kTrueType:
            return JSR::Result(settings, "Unsupported type. Colors can only be read from arrays or objects.", 
                Tasks::ReadField, Outcomes::Unsupported, path);
        
        default:
            return JSR::Result(settings, "Unknown json type encountered in Color.", Tasks::ReadField, Outcomes::Unknown, path);
        }
    }

    JsonSerializationResult::Result JsonColorSerializer::Store(rapidjson::Value& outputValue,
        rapidjson::Document::AllocatorType& allocator, const void* inputValue, 
        const void* defaultValue, const Uuid& valueTypeId, 
        StackedString& path, const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Color>() == valueTypeId, "Unable to serialize Color to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        const Color* color = reinterpret_cast<const Color*>(inputValue);
        AZ_Assert(color, "Input value for JsonColorSerializer can't be null.");
        const Color* defaultColor = reinterpret_cast<const Color*>(defaultValue);

        if (!settings.m_keepDefaults && defaultColor && *color == *defaultColor)
        {
            return JSR::Result(settings, "Default Color used.", ResultCode::Default(Tasks::WriteValue), path);
        }

        outputValue.SetArray();
        outputValue.PushBack(color->GetR(), allocator);
        outputValue.PushBack(color->GetG(), allocator);
        outputValue.PushBack(color->GetB(), allocator);

        if (settings.m_keepDefaults || !defaultColor || (defaultColor && color->GetA() != defaultColor->GetA()))
        {
            outputValue.PushBack(color->GetA(), allocator);
            return JSR::Result(settings, "Color successfully stored.", ResultCode::Success(Tasks::WriteValue), path);
        }
        else
        {
            return JSR::Result(settings, "Color partially stored.", ResultCode::PartialDefault(Tasks::WriteValue), path);
        }
    }

    JsonSerializationResult::Result JsonColorSerializer::LoadObject(Color& output, const rapidjson::Value& inputValue, 
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        if (inputValue.ObjectEmpty())
        {
            return JSR::Result(settings, "There's no data in the object to read the color from.",
                Tasks::ReadField, Outcomes::Unsupported, path);
        }

        ResultCode result = ResultCode(Tasks::ReadField, Outcomes::Unsupported);
        auto it = inputValue.MemberEnd() - 1;
        while (true)
        {
            // Note that the order for checking is important because RGBA will be accepted for RGB if RGBA isn't tested first.
            if (azstricmp(it->name.GetString(), "RGBA8") == 0)
            {
                if (it->value.IsArray())
                {
                    ScopedStackedString subPath(path, "RGBA8");
                    result = LoadByteArray(output, it->value, true, subPath, settings);
                }
            }
            else if (azstricmp(it->name.GetString(), "RGB8") == 0)
            {
                if (it->value.IsArray())
                {
                    ScopedStackedString subPath(path, "RGB8");
                    result = LoadByteArray(output, it->value, false, subPath, settings);
                }
            }
            else if (azstricmp(it->name.GetString(), "RGBA") == 0)
            {
                if (it->value.IsArray())
                {
                    ScopedStackedString subPath(path, "RGBA");
                    result = LoadFloatArray(output, it->value, LoadAlpha::Yes, subPath, settings);
                }
            }
            else if (azstricmp(it->name.GetString(), "RGB") == 0)
            {
                if (it->value.IsArray())
                {
                    ScopedStackedString subPath(path, "RGB");
                    result = LoadFloatArray(output, it->value, LoadAlpha::No, subPath, settings);
                }
            }
            else if (azstricmp(it->name.GetString(), "HEXA") == 0)
            {
                if (it->value.IsString())
                {
                    ScopedStackedString subPath(path, "HEXA");
                    result = LoadHexString(output, it->value, true, subPath, settings);
                }
            }
            else if (azstricmp(it->name.GetString(), "HEX") == 0)
            {
                if (it->value.IsString())
                {
                    ScopedStackedString subPath(path, "HEX");
                    result = LoadHexString(output, it->value, false, subPath, settings);
                }
            }
            else if (it != inputValue.MemberBegin())
            {
                --it;
                continue;
            }

            return JSR::Result(settings, result.GetProcessing() != Processing::Halted ? "Successfully read color." : "Problem encountered while reading color.",
                result, path);
        }
    }

    JsonSerializationResult::Result JsonColorSerializer::LoadFloatArray(Color& output, const rapidjson::Value& inputValue, LoadAlpha loadAlpha,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        rapidjson::SizeType arraySize = inputValue.Size();
        rapidjson::SizeType minimumLength = loadAlpha == LoadAlpha::Yes ? 4 : 3;
        float channels[4];
        if (arraySize < minimumLength)
        {
            return JSR::Result(settings, "Not enough numbers in array to load color from.",
                Tasks::ReadField, Outcomes::Unsupported, path);
        }

        const rapidjson::Value& redValue = inputValue[0];
        if (redValue.IsDouble())
        {
            channels[0] = aznumeric_caster(redValue.GetDouble());
        }
        else
        {
            return JSR::Result(settings, "Red channel couldn't be read because the value wasn't a decimal number.",
                Tasks::ReadField, Outcomes::Unsupported, path);
        }

        const rapidjson::Value& greenValue = inputValue[1];
        if (greenValue.IsDouble())
        {
            channels[1] = aznumeric_caster(greenValue.GetDouble());
        }
        else
        {
            return JSR::Result(settings, "Green channel couldn't be read because the value wasn't a decimal number.",
                Tasks::ReadField, Outcomes::Unsupported, path);
        }

        const rapidjson::Value& blueValue = inputValue[2];
        if (blueValue.IsDouble())
        {
            channels[2] = aznumeric_caster(blueValue.GetDouble());
        }
        else
        {
            return JSR::Result(settings, "Blue channel couldn't be read because the value wasn't a decimal number.",
                Tasks::ReadField, Outcomes::Unsupported, path);
        }

        if (loadAlpha == LoadAlpha::Yes || (loadAlpha == LoadAlpha::IfAvailable && arraySize >= 4))
        {
            const rapidjson::Value& alphaValue = inputValue[3];
            if (alphaValue.IsDouble())
            {
                channels[3] = aznumeric_caster(alphaValue.GetDouble());
            }
            else
            {
                return JSR::Result(settings, "Alpha channel couldn't be read because the value wasn't a decimal number.",
                    Tasks::ReadField, Outcomes::Unsupported, path);
            }
            minimumLength = 4; // Set to 4 in case the alpha was optional but present. This needed in order to report the correct result.
        }
        else
        {
            channels[3] = output.GetA();
        }

        output = Color::CreateFromFloat4(channels);
        
        return JSR::Result(settings, "Successfully loaded color from decimal array.",
            ResultCode::Success(Tasks::ReadField), path);
    }

    JsonSerializationResult::Result JsonColorSerializer::LoadByteArray(Color& output, const rapidjson::Value& inputValue, bool loadAlpha,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        rapidjson::SizeType arraySize = inputValue.Size();
        rapidjson::SizeType expectedSize = loadAlpha ? 4 : 3;
        uint32_t color = 0; // Stored as AABBGGRR
        if (arraySize < expectedSize)
        {
            return JSR::Result(settings, "Not enough numbers in array to load color from.",
                Tasks::ReadField, Outcomes::Unsupported, path);
        }

        const rapidjson::Value& redValue = inputValue[0];
        if (redValue.IsInt64())
        {
            color = ClampToByteColorRange(redValue.GetInt64());
        }
        else
        {
            return JSR::Result(settings, "Red channel couldn't be read because the value wasn't a byte number.",
                Tasks::ReadField, Outcomes::Unsupported, path);
        }

        const rapidjson::Value& greenValue = inputValue[1];
        if (greenValue.IsInt64())
        {
            color |= ClampToByteColorRange(greenValue.GetInt64()) << 8;
        }
        else
        {
            return JSR::Result(settings, "Green channel couldn't be read because the value wasn't a byte number.",
                Tasks::ReadField, Outcomes::Unsupported, path);
        }

        const rapidjson::Value& blueValue = inputValue[2];
        if (blueValue.IsInt64())
        {
            color |= ClampToByteColorRange(blueValue.GetInt64()) <<  16;
        }
        else
        {
            return JSR::Result(settings, "Blue channel couldn't be read because the value wasn't a byte number.",
                Tasks::ReadField, Outcomes::Unsupported, path);
        }

        if (loadAlpha)
        {
            const rapidjson::Value& alphaValue = inputValue[3];
            if (alphaValue.IsInt64())
            {
                color |= ClampToByteColorRange(alphaValue.GetInt64()) << 24;
            }
            else
            {
                return JSR::Result(settings, "Alpha channel couldn't be read because the value wasn't a byte number.",
                    Tasks::ReadField, Outcomes::Unsupported, path);
            }
            output.FromU32(color);
        }
        else
        {
            float alpha = output.GetA(); // Don't use the byte value here as it's a lot more imprecise than copying the float value.
            output.FromU32(color);
            output.SetA(alpha);
        }
        
        return JSR::Result(settings, "Successfully loaded color from byte array.", ResultCode::Success(Tasks::ReadField), path);
    }

    JsonSerializationResult::Result JsonColorSerializer::LoadHexString(Color& output, const rapidjson::Value& inputValue, bool loadAlpha,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        size_t minimumLength = loadAlpha ? 8 : 6; // 8 for RRGGBBAA and 6 for RRGGBB
        
        AZStd::string_view text(inputValue.GetString(), inputValue.GetStringLength());
        if (text.length() < minimumLength)
        {
            return JSR::Result(settings, "The hex text for color is too short.",
                Tasks::ReadField, Outcomes::Unsupported, path);
        }

        uint32_t color = 0;
        size_t hexDigitIndex;
        size_t length = AZStd::min(minimumLength, text.length());
        const char* front = text.begin();
        for (hexDigitIndex = 0; hexDigitIndex < length; ++hexDigitIndex)
        {
            color <<= 4;
            
            char character = *front;
            if (character >= '0' && character <= '9')
            {
                color |= character - '0';
            }
            else if (character >= 'a' && character <= 'f')
            {
                color |= character - 'a' + 10;
            }
            else if (character >= 'A' && character <= 'F')
            {
                color |= character - 'A' + 10;
            }
            else
            {
                break;
            }
            
            front++;
        }

        if (hexDigitIndex != minimumLength)
        {
            return JSR::Result(settings, "Failed to parse hex value. There might be an incorrect character in the string.",
                Tasks::ReadField, Outcomes::Unsupported, path);
        }

        if (loadAlpha)
        {
            output.SetR8((color >> 24) & 0xff);
            output.SetG8((color >> 16) & 0xff);
            output.SetB8((color >> 8) & 0xff);
            output.SetA8(color & 0xff);
        }
        else
        {
            output.SetR8((color >> 16) & 0xff);
            output.SetG8((color >> 8) & 0xff);
            output.SetB8(color & 0xff);
        }

        return JSR::Result(settings, "Successfully loaded color from hex string.", ResultCode::Success(Tasks::ReadField), path);
    }

    uint32_t JsonColorSerializer::ClampToByteColorRange(int64_t value) const
    {
        return static_cast<uint32_t>(AZStd::clamp<int64_t>(value, 0, 255));
    }
}