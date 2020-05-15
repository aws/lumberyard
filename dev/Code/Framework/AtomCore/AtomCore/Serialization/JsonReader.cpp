/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AtomCore/Serialization/JsonReader.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/JSON/error/en.h>

namespace AZ
{

    const char* JsonReader::Delimiter = "/";
    const char JsonReader::CommentIndicator = '#';

    JsonReader::JsonReader(const rapidjson::Value& node, AZStd::string_view path)
        : m_node(node)
        , m_path(path)
    {
    }

    JsonReader::~JsonReader()
    {
        // Calling CheckUnrecognizedKeys() is required because the client explicitly disallowed unrecognized keys by calling SetAllowUnrecognizedKeys(false). 
        // If they do this but they forget to call CheckUnrecognizedKeys() too, that's a programmer error.
        AZ_Assert(m_allowUnrecognizedFields || m_checkedUnrecognizedFields, "JsonReader was shut down without checking for unrecognized keys.");
    }

    void JsonReader::SetErrorHandler(ErrorHandler handler)
    {
        m_errorHandler = handler;
    }

    void JsonReader::DefaultErrorHandler(JsonReader& reader, AZStd::string_view jsonNodePath, AZStd::string_view message)
    {
        AZ_UNUSED(reader);
        AZ_UNUSED(message);

        if (jsonNodePath.empty())
        {
            AZ_Error("JsonReader", false, "%s", message.data());
        }
        else
        {
            AZ_Error("JsonReader", false, "Error in '%s': %s", jsonNodePath.data(), message.data());
        }
    }

    void JsonReader::SetAllowUnrecognizedKeys(bool allow)
    {
        m_allowUnrecognizedFields = allow;
    }

    bool JsonReader::CheckUnrecognizedKeys()
    {
        bool allClear = true;

        if (!m_allowUnrecognizedFields && m_node.IsObject())
        {
            for (auto item = m_node.MemberBegin(); item != m_node.MemberEnd(); ++item)
            {
                AZStd::string currentKey = item->name.GetString();
                if (m_recognizedKeys.end() == m_recognizedKeys.find(currentKey))
                {
                    const bool isComment = !currentKey.empty() && currentKey[0] == CommentIndicator;
                    if (!isComment)
                    {
                        ReportError("Field '%s' is not recognized.", currentKey.data());
                        allClear = false;
                    }
                }
            }
        }

        m_checkedUnrecognizedFields = true;

        return allClear;
    }

    uint32_t JsonReader::GetErrorCount() const
    {
        return m_errorCount;
    }

    void JsonReader::Ignore(const char* key)
    {
        m_recognizedKeys.insert(key);
    }

    bool JsonReader::CheckMember(Required required, const char* key, bool (rapidjson::Value::*isTypeFuncPtr)() const, const char* expectedTypeName)
    {
        if (!m_node.IsObject() || !m_node.HasMember(key))
        {
            if (required == Required::Yes)
            {
                ReportError("Field '%s' not found.", key);
            }

            return false;
        }
        else if (!(m_node[key].*isTypeFuncPtr)())
        {
            ReportError("Field '%s' must be type '%s'.", key, expectedTypeName);
            m_recognizedKeys.insert(key); // Even though the type is wrong, this key is still recognized
            return false;
        }
        else
        {
            m_recognizedKeys.insert(key);
            return true;
        }
    }

