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

// Description : Settings for all builds under MS Visual C++ compiler


#ifndef CRYINCLUDE_CRYCOMMON_MSVCSPECIFIC_H
#define CRYINCLUDE_CRYCOMMON_MSVCSPECIFIC_H
#pragma once


// omit frame pointers is only available in MSVC 32bit
// note that in platform.h, due to legacy, on a win64 system, both WIN32 and WIN64 is defined
// so in order for this not to cause compile issues on WIN64 systems, you must explicitly check.
#if defined(WIN32) && !defined(_RELEASE) && !defined(WIN64)
#pragma optimize( "y", off ) // Generate frame pointers on the program stack. (Disable Omit Frame Pointers optimization, call stack unwinding cannot work with it)
#endif

// Disable (and enable) specific compiler warnings.
// MSVC compiler is very confusing in that some 4xxx warnings are shown even with warning level 3,
// and some 4xxx warnings are NOT shown even with warning level 4.

#pragma warning(disable: 4018)  // signed/unsigned mismatch
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#pragma warning(disable: 4503)  // decorated name length exceeded, name was truncated
#pragma warning(disable: 6255)  // _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead. (Note: _malloca requires _freea.)


// Turn on the following very useful warnings.
#pragma warning(3: 4264)                // no override available for virtual member function from base 'class'; function is hidden
#pragma warning(3: 4266)                // no override available for virtual member function from base 'type'; function is hidden

#endif // CRYINCLUDE_CRYCOMMON_MSVCSPECIFIC_H
