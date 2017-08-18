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
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H_2E9D62A4_C062_41D7_B044_D8442297976E_INCLUDED_)
#define AFX_STDAFX_H_2E9D62A4_C062_41D7_B044_D8442297976E_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <CryModuleDefs.h>
#define eCryModule eCryM_AudioUnitTests

#define CRYUNITTESTS_EXPORTS

#include <vector>
#include <list>
#include <map>
#include <algorithm>

#include <platform.h>
#include <ISystem.h>

#include "SoundAllocator.h"

namespace Audio
{
    extern CSoundAllocator g_audioMemoryPool_UnitTests;
} // namespace Audio

#define AUDIO_ALLOCATOR_MEMORY_POOL g_audioMemoryPool_UnitTests

#include <STLSoundAllocator.h>
#include <AudioInternalInterfaces.h>

//extern SSystemGlobalEnvironment gEnv;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H_2E9D62A4_C062_41D7_B044_D8442297976E_INCLUDED_)
