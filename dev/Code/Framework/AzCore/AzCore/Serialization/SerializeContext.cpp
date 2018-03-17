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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/DataOverlay.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Serialization/Utils.h>
/// include AZStd any serializer support
#include <AzCore/Serialization/AZStdAnyDataContainer.inl>

#include <AzCore/std/functional.h>
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzCore/Math/MathReflection.h>
#include <AzCore/Math/MathUtils.h>

#include <AzCore/Debug/Profiler.h>

namespace AZ
{
    AZ_THREAD_LOCAL void* Internal::AZStdArrayEvents::m_indices = nullptr;

    //////////////////////////////////////////////////////////////////////////
    // Integral types serializers

    template<class T>
    class BinaryValueSerializer
        : public SerializeContext::IDataSerializer
    {
        /// Load the class data from a binary buffer.
        bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
        {
            if (stream.GetLength() < sizeof(T))
            {
                return false;
            }

            T* dataPtr = reinterpret_cast<T*>(classPtr);
            if (stream.Read(sizeof(T), reinterpret_cast<void*>(dataPtr)) == sizeof(T))
            {
                AZ_SERIALIZE_SWAP_ENDIAN(*dataPtr, isDataBigEndian);
                return true;
            }
            return false;
        }

        /// Store the class data into a stream.
        size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
        {
            T value = *reinterpret_cast<const T*>(classPtr);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<const void*>(&value)));
        }
    };


    // char, short, int
    template<class T>
    class IntSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t  DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%d", value);
            return static_cast<size_t>(out.Write(outText.size(), outText.c_str()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown char, short, int version!");
            (void)textVersion;
            long value = strtol(text, NULL, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // unsigned char, short, int, long
    template<class T>
    class UIntSerializer
        : public BinaryValueSerializer<T>
    {
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%u", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            unsigned long value = strtoul(text, NULL, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // long
    template<class T>
    class LongSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%ld", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false)
        {
            AZ_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            long value = strtol(text, NULL, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // unsigned long
    template<class T>
    class ULongSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%lu", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            unsigned long value = strtoul(text, NULL, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // long long
    class LongLongSerializer
        : public BinaryValueSerializer<AZ::s64>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(AZ::s64), "Invalid data size");

            AZ::s64 value;
            in.Read(sizeof(AZ::s64), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%lld", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            AZ::s64 value = strtoll(text, NULL, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(AZ::s64), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<AZ::s64>::CompareValues(lhs, rhs);
        }
    };

    // unsigned long long
    class ULongLongSerializer
        : public BinaryValueSerializer<AZ::u64>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(AZ::u64), "Invalid data size");

            AZ::u64 value;
            in.Read(sizeof(AZ::u64), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%llu", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            unsigned long long value = strtoull(text, NULL, 10);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(AZ::u64), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<AZ::u64>::CompareValues(lhs, rhs);
        }
    };

    // float, double
    template<class T, T GetEpsilon()>
    class FloatSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            AZStd::string outText = AZStd::string::format("%.7f", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            AZ_Assert(textVersion == 0, "Unknown float/double version!");
            (void)textVersion;
            double value = strtod(text, NULL);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);

            T data = static_cast<T>(value);
            size_t bytesWritten = static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&data)));

            AZ_Assert(bytesWritten == sizeof(T), "Invalid data size");

            return bytesWritten;
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return AZ::IsClose(*reinterpret_cast<const T*>(lhs), *reinterpret_cast<const T*>(rhs), GetEpsilon());
        }
    };

    // bool
    class BoolSerializer
        : public BinaryValueSerializer<bool>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            AZ_Assert(in.GetLength() == sizeof(bool), "Invalid data size");

            bool value;
            in.Read(sizeof(bool), reinterpret_cast<void*>(&value));
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);

            char outStr[7];
            azsnprintf(outStr, 6, "%s", value ? "true" : "false");

            return static_cast<size_t>(out.Write(strlen(outStr), outStr));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            (void)isDataBigEndian;
            (void)textVersion;
            bool value = strcmp("true", text) == 0;
            return static_cast<size_t>(stream.Write(sizeof(bool), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<bool>::CompareValues(lhs, rhs);
        }
    };

    // serializer without any data write
    class EmptySerializer
        : public SerializeContext::IDataSerializer
    {
    public:
        /// Store the class data into a stream.
        size_t  Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian) override
        {
            (void)classPtr;
            (void)stream;
            (void)isDataBigEndian;
            return 0;
        }

        /// Load the class data from a stream.
        bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian) override
        {
            (void)classPtr;
            (void)stream;
            (void)isDataBigEndian;
            return true;
        }

        /// Convert binary data to text.
        size_t  DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian) override
        {
            (void)in;
            (void)out;
            (void)isDataBigEndian;
            return 0;
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            (void)text;
            (void)textVersion;
            (void)stream;
            (void)isDataBigEndian;
            return 0;
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            (void)lhs;
            (void)rhs;
            return true;
        }
    };

    struct EnumerateBaseRTTIEnumCallbackData
    {
        AZStd::fixed_vector<Uuid, 32> m_reportedTypes; ///< Array with reported types (with more data - classData was found).
        const SerializeContext::TypeInfoCB* m_callback; ///< Pointer to the callback.
    };

    //=========================================================================
    // SerializeContext
    // [4/27/2012]
    //=========================================================================
    SerializeContext::SerializeContext(bool registerIntegralTypes, bool createEditContext)
        : m_editContext(nullptr)
    {
        if (registerIntegralTypes)
        {
            Class<char>()->
                Serializer<IntSerializer<char> >();
            Class<short>()->
                Serializer<IntSerializer<short> >();
            Class<int>()->
                Serializer<IntSerializer<int> >();
            Class<long>()->
                Serializer<LongSerializer<long> >();
            Class<AZ::s64>()->
                Serializer<LongLongSerializer>();

            Class<unsigned char>()->
                Serializer<UIntSerializer<unsigned char> >();
            Class<unsigned short>()->
                Serializer<UIntSerializer<unsigned short> >();
            Class<unsigned int>()->
                Serializer<UIntSerializer<unsigned int> >();
            Class<unsigned long>()->
                Serializer<ULongSerializer<unsigned long> >();
            Class<AZ::u64>()->
                Serializer<ULongLongSerializer>();

            Class<float>()->
                Serializer<FloatSerializer<float, &std::numeric_limits<float>::epsilon>>();
            Class<double>()->
                Serializer<FloatSerializer<double, &std::numeric_limits<double>::epsilon>>();

            Class<bool>()->
                Serializer<BoolSerializer>();

            MathReflect(this);

            Class<DataOverlayToken>()->
                Field("Uri", &DataOverlayToken::m_dataUri);
            Class<DataOverlayInfo>()->
                Field("ProviderId", &DataOverlayInfo::m_providerId)->
                Field("DataToken", &DataOverlayInfo::m_dataToken);

            Class<DynamicSerializableField>()->
                Field("TypeId", &DynamicSerializableField::m_typeId);
                // Value data is injected into the hierarchy per-instance, since type is dynamic.

            Internal::ReflectAny(this);
        }

        if (createEditContext)
        {
            CreateEditContext();
        }
    }

    //=========================================================================
    // DestroyEditContext
    // [10/26/2012]
    //=========================================================================
    SerializeContext::~SerializeContext()
    {
        DestroyEditContext();
    }

    //=========================================================================
    // CreateEditContext
    // [10/26/2012]
    //=========================================================================
    EditContext* SerializeContext::CreateEditContext()
    {
        if (m_editContext == nullptr)
        {
            m_editContext = aznew EditContext(*this);
        }

        return m_editContext;
    }

    //=========================================================================
    // DestroyEditContext
    // [10/26/2012]
    //=========================================================================
    void SerializeContext::DestroyEditContext()
    {
        if (m_editContext)
        {
            delete m_editContext;
            m_editContext = nullptr;
        }
    }

    //=========================================================================
    // GetEditContext
    // [11/8/2012]
    //=========================================================================
    EditContext* SerializeContext::GetEditContext() const
    {
        return m_editContext;
    }

    //=========================================================================
    // ClassDeprecate
    // [11/8/2012]
    //=========================================================================
    void SerializeContext::ClassDeprecate(const char* name, const AZ::Uuid& typeUuid, VersionConverter converter)
    {
        if (IsRemovingReflection())
        {
            m_uuidMap.erase(typeUuid);
            return;
        }

        UuidToClassMap::pair_iter_bool result = m_uuidMap.insert_key(typeUuid);
        AZ_Assert(result.second, "This class type %s has already been registered", name /*,typeUuid.ToString()*/);

        ClassData& cd = result.first->second;
        cd.m_name = name;
        cd.m_typeId = typeUuid;
        cd.m_version = VersionClassDeprecated;
        cd.m_converter = converter;
        cd.m_serializer = nullptr;
        cd.m_factory = nullptr;
        cd.m_container = nullptr;
        cd.m_azRtti = nullptr;
        cd.m_eventHandler = nullptr;
        cd.m_editData = nullptr;

        m_classNameToUuid.emplace(AZ::Crc32(name), typeUuid);
    }

    //=========================================================================
    // FindClassId
    // [5/31/2017]
    //=========================================================================
    AZStd::vector<AZ::Uuid> SerializeContext::FindClassId(const AZ::Crc32& classNameCrc) const
    {
        auto&& findResult = m_classNameToUuid.equal_range(classNameCrc);
        AZStd::vector<AZ::Uuid> retVal;
        for (auto&& currentIter = findResult.first; currentIter != findResult.second; ++currentIter)
        {
            retVal.push_back(currentIter->second);
        }
        return retVal;
    }

    //=========================================================================
    // FindClassData
    // [5/11/2012]
    //=========================================================================
    const SerializeContext::ClassData*
    SerializeContext::FindClassData(const Uuid& classId, const SerializeContext::ClassData* parent, u32 elementNameCrc) const
    {
        SerializeContext::UuidToClassMap::const_iterator it = m_uuidMap.find(classId);
        const SerializeContext::ClassData* cd = nullptr;

        if (it == m_uuidMap.end())
        {
            // this is not a registered type try to find it in the parent scope by name and check / type and flags
            if (parent)
            {
                if (parent->m_container)
                {
                    const SerializeContext::ClassElement* classElement = parent->m_container->GetElement(elementNameCrc);
                    if (classElement && classElement->m_genericClassInfo)
                    {
                        if (classElement->m_genericClassInfo->CanStoreType(classId))
                        {
                            cd = classElement->m_genericClassInfo->GetClassData();
                        }
                    }
                }
                else if (elementNameCrc)
                {
                    for (size_t i = 0; i < parent->m_elements.size(); ++i)
                    {
                        const SerializeContext::ClassElement& classElement = parent->m_elements[i];
                        if (classElement.m_nameCrc == elementNameCrc && classElement.m_genericClassInfo)
                        {
                            if (classElement.m_genericClassInfo->CanStoreType(classId))
                            {
                                cd = classElement.m_genericClassInfo->GetClassData();
                            }
                            break;
                        }
                    }
                }
            }

            /* If the ClassData could not be found in the normal UuidMap, then the GenericUuid map will be searched.
             The GenericUuid map contains a mapping of Uuids to ClassData from which registered GenericClassInfo
             when reflecting
             */
            if (!cd)
            {
                auto genericClassInfo = FindGenericClassInfo(classId);
                if (genericClassInfo)
                {
                    cd = genericClassInfo->GetClassData();
                }
            }
        }
        else
        {
            cd = &it->second;
        }

        return cd;
    }

    //=========================================================================
    // FindGenericClassInfo
    //=========================================================================
    GenericClassInfo* SerializeContext::FindGenericClassInfo(const Uuid& classId) const
    {
        auto genericClassIt = m_uuidGenericMap.find(classId);
        if (genericClassIt != m_uuidGenericMap.end())
        {
            return genericClassIt->second;
        }

        return nullptr;
    }

    void SerializeContext::RegisterGenericClassInfo(const Uuid& classId, GenericClassInfo* genericClassInfo, const CreateAnyFunc& createAnyFunc)
    {
        if (genericClassInfo)
        {
            if (IsRemovingReflection())
            {
                m_uuidGenericMap.erase(classId);
                m_uuidAnyCreationMap.erase(classId);
                m_classNameToUuid.erase(Crc32(genericClassInfo->GetClassData()->m_name));
            }
            else
            {
                m_uuidGenericMap.emplace(classId, genericClassInfo);
                m_uuidAnyCreationMap.emplace(classId, createAnyFunc);
                m_classNameToUuid.emplace(genericClassInfo->GetClassData()->m_name, classId);
            }
        }
    }

    AZStd::any SerializeContext::CreateAny(const Uuid& classId)
    {
        auto anyCreationIt = m_uuidAnyCreationMap.find(classId);
        return anyCreationIt != m_uuidAnyCreationMap.end() ? anyCreationIt->second(this) : AZStd::any();
    }

    //=========================================================================
    // CanDowncast
    // [6/5/2012]
    //=========================================================================
    bool SerializeContext::CanDowncast(const Uuid& fromClassId, const Uuid& toClassId, const IRttiHelper* fromClassHelper, const IRttiHelper* toClassHelper) const
    {
        // Same type
        if (fromClassId == toClassId)
        {
            return true;
        }

        // rtti info not provided, see if we can cast using reflection data first
        if (!fromClassHelper)
        {
            const ClassData* fromClass = FindClassData(fromClassId);
            if (!fromClass)
            {
                return false;   // Can't cast without class data or rtti
            }
            for (size_t i = 0; i < fromClass->m_elements.size(); ++i)
            {
                if ((fromClass->m_elements[i].m_flags & ClassElement::FLG_BASE_CLASS) != 0)
                {
                    if (fromClass->m_elements[i].m_typeId == toClassId)
                    {
                        return true;
                    }
                }
            }

            if (!fromClass->m_azRtti)
            {
                return false;   // Reflection info failed to cast and we can't find rtti info
            }
            fromClassHelper = fromClass->m_azRtti;
        }

        if (!toClassHelper)
        {
            const ClassData* toClass = FindClassData(toClassId);
            if (!toClass || !toClass->m_azRtti)
            {
                return false;   // We can't cast without class data or rtti helper
            }
            toClassHelper = toClass->m_azRtti;
        }

        // Rtti available, use rtti cast
        return fromClassHelper->IsTypeOf(toClassHelper->GetTypeId());
    }

    //=========================================================================
    // DownCast
    // [6/5/2012]
    //=========================================================================
    void* SerializeContext::DownCast(void* instance, const Uuid& fromClassId, const Uuid& toClassId, const IRttiHelper* fromClassHelper, const IRttiHelper* toClassHelper) const
    {
        // Same type
        if (fromClassId == toClassId)
        {
            return instance;
        }

        // rtti info not provided, see if we can cast using reflection data first
        if (!fromClassHelper)
        {
            const ClassData* fromClass = FindClassData(fromClassId);
            if (!fromClass)
            {
                return NULL;
            }

            for (size_t i = 0; i < fromClass->m_elements.size(); ++i)
            {
                if ((fromClass->m_elements[i].m_flags & ClassElement::FLG_BASE_CLASS) != 0)
                {
                    if (fromClass->m_elements[i].m_typeId == toClassId)
                    {
                        return (char*)(instance) + fromClass->m_elements[i].m_offset;
                    }
                }
            }

            if (!fromClass->m_azRtti)
            {
                return NULL;    // Reflection info failed to cast and we can't find rtti info
            }
            fromClassHelper = fromClass->m_azRtti;
        }

        if (!toClassHelper)
        {
            const ClassData* toClass = FindClassData(toClassId);
            if (!toClass || !toClass->m_azRtti)
            {
                return NULL;    // We can't cast without class data or rtti helper
            }
            toClassHelper = toClass->m_azRtti;
        }

        // Rtti available, use rtti cast
        return fromClassHelper->Cast(instance, toClassHelper->GetTypeId());
    }

    //=========================================================================
    // DataElement
    // [5/22/2012]
    //=========================================================================
    SerializeContext::DataElement::DataElement()
        : m_name(0)
        , m_nameCrc(0)
        , m_dataSize(0)
        , m_byteStream(&m_buffer)
        , m_stream(nullptr)
    {
    }

    //=========================================================================
    // ~DataElement
    // [5/22/2012]
    //=========================================================================
    SerializeContext::DataElement::~DataElement()
    {
        m_buffer.clear();
    }

    //=========================================================================
    // DataElement
    // [5/22/2012]
    //=========================================================================
    SerializeContext::DataElement::DataElement(const DataElement& rhs)
        : m_name(rhs.m_name)
        , m_nameCrc(rhs.m_nameCrc)
        , m_dataSize(rhs.m_dataSize)
        , m_byteStream(&m_buffer)
    {
        m_id = rhs.m_id;
        m_version = rhs.m_version;
        m_dataType = rhs.m_dataType;

        AZ_Assert(rhs.m_dataSize == rhs.m_buffer.size(), "Temp buffer must contain only the data for current element!");
        m_buffer = rhs.m_buffer;
        m_byteStream = IO::ByteContainerStream<AZStd::vector<char> >(&m_buffer);
        m_byteStream.Seek(rhs.m_byteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);
        if (rhs.m_stream == &rhs.m_byteStream)
        {
            m_stream = &m_byteStream;
        }
        else
        {
            m_stream = rhs.m_stream;
        }
    }

    //=========================================================================
    // DataElement
    // [5/22/2012]
    //=========================================================================
    SerializeContext::DataElement& SerializeContext::DataElement::operator= (const DataElement& rhs)
    {
        m_name = rhs.m_name;
        m_nameCrc = rhs.m_nameCrc;
        m_id = rhs.m_id;
        m_version = rhs.m_version;
        m_dataSize = rhs.m_dataSize;
        m_dataType = rhs.m_dataType;

        AZ_Assert(rhs.m_dataSize == rhs.m_buffer.size(), "Temp buffer must contain only the data for current element!");
        m_buffer = rhs.m_buffer;
        m_byteStream = IO::ByteContainerStream<AZStd::vector<char> >(&m_buffer);
        m_byteStream.Seek(rhs.m_byteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);

        if (rhs.m_stream == &rhs.m_byteStream)
        {
            m_stream = &m_byteStream;
        }
        else
        {
            m_stream = rhs.m_stream;
        }

        return *this;
    }

