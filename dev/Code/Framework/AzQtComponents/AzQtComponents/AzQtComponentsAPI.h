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

#include <AzCore/PlatformDef.h>

#if defined(AZ_QT_COMPONENTS_STATIC)
    // if we're statically linking, then we don't need to export or import symbols
    #define AZ_QT_COMPONENTS_API
#elif defined(AZ_QT_COMPONENTS_EXPORT_SYMBOLS)
    #define AZ_QT_COMPONENTS_API AZ_DLL_EXPORT
#else
    #define AZ_QT_COMPONENTS_API AZ_DLL_IMPORT
#endif

