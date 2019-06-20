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
#include "StdAfx.h"

#include <MathConversion.h>
#include <AzFramework/Physics/TouchBendingBus.h>
#include <AzCore/Math/ToString.h>

#include "Vegetation.h"
#include "TouchBendingCVegetationAgent.h"

namespace AZ
{ 
    static const float  DEFAULT_TREE_MASS = 0.010f; //10 grams.

    /// Children bones should have the mass of their parent bone times this factor.
    const float MASS_REDUCTION_FACTOR = 0.5f;

    //*************************************************************************
    // Helper Functions START *************************************************

    /** @brief Applies the mass value to all the segments of a tree.
     *
     *  @param firstSegmentMass The mass in Kg of the first segment.
     *  @param spineTreeInOut Reference to the tree.
     *  @returns void
     */
    static void ApplyMass(const float firstBoneMass, Physics::SpineTree& spineTreeInOut)
    {
        const size_t spineCount = spineTreeInOut.m_spines.size();

        for (size_t spineIndex = 0; spineIndex < spineCount; ++spineIndex)
        {
            Physics::Spine& spine = spineTreeInOut.m_spines[spineIndex];
            const size_t numberOfPoints = spine.m_points.size();
            for (size_t pointIndex = 0; pointIndex < numberOfPoints; ++pointIndex)
            {
                Physics::SpinePoint& spinePoint = spine.m_points[pointIndex];
                const float boneMassFactor = spinePoint.m_mass;
                spinePoint.m_mass = boneMassFactor * firstBoneMass;
            }
        }
    }

    static AZStd::string Vec3ToStr(const Vec3& v)
    {
        AZStd::string retStr = AZStd::string::format("%f, %f, %f", v.x, v.y, v.z);
        return retStr;
    }

    // A CStatObj is basically an archetype for static geometry. It is not a render node.
    // It has m_pSpines and m_nSpines and with that data CStatObj::PhysicalizeFoliage(...)
    // can generate a unique CStatObjFoliage object, which in turn owns one CRopeEntity
    // per SSpine found in CStatObj::m_pSpines. You can think of a Set of CRopeEntities (spines)
    // as a Tree, hence the name Physics::SpineTree.
    // REMARK: SSpine::pVtx contains position of each node within the Model Position, it is NOT
    // relative position of a node with respect to a previous Node.
    //
    // An alternate algorithm is going on, which is the calculation of mass of each segment.
    // The idea is that given a total mass, we need to find the mass of the first segment, so that all subsequent
    // segments have 1 / 2 the mass of the current segment.Also if a spineChild is connected to another spineParent
    // the first segment of spineChild should have the mass of the parent segment it is connected to.
    // The first big for loop in this function calculates each segment corresponding 1/2 FACTOR. 
    // When the loop is done, we calculate the mass of the first segment and do a second
    // run by calling ApplyMass().
    static bool BuildSpineTreeInternal(CStatObj* statObj, Physics::SpineTree& spineTreeOut)
    {
        const int spineCount = statObj->m_nSpines;
        spineTreeOut.m_spines.set_capacity(spineCount);
        
        //In this first pass SpineSegment.m_mass is a factor = 0.5 * prevMassFactor.
        float  massFactorAccumulatorX = 0.0f;
        for (int spineIndex = 0; spineIndex < spineCount; ++spineIndex)
        {
            const SSpine* crySpine = &statObj->m_pSpines[spineIndex];
            const int parentSpineIndex = crySpine->iAttachSpine;
            const int parentSpinePointIndex = crySpine->iAttachSeg;

            if (spineIndex > 0)
            {
                if ((parentSpineIndex < 0) || (parentSpinePointIndex < 0))
                {
                    AZ_Error(Physics::AZ_TOUCH_BENDING_WINDOW, false, "Non root spine index %d must have valid parent and point index", spineIndex);
                    return false;
                }
            }

            // A Spine is made of bones.
            const int pointCountInSpine = crySpine->nVtx;

            spineTreeOut.m_spines.emplace_back(Physics::Spine());
            Physics::Spine& azSpine = spineTreeOut.m_spines.back();
            azSpine.m_points.set_capacity(pointCountInSpine);
            azSpine.m_parentSpineIndex = parentSpineIndex;
            azSpine.m_parentPointIndex = parentSpinePointIndex;
            for (int pointIndex = 0; pointIndex < pointCountInSpine; ++pointIndex)
            {
                azSpine.m_points.emplace_back(Physics::SpinePoint());
                Physics::SpinePoint& spinePoint = azSpine.m_points.back();

                float boneMassFactor = 1.0f;
                if ((spineIndex == 0) && (pointIndex == 0))
                {
                    boneMassFactor = 1.0f;
                }
                else
                {
                    if (pointIndex == 0)
                    {
                        boneMassFactor = spineTreeOut.m_spines[parentSpineIndex].m_points[parentSpinePointIndex].m_mass;
                    }
                    else
                    {
                        boneMassFactor = MASS_REDUCTION_FACTOR * azSpine.m_points[pointIndex - 1].m_mass;
                    }
                }

                spinePoint.m_mass = boneMassFactor; //As said above, in this pass this member is not mass but a temporary record of mass factor.
                spinePoint.m_thickness = crySpine->pThickness[pointIndex];
                spinePoint.m_damping = crySpine->pDamping[pointIndex];
                spinePoint.m_stiffness = crySpine->pStiffness[pointIndex];
                spinePoint.m_position = LYVec3ToAZVec3(crySpine->pVtx[pointIndex]);
                massFactorAccumulatorX += boneMassFactor;
            }
        }

        //Ok, do a final run through all the points and assign their masses.
        const float massOfFirstBone = DEFAULT_TREE_MASS / massFactorAccumulatorX;
        ApplyMass(massOfFirstBone, spineTreeOut);

        return true;
    }

