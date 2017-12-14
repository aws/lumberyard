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
#ifndef AZCORE_TYPEINFO_H
#define AZCORE_TYPEINFO_H

#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <AzCore/std/typetraits/is_const.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/remove_pointer.h>
#include <AzCore/std/typetraits/remove_const.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/has_member_function.h>

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Crc.h>

#include <cstdio> // for snprintf

// VS2013 needs atomics for TypeIdHolder
#if defined(AZ_COMPILER_MSVC) && _MSC_VER <= 1800
#include <AzCore/std/parallel/atomic.h>
#endif

namespace AZStd
{
    template <class T>
    struct less;
    template <class T>
    struct less_equal;
    template <class T>
    struct greater;
    template <class T>
    struct greater_equal;
    template <class T>
    struct equal_to;
    template <class T>
    struct hash;
    template< class T1, class T2>
    struct pair;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class vector;
    template< class T, AZStd::size_t N >
    class array;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class list;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class forward_list;
    template<class Key, class MappedType, class Compare /*= AZStd::less<Key>*/, class Allocator /*= AZStd::allocator*/>
    class map;
    template<class Key, class MappedType, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/ >
    class unordered_map;
    template<class Key, class MappedType, class Hasher /* = AZStd::hash<Key>*/, class EqualKey /* = AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/ >
    class unordered_multimap;
    template <class Key, class Compare, class Allocator>
    class set;
    template<class Key, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/>
    class unordered_set;
    template<class Key, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/>
    class unordered_multiset;
    template<class T, size_t Capacity>
    class fixed_vector;
    template<AZStd::size_t NumBits>
    class bitset;

    template<class T>
    class intrusive_ptr;
    template<class T>
    class shared_ptr;

    template<class Element, class Traits, class Allocator>
    class basic_string;

    template<class Element>
    struct char_traits;

    template <class Element, class Traits>
    class basic_string_view;

    template<class Signature>
    class function;
}

namespace AZ
{
    /**
     * Used to reference unique types by their unique id.
     */
    using TypeId = AZ::Uuid;

    template <class T, bool IsEnum = AZStd::is_enum<T>::value>
    struct AzTypeInfo;

    namespace Internal
    {
        /// Check if a class has TypeInfo declared via AZ_TYPE_INFO in its declaration
        AZ_HAS_MEMBER(AZTypeInfoIntrusive, TYPEINFO_Enable, void, ());

        /// Check if a class has an AzTypeInfo specialization
        AZ_HAS_STATIC_MEMBER(AZTypeInfoSpecialized, Specialized, bool, ());

        template <class T>
        struct HasAZTypeInfo
        {
            static const bool value = HasAZTypeInfoSpecialized<AzTypeInfo<T, AZStd::is_enum<T>::value>>::value || HasAZTypeInfoIntrusive<T>::value;
            using type = typename AZStd::Utils::if_c<value, AZStd::true_type, AZStd::false_type>::type;
        };

        inline void AzTypeInfoSafeCat(char* destination, size_t maxDestination, const char* source)
        {
            if (!source || !destination || maxDestination == 0)
            {
                return;
            }
            size_t destinationLen = strlen(destination);
            size_t sourceLen = strlen(source);
            if (sourceLen == 0)
            {
                return;
            }
            size_t maxToCopy = maxDestination - destinationLen - 1;
            if (maxToCopy == 0)
            {
                return;
            }
            // clamp with max length
            if (sourceLen > maxToCopy)
            {
                sourceLen = maxToCopy;
            }
            destination += destinationLen;
            memcpy(destination, source, sourceLen);
            destination += sourceLen;
            *destination = 0;
        }

        template<class... Tn>
        struct AggregateTypes;

        template<class T1, class... Tn>
        struct AggregateTypes<T1, Tn...>
        {
            static void TypeName(char* buffer, size_t bufferSize)
            {
                AzTypeInfoSafeCat(buffer, bufferSize, AzTypeInfo<T1>::Name());
                AzTypeInfoSafeCat(buffer, bufferSize, " ");
                AggregateTypes<Tn...>::TypeName(buffer, bufferSize);
            }

