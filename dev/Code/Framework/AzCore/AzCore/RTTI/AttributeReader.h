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
#ifndef AZCORE_ATTRIBUTE_READER_H
#define AZCORE_ATTRIBUTE_READER_H

#include <AzCore/RTTI/ReflectContext.h>

namespace AZ
{
    namespace Internal
    {
        template<class AttrType, class DestType, typename ... Args>
        bool AttributeRead(DestType& value, Attribute* attr, void* instance, Args&& ... args)
        {
            // try a value
            AttributeData<AttrType>* data = azdynamic_cast<AttributeData<AttrType>*>(attr);
            if (data)
            {
                value = static_cast<DestType>(data->Get(instance));
                return true;
            }
            // try a function with return type
            AttributeFunction<AttrType(Args...)>* func = azdynamic_cast<AttributeFunction<AttrType(Args...)>*>(attr);
            if (func)
            {
                value = static_cast<DestType>(func->Invoke(instance, AZStd::forward<Args>(args) ...));
                return true;
            }
            // else you are on your own!
            return false;
        }

        /**
        * Integral types (except bool) can be converted char <-> short <-> int <-> int64
        * for bool we require exact matching (as comparing to zero is ambiguous)
        * The same applied for floating point types, we can convert float <-> double, but we don't allow to ints
        * if we want to do so it's very easy.
        */
        template<class AttrType, class DestType, bool IsIntegral = AZStd::is_integral<DestType>::value&& !AZStd::is_same<DestType, bool>::value, bool IsFloat = AZStd::is_floating_point<DestType>::value, typename ... Args >
        struct AttributeReader
        {
            static bool Read(DestType& value, Attribute* attr, void* instance, Args&& ... args)
            {
                return AttributeRead<AttrType, DestType>(value, attr, instance, AZStd::forward<Args>(args) ...);
            }
        };

        template<class AttrType, class DestType, typename ... Args >
        struct AttributeReaderArgs
        {
            static bool Read(DestType& value, Attribute* attr, void* instance, Args&& ... args)
            {
                return AttributeReader <
                    AttrType,
                    DestType,
                    AZStd::is_integral<DestType>::value && !AZStd::is_same<DestType, bool>::value,
                    AZStd::is_floating_point<DestType>::value,
                    Args ... > ::Read(value, attr, instance, AZStd::forward<Args>(args) ...);
            }
        };

        template<class AttrType, class DestType, typename ... Args>
        struct AttributeReader<AttrType, DestType, true, false, Args...>
        {
            static bool Read(DestType& value, Attribute* attr, void* classInstance, Args&& ... args)
            {
                if (AttributeRead<bool, DestType, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                if (AttributeRead<char, DestType, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                if (AttributeRead<unsigned char, DestType, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                if (AttributeRead<short, DestType, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                if (AttributeRead<unsigned short, DestType, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                if (AttributeRead<int, DestType, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                if (AttributeRead<unsigned int, Args..., DestType>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                if (AttributeRead<AZ::s64, DestType, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                if (AttributeRead<AZ::u64, DestType, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                if (AttributeRead<AZ::Crc32, DestType, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                return false;
            }
        };

        template<class AttrType, class DestType, typename ... Args>
        struct AttributeReader<AttrType, DestType, false, true, Args...>
        {
            static bool Read(DestType& value, Attribute* attr, void* classInstance, Args&& ... args)
            {
                if (AttributeRead<float, DestType, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                if (AttributeRead<double, DestType, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                return false;
            }
        };

        template<typename ... Args>
        struct AttributeReader<AZStd::string, AZStd::string, false, false, Args...>
        {
            static bool Read(AZStd::string& value, Attribute* attr, void* classInstance, Args&& ... args)
            {
                if (AttributeRead<const char*, AZStd::string, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                if (AttributeRead<AZStd::string, AZStd::string, Args...>(value, attr, classInstance, AZStd::forward<Args>(args) ...))
                {
                    return true;
                }
                return false;
            }
        };
    } // namespace Internal

    // this class is what you get in your reader function for attributes.
    // essentially, just call Read<ExpectedTypeOfData>(someVariableOfThatType), don't worry about it.
    // Special rules
    // ints will automatically convert if the type fits (for example, even if the attribute is a u8, you can read it in as a u64 or u32 and it will work)
    // floats will automatically convert if the type fits.
    // AZStd::string will also accept const char*  (But not the other way around)
    //
    class AttributeReader
    {
    public:
        AZ_CLASS_ALLOCATOR(AttributeReader, AZ::SystemAllocator, 0);

        // this is the only thing you should need to call!
        // returns false if it is an incompatible type or fails to read it.
        // for example, Read<int>(someInteger);
        // for example, Read<double>(someDouble);
        // for example, Read<MyStruct>(someStruct that has operator=(const other&) defined)
        template <class T, class U, typename ... Args>
        bool Read(U& value, Args&& ... args)
        {
            return Internal::AttributeReaderArgs<T, U, Args...>::Read(value, m_attribute, m_instancePointer, AZStd::forward<Args>(args) ...);
        }

        // ------------ private implementation ---------------------------------
        AttributeReader(void* instancePointer, Attribute* attrib)
            : m_instancePointer(instancePointer)
            , m_attribute(attrib)
        {
        }

        Attribute* GetAttribute() const { return m_attribute; }
        void* GetInstancePointer() const { return m_instancePointer; }

    private:
        void* m_instancePointer;
        Attribute* m_attribute;
    };


} // namespace AZ

#endif // AZCORE_ATTRIBUTE_READER_H
