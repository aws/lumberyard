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

#ifndef _CRY_PLATFORM_FORWARD_DECLARATIONS_H_
#define _CRY_PLATFORM_FORWARD_DECLARATIONS_H_

////////////////////////////////////////////////////////////////////////////
// check that we are allowed to be included
#if !defined(CRYPLATFROM_ALLOW_DETAIL_INCLUDES)
#   error Please include CryPlatfrom.h instead of this private implementation header
#endif


////////////////////////////////////////////////////////////////////////////
// util macro to create opaque objects which a specific size and alignment
#define DEFINE_OPAQUE_TYPE(name, size, alignment) ALIGN_STRUCTURE(alignment, struct name { private: char pad[size]; })


////////////////////////////////////////////////////////////////////////////
// Multithreading primitive functions
////////////////////////////////////////////////////////////////////////////
namespace CryMT {
    ////////////////////////////////////////////////////////////////////////////
    // structure forward declarations
    struct SInterlockedSListHeader;
    struct SInterlockedSListElement;

    ////////////////////////////////////////////////////////////////////////////
    // function forward declarations
    namespace detail
    {
        ////////////////////////////////////////////////////////////////////////////
        // interlocked single linked list functions
        void InterlockedSListInitialize(SInterlockedSListHeader* pHeader);
        void InterlockedSListFlush(SInterlockedSListHeader* pHeader);
        void InterlockedSListPush(SInterlockedSListHeader* pHeader, SInterlockedSListElement* pElement);
        SInterlockedSListElement* InterlockedSListPop(SInterlockedSListHeader* pHeader);
    } // namespace detail
} // namespace CryMT

////////////////////////////////////////////////////////////////////////////
#endif // _CRY_PLATFORM_FORWARD_DECLARATIONS_H_