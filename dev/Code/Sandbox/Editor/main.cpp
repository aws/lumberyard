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

#include <Include/SandboxAPI.h>
#include <platform_impl.h>

#if defined(AZ_PLATFORM_WINDOWS)
#pragma comment(lib, "Shell32.lib")
#ifdef _DEBUG
#pragma comment(lib, "qtmaind.lib")
#else
#pragma comment(lib, "qtmain.lib")
#endif
#endif

int SANDBOX_API CryEditMain(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    return CryEditMain(argc, argv);
}
