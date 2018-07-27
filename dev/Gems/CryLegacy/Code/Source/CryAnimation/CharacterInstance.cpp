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

#include "stdafx.h"

#include "ModelMesh.h"
#include "CharacterManager.h"
#include "IRenderAuxGeom.h"
#include "FacialAnimation/FacialInstance.h"
#include "GeomQuery.h"
#include "CharacterInstance.h"
#include <IJobManager_JobDelegator.h>

#include "Vertex/VertexCommandBuffer.h"

#include "ModelBoneNames.h"

CCharInstance::CCharInstance(const string& strFileName, _smart_ptr<CDefaultSkeleton> pDefaultSkeleton)
    :   m_skinningTransformationsCount(0)
    , m_skinningTransformationsMovement(0)
{
    LOADING_TIME_PROFILE_SECTION(g_pISystem);
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Character Instance");

    g_pCharacterManager->RegisterInstanceSkel(pDefaultSkeleton, this);
    m_pDefaultSkeleton = pDefaultSkeleton;
    m_strFilePath = strFileName;

    m_CharEditMode = 0;
    m_nRefCounter = 0;
    m_nInstanceUpdateCounter = 0;
    m_nAnimationLOD = 0;
    m_nLastRenderedFrameID = 0xffaaffaa;
    m_RenderPass = 0x55aa55aa;
    m_LastRenderedFrameID = 0;
    m_Viewdir = Vec3(0, 1, 0);

    m_nForceUpdate = 1;
    m_fDeltaTime = 0;
    m_fOriginalDeltaTime = 0;
    m_useDecals = 0;

    m_location.SetIdentity();

    m_AttachmentManager.m_pSkelInstance = this;

    m_fPostProcessZoomAdjustedDistanceFromCamera = 0;

    m_ResetMode = 0;
    m_HideMaster = 0;
    m_bWasVisible = 0;
    m_bPlayedPhysAnim = 0;
    m_bEnableStartAnimation =   1;

    m_fPlaybackScale = 1;
    m_LastUpdateFrameID_Pre = 0;
    m_LastUpdateFrameID_Post = 0;

    m_rpFlags = CS_FLAG_DRAW_MODEL;
    m_bUpdateMotionBlurSkinnning = false;
    memset(arrSkinningRendererData, 0, sizeof(arrSkinningRendererData));
    m_SkeletonAnim.InitSkeletonAnim(this, &this->m_SkeletonPose);
    RuntimeInit(0);
}

//--------------------------------------------------------------------------------------------
//--          re-initialize all members related to CDefaultSkeleton
//--------------------------------------------------------------------------------------------
void CCharInstance::RuntimeInit(_smart_ptr<CDefaultSkeleton> pExtDefaultSkeleton)
{

    //On reload we need to clear out the motion blur pose array of old data
    //we need to wait for async jobs to be done using the data before clearing
    for (uint32 i = 0; i < tripleBufferSize; i++)
    {
        SSkinningData* pSkinningData = arrSkinningRendererData[i].pSkinningData;
        int expectedNumBones = arrSkinningRendererData[i].nNumBones;
        if (pSkinningData
            && pSkinningData->nNumBones == expectedNumBones
            && pSkinningData->pPreviousSkinningRenderData
            && pSkinningData->pPreviousSkinningRenderData->nNumBones == expectedNumBones
            && pSkinningData->pPreviousSkinningRenderData->pAsyncJobExecutor)
        {
                pSkinningData->pPreviousSkinningRenderData->pAsyncJobExecutor->WaitForCompletion();
        }
    }

    memset(arrSkinningRendererData, 0, sizeof(arrSkinningRendererData));
    m_SkeletonAnim.FinishAnimationComputations();

    //-----------------------------------------------
    if (pExtDefaultSkeleton)
    {
        g_pCharacterManager->UnregisterInstanceSkel(m_pDefaultSkeleton, this);
        g_pCharacterManager->RegisterInstanceSkel(pExtDefaultSkeleton, this);
        m_pDefaultSkeleton = pExtDefaultSkeleton;
    }

    uint32 numJoints        = m_pDefaultSkeleton->m_arrModelJoints.size();
    m_skinningTransformationsCount = 0;
    m_skinningTransformationsMovement = 0;
    if (m_pDefaultSkeleton->m_ObjectType == CHR)
    {
        m_skinningTransformationsCount = numJoints;
    }

    m_SkeletonAnim.m_facialDisplaceInfo.Initialize(0);
    m_bFacialAnimationEnabled = true;
    m_pFacialInstance = 0;
    if (m_pDefaultSkeleton->GetFacialModel())
    {
        m_pFacialInstance = new CFacialInstance(m_pDefaultSkeleton->GetFacialModel(), this);
        m_pFacialInstance->AddRef();
    }

    m_SkeletonPose.InitSkeletonPose(this, &this->m_SkeletonAnim);

    //
    m_SkeletonPose.UpdateBBox(1);
    m_SkeletonPose.m_physics.m_bHasPhysics = m_pDefaultSkeleton->m_bHasPhysics2;
    m_SkeletonPose.m_physics.m_bHasPhysicsProxies = false;

    m_bHasVertexAnimation = false;
    m_bUseMatrixSkinning = false;
}




