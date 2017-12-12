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
#include <MCore/Source/Array.h>
#include <MCore/Source/Matrix4.h>
#include "BaseObject.h"


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;


    /**
     * The global space pose class.
     * This class is used for the global space controller blending support.
     * Global space controllers will output inside this pose.
     */
    class EMFX_API GlobalPose
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(GlobalPose, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_GLOBALPOSES);

    public:
        /**
         * The default creation method.
         */
        static GlobalPose* Create();

        /**
         * The extended creation method.
         * Automatically calls the SetNumTransforms method.
         * @param numTransforms The number of transformations to hold.
         */
        static GlobalPose* Create(uint32 numTransforms);

        /**
         * Initialize the global pose from an actor instance.
         * This will automatically allocate the appropriate number of transforms (equal to the number of nodes in the actor intstance).
         * Also it will init the global space matrices at the current actor instance's global space matrix values.
         * @param actorInstance The actor instance to initialize from.
         */
        void InitFromActorInstance(ActorInstance* actorInstance);

        /**
         * Blend the current actor instance's global space matrices into the ones of this pose.
         * @param actorInstance Use the global space matrices of this actor instance as start point.
         * @param weight The weight value, which must be in range of [0..1].
         * @param nodeMask The array of booleans that specifies which nodes we have to perform a blend for and which nodes we can skip.
         */
        void BlendAndUpdateActorMatrices(ActorInstance* actorInstance, float weight, const MCore::Array<bool>* nodeMask);

        /**
         * Copy the transformations into another pose.
         * @param destPose The destination pose to copy the current transforms into.
         */
        void CopyTo(GlobalPose* destPose);

        /**
         * Remove all transforms from memory.
         * This will result in the GetNumTransforms() method to return 0.
         */
        void Release();

        /**
         * Allocate space for a given number of transformations.
         * @param numTransforms The number of transformations to allocate space for.
         */
        void SetNumTransforms(uint32 numTransforms);

        /**
         * Get the number of transformations stored inside this pose.
         * @result The number of stored transformations.
         */
        MCORE_INLINE uint32 GetNumTransforms() const                { return mGlobalMatrices.GetLength(); }

        /**
         * Get the global space matrices.
         * @result A pointer to the global space matrix array data. The number of matrices equals the value returned by GetNumTransforms().
         */
        MCORE_INLINE MCore::Matrix* GetGlobalMatrices()             { return mGlobalMatrices.GetPtr(); }

        /**
         * Get the global space scale values.
         * @result A pointer to the scale values. The number of scale values equals the value returned by GetNumTransforms().
         */
        MCORE_INLINE MCore::Vector3* GetScales()                    { return mScales.GetPtr(); }

    protected:
        MCore::Array< MCore::Matrix >   mGlobalMatrices;        /**< The global space matrices. */
        MCore::Array< MCore::Vector3 >  mScales;            /**< The global space scale values (not always stored). */

        /**
         * The constructor.
         * Don't forget to call the SetNumTransforms method later on.
         */
        GlobalPose();

        /**
         * The extended constructor.
         * Automatically calls the SetNumTransforms method.
         * @param numTransforms The number of transformations to hold.
         */
        GlobalPose(uint32 numTransforms);

        /**
         * The destructor.
         */
        ~GlobalPose();
    };
}   // namespace EMotionFX