            static AZ::TypeId Uuid()
            {
                const AZ::TypeId tail = AggregateTypes<Tn...>::Uuid();
                if (!tail.IsNull())
                {
                    // Avoid accumulating a null uuid, since it affects the result.
                    return AzTypeInfo<T1>::Uuid() + tail;
                }
                else
                {
                    return AzTypeInfo<T1>::Uuid();
                }
            }
        };

        template<>
        struct AggregateTypes<>
        {
            static void TypeName(char* buffer, size_t bufferSize)
            {
                (void)buffer; (void)bufferSize;
            }

            static AZ::TypeId Uuid()
            {
                return AZ::TypeId::CreateNull();
            }
        };

        // VS2013 does not handle non-pod function statics correctly in a thread-safe manner
        // VS2015+/clang init them as part of static init, or interlocked (as per standard)
#if defined(AZ_COMPILER_MSVC) && _MSC_VER <= 1800
        struct TypeIdHolder
        {
            AZStd::aligned_storage<sizeof(AZ::TypeId), 16>::type m_uuidBuffer;
            AZStd::atomic<const AZ::TypeId*> m_uuid; // This is intentionally not initialized so it doesn't stomp another thread's init
            TypeIdHolder(const AZ::TypeId& uuid)
            {
                m_uuid.store(new (&m_uuidBuffer) AZ::TypeId(uuid));
            }
            operator const AZ::TypeId&() const
            {
                // spin wait for m_uuid to have the only possible correct value, someone must be constructing it
                const AZ::TypeId* typeId = nullptr;
                do {
                    typeId = m_uuid.load();
                } while (typeId != reinterpret_cast<const AZ::TypeId*>(&m_uuidBuffer));
                return *typeId;
            }
        };
#else
        using TypeIdHolder = AZ::TypeId;
#endif
    } // namespace Internal

      /**
      * Since we fully support cross shared libraries (DLL) operation, we can no longer rely on typeid, static templates, etc.
      * to generate the same result in different compilations. We will need to used (codegen too when used) to generate a unique
      * ID for each type. By default we will try to access to static functions inside a class that will provide this identifiers.
      * For classes when intrusive is not an option, you will need to specialize the AzTypeInfo. All of those are automated with
      * code generator.
      * For a reference we used AZ_FUNCTION_SIGNATURE to generate a type ID, but some compilers failed to always expand the template
      * type, thus giving is the same string for many time and we need a robust system.
      */

      // Specialization for types with intrusive (AZ_TYPE_INFO) type info
    template<class T>
    struct AzTypeInfo<T, false /* is_enum */>
    {
        AZ_STATIC_ASSERT(Internal::HasAZTypeInfoIntrusive<T>::value,
            "You should use AZ_TYPE_INFO or AZ_RTTI in your class/struct, or use AZ_TYPE_INFO_SPECIALIZE() externally. "
            "Make sure to include the header containing this information (usually your class header).");
        static const char* Name() { return T::TYPEINFO_Name(); }
        static const AZ::TypeId& Uuid() { return T::TYPEINFO_Uuid(); }
    };

    // Default Specialization for enums without an overload
    template <class T>
    struct AzTypeInfo<T, true /* is_enum */>
    {
        static const char* Name() { return nullptr; }
        static const AZ::TypeId& Uuid() { static AZ::TypeId nullUuid = AZ::TypeId::CreateNull(); return nullUuid; }
    };

    template<class T, class U>
    inline bool operator==(AzTypeInfo<T> const& lhs, AzTypeInfo<U> const& rhs)
    {
        return lhs.Uuid() == rhs.Uuid();
    }

    template<class T, class U>
    inline bool operator!=(AzTypeInfo<T> const& lhs, AzTypeInfo<U> const& rhs)
    {
        return lhs.Uuid() != rhs.Uuid();
    }

