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

// Description : Interface for the platfrom specific function libraries
//               Include this file instead of windows h and similar platfrom specific header files

#pragma once

////////////////////////////////////////////////////////////////////////////
// this define allows including the detail headers which are setting platfrom specific settings
#define CRYPLATFROM_ALLOW_DETAIL_INCLUDES

// this file can't include azcore since it is included by tools that use ancient compilers

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define CRYPLATFORM_H_SECTION_1 1
#define CRYPLATFORM_H_SECTION_2 2
#endif

#if defined(IOS)
#define CURRENT_PLATFORM_NAME "ios"
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(APPLETV)
#define CURRENT_PLATFORM_NAME "appletv"
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(DARWIN)
#define CURRENT_PLATFORM_NAME "osx"
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(LINUX) && !defined(ANDROID)
#define CURRENT_PLATFORM_NAME "linux"
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYPLATFORM_H_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/CryPlatform_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/CryPlatform_h_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32)
#define CURRENT_PLATFORM_NAME "windows"
#elif defined(ANDROID)
#define CURRENT_PLATFORM_NAME "android"
#else
#error Unknown or unsupported platform!
#endif

////////////////////////////////////////////////////////////////////////////
// some ifdef selection to include the correct platform implementation
#if defined(WIN64)
#   include "CryPlatform.Win64.h"
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYPLATFORM_H_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/CryPlatform_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/CryPlatform_h_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32)
#   include "CryPlatform.Win32.h"
#elif defined(LINUX)
#   include "CryPlatform.Linux.h"
#elif defined(APPLE)
#   include "CryPlatform.Darwin.h"
#else
#   error Current Platform not supported by CryPlatform. Please provide a concrete implementation library.
#endif

// include forward declarations of all *::detail functions which live in the specific CryPlatfrom libraries
#include "CryPlatform.fwd.h"


///////////////////////////////////////////////////////////////////////////
// verify that all requiered macros are set
///////////////////////////////////////////////////////////////////////////
#if !defined(CRYPLATFORM_INTERLOCKEDSLIST_HEADER_SIZE)
#   error CRYPLATFORM_INTERLOCKEDSLIST_HEADER_SIZE not defined for current platform
#endif

#if !defined(CRYPLATFORM_INTERLOCKEDSLIST_HEADER_ALIGNMENT)
#   error CRYPLATFORM_INTERLOCKEDSLIST_HEADER_ALIGNMENT not defined for current platform
#endif

#if !defined(CRYPLATFORM_INTERLOCKEDSLIST_ELEMENT_SIZE)
#   error CRYPLATFORM_INTERLOCKEDSLIST_ELEMENT_SIZE not defined for current platform
#endif

#if !defined(CRYPLATFORM_INTERLOCKEDSLIST_ELEMENT_ALIGNMENT)
#   error CRYPLATFORM_INTERLOCKEDSLIST_ELEMENT_ALIGNMENT not defined for current platform
#endif

#if !defined(__DETAIL__LINK_SYSTEM_PARTY_LIBRARY)
#   error __DETAIL__LINK_SYSTEM_PARTY_LIBRARY not defined for current platform
#endif

////////////////////////////////////////////////////////////////////////////
// Multithreading primitive functions
////////////////////////////////////////////////////////////////////////////
namespace CryMT {
    ////////////////////////////////////////////////////////////////////////////
    // Interlocked Single Linked List functions - without ABA Problem
    ////////////////////////////////////////////////////////////////////////////
    //
    // SInterlockedSListHeader:     Header of a interlocked singled linked list. Must be initialized before using.
    // SInterlockedSListElement:    Element of a interlocked single linked list.
    //
    DEFINE_OPAQUE_TYPE(SInterlockedSListHeader, CRYPLATFORM_INTERLOCKEDSLIST_HEADER_SIZE, CRYPLATFORM_INTERLOCKEDSLIST_HEADER_ALIGNMENT);
    DEFINE_OPAQUE_TYPE(SInterlockedSListElement, CRYPLATFORM_INTERLOCKEDSLIST_ELEMENT_SIZE, CRYPLATFORM_INTERLOCKEDSLIST_ELEMENT_ALIGNMENT);

    // Initialize a interlocked single linked list header. Must be called once before using the list.
    void InterlockedSListInitialize(SInterlockedSListHeader* pHeader);

    // Flush a interlocked single linked list. Afterwards the header points to an empty list. Returns XXX
    void InterlockedSListFlush(SInterlockedSListHeader* pHeader);

    // Pushes a new element at the beginning of the interlocked single linked list.
    void InterlockedSListPush(SInterlockedSListHeader* pHeader, SInterlockedSListElement* pElement);

    // Pops the first element from the interlocked single link list. Returns NULL if the list was empty.
    SInterlockedSListElement* InterlockedSListPop(SInterlockedSListHeader* pHeader);
} // namespace CryMT


////////////////////////////////////////////////////////////////////////////
// Interlocked Single Linked List functions Implementations
////////////////////////////////////////////////////////////////////////////
inline void CryMT::InterlockedSListInitialize(SInterlockedSListHeader* pHeader)
{
    CryMT::detail::InterlockedSListInitialize(pHeader);
}

////////////////////////////////////////////////////////////////////////////
inline void CryMT::InterlockedSListFlush(CryMT::SInterlockedSListHeader* pHeader)
{
    CryMT::detail::InterlockedSListFlush(pHeader);
}

////////////////////////////////////////////////////////////////////////////
inline void CryMT::InterlockedSListPush(CryMT::SInterlockedSListHeader* pHeader, CryMT::SInterlockedSListElement* pElement)
{
    CryMT::detail::InterlockedSListPush(pHeader, pElement);
}

////////////////////////////////////////////////////////////////////////////
inline CryMT::SInterlockedSListElement* CryMT::InterlockedSListPop(CryMT::SInterlockedSListHeader* pHeader)
{
    return CryMT::detail::InterlockedSListPop(pHeader);
}

////////////////////////////////////////////////////////////////////////////
// Include a platform library.
#define LINK_SYSTEM_LIBRARY(name) __DETAIL__LINK_SYSTEM_PARTY_LIBRARY(name)

////////////////////////////////////////////////////////////////////////////
// disallow including of detail header
#undef CRYPLATFROM_ALLOW_DETAIL_INCLUDES
