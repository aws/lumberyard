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
#ifndef AZCORE_MEMORY_H
#define AZCORE_MEMORY_H 1

#include <AzCore/base.h>
#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/aligned_storage.h>

#include <AzCore/std/typetraits/has_member_function.h>

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Module/Environment.h>

/**
 * AZ Memory allocation supports all best know allocation schemes. Even though we highly recommend using the
 * class overriding of new/delete operators for which we provide \ref ClassAllocators. We don't restrict to
 * use whatever you need, each way has it's benefits and drawback. Each of those will be described as we go along.
 * In every macro that doesn't require to specify an allocator AZ::SystemAllocator is implied.
 */
#if defined(AZCORE_ENABLE_MEMORY_TRACKING)
    #define aznew                                                   aznewex((const char*)nullptr)
    #define aznewex(_Name)                                          new(__FILE__, __LINE__, _Name)

/// azmalloc(size)
    #define azmalloc_1(_1)                                          AZ::AllocatorInstance< AZ::SystemAllocator >::Get().Allocate(_1, 0, 0, "azmalloc", __FILE__, __LINE__)
/// azmalloc(size,alignment)
    #define azmalloc_2(_1, _2)                                      AZ::AllocatorInstance< AZ::SystemAllocator >::Get().Allocate(_1, _2, 0, "azmalloc", __FILE__, __LINE__)
/// azmalloc(size,alignment,Allocator)
    #define azmalloc_3(_1, _2, _3)                                  AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0, "azmalloc", __FILE__, __LINE__)
/// azmalloc(size,alignment,Allocator,allocationName)
    #define azmalloc_4(_1, _2, _3, _4)                              AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0, _4, __FILE__, __LINE__)
/// azmalloc(size,alignment,Allocator,allocationName,flags)
    #define azmalloc_5(_1, _2, _3, _4, _5)                          AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, _5, _4, __FILE__, __LINE__)

/// azcreate(class,params)
    #define azcreate_2(_1, _2)                                      new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, AZ::SystemAllocator,#_1)) _1 _2
/// azcreate(class,params,Allocator)
    #define azcreate_3(_1, _2, _3)                                  new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3,#_1)) _1 _2
/// azcreate(class,params,Allocator,allocationName)
    #define azcreate_4(_1, _2, _3, _4)                              new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4)) _1 _2
/// azcreate(class,params,Allocator,allocationName,flags)
    #define azcreate_5(_1, _2, _3, _4, _5)                          new(azmalloc_5(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4, _5)) _1 _2
#else
    #define aznew           new((const AZ::Internal::AllocatorDummy*)nullptr)
    #define aznewex(_Name)  aznew

/// azmalloc(size)
    #define azmalloc_1(_1)                                          AZ::AllocatorInstance< AZ::SystemAllocator >::Get().Allocate(_1, 0, 0)
/// azmalloc(size,alignment)
    #define azmalloc_2(_1, _2)                                      AZ::AllocatorInstance< AZ::SystemAllocator >::Get().Allocate(_1, _2, 0)
/// azmalloc(size,alignment,Allocator)
    #define azmalloc_3(_1, _2, _3)                                  AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0)
/// azmalloc(size,alignment,Allocator,allocationName)
    #define azmalloc_4(_1, _2, _3, _4)                              AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0, _4)
/// azmalloc(size,alignment,Allocator,allocationName,flags)
    #define azmalloc_5(_1, _2, _3, _4, _5)                          AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, _5, _4)

/// azcreate(class)
    #define azcreate_1(_1)                                          new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, AZ::SystemAllocator, #_1)) _1()
/// azcreate(class,params)
    #define azcreate_2(_1, _2)                                      new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, AZ::SystemAllocator, #_1)) _1 _2
/// azcreate(class,params,Allocator)
    #define azcreate_3(_1, _2, _3)                                  new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, #_1)) _1 _2
/// azcreate(class,params,Allocator,allocationName)
    #define azcreate_4(_1, _2, _3, _4)                              new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4)) _1 _2
