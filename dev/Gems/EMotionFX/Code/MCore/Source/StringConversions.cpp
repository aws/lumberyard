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


#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>

#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>


namespace MCore
{
    const char* CharacterConstants::wordSeparators = " \n\t";

    AZStd::string GenerateUniqueString(const char* prefix, const AZStd::function<bool(const AZStd::string& value)>& validationFunction)
    {
        MCORE_ASSERT(validationFunction);
        const AZStd::string prefixString = prefix;

        // find the last letter index from the right
        size_t lastIndex = AZStd::string::npos;
        const size_t numCharacters = prefixString.size();
        for (size_t i = numCharacters - 1; i >= 0; --i)
        {
            if (!AZStd::is_digit(prefixString[i]))
            {
                lastIndex = i + 1;
                break;
            }
        }

        // copy the string
        AZStd::string nameWithoutLastDigits = prefixString.substr(0, lastIndex);

        // remove all space on the right
        AzFramework::StringFunc::TrimWhiteSpace(nameWithoutLastDigits, false /* leading */, true /* trailing */);

        // generate the unique name
        uint32 nameIndex = 0;
        AZStd::string uniqueName = nameWithoutLastDigits + "0";
        while (validationFunction(uniqueName) == false)
        {
            uniqueName = nameWithoutLastDigits + AZStd::to_string(++nameIndex);
        }

        return uniqueName;
    }

    AZStd::string ConstructStringSeparatedBySemicolons(const AZStd::vector<AZStd::string>& stringVec)
    {
        AZStd::string result;

        for (const AZStd::string& currentString : stringVec)
        {
            if (!result.empty())
            {
                result += CharacterConstants::semiColon;
            }

            result += currentString;
        }

        return result;
    }
}

namespace AZStd
{
    void to_string(string& str, bool value)
    {
        str = value ? "true" : "false";
    }

    void to_string(string& str, const AZ::Vector2& value)
    {
        str = AZStd::string::format("%.8f,%.8f", 
            static_cast<float>(value.GetX()), 
            static_cast<float>(value.GetY()));
    }

    void to_string(string& str, const AZ::Vector3& value)
    {
        str = AZStd::string::format("%.8f,%.8f,%.8f", 
            static_cast<float>(value.GetX()), 
            static_cast<float>(value.GetY()), 
            static_cast<float>(value.GetZ()));
    }

    void to_string(string& str, const AZ::Vector4& value)
    {
        str = AZStd::string::format("%.8f,%.8f,%.8f,%.8f", 
            static_cast<float>(value.GetX()), 
            static_cast<float>(value.GetY()), 
            static_cast<float>(value.GetZ()), 
            static_cast<float>(value.GetW()));
    }
    
    void to_string(string& str, const AZ::Quaternion& value)
    {
        str = AZStd::string::format("%.8f,%.8f,%.8f,%.8f",
            static_cast<float>(value.GetX()),
            static_cast<float>(value.GetY()),
            static_cast<float>(value.GetZ()),
            static_cast<float>(value.GetW()));
    }

    void to_string(string& str, const AZ::Matrix4x4& value)
    {
        str = AZStd::string::format("%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f", 
            static_cast<float>(value(0, 0)), static_cast<float>(value(1, 0)), static_cast<float>(value(2, 0)), static_cast<float>(value(3, 0)),
            static_cast<float>(value(0, 1)), static_cast<float>(value(1, 1)), static_cast<float>(value(2, 1)), static_cast<float>(value(3, 1)),
            static_cast<float>(value(0, 2)), static_cast<float>(value(1, 2)), static_cast<float>(value(2, 2)), static_cast<float>(value(3, 2)),
            static_cast<float>(value(0, 3)), static_cast<float>(value(1, 3)), static_cast<float>(value(2, 3)), static_cast<float>(value(3, 3)));
    }

