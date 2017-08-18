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
#include "precompiled.h"
#include "Annotation.h"

namespace CodeGenerator
{
    size_t Parser::Parse(const std::string& text, AttributeList& list)
    {
        // If the string is wrapped in ()s, which it will be if it's been stringified by the annotation macro
        // strip the parens
        std::string textBuffer(text);
        if (textBuffer[0] == '(' && textBuffer[textBuffer.length()-1] == ')')
        {
            textBuffer = textBuffer.substr(1, textBuffer.length() - 2);
        }

        const char* start = textBuffer.c_str();
        const char* str = textBuffer.c_str();

        while (char ch = *str)
        {
            switch (ch)
            {
            // We no longer recursively process the contents of parenthesis.
            // The contents are instead extracted as a string and dumped raw
            // into the JSON structure for processing later during the code
            // generation process.
            case '(':
            {
                int parenCount = 1;

                // We need to ignore all parens inside strings
                bool inString = false;
                bool escapeSequence = false;

                // Store our token starting point (1 character past the opening paren)
                const char* parenStart = str + 1;

                while (*str && parenCount)
                {
                    ++str;
                    switch ( ch = *str )
                    {
                    case '\"':
                        // We need to detect when we're in strings so we can
                        // ignore any parenthesis in them.
                        if (!escapeSequence)
                        {
                            inString = !inString;
                        }
                        break;
                    case '(':
                        if (!inString)
                        {
                            ++parenCount;
                        }
                        break;
                    case ')':
                        if (!inString)
                        {
                            --parenCount;
                        }
                        break;
                    }

                    // Detect escape sequences so that we can ignore
                    // escaped quotations when doing string detection
                    if (inString && !escapeSequence && ch == '\\')
                    {
                        escapeSequence = true;
                    }
                    else
                    {
                        escapeSequence = false;
                    }
                }

                if (!(*str))
                {
                    Output::Error("Parenthesis mismatch parsing annotation: %s", text.c_str());
                    return (str - start);
                }

                // Extract the substring within the parens...
                std::string token = parenStart;
                size_t len = str - parenStart;
                token = token.substr(0, len);

                // And toss it onto the back of the list
                if (list.empty())
                {
                    Output::Error("Attribute data missing tag: %s", text.c_str());
                    return (str - start);
                }

                // Create a new Attribute to hold the substring and
                // add it to our last attribute as a parameter
                Attribute value;
                value.m_name = token;
                list.back().m_params.push_back(value);

                // Move past the last paren
                ++str;
            }
            break;

            // Ignore separators
            case ',':
            case ' ':
            case '\t':
                ++str;
                break;

                // Capture tokens
            default:
                size_t length = 0;

                if (isdigit(ch))
                {
                    length = ParseNumber(str, list);
                }
                else
                {
                    length = ParseAttribute(str, list);
                }

                str += length;
                break;
            }
        }
        return 0;
    }

    void Parser::BuildAttributeMaps(const AttributeList& list, AttributeMap& map)
    {
        for (auto& it : list)
        {
            map[it.m_name].push_back(it);
        }
    }

    void Parser::Write(IWriter* writer, const AttributeList& list)
    {
        AttributeMap attributeMap;
        BuildAttributeMaps(list, attributeMap);
        WriteMap(writer, attributeMap);
    }

    void Parser::WriteAttribute(const Attribute& attribute, rapidjson::Value& object, rapidjson::Document::AllocatorType& allocator)
    {
        switch (attribute.m_type)
        {
        case Attribute::Type::Token:
            object.SetString(attribute.m_name.c_str(), allocator);
            break;
        case Attribute::Type::Double:
            object.SetDouble(attribute.m_value.d);
            break;
        case Attribute::Type::Integer:
            object.SetInt(attribute.m_value.i);
            break;
        default:
            break;
        }
    }

    void Parser::Write(IWriter* writer, const AttributeEntry& entry, rapidjson::Value& outObj, rapidjson::Document::AllocatorType& allocator)
    {
        using namespace rapidjson;

        for (const auto& param : entry.second)
        {
            if (param.m_params.empty())
            {
                Value jsonObj(kObjectType);
                WriteAttribute(param, jsonObj, allocator);
                outObj.CopyFrom(jsonObj, allocator);
            }
            else
            {
                AttributeEntry e;
                e.first = param.m_name;
                e.second = param.m_params;

                // Create a new object
                Value parentObj(kObjectType);

                Write(writer, e, parentObj, allocator);

                Value parentKey;
                parentKey.SetString(param.m_name.c_str(), allocator);

                // If an object already exists with this key, we'll convert it into an array.
                if (outObj.HasMember(parentKey))
                {
                    Value::MemberIterator item = outObj.FindMember(parentKey);
                    // If the item isn't already an array, convert it
                    if (!item->value.IsArray())
                    {
                        // Convert member's value to an array and preserve the existing item
                        rapidjson::Value v;

                        // We rename the local object in order to know that we need to promote it before writing.
                        item->value.Swap(v);
                        item->value.SetArray();
                        item->value.PushBack(v, allocator);
                    }

                    item->value.PushBack(parentObj, allocator);
                }
                else
                {
                    outObj.AddMember(parentKey, parentObj, allocator);
                }
            }
        }
    }

