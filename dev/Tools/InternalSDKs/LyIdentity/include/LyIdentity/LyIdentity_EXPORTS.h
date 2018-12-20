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

#ifdef LINK_LY_IDENTITY_DYNAMICALLY
    #if defined(_MSC_VER) // <- always set by the Visual Studio compilers, which use __declspec
        #pragma warning(disable : 4251)

        #ifdef LY_IDENTITY_EXPORTS
            #define LY_IDENTITY_API __declspec(dllexport)
        #else
            #define LY_IDENTITY_API __declspec(dllimport)
        #endif /* LY_IDENTITY_EXPORTS */
    #elif defined(_WIN32) && defined(__clang__)
        #ifdef LY_IDENTITY_EXPORTS
            #define LY_IDENTITY_API __attribute__ ((dllexport))
        #else
            #define LY_IDENTITY_API __attribute__ ((dllimport))
        #endif /* LY_IDENTITY_EXPORTS */
    #else /* _MSC_VER */
        #define LY_IDENTITY_API __attribute__ ((visibility("default")))
    #endif
#else
    #define LY_IDENTITY_API
#endif
