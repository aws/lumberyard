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

#pragma once

#include <AzCore/JSON/document.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/any.h>

// These classes are not directly referenced in this header only because the read functions are templatized. 
// But the API is still specific to these data types so we include them here.
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

namespace AZ
{

    // [GFX TODO] This class is hopefully just a placeholder and will eventually be replaced by 
    // an improved serialization system that is still based on reflection *and* has user-friendly 
    // JSON layouts (i.e. human readable, human editable, and no duplicate keys for proper
    // compatibility with standard JSON libraries). If you need a file format that is very 
    // user friendly and the new serialization system is not available yet, this JsonReader 
    // might be a good option. Otherwise stick with the current AZ serialization system.

    //! JsonReader provides a convenient way to unpack json data while validating along the way. 
    //! AZ data types like Vector and Color supported and errors are reported through AZ Trace.
    //! 
    //! JsonReader allows the structure of client code to be very clean and concise because it ...
    //! - natively supports AZ types like Vector and Color.
    //! - performs extensive error checking internally.
    //! - only requires each key name to be specified in one place.
    //! - uses callback functions in a way that allows the client code to form a hierarchy that 
    //!   resembles the layout of the JSON file itself.
    //! 
    //! The JsonReader references one Object node in the JSON document, and provides functions for
    //! reading the members of that Object node. Basic data members (floats, ints, strings, etc.) 
    //! and arrays of basic data are read directly. When reading Object members or arrays of Objects, 
    //! the JsonReader will automatically create more JsonReaders for each child node and pass 
    //! them through callbacks to the client code.
    //! 
    //! Supported data types include bool, int32_t, uint32_t, float, AZ::Vector2, AZ::Vector3, 
    //! AZ::Vector4, AZ::Color, AZStd::string, and user-defined enum types.
    //! 
    //! Note that if JSON Schema were available, the usefulness of this class might be reduced, 
    //! but Lumberyard currently uses JSON 1.0.2 and JSON Schema wasn't introduced until 1.1.0. 
    class JsonReader final
    {
    public:
        //! @param node  The rapidjson node to be read. Client code will usually pass a parsed rapidjson::document here.
        //! @param path  The current path of 'node'. This is used for debug messages only. The format should be like "/level1/level2/myNode"
        explicit JsonReader(const rapidjson::Value& node, AZStd::string_view path = "");

        ~JsonReader();

        // (Because lambda functions are able to bind for pass by value even though the prescribed signature is pass by references,
        //  and this breaks the automatic CheckUnrecognizedKeys())
        AZ_DISABLE_COPY_MOVE(JsonReader);

        using ErrorHandler = AZStd::function<void(JsonReader& reader, AZStd::string_view jsonNodePath, AZStd::string_view message)>;

        //! The default handler for reporting errors. This is kept public so custom error handlers can add functionality
        //! while still relying on the default handler for the actual error reporting.
        static void DefaultErrorHandler(JsonReader& reader, AZStd::string_view jsonNodePath, AZStd::string_view message);

        //! Sets a custom error handler function. The default is DefaultErrorHandler().
        //! This setting will be propagated to any child JsonReaders when reading child Objects.
        void SetErrorHandler(ErrorHandler handler);

        //! Sets whether unrecognized key names in the JSON should be ignored or reported as errors.
        //! This setting will be propagated to any child JsonReaders when reading child Objects.
        //!
        //! If false, you must call CheckUnrecognizedKeys() after reading all expected fields (note, 
        //! this is only required for the root JsonReader created by client code. Any JsonReaders 
        //! passed through callback functions will do this automatically).
        void SetAllowUnrecognizedKeys(bool allow);

        //! If AllowUnrecognizedKeys is false, this will report errors for any keys that exist in the
        //! referenced JSON node but were not read or explicitly ignored by the client code.
        //! @return  true if no errors were found
        bool CheckUnrecognizedKeys();

        //! Reports an error message as AZ_Error, with the current JSON node's path automatically included, and increments the internal error count.
        //! Client code can use this for a convenient way to report custom errors messages while reading JSON content.
        template<typename ... Args>
        void ReportError(const char* format, Args... args);

