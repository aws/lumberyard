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

#pragma once

// include required headers
#include "EMotionFXConfig.h"
#include "Transform.h"
#include <MCore/Source/AlignedArray.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/Quaternion.h>


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class Actor;
    class MotionInstance;
    class Node;
    class TransformData;
    class Skeleton;


    /**
     *
     *
     */
    class EMFX_API Pose
    {
        MCORE_MEMORYOBJECTCATEGORY(Pose, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_POSE);

    public:
        enum
        {
            FLAG_LOCALTRANSFORMREADY    = 1 << 0,
            FLAG_GLOBALTRANSFORMREADY   = 1 << 1
        };

        Pose();
        Pose(const Pose& other);
        ~Pose();

        void Clear(bool clearMem = true);
        void ClearFlags(uint8 newFlags = 0);

        void InitFromPose(const Pose* sourcePose);
        void InitFromBindPose(ActorInstance* actorInstance);
        void InitFromBindPose(Actor* actor);

        void LinkToActorInstance(ActorInstance* actorInstance, uint8 initialFlags = 0);
        void LinkToActor(Actor* actor, uint8 initialFlags = 0, bool clearAllFlags = true);
        void SetNumTransforms(uint32 numTransforms);

        void ApplyMorphWeightsToActorInstance();
        void ZeroMorphWeights();

        void UpdateAllLocalTranforms();
        void UpdateAllGlobalTranforms();
        void ForceUpdateFullLocalPose();
        void ForceUpdateFullGlobalPose();

        const Transform& GetLocalTransform(uint32 nodeIndex) const;
        const Transform& GetGlobalTransform(uint32 nodeIndex) const;
        Transform GetGlobalTransformInclusive(uint32 nodeIndex) const;
        void GetLocalTransform(uint32 nodeIndex, Transform* outResult) const;
        void GetGlobalTransform(uint32 nodeIndex, Transform* outResult) const;
        void GetGlobalTransformInclusive(uint32 nodeIndex, Transform* outResult) const;

        void SetLocalTransform(uint32 nodeIndex, const Transform& newTransform, bool invalidateGlobalTransforms = true);
        void SetGlobalTransform(uint32 nodeIndex, const Transform& newTransform, bool invalidateChildGlobalTransforms = true);
        void SetGlobalTransformInclusive(uint32 nodeIndex, const Transform& newTransform, bool invalidateChildGlobalTransforms = true);

        void UpdateGlobalTransform(uint32 nodeIndex) const;
        void UpdateLocalTransform(uint32 nodeIndex) const;

        void CompensateForMotionExtraction(EMotionExtractionFlags motionExtractionFlags=(EMotionExtractionFlags)0);
        void CompensateForMotionExtractionDirect(EMotionExtractionFlags motionExtractionFlags=(EMotionExtractionFlags)0);

        void InvalidateAllLocalTransforms();
        void InvalidateAllGlobalTransforms();
        void InvalidateAllLocalAndGlobalTransforms();

        Transform CalcTrajectoryTransform() const;

        MCORE_INLINE const Transform* GetLocalTransforms() const                                    { return mLocalTransforms.GetReadPtr(); }
        MCORE_INLINE const Transform* GetGlobalTransforms() const                                   { return mGlobalTransforms.GetReadPtr(); }
        MCORE_INLINE uint32 GetNumTransforms() const                                                { return mLocalTransforms.GetLength(); }
        MCORE_INLINE ActorInstance* GetActorInstance() const                                        { return mActorInstance; }
        MCORE_INLINE Actor* GetActor() const                                                        { return mActor; }
        MCORE_INLINE Skeleton* GetSkeleton() const                                                  { return mSkeleton; }

        MCORE_INLINE Transform& GetLocalTransformDirect(uint32 nodeIndex)                           { return mLocalTransforms[nodeIndex]; }
        MCORE_INLINE Transform& GetGlobalTransformDirect(uint32 nodeIndex)                          { return mGlobalTransforms[nodeIndex]; }
        MCORE_INLINE const Transform& GetLocalTransformDirect(uint32 nodeIndex) const               { return mLocalTransforms[nodeIndex]; }
        MCORE_INLINE const Transform& GetGlobalTransformDirect(uint32 nodeIndex) const              { return mGlobalTransforms[nodeIndex]; }
        MCORE_INLINE void SetLocalTransformDirect(uint32 nodeIndex, const Transform& transform)     { mLocalTransforms[nodeIndex]  = transform; mFlags[nodeIndex] |= FLAG_LOCALTRANSFORMREADY; }
        MCORE_INLINE void SetGlobalTransformDirect(uint32 nodeIndex, const Transform& transform)    { mGlobalTransforms[nodeIndex] = transform; mFlags[nodeIndex] |= FLAG_GLOBALTRANSFORMREADY; }
        MCORE_INLINE void InvalidateLocalTransform(uint32 nodeIndex)                                { mFlags[nodeIndex] &= ~FLAG_LOCALTRANSFORMREADY; }
        MCORE_INLINE void InvalidateGlobalTransform(uint32 nodeIndex)                               { mFlags[nodeIndex] &= ~FLAG_GLOBALTRANSFORMREADY; }

        MCORE_INLINE void SetMorphWeight(uint32 index, float weight)                                { mMorphWeights[index] = weight; }
        MCORE_INLINE float GetMorphWeight(uint32 index) const                                       { return mMorphWeights[index]; }
        MCORE_INLINE uint32 GetNumMorphWeights() const                                              { return mMorphWeights.GetLength(); }
        void ResizeNumMorphs(uint32 numMorphTargets);

        /**
         * Blend this pose into a specified destination pose.
         * @param destPose The destination pose to blend into.
         * @param weight The weight value to use.
         * @param instance The motion instance settings to use.
         */
        void Blend(const Pose* destPose, float weight, const MotionInstance* instance);

        /**
         * Blend the transforms for all enabled nodes in the actor instance.
         * @param destPose The destination pose to blend into.
         * @param weight The weight value to use, which must be in range of [0..1], where 1.0 is the dest pose.
         */
        void Blend(const Pose* destPose, float weight);

        /**
         * Additively blend the transforms for all enabled nodes in the actor instance.
         * You can see this as: thisPose += destPose * weight.
         * @param destPose The destination pose to blend into, additively, so displacing the current pose.
         * @param weight The weight value to use, which must be in range of [0..1], where 1.0 is the dest pose.
         */
        void BlendAdditiveUsingBindPose(const Pose* destPose, float weight);

        /**
         * Blend this pose into a specified destination pose.
         * @param destPose The destination pose to blend into.
         * @param weight The weight value to use.
         * @param instance The motion instance settings to use.
         * @param outPose the output pose, in which we will write the result.
         */
        void Blend(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose);

        Pose& PreMultiply(const Pose& other);
        Pose& Multiply(const Pose& other);
        Pose& MultiplyInverse(const Pose& other);

        void Zero();
        void NormalizeQuaternions();
        void Sum(const Pose* other, float weight);

        Pose& MakeRelativeTo(const Pose& other);
        Pose& MakeAdditive(const Pose& refPose);
        Pose& ApplyAdditive(const Pose& additivePose);
        Pose& ApplyAdditive(const Pose& additivePose, float weight);

        Pose& operator=(const Pose& other);

        MCORE_INLINE uint8 GetFlags(uint32 nodeIndex) const         { return mFlags[nodeIndex]; }
        MCORE_INLINE void SetFlags(uint32 nodeIndex, uint8 flags)   { mFlags[nodeIndex] = flags; }

    private:
        mutable MCore::AlignedArray<Transform, 16>  mLocalTransforms;
        mutable MCore::AlignedArray<Transform, 16>  mGlobalTransforms;
        mutable MCore::AlignedArray<uint8, 16>      mFlags;
        MCore::AlignedArray<float, 16>              mMorphWeights;      /**< The morph target weights. */
        ActorInstance*                              mActorInstance;
        Actor*                                      mActor;
        Skeleton*                                   mSkeleton;

        void RecursiveInvalidateGlobalTransforms(Actor* actor, uint32 nodeIndex);

        /**
         * Perform a non-mixed blend into the specified destination pose.
         * @param destPose The destination pose to blend into.
         * @param weight The weight value to use.
         * @param instance The motion instance settings to use.
         * @param outPose the output pose, in which we will write the result.
         */
        void BlendNonMixed(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose);

        /**
         * Perform mixed blend into the specified destination pose.
         * @param destPose The destination pose to blend into.
         * @param weight The weight value to use.
         * @param instance The motion instance settings to use.
         * @param outPose the output pose, in which we will write the result.
         */
        void BlendMixed(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose);

        /**
         * Blend a single local space transform into another transform, using additive blending.
         * This will basically do like: outTransform = source + (dest - baseTransform) * weight.
         * @param baseTransform The base pose local transform, used to calculate the additive amount.
         * @param source The source transformation.
         * @param dest The destination transformation.
         * @param weight The weight value, which must be in range of [0..1].
         * @param outTransform The transform object to output the result in.
         */
        void BlendTransformAdditiveUsingBindPose(const Transform& baseTransform, const Transform& source, const Transform& dest, float weight, Transform* outTransform);

        /**
         * Blend a single local space transform into another transform, including weight check.
         * This weight check will check if the weight is 0 or 1, and perform specific optimizations in such cases.
         * @param source The source transformation.
         * @param dest The destination transformation.
         * @param weight The weight value, which must be in range of [0..1].
         * @param outTransform The transform object to output the result in.
         */
        void BlendTransformWithWeightCheck(const Transform& source, const Transform& dest, float weight, Transform* outTransform);
    };
}   // namespace EMotionFX
