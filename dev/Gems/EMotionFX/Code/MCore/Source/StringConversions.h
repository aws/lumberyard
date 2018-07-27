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

#include <AzCore/std/functional.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace MCore
{
    class Matrix;

    AZStd::string GenerateUniqueString(const char* prefix, const AZStd::function<bool(const AZStd::string& value)>& validationFunction);

    class CharacterConstants
    {
    public:
        static const char space = ' ';
        static const char tab = '\t';
        static const char endLine = '\n';
        static const char comma = ',';
        static const char dot = '.';
        static const char backSlash = '\\';
        static const char forwardSlash = '/';
        static const char semiColon = ';';
        static const char colon = ':';
        static const char doubleQuote = '\"';
        static const char dash = '-';

        static const char* wordSeparators;
    };
}

namespace AZStd
{
    void to_string(string& str, bool value);
    void to_string(string& str, const AZ::Vector2& value);
    void to_string(string& str, const AZ::Vector3& value);
    void to_string(string& str, const AZ::Vector4& value);
    void to_string(string& str, const MCore::Matrix& value);

    inline AZStd::string to_string(bool val) { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(const AZ::Vector2& val) { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(const AZ::Vector3& val) { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(const AZ::Vector4& val) { AZStd::string str; to_string(str, val); return str; }
    inline AZStd::string to_string(const MCore::Matrix& val) { AZStd::string str; to_string(str, val); return str; }
}

namespace AzFramework
{
    namespace StringFunc
    {
        bool LooksLikeVector2(const char* in, AZ::Vector2* outVector = nullptr);
        AZ::Vector2 ToVector2(const char* in);

        bool LooksLikeVector3(const char* in, AZ::Vector3* outVector = nullptr);
        AZ::Vector3 ToVector3(const char* in);

        bool LooksLikeVector4(const char* in, AZ::Vector4* outVector = nullptr);
        AZ::Vector4 ToVector4(const char* in);
    }
    
}
