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

#ifndef CRYINCLUDE_CRYANIMATION_POSEMODIFIER_FEETLOCK_H
#define CRYINCLUDE_CRYANIMATION_POSEMODIFIER_FEETLOCK_H
#pragma once

struct SFeetData
{
    QuatT m_WorldEndEffector;
    uint32 m_IsEndEffector;
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

class CFeetPoseStore
    : public IAnimationPoseModifier
{
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IAnimationPoseModifier)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CFeetPoseStore, "AnimationPoseModifier_FeetPoseStore", 0x4095cfb096b5494f, 0x864d3c007b71d31d)

    // IAnimationPoseModifier
public:
    virtual bool Prepare(const SAnimationPoseModifierParams& params) { return true; }
    virtual bool Execute(const SAnimationPoseModifierParams& params);
    virtual void Synchronize() { }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_pFeetData);
    }
public://private:
    SFeetData* m_pFeetData;
};

class CFeetPoseRestore
    : public IAnimationPoseModifier
{
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IAnimationPoseModifier)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CFeetPoseRestore, "AnimationPoseModifier_FeetPoseRestore", 0x90662f0ed05a4bf4, 0x8fb69924b5da2872)

    // IAnimationPoseModifier
public:
    virtual bool Prepare(const SAnimationPoseModifierParams& params) { return true; }
    virtual bool Execute(const SAnimationPoseModifierParams& params);
    virtual void Synchronize() { }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_pFeetData);
    }
public://private:
    SFeetData* m_pFeetData;
};

class CFeetLock
{
private:
    IAnimationPoseModifierPtr m_store;
    IAnimationPoseModifierPtr m_restore;

public:
    CFeetLock();

public:
    void Reset() {}
    IAnimationPoseModifier* Store() { return m_store.get(); }
    IAnimationPoseModifier* Restore() { return m_restore.get(); }

private:
    SFeetData m_FeetData[MAX_FEET_AMOUNT];
};

#endif // CRYINCLUDE_CRYANIMATION_POSEMODIFIER_FEETLOCK_H
