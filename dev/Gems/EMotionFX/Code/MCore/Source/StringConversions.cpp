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


#include <AzCore/std/containers/vector.h>

#include "Matrix4.h"
#include "StringConversions.h"

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

    void to_string(string& str, const MCore::Matrix& value)
    {
        str = AZStd::string::format("%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f", 
            value.m16[0], value.m16[1], value.m16[2], value.m16[3],
            value.m16[4], value.m16[5], value.m16[6], value.m16[7],
            value.m16[8], value.m16[9], value.m16[10], value.m16[11],
            value.m16[12], value.m16[13], value.m16[14], value.m16[15]);
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
                float x;
                float y;
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
                float x;
                float y;
                float z;
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
                float x;
                float y;
                float z;
                float w;
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