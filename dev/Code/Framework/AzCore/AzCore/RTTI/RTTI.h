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
#ifndef AZCORE_RTTI_H
#define AZCORE_RTTI_H

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Module/Environment.h>

namespace AZStd
{
    template<class T>
    class shared_ptr;
    template<class T>
    class intrusive_ptr;
}

namespace AZ
{
    /**
     * AZ RTTI is a lightweight user defined RTTI system that
     * allows dynamic_cast like functionality on your object.
     * As C++ rtti, azrtti is implemented using the virtual table
     * so it will work only on polymorphic types.
     * Unlike C++ rtti, it will be assigned to the classes manually,
     * so you keep your memory footprint in check. In addition
     * azdynamic_cast is generally faster, and offers a few extensions
     * that are used by the serialization, script, etc. systems.
     * Limitations: As we add RTTI manually, we can NOT cast from
     * a void pointer. Because of this we can't catch unrelated pointer
     * cast at compile time.
     */
    #define azdynamic_cast  AZ::RttiCast
    #define azrtti_cast     AZ::RttiCast
    #define azrtti_istypeof AZ::RttiIsTypeOf
    /// For instances azrtti_typeid(instance) or azrtti_typeid<Type>() for types
    #define azrtti_typeid   AZ::RttiTypeId

    /// RTTI typeId
    typedef void (* RTTI_EnumCallback)(const AZ::TypeId& /*typeId*/, void* /*userData*/);

    // Disabling missing override warning because we intentionally want to allow for declaring RTTI base classes that don't impelment RTTI.
#if defined(AZ_COMPILER_CLANG)
#   define AZ_PUSH_DISABLE_OVERRIDE_WARNING \
    _Pragma("clang diagnostic push")        \
    _Pragma("clang diagnostic ignored \"-Winconsistent-missing-override\"")
#   define AZ_POP_DISABLE_OVERRIDE_WARNING \
    _Pragma("clang diagnostic pop")
#else
#   define AZ_PUSH_DISABLE_OVERRIDE_WARNING
#   define AZ_POP_DISABLE_OVERRIDE_WARNING
#endif

    // We require AZ_TYPE_INFO to be declared
    #define AZ_RTTI_COMMON()                                                                                                       \
    AZ_PUSH_DISABLE_OVERRIDE_WARNING                                                                                               \
    void RTTI_Enable();                                                                                                            \
    virtual inline const AZ::TypeId& RTTI_GetType() const { return RTTI_Type(); }                                                  \
    virtual inline const char*      RTTI_GetTypeName() const { return RTTI_TypeName(); }                                           \
    virtual inline bool             RTTI_IsTypeOf(const AZ::TypeId & typeId) const { return RTTI_IsContainType(typeId); }          \
    virtual void                    RTTI_EnumTypes(AZ::RTTI_EnumCallback cb, void* userData) { RTTI_EnumHierarchy(cb, userData); } \
    static inline const AZ::TypeId& RTTI_Type() { return TYPEINFO_Uuid(); }                                                        \
    static inline const char*       RTTI_TypeName() { return TYPEINFO_Name(); }                                                    \
    AZ_POP_DISABLE_OVERRIDE_WARNING

    //#define AZ_RTTI_1(_1)           AZ_STATIC_ASSERT(false,"You must provide a valid classUuid!")

