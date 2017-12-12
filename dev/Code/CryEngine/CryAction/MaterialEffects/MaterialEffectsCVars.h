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

// Description : CVars for the MaterialEffects subsystem


#ifndef CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MATERIALEFFECTSCVARS_H
#define CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MATERIALEFFECTSCVARS_H
#pragma once

struct ICVar;

class CMaterialEffectsCVars
{
public:

    float mfx_ParticleImpactThresh;    // "Impact threshold for particle effects. Default: 1.38" );
    float mfx_SoundImpactThresh;       //"Impact threshold for sound effects. Default: 1.5" );
    float mfx_RaisedSoundImpactThresh; // "Impact threshold for sound effects if we're rolling. Default: 3.5" );
    int   mfx_Debug;                   // "Turns on MaterialEffects debug messages." );
    int   mfx_DebugFlowGraphFX;             // "Turns on MaterialEffects FlowGraph FX debbug messages." );
    int     mfx_DebugVisual;
    ICVar* mfx_DebugVisualFilter;
    int   mfx_Enable;
    int   mfx_EnableFGEffects;
    int   mfx_EnableAttachedEffects;
    int   mfx_SerializeFGEffects;      // "Serialize Material Effect based effects. Default: On"
    float mfx_pfx_maxDist;
    float mfx_pfx_maxScale;
    float mfx_pfx_minScale;

    // Timeout avoids playing effects too often.
    // Effects should still be played at lower velocities, but not too often
    // as they cannot be distinguished when played too close together and are slowing down the system.
    // Therefore a tweakeable timeout
    float   mfx_Timeout;

    static __inline CMaterialEffectsCVars& Get()
    {
        assert (s_pThis != 0);
        return *s_pThis;
    }

private:
    friend class CCryAction; // Our only creator

    CMaterialEffectsCVars(); // singleton stuff
    ~CMaterialEffectsCVars();
    CMaterialEffectsCVars(const CMaterialEffectsCVars&);
    CMaterialEffectsCVars& operator= (const CMaterialEffectsCVars&);

    static CMaterialEffectsCVars* s_pThis;
};

#endif // CRYINCLUDE_CRYACTION_MATERIALEFFECTS_MATERIALEFFECTSCVARS_H
