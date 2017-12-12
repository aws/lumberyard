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

// Description : Launcher implementation for the linux client and the linux dedicated server


/*
extern "C" void __assert_fail(
        const char *assertion,
        const char *file,
        unsigned int line,
        const char *function)
  __attribute__ ((visibility("default")))
    ;

extern "C" void __assert_perror_fail(
        int errnum,
        const char *file,
        unsigned int line,
        const char *function)
    __attribute__ ((visibility("default")))
    ;

void AssertFail(
        const char *assertion,
        int errnum,
        const char *file,
        unsigned int line,
        const char *function);

void __assert_fail(
        const char *assertion,
        const char *file,
        unsigned int line,
        const char *function)
{
    AssertFail(assertion, 0, file, line, function);
}

void __assert_perror_fail(
        int errnum,
        const char *file,
        unsigned int line,
        const char *function)
{
    AssertFail(0, errnum, file, line, function);
}
*/
// vim:ts=2

