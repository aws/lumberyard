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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Utility functions for packing and unpacking stored structures
//               Structure packing unpacking is required for machines where the size of
//               a pointer is different from 4 bytes  e g  Cell PPU


#ifndef CRYINCLUDE_CRYCOMMON_CRYSTRUCTPACK_H
#define CRYINCLUDE_CRYCOMMON_CRYSTRUCTPACK_H
#pragma once

#include <algorithm> // functor.h needs std::find
#include "functor.h"

#include "CryTypeInfo.h"

#ifdef max
    #undef max
#endif
#ifdef min
    #undef min
#endif

#define NEED_STRUCT_PACK    (SIZEOF_PTR != 4)

#if NEED_STRUCT_PACK
uint32 StructSize(const CTypeInfo& typeInfo, uint32 limit = UINT_MAX);
uint32 StructUnpack(const CTypeInfo& typeInfo, uint8* ptr, const uint8* buffer,
    uint32 limit = UINT_MAX, bool expandPointers = false);
uint32 StructPack(const CTypeInfo& typeInfo, const uint8* ptr, uint8* buffer,
    uint32 limit = UINT_MAX, bool packPointers = false);
#else
static inline uint32 StructSize(const CTypeInfo& typeInfo,
    uint32 limit = UINT_MAX)
{
    return std::max((size_t)limit, typeInfo.Size);
}

static inline uint32 StructUnpack(const CTypeInfo& typeInfo,
    uint8* ptr,
    const uint8* buffer,
    uint32 limit = UINT_MAX,
    bool expandPointers = false)
{
    const uint32 size = StructSize(typeInfo, limit);

    memcpy(ptr, buffer, size);
    if (size < typeInfo.Size)
    {
        assert(size == limit);
        memset(ptr + size, 0, typeInfo.Size - size);
    }
#ifdef NEED_ENDIAN_SWAP
    SwapEndian(typeInfo, typeInfo.Size, ptr);
#endif
    return size;
}

static inline uint32 StructPack(const CTypeInfo& typeInfo,
    const uint8* ptr,
    uint8* buffer,
    uint32 limit = UINT_MAX,
    bool packPointers = false)
{
    const uint32 size = StructSize(typeInfo, limit);

    memcpy(buffer, ptr, size);
    if (size < typeInfo.Size)
    {
        assert(size == limit);
        memset(buffer + size, 0, typeInfo.Size - size);
    }
#ifdef NEED_ENDIAN_SWAP
    SwapEndian(typeInfo, typeInfo.Size, buffer);
#endif
    return size;
}

#endif

// Get the size of a packed structure. If only part of the structure is
// relevant, then (limit) must be specified as the offset of the first
// structure element not relevant for packing/unpacking. The offset may
// be determined using the offsetof() macro.
template <class T>
static inline uint32 StructSize(uint32 limit = UINT_MAX)
{
#if NEED_STRUCT_PACK
    return StructSize(TypeInfo((T*)0), limit);
#else
    return std::max((size_t)limit, sizeof(T));
#endif
}

// Unpack a stored structure.
// @param ptr
//      The unpacked object will be stored to *ptr. The function takes care
//      of endian swapping if needed.
// @param buffer
//      Buffer holding the raw data read from storage.
// @param limit
//      Optional. If only part of the structure should be unpacked, then limit is
//      the offset of the first structure element that should be skipped. The
//      limit can be determined using the offsetof() macro.
// @param expandPointers
//      Flag indicating if pointers should be expanded/extracted. The default is
//      false. If this flag is not set, then all pointers are set to NULL in
//      the extracted structure.
//
// Note: If a limit is specified, then the structure fields not read from
// storage are filled with zeroes. As a consequence, the size of the buffer
// referenced by ptr must be at least sizeof(T).
template <class T>
static inline void StructUnpack(T* ptr,
    const void* buffer,
    uint32 limit = UINT_MAX,
    bool expandPointers = false)
{
#if NEED_STRUCT_PACK
    StructUnpack(TypeInfo(ptr), (uint8*)ptr, (const uint8*)buffer, limit, expandPointers);
#else
    memcpy(ptr, buffer, std::max((size_t)limit, sizeof(T)));
    SwapEndian(ptr);
#endif
}

// Unpack a stored structure.
// This function is a variant of the StructUnpack() function above. The
// readFn functor is used for reading the structure from the storage.
template <class T>
static inline void StructUnpack(T* ptr,
    const Functor2<void*, uint32>& readFn,
    uint32 limit = UINT_MAX,
    bool expandPointers = false)
{
#if SIZE_OF_PTR != 4
#   if defined(__GNUC__)
    const uint32 size = StructSize<T>(limit);
    uint8 buffer[size];
# else
    const uint32 size = StructSize<T>(limit);
    uint8* buffer = (uint8*) alloca(size);
# endif
    readFn(buffer, size);
    StructUnpack(ptr, buffer, limit, expandPointers);
#else
    readFn(ptr, std::max(limit, sizeof T));
    SwapEndian(ptr);
#endif
}

// Pack a structure for storage.
template <class T>
static inline void StructPack(const T* ptr,
    void* buffer,
    uint32 limit = UINT_MAX,
    bool packPointers = false)
{
#if NEED_STRUCT_PACK
    StructPack(TypeInfo(ptr), (const void*)ptr, buffer, limit, packPointers);
#else
    const uint32 size = std::max((size_t)limit, sizeof(T));
    const CTypeInfo& typeInfo = TypeInfo(ptr);

#ifdef NEED_ENDIAN_SWAP
    if (size < typeInfo.Size)
    {
        assert(size == limit);
        uint8 tmpBuffer[typeInfo.Size];
        memcpy(tmpBuffer, ptr, size);
        memset(tmpBuffer + size, 0, typeInfo.Size - size);
        SwapEndian(tmpBuffer, 1, typeInfo, typeInfo.Size);
        memcpy(buffer, tmpBuffer, size);
    }
    else
    {
        assert(size == typeInfo.Size);
        memcpy(buffer, ptr, size);
        SwapEndian(typeInfo, size, buffer);
    }
#else
    memcpy(buffer, ptr, size);
#endif
#endif
}

template <class T>
static inline void StructPack(const T* ptr,
    const Functor2<const void*, uint32>& writeFn,
    uint32 limit = UINT_MAX,
    bool packPointers = false)
{
#if NEED_STRUCT_PACK
    const uint32 size = StructSize<T>(limit);
    uint8 buffer[size];

    StructPack(ptr, buffer, limit, packPointers);
    writeFn(buffer, size);
#else
    const uint32 size = std::max((size_t)limit, sizeof(T));
#ifdef NEED_ENDIAN_SWAP
    const CTypeInfo& typeInfo = TypeInfo(ptr);
    uint8 buffer[typeInfo.Size];
    memcpy(buffer, ptr, size);
    if (size < typeInfo.Size)
    {
        assert(size == limit);
        memset(buffer + size, 0, typeInfo.Size - size);
    }
    SwapEndian(buffer, TypeInfo(ptr), size);
    writeFn(buffer, size);
#else
    writeFn(ptr, size);
#endif
#endif
}

#endif // CRYINCLUDE_CRYCOMMON_CRYSTRUCTPACK_H
