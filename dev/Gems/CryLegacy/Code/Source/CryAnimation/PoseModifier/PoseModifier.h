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

#ifndef PoseModifier_h
#define PoseModifier_h

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS

#include <ICryAnimation.h>
#include <CryExtension/Impl/ClassWeaver.h>

namespace Serialization {
    class IArchive;
}

//

//struct ISkeletonPose;
//struct ISkeletonAnim;

//struct ICharacterModel;
//struct ICharacterModelSkeleton;

struct IAnimationPoseData;

//

class CPoseModifierStack
    : public IAnimationPoseModifier
{
public:
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IAnimationPoseModifier)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CPoseModifierStack, "AnimationPoseModifier_PoseModifierStack", 0xaf9efa2dfec04de4, 0xa1663950bde6a3c6)

public:
    void Clear() { m_modifiers.clear(); }
    bool Push(IAnimationPoseModifierPtr instance);

    // IAnimationPoseModifier
public:
    virtual bool Prepare(const SAnimationPoseModifierParams& params);
    virtual bool Execute(const SAnimationPoseModifierParams& params);
    virtual void Synchronize();

    void GetMemoryUsage(ICrySizer* pSizer) const { }

private:
    std::vector<IAnimationPoseModifierPtr> m_modifiers;
};

DECLARE_SMART_POINTERS(CPoseModifierStack);

//

class CPoseModifierSetup
    : public IAnimationSerializable
{
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IAnimationSerializable)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CPoseModifierSetup, "PoseModifierSetup", 0x18b8cca76db947cc, 0x84dd1f003e97cbee)

private:
    struct Entry
    {
        Entry()
            : enabled(true) { }

        IAnimationPoseModifierPtr instance;
        bool enabled;

        void Serialize(Serialization::IArchive& ar);
    };

public:
    bool Create(CPoseModifierSetup& setup);
    CPoseModifierStackPtr GetPoseModifierStack() { return m_pPoseModifierStack; }

private:
    bool CreateStack();

    // IAnimationSerializable
public:
    virtual void Serialize(Serialization::IArchive& ar);

private:
    std::vector<Entry> m_modifiers;
    CPoseModifierStackPtr m_pPoseModifierStack;
};

DECLARE_SMART_POINTERS(CPoseModifierSetup);

#endif // ENABLE_RUNTIME_POSE_MODIFIERS

#endif // PoseModifier_h