    void to_string(string& str, const AZ::Transform& value)
    {
        str = AZStd::string::format("%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f",
            static_cast<float>(value(0, 0)), static_cast<float>(value(1, 0)), static_cast<float>(value(2, 0)),
            static_cast<float>(value(0, 1)), static_cast<float>(value(1, 1)), static_cast<float>(value(2, 1)),
            static_cast<float>(value(0, 2)), static_cast<float>(value(1, 2)), static_cast<float>(value(2, 2)),
            static_cast<float>(value(0, 3)), static_cast<float>(value(1, 3)), static_cast<float>(value(2, 3)));
    }
}

namespace AzFramework
{
    namespace StringFunc
    {
        bool LooksLikeVector2(const char* in, AZ::Vector2* outVector)
        {
            AZStd::vector<AZStd::string> tokens;
            AzFramework::StringFunc::Tokenize(in, tokens, MCore::CharacterConstants::comma, false, true);
            if (tokens.size() == 2)
            {
                float x = 0.0f;
                float y = 0.0f;
                if (!AzFramework::StringFunc::LooksLikeFloat(tokens[0].c_str(), outVector ? &x : nullptr))
                {
                    return false;
                }
                if (!AzFramework::StringFunc::LooksLikeFloat(tokens[1].c_str(), outVector ? &y : nullptr))
                {
                    return false;
                }
                if (outVector)
                {
                    outVector->Set(x, y);
                }
                return true;
            }
            return false;
        }

        AZ::Vector2 ToVector2(const char* in)
        {
            AZ::Vector2 vector;
            LooksLikeVector2(in, &vector);
            return vector;
        }

        bool LooksLikeVector3(const char* in, AZ::Vector3* outVector)
        {
            AZStd::vector<AZStd::string> tokens;
            AzFramework::StringFunc::Tokenize(in, tokens, MCore::CharacterConstants::comma, false, true);
            if (tokens.size() == 3)
            {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                if (!AzFramework::StringFunc::LooksLikeFloat(tokens[0].c_str(), outVector ? &x : nullptr))
                {
                    return false;
                }
                if (!AzFramework::StringFunc::LooksLikeFloat(tokens[1].c_str(), outVector ? &y : nullptr))
                {
                    return false;
                }
                if (!AzFramework::StringFunc::LooksLikeFloat(tokens[2].c_str(), outVector ? &z : nullptr))
                {
                    return false;
                }
                if (outVector)
                {
                    outVector->Set(x, y, z);
                }
                return true;
            }
            return false;
        }

        AZ::Vector3 ToVector3(const char* in)
        {
            AZ::Vector3 vector;
            LooksLikeVector3(in, &vector);
            return vector;
        }

        bool LooksLikeVector4(const char* in, AZ::Vector4* outVector)
        {
            AZStd::vector<AZStd::string> tokens;
            AzFramework::StringFunc::Tokenize(in, tokens, MCore::CharacterConstants::comma, false, true);
            if (tokens.size() == 4)
            {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                float w = 0.0f;
                if (!AzFramework::StringFunc::LooksLikeFloat(tokens[0].c_str(), outVector ? &x : nullptr))
                {
                    return false;
                }
                if (!AzFramework::StringFunc::LooksLikeFloat(tokens[1].c_str(), outVector ? &y : nullptr))
                {
                    return false;
                }
                if (!AzFramework::StringFunc::LooksLikeFloat(tokens[2].c_str(), outVector ? &z : nullptr))
                {
                    return false;
                }
                if (!AzFramework::StringFunc::LooksLikeFloat(tokens[3].c_str(), outVector ? &w : nullptr))
                {
                    return false;
                }
                if (outVector)
                {
                    outVector->Set(x, y, z, w);
                }
                return true;
            }
            return false;
        }

        AZ::Vector4 ToVector4(const char* in)
        {
            AZ::Vector4 vector;
            LooksLikeVector4(in, &vector);
            return vector;
        }
    }
}
