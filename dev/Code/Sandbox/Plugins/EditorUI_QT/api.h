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
#ifndef CRYINCLUDE_EDITORUI_QT_API_H
#define CRYINCLUDE_EDITORUI_QT_API_H

#include <AzCore/PlatformDef.h>

#if defined(EDITOR_QT_UI_EXPORTS)
    #define EDITOR_QT_UI_API AZ_DLL_EXPORT
#elif defined(EDITOR_QT_UI_IMPORTS) || defined(AZ_PLATFORM_WINDOWS)
    #define EDITOR_QT_UI_API AZ_DLL_IMPORT
#else
    #define EDITOR_QT_UI_API
#endif

#endif // CRYINCLUDE_EDITORUI_QT_API_H
