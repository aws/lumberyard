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

#ifndef CRYINCLUDE_CRYANIMATION_CHARACTERINSTANCE_H
#define CRYINCLUDE_CRYANIMATION_CHARACTERINSTANCE_H
#pragma once

struct CryEngineDecalInfo;
struct CryParticleSpawnInfo;

extern f32 g_YLine;

#include <ICryAnimation.h>
#include "Model.h"
#include "SkeletonEffectManager.h"
#include "SkeletonAnim.h"
#include "SkeletonPose.h"
#include "AnimationThreadTask.h"
#include "AttachmentManager.h"  //embedded
#include "DecalManager.h"               //embedded

#include <AzCore/Component/EntityId.h>

//////////////////////////////////////////////////////////////////////

struct AnimData;
class CryCharManager;
class CFacialInstance;
class CModelMesh;
class CCamera;
class CSkeletonAnim;
class CSkeletonPose;




class CSkinningTransformationsComputationTask
    : public CAnimationThreadTask
{
public:
    CSkinningTransformationsComputationTask();

public:
    void Begin(const CDefaultSkeleton* pDefaultSkeleton, int renderLod, const Skeleton::CPoseData& poseData,   DualQuat* pSkinningTransformations, DualQuat* pSkinningTransformationsPrevious, uint32 skinningTransformationCount, f32* pSkinningTransformationsMovement);
    void Wait();
    DualQuat* GetSkinningTransformations() { return m_pSkinningTransformations; }
    DualQuat* GetSkinningTransformationsPrevious() { return m_pSkinningTransformationsPrevious; }
    void Execute();

private:
    int m_renderLod;
    const Skeleton::CPoseData* m_pPoseData;
    DualQuat* m_pSkinningTransformations;
    DualQuat* m_pSkinningTransformationsPrevious;
    uint32 m_skinningTransformationCount;
    f32* m_pSkinningTransformationsMovement;
};








