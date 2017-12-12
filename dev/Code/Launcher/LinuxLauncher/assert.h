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

// This is a hack to make __assert_fail and friends non-noreturn.
// Whenever <assert.h> is included, we'll include this file (enforced via -I).
#if !defined(_LinuxLauncher_assert_h_)
#define _LinuxLauncher_assert_h_

#define __assert_fail __assert_fail__SKIP
#define __assert_perror_fail __assert_perror_fail__SKIP
#include_next <assert.h>
#undef __assert_fail
#undef __assert_perror_fail

#if defined(__cplusplus)
extern "C" {
#endif
void __assert_fail(
    const char* assertion,
    const char* file,
    unsigned int line,
    const char* function)
__attribute__ ((visibility("default")))
;

void __assert_perror_fail(
    int errnum,
    const char* file,
    unsigned int line,
    const char* function)
__attribute__ ((visibility("default")))
;
#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _LinuxLauncher_assert_h_

// vim:ts=2

