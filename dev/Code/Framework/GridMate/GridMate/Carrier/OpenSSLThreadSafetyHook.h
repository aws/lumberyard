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
#pragma once

#include <GridMate/Memory.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/atomic.h>

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX)

namespace GridMate
{
    /*
    * Implements callbacks needed by OpenSSL for thread safety.
    * The current implementation is compatible with v1.0.2.
    * NOTE:
    * Due to the global nature of OpenSSL thread safety hooks, when this
    * implementation is installed, all OpenSSL requests will be handled
    * by it, including other users of OpenSSL.
    * This implementation is ref-counted so that the hooks will remain in
    * place until all instances have been released.
    * If another hook was already installed, this implementation will not
    * install itself.
    * This implementation purposedly uses a static variable to track refcounts
    * so its scope of incluence is the same as OpenSSL's (per process/DLL).
    */
    class OpenSSLThreadSafetyHook
    {
    public:
        OpenSSLThreadSafetyHook();
        ~OpenSSLThreadSafetyHook();

    protected:
        static AZStd::atomic_int s_refCount;
    };
} // namespace GridMate

#endif