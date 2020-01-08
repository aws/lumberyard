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

#include <AzCore/JSON/document.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    class BaseJsonSerializer;
    class JsonRegistrationContext;
    class SerializeContext;

    using JsonIssueCallback = AZStd::function<JsonSerializationResult::ResultCode(AZStd::string_view message, 
        JsonSerializationResult::ResultCode result, AZStd::string_view path)>;

    //! Optional settings used while loading a json document to an object.
    struct JsonDeserializerSettings final
    {
        //! Optional serialize context. If not provided the default instance will be retrieved through an EBus call.
        SerializeContext* m_serializeContext = nullptr;
        //! Optional json registration context. If not provided the default instance will be retrieved through an EBus call.
        JsonRegistrationContext* m_registrationContext = nullptr;
        //! Optional callback when issues are encountered. If not provided reporting will be forwarded to the default issue reporting.
        //! This can also be used to change the returned result code to alter the behavior of the deserializer.
        JsonIssueCallback m_reporting;
    };
    
    //! Optional settings used while storing an object to a json document.
    struct JsonSerializerSettings final
    {
        //! If true default value will be stored, otherwise only changed values will be stored.
        bool m_keepDefaults = false;

        //! Optional serialize context. If not provided the default instance will be retrieved through an EBus call.
        SerializeContext* m_serializeContext = nullptr;
        //! Optional json registration context. If not provided the default instance will be retrieved through an EBus call.
        JsonRegistrationContext* m_registrationContext = nullptr;
        //! Optional callback when issues are encountered. If not provided reporting will be forwarded to the default issue reporting.
        //! This can also be used to change the returned result code to alter the behavior of the serializer.
        JsonIssueCallback m_reporting;
    };

    enum class JsonSerializerCompareResult
    {
        Less,
        Equal,
        Greater,
        Error
    };
    
    //! Core class to handle serialization to and from json documents.
    class JsonSerialization final
    {
    public:
        static const char* TypeIdFieldIdentifier;
        static const char* DefaultStringIdentifier;
        static const char* KeyFieldIdentifier;
        static const char* ValueFieldIdentifier;

        //! Loads the data from the provided json value into the supplied object. The object is expected to be created before calling load.
        //! @param object Object where the data will be loaded into.
        //! @param root The Value or Document where the deserializer will start reading data from.
        //! @param settings The settings used during deserialization. Use the value passed in from Load.
        template<typename T>
        static JsonSerializationResult::ResultCode Load(T& object, const rapidjson::Value& root,
            JsonDeserializerSettings settings = JsonDeserializerSettings{});
        //! Loads the data from the provided json value into the supplied object. The object is expected to be created before calling load.
        //! @param object Pointer to the object where the data will be loaded into.
        //! @param objectType Type id of the object passed in.
        //! @param root The Value or Document from where the deserializer will start reading data.
        //! @param settings The settings used during deserialization. Use the value passed in from Load.
        static JsonSerializationResult::ResultCode Load(void* object, const Uuid& objectType, const rapidjson::Value& root,
            JsonDeserializerSettings settings = JsonDeserializerSettings{});

        //! Loads the type id from the provided input.
        //! Note: it's not recommended to use this function (frequently) as it requires users of the json file to have knowledge of the internal
        //!     type structure and is therefore harder to use.
        //! @param typeId The uuid where the loaded data will be written to. If loading fails this will be a null uuid.
        //! @param input The json node to load from. The node is expected to contain a string.
        //! @param baseClassTypeId. An optional type id for the base class, if known. If a type name is stored in the string which
        //!     references multiple types then the baseClassTypeId will be used to disambiguate between the different types by looking
        //!     if exactly one of the types inherits from the base class that baseClassTypeId points to.
        //! @param jsonPath An optional path to the json node. This will be used for reporting.
        //! @param settings An optional settings object to change where this function collects information from. This can be same settings
        //!     as used for the other Load functions.
        static JsonSerializationResult::ResultCode LoadTypeId(Uuid& typeId, const rapidjson::Value& input, 
            const Uuid* baseClassTypeId = nullptr, AZStd::string_view jsonPath = AZStd::string_view{}, 
            JsonDeserializerSettings settings = JsonDeserializerSettings{});

        //! Stores the data in the provided object as json values starting at the provided value.
        //! @param output The Value or Document where the converted data will start writing to.
        //! @param allocator The memory allocator used by RapidJSON to create the json document.
        //! @param object The object that will be read from for values to convert.
        //! @param settings The settings used during serialization. Use the value passed in from Store.
        template<typename T>
        static JsonSerializationResult::ResultCode Store(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
            const T& object, JsonSerializerSettings settings = JsonSerializerSettings{});
        
        //! Stores the data in the provided object as json values starting at the provided value.
        //! @param output The Value or Document where the converted data will start writing to.
        //! @param allocator The memory allocator used by RapidJSON to create the json document.
        //! @param object The object that will be read from for values to convert.
        //! @param defaultObject Default object used to compare the object to in order to determine if values are
        //!     defaulted or not. If this is argument is provided m_keepDefaults in the settings will automatically 
        //!     be set to true.
        //! @param settings The settings used during serialization. Use the value passed in from Store.
        template<typename T>
        static JsonSerializationResult::ResultCode Store(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
            const T& object, const T& defaultObject, JsonSerializerSettings settings = JsonSerializerSettings{});
        
        //! Stores the data in the provided object as json values starting at the provided value.
        //! @param output The Value or Document where the converted data will start writing to.
        //! @param allocator The memory allocator used by RapidJSON to create the json document.
        //! @param object Pointer to the object that will be read from for values to convert.
        //! @param defaultObject Pointer to a default object used to compare the object to in order to determine if values are
        //!     defaulted or not. This argument can be null, in which case a temporary default may be created if required by
        //!     the settings. If this is argument is provided m_keepDefaults in the settings will automatically  be set to true.
        //! @param objectType The type id of the object and default object.
        //! @param settings The settings used during serialization. Use the value passed in from Store.
        static JsonSerializationResult::ResultCode Store(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
            const void* object, const void* defaultObject, const Uuid& objectType, 
            JsonSerializerSettings settings = JsonSerializerSettings{});

        //! Stores a name for the type id in the provided output. The name can be safely used to reference a type such as a class during loading.
        //! Note: it's not recommended to use this function (frequently) as it requires users of the json file to have knowledge of the internal
        //!     type structure and is therefore harder to use.
        //! @param output The json value the result will be written to. If successful this will contain a string object otherwise a default object.
        //! @param allocator The allocator associated with the document that will or already holds the output.
        //! @param typeId The type id to store.
        //! @param elementPath An optional path to the element. This will be used for reporting.
        //! @param settings An optional settings object to change where this function collects information from. This can be same settings
        //!     as used for the other Store functions.
        static JsonSerializationResult::ResultCode StoreTypeId(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
            const Uuid& typeId, AZStd::string_view elementPath = AZStd::string_view{}, JsonSerializerSettings settings = JsonSerializerSettings{});

        //! Compares two json values of any type and determines if the left is less, equal or greater than the right.
        //! @param lhs The left hand side value for the compare.
        //! @param rhs The right hand side value for the compare.
        //! @return An enum containing less, equal or greater. In case of an error, the value for the enum will "error".
        static JsonSerializerCompareResult Compare(const rapidjson::Value& lhs, const rapidjson::Value& rhs);

    private:
        JsonSerialization() = delete;
        ~JsonSerialization() = delete;
        JsonSerialization& operator=(const JsonSerialization& rhs) = delete;
        JsonSerialization& operator=(JsonSerialization&& rhs) = delete;
        JsonSerialization(const JsonSerialization& rhs) = delete;
        JsonSerialization(JsonSerialization&& rhs) = delete;

        static JsonSerializationResult::ResultCode DefaultIssueReporter(AZ::OSString& scratchBuffer, AZStd::string_view message,
            JsonSerializationResult::ResultCode result, AZStd::string_view path);

        static JsonSerializerCompareResult CompareObject(const rapidjson::Value& lhs, const rapidjson::Value& rhs);
        static JsonSerializerCompareResult CompareArray(const rapidjson::Value& lhs, const rapidjson::Value& rhs);
        static JsonSerializerCompareResult CompareString(const rapidjson::Value& lhs, const rapidjson::Value& rhs);
    };

    template<typename T>
    JsonSerializationResult::ResultCode JsonSerialization::Load(T& object, const rapidjson::Value& root, JsonDeserializerSettings settings)
    {
        return Load(&object, azrtti_typeid(object), root, settings);
    }

    template<typename T>
    JsonSerializationResult::ResultCode JsonSerialization::Store(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const T& object, JsonSerializerSettings settings)
    {
        return Store(output, allocator, &object, nullptr, azrtti_typeid(object), settings);
    }

    template<typename T>
    JsonSerializationResult::ResultCode JsonSerialization::Store(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const T& object, const T& defaultObject, JsonSerializerSettings settings)
    {
        return Store(output, allocator, &object,& defaultObject, azrtti_typeid(object), settings);
    }
} // namespace AZ