    void Parser::Write(IWriter* writer, const AttributeEntry& entry, rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator)
    {
        using namespace rapidjson;

        for (const auto& param : entry.second)
        {
            // Create a new object
            Value parentObj(kObjectType);
            Value parentKey;
            parentKey.SetString(param.m_name.c_str(), allocator);

            if (param.m_params.empty())
            {
                Value jsonObj(kObjectType);
                WriteAttribute(param, jsonObj, allocator);
                if (parentKey == jsonObj)
                {
                    jsonObj.SetNull();
                }

                Value jsonKey;
                jsonKey.SetString(entry.first.c_str(), allocator);

                document.AddMember(jsonKey, jsonObj, allocator);
            }
            else
            {
                AttributeEntry e;
                e.first = param.m_name;
                e.second = param.m_params;

                Write(writer, e, parentObj, allocator);

                Value key;
                key.SetString(e.first.c_str(), allocator);

                // If an object already exists with this key, we will convert it into an array.
                auto it = document.FindMember(key);
                if (it != document.MemberEnd())
                {
                    Value::MemberIterator item = document.FindMember(key);
                    if (!item->value.IsArray())
                    {
                        // Convert member's value to an array and preserve the existing item
                        rapidjson::Value v;

                        // We rename the local object in order to know that we need to promote it before writing.
                        item->value.Swap(v);
                        item->value.SetArray();
                        item->value.PushBack(v, allocator);
                    }

                    item->value.PushBack(parentObj, allocator);
                }
                else
                {
                    document.AddMember(parentKey, parentObj, allocator);
                }
            }
        }
    }

    void Parser::WriteMap(IWriter* writer, const AttributeMap& map)
    {
        using namespace rapidjson;

        Document document;
        Document::AllocatorType& allocator = document.GetAllocator();

        document.SetObject();

        for (const auto& entries : map)
        {
            Write(writer, entries, document, allocator);
        }

        // Write the document we built into the JSON writer (DOM -> SAX)
        auto& jsonWriter = static_cast<JSONWriter<rapidjson::StringBuffer>*>(writer)->GetWriter();
        document.Accept(jsonWriter);
    }

    size_t Parser::ParseAttribute(const std::string& str, AttributeList& list)
    {
        const char* start = str.c_str();
        const char* text = str.c_str();
        size_t quoteDepth = 0; // Used to support strings
        size_t templateDepth = 0;
        bool escape = false;

        // Stop processing if we find a paren, unless we're currently looking in a string
        while (*text && ( ( quoteDepth && ( quoteDepth % 2 ) ) || (*text != '(' && *text != ')')))
        {
            if (*text == '\"' && !escape)
            {
                ++quoteDepth;
            }

            if (*text == '<')
            {
                ++templateDepth;
            }
            if (*text == '>')
            {
                --templateDepth;
            }

            const bool isDelimeter = (*text == ' ' || *text == ',' || *text == '\t');
            if (templateDepth == 0 && quoteDepth % 2 == 0 && isDelimeter)
            {
                break;
            }

            // Detect escape sequences
            if ((quoteDepth && (quoteDepth % 2)))
            {
                if (!escape && *text == '\\')
                {
                    escape = true;
                }
                else
                {
                    escape = false;
                }
            }

            ++text;
        }

        std::string token = start;
        size_t len = text - start;
        token = token.substr(0, len);
        Attribute a;
        a.m_name = token;
        list.push_back(a);

        return len;
    }

    size_t Parser::ParseNumber(const std::string& str, AttributeList& list)
    {
        const char* start = str.c_str();
        const char* text = str.c_str();

        bool isDecimal = false;
        while (*text && (isdigit(*text) || *text == '.'))
        {
            if (!isDecimal && *text == '.')
            {
                isDecimal = true;
            }

            ++text;
        }

        std::string token = start;
        size_t len = text - start;
        token = token.substr(0, len);
        Attribute a;
        a.m_name = token;
        if (isDecimal)
        {
            a.m_type = Attribute::Type::Double;
            a.m_value.d = std::atof(token.c_str());
        }
        else
        {
            a.m_type = Attribute::Type::Integer;
            a.m_value.i = std::atoi(token.c_str());
        }

        list.push_back(a);

        return len;
    }
}