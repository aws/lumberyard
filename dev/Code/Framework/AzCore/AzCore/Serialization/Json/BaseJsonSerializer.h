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

namespace AZ
{
    struct Uuid;
    struct JsonDeserializerSettings;
    struct JsonSerializerSettings;
    class StackedString;

    //! The abstract class for primitive Json Serializers. 
    //! It is intentionally not templated, and uses void* for output value parameters.
    class BaseJsonSerializer
    {
    public:
        AZ_RTTI(BaseJsonSerializer, "{7291FFDC-D339-40B5-BB26-EA067A327B21}");
        
        enum Flags
        {
            None = 0,        //! No extra flags.
            ResolvePointer = 1 << 0,   //! The pointer passed in contains a pointer. The (de)serializer will attempt to resolve to an instance.
            ReplaceDefault = 1 << 1    //! The default value provided for storing will be replaced with a newly created one.
        };

        virtual ~BaseJsonSerializer() = default;

        //! Transforms the data from the rapidjson Value to outputValue, if the conversion is possible and supported.
        //! The serializer is responsible for casting to the proper type and safely writing to the outputValue memory.
        virtual JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings) = 0;
        
        //! Write the input value to a rapidjson value if the default value is not null and doesn't match the input value, otherwise
        //! an error is returned and sets the rapidjson value to a null value.
        virtual JsonSerializationResult::Result Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
            const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, 
            StackedString& path, const JsonSerializerSettings& settings) = 0;

    protected:
        //! Continues loading of a (sub)value. Use this function to load member variables for instance. This is more optimal than 
        //! directly calling the json serialization.
        //! @param object Pointer to the object where the data will be loaded into.
        //! @param typeId Type id of the object passed in.
        //! @param value The value in the JSON document where the deserializer will start reading data from.
        //! @param settings The settings used during deserialization. Use the value passed in from Load.
        JsonSerializationResult::ResultCode ContinueLoading(void* object, const Uuid& typeId, const rapidjson::Value& value,
            StackedString& path, const JsonDeserializerSettings& settings, Flags flags = Flags::None);

        //! Continues storing of a (sub)value. Use this function to store member variables for instance. This is more optimal than 
        //! directly calling the json serialization.
        //! @param output The value in the json document where the converted data will start writing to.
        //! @param allocator The memory allocator used by RapidJSON to create the json document.
        //! @param object Pointer to the object that will be read from for values to convert.
        //! @param defaultObject Pointer to a default object used to compare the object to in order to determine if values are
        //!     defaulted or not. This argument can be null, in which case a temporary default may be created if required by
        //!     the settings.
        //! @param typeId The type id of the object and default object.
        //! @param settings The settings used during serialization. Use the value passed in from Store.
        JsonSerializationResult::ResultCode ContinueStoring(rapidjson::Value& output, 
            rapidjson::Document::AllocatorType& allocator, const void* object, const void* defaultObject, 
            const Uuid& typeId, StackedString& path, const JsonSerializerSettings& settings, Flags flags = Flags::None);

        //! Checks if a value is an explicit default. This useful for containers where not storing anything as a default would mean
        //! a slot wouldn't be used so something has to be added to represent the fully default target.
        bool IsExplicitDefault(const rapidjson::Value& value);
    };

} // namespace AZ
AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::BaseJsonSerializer::Flags)