        //! Returns the number of errors that were detected, including any errors detected by child JsonReaders
        //! that were created while reading child Object nodes.
        uint32_t GetErrorCount() const;

        //! Explicitly ignores a specific member of the current JSON node, so that it is not reported as 'unrecognized'.
        void Ignore(const char* key);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Read basic values and Vector/Color types

        //! Reads a required data member from the current JSON node. 
        //! Errors will be reported if the key is not found or the value does not match DataType.
        //! @return - true indicates the value was found and read successfully. false indicates that 'out' was not modified and errors were reported.
        template<typename DataType>
        bool ReadRequiredValue(const char* key, DataType& out);
        
        //! Reads an optional data member from the current JSON node. 
        //! Errors will only be reported if the key is found but the value does not match DataType.
        //! @return - true indicates the value was found and read successfully. false indicates that 'out' was not modified.
        template<typename DataType>
        bool ReadOptionalValue(const char* key, DataType& out);
        
        //! Reads an optional data member from the current JSON node. 
        //! If the member is not found, 'out' will be set to 'defaultValue'.
        //! Errors will only be reported if the key is found but the value does not match DataType.
        //! @return - true indicates the value was found and read successfully. false indicates that 'out' was set to 'defaultValue'.
        template<typename DataType>
        bool ReadOptionalValue(const char* key, DataType& out, const DataType& defaultValue);
        
        //! Reads a member Array of data from the current JSON node.
        //! Errors will be reported if any element does not match DataType.
        //! Note that Vector and Color data types are not supported (we could add support, but it's an unlikely use case so hasn't been done yet).
        //! @param callback - called for each element in the array.
        //! @return - true if no errors were detected.
        template<typename DataType>
        bool ReadArrayValues(const char* key, AZStd::function<void(size_t index, const DataType& value)> callback);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Read Any values

        //! Reads a required data member from the current JSON node.
        //! Errors will be reported if the key is not found or the value does not match DataType.
        //! @return - true indicates the value was found and read successfully. false indicates that 'out' was not modified and errors were reported.
        template<typename DataType>
        bool ReadRequiredAny(const char* key, AZStd::any& out);

        //! Reads an optional data member from the current JSON node.
        //! Errors will only be reported if the key is found but the value does not match DataType.
        //! @return - true indicates the value was found and read successfully. false indicates that 'out' was not modified.
        template<typename DataType>
        bool ReadOptionalAny(const char* key, AZStd::any& out);

        //! Reads an optional data member from the current JSON node.
        //! If the member is not found, 'out' will be set to 'defaultValue'.
        //! Errors will only be reported if the key is found but the value does not match DataType.
        //! @return - true indicates the value was found and read successfully. false indicates that 'out' was set to 'defaultValue'.
        template<typename DataType>
        bool ReadOptionalAny(const char* key, AZStd::any& out, const DataType& defaultValue);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Read Enum values

        //! Defines a callback function for parsing an enum string. If the string is identified, the function
        //! must set the 'value' and return true, otherwise return false.
        template<typename EnumType>
        using EnumStringParseFunction = AZStd::function<bool(AZStd::string_view string, EnumType& value)>;
        
        //! Reads a required enum data member from the current JSON node. 
        //! Errors will be reported if the key is not found, if the value is not a string, or if the string is not recognized by the 'parseEnumString' function.
        //! @param parseEnumString - callback function that takes a string and outputs the matching enum value.
        //! @return - true indicates the value was found and read successfully. false indicates that 'out' was not modified and errors were reported.
        template<typename EnumType>
        bool ReadRequiredEnum(const char* key, EnumStringParseFunction<EnumType> parseEnumString, EnumType& out);
        
        //! Reads an optional enum data member from the current JSON node. 
        //! Errors will be reported if the key is found but the value is not a string, or the string is not recognized by the 'parseEnumString' function.
        //! @param parseEnumString - callback function that takes a string and outputs the matching enum value.
        //! @return - true indicates the value was found and read successfully. false indicates that 'out' was not modified.
        template<typename EnumType>
        bool ReadOptionalEnum(const char* key, EnumStringParseFunction<EnumType> parseEnumString, EnumType& out);
        
