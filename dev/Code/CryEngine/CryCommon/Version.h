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

// Description : Contains the build version of the Engine


#ifndef CRYINCLUDE_CRYCOMMON_VERSION_H
#define CRYINCLUDE_CRYCOMMON_VERSION_H
#pragma once


// The macros below define the engine version. Their names are self-
// -explanatory. These macros can be overriden by defines in two ways:
// 1) Hard coded - Define each of the CRYENGINE_VERSION_* in CryCommon/Version_override.h
// 2) Config tool - Define eacho of the CRYENGINE_CONFIG_VERSION_*
//
// If there are no defines, all the values will default to 0
//

#include "Version_override.h"

#ifndef CRYENGINE_VERSION_MAJOR
    #ifndef CRYENGINE_CONFIG_VERSION_MAJOR
        #define CRYENGINE_VERSION_MAJOR 0
    #else
        #define CRYENGINE_VERSION_MAJOR CRYENGINE_CONFIG_VERSION_MAJOR
    #endif
#endif

#ifndef CRYENGINE_VERSION_MINOR
    #ifndef CRYENGINE_CONFIG_VERSION_MINOR
        #define CRYENGINE_VERSION_MINOR 0
    #else
        #define CRYENGINE_VERSION_MINOR CRYENGINE_CONFIG_VERSION_MINOR
    #endif
#endif

#ifndef CRYENGINE_VERSION_REVISION
    #ifndef CRYENGINE_CONFIG_VERSION_REVISION
        #define CRYENGINE_VERSION_REVISION 0
    #else
        #define CRYENGINE_VERSION_REVISION CRYENGINE_CONFIG_VERSION_REVISION
    #endif
#endif

#ifndef CRYENGINE_VERSION_BUILDNUM
    #ifndef CRYENGINE_CONFIG_VERSION_BUILDNUM
        #define CRYENGINE_VERSION_BUILDNUM 0
    #else
        #define CRYENGINE_VERSION_BUILDNUM CRYENGINE_CONFIG_VERSION_BUILDNUM
    #endif
#endif

#endif // CRYINCLUDE_CRYCOMMON_VERSION_H
