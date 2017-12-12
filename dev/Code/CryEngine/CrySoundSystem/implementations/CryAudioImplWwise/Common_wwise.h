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
#define WWISE_IMPL_INFO_STRING "Wwise " AK_WWISESDK_VERSIONNAME

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

#if (AK_WWISESDK_VERSION_MAJOR <= 2015)

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline void ATLTransformToAKTransform(const SATLWorldPosition& atlTransform, AkListenerPosition& akListenerPosition)
    {
        akListenerPosition.Position = EngineVec3ToAkVector(atlTransform.mPosition.GetColumn3());
        akListenerPosition.OrientationFront = EngineVec3ToAkVector(atlTransform.mPosition.GetColumn1().GetNormalized());
        akListenerPosition.OrientationTop = EngineVec3ToAkVector(atlTransform.mPosition.GetColumn2().GetNormalized());
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline void ATLTransformToAKTransform(const SATLWorldPosition& atlPosition, AkSoundPosition& akSoundPosition)
    {
        akSoundPosition.Position = EngineVec3ToAkVector(atlPosition.mPosition.GetColumn3());
        akSoundPosition.Orientation = EngineVec3ToAkVector(atlPosition.mPosition.GetColumn1().GetNormalized());
    }

#else

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline void ATLTransformToAKTransform(const SATLWorldPosition& atlTransform, AkTransform& akTransform)
    {
        akTransform.Set(
            EngineVec3ToAkVector(atlTransform.mPosition.GetColumn3()),
            EngineVec3ToAkVector(atlTransform.mPosition.GetColumn1().GetNormalized()),  // Wwise SDK requires that the Orientation vectors
            EngineVec3ToAkVector(atlTransform.mPosition.GetColumn2().GetNormalized())   // are normalized prior to sending to the apis.
            );
    }

#endif // AK_WWISESDK_VERSION_MAJOR

} // namespace Audio