/// azcreate(class,params,Allocator,allocationName,flags)
    #define azcreate_5(_1, _2, _3, _4, _5)                          new(azmalloc_5(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4, _5)) _1 _2
#endif

/**
 * azmalloc is equivalent to malloc(...). It should be used with corresponding azfree call.
 * macro signature: azmalloc(size_t byteSize, size_t alignment = DefaultAlignment, AllocatorType = AZ::SystemAllocator, const char* name = "Default Name", int flags = 0)
 */
#define azmalloc(...)       AZ_MACRO_SPECIALIZE(azmalloc_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/**
 * azcreate is customized aznew function call. aznew can be used anywhere where we use new, while azcreate has a function call signature.
 * azcreate allows you to override the operator new and by this you can override the allocator per object instance. It should
 * be used with corresponding azdestroy call.
 * macro signature: azcreate(ClassName, CtorParams = (), AllocatorType = AZ::SystemAllocator, AllocationName = "ClassName", int flags = 0)
 */
#define azcreate(...)       AZ_MACRO_SPECIALIZE(azcreate_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// azfree(pointer)
#define azfree_1(_1)                do { if (_1) { AZ::AllocatorInstance< AZ::SystemAllocator >::Get().DeAllocate(_1); } }   while (0)
/// azfree(pointer,allocator)
#define azfree_2(_1, _2)            do { if (_1) { AZ::AllocatorInstance< _2 >::Get().DeAllocate(_1); } }                    while (0)
/// azfree(pointer,allocator,size)
#define azfree_3(_1, _2, _3)        do { if (_1) { AZ::AllocatorInstance< _2 >::Get().DeAllocate(_1, _3); } }                while (0)
/// azfree(pointer,allocator,size,alignment)
#define azfree_4(_1, _2, _3, _4)    do { if (_1) { AZ::AllocatorInstance< _2 >::Get().DeAllocate(_1, _3, _4); } }            while (0)

/**
 * azfree is equivalent to free(...). Is should be used with corresponding azmalloc call.
 * macro signature: azfree(Pointer* ptr, AllocatorType = AZ::SystemAllocator, size_t byteSize = Unknown, size_t alignment = DefaultAlignment);
 * \note Providing allocation size (byteSize) and alignment is optional, but recommended when possible. It will generate faster code.
 */
#define azfree(...)         AZ_MACRO_SPECIALIZE(azfree_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// Returns allocation size, based on it's pointer \ref AZ::IAllocatorAllocate::AllocationSize.
#define azallocsize(_Ptr, _Allocator)    AZ::AllocatorInstance< _Allocator >::Get().AllocationSize(_Ptr)
/// Returns the new expanded size or 0 if NOT supported by the allocator \ref AZ::IAllocatorAllocate::Resize.
#define azallocresize(_Ptr, _NewSize, _Allocator) AZ::AllocatorInstance< _Allocator >::Get().Resize(_Ptr, _NewSize)

namespace AZ {
    // \note we can use AZStd::Internal::destroy<pointer_type>::single(ptr) if we template the entire function.
    namespace Memory {
        namespace Internal {
            template<class T>
            AZ_FORCE_INLINE void call_dtor(T* ptr)          { (void)ptr; ptr->~T(); }
        }
    }
}

#define azdestroy_1(_1)         do { AZ::Memory::Internal::call_dtor(_1); azfree_1(_1); } while (0)
#define azdestroy_2(_1, _2)      do { AZ::Memory::Internal::call_dtor(_1); azfree_2(_1, _2); } while (0)
#define azdestroy_3(_1, _2, _3)     do { AZ::Memory::Internal::call_dtor(reinterpret_cast<_3*>(_1)); azfree_4(_1, _2, sizeof(_3), AZStd::alignment_of< _3 >::value); } while (0)

/**
 * azdestroy should be used only with corresponding azcreate.
 * macro signature: azdestroy(Pointer*, AllocatorType = AZ::SystemAllocator, ClassName = Unknown)
 * \note Providing ClassName is optional, but recommended when possible. It will generate faster code.
 */
