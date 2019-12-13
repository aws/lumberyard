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

// MSVC warns that these typedef members are being used and therefore must be acknowledged by using the following macro
#pragma push_macro("_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
#ifndef _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#endif
#include <QtGlobal>

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(5033, "-Wunused-warning-option") // Disabling C++17 warning - warning C5033: 'register' is no longer a supported storage class

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <Cry_Math.h>
#include <ISystem.h>
#include <ISerialize.h>
#include <CryName.h>

/////////////////////////////////////////////////////////////////////////////
// STL
/////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
AZ_POP_DISABLE_WARNING
#pragma pop_macro("_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")