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

#pragma once

#include <platform.h>
#include <AzCore/Debug/Trace.h>
#include <ProjectDefines.h>
#include <StlUtils.h>

#if !defined(_RELEASE)
    // Enable logging for non-Release builds.
    #define ENABLE_AUDIO_LOGGING
    // Production code is enabled in non-Release builds.
    #define INCLUDE_WWISE_IMPL_PRODUCTION_CODE
#endif // !_RELEASE

#include <CryAudioImplWwise_Traits_Platform.h>

#include <AudioAllocators.h>
#include <AudioLogger.h>

namespace Audio
{
    extern CAudioLogger g_audioImplLogger_wwise;
} // namespace Audio


// Secondary Memory Allocation Pool
#if AZ_TRAIT_CRYAUDIOIMPLWWISE_PROVIDE_IMPL_SECONDARY_POOL

#include <CryPool/PoolAlloc.h>

typedef NCryPoolAlloc::CThreadSafe<NCryPoolAlloc::CBestFit<NCryPoolAlloc::CReferenced<NCryPoolAlloc::CMemoryDynamic, 4 * 1024, true>, NCryPoolAlloc::CListItemReference> > tMemoryPoolReferenced;

namespace Audio
{
    extern tMemoryPoolReferenced g_audioImplMemoryPoolSecondary_wwise;
} // namespace Audio

#endif // AZ_TRAIT_CRYAUDIOIMPLWWISE_PROVIDE_IMPL_SECONDARY_POOL