#define azdestroy(...)      AZ_MACRO_SPECIALIZE(azdestroy_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/**
 * Class Allocator (override new/delete operators)
 *
 * use this macro inside your objects to define it's default allocation (ex.):
 * class MyObj
 * {
 *   public:
 *          AZ_CLASS_ALLOCATOR(MyObj,SystemAllocator,0) - inline version requires to include the allocator (in this case sysallocator.h)
 *      or
 *          AZ_CLASS_ALLOCATOR_DECL in the header and AZ_CLASS_ALLOCATOR_IMPL(MyObj,SystemAllocator,0) in the cpp file. This way you don't need
 *          to include the allocator header where you decl your class (MyObj).
 *      ...
 * };
 *
 * \note We don't support array operators [] because they insert a compiler/platform
 * dependent array header, which then breaks the alignment in some cases.
 * If you want to use dynamic array use AZStd::vector or AZStd::fixed_vector.
 * Of course you can use placement new and do the array allocation anyway if
 * it's really needed.
 */
#define AZ_CLASS_ALLOCATOR(_Class, _Allocator, _Flags)                                                                                                                                                                                              \
    /* ========== placement operators (default) ========== */                                                                                                                                                                                       \
    AZ_FORCE_INLINE void* operator new(std::size_t, void* p)    { return p; }   /* placement new */                                                                                                                                                 \
    AZ_FORCE_INLINE void* operator new[](std::size_t, void* p)  { return p; }   /* placement array new */                                                                                                                                           \
    AZ_FORCE_INLINE void  operator delete(void*, void*)         { }             /* placement delete, called when placement new asserts */                                                                                                           \
    AZ_FORCE_INLINE void  operator delete[](void*, void*)       { }             /* placement array delete */                                                                                                                                        \
    /* ========== standard operator new/delete ========== */                                                                                                                                                                                        \
    AZ_FORCE_INLINE void* operator new(std::size_t size) {                      /* default operator new (called with "new _Class()") */                                                                                                             \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));                                                                           \
        AZ_Warning(0, true/*false*/, "Make sure you use aznew, offers better tracking!" /*Warning temporarily disabled until engine is using AZ allocators.*/);                                                                                     \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value, _Flags,#_Class);                                                                                                                     \
    }                                                                                                                                                                                                                                               \
    AZ_FORCE_INLINE void  operator delete(void* p, std::size_t size) {    /* default operator delete */                                                                                                                                             \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p, size, AZStd::alignment_of< _Class >::value); }                                                                                                                            \
    }                                                                                                                                                                                                                                               \
    /* ========== aznew (called "aznew _Class()") ========== */                                                                                                                                                                                     \
    AZ_FORCE_INLINE void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name) { /* with tracking */                                                                                                                 \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                                                                                 \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value, _Flags, (name == 0) ?#_Class: name, fileName, lineNum);                                                                              \
    }                                                                                                                                                                                                                                               \
    AZ_FORCE_INLINE void* operator new(std::size_t size, const AZ::Internal::AllocatorDummy*) {                 /* without tracking */                                                                                                              \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                                                                                 \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value);                                                                                                                                     \
    }                                                                                                                                                                                                                                               \
    /* ========== Symetrical delete operators (required incase aznew throws) ========== */                                                                                                                                                          \
    AZ_FORCE_INLINE void  operator delete(void* p, const char*, int, const char*) {                                                                                                                                               \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                                            \
    }                                                                                                                                                                                                                                               \
    AZ_FORCE_INLINE void  operator delete(void* p, const AZ::Internal::AllocatorDummy*) {                                                                                                                                         \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                                            \
    }                                                                                                                                                                                                                                               \
    /* ========== Unsupported operators ========== */                                                                                                                                                                                               \
    AZ_FORCE_INLINE void* operator new[](std::size_t) {                                         /* default array operator new (called with "new _Class[x]") */                                                                                      \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                           new[] inserts a header (platform dependent) to keep track of the array size!\n\
                           Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                                                                                     \
        return AZ_INVALID_POINTER;                                                                                                                                                                                                                  \
    }                                                                                                                                                                                                                                               \
    AZ_FORCE_INLINE void  operator delete[](void*) {                                            /* default array operator delete */                                                                                                                 \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                           new[] inserts a header (platform dependent) to keep track of the array size!\n\
                           Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                                                                                     \
    }                                                                                                                                                                                                                                               \
    AZ_FORCE_INLINE void* operator new[](std::size_t, const char*, int, const char*) {          /* array operator aznew with tracking (called with "aznew _Class[x]") */                                                                            \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                          new[] inserts a header (platform dependent) to keep track of the array size!\n\
                          Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                                                                                      \
        return AZ_INVALID_POINTER;                                                                                                                                                                                                                  \
    }                                                                                                                                                                                                                                               \
    AZ_FORCE_INLINE void* operator new[](std::size_t, const AZ::Internal::AllocatorDummy*) {    /* array operator aznew without tracking (called with "aznew _Class[x]") */                                                                         \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
          new[] inserts a header (platform dependent) to keep track of the array size!\n\
          Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                                                                                                      \
        return AZ_INVALID_POINTER;                                                                                                                                                                                                                  \
    }                                                                                                                                                                                                                                               \
    /* ========== AZ_CLASS_ALLOCATOR API ========== */                                                                                                                                                                                              \
    AZ_FORCE_INLINE static void* AZ_CLASS_ALLOCATOR_Allocate() {                                                                                                                                                                                    \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(sizeof(_Class), AZStd::alignment_of< _Class >::value, _Flags, #_Class);                                                                                                          \
    }                                                                                                                                                                                                                                               \
    AZ_FORCE_INLINE static void AZ_CLASS_ALLOCATOR_DeAllocate(void* object) {                                                                                                                                                                       \
        AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(object, sizeof(_Class), AZStd::alignment_of< _Class >::value);                                                                                                                        \
    }                                                                                                                                                                                                                                               \
    void AZ_CLASS_ALLOCATOR_DECLARED();

// If you want to avoid including the memory manager class in the header file use the _DECL (declaration) and _IMPL (implementations/definition) macros
#define AZ_CLASS_ALLOCATOR_DECL                                                                                                                                                                                                                     \
    /* ========== placement operators (default) ========== */                                                                                                                                                                                       \
    AZ_FORCE_INLINE void* operator new(std::size_t, void* p)    { return p; }                                                                                                                                                                       \
    AZ_FORCE_INLINE void* operator new[](std::size_t, void* p)  { return p; }                                                                                                                                                                       \
    AZ_FORCE_INLINE void operator delete(void*, void*)          { }                                                                                                                                                                                 \
    AZ_FORCE_INLINE void operator delete[](void*, void*)        { }                                                                                                                                                                                 \
    /* ========== standard operator new/delete ========== */                                                                                                                                                                                        \
    void* operator new(std::size_t size);                                                                                                                                                                                                           \
    void  operator delete(void* p, std::size_t size);                                                                                                                                                                                               \
    /* ========== aznew (called "aznew _Class()")========== */                                                                                                                                                                                      \
    void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name);                                                                                                                                                      \
    void* operator new(std::size_t size, const AZ::Internal::AllocatorDummy*);                                                                                                                                                                      \
    /* ========== Symetrical delete operators (required incase aznew throws) ========== */                                                                                                                                                          \
    void  operator delete(void* p, const char*, int, const char*);                                                                                                                                                                                  \
    void  operator delete(void* p, const AZ::Internal::AllocatorDummy*);                                                                                                                                                                            \
    /* ========== Unsupported operators ========== */                                                                                                                                                                                               \
    AZ_FORCE_INLINE void* operator new[](std::size_t) {                                                                                                                                                                                             \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                        new[] inserts a header (platform dependent) to keep track of the array size!\n\
                        Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                                                                                        \
        return AZ_INVALID_POINTER;                                                                                                                                                                                                                  \
    }                                                                                                                                                                                                                                               \
    AZ_FORCE_INLINE void  operator delete[](void*) {                                                                                                                                                                                                \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                        new[] inserts a header (platform dependent) to keep track of the array size!\n\
                        Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                                                                                        \
    }                                                                                                                                                                                                                                               \
    AZ_FORCE_INLINE void* operator new[](std::size_t, const char*, int, const char*, const AZ::Internal::AllocatorDummy*) {                                                                                                                         \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
            new[] inserts a header (platform dependent) to keep track of the array size!\n\
            Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                                                                                                    \
        return AZ_INVALID_POINTER;                                                                                                                                                                                                                  \
    }                                                                                                                                                                                                                                               \
    AZ_FORCE_INLINE void* operator new[](std::size_t, const AZ::Internal::AllocatorDummy*) {                                                                                                                                                        \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
            new[] inserts a header (platform dependent) to keep track of the array size!\n\
            Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                                                                                                    \
        return AZ_INVALID_POINTER;                                                                                                                                                                                                                  \
    }                                                                                                                                                                                                                                               \
    /* ========== AZ_CLASS_ALLOCATOR API ========== */                                                                                                                                                                                              \
    static void* AZ_CLASS_ALLOCATOR_Allocate();                                                                                                                                                                                                     \
    static void  AZ_CLASS_ALLOCATOR_DeAllocate(void* object);                                                                                                                                                                                       \
    void AZ_CLASS_ALLOCATOR_DECLARED();

