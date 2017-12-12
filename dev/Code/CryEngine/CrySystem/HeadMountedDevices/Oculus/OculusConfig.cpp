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

#include "StdAfx.h"

// include this header at least here to properly link to the respective Oculus libs on demand
#include "OculusConfig.h"


#ifndef EXCLUDE_OCULUS_SDK

#if defined(WIN32)
#   if defined(WIN64)
#       if defined(_DEBUG)
#           pragma message (">>> include lib: libovr64d.lib")
#           pragma comment (lib, "../../SDKs/OculusSDK/LibOVR/Lib/x64/VS2013/libovr64d.lib")
#       else
#           pragma message (">>> include lib: libovr64.lib")
#           pragma comment (lib, "../../SDKs/OculusSDK/LibOVR/Lib/x64/VS2013/libovr64.lib")
#       endif
#   else
#       pragma error ("32 bit not supported")
#   endif
#endif

#endif // #ifndef EXCLUDE_OCULUS_SDK