    // AzTypeInfo specialization helper for non intrusive TypeInfo
#define AZ_TYPE_INFO_SPECIALIZE(_ClassName, _ClassUuid)                                      \
    template<>                                                                               \
    struct AzTypeInfo<_ClassName, AZStd::is_enum<_ClassName>::value>                         \
    {                                                                                        \
        static const char* Name() { return #_ClassName; }                                    \
        static const AZ::TypeId& Uuid() {                                                      \
            static AZ::Internal::TypeIdHolder s_uuid(_ClassUuid);                  \
            return s_uuid;                                                                   \
        }                                                                                    \
        static bool Specialized() { return true; }                                           \
    };

    AZ_TYPE_INFO_SPECIALIZE(char, "{3AB0037F-AF8D-48ce-BCA0-A170D18B2C03}");
    //    AZ_TYPE_INFO_SPECIALIZE(signed char, "{CFD606FE-41B8-4744-B79F-8A6BD97713D8}");
    AZ_TYPE_INFO_SPECIALIZE(AZ::s8, "{58422C0E-1E47-4854-98E6-34098F6FE12D}");
    AZ_TYPE_INFO_SPECIALIZE(short, "{B8A56D56-A10D-4dce-9F63-405EE243DD3C}");
    AZ_TYPE_INFO_SPECIALIZE(int, "{72039442-EB38-4d42-A1AD-CB68F7E0EEF6}");
    AZ_TYPE_INFO_SPECIALIZE(long, "{8F24B9AD-7C51-46cf-B2F8-277356957325}");
    AZ_TYPE_INFO_SPECIALIZE(AZ::s64, "{70D8A282-A1EA-462d-9D04-51EDE81FAC2F}");
    AZ_TYPE_INFO_SPECIALIZE(unsigned char, "{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}");
    AZ_TYPE_INFO_SPECIALIZE(unsigned short, "{ECA0B403-C4F8-4b86-95FC-81688D046E40}");
    AZ_TYPE_INFO_SPECIALIZE(unsigned int, "{43DA906B-7DEF-4ca8-9790-854106D3F983}");
    AZ_TYPE_INFO_SPECIALIZE(unsigned long, "{5EC2D6F7-6859-400f-9215-C106F5B10E53}");
    AZ_TYPE_INFO_SPECIALIZE(AZ::u64, "{D6597933-47CD-4fc8-B911-63F3E2B0993A}");
    AZ_TYPE_INFO_SPECIALIZE(float, "{EA2C3E90-AFBE-44d4-A90D-FAAF79BAF93D}");
    AZ_TYPE_INFO_SPECIALIZE(double, "{110C4B14-11A8-4e9d-8638-5051013A56AC}");
    AZ_TYPE_INFO_SPECIALIZE(bool, "{A0CA880C-AFE4-43cb-926C-59AC48496112}");
    AZ_TYPE_INFO_SPECIALIZE(AZ::Uuid, "{E152C105-A133-4d03-BBF8-3D4B2FBA3E2A}");
    AZ_TYPE_INFO_SPECIALIZE(void, "{C0F1AFAD-5CB3-450E-B0F5-ADB5D46B0E22}");
    AZ_TYPE_INFO_SPECIALIZE(Crc32, "{9F4E062E-06A0-46D4-85DF-E0DA96467D3A}");
    AZ_TYPE_INFO_SPECIALIZE(PlatformID, "{0635D08E-DDD2-48DE-A7AE-73CC563C57C3}")

    // specialize for function pointers
    template<class R, class... Args>
    struct AzTypeInfo<R(Args...), false>
    {
        static const char* Name()
        {
            static char typeName[64] = { 0 };
            if (typeName[0] == 0)
            {
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "{");
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<R>::Name());
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "(");
                AZ::Internal::AggregateTypes<Args...>::TypeName(typeName, AZ_ARRAY_SIZE(typeName));
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ")}");
            }
            return typeName;
        }
        static const AZ::TypeId& Uuid()
        {
            static AZ::Internal::TypeIdHolder s_uuid(AZ::Internal::AggregateTypes<R, Args...>::Uuid());
            return s_uuid;
        }
    };

    // specialize for member function pointers?

    // Helper macro to generically specialize const, volatile, pointers, etc