#define AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _Flags, _Template)                                                                                                                                                                                         \
    /* ========== standard operator new/delete ========== */                                                                                                                                                                                        \
    _Template                                                                                                                                                                                                                                       \
    void* _Class::operator new(std::size_t size)                                                                                                                                                                                                    \
    {                                                                                                                                                                                                                                               \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_Class): %d", size, sizeof(_Class));                                                                                \
        AZ_Warning(0, false, "Make sure you use aznew, offers better tracking!");                                                                                                                                                                   \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value, _Flags,#_Class);                                                                                                                     \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void _Class::operator delete(void* p, std::size_t size)  {                                                                                                                                                                                      \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p, size, AZStd::alignment_of< _Class >::value); }                                                                                                                            \
    }                                                                                                                                                                                                                                               \
    /* ========== aznew (called "aznew _Class()")========== */                                                                                                                                                                                      \
    _Template                                                                                                                                                                                                                                       \
    void* _Class::operator new(std::size_t size, const char* fileName, int lineNum, const char* name) {                                                                                                                                             \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                                                                                 \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value, _Flags, (name == 0) ?#_Class: name, fileName, lineNum);                                                                              \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void* _Class::operator new(std::size_t size, const AZ::Internal::AllocatorDummy*) {                                                                                                                                                             \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                                                                                 \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value);                                                                                                                                     \
    }                                                                                                                                                                                                                                               \
    /* ========== Symetrical delete operators (required incase aznew throws) ========== */                                                                                                                                                          \
    _Template                                                                                                                                                                                                                                       \
    void  _Class::operator delete(void* p, const char*, int, const char*) {                                                                                                                                                                         \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                                                                                        \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void  _Class::operator delete(void* p, const AZ::Internal::AllocatorDummy*) {                                                                                                                                                                   \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                                                                                        \
    }                                                                                                                                                                                                                                               \
    /* ========== AZ_CLASS_ALLOCATOR API ========== */                                                                                                                                                                                              \
    _Template                                                                                                                                                                                                                                       \
    void* _Class::AZ_CLASS_ALLOCATOR_Allocate() {                                                                                                                                                                                                   \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(sizeof(_Class), AZStd::alignment_of< _Class >::value, _Flags, #_Class);                                                                                                          \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void _Class::AZ_CLASS_ALLOCATOR_DeAllocate(void* object) {                                                                                                                                                                                      \
        AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(object, sizeof(_Class), AZStd::alignment_of< _Class >::value);                                                                                                                        \
    }

