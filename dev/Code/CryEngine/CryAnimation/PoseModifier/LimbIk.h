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

#ifndef CRYINCLUDE_CRYANIMATION_POSEMODIFIER_LIMBIK_H
#define CRYINCLUDE_CRYANIMATION_POSEMODIFIER_LIMBIK_H
#pragma once

class CLimbIk
    : public IAnimationPoseModifier
{
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IAnimationPoseModifier)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CLimbIk, "AnimationPoseModifier_LimbIk", 0x3b00bbad5b9c4fa4, 0x97e9b720fcbc8839)

private:
    struct Setup
    {
        LimbIKDefinitionHandle setup;
        Vec3 targetPositionLocal;
    };

private:
    Setup m_setupsBuffer[2][16];

    Setup* m_pSetups;
    uint32 m_setupCount;

    Setup* m_pSetupsExecute;
    uint32 m_setupCountExecute;

public:
    void AddSetup(LimbIKDefinitionHandle setup, const Vec3& targetPositionLocal);

    // IAnimationPoseModifier
public:
    virtual bool Prepare(const SAnimationPoseModifierParams& params);
    virtual bool Execute(const SAnimationPoseModifierParams& params);
    virtual void Synchronize();

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

#endif // CRYINCLUDE_CRYANIMATION_POSEMODIFIER_LIMBIK_H