#define AZ_TYPE_INFO_SPECIALIZE_CV(_T1, _Specialization, _NamePrefix, _NameSuffix)                               \
    template<class _T1>                                                                                          \
    struct AzTypeInfo<_Specialization, false> {                                                                  \
        static const char* Name() {                                                                              \
            static char typeName[64] = { 0 };                                                                    \
            if (typeName[0] == 0) {                                                                              \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NamePrefix);                 \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T1>::Name()); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NameSuffix);                 \
            }                                                                                                    \
            return typeName;                                                                                     \
        }                                                                                                        \
        static const AZ::TypeId& Uuid() {                                                                          \
            static AZ::Internal::TypeIdHolder s_uuid(AZ::AzTypeInfo<_T1>::Uuid());                    \
            return s_uuid;                                                                                       \
        }                                                                                                        \
        static bool Specialized() { return true; }                                                               \
    }

    AZ_TYPE_INFO_SPECIALIZE_CV(T, T*, "", "*");
    AZ_TYPE_INFO_SPECIALIZE_CV(T, T &, "", "&");
    AZ_TYPE_INFO_SPECIALIZE_CV(T, T &&, "", "&&");
    AZ_TYPE_INFO_SPECIALIZE_CV(T, const T*, "const ", "*");
    AZ_TYPE_INFO_SPECIALIZE_CV(T, const T&, "const ", "&");
    AZ_TYPE_INFO_SPECIALIZE_CV(T, const T&&, "const ", "&&");
    AZ_TYPE_INFO_SPECIALIZE_CV(T, const T, "const ", "");

    // Helper macros to generically specialize for pointers, ref, const, etc.
#define AZ_INTERNAL_VARIATION_SPECIALIZATION_1(_T1, _Specialization, _NamePrefix, _NameSuffix, _AddUuid)         \
    template<class _T1>                                                                                          \
    struct AzTypeInfo<_Specialization<_T1>, false> {                                                                  \
        static const char* Name() {                                                                              \
            static char typeName[64] = { 0 };                                                                    \
            if (typeName[0] == 0) {                                                                              \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NamePrefix);                 \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T1>::Name()); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NameSuffix);                 \
            }                                                                                                    \
            return typeName;                                                                                     \
        }                                                                                                        \
        static const AZ::TypeId& Uuid() {                                                                          \
            static AZ::Internal::TypeIdHolder s_uuid(AZ::AzTypeInfo<_T1>::Uuid() + AZ::TypeId(_AddUuid)); \
            return s_uuid;                                                                                       \
        }                                                                                                        \
        static bool Specialized() { return true; }                                                               \
    }

#define AZ_INTERNAL_VARIATION_SPECIALIZATION_2(_T1, _T2, _Specialization, _NamePrefix, _NameSuffix, _AddUuid)        \
    template<class _T1, class _T2>                                                                                   \
    struct AzTypeInfo<_Specialization<_T1, _T2>, false> {                                                            \
        static const char* Name() {                                                                                  \
            static char typeName[128] = { 0 };                                                                       \
            if (typeName[0] == 0) {                                                                                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NamePrefix);                     \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T1>::Name());     \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", ");                            \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T2>::Name());     \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NameSuffix);                     \
            }                                                                                                        \
            return typeName;                                                                                         \
        }                                                                                                            \
        static const AZ::TypeId& Uuid() {                                                                              \
            static AZ::Internal::TypeIdHolder s_uuid(AZ::AzTypeInfo<_T1>::Uuid() + AZ::AzTypeInfo<_T2>::Uuid() + AZ::TypeId(_AddUuid)); \
            return s_uuid;                                                                                           \
        }                                                                                                            \
        static bool Specialized() { return true; }                                                                   \
    }