#define AZ_CLASS_ALLOCATOR_IMPL(_Class, _Allocator, _Flags)                                                                                                                                                                                         \
    AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _Flags,)

#define AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(_Class, _Allocator, _Flags)                                                                                                                                                                                \
    AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _Flags, template<>)

//////////////////////////////////////////////////////////////////////////
// new operator overloads

// you can redefine this macro to whatever suits you.
#ifndef AZCORE_GLOBAL_NEW_ALIGNMENT
#   define AZCORE_GLOBAL_NEW_ALIGNMENT 16
#endif


/**
 * By default AZCore doesn't overload operator new and delete. This is a no-no for middle-ware.
 * You are encouraged to do that in your executable. What you need to do is to pipe all allocation trough AZCore memory manager.
 * AZCore relies on \ref AZ_CLASS_ALLOCATOR to specify the class default allocator or on explicit
 * azcreate/azdestroy calls which provide the allocator. If you class doesn't not implement the
 * \ref AZ_CLASS_ALLOCATOR when you call a new/delete they will use the global operator new/delete. In addition
 * if you call aznew on a class without AZ_CLASS_ALLOCATOR you will need to implement new operator specific to
 * aznew call signature.
 * So in an exception free environment (AZLibs don't have exception support) you need to implement the following functions:
 *
 * void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*);
 * void* operator new[](std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*);
 * void* operator new(std::size_t);
 * void* operator new[](std::size_t);
 * void operator delete(void*);
 * void operator delete[](void*);
 *
 * You can implement those functions anyway you like, or you can use the provided implementations for you! \ref Global New/Delete Operators
 * All allocations will happen using the AZ::SystemAllocator. Make sure you create it properly before any new calls.
 * If you use our default new implementation you should map the global functions like that:
 *
 * void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*)       { return AZ::OperatorNew(size,fileName,lineNum,name); }
 * void* operator new[](std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*)     { return AZ::OperatorNewArray(size,fileName,lineNum,name); }
 * void* operator new(std::size_t size)         { return AZ::OperatorNew(size); }
 * void* operator new[](std::size_t size)       { return AZ::OperatorNewArray(size); }
 * void operator delete(void* ptr)              { AZ::OperatorDelete(ptr); }
 * void operator delete[](void* ptr)            { AZ::OperatorDeleteArray(ptr); }
 */
