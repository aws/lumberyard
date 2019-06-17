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

#include <AzFramework/Physics/TouchBendingBus.h>

class CVegetation;
struct CStatObj;
struct IFoliage;

namespace Physics
{
    class ITouchBendingCallback;
    struct SpineTree;
    struct TouchBendingTriggerHandle;
    struct TouchBendingSkeletonHandle;
};

namespace AZ
{
    /// This is a Lazy Singleton by StaticInstance<TouchBendingCVegetationAgent>
    /// It works on behalf of CVegetation and CStatObjFoliage to provide
    /// support for the new TouchBending Gem.
    class TouchBendingCVegetationAgent
        : public Cry3DEngineBase,
          public Physics::ITouchBendingCallback
    {
    public:
        TouchBendingCVegetationAgent();
        virtual ~TouchBendingCVegetationAgent();

        static TouchBendingCVegetationAgent* GetInstance();

        ///Before calling this method, the caller should validate that \p vegetationNode
        ///is touch bendable.
        ///Equivalent to CVegeation::Physicalize(). A Physics::TouchBendingTriggerHandle will be created
        ///by the TocuhBending Gem and initially it will only instantiate a simple trigger box. Only when
        ///this box is touched by a collider, then a PhysicalizedSkeleton will be created by the TouchBending Gem and a corresponding
        ///CStatObjFoliage will be created in the Engine.
        ///This method is called each time a Vegetation object is painted with the Rollup Bar (LEGACY) UI.
        ///Also, if the Dynamic Vegetation Gem is being used, this method will be called as each CVegetation
        ///node appears in the camera frustum.
        bool Physicalize(CVegetation* vegetationNode);

        /// Equivalent to CVegeation::Dehysicalize().
        ///This method is called each time a Vegetation object is removed with the Rollup Bar (LEGACY) UI.
        ///Also, if the Dynamic Vegetation Gem is being used, this method will be called as each CVegetation
        ///node disappears from the camera frustum.
        void Dephysicalize(CVegetation* vegetationNode);

        //Physics::ITouchBendingCallback START
        Physics::SpineTreeIDType CheckDistanceToCamera(const void* privateData) override;
        bool BuildSpineTree(const void* privateData, Physics::SpineTreeIDType spineTreeId, Physics::SpineTree& spineTreeOut) override;
        void OnPhysicalizedTouchBendingSkeleton(const void* privateData, Physics::TouchBendingSkeletonHandle* skeletonHandle) override;
        //Physics::ITouchBendingCallback END

        //Used by CStatObjFoliage START
        void OnFoliageDestroyed(IFoliage* pStatObjFoliage);
        void UpdateFoliage(IFoliage* pStatObjFoliage, float dt, const CCamera& rCamera);
        //Used by CStatObjFoliage END

    private:
        /// Returns true if something answers the call on the TouchBendingBus. Otherwise
        /// returns false and CVegetation should default to CryPhysics. 
        bool IsTouchBendingEnabled();

        /** @brief Called internally upon TouchBendingSkeletonHandle object creation by TouchBending Gem.
         *
         *  Creates a new CStatObjFoliage that corresponds to a new TouchBendingSkeletonHandle object created by the
         *  TouchBending Gem.
         * 
         *  @param pStatObj The CryEngine archetype of the set of Spines used to define the Tree.
         *  @param worldAabb AABB in world coordinates of the CVegetation object.
         *  @param pIRes Reference to the CStatObjFoliage object that the agent will create and that
         *         will be attached to the CVegetation render node.
         *  @param lifeTime How long the CStatObjFoliage should remain alive when not being touched.
         *         Its value is defined by CVar e_FoliageBranchesTimeout.
         * @param skeletonHandle The opaque skeleton handle as created by TouchBending Gem.
         *  @returns TRUE if a new CStatObjFoliage was created. Otherwise returns false.
         */
        bool OnPhysicalizedFoliage(CStatObj* pStatObj, const AABB& worldAabb, IFoliage*& pIRes, float lifeTime, Physics::TouchBendingSkeletonHandle* skeletonHandle);
        

        /** @brief Updates the m_pSkinningTransformations array of CStatObjFoliage with new bone positions.
         *
         *  This method is called within the stack of the public method UpdateFoliage().
         *  Behaves similar to CStatObjFoliage:ComputeSkinningTransformations().
         *  The idea is to populate the m_pSkinningTransformations array with Top and Bottom position
         *  of each bone of the physicalized skeleton.
         *
         *  @param pFoliage The CStatObjFoliage used by the Engine as the PhysicalizedSkeleton instance.
         *  @param boneCount How many bones are present inside the Tree instance of the \p handle.
         *  @param nList NOT AN INTUITIVE name, but its value is ALWAYS zero. m_pSkinningTransformations[nList]
         *         is the array where the current joint positions should be written to.
         *  @returns void.
         */
        void ComputeSkinningTransformations(IFoliage* pFoliage, uint32 boneCount, uint32 nList);

        /// Cached flag where this class records if the TouchBending Gem is available or not.
        bool m_isEnabled;

    };
}; //namespace AZ