#define AZ_INTERNAL_VARIATION_SPECIALIZATION_3(_T1, _T2, _T3, _Specialization, _NamePrefix, _NameSuffix, _AddUuid)                                  \
    template<class _T1, class _T2, class _T3>                                                                                                       \
    struct AzTypeInfo<_Specialization<_T1, _T2, _T3>, false> {                                                                                      \
        static const char* Name() {                                                                                                                 \
            static char typeName[128] = { 0 };                                                                                                      \
            if (typeName[0] == 0) {                                                                                                                 \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NamePrefix);                                                    \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T1>::Name());                                    \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", ");                                                           \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T2>::Name());                                    \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", ");                                                           \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T3>::Name());                                    \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NameSuffix);                                                    \
            }                                                                                                                                       \
            return typeName;                                                                                                                        \
        }                                                                                                                                           \
        static const AZ::TypeId& Uuid() {                                                                                                             \
            static AZ::Internal::TypeIdHolder s_uuid(                                                                 \
                AZ::AzTypeInfo<_T1>::Uuid() + AZ::AzTypeInfo<_T2>::Uuid() + AZ::AzTypeInfo<_T3>::Uuid() + AZ::TypeId(_AddUuid));                      \
            return s_uuid;                                                                                                                          \
        }                                                                                                                                           \
        static bool Specialized() { return true; }                                                                                                  \
    }

#define AZ_INTERNAL_VARIATION_SPECIALIZATION_4(_T1, _T2, _T3, _T4, _Specialization, _NamePrefix, _NameSuffix, _AddUuid)                                                           \
    template<class _T1, class _T2, class _T3, class _T4>                                                                                                                          \
    struct AzTypeInfo<_Specialization<_T1, _T2, _T3, _T4>, false> {                                                                                                               \
        static const char* Name() {                                                                                                                                               \
            static char typeName[128] = { 0 };                                                                                                                                    \
            if (typeName[0] == 0) {                                                                                                                                               \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NamePrefix);                                                                                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T1>::Name());                                                                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", ");                                                                                         \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T2>::Name());                                                                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", ");                                                                                         \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T3>::Name());                                                                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", ");                                                                                         \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T4>::Name());                                                                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NameSuffix);                                                                                  \
            }                                                                                                                                                                     \
            return typeName;                                                                                                                                                      \
        }                                                                                                                                                                         \
        static const AZ::TypeId& Uuid() {                                                                                                                                           \
            static AZ::Internal::TypeIdHolder s_uuid(                                                                                          \
                AZ::AzTypeInfo<_T1>::Uuid() + AZ::AzTypeInfo<_T2>::Uuid() + AZ::AzTypeInfo<_T3>::Uuid() + AZ::AzTypeInfo<_T4>::Uuid() + AZ::TypeId(_AddUuid));                      \
            return s_uuid;                                                                                                                                                        \
        }                                                                                                                                                                         \
        static bool Specialized() { return true; }                                                                                                                                \
    }

#define AZ_INTERNAL_VARIATION_SPECIALIZATION_5(_T1, _T2, _T3, _T4, _T5, _Specialization, _NamePrefix, _NameSuffix, _AddUuid)      \
    template<class _T1, class _T2, class _T3, class _T4, class _T5>                                                               \
    struct AzTypeInfo<_Specialization<_T1, _T2, _T3, _T4, _T5>, false> {                                                          \
        static const char* Name() {                                                                                               \
            static char typeName[128] = { 0 };                                                                                    \
            if (typeName[0] == 0) {                                                                                               \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NamePrefix);                                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T1>::Name());                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", ");                                         \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T2>::Name());                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", ");                                         \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T3>::Name());                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", ");                                         \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T4>::Name());                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", ");                                         \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T5>::Name());                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NameSuffix);                                  \
            }                                                                                                                     \
            return typeName;                                                                                                      \
        }                                                                                                                         \
        static const AZ::TypeId& Uuid() {                                                                                           \
            static AZ::Internal::TypeIdHolder s_uuid(                                     \
                AZ::AzTypeInfo<_T1>::Uuid() + AZ::AzTypeInfo<_T2>::Uuid() + AZ::AzTypeInfo<_T3>::Uuid() +                         \
                    AZ::AzTypeInfo<_T4>::Uuid() + AZ::AzTypeInfo<_T5>::Uuid() + AZ::TypeId(_AddUuid));                              \
            return s_uuid;                                                                                                        \
        }                                                                                                                         \
        static bool Specialized() { return true; }                                                                                \
    }