//////////////////////////////////////////////////////////////////////////
CCharInstance::~CCharInstance()
{
    CRY_ASSERT(m_nRefCounter == 0);
    SAFE_RELEASE(m_pFacialInstance);
    m_SkeletonPose.m_physics.DestroyPhysics();
    KillAllSkeletonEffects();

    // unregister from animation queue
    g_pCharacterManager->RemoveAnimationToSyncQueue(this);

    // sync skinning jobs if there are any
    int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
    int nList = nFrameID % 3;
    if (arrSkinningRendererData[nList].nFrameID == nFrameID && arrSkinningRendererData[nList].pSkinningData && arrSkinningRendererData[nList].pSkinningData->pAsyncJobExecutor)
    {
        arrSkinningRendererData[nList].pSkinningData->pAsyncJobExecutor->WaitForCompletion();
    }

    // make sure skeleton animation job finishes
    m_SkeletonAnim.m_threadTask.Wait();

    const char* pFilePath = GetFilePath();
    g_pCharacterManager->ReleaseCDF(pFilePath);

    if (m_pDefaultSkeleton)
    {
        g_pCharacterManager->UnregisterInstanceSkel(m_pDefaultSkeleton, this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCharInstance::SetOwnerId(const AZ::EntityId& id)
{
    m_ownerId = id;
}

//////////////////////////////////////////////////////////////////////////
const AZ::EntityId& CCharInstance::GetOwnerId() const
{
    return m_ownerId;
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------


void CCharInstance::StartAnimationProcessing(const SAnimationProcessParams& params)
{
    DEFINE_PROFILER_FUNCTION();
    ANIMATION_LIGHT_PROFILER();

    static ICVar* pUseSinglePositionVar = gEnv->pConsole->GetCVar("g_useSinglePosition");
    if (pUseSinglePositionVar && pUseSinglePositionVar->GetIVal() & 4)
    {
        m_SkeletonPose.m_physics.SynchronizeWithPhysics(m_SkeletonPose.GetPoseDataExplicitWriteable());
    }

    m_location                  = params.locationAnimation; //this the current location
    m_fPostProcessZoomAdjustedDistanceFromCamera = params.zoomAdjustedDistanceFromCamera;

    //float fColor[4] = {1,1,0,1};
    //g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"Uniform-Scaling: %f",m_location.s );   g_YLine+=16.0f;

    //_controlfp(0, _EM_INVALID|_EM_ZERODIVIDE | _PC_64 );

    uint32 nErrorCode = 0;
    uint32 minValid = m_SkeletonPose.m_AABB.min.IsValid();
    if (minValid == 0)
    {
        nErrorCode |= 0x8000;
    }
    uint32 maxValid = m_SkeletonPose.m_AABB.max.IsValid();
    if (maxValid == 0)
    {
        nErrorCode |= 0x8000;
    }
    if (Console::GetInst().ca_SaveAABB && nErrorCode)
    {
        m_SkeletonPose.m_AABB.max = Vec3(2, 2, 2);
        m_SkeletonPose.m_AABB.min = Vec3(-2, -2, -2);
    }

    float fNewDeltaTime = clamp_tpl(g_pITimer->GetFrameTime(), 0.0f, 0.5f);
    fNewDeltaTime = (float) fsel(params.overrideDeltaTime, params.overrideDeltaTime, fNewDeltaTime);
    m_fOriginalDeltaTime = fNewDeltaTime;
    m_fDeltaTime                 =  fNewDeltaTime * m_fPlaybackScale;

    //---------------------------------------------------------------------------------

    m_SkeletonPose.m_bFullSkeletonUpdate = 0;
    int nCurrentFrameID = g_pCharacterManager->m_nUpdateCounter; //g_pIRenderer->GetFrameID(false);
    uint32 dif = nCurrentFrameID - m_LastRenderedFrameID;
    m_SkeletonPose.m_bInstanceVisible = (dif < 5) || m_SkeletonAnim.GetTrackViewStatus();


    if (m_SkeletonPose.m_bInstanceVisible)
    {
        m_SkeletonPose.m_bFullSkeletonUpdate = 1;
    }
    if (m_SkeletonAnim.m_TrackViewExclusive)
    {
        m_SkeletonPose.m_bFullSkeletonUpdate = 1;
    }
    if (m_SkeletonPose.m_nForceSkeletonUpdate)
    {
        m_SkeletonPose.m_bFullSkeletonUpdate = 1;
    }
    if (Console::GetInst().ca_ForceUpdateSkeletons != 0)
    {
        m_SkeletonPose.m_bFullSkeletonUpdate = 1;
    }

    // depending on how far the character from the player is, the bones may be updated once per several frames
    m_nInstanceUpdateCounter++;

    m_SkeletonAnim.m_IsAnimPlaying = 0;
    m_SkeletonAnim.ProcessAnimations(params.locationAnimation, 0);
}


//////////////////////////////////////////////////////////////////////////
bool CCharInstance::CopyPoseFrom(const ICharacterInstance& rICharInstance)
{
    const ISkeletonPose* srcIPose = rICharInstance.GetISkeletonPose();
    const IDefaultSkeleton& rIDefaultSkeleton = rICharInstance.GetIDefaultSkeleton();

    if (srcIPose && m_pDefaultSkeleton->GetJointCount() == rIDefaultSkeleton.GetJointCount())
    {
        const CSkeletonPose* srcPose = (CSkeletonPose*)srcIPose;
        m_SkeletonPose.GetPoseDataForceWriteable().Initialize(srcPose->GetPoseData());
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
// TODO: Should be part of CSkeletonPhysics!
void CCharInstance::UpdatePhysicsCGA(Skeleton::CPoseData& poseData, float fScale, const QuatTS& rAnimLocationNext)
{
    DEFINE_PROFILER_FUNCTION();
    CAnimationSet* pAnimationSet = m_pDefaultSkeleton->m_pAnimationSet;

    if (!m_SkeletonPose.GetCharacterPhysics())
    {
        return;
    }

    if (m_fOriginalDeltaTime <= 0.0f)
    {
        return;
    }

    QuatT* const __restrict pAbsolutePose = poseData.GetJointsAbsolute();

    pe_params_part params;
    pe_action_set_velocity asv;
    pe_status_pos sp;
    int numNodes = poseData.GetJointCount();

    int i, iLayer, iLast = -1;
    bool bSetVel = 0;
    uint32 nType = m_SkeletonPose.GetCharacterPhysics()->GetType();
    bool bBakeRotation = m_SkeletonPose.GetCharacterPhysics()->GetType() == PE_ARTICULATED;
    if (bSetVel = bBakeRotation)
    {
        for (iLayer = 0; iLayer < numVIRTUALLAYERS && m_SkeletonAnim.GetNumAnimsInFIFO(iLayer) == 0; iLayer++)
        {
            ;
        }
        bSetVel = iLayer < numVIRTUALLAYERS;
    }
    params.bRecalcBBox = false;


    for (i = 0; i < numNodes; i++)
    {
SetAgain:
        if (m_SkeletonPose.m_physics.m_ppBonePhysics && m_SkeletonPose.m_physics.m_ppBonePhysics[i])
        {
            sp.ipart = 0;
            MARK_UNUSED sp.partid;
            m_SkeletonPose.m_physics.m_ppBonePhysics[i]->GetStatus(&sp);
            pAbsolutePose[i].q = !rAnimLocationNext.q * sp.q;
            pAbsolutePose[i].t = !rAnimLocationNext.q * (sp.pos - rAnimLocationNext.t);
            continue;
        }
        if (m_pDefaultSkeleton->m_arrModelJoints[i].m_NodeID == ~0)
        {
            continue;
        }
        //  params.partid = joint->m_nodeid;
        params.partid = m_pDefaultSkeleton->m_arrModelJoints[i].m_NodeID;

        CRY_ASSERT(pAbsolutePose[i].IsValid());
        params.q        =   pAbsolutePose[i].q;

        params.pos  = pAbsolutePose[i].t * rAnimLocationNext.s;
        if (rAnimLocationNext.s != 1.0f)
        {
            params.scale = rAnimLocationNext.s;
        }

        if (bBakeRotation)
        {
            params.pos = rAnimLocationNext.q * params.pos;
            params.q = rAnimLocationNext.q * params.q;
        }
        if (m_SkeletonPose.GetCharacterPhysics()->SetParams(&params))
        {
            iLast = i;
        }
        if (params.bRecalcBBox)
        {
            break;
        }
    }

    // Recompute box after.
    if (iLast >= 0 && !params.bRecalcBBox)
    {
        params.bRecalcBBox = true;
        i = iLast;
        goto SetAgain;
    }

    if (bSetVel)
    {
        ApplyJointVelocitiesToPhysics(m_SkeletonPose.GetCharacterPhysics(), rAnimLocationNext.q);
        m_bPlayedPhysAnim = 1;
    }
    else if (bBakeRotation && m_bPlayedPhysAnim)
    {
        asv.v.zero();
        asv.w.zero();
        for (i = 0; i < numNodes; i++)
        {
            if ((asv.partid = m_pDefaultSkeleton->m_arrModelJoints[i].m_NodeID) != ~0)
            {
                m_SkeletonPose.GetCharacterPhysics()->Action(&asv);
            }
        }
        m_bPlayedPhysAnim = 0;
    }

    if (m_SkeletonPose.m_physics.m_ppBonePhysics)
    {
        m_SkeletonPose.UpdateBBox(1);
    }
}


void CCharInstance::ApplyJointVelocitiesToPhysics(IPhysicalEntity* pent, const Quat& qrot, const Vec3& velHost)
{
    int i, iParent, numNodes = m_SkeletonPose.GetPoseData().GetJointCount();
    QuatT qParent;
    IController* pController;
    float t, dt;
    pe_action_set_velocity asv;
    CAnimationSet* pAnimationSet = m_pDefaultSkeleton->m_pAnimationSet;

    PREFAST_SUPPRESS_WARNING(6255)
    QuatT * qAnimCur = (QuatT*)alloca(numNodes * sizeof(QuatT));
    PREFAST_SUPPRESS_WARNING(6255)
    QuatT * qAnimNext = (QuatT*)alloca(numNodes * sizeof(QuatT));
    for (i = 0; i < numNodes; i++)
    {
        qAnimCur[i] = qAnimNext[i] = m_SkeletonPose.GetPoseData().GetJointRelative(i);
    }

    if (!m_SkeletonAnim.m_layers[0].m_transitionQueue.m_animations.size())
    {
        for (i = 0; i < numNodes; i++)
        {
            if (m_pDefaultSkeleton->m_arrModelJoints[i].m_NodeID != ~0)
            {
                asv.partid = m_pDefaultSkeleton->m_arrModelJoints[i].m_NodeID;
                asv.v = velHost;
                pent->Action(&asv);
            }
        }
        return;
    }
    const CAnimation& anim = m_SkeletonAnim.GetAnimFromFIFO(0, 0);

    int idGlobal = pAnimationSet->GetGlobalIDByAnimID_Fast(anim.GetAnimationId());
    GlobalAnimationHeaderCAF& animHdr = g_AnimationManager.m_arrGlobalCAF[idGlobal];

    const float FRAME_TIME = 0.01f;
    const float fAnimDuration = max(anim.GetExpectedTotalDurationSeconds(), 1e-5f);
    dt = static_cast<float>(fsel(fAnimDuration - FRAME_TIME, FRAME_TIME / fAnimDuration, 1.0f));
    t = min(1.0f - dt, m_SkeletonAnim.GetAnimationNormalizedTime(&anim));

    for (i = 0; i < numNodes; i++)
    {
        if (pController = animHdr.GetControllerByJointCRC32(m_pDefaultSkeleton->m_arrModelJoints[i].m_nJointCRC32))                        //m_pDefaultSkeleton->m_arrModelJoints[i].m_arrControllersMJoint[idAnim])
        {
            pController->GetOP(animHdr.NTime2KTime(t), qAnimCur[i].q, qAnimCur[i].t),
            pController->GetOP(animHdr.NTime2KTime(t + dt), qAnimNext[i].q, qAnimNext[i].t);
        }
    }

    for (i = 0; i < numNodes; i++)
    {
        if ((iParent = m_pDefaultSkeleton->m_arrModelJoints[i].m_idxParent) >= 0 && iParent != i)
        {
            PREFAST_ASSUME(iParent >= 0 && iParent < numNodes);
            qAnimCur[i] =   qAnimCur[iParent] * qAnimCur[i], qAnimNext[i] =   qAnimNext[iParent] * qAnimNext[i];
        }
    }

    const float INV_FRAME_TIME = 1.0f / FRAME_TIME;
    for (i = 0; i < numNodes; i++)
    {
        if (m_pDefaultSkeleton->m_arrModelJoints[i].m_NodeID != ~0)
        {
            asv.partid = m_pDefaultSkeleton->m_arrModelJoints[i].m_NodeID;
            asv.v = qrot * ((qAnimNext[i].t - qAnimCur[i].t) * INV_FRAME_TIME) + velHost;
            Quat dq = qAnimNext[i].q * !qAnimCur[i].q;
            float sin2 = dq.v.len();
            asv.w = qrot * (sin2 > 0 ? dq.v * (atan2_tpl(sin2 * dq.w * 2, dq.w * dq.w - sin2 * sin2) * INV_FRAME_TIME / sin2) : Vec3(0));
            pent->Action(&asv);
        }
    }
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

void CCharInstance::ComputeGeometricMean(SMeshLodInfo& lodInfo) const
{
    lodInfo.Clear();

    const int attachmentCount = m_AttachmentManager.GetAttachmentCount();
    for (int i = 0; i < attachmentCount; ++i)
    {
        SMeshLodInfo attachmentLodInfo;

        const IAttachment* const pIAttachment = m_AttachmentManager.GetInterfaceByIndex(i);

        if (pIAttachment != NULL && pIAttachment->GetIAttachmentObject() != NULL)
        {
            const IAttachmentObject* const pIAttachmentObject = pIAttachment->GetIAttachmentObject();

            if (pIAttachmentObject->GetIAttachmentSkin())
            {
                pIAttachmentObject->GetIAttachmentSkin()->ComputeGeometricMean(attachmentLodInfo);
            }
            else if (pIAttachmentObject->GetIStatObj())
            {
                pIAttachmentObject->GetIStatObj()->ComputeGeometricMean(attachmentLodInfo);
            }
            else if (pIAttachmentObject->GetICharacterInstance())
            {
                pIAttachmentObject->GetICharacterInstance()->ComputeGeometricMean(attachmentLodInfo);
            }

            lodInfo.Merge(attachmentLodInfo);
        }
    }

    if (GetObjectType() == CGA)
    {
        // joints
        if (ISkeletonPose* pSkeletonPose = const_cast<CCharInstance*>(this)->GetISkeletonPose())
        {
            uint32 numJoints = GetIDefaultSkeleton().GetJointCount();

            // check StatObj attachments
            for (uint32 i = 0; i < numJoints; i++)
            {
                SMeshLodInfo attachmentLodInfo;
                IStatObj* pStatObj = pSkeletonPose->GetStatObjOnJoint(i);
                if (pStatObj)
                {
                    pStatObj->ComputeGeometricMean(attachmentLodInfo);
                    lodInfo.Merge(attachmentLodInfo);
                }
            }
        }
    }
}

const AABB& CCharInstance::GetAABB() const
{
    return m_SkeletonPose.GetAABB();
}

float CCharInstance::GetRadiusSqr() const
{
    return m_SkeletonPose.GetAABB().GetRadiusSqr();
}

void CCharInstance::UpdateAABB()
{
    m_SkeletonPose.UpdateBBox();
}

float CCharInstance::GetExtent(EGeomForm eForm)
{
    // Sum extents from base mesh, CGA joints, and attachments.
    CGeomExtent& extent = m_Extents.Make(eForm);
    if (!extent)
    {
        extent.ReserveParts(3);

        // Add base model as first part.
        float fModelExt = 0.f;
        if (m_pDefaultSkeleton)
        {
            if (IRenderMesh* pMesh = m_pDefaultSkeleton->GetIRenderMesh())
            {
                fModelExt = pMesh->GetExtent(eForm);
            }
        }
        extent.AddPart(fModelExt);

        extent.AddPart(m_SkeletonPose.GetExtent(eForm));
        extent.AddPart(m_AttachmentManager.GetExtent(eForm));
    }
    return extent.TotalExtent();
}

void CCharInstance::GetRandomPos(PosNorm& ran, EGeomForm eForm) const
{
    CGeomExtent const& ext = m_Extents[eForm];
    int iPart = ext.RandomPart();

    if (iPart-- == 0)
    {
        // Base model.
        IRenderMesh* pMesh = m_pDefaultSkeleton->GetIRenderMesh();

        SSkinningData* pSkinningData = NULL;
        int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
        for (int n = 0; n < 3; n++)
        {
            int nList = (nFrameID - n) % 3;
            if (arrSkinningRendererData[nList].nFrameID == nFrameID - n)
            {
                pSkinningData = arrSkinningRendererData[nList].pSkinningData;
                break;
            }
        }

        return pMesh->GetRandomPos(ran, eForm, pSkinningData);
    }

    if (iPart-- == 0)
    {
        // Choose CGA joint.
        return m_SkeletonPose.GetRandomPos(ran, eForm);
    }

    if (iPart-- == 0)
    {
        // Choose attachment.
        return m_AttachmentManager.GetRandomPos(ran, eForm);
    }

    ran.zero();
}

void CCharInstance::OnDetach()
{
    pe_params_rope pr;
    pr.pEntTiedTo[0] = pr.pEntTiedTo[1] = 0;
    m_SkeletonPose.m_physics.SetAuxParams(&pr);
}

// Skinning

void CCharInstance::BeginSkinningTransformationsComputation(SSkinningData* pSkinningData)
{
    if (m_pDefaultSkeleton->m_ObjectType == CHR && pSkinningData->pMasterSkinningDataList)
    {
        *pSkinningData->pMasterSkinningDataList = SKINNING_TRANSFORMATION_RUNNING_INDICATOR;

        CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton.get();
        const Skeleton::CPoseData* pPoseData = &m_SkeletonPose.GetPoseData();
        f32* pSkinningTransformationsMovement = &m_skinningTransformationsMovement;
        pSkinningData->pAsyncJobExecutor->StartJob([this, pSkinningData, pDefaultSkeleton, pPoseData, pSkinningTransformationsMovement]()
        {
            this->SkinningTransformationsComputation(pSkinningData, pDefaultSkeleton, 0, pPoseData, pSkinningTransformationsMovement);
        }); // Legacy JobManager priority: eHighPriority
    }
}

//////////////////////////////////////////////////////////////////////////
void CCharInstance::Serialize(TSerialize ser)
{
    if (ser.GetSerializationTarget() != eST_Network)
    {
        MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Character instance serialization");

        ser.BeginGroup("CCharInstance");
        ser.Value("fPlaybackScale", (float&)(m_fPlaybackScale));
        ser.EndGroup();

        m_SkeletonAnim.Serialize(ser);
        m_AttachmentManager.Serialize(ser);

        if (ser.IsReading())
        {
            // Make sure that serialized characters that are ragdoll are updated even if their physic entity is sleeping
            m_SkeletonPose.m_physics.m_bPhysicsWasAwake = true;
        }
    }
}


#ifdef EDITOR_PCDEBUGCODE
void CCharInstance::ReloadCHRPARAMS(const std::string* optionalBuffer)
{
    m_SkeletonAnim.StopAnimationsAllLayers();
    m_SkeletonAnim.FinishAnimationComputations();
    g_pCharacterManager->SyncAllAnimations();

    g_pCharacterManager->GetParamLoader().ClearLists();
    m_pDefaultSkeleton->m_pAnimationSet->ReleaseAnimationData();
    string paramFileNameBase(m_pDefaultSkeleton->GetModelFilePath());
    // Extended skeletons are identified with underscore prefix
    if (!paramFileNameBase.empty() && paramFileNameBase[0] == '_')
    {
        paramFileNameBase.erase(0, 1);
    }
    paramFileNameBase.replace(".chr", ".chrparams");
    paramFileNameBase.replace(".cga", ".chrparams");
    CryLog("Reloading %s", paramFileNameBase.c_str());
    m_pDefaultSkeleton->LoadCHRPARAMS(paramFileNameBase.c_str(), optionalBuffer);
    m_pDefaultSkeleton->m_pAnimationSet->NotifyListenersAboutReload();

    //--------------------------------------------------------------------------
    //--- check if there is an attached SKEL and reload its list as well
    //--------------------------------------------------------------------------
    uint32 numAttachments = m_AttachmentManager.m_arrAttachments.size();
    for (uint32 i = 0; i < numAttachments; i++)
    {
        IAttachment* pIAttachment = m_AttachmentManager.m_arrAttachments[i];
        IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
        if (pIAttachmentObject == 0)
        {
            continue;
        }

        uint32 type = pIAttachment->GetType();
        CCharInstance* pCharInstance = (CCharInstance*)pIAttachmentObject->GetICharacterInstance();
        if (pCharInstance == 0)
        {
            continue;  //its not a CHR at all
        }
        //reload Animation list for an attached character (attached SKELs are a rare case)
        g_pCharacterManager->GetParamLoader().ClearLists();
        m_pDefaultSkeleton->m_pAnimationSet->ReleaseAnimationData();
        string paramFileName(m_pDefaultSkeleton->GetModelFilePath());
        paramFileName.replace(".chr", ".chrparams");
        CryLog("Reloading %s", paramFileName.c_str());
        m_pDefaultSkeleton->LoadCHRPARAMS(paramFileName.c_str());
    }
}
#endif













//////////////////////////////////////////////////////////////////////////
IFacialInstance* CCharInstance::GetFacialInstance()
{
    return m_pFacialInstance;
};


//////////////////////////////////////////////////////////////////////////
const IFacialInstance* CCharInstance::GetFacialInstance() const
{
    return m_pFacialInstance;
};


//////////////////////////////////////////////////////////////////////////
void CCharInstance::LipSyncWithSound(uint32 nSoundId, bool bStop)
{
    IFacialInstance* pFacialInstance = GetFacialInstance();
    if (pFacialInstance)
    {
        pFacialInstance->LipSyncWithSound(nSoundId, bStop);
    }
}


//////////////////////////////////////////////////////////////////////////
void CCharInstance::EnableFacialAnimation(bool bEnable)
{
    m_bFacialAnimationEnabled = bEnable;

    for (int attachmentIndex = 0, end = int(m_AttachmentManager.m_arrAttachments.size()); attachmentIndex < end; ++attachmentIndex)
    {
        IAttachment* pIAttachment = m_AttachmentManager.m_arrAttachments[attachmentIndex];
        IAttachmentObject* pIAttachmentObject = (pIAttachment ? pIAttachment->GetIAttachmentObject() : 0);
        ICharacterInstance* pIAttachmentCharacter = (pIAttachmentObject ? pIAttachmentObject->GetICharacterInstance() : 0);
        if (pIAttachmentCharacter)
        {
            pIAttachmentCharacter->EnableFacialAnimation(bEnable);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CCharInstance::EnableProceduralFacialAnimation(bool bEnable)
{
    IFacialInstance* pInst = GetFacialInstance();
    if (pInst)
    {
        pInst->EnableProceduralFacialAnimation(bEnable);
    }
}




void CCharInstance::ProcessAttachment(IAttachment* pIAttachment)
{
    //this is a critical case:
    //we have an attachment, which is also a skeleton-hierarchy and which can have its own attachments.
    //The only way to process this, is to do an immediate animation-update and call the PostProcess directly

    //float fColor[4] = {0,1,0,1};
    //g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"ProcessAttachment: %s", GetFilePath() );
    //g_YLine+=16.0f;

    CCharInstance* pInstanceMaster = 0;

    uint32 nType = pIAttachment->GetType();
    if (nType == CA_BONE)
    {
        pInstanceMaster = ((CAttachmentBONE*)pIAttachment)->m_pAttachmentManager->m_pSkelInstance;
    }
    if (nType == CA_FACE)
    {
        pInstanceMaster = ((CAttachmentFACE*)pIAttachment)->m_pAttachmentManager->m_pSkelInstance;
    }

    PREFAST_ASSUME(pInstanceMaster);

    //initialize the attached CHR with values from its parent-CHR
    m_location      =   pIAttachment->GetAttWorldAbsolute();
    m_fPostProcessZoomAdjustedDistanceFromCamera = pInstanceMaster->m_fPostProcessZoomAdjustedDistanceFromCamera;
    m_fOriginalDeltaTime = pInstanceMaster->m_fOriginalDeltaTime;
    m_fDeltaTime    = pInstanceMaster->m_fDeltaTime;
    m_SkeletonPose.m_bFullSkeletonUpdate = pInstanceMaster->m_SkeletonPose.m_bFullSkeletonUpdate;
    m_SkeletonPose.m_bInstanceVisible    = pInstanceMaster->m_SkeletonPose.m_bInstanceVisible;
    m_nAnimationLOD = pInstanceMaster->m_nAnimationLOD;
    m_SkeletonAnim.m_IsAnimPlaying = false;
    m_SkeletonAnim.ProcessAnimations(m_location, 1); //immediate animation update

    m_skeletonEffectManager.Update(&m_SkeletonAnim, &m_SkeletonPose, m_location);
}


void CCharInstance::SkeletonPostProcess()
{
    f32 fZoomAdjustedDistanceFromCamera = m_fPostProcessZoomAdjustedDistanceFromCamera;
    QuatTS rPhysLocation = m_location;
    m_SkeletonPose.SkeletonPostProcess(m_SkeletonPose.GetPoseDataExplicitWriteable());
    m_skeletonEffectManager.Update(&m_SkeletonAnim, &m_SkeletonPose, rPhysLocation);
};


void CCharInstance::SkinningTransformationsComputation(SSkinningData* pSkinningData, CDefaultSkeleton* pDefaultSkeleton, int nRenderLod, const Skeleton::CPoseData* pPoseData, f32* pSkinningTransformationsMovement)
{
    DEFINE_PROFILER_FUNCTION();
    CRY_ASSERT(pSkinningData);
    CRY_ASSERT(pSkinningData->pPreviousSkinningRenderData);
    CRY_ASSERT(pSkinningData->pPreviousSkinningRenderData->pBoneQuatsS);
    CRY_ASSERT(pSkinningData->pMasterSkinningDataList);

    //  float fColor[4] = {1,0,0,1};
    //  g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"SkinningTransformationsComputationTask" );
    //  g_YLine+=16.0f;

    const QuatT* pJointsAbsolute = pPoseData->GetJointsAbsolute();
    const QuatT* pJointsAbsoluteDefault = pDefaultSkeleton->m_poseDefaultData.GetJointsAbsolute();

    uint32 jointCount = pDefaultSkeleton->GetJointCount();
    const CDefaultSkeleton::SJoint* pJoints = &pDefaultSkeleton->m_arrModelJoints[0];

    //--- Deliberate querying off the end of arrays for speed, prefetching off the end of an array is safe & avoids extra branches
    const QuatT* absPose_PreFetchOnly = &pJointsAbsolute[0];

    f32& movement = *pSkinningTransformationsMovement;
    movement = 0.0f;

    QuatT currentTransform = pJointsAbsolute[0] * pJointsAbsoluteDefault[0].GetInverted();

    if (pSkinningData->nHWSkinningFlags & eHWS_Skinning_Matrix)
    {
        CRY_ASSERT(pSkinningData->pBoneMatrices);

        Matrix34* pSkinningMatrices = pSkinningData->pBoneMatrices;
        Matrix34* pSkinningMatricesPrevious = pSkinningData->pPreviousSkinningRenderData->pBoneMatrices;

        pSkinningMatrices[0] = Matrix34(currentTransform);

        for (uint32 i = 1; i < jointCount; ++i)
        {
#ifndef _DEBUG
            CryPrefetch(&pJointsAbsoluteDefault[i + 1]);
            CryPrefetch(&absPose_PreFetchOnly[i + 4]);
            CryPrefetch(&pSkinningMatrices[i + 4]);
            CryPrefetch(&pJoints[i + 1].m_idxParent);
#endif
            currentTransform = pJointsAbsolute[i] * pJointsAbsoluteDefault[i].GetInverted();

            int32 parentIdx = pJoints[i].m_idxParent;

            if (parentIdx > -1)
            {
                Quat parentQuat = Quat(pSkinningMatrices[parentIdx]);
                f32 cosine = currentTransform.q | parentQuat;
                f32 mul = (f32)fsel(cosine, 1.0f, -1.0f);
                currentTransform.q *= mul;
            }

            // Note for future work:
            // Uniform and non-uniform scaling should be hooked up here
            pSkinningMatrices[i] = Matrix34(currentTransform);

            const Quat& q0 = currentTransform.q;
            const Quat& q1 = Quat(pSkinningMatricesPrevious[i]);
            f32 fQdot = q0 | q1;
            f32 fQdotSign = (f32)fsel(fQdot, 1.0f, -1.0f);
            movement += 1.0f - (fQdot * fQdotSign);
        }
    }
    else    // Dual Quaternion skinning
    {
        CRY_ASSERT(pSkinningData->pBoneQuatsS);

        DualQuat* pSkinningTransformations = pSkinningData->pBoneQuatsS;
        DualQuat* pSkinningTransformationsPrevious = pSkinningData->pPreviousSkinningRenderData->pBoneQuatsS;

        pSkinningTransformations[0] = currentTransform;

        for (uint32 i = 1; i < jointCount; ++i)
        {
#ifndef _DEBUG
            CryPrefetch(&pJointsAbsoluteDefault[i + 1]);
            CryPrefetch(&absPose_PreFetchOnly[i + 4]);
            CryPrefetch(&pSkinningTransformations[i + 4]);
            CryPrefetch(&pJoints[i + 1].m_idxParent);
#endif
            currentTransform = pJointsAbsolute[i] * pJointsAbsoluteDefault[i].GetInverted();

            int32 parentIdx = pJoints[i].m_idxParent;
            
            if (parentIdx > -1)
            {
                Quat parentQuat = pSkinningTransformations[parentIdx].nq;
                f32 cosine = currentTransform.q | parentQuat;
                f32 mul = (f32)fsel(cosine, 1.0f, -1.0f);
                currentTransform.q *= mul;
            }

            pSkinningTransformations[i] = currentTransform;

            const Quat& q0 = currentTransform.q;
            const Quat& q1 = pSkinningTransformationsPrevious[i].nq;
            f32 fQdot = q0 | q1;
            f32 fQdotSign = (f32)fsel(fQdot, 1.0f, -1.0f);
            movement += 1.0f - (fQdot * fQdotSign);
        }
    }

    // set the list to NULL to indicate the mainthread that the skinning transformation job has finished
    SSkinningData* pSkinningJobList = NULL;
    do
    {
        pSkinningJobList = *(const_cast<SSkinningData* volatile*>(pSkinningData->pMasterSkinningDataList));
    } while (CryInterlockedCompareExchangePointer(alias_cast<void* volatile*>(pSkinningData->pMasterSkinningDataList), NULL, pSkinningJobList) != pSkinningJobList);

    // start SW-Skinning Job till the list is empty
    while (pSkinningJobList != SKINNING_TRANSFORMATION_RUNNING_INDICATOR)
    {
        SVertexAnimationJob* pVertexAnimation = static_cast<SVertexAnimationJob*>(pSkinningJobList->pCustomData);
        pVertexAnimation->Begin(pSkinningJobList->pAsyncJobExecutor);

        pSkinningJobList = pSkinningJobList->pNextSkinningData;
    }
}


//////////////////////////////////////////////////////////////////////////
size_t CCharInstance::SizeOfCharInstance()
{
    size_t nSizeOfCharInstance  = 0;
    //--------------------------------------------------------------------
    //---        this is the size of the SkinInstance class           ----
    //--------------------------------------------------------------------
    nSizeOfCharInstance += SizeOfAttachmentManager();

    {
        size_t size     = sizeof(CAnimDecalManager) + m_DecalManager.SizeOfThis();
        nSizeOfCharInstance += size;
    }

    {
        size_t size     = sizeof(CFacialInstance);
        if (m_pFacialInstance)
        {
            size = m_pFacialInstance->SizeOfThis();
        }
        nSizeOfCharInstance += size;
    }

    {
        size_t size     = sizeof(CSkeletonEffectManager);
        size += m_skeletonEffectManager.SizeOfThis();
        nSizeOfCharInstance += size;
    }

    //--------------------------------------------------------------------
    //---        this is the size of the CharInstance class           ----
    //--------------------------------------------------------------------

    {
        size_t size = sizeof(CSkeletonAnim) + m_SkeletonAnim.SizeOfThis();
        nSizeOfCharInstance += size;
    }

    {
        size_t size = sizeof(CSkeletonPose) + m_SkeletonPose.SizeOfThis();
        nSizeOfCharInstance += size;
    }

    //--------------------------------------------------------------------
    //---          evaluate the real size of CCharInstance            ----
    //--------------------------------------------------------------------

    {
        size_t size     =   sizeof(CCharInstance) - sizeof(CAttachmentManager) - sizeof(CAnimDecalManager) - sizeof(CSkeletonAnim) - sizeof(CSkeletonPose) - sizeof(CSkeletonEffectManager);
        size += m_strFilePath.capacity();

        nSizeOfCharInstance += size;
    }

    return nSizeOfCharInstance;
};

void CCharInstance::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));

    //--------------------------------------------------------------------
    //---        this is the size of the SkinInstance class           ----
    //--------------------------------------------------------------------
    {
        SIZER_COMPONENT_NAME(pSizer, "CAttachmentManager");
        pSizer->AddObject(m_AttachmentManager);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "DecalManager");
        pSizer->AddObject(m_DecalManager);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "FacialInstance");
        pSizer->AddObject(m_pFacialInstance);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "skeletonEffectManager");
        pSizer->AddObject(m_skeletonEffectManager);
    }

    //--------------------------------------------------------------------
    //---        this is the size of the CharInstance class           ----
    //--------------------------------------------------------------------

    {
        SIZER_COMPONENT_NAME(pSizer, "SkeletonAnim");
        pSizer->AddObject(m_SkeletonAnim);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "SkeletonPose");
        pSizer->AddObject(m_SkeletonPose);
    }

    //--------------------------------------------------------------------
    //---          evaluate the real size of CCharInstance            ----
    //--------------------------------------------------------------------

    {
        SIZER_COMPONENT_NAME(pSizer, "CCharInstance");
    }


    {
        SIZER_SUBCOMPONENT_NAME(pSizer, "CAttachmentManager");
        pSizer->AddObject(m_AttachmentManager);
    }

    {
        SIZER_SUBCOMPONENT_NAME(pSizer, "DecalManager");
        pSizer->AddObject(m_DecalManager);
    }

    {
        SIZER_SUBCOMPONENT_NAME(pSizer, "FacialInstance");
        pSizer->AddObject(m_pFacialInstance);
    }
};



