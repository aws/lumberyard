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

#include "ReflectionSerializer.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Utils.h>

namespace MCore
{
    const AZ::SerializeContext::ClassElement* RecursivelyFindClassElement(const AZ::SerializeContext* context, const AZ::SerializeContext::ClassData* parentClassData, const AZ::Crc32 nameCrc)
    {
        // Find in parentClassData
        for (const AZ::SerializeContext::ClassElement& classElement : parentClassData->m_elements)
        {
            if (classElement.m_nameCrc == nameCrc)
            {
                return &classElement;
            }
        }
        // Check in base classes
        for (const AZ::SerializeContext::ClassElement& classElement : parentClassData->m_elements)
        {
            if (classElement.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS)
            {
                const AZ::SerializeContext::ClassData* baseClassData = context->FindClassData(classElement.m_typeId);
                const AZ::SerializeContext::ClassElement* baseResult = RecursivelyFindClassElement(context, baseClassData, nameCrc);
                if (baseResult)
                {
                    return baseResult;
                }
            }
        }
        // not found
        return nullptr;
    }

    AZ::Outcome<AZStd::string> ReflectionSerializer::SerializeMember(const AZ::TypeId& classTypeId, const void* classPtr, const char* memberName)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return AZ::Failure();
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(classTypeId);
        AZ_Assert(classData, "Expected valid class data, is the type reflected?");