namespace AZ
{
    /**
     * We use the memory managers in very weird tools environments, when allocators are created during initialization. This is why
     * we can't use the above version that uses an instance. Ideally such code should be avoided, but as of today many tools include
     * engine libs, in such cases fixing the issue becomes really hard. Especially since the engine libs override operator new.
     * So as of today we use a POD buffer for the allocator storage and create it on first request. This should BE considered TEMP solution and
     * we should attempt to remove this this implementation and use the one above ASAP.
     * IsReady function is specially made so it's "safe" to call it even before the Allocator is constructed. Again this is hack and a temp solution.
     * As of today the known offenders are MFC static lib that calls new.
     */
    template<class Allocator>
    class AllocatorInstance
    {
    public:
        typedef typename Allocator::Descriptor Descriptor;

        AZ_FORCE_INLINE static Allocator&   Get()
        {
            if (!s_allocator)
            {
                s_allocator = Environment::FindVariable<Allocator>(AzTypeInfo<Allocator>::Name());
                AZ_Assert(s_allocator, "Allocator '%s' NOT ready for use! Call Create first!", AzTypeInfo<Allocator>::Name());
            }
            return *s_allocator;
        }

        static void Create(const Descriptor& desc = Descriptor())
        {
            if (!s_allocator)
            {
                s_allocator = Environment::CreateVariable<Allocator>(AzTypeInfo<Allocator>::Name());
                if (s_allocator->IsReady()) // already created in a different module
                {
                    return;
                }
            }
            else
            {
                AZ_Assert(s_allocator->IsReady(), "Allocator '%s' already created!", AzTypeInfo<Allocator>::Name());
            }

            s_allocator->Create(desc);
        }