#define AZ_INTERNAL_VARIATION_SPECIALIZATION_TEMPLATE(_Specialization, _ClassName, _ClassUuid)\
    template<typename... Args> \
    struct AzTypeInfo<_Specialization<Args...>, false> {\
        static const char* Name() { \
            static char typeName[128] = { 0 }; \
            if (typeName[0] == 0) { \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), #_ClassName "<"); \
                AZ::Internal::AggregateTypes<Args...>::TypeName(typeName, AZ_ARRAY_SIZE(typeName)); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ">"); \
            }\
            return typeName; \
        } \
        static const AZ::TypeId& Uuid() { \
            static AZ::Internal::TypeIdHolder s_uuid(AZ::TypeId(_ClassUuid) + AZ::Internal::AggregateTypes<Args...>::Uuid()); \
            return s_uuid;\
        } \
        static bool Specialized() { return true; } \
    }


#define AZ_INTERNAL_FUNCTION_VARIATION_SPECIALIZATION(_Specialization, _ClassName, _ClassUuid)\
    template<typename R, typename... Args> \
    struct AzTypeInfo<_Specialization<R(Args...)>, false> {\
        static const char* Name() { \
            static char typeName[128] = { 0 }; \
            if (typeName[0] == 0) { \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), #_ClassName "<"); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<R>::Name()); \
                AZ::Internal::AggregateTypes<Args...>::TypeName(typeName, AZ_ARRAY_SIZE(typeName)); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ">"); \
            }\
            return typeName; \
        } \
        static const AZ::Uuid& Uuid() { \
            static AZ::Internal::TypeIdHolder s_uuid(AZ::Uuid(_ClassUuid) + AZ::AzTypeInfo<R>::Uuid() + AZ::Internal::AggregateTypes<Args...>::Uuid()); \
            return s_uuid;\
        } \
        static bool Specialized() { return true; } \
    }

    /* This version of AZ_INTERNAL_VARIATION_SPECIALIZATION_2 only uses the first argument for UUID generation purposes */
#define AZ_INTERNAL_VARIATION_SPECIALIZATION_2_CONCAT_1(_T1, _T2, _Specialization, _NamePrefix, _NameSuffix, _AddUuid)        \
    template<class _T1, class _T2>                                                                                   \
    struct AzTypeInfo<_Specialization<_T1, _T2>, false> {                                                            \
        static const char* Name() {                                                                                  \
            static char typeName[128] = { 0 };                                                                       \
            if (typeName[0] == 0) {                                                                                  \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NamePrefix);                     \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<_T1>::Name());     \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), _NameSuffix);                     \
            }                                                                                                        \
            return typeName;                                                                                         \
        }                                                                                                            \
        static const AZ::TypeId& Uuid() {                                                                              \
            static AZ::Internal::TypeIdHolder s_uuid(AZ::AzTypeInfo<_T1>::Uuid() + AZ::TypeId(_AddUuid)); \
            return s_uuid;                                                                                           \
        }                                                                                                            \
        static bool Specialized() { return true; }                                                                   \
    }

#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC < 1900
    /* MSVC pre 2015 didn't support %z */
#define AZ_INTERNAL_TYPEINFO_NUM_TO_STR(nStr, N) azsnprintf((nStr), AZ_ARRAY_SIZE((nStr)), "%Iu", (N))
#else
#define AZ_INTERNAL_TYPEINFO_NUM_TO_STR(nStr, N) azsnprintf((nStr), AZ_ARRAY_SIZE((nStr)), "%zu", (N))
#endif

#define AZ_INTERNAL_FIXED_SPECIALIZATION_1(_TT, _AddUuid) \
    template<AZStd::size_t N> \
    struct AzTypeInfo<_TT<N>, false> { \
        static const char* NStr(AZStd::size_t Num) \
        { \
            static char nStr[64] = { 0 }; \
            if (nStr[0] == 0) \
            { \
                AZ_INTERNAL_TYPEINFO_NUM_TO_STR(nStr, Num); \
            } \
            return nStr; \
        } \
       \
        static const char* Name() { \
            static char typeName[128] = { 0 }; \
            if (typeName[0] == 0) { \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), #_TT); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "<"); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), NStr(N)); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ">"); \
            } \
            return typeName; \
        } \
        static const AZ::TypeId& Uuid() { \
            static AZ::Internal::TypeIdHolder s_uuid(AZ::TypeId::CreateName(NStr(N)) + AZ::TypeId(_AddUuid)); \
            return s_uuid; \
        } \
        static bool Specialized() { return true; } \
    }

