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

#include <AzCore/Math/MathReflection.h>
#include <AzCore/Serialization/Json/BasicContainerSerializer.h>
#include <AzCore/Serialization/Json/BoolSerializer.h>
#include <AzCore/Serialization/Json/DoubleSerializer.h>
#include <AzCore/Serialization/Json/EnumSerializer.h>
#include <AzCore/Serialization/Json/IntSerializer.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/MapSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StringSerializer.h>
#include <AzCore/Serialization/Json/TupleSerializer.h>
#include <AzCore/Serialization/Json/UnorderedSetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/utils.h>

namespace AZ
{
    void JsonSystemComponent::Activate()
    {}

    void JsonSystemComponent::Deactivate()
    {}

    void JsonSystemComponent::Reflect(ReflectContext* reflectContext)
    {
        if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(reflectContext))
        {
            jsonContext->Serializer<JsonBoolSerializer>()->HandlesType<bool>();
            jsonContext->Serializer<JsonDoubleSerializer>()->HandlesType<double>();
            jsonContext->Serializer<JsonFloatSerializer>()->HandlesType<float>();
            jsonContext->Serializer<JsonCharSerializer>()->HandlesType<char>();
            jsonContext->Serializer<JsonShortSerializer>()->HandlesType<short>();
            jsonContext->Serializer<JsonIntSerializer>()->HandlesType<int>();
            jsonContext->Serializer<JsonLongSerializer>()->HandlesType<long>();
            jsonContext->Serializer<JsonLongLongSerializer>()->HandlesType<long long>();
            jsonContext->Serializer<JsonUnsignedCharSerializer>()->HandlesType<unsigned char>();
            jsonContext->Serializer<JsonUnsignedShortSerializer>()->HandlesType<unsigned short>();
            jsonContext->Serializer<JsonUnsignedIntSerializer>()->HandlesType<unsigned int>();
            jsonContext->Serializer<JsonUnsignedLongSerializer>()->HandlesType<unsigned long>();
            jsonContext->Serializer<JsonUnsignedLongLongSerializer>()->HandlesType<unsigned long long>();

            jsonContext->Serializer<JsonEnumSerializer>();
            
            jsonContext->Serializer<JsonStringSerializer>()->HandlesType<AZStd::string>();
            jsonContext->Serializer<JsonOSStringSerializer>()->HandlesType<OSString>();
            jsonContext->Serializer<JsonBasicContainerSerializer>()
                ->HandlesType<AZStd::list>()
                ->HandlesType<AZStd::set>()
                ->HandlesType<AZStd::vector>();
            jsonContext->Serializer<JsonMapSerializer>()->HandlesType<AZStd::map>();
            jsonContext->Serializer<JsonUnorderedMapSerializer>()
                ->HandlesType<AZStd::unordered_map>()
                ->HandlesType<AZStd::unordered_multimap>();
            jsonContext->Serializer<JsonUnorderedSetContainerSerializer>()
                ->HandlesType<AZStd::unordered_set>()
                ->HandlesType<AZStd::unordered_multiset>();
            jsonContext->Serializer<JsonTupleSerializer>()
                ->HandlesType<AZStd::pair>()
                ->HandlesType<AZStd::tuple>();

            MathReflect(jsonContext);
        }
        else if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflectContext))
        {
            serializeContext->Class<JsonSystemComponent, AZ::Component>();
        }
    }
    
} // namespace AZ