    bool JsonReader::CheckMemberArray(Required required, const char* key, size_t minCount, size_t maxCount)
    {
        AZ_Assert(minCount <= maxCount, "minCount and maxCount are flipped");

        if (!CheckMember(required, key, &rapidjson::Value::IsArray, "array"))
        {
            return false;
        }

        bool errorFound = false;

        size_t count = m_node[key].Size();
        if (count < minCount || maxCount < count)
        {
            if (minCount == maxCount)
            {
                ReportError("Array '%s' must have %d elements but has %d.", key, minCount, count);
                errorFound = true;
            }
            else
            {
                ReportError("Array '%s' must have %d-%d elements but has %d.", key, minCount, maxCount, count);
                errorFound = true;
            }
        }

        // Note, we don't need to check the type of each element because ReadArrayImpl() handles that

        return !errorFound;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read bool

    template<>
    bool JsonReader::ReadRequiredValue<bool>(const char* key, bool& out)
    {
        return ReadImpl<bool>(Required::Yes, key, &rapidjson::Value::IsBool, &rapidjson::Value::GetBool, "bool", out);
    }

    template<>
    bool JsonReader::ReadOptionalValue<bool>(const char* key, bool& out)
    {
        return ReadImpl<bool>(Required::No, key, &rapidjson::Value::IsBool, &rapidjson::Value::GetBool, "bool", out);
    }

    template<>
    bool JsonReader::ReadArrayValues<bool>(const char* key, AZStd::function<void(size_t index, const bool& value)> callback)
    {
        return ReadArrayImpl<bool>(key, &rapidjson::Value::IsBool, &rapidjson::Value::GetBool, "bool", 
            [&](size_t index, bool value) 
            { 
                callback(index, value); 
                return true; 
            });
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read int32_t

    template<>
    bool JsonReader::ReadRequiredValue<int32_t>(const char* key, int32_t& out)
    {
        return ReadImpl<int32_t>(Required::Yes, key, &rapidjson::Value::IsInt, &rapidjson::Value::GetInt, "int", out);
    }

    template<>
    bool JsonReader::ReadOptionalValue<int32_t>(const char* key, int32_t& out)
    {
        return ReadImpl<int32_t>(Required::No, key, &rapidjson::Value::IsInt, &rapidjson::Value::GetInt, "int", out);
    }

    template<>
    bool JsonReader::ReadArrayValues<int32_t>(const char* key, AZStd::function<void(size_t index, const int32_t& value)> callback)
    {
        return ReadArrayImpl<int32_t>(key, &rapidjson::Value::IsInt, &rapidjson::Value::GetInt, "int", 
            [&](size_t index, int32_t value)
            { 
                callback(index, value); 
                return true;
            });
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read uint32_t

    template<>
    bool JsonReader::ReadRequiredValue<uint32_t>(const char* key, uint32_t& out)
    {
        return ReadImpl<uint32_t>(Required::Yes, key, &rapidjson::Value::IsUint, &rapidjson::Value::GetUint, "uint", out);
    }

    template<>
    bool JsonReader::ReadOptionalValue<uint32_t>(const char* key, uint32_t& out)
    {
        return ReadImpl<uint32_t>(Required::No, key, &rapidjson::Value::IsUint, &rapidjson::Value::GetUint, "uint", out);
    }

    template<>
    bool JsonReader::ReadArrayValues<uint32_t>(const char* key, AZStd::function<void(size_t index, const uint32_t& value)> callback)
    {
        return ReadArrayImpl<uint32_t>(key, &rapidjson::Value::IsUint, &rapidjson::Value::GetUint, "uint", 
            [&](size_t index, uint32_t value) 
            { 
                callback(index, value); 
                return true;
            });
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read float

    template<>
    bool JsonReader::ReadRequiredValue<float>(const char* key, float& out)
    {
        double value;
        bool result = ReadImpl<double>(Required::Yes, key, &rapidjson::Value::IsDouble, &rapidjson::Value::GetDouble, "float", value);

        if (result)
        {
            out = static_cast<float>(value);
        }

        return result;
    }

    template<>
    bool JsonReader::ReadOptionalValue<float>(const char* key, float& out)
    {
        double value = out;
        bool result = ReadImpl<double>(Required::No, key, &rapidjson::Value::IsDouble, &rapidjson::Value::GetDouble, "float", value);
        out = static_cast<float>(value);
        return result;
    }

    template<>
    bool JsonReader::ReadArrayValues<float>(const char* key, AZStd::function<void(size_t index, const float& value)> callback)
    {
        return ReadArrayImpl<double>(key, &rapidjson::Value::IsDouble, &rapidjson::Value::GetDouble, "float", 
            [&](size_t index, double value) 
            { 
                callback(index, static_cast<float>(value)); 
                return true;
            });
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read Vector2

    template<>
    bool JsonReader::ReadRequiredValue<Vector2>(const char* key, Vector2& out)
    {
        return 2 == ReadVectorImpl<Vector2, 2, 2>(Required::Yes, key, out);
    }

    template<>
    bool JsonReader::ReadOptionalValue<Vector2>(const char* key, Vector2& out)
    {
        return 2 == ReadVectorImpl<Vector2, 2, 2>(Required::No, key, out);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read Vector3

    template<>
    bool JsonReader::ReadRequiredValue<Vector3>(const char* key, Vector3& out)
    {
        return 3 == ReadVectorImpl<Vector3, 3, 3>(Required::Yes, key, out);
    }

    template<>
    bool JsonReader::ReadOptionalValue<Vector3>(const char* key, Vector3& out)
    {
        return 3 == ReadVectorImpl<Vector3, 3, 3>(Required::No, key, out);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read Vector4

    template<>
    bool JsonReader::ReadRequiredValue<Vector4>(const char* key, Vector4& out)
    {
        return 4 == ReadVectorImpl<Vector4, 4, 4>(Required::Yes, key, out);
    }

    template<>
    bool JsonReader::ReadOptionalValue<Vector4>(const char* key, Vector4& out)
    {
        return 4 == ReadVectorImpl<Vector4, 4, 4>(Required::No, key, out);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read Color

    template<>
    bool JsonReader::ReadRequiredValue<Color>(const char* key, Color& out)
    {
        size_t readResult = ReadVectorImpl<Color, 3, 4>(Required::Yes, key, out);
        if (readResult == 3)
        {
            out.SetElement(3, 1.0f);
        }

        return readResult == 3 || readResult == 4;
    }

    template<>
    bool JsonReader::ReadOptionalValue<Color>(const char* key, Color& out)
    {
        size_t readResult = ReadVectorImpl<Color, 3, 4>(Required::No, key, out);
        if (readResult == 3)
        {
            out.SetElement(3, 1.0f);
        }

        return readResult == 3 || readResult == 4;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read AZStd::string

    template<>
    bool JsonReader::ReadRequiredValue<AZStd::string>(const char* key, AZStd::string& out)
    {
        const char* value = "";

        bool result = ReadImpl<const char*>(Required::Yes, key, &rapidjson::Value::IsString, &rapidjson::Value::GetString, "string", value);

        if (result)
        {
            out = value;
        }

        return result;
    }

    template<>
    bool JsonReader::ReadOptionalValue<AZStd::string>(const char* key, AZStd::string& out)
    {
        const char* value;

        bool result = ReadImpl<const char*>(Required::No, key, &rapidjson::Value::IsString, &rapidjson::Value::GetString, "string", value);

        if (result)
        {
            out = value;
        }

        return result;
    }

    template<>
    bool JsonReader::ReadArrayValues<AZStd::string>(const char* key, AZStd::function<void(size_t index, const AZStd::string& value)> callback)
    {
        return ReadArrayImpl<const char*>(key, &rapidjson::Value::IsString, &rapidjson::Value::GetString, "string", 
            [&](size_t index, const char* value) 
            { 
                callback(index, value); 
                return true;
            });
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read objects

    bool JsonReader::ReadRequiredObject(const char* key, AZStd::function<void(JsonReader& reader)> callback)
    {
        if (CheckMember(Required::Yes, key, &rapidjson::Value::IsObject, "object"))
        {
            JsonReader reader(m_node[key], m_path + Delimiter + key);
            reader.SetErrorHandler(m_errorHandler);
            reader.SetAllowUnrecognizedKeys(m_allowUnrecognizedFields);
            callback(reader);
            reader.CheckUnrecognizedKeys();
            m_errorCount += reader.GetErrorCount();
            return true;
        }
        else
        {
            return false;
        }
    }

    bool JsonReader::ReadOptionalObject(const char* key, AZStd::function<void(JsonReader& reader)> callback)
    {
        if (CheckMember(Required::No, key, &rapidjson::Value::IsObject, "object"))
        {
            JsonReader reader(m_node[key], m_path + Delimiter + key);
            reader.SetErrorHandler(m_errorHandler);
            reader.SetAllowUnrecognizedKeys(m_allowUnrecognizedFields);
            callback(reader);
            reader.CheckUnrecognizedKeys();
            m_errorCount += reader.GetErrorCount();
            return true;
        }
        else
        {
            return false;
        }
    }

    bool JsonReader::ReadObjectArray(const char* key, AZStd::function<void(size_t index, JsonReader& reader)> callback)
    {
        if (CheckMember(Required::No, key, &rapidjson::Value::IsArray, "array"))
        {
            const rapidjson::Value& array = m_node[key];
            int index = 0;
            for (auto item = array.Begin(); item != array.End(); ++item, ++index)
            {
                JsonReader reader(*item, AZStd::string::format("%s%s%s[%d]", m_path.data(), Delimiter, key, index));
                reader.SetErrorHandler(m_errorHandler);
                reader.SetAllowUnrecognizedKeys(m_allowUnrecognizedFields);
                callback(index, reader);
                reader.CheckUnrecognizedKeys();
                m_errorCount += reader.GetErrorCount();
            }

            return true;
        }
        else
        {
            return false;
        }
    }

    void JsonReader::ReadMembers(AZStd::function<void(const char* key, JsonReader& reader)> callback)
    {
        for (auto member = m_node.MemberBegin(); member != m_node.MemberEnd(); ++member)
        {
            bool isComment = member->name.GetString() && member->name.GetString()[0] == CommentIndicator;
            if (!isComment)
            {
                callback(member->name.GetString(), *this);
            }
        }
    }
    
    namespace Internal
    {
        Outcome<rapidjson::Document, AZStd::string> LoadJsonFromFileImpl(AZStd::string_view filePath, RapidJsonParseFn parseFn)
        {
            IO::FileIOStream file;
            if (!file.Open(filePath.data(), IO::OpenMode::ModeRead))
            {
                return AZ::Failure(AZStd::string::format("Failed to open '%s'. Please ensure file exists.", filePath.data()));
            }

            AZ::u64 fileSize = file.GetLength();
            AZ_Verify(fileSize > 0, "Failed to get config/mainview.json file size.");

            AZStd::string documentJson(fileSize, 0);
            if (file.Read(file.GetLength(), documentJson.data()) != file.GetLength())
            {
                return AZ::Failure(AZStd::string::format("Failed to read '%s'. Please ensure file is valid.", filePath.data()));
            }

            rapidjson::Document document;
            (document.*parseFn)(documentJson.c_str());
            if (document.HasParseError())
            {
                int lineNumber = 1;

                const AZStd::size_t errorOffset = document.GetErrorOffset();
                for (AZStd::size_t searchOffset = documentJson.find('\n');
                    searchOffset < errorOffset && searchOffset < AZStd::string::npos;
                    searchOffset = documentJson.find('\n', searchOffset + 1))
                {
                    lineNumber++;
                }

                return AZ::Failure(AZStd::string::format("Failed to parse '%s' line %d. %s", filePath.data(), lineNumber, rapidjson::GetParseError_En(document.GetParseError())));
            }

            return AZ::Success(AZStd::move(document));
        }
    } // namespace Internal

} // namespace AZ