#define AZ_INTERNAL_FIXED_SPECIALIZATION_2(_TT, _AddUuid) \
    template<typename T, AZStd::size_t N> \
    struct AzTypeInfo<_TT<T, N>, false> { \
        static const char* NStr(AZStd::size_t Num) \
        { \
            static char nStr[64] = { 0 }; \
            if (nStr[0] == 0) \
            { \
                AZ_INTERNAL_TYPEINFO_NUM_TO_STR(nStr, Num); \
            } \
            return nStr; \
        } \
       \
        static const char* Name() { \
            static char typeName[128] = { 0 }; \
            if (typeName[0] == 0) { \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), #_TT); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), "<"); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), AZ::AzTypeInfo<T>::Name()); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ", "); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), NStr(N)); \
                AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ">"); \
            } \
            return typeName; \
        } \
        static const AZ::TypeId& Uuid() { \
            static AZ::Internal::TypeIdHolder s_uuid(AZ::AzTypeInfo<T>::Uuid() + AZ::TypeId::CreateName(NStr(N)) + AZ::TypeId(_AddUuid)); \
            return s_uuid; \
        } \
        static bool Specialized() { return true; } \
    }

    // std
    AZ_INTERNAL_VARIATION_SPECIALIZATION_1(T, AZStd::less, "AZStd::less<", ">", "{41B40AFC-68FD-4ED9-9EC7-BA9992802E1B}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_1(T, AZStd::less_equal, "AZStd::less_equal<", ">", "{91CC0BDC-FC46-4617-A405-D914EF1C1902}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_1(T, AZStd::greater, "AZStd::greater<", ">", "{907F012A-7A4F-4B57-AC23-48DC08D0782E}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_1(T, AZStd::greater_equal, "AZStd::greater_equal<", ">", "{EB00488F-E20F-471A-B862-F1E3C39DDA1D}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_1(T, AZStd::equal_to, "AZStd::equal_to<", ">", "{4377BCED-F78C-4016-80BB-6AFACE6E5137}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_1(T, AZStd::hash, "AZStd::hash<", ">", "{EFA74E54-BDFA-47BE-91A7-5A05DA0306D7}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_2(T1, T2, AZStd::pair, "AZStd::pair<", ">", "{919645C1-E464-482B-A69B-04AA688B6847}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_2(T, Allocator, AZStd::vector, "AZStd::vector<", ">", "{A60E3E61-1FF6-4982-B6B8-9E4350C4C679}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_2(T, Allocator, AZStd::list, "AZStd::list<", ">", "{E1E05843-BB02-4F43-B7DC-3ADB28DF42AC}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_2(T, Allocator, AZStd::forward_list, "AZStd::forward_list<", ">", "{D7E91EA3-326F-4019-87F0-6F45924B909A}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_3(T, Compare, Allocator, AZStd::set, "AZStd::set<", ">", "{6C51837F-B0C9-40A3-8D52-2143341EDB07}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_4(T, Hasher, Compare, Allocator, AZStd::unordered_set, "AZStd::unordered_set<", ">", "{8D60408E-DA65-4670-99A2-8ABB574625AE}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_4(T, Hasher, Compare, Allocator, AZStd::unordered_multiset, "AZStd::unordered_multiset<", ">", "{B5950921-7F70-4806-9C13-8C7DF841BB90}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_4(K, M, Compare, Allocator, AZStd::map, "AZStd::map<", ">", "{F8ECF58D-D33E-49DC-BF34-8FA499AC3AE1}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_5(K, M, Hasher, Compare, Allocator, AZStd::unordered_map, "AZStd::unordered_map<", ">", "{41171F6F-9E5E-4227-8420-289F1DD5D005}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_5(K, M, Hasher, Compare, Allocator, AZStd::unordered_multimap, "AZStd::unordered_multimap<", ">", "{9ED846FA-31C1-4133-B4F4-91DF9750BA96}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_1(T, AZStd::shared_ptr, "AZStd::shared_ptr<", ">", "{FE61C84E-149D-43FD-88BA-1C3DB7E548B4}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_1(T, AZStd::intrusive_ptr, "AZStd::intrusive_ptr<", ">", "{530F8502-309E-4EE1-9AEF-5C0456B1F502}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_3(Element, Traits, Allocator, AZStd::basic_string, "AZStd::basic_string<", ">", "{C26397ED-8F60-4DF6-8320-0D0C592DA3CD}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_1(Element, AZStd::char_traits, "AZStd::char_traits<", ">", "{9B018C0C-022E-4BA4-AE91-2C1E8592DBB2}");
    AZ_INTERNAL_VARIATION_SPECIALIZATION_2(Element, Traits, AZStd::basic_string_view, "AZStd::basic_string_view<", ">", "{D348D661-6BDE-4C0A-9540-FCEA4522D497}");

    AZ_INTERNAL_FIXED_SPECIALIZATION_2(AZStd::fixed_vector, "{74044B6F-E922-4FD7-915D-EFC5D1DC59AE}");
    AZ_INTERNAL_FIXED_SPECIALIZATION_2(AZStd::array, "{911B2EA8-CCB1-4F0C-A535-540AD00173AE}");
    AZ_INTERNAL_FIXED_SPECIALIZATION_1(AZStd::bitset, "{6BAE9836-EC49-466A-85F2-F4B1B70839FB}");
    AZ_INTERNAL_FUNCTION_VARIATION_SPECIALIZATION(AZStd::function, "AZStd::function", "{C9F9C644-CCC3-4F77-A792-F5B5DBCA746E}");
}

#define AZ_TYPE_INFO_1(_ClassName) AZ_STATIC_ASSERT(false, "You must provide a ClassName,ClassUUID")

#define AZ_TYPE_INFO_2(_ClassName, _ClassUuid)                 \
    void TYPEINFO_Enable();                                    \
    static const char* TYPEINFO_Name() { return #_ClassName; } \
    static const AZ::TypeId& TYPEINFO_Uuid() { static AZ::TypeId s_uuid(_ClassUuid); return s_uuid; }

// Template class type info
#define AZ_TYPE_INFO_TEMPLATE(_ClassName, _ClassUuid, ...)\
    void TYPEINFO_Enable();\
    static const char* TYPEINFO_Name() {\
         static char typeName[128] = { 0 };\
         if (typeName[0]==0) {\
            AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), #_ClassName "<");\
            AZ::Internal::AggregateTypes<__VA_ARGS__>::TypeName(typeName, AZ_ARRAY_SIZE(typeName));\
            AZ::Internal::AzTypeInfoSafeCat(typeName, AZ_ARRAY_SIZE(typeName), ">");\
                           }\
         return typeName; } \
    static const AZ::TypeId& TYPEINFO_Uuid() {\
        static AZ::Internal::TypeIdHolder s_uuid(AZ::TypeId(_ClassUuid) + AZ::Internal::AggregateTypes<__VA_ARGS__>::Uuid());\
        return s_uuid;\
    }

// all template versions are handled by a variadic template handler
#define AZ_TYPE_INFO_3 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_4 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_5 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_6 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_7 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_8 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_9 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_10 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_11 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_12 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_13 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_14 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_15 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_16 AZ_TYPE_INFO_TEMPLATE
#define AZ_TYPE_INFO_17 AZ_TYPE_INFO_TEMPLATE

/**
* Use this macro inside your class to allow it to be identified across modules and serialized (in different contexts)
* You are required to pass at least type info for current.
* AZ_TYPE_INFO(_ClassName,_ClassUuid, (optional template parameters)
* Templates and type ID are special case. Generally avoid that and typeID specializations, when you have to use though you can.
* We require all template types to have AzTypeInfo and they can't be non-type arguments (we can add it if needed)
*/
#define AZ_TYPE_INFO(...) AZ_MACRO_SPECIALIZE(AZ_TYPE_INFO_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))



#endif // AZCORE_TYPEINFO_H
#pragma once