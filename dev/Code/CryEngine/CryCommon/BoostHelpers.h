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

// Description : Helper macros/methods/classes for boost.


#ifndef CRYINCLUDE_CRYCOMMON_BOOSTHELPERS_H
#define CRYINCLUDE_CRYCOMMON_BOOSTHELPERS_H
#pragma once



#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>

#define DECLARE_SMART_POINTERS(name)                      \
    typedef AZStd::shared_ptr<name> name##Ptr;            \
    typedef AZStd::shared_ptr<const name> name##ConstPtr; \
    typedef AZStd::weak_ptr<name> name##WeakPtr;          \
    typedef AZStd::weak_ptr<const name> name##ConstWeakPtr;

// HACK for pre-VS2013 builds to avoid macro redefinitions
// Sandbox includes afxcontrolbars.h, boost variant and mpl include stdint.h.
// Both define the following macros unconditionally...

#if defined(SANDBOX_EXPORTS)
#if defined(_MSC_VER)
#if _MSC_VER < 1800

#   if defined(INT8_MIN)
#       undef INT8_MIN
#   endif
#   if defined(INT16_MIN)
#       undef INT16_MIN
#   endif
#   if defined(INT32_MIN)
#       undef INT32_MIN
#   endif
#   if defined(INT64_MIN)
#       undef INT64_MIN
#   endif

#   if defined(INT8_MAX)
#       undef INT8_MAX
#   endif
#   if defined(INT16_MAX)
#       undef INT16_MAX
#   endif
#   if defined(INT32_MAX)
#       undef INT32_MAX
#   endif
#   if defined(INT64_MAX)
#       undef INT64_MAX
#   endif

#   if defined(UINT8_MIN)
#       undef UINT8_MIN
#   endif
#   if defined(UINT16_MIN)
#       undef UINT16_MIN
#   endif
#   if defined(UINT32_MIN)
#       undef UINT32_MIN
#   endif
#   if defined(UINT64_MIN)
#       undef UINT64_MIN
#   endif

#   if defined(UINT8_MAX)
#       undef UINT8_MAX
#   endif
#   if defined(UINT16_MAX)
#       undef UINT16_MAX
#   endif
#   if defined(UINT32_MAX)
#       undef UINT32_MAX
#   endif
#   if defined(UINT64_MAX)
#       undef UINT64_MAX
#   endif
#endif // #if defined(_MSC_VER)
#endif // #if _MSC_VER < 1800
#endif // #if defined(SANDBOX_EXPORTS)

#pragma warning(push)
#pragma warning(disable : 4345)
#include <boost/variant.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/find.hpp>
#pragma warning(pop)

#endif // CRYINCLUDE_CRYCOMMON_BOOSTHELPERS_H