    // Helper Functions END ***************************************************
    //*************************************************************************


    StaticInstance<TouchBendingCVegetationAgent> s_instance;
    TouchBendingCVegetationAgent::TouchBendingCVegetationAgent() : m_isEnabled(false)
    {
    }

    TouchBendingCVegetationAgent::~TouchBendingCVegetationAgent()
    {
    }

    TouchBendingCVegetationAgent* TouchBendingCVegetationAgent::GetInstance()
    {
        return s_instance;
    }

    bool TouchBendingCVegetationAgent::IsTouchBendingEnabled()
    {
        if (m_isEnabled)
        {
            return true;
        }
        m_isEnabled = Physics::IsTouchBendingEnabled();
        return m_isEnabled;
    }

    bool TouchBendingCVegetationAgent::Physicalize(CVegetation* vegetationNode)
    {
        if (!IsTouchBendingEnabled())
        {
            return false;
        }

        StatInstGroup& vegetGroup = vegetationNode->GetStatObjGroup();
        IStatObj* pBody = vegetGroup.GetStatObj();
        AZ_Assert(pBody && pBody->GetSpineCount() && pBody->GetSpines(), "Invalid StatObj or no spines");
        phys_geometry* physgeom = pBody->GetArrPhysGeomInfo()[PHYS_GEOM_TYPE_NO_COLLIDE];
        AZ_Assert(physgeom, "Invalid Physical Geometry");

        IGeometry* const geom = physgeom->pGeom;
        const int geomType = geom->GetType();
        if ((geomType != geomtypes::GEOM_CYLINDER) && (geomType != geomtypes::GEOM_CAPSULE) && (geomType != geomtypes::GEOM_SPHERE) && (geomType != geomtypes::GEOM_BOX))
        {
            AZ_WarningOnce("Touchbending", false, "Touchbending is being requested for a vegetation asset with a non-performant IGeometry Type (%d), it will use the bounding box for initial physics detection.", geomType);
        }

        //const Matrix34& worldMatrix34, const float scale
        Matrix34A memAlignedWorldMatrix34;
        vegetationNode->CalcMatrix(memAlignedWorldMatrix34);
        const AZ::Transform worldTransform = LYTransformToAZTransform(memAlignedWorldMatrix34);
        const AABB veg_aabb = vegetationNode->GetBBox(); //World AABB
        const AZ::Aabb worldAabb = LyAABBToAZAabb(veg_aabb);

        Physics::TouchBendingTriggerHandle* handle = nullptr;
        Physics::TouchBendingBus::BroadcastResult(handle, &Physics::TouchBendingRequest::CreateTouchBendingTrigger, worldTransform, worldAabb,
            this, vegetationNode);
        if (handle == nullptr)
        {
            AZ_Error(Physics::AZ_TOUCH_BENDING_WINDOW, false, "Failed to create TouchBendingTriggerHandle\n");
            return false;
        }
        vegetationNode->m_touchBendingTriggerProxy = handle;
        return true;
    }