///////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of ICharInstance interface, the main interface in the Animation module
///////////////////////////////////////////////////////////////////////////////////////////////
class CCharInstance
    : public ICharacterInstance
{
    friend class CAttachmentManager;
    friend class CAttachmentSKIN;
    friend class CModelMesh;

public:
    ~CCharInstance();
    CCharInstance (const string& strFileName, _smart_ptr<CDefaultSkeleton> pDefaultSkeleton);

    void RuntimeInit(_smart_ptr<CDefaultSkeleton> pExtDefaultSkeleton);
    SSkinningData* GetSkinningData();

    void SetOwnerId(const AZ::EntityId& id) override;
    const AZ::EntityId& GetOwnerId() const override;

    virtual IAttachmentManager* GetIAttachmentManager() { return &m_AttachmentManager; }
    virtual const IAttachmentManager* GetIAttachmentManager() const { return &m_AttachmentManager; }
    virtual IDefaultSkeleton& GetIDefaultSkeleton() { return *m_pDefaultSkeleton; }
    virtual const IDefaultSkeleton& GetIDefaultSkeleton() const { return *m_pDefaultSkeleton; }
    virtual IAnimationSet* GetIAnimationSet() { return m_pDefaultSkeleton->GetIAnimationSet(); }
    virtual const IAnimationSet* GetIAnimationSet() const { return m_pDefaultSkeleton->GetIAnimationSet(); }
    virtual const char* GetModelAnimEventDatabase() const { return m_pDefaultSkeleton->GetModelAnimEventDatabaseCStr(); }

    virtual int GetObjectType() const { return m_pDefaultSkeleton->m_ObjectType; }
    virtual const char* GetFilePath() const { return m_strFilePath.c_str(); }
    void SetFilePath(const AZStd::string& filePath) {  m_strFilePath = filePath;   }
    virtual f32 GetAverageFrameTime() const { return g_AverageFrameTime; };

    virtual _smart_ptr<IMaterial> GetIMaterial() const {   return m_pInstanceMaterial ? m_pInstanceMaterial : m_pDefaultSkeleton->GetIMaterial();    }
    virtual void SetIMaterial_Instance(_smart_ptr<IMaterial> pMaterial) {    m_pInstanceMaterial = pMaterial; }
    virtual _smart_ptr<IMaterial> GetIMaterial_Instance() const    {   return m_pInstanceMaterial; }
    _smart_ptr<IMaterial> m_pInstanceMaterial;

    void SkinningTransformationsComputation(SSkinningData* pSkinningData, CDefaultSkeleton* pDefaultSkeleton, int nRenderLod, const Skeleton::CPoseData* pPoseData, f32* pSkinningTransformationsMovement);
    _smart_ptr<CDefaultSkeleton> m_pDefaultSkeleton;
    CAttachmentManager m_AttachmentManager;
    CFacialInstance* m_pFacialInstance;

    int m_nRefCounter;  // Reference count.

    static const int tripleBufferSize = 3;
    struct
    {
        SSkinningData* pSkinningData;
        int nNumBones;
        int nFrameID;
    } arrSkinningRendererData[tripleBufferSize];                                                      // triple buffered for motion blur


    AZStd::string m_strFilePath;
    uint32 m_nInstanceUpdateCounter;
    uint32 m_nLastRenderedFrameID;
    uint32 m_LastRenderedFrameID;     // Can be accessed from main thread only!
    uint32 m_RenderPass;              // Can be accessed from render thread only!

    int32 m_nAnimationLOD;

    bool m_bFacialAnimationEnabled : 1;
    uint32 m_bWasVisible : 1;
    uint32 m_bPlayedPhysAnim : 1;

    void SetWasVisible(bool wasVisible) {       m_bWasVisible = wasVisible; }
    uint32 GetWasVisible() { return m_bWasVisible; };
    uint32 GetAnimationLOD() {  return m_nAnimationLOD; }
    bool FacialAnimationEnabled() const
    {
        return m_bFacialAnimationEnabled;
    }
    virtual IFacialInstance* GetFacialInstance();
    virtual const IFacialInstance* GetFacialInstance() const;
    virtual void EnableProceduralFacialAnimation(bool bEnable);
    virtual void EnableFacialAnimation(bool bEnable);
    virtual void LipSyncWithSound(uint32 nSoundId, bool bStop = false);





    virtual void SetViewdir(const Vec3& rViewdir) { m_Viewdir = rViewdir; }
    virtual float GetUniformScale() const { return m_location.s; }

    virtual void ProcessAttachment(IAttachment* pIAttachment);



    //-------------------------------------------------------------------------------
    //-----   decals  ---------------------------------------------------------------
    //-------------------------------------------------------------------------------
    uint32 m_useDecals : 1;
    CAnimDecalManager m_DecalManager;
    virtual void EnableDecals(uint32 d) { m_useDecals = d > 0; };
    virtual void CreateDecal(CryEngineDecalInfo& DecalLCS) {};
    void AddDecalsToRenderer(CCharInstance* pMaster, CAttachmentSKIN* pAttachment, const Matrix34& RotTransMatrix34, const SRenderingPassInfo& passInfo) {};
    void FillUpDecalBuffer(CCharInstance* pMaster, DualQuat* parrNewSkinQuat, SVF_P3F_C4B_T2F* pVertices, SPipTangents* pTangents, uint16* pIndices, size_t decalIdx) {};
    void DeleteDecals() {};
    void DrawDecalsBBoxes(CCharInstance* pMaster, CAttachmentSKIN* pAttachment, const Matrix34& rRenderMat34) {};

    virtual void ComputeGeometricMean(SMeshLodInfo& lodInfo) const override;







    virtual bool HasVertexAnimation() const { return m_bHasVertexAnimation; }
    virtual bool UseMatrixSkinning() const { return m_bUseMatrixSkinning; }

    virtual const AABB& GetAABB() const;
    virtual float GetRadiusSqr() const;
    //used to refresh bounding box data
    void UpdateAABB();


    virtual ISkeletonAnim* GetISkeletonAnim() { return &m_SkeletonAnim; }
    virtual const ISkeletonAnim* GetISkeletonAnim() const { return &m_SkeletonAnim; }
    virtual ISkeletonPose* GetISkeletonPose() { return &m_SkeletonPose; }
    virtual const ISkeletonPose* GetISkeletonPose() const { return &m_SkeletonPose; }

    virtual void StartAnimationProcessing(const SAnimationProcessParams& params);
    void SkeletonPostProcess();

    //this is a hack to keep entity attachments in synch
    virtual void SetAttachmentLocation_DEPRECATED(const QuatTS& newCharacterLocation)
    {
        m_location = newCharacterLocation;
    }

    virtual void FinishAnimationComputations() { m_SkeletonAnim.FinishAnimationComputations(); }
    virtual bool CopyPoseFrom(const ICharacterInstance& instance);

    bool m_bEnableStartAnimation;
    virtual void EnableStartAnimation (bool bEnable)    {   m_bEnableStartAnimation = bEnable;  }

    uint32 m_ResetMode;
    virtual uint32 GetResetMode() const { return m_ResetMode; };
    virtual void SetResetMode(uint32 rm) { m_ResetMode = rm > 0; };

    uint32 m_CharEditMode;
    virtual uint32 GetCharEditMode() const { return m_CharEditMode; };
    virtual void SetCharEditMode(uint32 m) {  m_CharEditMode = m; };

    f32 m_fDeltaTime;
    f32 m_fOriginalDeltaTime;

    uint32 m_nForceUpdate : 1;
    uint32 m_HideMaster : 1;
    virtual void HideMaster(uint32 h) { m_HideMaster = h > 0; };

    CSkeletonAnim m_SkeletonAnim;
    CSkeletonPose m_SkeletonPose;

    uint32 GetPhysicsRelinquished() { return m_SkeletonPose.m_physics.m_bPhysicsRelinquished; };
    uint32 IsCharacterVisible() const { return m_SkeletonPose.m_bInstanceVisible; };

    void UpdatePhysicsCGA(Skeleton::CPoseData& poseData, f32 fScale, const QuatTS& rAnimLocationNext);
    void ApplyJointVelocitiesToPhysics(IPhysicalEntity* pent, const Quat& qrot = Quat(IDENTITY), const Vec3& velHost = Vec3(ZERO));
    virtual void OnDetach();
    virtual float GetExtent(EGeomForm eForm);
    virtual void GetRandomPos(PosNorm& ran, EGeomForm eForm) const;
    virtual void Serialize(TSerialize ser);
    virtual size_t SizeOfCharInstance();
    size_t SizeOfAttachmentManager()    {   return sizeof(CAttachmentManager) + m_AttachmentManager.SizeOfAllAttachments(); }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    virtual CLodValue ComputeLod(int wantedLod, const SRenderingPassInfo& passInfo) override;
    virtual void Render(const SRendParams& rParams, const QuatTS& Offset, const SRenderingPassInfo& passInfo);
    void RenderCGA(const struct SRendParams& rParams, const Matrix34& RotTransMatrix34, const SRenderingPassInfo& passInfo);
    void RenderCHR(const struct SRendParams& rParams, const Matrix34& RotTransMatrix34, const SRenderingPassInfo& passInfo);

#ifdef EDITOR_PCDEBUGCODE
    virtual void ReloadCHRPARAMS(const std::string* optionalBuffer = nullptr);
    virtual void DrawWireframeStatic(const Matrix34& m34, int nLOD, uint32 color)
    {
        CModelMesh* pModelMesh = m_pDefaultSkeleton->GetModelMesh();
        if (pModelMesh)
        {
            pModelMesh->DrawWireframeStatic(m34, color);
        }
    }
#endif

    // This is the scale factor that affects the animation speed of the character.
    // All the animations are played with the constant real-time speed multiplied by this factor.
    // So, 0 means still animations (stuck at some frame), 1 - normal, 2 - twice as fast, 0.5 - twice slower than normal.
    f32 m_fPlaybackScale;
    virtual void SetPlaybackScale(f32 speed) { m_fPlaybackScale = max(0.0f, speed); }
    virtual f32 GetPlaybackScale() const { return m_fPlaybackScale; }

    Vec3 m_Viewdir;
    QuatTS m_location;
    float m_fPostProcessZoomAdjustedDistanceFromCamera;


    uint32 m_LastUpdateFrameID_Pre;
    uint32 m_LastUpdateFrameID_Post;

    void UpdatePreviousFrameBones(int nLOD);
    bool MotionBlurMotionCheck(uint64 nObjFlags);


    bool   m_bUpdateMotionBlurSkinnning;


    virtual void SpawnSkeletonEffect(int animID, const char* animName, const char* effectName, const char* boneName, const char* secondBoneName, const Vec3& offset, const Vec3& dir, const QuatTS& entityLoc)
    {
        m_skeletonEffectManager.SpawnEffect(this, animID, animName, effectName, boneName, secondBoneName, offset, dir, entityLoc);
    }

    virtual void KillAllSkeletonEffects()
    {
        m_skeletonEffectManager.KillAllEffects();
    }

    CSkeletonEffectManager m_skeletonEffectManager;

    uint32 m_rpFlags;
    virtual void SetFlags(int nFlags) { m_rpFlags = nFlags; }
    virtual int  GetFlags() const { return m_rpFlags; }


    // Skinning
private:
    static const uint32 sSkiningTransCnt = 3;
    static const uint32 sSkinningJobCnt = 2;

    // NOTE: Skinning transformations array's elements need to be 16bytes aligned.
    // QuatTS contains 8 f32s, thus does not require padding.
    uint32 m_skinningTransformationsCount;
    f32 m_skinningTransformationsMovement;

    CGeomExtents                m_Extents;

    AZ::EntityId                m_ownerId; ///< Owning entity Id.

public:

    uint32 GetSkinningTransformationCount() const { return m_skinningTransformationsCount; }

    void BeginSkinningTransformationsComputation(SSkinningData* pSkinningData);

    virtual void AddRef()
    {
        ++m_nRefCounter;
    }

    virtual int GetRefCount() const
    {
        return m_nRefCounter;
    }

    virtual void Release()
    {
        if (--m_nRefCounter == 0)
        {
            m_AttachmentManager.RemoveAllAttachments();
            delete this;
        }
    }

    bool m_bHasVertexAnimation;
    bool m_bUseMatrixSkinning;

protected:
    CCharInstance () { }
} _ALIGN(128);


#endif // CRYINCLUDE_CRYANIMATION_CHARACTERINSTANCE_H