#ifdef AZ_HAS_RVALUE_REFS
    //=========================================================================
    // DataElement
    //=========================================================================
    SerializeContext::DataElement::DataElement(DataElement&& rhs)
        : m_name(rhs.m_name)
        , m_nameCrc(rhs.m_nameCrc)
        , m_dataSize(rhs.m_dataSize)
        , m_byteStream(&m_buffer)
    {
        m_id = rhs.m_id;
        m_version = rhs.m_version;
        m_dataType = rhs.m_dataType;
        if (rhs.m_stream == &rhs.m_byteStream)
        {
            m_stream = &m_byteStream;
        }
        else
        {
            m_stream = rhs.m_stream;
        }

        AZ_Assert(rhs.m_dataSize == rhs.m_buffer.size(), "Temp buffer must contain only the data for current element!");
        m_buffer = AZStd::move(rhs.m_buffer);
        m_byteStream = IO::ByteContainerStream<AZStd::vector<char> >(&m_buffer);
        m_byteStream.Seek(rhs.m_byteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);

        rhs.m_name = nullptr;
        rhs.m_nameCrc = 0;
        rhs.m_id = Uuid::CreateNull();
        rhs.m_version = 0;
        rhs.m_dataSize = 0;
        rhs.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        if (m_stream == &m_byteStream)
        {
            rhs.m_stream = &rhs.m_byteStream;
        }
        else
        {
            rhs.m_stream = nullptr;
        }
    }

    //=========================================================================
    // DataElement
    //=========================================================================
    SerializeContext::DataElement& SerializeContext::DataElement::operator= (DataElement&& rhs)
    {
        m_name = rhs.m_name;
        m_nameCrc = rhs.m_nameCrc;
        m_id = rhs.m_id;
        m_version = rhs.m_version;
        m_dataSize = rhs.m_dataSize;
        m_dataType = rhs.m_dataType;
        if (rhs.m_stream == &rhs.m_byteStream)
        {
            m_stream = &m_byteStream;
        }
        else
        {
            m_stream = rhs.m_stream;
        }

        AZ_Assert(rhs.m_dataSize == rhs.m_buffer.size(), "Temp buffer must contain only the data for current element!");

        m_buffer = AZStd::move(rhs.m_buffer);
        m_byteStream = IO::ByteContainerStream<AZStd::vector<char> >(&m_buffer);
        m_byteStream.Seek(rhs.m_byteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);

        rhs.m_name = nullptr;
        rhs.m_nameCrc = 0;
        rhs.m_id = Uuid::CreateNull();
        rhs.m_version = 0;
        rhs.m_dataSize = 0;
        rhs.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        if (m_stream == &m_byteStream)
        {
            rhs.m_stream = &rhs.m_byteStream;
        }
        else
        {
            rhs.m_stream = nullptr;
        }

        return *this;
    }
