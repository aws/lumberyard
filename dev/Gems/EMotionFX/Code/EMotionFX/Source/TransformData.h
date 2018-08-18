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

// include the required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"
#include "Pose.h"
//#include "Node.h"
#include <MCore/Source/Matrix4.h>


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;

    /**
     * The transformation data class.
     * This class holds all transformation data for each node.
     * This includes local space transforms, local space matrices as well as global space matrices.
     * If for example you wish to get the global space matrices for all nodes, to be used for rendering, you will have to use this class.
     */
    class EMFX_API TransformData
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Some flags per node, that allow specific optimizations inside EMotion FX.
         */
        enum ENodeFlags
        {
            FLAG_HASSCALE   = 1 << 0,   /**< Has the node a scale factor? */
        };

        //---------------------------------------------------------------------

        /**
         * The creation method.
         */
        static TransformData* Create();

        /**
         * Initialize the transformation data for a given ActorInstance object.
         * This will allocate data for the number of nodes in the actor.
         * You can call this multiple times if needed.
         * @param actorInstance The actor instance to initialize for.
         */
        void InitForActorInstance(ActorInstance* actorInstance);

        /**
         * Release all allocated memory.
         */
        void Release();

        /**
         * Get the global space matrices (including the global actor transformation taken into account).
         * The size of the returned array is equal to the amount of nodes in the actor or the value returned by GetNumTransforms()
         * @result The array of global space matrices.
         */
        MCORE_INLINE MCore::Matrix* GetGlobalInclusiveMatrices()                                        { return mGlobalMatrices; }

        /**
         * Get the global space matrices (including the global actor transformation taken into account), in read-only (const) mode.
         * The size of the returned array is equal to the amount of nodes in the actor or the value returned by GetNumTransforms()
         * @result The array of global space matrices.
         */
        MCORE_INLINE const MCore::Matrix* GetGlobalInclusiveMatrices() const                            { return mGlobalMatrices; }

        /**
         * Get the local space space matrices.
         * The size of the returned array is equal to the amount of nodes in the actor or the value returned by GetNumTransforms()
         * @result The array of local space matrices.
         */
        MCORE_INLINE MCore::Matrix* GetLocalMatrices()                                                  { return mLocalMatrices; }

        /**
         * Get the local space space matrices, in read-only (const) mode.
         * The size of the returned array is equal to the amount of nodes in the actor or the value returned by GetNumTransforms()
         * @result The array of local space matrices.
         */
        MCORE_INLINE const MCore::Matrix* GetLocalMatrices() const                                      { return mLocalMatrices; }

        MCORE_INLINE Pose* GetBindPose() const                                                          { return mBindPose; }
        MCORE_INLINE const Pose* GetCurrentPose() const                                                 { return &mPose; }
        MCORE_INLINE Pose* GetCurrentPose()                                                             { return &mPose; }

        /**
         * Reset the local space transform of a given node to its bind pose local space transform.
         * @param nodeIndex The node number, which must be in range of [0..GetNumTransforms()-1].
         */
        void ResetToBindPoseTransformation(uint32 nodeIndex)                                            { mPose.SetLocalTransform(nodeIndex, mBindPose->GetLocalTransform(nodeIndex)); }

        /**
         * Reset all local space transforms to the local space transforms of the bind pose.
         */
        void ResetToBindPoseTransformations()
        {
            for (uint32 i = 0; i < mNumTransforms; ++i)
            {
                mPose.SetLocalTransform(i, mBindPose->GetLocalTransform(i));
            }
        }

        /**
         * Get the global space position for a given node.
         * @param nodeIndex The node number, which must be in range of [0..GetNumTransforms()-1].
         * @result The global space position.
         */
        MCORE_INLINE AZ::Vector3 GetGlobalPosition(uint32 nodeIndex) const                              { return mGlobalMatrices[nodeIndex].GetTranslation(); }

        /**
         * Get the global space transformation matrix for a given node.
         * @param nodeIndex The node number, which must be in range of [0..GetNumTransforms()-1].
         * @result The global space transformation matrix.
         */
        MCORE_INLINE const MCore::Matrix& GetGlobalInclusiveMatrix(uint32 nodeIndex)                    { return mGlobalMatrices[nodeIndex]; }

        /**
         * Get the local space transformation matrix for a given node.
         * @param nodeIndex The node number, which must be in range of [0..GetNumTransforms()-1].
         * @result The local space transformation matrix.
         */
        MCORE_INLINE const MCore::Matrix& GetLocalMatrix(uint32 nodeIndex)                              { return mLocalMatrices[nodeIndex]; }

        EMFX_SCALECODE
        (
            void SetBindPoseLocalScaleInherit(uint32 nodeIndex, const AZ::Vector3 & scale);
            void SetBindPoseLocalScale(uint32 nodeIndex, const AZ::Vector3 & scale);
        )

        MCORE_INLINE ActorInstance * GetActorInstance() const            {
            return mPose.GetActorInstance();
        }
        MCORE_INLINE uint32 GetNumTransforms() const                    { return mNumTransforms; }

        void MakeBindPoseTransformsUnique();
        void UpdateNodeFlags();

        /**
         * Get the node optimization flags.
         * The size of the returned array is equal to the amount of nodes in the actor or the value returned by GetNumTransforms()
         * @result The array of node optimization flags.
         */
        MCORE_INLINE ENodeFlags* GetNodeFlags()                                                         { return mFlags; }

        /**
         * Get the node optimization flags, in read-only (const) mode.
         * The size of the returned array is equal to the amount of nodes in the actor or the value returned by GetNumTransforms()
         * @result The array of node optimization flags.
         */
        MCORE_INLINE const ENodeFlags* GetNodeFlags() const                                             { return mFlags; }

        /**
         * Reset all node optimization flags.
         */
        MCORE_INLINE void ResetNodeFlags()                                                              { MCore::MemSet(mFlags, 0, sizeof(ENodeFlags) * mNumTransforms); }

        /**
         * Get the node optimization flags for a given node.
         * @param nodeIndex The node number, which must be in range of [0..GetNumTransforms()-1].
         * @result The currently used flags for the given node.
         */
        MCORE_INLINE ENodeFlags GetNodeFlags(uint32 nodeIndex) const                                    { return mFlags[nodeIndex]; }

        /**
         * Set the node flags for a given node.
         * @param nodeIndex The node number, which must be in range of [0..GetNumTransforms()-1].
         * @param flags The flags to use for this node.
         */
        MCORE_INLINE void SetNodeFlags(uint32 nodeIndex, ENodeFlags flags)                              { mFlags[nodeIndex] = flags; }

        /**
         * Enable or disable specific node optimization flags for a given node.
         * @param nodeIndex The node number, which must be in range of [0..GetNumTransforms()-1].
         * @param flags The flags to enable or disable.
         * @param enable Set to true to enable the specified flags, or set to false to disable them.
         */
        MCORE_INLINE void SetNodeFlag(uint32 nodeIndex, ENodeFlags flags, bool enable)
        {
            uint32 curFlags = (uint32)mFlags[nodeIndex];

            if (enable)
            {
                curFlags |= (uint32)flags;
            }
            else
            {
                curFlags &= (uint32) ~flags;
            }

            mFlags[nodeIndex] = (ENodeFlags)curFlags;
        }


        void SetNumMorphWeights(uint32 numMorphWeights);


    private:
        Pose            mPose;                  /**< The current pose, used to calculate the local and global matrices. */
        Pose*           mBindPose;              /**< The bind pose, which can be unique or point to the bind pose in the actor. */
        MCore::Matrix*  mGlobalMatrices;        /**< The global space matrices, inclusive the global transformation of the actor instance. */
        MCore::Matrix*  mLocalMatrices;         /**< The local space matrices (transforms relative to the parent node, non-inclusive of the actor instance global transformation). */
        ENodeFlags*     mFlags;                 /**< The flags for each node. */
        uint32          mNumTransforms;         /**< The number of transforms, which is equal to the number of nodes in the linked actor instance. */
        bool            mHasUniqueBindPose;     /**< Do we have a unique bind pose (when set to true) or do we use the one from the Actor object (when set to false)? */

        /**
         * The constructor.
         */
        TransformData();

        /**
         * The destructor.
         */
        ~TransformData();
    };
}   // namespace EMotionFX