        static void Destroy()
        {
            if (s_allocator)
            {
                if (s_allocator.IsOwner())
                {
                    s_allocator->Destroy();
                }
                s_allocator = nullptr;
            }
            else
            {
                AZ_Assert(false, "Allocator '%s' NOT ready for use! Call Create first!", AzTypeInfo<Allocator>::Name());
            }
        }
        AZ_FORCE_INLINE static bool IsReady()
        {
            if (!s_allocator) // if not there check the environment
            {
                s_allocator = Environment::FindVariable<Allocator>(AzTypeInfo<Allocator>::Name());
            }
            return s_allocator;
        }
    private:
        static EnvironmentVariable<Allocator> s_allocator;
    };

    template<class Allocator>
    EnvironmentVariable<Allocator> AllocatorInstance<Allocator>::s_allocator;

    /**
    * Generic wrapper for binding allocator to an AZStd one.
    * \note AZStd allocators are one of the few differences from STD/STL.
    * It's very tedious to write a wrapper for STD too.
    */
    template<class Allocator>
    class AZStdAlloc
    {
    public:
        AZ_TYPE_INFO(AZStdAlloc, "{42D0AA1E-3C6C-440E-ABF8-435931150470}", Allocator);
        typedef void*               pointer_type;
        typedef AZStd::size_t       size_type;
        typedef AZStd::ptrdiff_t    difference_type;
        typedef AZStd::false_type   allow_memory_leaks;         ///< Regular allocators should not leak.

        AZ_FORCE_INLINE AZStdAlloc()
        {
            if (AllocatorInstance<Allocator>::IsReady())
            {
                m_name = AllocatorInstance<Allocator>::Get().GetName();
            }
            else
            {
                m_name = AzTypeInfo<Allocator>::Name();
            }
        }
        AZ_FORCE_INLINE AZStdAlloc(const char* name)
            : m_name(name)     {}
        AZ_FORCE_INLINE AZStdAlloc(const AZStdAlloc& rhs)
            : m_name(rhs.m_name)  {}
        AZ_FORCE_INLINE AZStdAlloc(const AZStdAlloc& rhs, const char* name)
            : m_name(name) { (void)rhs; }
        AZ_FORCE_INLINE AZStdAlloc& operator=(const AZStdAlloc& rhs) { m_name = rhs.m_name; return *this; }
        AZ_FORCE_INLINE pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0)
        {
            return AllocatorInstance<Allocator>::Get().Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
        }
        AZ_FORCE_INLINE size_type resize(pointer_type ptr, size_type newSize)
        {
            return AllocatorInstance<Allocator>::Get().Resize(ptr, newSize);
        }
        AZ_FORCE_INLINE void deallocate(pointer_type ptr, size_type byteSize, size_type alignment)
        {
            AllocatorInstance<Allocator>::Get().DeAllocate(ptr, byteSize, alignment);
        }
        AZ_FORCE_INLINE const char* get_name() const            { return m_name; }
        AZ_FORCE_INLINE void        set_name(const char* name)  { m_name = name; }
        size_type                   get_max_size() const        { return AllocatorInstance<Allocator>::Get().GetMaxAllocationSize(); }
        size_type                   get_allocated_size() const  { return AllocatorInstance<Allocator>::Get().NumAllocatedBytes(); }