        const AZ::Crc32 nameCrc(memberName);
        const AZ::SerializeContext::ClassElement* classElement = RecursivelyFindClassElement(context, classData, nameCrc);
        if (classElement)
        {
            const AZ::SerializeContext::ClassData* classDataElement = context->FindClassData(classElement->m_typeId);
            if (classDataElement && classDataElement->m_serializer)
            {
                AZStd::vector<char> inBuffer;
                AZ::IO::ByteContainerStream<AZStd::vector<char>> inStream(&inBuffer);
                classDataElement->m_serializer->Save(static_cast<const char*>(classPtr) + classElement->m_offset, inStream);
                inStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

                AZStd::string outBuffer;
                AZ::IO::ByteContainerStream<AZStd::string> outStream(&outBuffer);
                if (!classDataElement->m_serializer->DataToText(inStream, outStream, false))
                {
                    return AZ::Success(outBuffer);
                }
            }
        }
        return AZ::Failure();
    }

    bool ReflectionSerializer::DeserializeIntoMember(const AZ::TypeId& classTypeId, void* classPtr, const char* memberName, const AZStd::string& value)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return false;
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(classTypeId);
        AZ_Assert(classData, "Expected valid class data, is the type reflected?");

        const AZ::Crc32 nameCrc(memberName);
        const AZ::SerializeContext::ClassElement* classElement = RecursivelyFindClassElement(context, classData, nameCrc);
        if (classElement)
        {
            const AZ::SerializeContext::ClassData* classDataElement = context->FindClassData(classElement->m_typeId);
            if (classDataElement->m_serializer)
            {
                AZ::IO::MemoryStream memoryStream(static_cast<char*>(classPtr) + classElement->m_offset, classElement->m_dataSize, 0);
                return classDataElement->m_serializer->TextToData(value.c_str(), 0, memoryStream) != 0;
            }
            else
            {
                AZ::IO::ByteContainerStream<const AZStd::string> inputStream(&value);
                return AZ::Utils::LoadObjectFromStreamInPlace(inputStream, context, classElement->m_typeId, static_cast<char*>(classPtr) + classElement->m_offset);

            }
        }
        return false;
    }

    AZ::Outcome<AZStd::string> ReflectionSerializer::Serialize(const AZ::TypeId& classTypeId, const void* classPtr)
    {
        AZStd::string destinationBuffer;
        AZ::IO::ByteContainerStream<AZStd::string> byteStream(&destinationBuffer);
        if (AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_XML, classPtr, classTypeId))
        {
            return AZ::Success(destinationBuffer);
        }
        else
        {
            return AZ::Failure();
        }
    }

    bool ReflectionSerializer::Deserialize(const AZ::TypeId& classTypeId, void* classPtr, const AZStd::string& sourceBuffer)
    {
        AZ::IO::ByteContainerStream<const AZStd::string> byteStream(&sourceBuffer);
        return AZ::Utils::LoadObjectFromStreamInPlace(byteStream, nullptr, classTypeId, classPtr);
    }

    void RecursivelyGetClassElement(AZStd::vector<const AZ::SerializeContext::ClassElement*>& elements, const AZ::SerializeContext* context, const AZ::SerializeContext::ClassData* parentClassData)
    {
        // Check in base classes
        for (const AZ::SerializeContext::ClassElement& classElement : parentClassData->m_elements)
        {
            if (classElement.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS)
            {
                const AZ::SerializeContext::ClassData* baseClassData = context->FindClassData(classElement.m_typeId);
                RecursivelyGetClassElement(elements, context, baseClassData);
            }
            else
            {
                elements.emplace_back(&classElement);
            }
        }
    }

    AZ::Outcome<AZStd::string> ReflectionSerializer::SerializeIntoCommandLine(const AZ::TypeId& classTypeId, const void* classPtr)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return AZ::Failure();
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(classTypeId);
        AZ_Assert(classData, "Expected valid class data, is the type reflected?");

        AZStd::vector<const AZ::SerializeContext::ClassElement*> elements;
        RecursivelyGetClassElement(elements, context, classData);

        AZStd::string result;
        result.reserve(1024);

        for (const AZ::SerializeContext::ClassElement* element : elements)
        {
            const AZ::SerializeContext::ClassData* classDataElement = context->FindClassData(element->m_typeId);

            if (classDataElement && classDataElement->m_serializer)
            {
                AZStd::vector<char> inBuffer;
                AZ::IO::ByteContainerStream<AZStd::vector<char>> inStream(&inBuffer);
                classDataElement->m_serializer->Save(static_cast<const char*>(classPtr) + element->m_offset, inStream);
                inStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

                AZStd::string outBuffer;
                AZ::IO::ByteContainerStream<AZStd::string> outStream(&outBuffer);
                if (!classDataElement->m_serializer->DataToText(inStream, outStream, false))
                {
                    AZ_Error("EMotionFX", false, "Error serializing member \"%s\"", element->m_name);
                    return AZ::Failure();
                }
                result += "-" + AZStd::string(element->m_name) + " {" + outBuffer + "} ";
            }
        }
        if (!result.empty())
        {
            result.pop_back(); // remove the last space
        }
        return AZ::Success(result);
    }

    bool ReflectionSerializer::Deserialize(const AZ::TypeId& classTypeId, void* classPtr, const MCore::CommandLine& sourceCommandLine)
    {
        bool someError = false;
        const uint32 numParameters = sourceCommandLine.GetNumParameters();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            someError |= DeserializeIntoMember(classTypeId, classPtr, sourceCommandLine.GetParameterName(i).c_str(), sourceCommandLine.GetParameterValue(i).c_str());
        }
        // We are deserializing without checking if all the members can be deserialized, therefore this is not an atomic operation
        // If we need it to be atomic, we can serialize the class at the beginning, if something fails we roll it back
        return someError;
    }

    void* ReflectionSerializer::Clone(const AZ::TypeId& classTypeId, const void* classPtr)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        return context->CloneObject(classPtr, classTypeId);
    }

    void* ReflectionSerializer::GetPointerToMember(AZ::SerializeContext* context, const AZ::TypeId& classTypeId, void* classPtr, const char* memberName)
    {
        AZ_Assert(context, "Invalid serialize context.");

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(classTypeId);
        AZ_Assert(classData, "Expected valid class data, is the type reflected?");

        const AZ::Crc32 nameCrc(memberName);
        const AZ::SerializeContext::ClassElement* classElement = RecursivelyFindClassElement(context, classData, nameCrc);
        if (classElement)
        {
            const AZ::SerializeContext::ClassData* classDataElement = context->FindClassData(classElement->m_typeId);
            return static_cast<char*>(classPtr) + classElement->m_offset;
        }
        return nullptr;
    }

}   // namespace EMotionFX