        //! Reads an optional enum data member from the current JSON node. 
        //! If the member is not found, 'out' will be set to 'defaultValue'.
        //! Errors will be reported if the key is found but the value is not a string, or the string is not recognized by the 'parseEnumString' function.
        //! @param parseEnumString - callback function that takes a string and outputs the matching enum value.
        //! @return - true indicates the value was found and read successfully. false indicates that 'out' was set to 'defaultValue'.
        template<typename EnumType>
        bool ReadOptionalEnum(const char* key, EnumStringParseFunction<EnumType> parseEnumString, EnumType& out, const EnumType& defaultValue);
        
        //! Reads a member Array that contains enum values from the current JSON node.
        //! Errors will be reported if any value is not a string, or any string is not recognized by the 'parseEnumString' function.
        //! @param callback - called for each element in the array.
        //! @return - true if no errors were detected.
        template<typename EnumType>
        bool ReadArrayEnums(const char* key, EnumStringParseFunction<EnumType> parseEnumString, AZStd::function<void(size_t index, const EnumType& value)> callback);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Read objects
        
        //! Reads a required member Object from the current JSON node. 
        //! This will create a new JsonReader for the child Object node and pass it to a callback function to be read by the client.
        //! Errors will be reported if the key is not found.
        //! @return - true indicates the Object member was found and the callback was called.
        bool ReadRequiredObject(const char* key, AZStd::function<void(JsonReader& reader)> callback);
        
        //! Reads an optional member Object from the current JSON node. 
        //! This will create a new JsonReader for the child Object node and pass it to a callback function to be read by the client.
        //! @return - true indicates the Object member was found and the callback was called.
        bool ReadOptionalObject(const char* key, AZStd::function<void(JsonReader& reader)> callback);
        
        //! Reads a member Array of Objects from the current JSON node. 
        //! This will create a new JsonReader for each child Object in the Array and pass it to a callback function to be read by the client.
        //! @return - true if no errors were detected.
        bool ReadObjectArray(const char* key, AZStd::function<void(size_t index, JsonReader& reader)> callback);

        //! If the current JSON node is an object, runs the 'callback' for each member. This is helpful in situations where you don't
        //! know exactly what keys are expected and need to discover them. The callback's JsonReader is the same as 'this' so the
        //! callback can call "reader.ReadRequiredValue(key, ...)" to read the members.
        void ReadMembers(AZStd::function<void(const char* key, JsonReader& reader)> callback);

    private:

        static const char* Delimiter;
        static const char CommentIndicator;

        enum class Required
        {
            Yes,
            No
        };

        //! Checks that the current JSON node contains a specific key and that it has the expected type. Reports errors otherwise.
        bool CheckMember(Required required, const char* key, bool (rapidjson::Value::*isTypeFuncPtr)() const, const char* expectedTypeName);

        //! Checks that the current JSON node contains a specific key, that the value is an array, and that it has the right number of elements. Reports errors otherwise.
        bool CheckMemberArray(Required required, const char* key, size_t minCount, size_t maxCount);

        //! Common implementation for ReadRequiredValue() and ReadOptionalValue() functions.
        template<typename JsonType>
        bool ReadImpl(Required required, const char* key, bool (rapidjson::Value::*isTypeFuncPtr)() const, JsonType(rapidjson::Value::*readValue)() const, const char* expectedTypeName, JsonType& out);

        //! Common implementation for ReadArrayValues() functions.
        template<typename JsonType>
        bool ReadArrayImpl(const char* key, bool (rapidjson::Value::*elementIsTypeFuncPtr)() const, JsonType(rapidjson::Value::*readValue)() const, const char* expectedTypeName, AZStd::function<bool(size_t index, JsonType value)> callback);