        AZ_FORCE_INLINE bool is_lock_free()                     { return false; }
        AZ_FORCE_INLINE bool is_stale_read_allowed()            { return false; }
        AZ_FORCE_INLINE bool is_delayed_recycling()             { return false; }
    private:
        const char* m_name;
    };

    template<class Allocator>
    AZ_FORCE_INLINE bool operator==(const AZStdAlloc<Allocator>& a, const AZStdAlloc<Allocator>& b) { (void)a; (void)b; return true; } // always true since they use the same instance of AllocatorInstance<Allocator>
    template<class Allocator>
    AZ_FORCE_INLINE bool operator!=(const AZStdAlloc<Allocator>& a, const AZStdAlloc<Allocator>& b) { (void)a; (void)b; return false; } // always false since they use the same instance of AllocatorInstance<Allocator>

    /**
     * Generic wrapper for binding IAllocator interface allocator.
     * This is basically the same as \ref AZStdAlloc but it allows
     * you to remove the template parameter and set you interface on demand.
     * of course at a cost of a pointer.
     */
    class AZStdIAllocator
    {
    public:
        typedef void*               pointer_type;
        typedef AZStd::size_t       size_type;
        typedef AZStd::ptrdiff_t    difference_type;
        typedef AZStd::false_type   allow_memory_leaks;         ///< Regular allocators should not leak.

        AZ_FORCE_INLINE AZStdIAllocator(IAllocatorAllocate* allocator, const char* name = "AZ::AZStdIAllocator")
            : m_allocator(allocator)
            , m_name(name)
        {
            AZ_Assert(m_allocator != NULL, "You must provide a valid allocator!");
        }
        AZ_FORCE_INLINE AZStdIAllocator(const AZStdIAllocator& rhs)
            : m_allocator(rhs.m_allocator)
            , m_name(rhs.m_name)  {}
        AZ_FORCE_INLINE AZStdIAllocator(const AZStdIAllocator& rhs, const char* name)
            : m_allocator(rhs.m_allocator)
            , m_name(name) { (void)rhs; }
        AZ_FORCE_INLINE AZStdIAllocator& operator=(const AZStdIAllocator& rhs) { m_allocator = rhs.m_allocator; m_name = rhs.m_name; return *this; }
        AZ_FORCE_INLINE pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0)
        {
            return m_allocator->Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
        }
        AZ_FORCE_INLINE size_type resize(pointer_type ptr, size_t newSize)
        {
            return m_allocator->Resize(ptr, newSize);
        }
        AZ_FORCE_INLINE void deallocate(pointer_type ptr, size_t byteSize, size_t alignment)
        {
            m_allocator->DeAllocate(ptr, byteSize, alignment);
        }
        AZ_FORCE_INLINE const char* get_name() const { return m_name; }
        AZ_FORCE_INLINE void        set_name(const char* name) { m_name = name; }
        size_type                   get_max_size() const { return m_allocator->GetMaxAllocationSize(); }
        size_type                   get_allocated_size() const { return m_allocator->NumAllocatedBytes(); }

        AZ_FORCE_INLINE bool operator==(const AZStdIAllocator& rhs) const { return m_allocator == rhs.m_allocator; }
        AZ_FORCE_INLINE bool operator!=(const AZStdIAllocator& rhs) const { return m_allocator != rhs.m_allocator; }
    private:
        IAllocatorAllocate* m_allocator;
        const char* m_name;
    };

    /**
    * Helper class to determine if type T has a AZ_CLASS_ALLOCATOR defined,
    * so we can safely call aznew on it. -  AZClassAllocator<ClassType>....
    */
    AZ_HAS_MEMBER(AZClassAllocator, AZ_CLASS_ALLOCATOR_DECLARED, void, ())

    // {@ Global New/Delete Operators
    void* OperatorNew(std::size_t size, const char* fileName, int lineNum);
    void* OperatorNew(std::size_t byteSize);
    void OperatorDelete(void* ptr);

    void* OperatorNewArray(std::size_t size, const char* fileName, int lineNum);
    void* OperatorNewArray(std::size_t byteSize);
    void OperatorDeleteArray(void* ptr);
    // @}
}

// define unlimited allocator limits (scaled to real number when we check if there is enough memory to allocate)
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
#   define AZ_CORE_MAX_ALLOCATOR_SIZE ((size_t)2 * 1024 * 1024 * 1024)
#elif defined(AZ_PLATFORM_WINDOWS_X64) || defined(AZ_PLATFORM_APPLE_OSX)
#   define AZ_CORE_MAX_ALLOCATOR_SIZE ((size_t)8 * 1024 * 1024 * 1024)
#elif defined(AZ_PLATFORM_LINUX)
#   define AZ_CORE_MAX_ALLOCATOR_SIZE ((size_t)8 * 1024 * 1024 * 1024)
#else
#   define AZ_CORE_MAX_ALLOCATOR_SIZE ((size_t)10 * 1024 * 1024)
#endif

#endif // AZCORE_MEMORY_H
#pragma once



