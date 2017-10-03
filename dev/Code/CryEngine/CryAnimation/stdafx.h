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

#ifndef CRYINCLUDE_CRYANIMATION_STDAFX_H
#define CRYINCLUDE_CRYANIMATION_STDAFX_H

//#define NOT_USE_CRY_MEMORY_MANAGER
#define CRYANIMATION_EXPORTS

//#define DEFINE_MODULE_NAME "CryAnimation"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>

//! Include standard headers.
#include <platform.h>

#if defined(WIN64)
#define CRY_INTEGRATE_DX12  
#endif

#ifdef ASSERT_ON_ANIM_CHECKS
#   define ANIM_ASSERT_TRACE(condition, parenthese_message) CRY_ASSERT_TRACE(condition, parenthese_message)
#   define ANIM_ASSERT(condition) CRY_ASSERT(condition)
#else
#   define ANIM_ASSERT_TRACE(condition, parenthese_message)
#   define ANIM_ASSERT(condition)
#endif

#define LOG_ASSET_PROBLEM(condition, parenthese_message)                   \
    do                                                                     \
    {                                                                      \
        gEnv->pLog->LogWarning("Anim asset error: %s failed", #condition); \
        gEnv->pLog->LogWarning parenthese_message;                         \
    } while (0)

#define ANIM_ASSET_CHECK_TRACE(condition, parenthese_message)     \
    do                                                            \
    {                                                             \
        bool anim_asset_ok = (condition);                         \
        if (!anim_asset_ok)                                       \
        {                                                         \
            ANIM_ASSERT_TRACE(anim_asset_ok, parenthese_message); \
            LOG_ASSET_PROBLEM(condition, parenthese_message);     \
        }                                                         \
    } while (0)

#pragma warning(disable : 4996) //'function': was declared deprecated
#pragma warning(default:4996)   // 'stricmp' was declared deprecated


#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Casting/lossy_cast.h>

//! Include main interfaces.
#include <Cry_Math.h>

#include <ISystem.h>
#include <ITimer.h>
#include <ILog.h>
#include <IConsole.h>
#include <ICryPak.h>
#include <StlUtils.h>

#include <ICryAnimation.h>
#include "AnimationBase.h"

#define SIZEOF_ARRAY(arr) (sizeof(arr) / sizeof((arr)[0]))

// maximum number of LODs per one geometric model (CryGeometry)
enum
{
    g_nMaxGeomLodLevels = 6
};




#endif