        //! Common implementation for ReadRequiredValue() and ReadOptionalValue() functions, specifically for AZ::Vector and AZ::Color data types.
        template<typename VectorType, size_t MinCount, size_t MaxCount>
        size_t ReadVectorImpl(Required required, const char* key, VectorType& out);

        //! Common implementation for ReadRequiredEnum() and ReadOptionalEnum() functions.
        template<typename EnumType>
        bool ReadEnumImpl(Required required, const char* key, EnumStringParseFunction<EnumType> parseEnumString, EnumType& out);

        const rapidjson::Value& m_node;                         //!< The current JSON node represented by this JsonReader
        uint32_t m_errorCount = 0;                              //!< The accumulated number of errors detected, included errors detected by child JsonReaders
        ErrorHandler m_errorHandler = &DefaultErrorHandler;     //!< Error handler that will report error messages.
        AZStd::string m_path;                                   //!< The path of the current node (e.g. "/properties[2]/metadata/description")
        AZStd::unordered_set<AZStd::string> m_recognizedKeys;   //!< The set of all keys that the client read (or explicitly ignored) in this JSON node
        bool m_allowUnrecognizedFields = true;                  //!< If false, the JsonReader will report errors for members that don't appear in m_recognizedKeys
        bool m_checkedUnrecognizedFields = false;               //!< Tracks whether CheckUnrecognizedKeys() was called
    };

