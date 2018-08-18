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

#ifndef CRYINCLUDE_CRYANIMATION_CRYMODEFFMORPH_H
#define CRYINCLUDE_CRYANIMATION_CRYMODEFFMORPH_H
#pragma once

#include <CryCharMorphParams.h>

class CryModelAnimationContainer;

class CryModEffMorph
{
public:
    // advances the current time of the played animation and returns the blending factor by which this animation affects the bone pose
    void Tick (f32 fDeltaTime);

    // starts the morphing sequence
    void StartMorph (int nMorphTargetId, const CryCharMorphParams& rParams);

    // returns false when this morph target is inactive
    bool isActive() const;

    // returns the blending factor for this morph target
    f32 getBlending() const;

    // returns the morph target
    int getMorphTargetId () const;

    void setTime(f32 fTime) {m_fTime = fTime; }
    void setSpeed (f32 fSpeed) {m_Params.m_fSpeed = fSpeed; }
    void stop();

    f32 getTime() const {return m_fTime; }
    void freeze() {m_nFlags |= m_Params.FLAGS_FREEZE; }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(&m_Params, sizeof(m_Params));
        pSizer->AddObject(m_fTime);
        pSizer->AddObject(m_nMorphTargetId);
        pSizer->AddObject(m_nFlags);
    }
public:

    // the animation container that will answer all questions regarding the morph target
    //CryModelAnimationContainer* m_pAnimations;

    // the blend time
    CryCharMorphParams m_Params;
    // time of morphing
    f32 m_fTime;
    // morph target id
    int m_nMorphTargetId;
    unsigned m_nFlags; // the copy of the flags from m_Params
};

#endif // CRYINCLUDE_CRYANIMATION_CRYMODEFFMORPH_H
