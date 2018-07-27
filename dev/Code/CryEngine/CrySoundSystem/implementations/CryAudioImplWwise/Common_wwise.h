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

#include "AK/SoundEngine/Common/AkTypes.h"
#include "AK/AkWwiseSDKVersion.h"
#include <IAudioSystem.h>

#define WWISE_IMPL_BASE_PATH "sounds/wwise/"
#define WWISE_IMPL_BANK_PATH "" // No further sub folders necessary.
#define WWISE_IMPL_BANK_FULL_PATH WWISE_IMPL_BASE_PATH WWISE_IMPL_BANK_PATH

#if defined(WWISE_LTX)
    #define WWISE_FLAVOR_STRING     "Wwise LTX"
#else
    #define WWISE_FLAVOR_STRING     "Wwise"
#endif // WWISE_LTX

#define WWISE_IMPL_VERSION_STRING   WWISE_FLAVOR_STRING " " AK_WWISESDK_VERSIONNAME

#define ASSERT_WWISE_OK(x) (AKASSERT((x) == AK_Success))
#define IS_WWISE_OK(x)     ((x) == AK_Success)

namespace Audio
{
    // wwise-specific helper functions
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline AkVector EngineVec3ToAkVector(const Vec3& vec3)
    {
        // swizzle Y <--> Z
        AkVector akVec;
        akVec.X = vec3.x;
        akVec.Y = vec3.z;
        akVec.Z = vec3.y;
        return akVec;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline void ATLTransformToAKTransform(const SATLWorldPosition& atlTransform, AkTransform& akTransform)
    {
        akTransform.Set(
            EngineVec3ToAkVector(atlTransform.mPosition.GetColumn3()),
            EngineVec3ToAkVector(atlTransform.mPosition.GetColumn1().GetNormalized()),  // Wwise SDK requires that the Orientation vectors
            EngineVec3ToAkVector(atlTransform.mPosition.GetColumn2().GetNormalized())   // are normalized prior to sending to the apis.
            );
    }

} // namespace Audio
