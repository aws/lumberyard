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

#ifndef _CRY_PLATFORM_WIN64_H_
#define _CRY_PLATFORM_WIN64_H_


////////////////////////////////////////////////////////////////////////////
// check that we are allowed to be included
#if !defined(CRYPLATFROM_ALLOW_DETAIL_INCLUDES)
#   error Please include CryPlatfrom.h instead of this private implementation header
#endif


////////////////////////////////////////////////////////////////////////////
// size and alignment settings for platfrom specific primities
////////////////////////////////////////////////////////////////////////////
// Interlocked singled linked list settings
#define CRYPLATFORM_INTERLOCKEDSLIST_HEADER_SIZE                8
#define CRYPLATFORM_INTERLOCKEDSLIST_HEADER_ALIGNMENT       16
#define CRYPLATFORM_INTERLOCKEDSLIST_ELEMENT_SIZE               8
#define CRYPLATFORM_INTERLOCKEDSLIST_ELEMENT_ALIGNMENT  16


////////////////////////////////////////////////////////////////////////////
// macros for platfrom specific functionality which cannot be expressed as a C++ function
////////////////////////////////////////////////////////////////////////////
// mark a structure as aligned
#define ALIGN_STRUCTURE(alignment, structure) __declspec(align(alignment)) structure


////////////////////////////////////////////////////////////////////////////
// Create a string from an Preprocessor macro or a literal text
#define CRY_DETAIL_CREATE_STRING(string) #string
#define CRY_CREATE_STRING(string) CRY_DETAIL_CREATE_STRING(string)
#define RESOLVE_MACRO(x) x

////////////////////////////////////////////////////////////////////////////
#define __DETAIL__LINK_SYSTEM_PARTY_LIBRARY(name)                                                                       \
    __pragma(message(__FILE__ "(" CRY_CREATE_STRING(__LINE__) "): Including System Library: " CRY_CREATE_STRING(name))) \
    __pragma(comment(lib, CRY_CREATE_STRING(name)))

#endif // _CRY_PLATFORM_WIN64_H_