    template<typename ... Args>
    void JsonReader::ReportError(const char* format, Args... args)
    {
        // If we were to wrap the m_errorHandler code in AZ_ENABLE_TRACING then
        // some compilers complain about args: "parameter pack must be expanded in this context".
        bool tracingEnabled = false;
#ifdef AZ_ENABLE_TRACING
        tracingEnabled = true;
#endif

        ++m_errorCount;
        if (m_errorHandler && tracingEnabled)
        {
            AZStd::string message = AZStd::string::format(format, args...);
            m_errorHandler(*this, m_path, message);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Common Read function implementations

    template<typename JsonType>
    bool JsonReader::ReadImpl(
        Required required,
        const char* key,
        bool (rapidjson::Value::*isTypeFuncPtr)() const,
        JsonType(rapidjson::Value::*readValue)() const,
        const char* expectedTypeName,
        JsonType& out)
    {
        if (CheckMember(required, key, isTypeFuncPtr, expectedTypeName))
        {
            out = (m_node[key].*readValue)();
            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename JsonType>
    bool JsonReader::ReadArrayImpl(
        const char* key,
        bool (rapidjson::Value::*elementIsTypeFuncPtr)() const,
        JsonType(rapidjson::Value::*readValue)() const,
        const char* expectedTypeName,
        AZStd::function<bool(size_t index, JsonType value)> callback)
    {
        if (CheckMember(Required::No, key, &rapidjson::Value::IsArray, "array"))
        {
            const rapidjson::Value& array = m_node[key];
            int index = 0;
            for (auto item = array.Begin(); item != array.End(); ++item, ++index)
            {
                if (!(*item.*elementIsTypeFuncPtr)())
                {
                    ReportError("Field '%s[%d]' must be type '%s'.", key, index, expectedTypeName);
                    return false;
                }
                else
                {
                    JsonType value = (*item.*readValue)();
                    if (!callback(index, value))
                    {
                        return false;
                    }
                }
            }

            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename VectorType, size_t MinCount, size_t MaxCount>
    size_t JsonReader::ReadVectorImpl(Required required, const char* key, VectorType& out)
    {
        size_t result = 0;

        if (CheckMemberArray(required, key, MinCount, MaxCount))
        {
            VectorType vectorValue;
            size_t numValuesRead = 0;

            bool readArrayResult = ReadArrayValues<float>(key, [&](size_t index, const float& value)
            {
                vectorValue.SetElement(static_cast<int>(index), value);
                ++numValuesRead;
            });

            if (readArrayResult)
            {
                out = vectorValue;
                result = numValuesRead;
            }
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read basic values and Vector/Color types

    // This overload simply wraps the other one, so it doesn't need to be specialized.
    template<typename DataType>
    bool JsonReader::ReadOptionalValue(const char* key, DataType& out, const DataType& defaultValue)
    {
        out = defaultValue;
        return ReadOptionalValue(key, out);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read Any values 

    template<typename DataType>
    bool JsonReader::ReadRequiredAny(const char* key, AZStd::any& out)
    {
        DataType data;
        if (ReadRequiredValue(key, data))
        {
            out = data;
            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename DataType>
    bool JsonReader::ReadOptionalAny(const char* key, AZStd::any& out)
    {
        DataType data;
        if (ReadOptionalValue(key, data))
        {
            out = data;
            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename DataType>
    bool JsonReader::ReadOptionalAny(const char* key, AZStd::any& out, const DataType& defaultValue)
    {
        out = defaultValue;
        return ReadOptionalAny<DataType>(key, out);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read Enum values

    template<typename EnumType>
    bool JsonReader::ReadEnumImpl(Required required, const char* key, EnumStringParseFunction<EnumType> parseEnumString, EnumType& out)
    {
        const char* stringValue;

        if (!ReadImpl<const char*>(required, key, &rapidjson::Value::IsString, &rapidjson::Value::GetString, "string", stringValue))
        {
            return false;
        }

        if (!parseEnumString(stringValue, out))
        {
            ReportError("Field '%s' has unrecognized value '%s'", key, stringValue);
            return false;
        }

        return true;
    }

    template<typename EnumType>
    bool JsonReader::ReadRequiredEnum(const char* key, EnumStringParseFunction<EnumType> parseEnumString, EnumType& out)
    {
        return ReadEnumImpl<EnumType>(Required::Yes, key, parseEnumString, out);
    }

    template<typename EnumType>
    bool JsonReader::ReadOptionalEnum(const char* key, EnumStringParseFunction<EnumType> parseEnumString, EnumType& out)
    {
        return ReadEnumImpl<EnumType>(Required::No, key, parseEnumString, out);
    }

    template<typename EnumType>
    bool JsonReader::ReadOptionalEnum(const char* key, EnumStringParseFunction<EnumType> parseEnumString, EnumType& out, const EnumType& defaultValue)
    {
        out = defaultValue;
        return ReadEnumImpl<EnumType>(Required::No, key, parseEnumString, out);
    }

    template<typename EnumType>
    bool JsonReader::ReadArrayEnums(const char* key, EnumStringParseFunction<EnumType> parseEnumString, AZStd::function<void(size_t index, const EnumType& value)> callback)
    {
        bool result = ReadArrayImpl<const char*>(key, &rapidjson::Value::IsString, &rapidjson::Value::GetString, "string",
            [&](size_t index, const char* stringValue)
            {
                EnumType enumValue;
                if (!parseEnumString(stringValue, enumValue))
                {
                    ReportError("Field '%s' has unrecognized value '%s'", key, stringValue);
                    return false; // Stop looping through the array
                }
                else
                {
                    callback(index, enumValue);
                    return true;
                }
            });

        return result;
    }
    
    namespace Internal
    {
        using RapidJsonParseFn = rapidjson::Document&(rapidjson::Document::*)(const rapidjson::Document::Ch*);
        Outcome<rapidjson::Document, AZStd::string> LoadJsonFromFileImpl(AZStd::string_view filePath, RapidJsonParseFn parseFn);
    }

    //! Read the file specified (using FileIOBase::GetInstance()), and parse to a JSON document.
    template <unsigned parseFlags>
    inline Outcome<rapidjson::Document, AZStd::string> LoadJsonFromFile(AZStd::string_view filePath)
    {
        return Internal::LoadJsonFromFileImpl(AZStd::move(filePath), static_cast<Internal::RapidJsonParseFn>(&rapidjson::Document::Parse<parseFlags>));
    }

    inline Outcome<rapidjson::Document, AZStd::string> LoadJsonFromFile(AZStd::string_view filePath)
    {
        return LoadJsonFromFile<rapidjson::kParseDefaultFlags>(AZStd::move(filePath));
    }

} // namespace AZ