    /// AZ_RTTI()
    #define AZ_RTTI_1()             AZ_RTTI_COMMON()                                                                        \
    static bool                 RTTI_IsContainType(const AZ::TypeId& id) { return id == RTTI_Type(); }                      \
    static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) { cb(RTTI_Type(), userData); } \
    virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const { return (id == RTTI_Type()) ? this : nullptr; } \
    virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) { return (id == RTTI_Type()) ? this : nullptr; }

    /// AZ_RTTI(BaseClass)
    #define AZ_RTTI_2(_1)           AZ_RTTI_COMMON()                                           \
    static bool                 RTTI_IsContainType(const AZ::TypeId& id) {                     \
        if (id == RTTI_Type()) { return true; }                                                \
        return AZ::Internal::RttiCaller<_1>::RTTI_IsContainType(id); }                         \
    static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) { \
        cb(RTTI_Type(), userData);                                                             \
        AZ::Internal::RttiCaller<_1>::RTTI_EnumHierarchy(cb, userData); }                      \
    AZ_PUSH_DISABLE_OVERRIDE_WARNING                                                           \
    virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const {                   \
        if (id == RTTI_Type()) { return this; }                                                \
        return AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); }                       \
    virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) {                         \
        if (id == RTTI_Type()) { return this; }                                                \
        return AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); }                       \
    AZ_POP_DISABLE_OVERRIDE_WARNING

    /// AZ_RTTI(BaseClass1,BaseClass2)
    #define AZ_RTTI_3(_1, _2)        AZ_RTTI_COMMON()                                                \
    static bool                 RTTI_IsContainType(const AZ::TypeId& id) {                           \
        if (id == RTTI_Type()) { return true; }                                                      \
        if (AZ::Internal::RttiCaller<_1>::RTTI_IsContainType(id)) { return true; }                   \
        return AZ::Internal::RttiCaller<_2>::RTTI_IsContainType(id); }                               \
    static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) {       \
        cb(RTTI_Type(), userData);                                                                   \
        AZ::Internal::RttiCaller<_1>::RTTI_EnumHierarchy(cb, userData);                              \
        AZ::Internal::RttiCaller<_2>::RTTI_EnumHierarchy(cb, userData); }                            \
    AZ_PUSH_DISABLE_OVERRIDE_WARNING                                                                 \
    virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const {                         \
        if (id == RTTI_Type()) { return this; }                                                      \
        const void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; } \
        return AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); }                             \
    virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) {                               \
        if (id == RTTI_Type()) { return this; }                                                      \
        void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; }       \
        return AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); }                             \
    AZ_POP_DISABLE_OVERRIDE_WARNING

    /// AZ_RTTI(BaseClass1,BaseClass2,BaseClass3)
    #define AZ_RTTI_4(_1, _2, _3)    AZ_RTTI_COMMON()                                                \
    static bool                 RTTI_IsContainType(const AZ::TypeId& id) {                           \
        if (id == RTTI_Type()) { return true; }                                                      \
        if (AZ::Internal::RttiCaller<_1>::RTTI_IsContainType(id)) { return true; }                   \
        if (AZ::Internal::RttiCaller<_2>::RTTI_IsContainType(id)) { return true; }                   \
        return AZ::Internal::RttiCaller<_3>::RTTI_IsContainType(id); }                               \
    static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) {       \
        cb(RTTI_Type(), userData);                                                                   \
        AZ::Internal::RttiCaller<_1>::RTTI_EnumHierarchy(cb, userData);                              \
        AZ::Internal::RttiCaller<_2>::RTTI_EnumHierarchy(cb, userData);                              \
        AZ::Internal::RttiCaller<_3>::RTTI_EnumHierarchy(cb, userData); }                            \
    AZ_PUSH_DISABLE_OVERRIDE_WARNING                                                                 \
    virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const {                         \
        if (id == RTTI_Type()) { return this; }                                                      \
        const void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; } \
        r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
        return AZ::Internal::RttiCaller<_3>::RTTI_AddressOf(this, id); }                             \
    virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) {                               \
        if (id == RTTI_Type()) { return this; }                                                      \
        void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; }       \
        r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
        return AZ::Internal::RttiCaller<_3>::RTTI_AddressOf(this, id); }                             \
    AZ_POP_DISABLE_OVERRIDE_WARNING

    /// AZ_RTTI(BaseClass1,BaseClass2,BaseClass3,BaseClass4)
    #define AZ_RTTI_5(_1, _2, _3, _4)  AZ_RTTI_COMMON()                                              \
    static bool                 RTTI_IsContainType(const AZ::TypeId& id) {                           \
        if (id == RTTI_Type()) { return true; }                                                      \
        if (AZ::Internal::RttiCaller<_1>::RTTI_IsContainType(id)) { return true; }                   \
        if (AZ::Internal::RttiCaller<_2>::RTTI_IsContainType(id)) { return true; }                   \
        if (AZ::Internal::RttiCaller<_3>::RTTI_IsContainType(id)) { return true; }                   \
        return AZ::Internal::RttiCaller<_4>::RTTI_IsContainType(id); }                               \
    static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) {       \
        cb(RTTI_Type(), userData);                                                                   \
        AZ::Internal::RttiCaller<_1>::RTTI_EnumHierarchy(cb, userData);                              \
        AZ::Internal::RttiCaller<_2>::RTTI_EnumHierarchy(cb, userData);                              \
        AZ::Internal::RttiCaller<_3>::RTTI_EnumHierarchy(cb, userData);                              \
        AZ::Internal::RttiCaller<_4>::RTTI_EnumHierarchy(cb, userData); }                            \
    AZ_PUSH_DISABLE_OVERRIDE_WARNING                                                                 \
    virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const {                         \
        if (id == RTTI_Type()) { return this; }                                                      \
        const void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; } \
        r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
        r = AZ::Internal::RttiCaller< _3>::RTTI_AddressOf(this, id); if (r) { return r; }            \
        return AZ::Internal::RttiCaller<_4>::RTTI_AddressOf(this, id); }                             \
    virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) {                               \
        if (id == RTTI_Type()) { return this; }                                                      \
        void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; }       \
        r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
        r = AZ::Internal::RttiCaller<_3>::RTTI_AddressOf(this, id); if (r) { return r; }             \
        return AZ::Internal::RttiCaller<_4>::RTTI_AddressOf(this, id); }                             \
    AZ_POP_DISABLE_OVERRIDE_WARNING

    /// AZ_RTTI(BaseClass1,BaseClass2,BaseClass3,BaseClass4,BaseClass5)
    #define AZ_RTTI_6(_1, _2, _3, _4, _5)  AZ_RTTI_COMMON()                                          \
    static bool                 RTTI_IsContainType(const AZ::TypeId& id) {                           \
        if (id == RTTI_Type()) { return true; }                                                      \
        if (AZ::Internal::RttiCaller<_1>::RTTI_IsContainType(id)) { return true; }                   \
        if (AZ::Internal::RttiCaller<_2>::RTTI_IsContainType(id)) { return true; }                   \
        if (AZ::Internal::RttiCaller<_3>::RTTI_IsContainType(id)) { return true; }                   \
        if (AZ::Internal::RttiCaller<_4>::RTTI_IsContainType(id)) { return true; }                   \
        return AZ::Internal::RttiCaller<_5>::RTTI_IsContainType(id); }                               \
    static void                 RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData) {       \
        cb(RTTI_Type(), userData);                                                                   \
        AZ::Internal::RttiCaller<_1>::RTTI_EnumHierarchy(cb, userData);                              \
        AZ::Internal::RttiCaller<_2>::RTTI_EnumHierarchy(cb, userData);                              \
        AZ::Internal::RttiCaller<_3>::RTTI_EnumHierarchy(cb, userData);                              \
        AZ::Internal::RttiCaller<_4>::RTTI_EnumHierarchy(cb, userData);                              \
        AZ::Internal::RttiCaller<_5>::RTTI_EnumHierarchy(cb, userData); }                            \
    AZ_PUSH_DISABLE_OVERRIDE_WARNING                                                                 \
    virtual inline const void*  RTTI_AddressOf(const AZ::TypeId& id) const {                         \
        if (id == RTTI_Type()) { return this; }                                                      \
        const void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; } \
        r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
        r = AZ::Internal::RttiCaller<_3>::RTTI_AddressOf(this, id); if (r) { return r; }             \
        r = AZ::Internal::RttiCaller<_4>::RTTI_AddressOf(this, id); if (r) { return r; }             \
        return AZ::Internal::RttiCaller<_5>::RTTI_AddressOf(this, id); }                             \
    virtual inline void*        RTTI_AddressOf(const AZ::TypeId& id) {                               \
        if (id == RTTI_Type()) { return this; }                                                      \
        void* r = AZ::Internal::RttiCaller<_1>::RTTI_AddressOf(this, id); if (r) { return r; }       \
        r = AZ::Internal::RttiCaller<_2>::RTTI_AddressOf(this, id); if (r) { return r; }             \
        r = AZ::Internal::RttiCaller<_3>::RTTI_AddressOf(this, id); if (r) { return r; }             \
        r = AZ::Internal::RttiCaller<_4>::RTTI_AddressOf(this, id); if (r) { return r; }             \
        return AZ::Internal::RttiCaller<_5>::RTTI_AddressOf(this, id); }                             \
    AZ_POP_DISABLE_OVERRIDE_WARNING

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // MACRO specialization to allow optional parameters for template version of AZ_RTTI
    #define AZ_INTERNAL_EXTRACT(...) AZ_INTERNAL_EXTRACT __VA_ARGS__
    #define AZ_INTERNAL_NOTHING_AZ_INTERNAL_EXTRACT
    #define AZ_INTERNAL_PASTE(_x, ...)  _x ## __VA_ARGS__
    #define AZ_INTERNAL_EVALUATING_PASTE(_x, ...) AZ_INTERNAL_PASTE(_x, __VA_ARGS__)
    #define AZ_INTERNAL_REMOVE_PARENTHESIS(_x) AZ_INTERNAL_EVALUATING_PASTE(AZ_INTERNAL_NOTHING_, AZ_INTERNAL_EXTRACT _x)

    #define AZ_INTERNAL_USE_FIRST_ELEMENT(_1, ...) _1
    #define AZ_INTERNAL_SKIP_FIRST(_1, ...)  __VA_ARGS__

    #define AZ_RTTI_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS) MACRO_NAME##NPARAMS PARAMS
    #define AZ_RTTI_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)
    #define AZ_RTTI_MACRO_SPECIALIZE(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)

    #define AZ_RTTI_I_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS) MACRO_NAME##NPARAMS PARAMS
    #define AZ_RTTI_I_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_I_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)
    #define AZ_RTTI_I_MACRO_SPECIALIZE(MACRO_NAME, NPARAMS, PARAMS) AZ_RTTI_I_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)

    #define AZ_RTTI_HELPER_1(_Name, ...) AZ_TYPE_INFO_2(_Name, AZ_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__)); AZ_RTTI_I_MACRO_SPECIALIZE(AZ_RTTI_, AZ_VA_NUM_ARGS(__VA_ARGS__), (AZ_INTERNAL_SKIP_FIRST(__VA_ARGS__)))

    #define AZ_RTTI_HELPER_3(...)  AZ_TYPE_INFO(AZ_INTERNAL_REMOVE_PARENTHESIS(AZ_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__))); AZ_RTTI_I_MACRO_SPECIALIZE(AZ_RTTI_, AZ_VA_NUM_ARGS(__VA_ARGS__), (AZ_INTERNAL_SKIP_FIRST(__VA_ARGS__)))
    #define AZ_RTTI_HELPER_2(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_4(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_5(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_6(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_7(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_8(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    #define AZ_RTTI_HELPER_9(...)  AZ_RTTI_HELPER_3(__VA_ARGS__)
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Check if a class has Rtti support - HasAZRtti<ClassType>....
    AZ_HAS_MEMBER(AZRtti, RTTI_Enable, void, ())

    /**
     * Use AZ_RTTI macro to enable RTTI for the specific class. Keep in mind AZ_RTTI uses virtual function
     * so make sure you have virtual destructor.
     * AZ_RTTI includes the functionality of AZ_TYPE_INFO, so you don't have to declare TypeInfo it if you use AZ_RTTI
     * The syntax is AZ_RTTI(ClassName,ClassUuid,(BaseClass1..N)) you can have 0 to N base classes this will allow us
     * to perform dynamic casts and query about types.
     * 
     *  \note A more complex use case is when you use templates, the you have to group the parameters for the TypeInfo call.
     * ex. AZ_RTTI( ( (ClassName<TemplateArg1, TemplateArg2, ...>), ClassUuid, TemplateArg1, TemplateArg2, ...), BaseClass1...)
     *
     * Take care with the parentheses, excruciatingly explicitly delineated here: AZ_RTTI (3 (2 (1 fully templated class name 1), Uuid, template args... 2), base classes... 3)
     */
    #define AZ_RTTI(...)  AZ_RTTI_MACRO_SPECIALIZE(AZ_RTTI_HELPER_, AZ_VA_NUM_ARGS(AZ_INTERNAL_REMOVE_PARENTHESIS(AZ_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__))), (__VA_ARGS__))

    namespace Internal
    {
        template<class T, class U>
        inline T        RttiCastHelper(U ptr, const AZStd::true_type& /* HasAZRtti<U> */)
        {
            typedef typename AZStd::remove_pointer<T>::type CastType;
            if (ptr)
            {
                return reinterpret_cast<T>(ptr->RTTI_AddressOf(AzTypeInfo<CastType>::Uuid()));
            }
            else
            {
                return nullptr;
            }
        }

        template<class T, class U>
        struct RttiIsSameCast
        {
            static inline T Cast(U)         { return nullptr; }
        };

        template<class T>
        struct RttiIsSameCast<T, T>
        {
            static inline T Cast(T ptr)     { return ptr; }
        };

        template<class T, class U>
        inline T        RttiCastHelper(U ptr, const AZStd::false_type& /* HasAZRtti<U> */)
        {
            AZ_STATIC_ASSERT(Internal::HasAZTypeInfo<U>::value, "AZ_TYPE_INFO is required to perform an azrtti_cast");
            return RttiIsSameCast<T, U>::Cast(ptr);
        }

        template<class T, bool IsConst = AZStd::is_const<T>::value >
        struct AddressTypeHelper
        {
            typedef const void* type;
        };

        template<class T>
        struct AddressTypeHelper<T, false>
        {
            typedef void* type;
        };

        template<class U>
        inline typename AddressTypeHelper<U>::type RttiAddressOfHelper(U ptr, const AZ::TypeId& id, const AZStd::true_type& /* HasAZRtti<U> */)
        {
            if (ptr)
            {
                return ptr->RTTI_AddressOf(id);
            }
            else
            {
                return nullptr;
            }
        }

        template<class U>
        inline typename AddressTypeHelper<U>::type  RttiAddressOfHelper(U ptr, const AZ::TypeId& id, const AZStd::false_type& /* HasAZRtti<U> */)
        {
            typedef typename AZStd::remove_pointer<U>::type CastType;
            if (id == AzTypeInfo<CastType>::Uuid())
            {
                return ptr;
            }
            return nullptr;
        }

        template<class T>
        struct RttiRemoveQualifiers
        {
            typedef typename AZStd::remove_cv<typename AZStd::remove_reference<typename AZStd::remove_pointer<T>::type>::type>::type type;
        };

        template<class T, class U>
        struct RttiIsTypeOfHelper
        {
            static inline bool  Check(U& ref, const AZStd::true_type& /* HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                return ref.RTTI_IsTypeOf(AzTypeInfo<CheckType>::Uuid());
            }

            static inline bool  Check(U&, const AZStd::false_type& /* HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                if (AzTypeInfo<CheckType>::Uuid() == AzTypeInfo<SrcType>::Uuid())
                {
                    return true;
                }
                return false;
            }
        };
        template<class T, class U>
        struct RttiIsTypeOfHelper<T, U*>
        {
            static inline bool  Check(U* ptr, const AZStd::true_type& /* HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                if (ptr)
                {
                    return ptr->RTTI_IsTypeOf(AzTypeInfo<CheckType>::Uuid());
                }
                else
                {
                    return false;
                }
            }

            static inline bool  Check(U*, const AZStd::false_type& /* HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                if (AzTypeInfo<CheckType>::Uuid() == AzTypeInfo<SrcType>::Uuid())
                {
                    return true;
                }
                return false;
            }
        };

        template<class U>
        struct RttiIsTypeOfIdHelper
        {
            static inline bool  Check(const AZ::TypeId& id, const U& ref, const AZStd::true_type& /* HasAZRtti<U> */)
            {
                return ref.RTTI_IsTypeOf(id);
            }

            static inline bool  Check(const AZ::TypeId& id, const U&, const AZStd::false_type& /* HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                if (id == AzTypeInfo<SrcType>::Uuid())
                {
                    return true;
                }
                return false;
            }

            static inline const AZ::TypeId& Type(const U& ref, const AZStd::true_type& /* HasAZRtti<U> */)
            {
                return ref.RTTI_GetType();
            }

            static inline const AZ::TypeId& Type(const U&, const AZStd::false_type& /* HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return AzTypeInfo<SrcType>::Uuid();
            }
        };

        template<class U>
        struct RttiIsTypeOfIdHelper<U*>
        {
            static inline bool Check(const AZ::TypeId& id, U* ptr, const AZStd::true_type& /* HasAZRtti<U> */)
            {
                if (ptr)
                {
                    return ptr->RTTI_IsTypeOf(id);
                }
                else
                {
                    return false;
                }
            }

            static inline bool  Check(const AZ::TypeId& id, U*, const AZStd::false_type& /* HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                if (id == AzTypeInfo<SrcType>::Uuid())
                {
                    return true;
                }
                return false;
            }

            static inline const AZ::TypeId& Type(U* ptr, const AZStd::true_type& /* HasAZRtti<U> */)
            {
                if (ptr)
                {
                    return ptr->RTTI_GetType();
                }
                else
                {
                    static AZ::TypeId s_invalidUuid = AZ::TypeId::CreateNull();
                    return s_invalidUuid;
                }
            }

            static inline const AZ::TypeId& Type(U*, const AZStd::false_type& /* HasAZRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return AzTypeInfo<SrcType>::Uuid();
            }
        };

        template<class T>
        void RttiEnumHierarchyHelper(RTTI_EnumCallback cb, void* userData, const AZStd::true_type& /* HasAZRtti<T> */)
        {
            T::RTTI_EnumHierarchy(cb, userData);
        }

        template<class T>
        void RttiEnumHierarchyHelper(RTTI_EnumCallback cb, void* userData, const AZStd::false_type& /* HasAZRtti<T> */)
        {
            cb(AzTypeInfo<T>::Uuid(), userData);
        }

        // Helper to prune types that are base class, but don't support RTTI
        template<class T, bool isRtti = HasAZRtti<T>::value >
        struct RttiCaller
        {
            AZ_FORCE_INLINE static bool RTTI_IsContainType(const AZ::TypeId& id)
            {
                return T::RTTI_IsContainType(id);
            }

            AZ_FORCE_INLINE static void RTTI_EnumHierarchy(AZ::RTTI_EnumCallback cb, void* userData)
            {
                T::RTTI_EnumHierarchy(cb, userData);
            }

            AZ_FORCE_INLINE static const void* RTTI_AddressOf(const T* obj, const AZ::TypeId& id)
            {
                return obj->T::RTTI_AddressOf(id);
            }

            AZ_FORCE_INLINE static void* RTTI_AddressOf(T* obj, const AZ::TypeId& id)
            {
                return obj->T::RTTI_AddressOf(id);
            }
        };

        // Specialization for classes that don't have RTTI implemented
        template<class T>
        struct RttiCaller<T, false>
        {
            AZ_FORCE_INLINE static bool RTTI_IsContainType(const AZ::TypeId&)
            {
                return false;
            }

            AZ_FORCE_INLINE static void RTTI_EnumHierarchy(AZ::RTTI_EnumCallback, void*)
            {
            }

            AZ_FORCE_INLINE static const void* RTTI_AddressOf(const T*, const AZ::TypeId&)
            {
                return nullptr;
            }

            AZ_FORCE_INLINE static void* RTTI_AddressOf(T*, const AZ::TypeId&)
            {
                return nullptr;
            }
        };
    }

    /// Enumerate class hierarchy (static) based on input type. Safe to call for type not supporting AZRtti returns the AzTypeInfo<T>::Uuid only.
    template<class T>
    inline void     RttiEnumHierarchy(RTTI_EnumCallback cb, void* userData)
    {
        Internal::RttiEnumHierarchyHelper<T>(cb, userData, typename HasAZRtti<typename AZStd::remove_pointer<T>::type>::type());
    }

    /// Performs a RttiCast when possible otherwise return NULL. Safe to call for type not supporting AZRtti (returns NULL unless type fully match).
    template<class T, class U>
    inline T        RttiCast(U ptr)
    {
        // We do support only pointer types, because we don't use exceptions. So
        // if we have a reference and we can't convert we can't really check if the returned
        // reference is correct.
        AZ_STATIC_ASSERT(AZStd::is_pointer<T>::value, "azrtti_cast supports only pointer types");
        return Internal::RttiCastHelper<T>(ptr, typename HasAZRtti<typename AZStd::remove_pointer<U>::type>::type());
    }

    /// Specialization for nullptr_t, it's convertible to anything
    template <class T>
    inline T        RttiCast(AZStd::nullptr_t)
    {
        AZ_STATIC_ASSERT(AZStd::is_pointer<T>::value, "azrtti_cast supports only pointer types");
        return nullptr;
    }

    /// RttiCast specialization for shared_ptr.
    template<class T, class U>
    inline AZStd::shared_ptr<typename AZStd::remove_pointer<T>::type>       RttiCast(const AZStd::shared_ptr<U>& ptr)
    {
        using DestType = typename AZStd::remove_pointer<T>::type;
        // We do support only pointer types, because we don't use exceptions. So
        // if we have a reference and we can't convert we can't really check if the returned
        // reference is correct.
        AZ_STATIC_ASSERT(AZStd::is_pointer<T>::value, "azrtti_cast supports only pointer types");
        T castPtr = Internal::RttiCastHelper<T>(ptr.get(), typename HasAZRtti<U>::type());
        if (castPtr)
        {
            return AZStd::shared_ptr<DestType>(ptr, castPtr);
        }
        else
        {
            return AZStd::shared_ptr<DestType>();
        }
    }

    /// RttiCast specialization for intrusive_ptr.
    template<class T, class U>
    inline AZStd::intrusive_ptr<typename AZStd::remove_pointer<T>::type>    RttiCast(const AZStd::intrusive_ptr<U>& ptr)
    {
        // We do support only pointer types, because we don't use exceptions. So
        // if we have a reference and we can't convert we can't really check if the returned
        // reference is correct.
        AZ_STATIC_ASSERT(AZStd::is_pointer<T>::value, "rtti_cast supports only pointer types");
        return Internal::RttiCastHelper<T>(ptr.get(), typename HasAZRtti<U>::type());
    }

    /// Gets address of a contained type or NULL. Safe to call for type not supporting AZRtti (returns 0 unless type fully match).
    template<class T>
    inline typename Internal::AddressTypeHelper<T>::type RttiAddressOf(T ptr, const AZ::TypeId& id)
    {
        // we can support references (as not exception is needed), but pointer should be sufficient when it comes to addresses!
        AZ_STATIC_ASSERT(AZStd::is_pointer<T>::value, "RttiAddressOf supports only pointer types");
        return Internal::RttiAddressOfHelper(ptr, id, typename HasAZRtti<typename AZStd::remove_pointer<T>::type>::type());
    }

    /// Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<class T, class U>
    inline bool     RttiIsTypeOf(U data)
    {
        return Internal::RttiIsTypeOfHelper<T, U>::Check(data, typename HasAZRtti<typename AZStd::remove_pointer<U>::type>::type());
    }

    /// Returns true if the type (by id) is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<class U>
    inline bool     RttiIsTypeOf(const AZ::TypeId& id, U data)
    {
        return Internal::RttiIsTypeOfIdHelper<U>::Check(id, data, typename HasAZRtti<typename AZStd::remove_pointer<U>::type>::type());
    }

    template<class U>
    inline const AZ::TypeId& RttiTypeId()
    {
        return AzTypeInfo<U>::Uuid();
    }

    template<class U>
    inline const AZ::TypeId& RttiTypeId(const U& data)
    {
        return Internal::RttiIsTypeOfIdHelper<U>::Type(data, typename HasAZRtti<typename AZStd::remove_pointer<U>::type>::type());
    }

    /**
     * Interface for resolving RTTI information when no compile-time type information is available.
     * The serializer retrieves an instance of the helper during type registration (when type information
     * is still available) and uses it to get RTTI information during serialization.
     */
    class IRttiHelper
    {
    public:
        virtual ~IRttiHelper() = default;
        virtual const AZ::TypeId& GetActualUuid(const void* p) const = 0;
        virtual const char*       GetActualTypeName(const void* p) const = 0;
        virtual const void*       Cast(const void* p, const AZ::TypeId& asType) const = 0;
        virtual void*             Cast(void* p, const AZ::TypeId& asType) const = 0;
        virtual const AZ::TypeId& GetTypeId() const = 0;
        virtual bool              IsTypeOf(const AZ::TypeId& id) const = 0;
        virtual void              EnumHierarchy(RTTI_EnumCallback callback, void* userData = nullptr) const = 0;

        // Template helpers
        template <typename TargetType>
        AZ_FORCE_INLINE const TargetType* Cast(const void* p)
        {
            return reinterpret_cast<const TargetType*>(Cast(p, azrtti_typeid<TargetType>()));
        }

        template <typename TargetType>
        AZ_FORCE_INLINE TargetType* Cast(void* p)
        {
            return reinterpret_cast<TargetType*>(Cast(p, azrtti_typeid<TargetType>()));
        }

        template <typename QueryType>
        AZ_FORCE_INLINE bool IsTypeOf()
        {
            return IsTypeOf(azrtti_typeid<QueryType>());
        }
    };

    namespace Internal
    {
        /*
         * Helper class to retrieve AZ::TypeIds and perform AZRtti queries.
         * This helper uses AZRtti when available, and does what it can when not.
         * It automatically resolves pointer-to-pointers to their value types
         * and supports queries without type information through the IRttiHelper
         * interface.
         * Call GetRttiHelper<T>() to retrieve an IRttiHelper interface
         * for AZRtti-enabled types while type info is still available, so it
         * can be used for queries after type info is lost.
         */
        template<typename T>
        struct RttiHelper
            : public IRttiHelper
        {
        public:
            using ValueType = T;
            AZ_STATIC_ASSERT(HasAZRtti<ValueType>::value, "Type parameter for RttiHelper must support AZ RTTI.");

            //////////////////////////////////////////////////////////////////////////
            // IRttiHelper
            const AZ::TypeId& GetActualUuid(const void* p) const override
            {
                return p
                    ? reinterpret_cast<const T*>(p)->RTTI_GetType()
                    : T::RTTI_Type();
            }
            const char* GetActualTypeName(const void* p) const override
            {
                return p
                    ? reinterpret_cast<const T*>(p)->RTTI_GetTypeName()
                    : T::RTTI_TypeName();
            }
            const void* Cast(const void* p, const AZ::TypeId& asType) const override
            {
                return p
                    ? reinterpret_cast<const T*>(p)->RTTI_AddressOf(asType)
                    : nullptr;
            }
            void* Cast(void* p, const AZ::TypeId& asType) const override
            {
                return p
                    ? reinterpret_cast<T*>(p)->RTTI_AddressOf(asType)
                    : nullptr;
            }
            const AZ::TypeId& GetTypeId() const override
            {
                return T::RTTI_Type();
            }
            bool IsTypeOf(const AZ::TypeId& id) const override
            {
                return T::RTTI_IsContainType(id);
            }
            void EnumHierarchy(RTTI_EnumCallback callback, void* userData = nullptr) const override
            {
                return T::RTTI_EnumHierarchy(callback, userData);
            }
            //////////////////////////////////////////////////////////////////////////
        };

        // Overload for AZRtti
        // Named _Internal so that GetRttiHelper() calls from inside namespace Internal don't resolve here
        template<typename T>
        IRttiHelper* GetRttiHelper_Internal(AZStd::true_type /*hasRtti*/)
        {
            static RttiHelper<T> s_instance;
            return &s_instance;
        }
        // Overload for no typeinfo available
        template<typename>
        IRttiHelper* GetRttiHelper_Internal(AZStd::false_type /*hasRtti*/)
        {
            return nullptr;
        }
    } // namespace Internal

    template<typename T>
    IRttiHelper* GetRttiHelper()
    {
        using ValueType = std::remove_const_t<std::remove_pointer_t<T>>;
        return Internal::GetRttiHelper_Internal<ValueType>(typename HasAZRtti<ValueType>::type());
    }
} // namespace AZ

#endif // AZCORE_RTTI_H
#pragma once