    void TouchBendingCVegetationAgent::Dephysicalize(CVegetation* vegetationNode)
    {
        if (!m_isEnabled || (vegetationNode->m_touchBendingTriggerProxy == nullptr))
        {
            return;
        }

        Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::DeleteTouchBendingTrigger, vegetationNode->m_touchBendingTriggerProxy);
        vegetationNode->m_touchBendingTriggerProxy = nullptr;
    }

    
    
    //*********************************************************************
    // Physics::ITouchBendingCallback START ********************************

    Physics::SpineTreeIDType TouchBendingCVegetationAgent::CheckDistanceToCamera(const void* privateData)
    {
        //For this class it is totally safe to un-const privateData, because we generated it.
        CVegetation* vegetation = static_cast<CVegetation*>(const_cast<void*>(privateData));
        
        //The following distance based logic comes from CPhysCallbacks::OnFoliageTouched()
        CCamera& camera = gEnv->pSystem->GetViewCamera();
        int cullingDistance = GetCVars()->e_CullVegActivation;
        if (!cullingDistance ||
            (vegetation->GetDrawFrame() + 10 > gEnv->pRenderer->GetFrameID()) &&
            ((camera.GetPosition() - vegetation->GetPos()).len2() * sqr(camera.GetFov()) < sqr(cullingDistance * 1.04f)))
        {
            IStatObj* pStatObj = vegetation->GetStatObj();
            return static_cast<Physics::SpineTreeIDType>(pStatObj);
        }

        return nullptr;
    }

    bool TouchBendingCVegetationAgent::BuildSpineTree(const void* privateData, Physics::SpineTreeIDType spineTreeId, Physics::SpineTree& spineTreeOut)
    {
        CVegetation* vegetation = static_cast<CVegetation*>(const_cast<void*>(privateData));
        IStatObj* pStatObj = vegetation->GetStatObj();
        Physics::SpineTreeIDType pStatObjAsId = static_cast<Physics::SpineTreeIDType>(pStatObj);

        //If this assert trips, it is clearly a programmer error.
        //A simple Assert should be enough to hint for a required solution.
        AZ_Assert(pStatObjAsId == spineTreeId, "SpineTree ID Mismatch!!");

        spineTreeOut.m_spineTreeId = spineTreeId;
        return BuildSpineTreeInternal(static_cast<CStatObj*>(pStatObj), spineTreeOut);
    }

    //Equivalent to CVegetation::PhysicalizeFoliage()
    void TouchBendingCVegetationAgent::OnPhysicalizedTouchBendingSkeleton(const void* privateData, Physics::TouchBendingSkeletonHandle* skeletonHandle)
    {
        CVegetation* vegetation = static_cast<CVegetation*>(const_cast<void*>(privateData));
        CStatObj* pStatObj = static_cast<CStatObj*>(vegetation->GetStatObj());

        // we need to create a temporary SRenderingPassInfo structuture here to pass into CheckCreateRNTmpData,
        // CheckCreateRNTmpData, can call OnBecomeVisible, which uses the camera
        Get3DEngine()->CheckCreateRNTmpData(&vegetation->m_pRNTmpData, vegetation, SRenderingPassInfo::CreateGeneralPassRenderingInfo(gEnv->p3DEngine->GetRenderingCamera()));
        gEnv->p3DEngine->Tick(); // clear temp stored Cameras to prevent wasting space due many temp cameras
        AZ_Assert(vegetation->m_pRNTmpData, "Failed to creare temporary Render Data");

        //We make a conservatively sized world positioned AABB.
        //The idea is not having to ask each bone of the Foliage for its AABB 
        //and avoid calculating the dynamic AABB of the whole Tree.
        AABB worldAabb = vegetation->GetBBox();
        const Vec3 worldAabbSize = worldAabb.GetSize();
        const float maxDimension = max(max(worldAabbSize.x, worldAabbSize.y), worldAabbSize.z);
        const Vec3 expandVector(maxDimension - worldAabbSize.x, maxDimension - worldAabbSize.y, maxDimension - worldAabbSize.z);
        worldAabb.Expand(expandVector);

        if (OnPhysicalizedFoliage(pStatObj, worldAabb, vegetation->m_pRNTmpData->userData.m_pFoliage, GetCVars()->e_FoliageBranchesTimeout, skeletonHandle))
        {
            ((CStatObjFoliage*)vegetation->m_pRNTmpData->userData.m_pFoliage)->m_pVegInst = vegetation;
        }
    }

    // Physics::ITouchBendingCallback END **********************************
    //*********************************************************************



    //*************************************************************************
    // Used by CStatObjFoliage START ************************************

    void TouchBendingCVegetationAgent::OnFoliageDestroyed(IFoliage* pIFoliage)
    {
        CStatObjFoliage* pStatObjFoliage = static_cast<CStatObjFoliage*>(pIFoliage);
        AZ_Assert(pStatObjFoliage != nullptr, "Invalid CStatObjFoliage");
        
        Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::DephysicalizeTouchBendingSkeleton, pStatObjFoliage->m_touchBendingSkeletonProxy);
    }

    //Following the logic of CStatObjFoliage::Update()
    void TouchBendingCVegetationAgent::UpdateFoliage(IFoliage* pIFoliage, float dt, const CCamera& rCamera)
    {
        CStatObjFoliage* pStatObjFoliage = static_cast<CStatObjFoliage*>(pIFoliage);
        AZ_Assert(pStatObjFoliage != nullptr, "Invalid CStatObjFoliage");

        const AABB &bbox = pStatObjFoliage->m_worldAabb;
        int clipDist = GetCVars()->e_CullVegActivation;
        bool bVisible = rCamera.IsAABBVisible_E(bbox);
        bool bEnable = bVisible && isneg(((rCamera.GetPosition() - bbox.GetCenter()).len2() - sqr(clipDist)) * clipDist - 0.0001f);
        if (!bEnable)
        {
            if (inrange(pStatObjFoliage->m_timeInvisible += dt, 6.0f, 8.0f))
            {
                bEnable = true;
            }
        }
        else
        {
            pStatObjFoliage->m_timeInvisible = 0.0f;
        }

        pStatObjFoliage->m_bEnabled = bEnable;
        AZ::u32 boneCount = 0;
        AZ::u32 touchCount = 0;
        Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::SetTouchBendingSkeletonVisibility, pStatObjFoliage->m_touchBendingSkeletonProxy, bEnable, boneCount, touchCount);

        if (touchCount && bVisible)
        {
            pStatObjFoliage->m_timeIdle = 0;
        }
        if (pStatObjFoliage->m_lifeTime > 0 && (pStatObjFoliage->m_timeIdle += dt) > pStatObjFoliage->m_lifeTime)
        {
            *pStatObjFoliage->m_ppThis = nullptr;
            pStatObjFoliage->m_bDelete = 2;
        }
        threadID nThreadID = 0;
        gEnv->pRenderer->EF_Query(EFQ_MainThreadList, nThreadID);

        ComputeSkinningTransformations(pStatObjFoliage, boneCount, nThreadID);

    } //TouchBendingCVegetationAgent::UpdateFoliage

    // AZ::IFoliageTouchBendingProxy END **************************************
    //*************************************************************************



    //Equivalent to CStatObj::PhysicalizeFoliage()
    bool TouchBendingCVegetationAgent::OnPhysicalizedFoliage(CStatObj* pStatObj, const AABB& worldAabb, IFoliage*& pIRes, float lifeTime, Physics::TouchBendingSkeletonHandle* skeletonHandle)
    {
        CStatObjFoliage* pRes = (CStatObjFoliage*)pIRes, *pFirstFoliage = Get3DEngine()->m_pFirstFoliage;
        pRes = new CStatObjFoliage;
        if (!pRes)
        {
            AZ_Assert(pRes, "Failed to create a new CStatObjFoliage");
            return false;
        }

        pStatObj->AddRef();

        pRes->m_next = pFirstFoliage;
        pRes->m_prev = pFirstFoliage->m_prev;
        pFirstFoliage->m_prev->m_next = pRes;
        pFirstFoliage->m_prev = pRes;
        pRes->m_lifeTime = lifeTime;
        pRes->m_ppThis = &pIRes;
        pRes->m_timeIdle = 0;
        pRes->m_iActivationSource = 0;

        pRes->m_pStatObj = pStatObj;
        pRes->m_pTrunk = nullptr; //Supposed to be the Cry Physics Entity, but we are not using Cry Physics.
        pRes->m_pRopes = nullptr;
        pRes->m_nRopes = 0;

        pRes->m_touchBendingSkeletonProxy = skeletonHandle;
        pRes->m_worldAabb = worldAabb;

        pIRes = pRes;
        return true;
    } //TouchBendingCVegetationAgent::OnPhysicalizedFoliageInternal

    //Equivalent to CStatObjFoliage::ComputeSkinningTransformations(uint32 nList)
    void TouchBendingCVegetationAgent::ComputeSkinningTransformations(IFoliage* pFoliage,
                                                                      uint32 boneCount,
                                                                      uint32 nList)
    {
        CStatObjFoliage* pStatObjFoliage = static_cast<CStatObjFoliage*>(pFoliage);
        // We use boneCount + 1 because the Skinning code requires the first location of the array
        // to be all zeroes to work well.
        const uint32 boneOffset = 1;
        const uint32 skinningBoneCount = boneCount + boneOffset;

        if (!pStatObjFoliage->m_pSkinningTransformations[0])
        {
            pStatObjFoliage->m_pSkinningTransformations[0] = new QuatTS[skinningBoneCount];
            pStatObjFoliage->m_pSkinningTransformations[1] = new QuatTS[skinningBoneCount];
            memset(pStatObjFoliage->m_pSkinningTransformations[0], 0, skinningBoneCount * sizeof(QuatTS));
            memset(pStatObjFoliage->m_pSkinningTransformations[1], 0, skinningBoneCount * sizeof(QuatTS));
        }

        QuatTS* pPose = pStatObjFoliage->m_pSkinningTransformations[nList];

        if (pStatObjFoliage->m_bEnabled)
        {
            Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::ReadJointPositionsOfSkeleton,
                pStatObjFoliage->m_touchBendingSkeletonProxy, reinterpret_cast<Physics::JointPositions*>(&pPose[boneOffset]));
        }
        else
        {
            //Regarding "(nList + 1) & 1". This comes from  CStatObjFoliage::ComputeSkinningTransformations(uint32 nList).
            //It suggests that nList at times is different than zero. In reality it is always 0. Not sure under what circumnstances
            //it may be different than zero. To be conservative the same code is still being used here.
            memcpy(&pPose[boneOffset], &pStatObjFoliage->m_pSkinningTransformations[(nList + 1) & 1][boneOffset], (skinningBoneCount - boneOffset) * sizeof(QuatTS));
        }
    }
}; //namespace AZ