#endif //
    
    //=========================================================================
    // Convert
    //=========================================================================
    bool SerializeContext::DataElementNode::Convert(SerializeContext& sc, const Uuid& id)
    {
        // remove sub elements
        while (!m_subElements.empty())
        {
            RemoveElement(static_cast<int>(m_subElements.size()) - 1);
        }

        // replace element
        m_element.m_id = id;
        m_element.m_dataSize = 0;
        m_element.m_buffer.clear();
        m_element.m_stream = &m_element.m_byteStream;

        UuidToClassMap::iterator it = sc.m_uuidMap.find(id);
        AZ_Assert(it != sc.m_uuidMap.end(), "You are adding element to an unregistered class!");
        m_classData = &it->second;
        m_element.m_version = m_classData->m_version;

        return true;
    }

    //=========================================================================
    // Convert
    //=========================================================================
    bool SerializeContext::DataElementNode::Convert(SerializeContext& sc, const char* name, const Uuid& id)
    {
        AZ_Assert(name != NULL && strlen(name) > 0, "Empty name is an INVALID element name!");
        u32 nameCrc = Crc32(name);

#if defined(AZ_DEBUG_BUILD)
        if (FindElement(nameCrc) != -1)
        {
            AZ_Error("Serialize", false, "We already have a class member %s!", name);
            return false;
        }
#endif // AZ_DEBUG_BUILD

        // remove sub elements
        while (!m_subElements.empty())
        {
            RemoveElement(static_cast<int>(m_subElements.size()) - 1);
        }

        // replace element
        m_element.m_name = name;
        m_element.m_nameCrc = nameCrc;
        m_element.m_id = id;
        m_element.m_dataSize = 0;
        m_element.m_buffer.clear();
        m_element.m_stream = &m_element.m_byteStream;

        UuidToClassMap::iterator it = sc.m_uuidMap.find(id);
        AZ_Assert(it != sc.m_uuidMap.end(), "You are adding element to an unregistered class!");
        m_classData = &it->second;
        m_element.m_version = m_classData->m_version;

        return true;
    }

    //=========================================================================
    // FindElement
    // [5/22/2012]
    //=========================================================================
    int SerializeContext::DataElementNode::FindElement(u32 crc)
    {
        for (size_t i = 0; i < m_subElements.size(); ++i)
        {
            if (m_subElements[i].m_element.m_nameCrc == crc)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    //=========================================================================
    // FindSubElement
    //=========================================================================
    SerializeContext::DataElementNode* SerializeContext::DataElementNode::FindSubElement(u32 crc)
    {
        int index = FindElement(crc);
        return index >= 0 ? &m_subElements[index] : nullptr;
    }

    //=========================================================================
    // RemoveElement
    // [5/22/2012]
    //=========================================================================
    void SerializeContext::DataElementNode::RemoveElement(int index)
    {
        AZ_Assert(index >= 0 && index < static_cast<int>(m_subElements.size()), "Invalid index passed to RemoveElement");

        DataElementNode& node = m_subElements[index];

        node.m_element.m_dataSize = 0;
        node.m_element.m_buffer.clear();
        node.m_element.m_stream = nullptr;

        while (!node.m_subElements.empty())
        {
            node.RemoveElement(static_cast<int>(node.m_subElements.size()) - 1);
        }

        m_subElements.erase(m_subElements.begin() + index);
    }

    //=========================================================================
    // RemoveElementByName
    //=========================================================================
    bool SerializeContext::DataElementNode::RemoveElementByName(u32 crc)
    {
        int index = FindElement(crc);
        if (index >= 0)
        {
            RemoveElement(index);
            return true;
        }
        return false;
    }

    //=========================================================================
    // AddElement
    // [5/22/2012]
    //=========================================================================
    int SerializeContext::DataElementNode::AddElement(const DataElementNode& elem)
    {
        m_subElements.push_back(elem);
        return static_cast<int>(m_subElements.size() - 1);
    }

    //=========================================================================
    // AddElement
    // [5/22/2012]
    //=========================================================================
    int SerializeContext::DataElementNode::AddElement(SerializeContext& sc, const char* name, const Uuid& id)
    {
        UuidToClassMap::iterator it = sc.m_uuidMap.find(id);
        AZ_Assert(it != sc.m_uuidMap.end(), "You are adding element to an unregistered class!");
        return AddElement(sc, name, it->second);
    }

    //=========================================================================
    // AddElement
    //=========================================================================
    int SerializeContext::DataElementNode::AddElement(SerializeContext& sc, const char* name, const ClassData& classData)
    {
        (void)sc;
        AZ_Assert(name != NULL && strlen(name) > 0, "Empty name is an INVALID element name!");
        u32 nameCrc = Crc32(name);

    #if defined(AZ_DEBUG_BUILD)
        if (!m_classData->m_container && FindElement(nameCrc) != -1)
        {
            AZ_Error("Serialize", false, "We already have a class member %s!", name);
            return -1;
        }
    #endif // AZ_DEBUG_BUILD

        DataElementNode node;
        node.m_element.m_name = name;
        node.m_element.m_nameCrc = nameCrc;
        node.m_element.m_id = classData.m_typeId;
        node.m_classData = &classData;
        node.m_element.m_version = classData.m_version;

        m_subElements.push_back(node);
        return static_cast<int>(m_subElements.size() - 1);
    }

    int SerializeContext::DataElementNode::AddElement(SerializeContext& sc, AZStd::string_view name, GenericClassInfo* genericClassInfo)
    {
        (void)sc;
        AZ_Assert(!name.empty(), "Empty name is an INVALID element name!");

        if (!genericClassInfo)
        {
            AZ_Assert(false, "Supplied GenericClassInfo is nullptr. ClassData cannot be retrieved.");
            return -1;
        }

        u32 nameCrc = Crc32(name.data());
        DataElementNode node;
        node.m_element.m_name = name.data();
        node.m_element.m_nameCrc = nameCrc;
        node.m_element.m_id = genericClassInfo->GetClassData()->m_typeId;
        node.m_classData = genericClassInfo->GetClassData();
        node.m_element.m_version = node.m_classData->m_version;

        m_subElements.push_back(node);
        return static_cast<int>(m_subElements.size() - 1);
    }

    //=========================================================================
    // ReplaceElement
    //=========================================================================
    int SerializeContext::DataElementNode::ReplaceElement(SerializeContext& sc, int index, const char* name, const Uuid& id)
    {
        DataElementNode& node = m_subElements[index];
        if (node.Convert(sc, name, id))
        {
            return index;
        }
        else
        {
            return -1;
        }
    }

    //=========================================================================
    // SetName
    // [1/16/2013]
    //=========================================================================
    void SerializeContext::DataElementNode::SetName(const char* newName)
    {
        m_element.m_name = newName;
        m_element.m_nameCrc = Crc32(newName);
    }

    //=========================================================================
    // SetDataHierarchy
    //=========================================================================
    bool SerializeContext::DataElementNode::SetDataHierarchy(SerializeContext& sc, const void* objectPtr, const Uuid& classId, ErrorHandler* errorHandler, const ClassData* classData)
    {
        AZ_Assert(m_element.m_id == classId, "SetDataHierarchy called with mismatched class type {%s} for element %s",
            classId.ToString<AZStd::string>().c_str(), m_element.m_name);

        AZStd::vector<DataElementNode*> nodeStack;
        DataElementNode* topNode = this;
        bool success;

        auto beginCB = [&sc, &nodeStack, topNode, &success, errorHandler](void* ptr, const SerializeContext::ClassData* elementClassData, const SerializeContext::ClassElement* elementData) -> bool
            {
                if (nodeStack.empty())
                {
                    success = true;
                    nodeStack.push_back(topNode);
                    return true;
                }

                DataElementNode* parentNode = nodeStack.back();
                parentNode->m_subElements.reserve(64);

                AZ_Assert(elementData, "Missing element data");
                AZ_Assert(elementClassData, "Missing class data for element %s", elementData ? elementData->m_name : "<unknown>");

                int elementIndex = -1;
                if (elementData)
                {
                    elementIndex = elementData->m_genericClassInfo
                        ? parentNode->AddElement(sc, elementData->m_name, elementData->m_genericClassInfo)
                        : parentNode->AddElement(sc, elementData->m_name, *elementClassData);
                }
                if (elementIndex >= 0)
                {
                    DataElementNode& newNode = parentNode->GetSubElement(elementIndex);

                    if (elementClassData->m_serializer)
                    {
                        void* resolvedObject = ptr;
                        if (elementData && elementData->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                        {
                            resolvedObject = *(void**)(resolvedObject);

                            if (resolvedObject && elementData->m_azRtti)
                            {
                                AZ::Uuid actualClassId = elementData->m_azRtti->GetActualUuid(resolvedObject);
                                if (actualClassId != elementData->m_typeId)
                                {
                                    const AZ::SerializeContext::ClassData* actualClassData = sc.FindClassData(actualClassId);
                                    if (actualClassData)
                                    {
                                        resolvedObject = elementData->m_azRtti->Cast(resolvedObject, actualClassData->m_azRtti->GetTypeId());
                                    }
                                }
                            }
                        }

                        AZ_Assert(newNode.m_element.m_byteStream.GetCurPos() == 0, "This stream should be only for our data element");
                        newNode.m_element.m_dataSize = elementClassData->m_serializer->Save(resolvedObject, newNode.m_element.m_byteStream);
                        newNode.m_element.m_byteStream.Truncate();
                        newNode.m_element.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN); // reset stream position
                        newNode.m_element.m_stream = &newNode.m_element.m_byteStream;

                        newNode.m_element.m_dataType = DataElement::DT_BINARY;
                    }

                    nodeStack.push_back(&newNode);
                    success = true;
                }
                else
                {
                    const AZStd::string error =
                        AZStd::string::format("Failed to add sub-element \"%s\" to element \"%s\".",
                            elementData ? elementData->m_name : "<unknown>",
                            parentNode->m_element.m_name);

                    if (errorHandler)
                    {
                        errorHandler->ReportError(error.c_str());
                    }
                    else
                    {
                        AZ_Error("Serialize", "%s", error.c_str());
                    }

                    success = false;
                    return false; // Stop enumerating.
                }

                return true;
            };

        auto endCB = [&nodeStack, &success]() -> bool
            {
                if (success)
                {
                    nodeStack.pop_back();
                }

                return true;
            };

        sc.EnumerateInstanceConst(
            objectPtr,
            classId,
            beginCB,
            endCB,
            SerializeContext::ENUM_ACCESS_FOR_READ,
            classData,
            nullptr,
            errorHandler
            );

        return success;
    }

    bool SerializeContext::DataElementNode::GetClassElement(ClassElement& classElement, const DataElementNode& parentDataElement, ErrorHandler* errorHandler) const
    {
        bool elementFound = false;
        if (parentDataElement.m_classData)
        {
            const ClassData* parentClassData = parentDataElement.m_classData;
            if (parentClassData->m_container)
            {
                IDataContainer* classContainer = parentClassData->m_container;
                // store the container element in classElementMetadata
                ClassElement classElementMetadata;
                bool classElementFound = classContainer->GetElement(classElementMetadata, m_element);
                AZ_Assert(classElementFound, "'%s'(0x%x) is not a valid element name for container type %s", m_element.m_name ? m_element.m_name : "NULL", m_element.m_nameCrc, parentClassData->m_name);
                if (classElementFound)
                {
                    // if the container contains pointers, then the elements could be a derived type,
                    // otherwise we need the uuids to be exactly the same.
                    if (classElementMetadata.m_flags & SerializeContext::ClassElement::FLG_POINTER)
                    {
                        bool downcastPossible = m_element.m_id == classElementMetadata.m_typeId;
                        downcastPossible = downcastPossible || (m_classData->m_azRtti && classElementMetadata.m_azRtti && m_classData->m_azRtti->IsTypeOf(classElementMetadata.m_azRtti->GetTypeId()));
                        if (!downcastPossible)
                        {
                            AZStd::string error = AZStd::string::format("Element of type %s cannot be added to container of pointers to type %s!"
                                , m_element.m_id.ToString<AZStd::string>().c_str(), classElementMetadata.m_typeId.ToString<AZStd::string>().c_str());
                            errorHandler->ReportError(error.c_str());

                            return false;
                        }
                    }
                    else
                    {
                        if (m_element.m_id != classElementMetadata.m_typeId)
                        {
                            AZStd::string error = AZStd::string::format("Element of type %s cannot be added to container of type %s!"
                                , m_element.m_id.ToString<AZStd::string>().c_str(), classElementMetadata.m_typeId.ToString<AZStd::string>().c_str());
                            errorHandler->ReportError(error.c_str());

                            return false;
                        }
                    }
                    classElement = classElementMetadata;
                    elementFound = true;
                }
            }
            else
            {
                for (size_t i = 0; i < parentClassData->m_elements.size(); ++i)
                {
                    const SerializeContext::ClassElement* childElement = &parentClassData->m_elements[i];
                    if (childElement->m_nameCrc == m_element.m_nameCrc)
                    {
                        // if the member is a pointer type, then the pointer could be a derived type,
                        // otherwise we need the uuids to be exactly the same.
                        if (childElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                        {
                            // Verify that a downcast is possible
                            bool downcastPossible = m_element.m_id == childElement->m_typeId;
                            downcastPossible = downcastPossible || (m_classData->m_azRtti && childElement->m_azRtti && m_classData->m_azRtti->IsTypeOf(childElement->m_azRtti->GetTypeId()));
                            if (downcastPossible)
                            {
                                classElement = *childElement;
                            }
                        }
                        else
                        {
                            if (m_element.m_id == childElement->m_typeId)
                            {
                                classElement = *childElement;
                            }
                        }
                        elementFound = true;
                        break;
                    }
                }
            }
        }
        return elementFound;
    }

    bool SerializeContext::DataElementNode::GetDataHierarchyEnumerate(ErrorHandler* errorHandler, NodeStack& nodeStack)
    {
        if (nodeStack.empty())
        {
            return true;
        }

        void* parentPtr = nodeStack.back().m_ptr;
        DataElementNode* parentDataElement = nodeStack.back().m_dataElement;
        bool success = true;

        AZ::SerializeContext::ClassElement classElement;
        bool classElementFound = parentDataElement && GetClassElement(classElement, *parentDataElement, errorHandler);

        if (classElementFound)
        {
            void* dataAddress = nullptr;
            void* reserveAddress = nullptr;
            IDataContainer* dataContainer = parentDataElement->m_classData->m_container;
            if (dataContainer) // container elements
            {
                int& parentCurrentContainerElementIndex = nodeStack.back().m_currentContainerElementIndex;
                // add element to the array
                if (dataContainer->CanAccessElementsByIndex() && dataContainer->Size(parentPtr) > parentCurrentContainerElementIndex)
                {
                    dataAddress = dataContainer->GetElementByIndex(parentPtr, &classElement, parentCurrentContainerElementIndex);
                }
                else
                {
                    dataAddress = dataContainer->ReserveElement(parentPtr, &classElement);
                }

                if (dataAddress == nullptr)
                {
                    AZStd::string error = AZStd::string::format("Failed to reserve element in container. The container may be full. Element %u will not be added to container.", static_cast<unsigned int>(parentCurrentContainerElementIndex));
                    errorHandler->ReportError(error.c_str());
                }

                parentCurrentContainerElementIndex++;
            }
            else
            {   // normal elements
                dataAddress = reinterpret_cast<char*>(parentPtr) + classElement.m_offset;
            }

            reserveAddress = dataAddress;

            // create a new instance if needed
            if (classElement.m_flags & SerializeContext::ClassElement::FLG_POINTER)
            {
                // create a new instance if we are referencing it by pointer
                AZ_Assert(m_classData->m_factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide factory or change data member '%s' to value not pointer!", m_classData->m_name, classElement.m_name);
                void* newDataAddress = m_classData->m_factory->Create(m_classData->m_name);

                // we need to account for additional offsets if we have a pointer to
                // a base class.
                void* basePtr = nullptr;
                if (m_element.m_id == classElement.m_typeId)
                {
                    basePtr = newDataAddress;
                }
                else if(m_classData->m_azRtti && classElement.m_azRtti)
                {
                    basePtr = m_classData->m_azRtti->Cast(newDataAddress, classElement.m_azRtti->GetTypeId());
                }
                AZ_Assert(basePtr != nullptr, dataContainer
                    ? "Can't cast container element %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                    : "Can't cast %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                    , m_element.m_name ? m_element.m_name : "NULL"
                    , m_element.m_nameCrc
                    , m_classData->m_name);

                *reinterpret_cast<void**>(dataAddress) = basePtr; // store the pointer in the class
                dataAddress = newDataAddress;
            }

            DataElement& rawElement = m_element;
            if (m_classData->m_serializer && rawElement.m_dataSize != 0)
            {
                if (rawElement.m_dataType == DataElement::DT_TEXT)
                {
                    // convert to binary so we can load the data
                    AZStd::string text;
                    text.resize_no_construct(rawElement.m_dataSize);
                    rawElement.m_byteStream.Read(text.size(), reinterpret_cast<void*>(text.data()));
                    rawElement.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                    rawElement.m_dataSize = m_classData->m_serializer->TextToData(text.c_str(), rawElement.m_version, rawElement.m_byteStream);
                    rawElement.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                    rawElement.m_dataType = DataElement::DT_BINARY;
                }

                bool isLoaded = m_classData->m_serializer->Load(dataAddress, rawElement.m_byteStream, rawElement.m_version, rawElement.m_dataType == DataElement::DT_BINARY_BE);
                rawElement.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN); // reset stream position
                if (!isLoaded)
                {
                    const AZStd::string error =
                        AZStd::string::format(R"(Failed to load rawElement "%s" for class "%s" with parent element "%s".)",
                            rawElement.m_name ? rawElement.m_name : "<unknown>",
                            m_classData->m_name,
                            m_element.m_name);

                    if (errorHandler)
                    {
                        errorHandler->ReportError(error.c_str());
                    }
                    else
                    {
                        AZ_Error("Serialize", "%s", error.c_str());
                    }

                    success = false;
                }
            }

            // Push current DataElementNode to stack
            DataElementInstanceData node;
            node.m_ptr = dataAddress;
            node.m_dataElement = this;
            nodeStack.push_back(node);

            for (int dataElementIndex = 0; dataElementIndex < m_subElements.size(); ++dataElementIndex)
            {
                DataElementNode& subElement = m_subElements[dataElementIndex];
                if (!subElement.GetDataHierarchyEnumerate(errorHandler, nodeStack))
                {
                    success = false;
                    break;
                }
            }

            // Pop stack
            nodeStack.pop_back();

            if (dataContainer)
            {
                dataContainer->StoreElement(parentPtr, reserveAddress);
            }
        }

        return success;
    }

    //=========================================================================
    // GetDataHierarchy
    //=========================================================================
    bool SerializeContext::DataElementNode::GetDataHierarchy(void* objectPtr, const Uuid& classId, ErrorHandler* errorHandler)
    {
        (void)classId;
        AZ_Assert(m_element.m_id == classId, "GetDataHierarchy called with mismatched class type {%s} for element %s",
            classId.ToString<AZStd::string>().c_str(), m_element.m_name);

        NodeStack nodeStack;
        DataElementInstanceData topNode;
        topNode.m_ptr = objectPtr;
        topNode.m_dataElement = this;
        nodeStack.push_back(topNode);

        bool success = true;
        for (size_t i = 0; i < m_subElements.size(); ++i)
        {
            if (!m_subElements[i].GetDataHierarchyEnumerate(errorHandler, nodeStack))
            {
                success = false;
                break;
            }
        }

        return success;
    }

    //=========================================================================
    // ClassBuilder::Version
    // [10/05/2012]
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Version(unsigned int version, VersionConverter converter)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        AZ_Assert(version != VersionClassDeprecated, "You cannot use %u as the version, it is reserved by the system!");
        m_classData->second.m_version = version;
        m_classData->second.m_converter = converter;
        return this;
    }

    //=========================================================================
    // ClassBuilder::Serializer
    // [10/05/2012]
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Serializer(IDataSerializer* serializer)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }

        AZ_Assert(m_classData->second.m_elements.empty(),
            "Class %s has a custom serializer, and can not have additional fields. Classes can either have a custom serializer or child fields.",
            m_classData->second.m_name);

        m_classData->second.m_serializer = serializer;
        return this;
    }

    //=========================================================================
    // ClassBuilder::Serializer
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::SerializerForEmptyClass()
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.m_serializer = &Serialize::StaticInstance<EmptySerializer>::s_instance;
        return this;
    }

    //=========================================================================
    // ClassBuilder::EventHandler
    // [10/05/2012]
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::EventHandler(IEventHandler* eventHandler)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.m_eventHandler = eventHandler;
        return this;
    }

    //=========================================================================
    // ClassBuilder::DataContainer
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::DataContainer(IDataContainer* dataContainer)
    {
        if (m_context->IsRemovingReflection())
        {
            return this;
        }
        m_classData->second.m_container = dataContainer;
        return this;
    }

    //=========================================================================
    // ClassBuilder::PersistentId
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::PersistentId(ClassPersistentId persistentId)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.m_persistentId = persistentId;
        return this;
    }

    //=========================================================================
    // ClassBuilder::SerializerDoSave
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::SerializerDoSave(ClassDoSave doSave)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.m_doSave = doSave;
        return this;
    }

    //=========================================================================
    // EnumerateInstanceConst
    // [10/31/2012]
    //=========================================================================
    bool SerializeContext::EnumerateInstanceConst(const void* ptr, const Uuid& classId, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, const ClassData* classData, const ClassElement* classElement, ErrorHandler* errorHandler) const
    {
        AZ_Assert((accessFlags & ENUM_ACCESS_FOR_WRITE) == 0, "You are asking the serializer to lock the data for write but you only have a const pointer!");
        return EnumerateInstance(const_cast<void*>(ptr), classId, beginElemCB, endElemCB, accessFlags, classData, classElement, errorHandler);
    }

    //=========================================================================
    // EnumerateInstance
    // [10/31/2012]
    //=========================================================================
    bool SerializeContext::EnumerateInstance(void* ptr, const Uuid& _classId, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, const ClassData* classData, const ClassElement* classElement, ErrorHandler* errorHandler) const
    {
        // if useClassData is provided, just use it, otherwise try to find it using the classId provided.
        void* objectPtr = ptr;
        AZ::Uuid classId = _classId;
        const SerializeContext::ClassData* dataClassInfo = classData ? classData : FindClassData(classId, nullptr, 0);

    #if defined(AZ_ENABLE_TRACING)
        AZStd::unique_ptr<ErrorHandler> standardErrorHandler;
        if (!errorHandler)
        {
            standardErrorHandler.reset(aznew ErrorHandler());
            errorHandler = standardErrorHandler.get();
        }
    #endif // AZ_ENABLE_TRACING

        if (classElement)
        {
            // if we are a pointer, then we may be pointing to a derived type.
            if (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
            {
                // if ptr is a pointer-to-pointer, cast its value to a void* (or const void*) and dereference to get to the actual object pointer.
                objectPtr = *(void**)(ptr);

                if (objectPtr == nullptr)
                {
                    return true;    // nothing to serialize
                }
                if (classElement->m_azRtti)
                {
                    AZ::Uuid actualClassId = classElement->m_azRtti->GetActualUuid(objectPtr);
                    if (actualClassId != classId)
                    {
                        // we are pointing to derived type, adjust class data, uuid and pointer.
                        classId = actualClassId;
                        dataClassInfo = FindClassData(actualClassId);
                        if (dataClassInfo)
                        {
                            objectPtr = classElement->m_azRtti->Cast(objectPtr, dataClassInfo->m_azRtti->GetTypeId());
                        }
                    }
                }
            }
        }

    #if defined(AZ_ENABLE_TRACING)
        {
            DbgStackEntry de;
            de.m_dataPtr = objectPtr;
            de.m_uuidPtr = &classId;
            de.m_elementName = classElement ? classElement->m_name : NULL;
            de.m_classData = dataClassInfo;
            de.m_classElement = classElement;
            errorHandler->Push(de);
        }
    #endif // AZ_ENABLE_TRACING

        if (dataClassInfo == NULL)
        {
            // if we are a non-reflected base class, assume that the base class simply didn't need any reflection
            if (!(classElement && classElement->m_flags & SerializeContext::ClassElement::FLG_BASE_CLASS))
            {
                // output an error
                AZStd::string error = AZStd::string::format("Element with class ID '%s' is not registered with the serializer!", classId.ToString<AZStd::string>().c_str());
                errorHandler->ReportError(error.c_str());
            }
    #if defined (AZ_ENABLE_TRACING)
            errorHandler->Pop();
    #endif // AZ_ENABLE_TRACING

            return true;    // we errored, but return true to continue enumeration of our siblings and other unrelated hierarchies
        }

        if (dataClassInfo->m_eventHandler)
        {
            if ((accessFlags & ENUM_ACCESS_FOR_WRITE) == ENUM_ACCESS_FOR_WRITE)
            {
                dataClassInfo->m_eventHandler->OnWriteBegin(objectPtr);
            }
            else
            {
                dataClassInfo->m_eventHandler->OnReadBegin(objectPtr);
            }
        }

        bool keepEnumeratingSiblings = true;

        // Call beginElemCB for this element if there is one. If the callback
        // returns false, stop enumeration of this branch
        // pass the original ptr to the user instead of objectPtr because
        // he may want to replace the actual object.
        if (!beginElemCB || beginElemCB(ptr, dataClassInfo, classElement))
        {
            if (dataClassInfo->m_container)
            {
                dataClassInfo->m_container->EnumElements(objectPtr, AZStd::bind(&SerializeContext::EnumerateInstance, this
                        , AZStd::placeholders::_1
                        , AZStd::placeholders::_2
                        , beginElemCB
                        , endElemCB
                        , accessFlags
                        , AZStd::placeholders::_3
                        , AZStd::placeholders::_4
                        , errorHandler
                        ));
            }
            else
            {
                for (size_t i = 0, n = dataClassInfo->m_elements.size(); i < n; ++i)
                {
                    const SerializeContext::ClassElement& ed = dataClassInfo->m_elements[i];
                    void* dataAddress = (char*)(objectPtr) + ed.m_offset;
                    if (dataAddress)
                    {
                        const SerializeContext::ClassData* elemClassInfo = ed.m_genericClassInfo ? ed.m_genericClassInfo->GetClassData() : FindClassData(ed.m_typeId, dataClassInfo, ed.m_nameCrc);

                        keepEnumeratingSiblings = EnumerateInstance(dataAddress, ed.m_typeId, beginElemCB, endElemCB, accessFlags, elemClassInfo, &ed, errorHandler);
                        if (!keepEnumeratingSiblings)
                        {
                            break;
                        }
                    }
                }

                if (dataClassInfo->m_typeId == SerializeTypeInfo<DynamicSerializableField>::GetUuid())
                {
                    AZ::DynamicSerializableField* dynamicFieldDesc = reinterpret_cast<AZ::DynamicSerializableField*>(objectPtr);
                    if (dynamicFieldDesc->IsValid())
                    {
                        const AZ::SerializeContext::ClassData* dynamicTypeMetadata = FindClassData(dynamicFieldDesc->m_typeId);
                        if (dynamicTypeMetadata)
                        {
                            AZ::SerializeContext::ClassElement dynamicElementData;
                            dynamicElementData.m_name = "m_data";
                            dynamicElementData.m_nameCrc = AZ_CRC("m_data", 0x335cc942);
                            dynamicElementData.m_typeId = dynamicFieldDesc->m_typeId;
                            dynamicElementData.m_dataSize = sizeof(void*);
                            dynamicElementData.m_offset = reinterpret_cast<size_t>(&(reinterpret_cast<AZ::DynamicSerializableField const volatile*>(0)->m_data));
                            dynamicElementData.m_azRtti = nullptr; // we won't need this because we always serialize top classes.
                            dynamicElementData.m_genericClassInfo = FindGenericClassInfo(dynamicFieldDesc->m_typeId);
                            dynamicElementData.m_editData = nullptr; // we cannot have element edit data for dynamic fields.
                            dynamicElementData.m_flags = ClassElement::FLG_DYNAMIC_FIELD | ClassElement::FLG_POINTER;
                            EnumerateInstance(&dynamicFieldDesc->m_data, dynamicTypeMetadata->m_typeId, beginElemCB, endElemCB, accessFlags, dynamicTypeMetadata, &dynamicElementData, errorHandler);
                        }
                        else
                        {
                            AZ_Error("Serialization", false, "Failed to find class data for 'Dynamic Serializable Field' with type=%s address=%p. Make sure this type is reflected, \
                                otherwise you will lose data during serialization!\n", dynamicFieldDesc->m_typeId.ToString<AZStd::string>().c_str(), dynamicFieldDesc->m_data);
                        }
                    }
                }
            }
        }

        // call endElemCB
        if (endElemCB)
        {
            keepEnumeratingSiblings = endElemCB();
        }

        if (dataClassInfo->m_eventHandler)
        {
            if ((accessFlags & ENUM_ACCESS_HOLD) == 0)
            {
                if ((accessFlags & ENUM_ACCESS_FOR_WRITE) == ENUM_ACCESS_FOR_WRITE)
                {
                    dataClassInfo->m_eventHandler->OnWriteEnd(objectPtr);
                }
                else
                {
                    dataClassInfo->m_eventHandler->OnReadEnd(objectPtr);
                }
            }
        }

    #if defined(AZ_ENABLE_TRACING)
        errorHandler->Pop();
    #endif // AZ_ENABLE_TRACING

        return keepEnumeratingSiblings;
    }

    //=========================================================================
    // CloneObject
    // [03/21/2014]
    //=========================================================================
    struct ObjectCloneData
    {
        ObjectCloneData()
        {
            m_parentStack.reserve(10);
        }
        struct ParentInfo
        {
            void* m_ptr;
            void* m_reservePtr; ///< Used for associative containers like set to store the original address returned by ReserveElement
            const SerializeContext::ClassData* m_classData;
            size_t m_containerIndexCounter; ///< Used for fixed containers like array, where the container doesn't store the size.
        };
        typedef AZStd::vector<ParentInfo> ObjectParentStack;

        void*               m_ptr;
        ObjectParentStack   m_parentStack;
    };

    void* SerializeContext::CloneObject(const void* ptr, const Uuid& classId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

        ObjectCloneData cloneData;
        ErrorHandler m_errorLogger;
        EnumerateInstance(const_cast<void*>(ptr)
            , classId
            , AZStd::bind(&SerializeContext::BeginCloneElement, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, &cloneData, &m_errorLogger)
            , AZStd::bind(&SerializeContext::EndCloneElement, this, &cloneData)
            , SerializeContext::ENUM_ACCESS_FOR_READ
            , nullptr
            , nullptr
            , &m_errorLogger
            );
        return cloneData.m_ptr;
    }

    void SerializeContext::CloneObjectInplace(void* dest, const void* ptr, const Uuid& classId)
    {
        ObjectCloneData cloneData;
        cloneData.m_ptr = dest;
        ErrorHandler m_errorLogger;
        EnumerateInstanceConst(ptr
            , classId
            , AZStd::bind(&SerializeContext::BeginCloneElementInplace, this, dest, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, &cloneData, &m_errorLogger)
            , AZStd::bind(&SerializeContext::EndCloneElement, this, &cloneData)
            , SerializeContext::ENUM_ACCESS_FOR_READ
            , nullptr
            , nullptr
            , &m_errorLogger
        );
    }


    bool SerializeContext::BeginCloneElement(void* ptr, const ClassData* classData, const ClassElement* elementData, void* data, ErrorHandler* errorHandler)
    {
        ObjectCloneData* cloneData = reinterpret_cast<ObjectCloneData*>(data);

        if (cloneData->m_parentStack.empty())
        {
            // Since this is the root element, we will need to allocate it using the creator provided
            AZ_Assert(classData->m_factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide factory or change data member '%s' to value not pointer!", classData->m_name, elementData->m_name);
            cloneData->m_ptr = classData->m_factory->Create(classData->m_name);
        }
        
        return BeginCloneElementInplace(cloneData->m_ptr, ptr, classData, elementData, data, errorHandler);
    }

    bool SerializeContext::BeginCloneElementInplace(void* rootDestPtr, void* ptr, const ClassData* classData, const ClassElement* elementData, void* data, ErrorHandler* errorHandler)
    {
        ObjectCloneData* cloneData = reinterpret_cast<ObjectCloneData*>(data);

        void* srcPtr = ptr;
        void* destPtr = nullptr;
        void* reservePtr = nullptr;
        if (!cloneData->m_parentStack.empty())
        {
            AZ_Assert(elementData, "Non-root nodes need to have a valid elementData!");
            ObjectCloneData::ParentInfo& parentInfo = cloneData->m_parentStack.back();
            void* parentPtr = parentInfo.m_ptr;
            const ClassData* parentClassData = parentInfo.m_classData;
            if (parentClassData->m_container)
            {
                if (parentClassData->m_container->CanAccessElementsByIndex() && parentClassData->m_container->Size(parentPtr) > parentInfo.m_containerIndexCounter)
                {
                    destPtr = parentClassData->m_container->GetElementByIndex(parentPtr, elementData, parentInfo.m_containerIndexCounter);
                }
                else
                {
                    destPtr = parentClassData->m_container->ReserveElement(parentPtr, elementData);
                }

                ++parentInfo.m_containerIndexCounter;
            }
            else
            {
                // Allocate memory for our element using the creator provided
                destPtr = reinterpret_cast<char*>(parentPtr) + elementData->m_offset;
            }

            reservePtr = destPtr;
            if (elementData->m_flags & ClassElement::FLG_POINTER)
            {
                AZ_Assert(classData->m_factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide a factory or change data member '%s' to value not pointer!", classData->m_name, elementData->m_name);
                void* newElement = classData->m_factory->Create(classData->m_name);
                void* basePtr = DownCast(newElement, classData->m_typeId, elementData->m_typeId, classData->m_azRtti, elementData->m_azRtti);
                *reinterpret_cast<void**>(destPtr) = basePtr; // store the pointer in the class
                destPtr = newElement;
            }

            if (!destPtr && errorHandler)
            {
                AZStd::string error = AZStd::string::format("Failed to reserve element in container. The container may be full. Element %u will not be added to container.", static_cast<unsigned int>(parentInfo.m_containerIndexCounter));
                errorHandler->ReportError(error.c_str());
            }
        }
        else
        {
            destPtr = rootDestPtr;
            reservePtr = rootDestPtr;
        }

        if (elementData && elementData->m_flags & ClassElement::FLG_POINTER)
        {
            // if ptr is a pointer-to-pointer, cast its value to a void* (or const void*) and dereference to get to the actual object pointer.
            srcPtr = *(void**)(ptr);
            if (elementData->m_azRtti)
            {
                srcPtr = elementData->m_azRtti->Cast(srcPtr, classData->m_azRtti->GetTypeId());
            }
        }

        if (classData->m_eventHandler)
        {
            classData->m_eventHandler->OnWriteBegin(destPtr);
        }
        
        if (classData->m_serializer)
        {
            AZStd::vector<char> buffer;
            IO::ByteContainerStream<AZStd::vector<char> > stream(&buffer);

            classData->m_serializer->Save(srcPtr, stream);
            stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            classData->m_serializer->Load(destPtr, stream, classData->m_version);
        }

        // push this node in the stack
        cloneData->m_parentStack.push_back();
        ObjectCloneData::ParentInfo& parentInfo = cloneData->m_parentStack.back();
        parentInfo.m_ptr = destPtr;
        parentInfo.m_reservePtr = reservePtr;
        parentInfo.m_classData = classData;
        parentInfo.m_containerIndexCounter = 0;
        return true;
    }

    bool SerializeContext::EndCloneElement(void* data)
    {
        ObjectCloneData* cloneData = reinterpret_cast<ObjectCloneData*>(data);
        void* dataPtr = cloneData->m_parentStack.back().m_ptr;
        void* reservePtr = cloneData->m_parentStack.back().m_reservePtr;

        const ClassData* classData = cloneData->m_parentStack.back().m_classData;
        if (classData->m_eventHandler)
        {
            classData->m_eventHandler->OnWriteEnd(dataPtr);
        }

        cloneData->m_parentStack.pop_back();
        if (!cloneData->m_parentStack.empty())
        {
            const ClassData* parentClassData = cloneData->m_parentStack.back().m_classData;
            if (parentClassData->m_container)
            {
                // Pass in the address returned by IDataContainer::ReserveElement.
                //AZStdAssociativeContainer is the only DataContainer that uses the second argument passed into IDataContainer::StoreElement
                parentClassData->m_container->StoreElement(cloneData->m_parentStack.back().m_ptr, reservePtr);
            }
        }
        return true;
    }

    //=========================================================================
    // EnumerateDerived
    // [11/13/2012]
    //=========================================================================
    void SerializeContext::EnumerateDerived(const TypeInfoCB& callback, const Uuid& classId, const Uuid& typeId)
    {
        // right now this function is SLOW, traverses all serialized types. If we need faster
        // we will need to cache/store derived type in the base type.
        for (SerializeContext::UuidToClassMap::const_iterator it = m_uuidMap.begin(); it != m_uuidMap.end(); ++it)
        {
            const ClassData& cd = it->second;

            if (cd.m_typeId == classId)
            {
                continue;
            }

            if (cd.m_azRtti && typeId != Uuid::CreateNull())
            {
                if (cd.m_azRtti->IsTypeOf(typeId))
                {
                    if (!callback(&cd, 0))
                    {
                        return;
                    }
                }
            }

            if (!classId.IsNull())
            {
                for (size_t i = 0; i < cd.m_elements.size(); ++i)
                {
                    if ((cd.m_elements[i].m_flags & ClassElement::FLG_BASE_CLASS) != 0)
                    {
                        if (cd.m_elements[i].m_typeId == classId)
                        {
                            // if both classes have azRtti they will be enumerated already by the code above (azrtti)
                            if (cd.m_azRtti == nullptr || cd.m_elements[i].m_azRtti == nullptr)
                            {
                                if (!callback(&cd, 0))
                                {
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //=========================================================================
    // EnumerateBase
    // [11/13/2012]
    //=========================================================================
    void SerializeContext::EnumerateBase(const TypeInfoCB& callback, const Uuid& classId)
    {
        const ClassData* cd = FindClassData(classId);
        if (cd)
        {
            EnumerateBaseRTTIEnumCallbackData callbackData;
            callbackData.m_callback = &callback;
            callbackData.m_reportedTypes.push_back(cd->m_typeId);
            for (size_t i = 0; i < cd->m_elements.size(); ++i)
            {
                if ((cd->m_elements[i].m_flags & ClassElement::FLG_BASE_CLASS) != 0)
                {
                    const ClassData* baseClassData = FindClassData(cd->m_elements[i].m_typeId);
                    if (baseClassData)
                    {
                        callbackData.m_reportedTypes.push_back(baseClassData->m_typeId);
                        if (!callback(baseClassData, 0))
                        {
                            return;
                        }
                    }
                }
            }
            if (cd->m_azRtti)
            {
                cd->m_azRtti->EnumHierarchy(&SerializeContext::EnumerateBaseRTTIEnumCallback, &callbackData);
            }
        }
    }

    //=========================================================================
    // EnumerateAll
    //=========================================================================
    void SerializeContext::EnumerateAll(const TypeInfoCB& callback) const
    {
        for (auto& uuidToClassPair : m_uuidMap)
        {
            const ClassData& classData = uuidToClassPair.second;
            if (!callback(&classData, classData.m_typeId))
            {
                return;
            }
        }
    }

    //=========================================================================
    // EnumerateBaseRTTIEnumCallback
    // [11/13/2012]
    //=========================================================================
    void SerializeContext::EnumerateBaseRTTIEnumCallback(const Uuid& id, void* userData)
    {
        EnumerateBaseRTTIEnumCallbackData* callbackData = reinterpret_cast<EnumerateBaseRTTIEnumCallbackData*>(userData);
        // if not reported, report
        if (AZStd::find(callbackData->m_reportedTypes.begin(), callbackData->m_reportedTypes.end(), id) == callbackData->m_reportedTypes.end())
        {
            (*callbackData->m_callback)(nullptr, id);
        }
    }

    //=========================================================================
    // ClassData
    //=========================================================================
    SerializeContext::ClassData::ClassData()
    {
        m_name = nullptr;
        m_typeId = Uuid::CreateNull();
        m_version = 0;
        m_converter = nullptr;
        m_serializer = nullptr;
        m_factory = nullptr;
        m_persistentId = nullptr;
        m_doSave = nullptr;
        m_eventHandler = nullptr;
        m_container = nullptr;
        m_azRtti = nullptr;
        m_editData = nullptr;
    }

    //=========================================================================
    // ClassData
    //=========================================================================
    void SerializeContext::ClassData::ClearAttributes()
    {
        for (auto& attributePair : m_attributes)
        {
            delete attributePair.second;
        }
        m_attributes.clear();

        for (ClassElement& classElement : m_elements)
        {
            for (auto& attributePair : classElement.m_attributes)
            {
                delete attributePair.second;
            }
            classElement.m_attributes.clear();
        }
    }

    SerializeContext::ClassPersistentId SerializeContext::ClassData::GetPersistentId(const SerializeContext& context) const
    {
        ClassPersistentId persistentIdFunction = m_persistentId;
        if (!persistentIdFunction)
        {
            // check the base classes
            for (const SerializeContext::ClassElement& element : m_elements)
            {
                if (element.m_flags & ClassElement::FLG_BASE_CLASS)
                {
                    const SerializeContext::ClassData* baseClassData = context.FindClassData(element.m_typeId);
                    if (baseClassData)
                    {
                        persistentIdFunction = baseClassData->GetPersistentId(context);
                        if (persistentIdFunction)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    // base classes are in the beginning of the array
                    break;
                }
            }
        }
        return persistentIdFunction;
    }

    //=========================================================================
    // ToString
    // [11/1/2012]
    //=========================================================================
    void SerializeContext::DbgStackEntry::ToString(AZStd::string& str) const
    {
        str += "[";
        if (m_elementName)
        {
            str += AZStd::string::format(" Element: '%s' of", m_elementName);
        }
        //if( m_classElement )
        //  str += AZStd::string::format(" Offset: %d",m_classElement->m_offset);
        if (m_classData)
        {
            str += AZStd::string::format(" Class: '%s' Version: %d", m_classData->m_name, m_classData->m_version);
        }
        str += AZStd::string::format(" Address: %p Uuid: %s", m_dataPtr, m_uuidPtr->ToString<AZStd::string>().c_str());
        str += " ]\n";
    }

    //=========================================================================
    // FreeElementPointer
    // [12/7/2012]
    //=========================================================================
    void SerializeContext::IDataContainer::DeletePointerData(SerializeContext* context, const ClassElement* classElement, void* element)
    {
        AZ_Assert(context != NULL && classElement != NULL && element != NULL, "Invalid input");
        const AZ::Uuid* elemUuid = &classElement->m_typeId;
        void* dataPtr = *reinterpret_cast<void**>(element);
        // find the class data for the specific element
        const SerializeContext::ClassData* classData = classElement->m_genericClassInfo ? classElement->m_genericClassInfo->GetClassData() : context->FindClassData(*elemUuid, NULL, 0);
        if (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
        {
            // if dataAddress is a pointer in this case, cast it's value to a void* (or const void*) and dereference to get to the actual class.
            if (dataPtr && classElement->m_azRtti)
            {
                const AZ::Uuid* actualClassId = &classElement->m_azRtti->GetActualUuid(dataPtr);
                if (*actualClassId != *elemUuid)
                {
                    // we are pointing to derived type, adjust class data, uuid and pointer.
                    classData = context->FindClassData(*actualClassId, NULL, 0);
                    elemUuid = actualClassId;
                    if (classData)
                    {
                        dataPtr = classElement->m_azRtti->Cast(dataPtr, classData->m_azRtti->GetTypeId());
                    }
                }
            }
        }
        if (classData == NULL)
        {
            if ((classElement->m_flags & ClassElement::FLG_POINTER) != 0)
            {
                AZ_Warning("Serialization", false, "Failed to find class id%s for %p! Memory could leak.", elemUuid->ToString<AZStd::string>().c_str(), dataPtr);
            }
            return;
        }

        if (classData->m_container)  // if element is container forward the message
        {
            // clear all container data
            classData->m_container->ClearElements(element, context);
        }
        else
        {
            if ((classElement->m_flags & ClassElement::FLG_POINTER) == 0)
            {
                return; // element is stored by value nothing to free
            }
            if (classData->m_factory)
            {
                classData->m_factory->Destroy(dataPtr);
            }
            else
            {
                AZ_Warning("Serialization", false, "Failed to delete %p '%s' element, no destructor is provided! Memory could leak.", dataPtr, classData->m_name);
            }
        }
    }

    //=========================================================================
    // ReportError
    //=========================================================================
    void SerializeContext::RemoveClassData(ClassData* classData)
    {
        if (m_editContext)
        {
            m_editContext->RemoveClassData(classData);
        }

        classData->ClearAttributes();
    }

    //=========================================================================
    // GetStackDescription
    //=========================================================================
    AZStd::string SerializeContext::ErrorHandler::GetStackDescription() const
    {
        AZStd::string stackDescription;

    #ifdef AZ_ENABLE_TRACING
        if (!m_stack.empty())
        {
            stackDescription += "\n=== Serialize stack ===\n";
            for (size_t i = 0; i < m_stack.size(); ++i)
            {
                m_stack[i].ToString(stackDescription);
            }
            stackDescription += "\n";
        }
    #endif // AZ_ENABLE_TRACING

        return stackDescription;
    }

    //=========================================================================
    // ReportError
    // [12/11/2012]
    //=========================================================================
    void SerializeContext::ErrorHandler::ReportError(const char* message)
    {
        (void)message;
        AZ_Error("Serialize", false, "%s\n%s", message, GetStackDescription().c_str());
        m_nErrors++;
    }
    
    //=========================================================================
    // ReportWarning
    //=========================================================================
    void SerializeContext::ErrorHandler::ReportWarning(const char* message)
    {
        (void)message;
        AZ_Warning("Serialize", false, "%s\n%s", message, GetStackDescription().c_str());
        m_nWarnings++;
    }

    //=========================================================================
    // Push
    // [1/3/2013]
    //=========================================================================
    void SerializeContext::ErrorHandler::Push(const DbgStackEntry& de)
    {
        (void)de;
    #ifdef AZ_ENABLE_TRACING
        m_stack.push_back((de));
    #endif // AZ_ENABLE_TRACING
    }

    //=========================================================================
    // Pop
    // [1/3/2013]
    //=========================================================================
    void SerializeContext::ErrorHandler::Pop()
    {
    #ifdef AZ_ENABLE_TRACING
        m_stack.pop_back();
    #endif // AZ_ENABLE_TRACING
    }

    //=========================================================================
    // Reset
    // [1/23/2013]
    //=========================================================================
    void SerializeContext::ErrorHandler::Reset()
    {
    #ifdef AZ_ENABLE_TRACING
        m_stack.clear();
    #endif
        m_nErrors = 0;
    } // AZ_ENABLE_TRACING
}   // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD
