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

#include <PhysX_precompiled.h>
#include <Source/Collision.h>

namespace PhysX
{
    namespace Collision
    {
        // Combines two 32 bit values into 1 64 bit
        AZ::u64 Combine(AZ::u32 word0, AZ::u32 word1)
        {
            return (AZ::u64)word0 << 32 | word1;
        }

        physx::PxFilterFlags DefaultFilterShader(
            physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
            physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
            physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
        {
            const AZ::u64 layer0 = Combine(filterData0.word0, filterData0.word1);
            const AZ::u64 layer1 = Combine(filterData1.word0, filterData1.word1);
            const AZ::u64 group0 = Combine(filterData0.word2, filterData0.word3);
            const AZ::u64 group1 = Combine(filterData1.word2, filterData1.word3);
            const bool shouldCollide = ((layer0 & group1) && (layer1 & group0));
            if (!shouldCollide)
            {
                return physx::PxFilterFlag::eSUPPRESS;
            }

            // let triggers through
            if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
            {
                pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
                return physx::PxFilterFlag::eDEFAULT;
            }

            //If any of the actors is in the TouchBend layer then we are not interested
            //in contact data, nor interested in eNOTIFY_* callbacks.
            const AZ::u64 touchBendLayerMask = Physics::CollisionLayer::TouchBend.GetMask();
            if (layer0 == touchBendLayerMask || layer1 == touchBendLayerMask)
            {
                pairFlags = physx::PxPairFlag::eSOLVE_CONTACT |
                    physx::PxPairFlag::eDETECT_DISCRETE_CONTACT;
                return physx::PxFilterFlag::eDEFAULT;
            }

            // generate contacts for all that were not filtered above
            pairFlags =
                physx::PxPairFlag::eCONTACT_DEFAULT |
                physx::PxPairFlag::eNOTIFY_TOUCH_FOUND |
                physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
                physx::PxPairFlag::eNOTIFY_TOUCH_LOST |
                physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

            // generate callbacks for collisions between kinematic and dynamic objects
            if (physx::PxFilterObjectIsKinematic(attributes0) != physx::PxFilterObjectIsKinematic(attributes1))
            {
                return physx::PxFilterFlag::eCALLBACK;
            }

            return physx::PxFilterFlag::eDEFAULT;

        }

        physx::PxFilterFlags DefaultFilterShaderCCD(
            physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
            physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
            physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
        {
            const AZ::u64 layer0 = Combine(filterData0.word0, filterData0.word1);
            const AZ::u64 layer1 = Combine(filterData1.word0, filterData1.word1);
            const AZ::u64 group0 = Combine(filterData0.word2, filterData0.word3);
            const AZ::u64 group1 = Combine(filterData1.word2, filterData1.word3);
            const bool shouldCollide = ((layer0 & group1) && (layer1 & group0));
            if (!shouldCollide)
            {
                return physx::PxFilterFlag::eSUPPRESS;
            }

            // let triggers through
            if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
            {
                pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT |
                    physx::PxPairFlag::eNOTIFY_TOUCH_CCD;
                return physx::PxFilterFlag::eDEFAULT;
            }

            //If any of the actors is in the TouchBend layer then we are not interested
            //in contact data, nor interested in eNOTIFY_* callbacks.
            const AZ::u64 touchBendLayerMask = Physics::CollisionLayer::TouchBend.GetMask();
            if (layer0 == touchBendLayerMask || layer1 == touchBendLayerMask)
            {
                pairFlags = physx::PxPairFlag::eSOLVE_CONTACT |
                    physx::PxPairFlag::eDETECT_DISCRETE_CONTACT |
                    physx::PxPairFlag::eDETECT_CCD_CONTACT;
                return physx::PxFilterFlag::eDEFAULT;
            }

            // generate contacts for all that were not filtered above
            pairFlags =
                physx::PxPairFlag::eCONTACT_DEFAULT |
                physx::PxPairFlag::eNOTIFY_TOUCH_FOUND |
                physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
                physx::PxPairFlag::eNOTIFY_TOUCH_LOST |
                physx::PxPairFlag::eNOTIFY_TOUCH_CCD |
                physx::PxPairFlag::eNOTIFY_CONTACT_POINTS |
                physx::PxPairFlag::eDETECT_CCD_CONTACT |
                physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

            // generate callbacks for collisions between kinematic and dynamic objects
            if (physx::PxFilterObjectIsKinematic(attributes0) != physx::PxFilterObjectIsKinematic(attributes1))
            {
                return physx::PxFilterFlag::eCALLBACK;
            }

            return physx::PxFilterFlag::eDEFAULT;
        }

        physx::PxFilterData CreateFilterData(const Physics::CollisionLayer& layer, const Physics::CollisionGroup& group)
        {
            physx::PxFilterData data;
            SetLayer(layer, data);
            SetGroup(group, data);
            return data;
        }

        void SetLayer(const Physics::CollisionLayer& layer, physx::PxFilterData& filterData)
        {
            filterData.word0 = (physx::PxU32)(layer.GetMask() >> 32);
            filterData.word1 = (physx::PxU32)(layer.GetMask());
        }

        void SetGroup(const Physics::CollisionGroup& group, physx::PxFilterData& filterData)
        {
            filterData.word2 = (physx::PxU32)(group.GetMask() >> 32);
            filterData.word3 = (physx::PxU32)(group.GetMask());
        }

        bool ShouldCollide(const physx::PxFilterData& filterData0, const physx::PxFilterData& filterData1)
        {
            AZ::u64 layer0 = Combine(filterData0.word0, filterData0.word1);
            AZ::u64 layer1 = Combine(filterData1.word0, filterData1.word1);
            AZ::u64 group0 = Combine(filterData0.word2, filterData0.word3);
            AZ::u64 group1 = Combine(filterData1.word2, filterData1.word3);
            return (layer0 & group1) && (layer1 & group0);
        }
    }
}