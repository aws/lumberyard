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

// Description : Here the actual input implementation gets chosen for the
//               different platforms


#ifndef CRYINCLUDE_CRYINPUT_CRYINPUT_H
#define CRYINCLUDE_CRYINPUT_CRYINPUT_H
#pragma once

#include <AzFramework/Input/System/InputSystemComponent.h>

#if !defined(DEDICATED_SERVER)
    #if defined(AZ_FRAMEWORK_INPUT_ENABLED)
        #define USE_AZ_TO_LY_INPUT
        #include "AzToLyInput.h"
    #elif defined(ANDROID)
        #define USE_ANDROIDINPUT
        #include "AndroidInput.h"
    #elif defined(IOS)
// Mobile client
        #define USE_IOSINPUT
        #include "IosInput.h"
    #elif defined(APPLETV)
        #define USE_APPLETVINPUT
        #include "AppleTVInput.h"
    #elif defined(LINUX) || defined(APPLE)
// Linux client (not dedicated server)
        #define USE_LINUXINPUT
        #include "LinuxInput.h"
    #elif defined (WIN32)
        #define USE_DXINPUT
        #include "DXInput.h"
    #endif
    #if !defined(_RELEASE) && !defined(WIN32)
        #define USE_SYNERGY_INPUT
    #endif
    #include "InputCVars.h"
#endif

#endif // CRYINCLUDE_CRYINPUT_CRYINPUT_H

