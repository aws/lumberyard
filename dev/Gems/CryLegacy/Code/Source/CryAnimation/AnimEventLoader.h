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

#ifndef CRYINCLUDE_CRYANIMATION_ANIMEVENTLOADER_H
#define CRYINCLUDE_CRYANIMATION_ANIMEVENTLOADER_H
#pragma once

class CDefaultSkeleton;

namespace AnimEventLoader
{
    // loads the data from the animation event database file (.animevent) - this is usually
    // specified in the CHRPARAMS file.
    bool LoadAnimationEventDatabase(CDefaultSkeleton* pDefaultSkeleton, const char* pFileName);

    // Toggles preloading of particle effects
    void SetPreLoadParticleEffects(bool bPreLoadParticleEffects);
}

#endif // CRYINCLUDE_CRYANIMATION_ANIMEVENTLOADER_H
