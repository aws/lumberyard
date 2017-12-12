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

#ifndef CRYINCLUDE_CRYANIMATION_ATTACHMENTSKIN_H
#define CRYINCLUDE_CRYANIMATION_ATTACHMENTSKIN_H
#pragma once

#include "AttachmentBase.h"
#include "Vertex/VertexData.h"
#include "Vertex/VertexAnimation.h"
#include "ModelSkin.h"

extern QuatT g_IdentityQuatT; // So we don't have to include all of AnimationBase.h

class CModelMesh;
struct SVertexAnimationJob;

//////////////////////////////////////////////////////////////////////
// Special value to indicate a Skinning Transformation job is still running
#define SKINNING_TRANSFORMATION_RUNNING_INDICATOR (reinterpret_cast<SSkinningData*>(azlossy_cast<intptr_t>(-1)))

class CAttachmentSKIN
    : public IAttachment
    , public IAttachmentSkin
    , public SAttachmentBase
{
public:
    CAttachmentSKIN()
    {
        for (uint32 j = 0; j < 2; ++j)
        {
            m_pRenderMeshsSW[j] = NULL;
        }
        memset(m_arrSkinningRendererData, 0, sizeof(m_arrSkinningRendererData));
        m_pModelSkin = 0;
    };

    ~CAttachmentSKIN();

    virtual void AddRef()
    {
        ++m_nRefCounter;
    }

    virtual void Release()
    {
        if (--m_nRefCounter == 0)
        {
            delete this;
        }
    }

    virtual uint32 GetType() const { return CA_SKIN; }
    virtual uint32 SetJointName(const char* szJointName = 0) {return 0; } //not possible

    virtual const char* GetName() const { return m_strSocketName; };
    virtual uint32 GetNameCRC() const { return m_nSocketCRC32; }
    virtual uint32 ReName(const char* strSocketName, uint32 crc) {    m_strSocketName.clear();    m_strSocketName = strSocketName; m_nSocketCRC32 = crc;    return 1;   };

    virtual uint32 GetFlags() { return m_AttFlags; }
    virtual void SetFlags(uint32 flags) { m_AttFlags = flags; }

    void ReleaseRemapTablePair();
    void ReleaseSoftwareRenderMeshes();

    virtual uint32 AddBinding(IAttachmentObject* pIAttachmentObject, _smart_ptr<ISkin> pISkin = nullptr, uint32 nLoadingFlags = 0);
    void Rebind(uint32 nLoadingFlags);
    bool PopulateRemapTable(IDefaultSkeleton* pDefaultSkeleton, uint32 nLoadingFlags);
    void PatchRemapping(CDefaultSkeleton* pDefaultSkeleton);
    virtual void ClearBinding(uint32 nLoadingFlags = 0);
    virtual uint32 SwapBinding(IAttachment* pNewAttachment);
    virtual IAttachmentObject* GetIAttachmentObject() const { return m_pIAttachmentObject; }
    virtual IAttachmentSkin* GetIAttachmentSkin() { return this; }

    virtual void HideAttachment(uint32 x)
    {
        if (x)
        {
            m_AttFlags |= (FLAGS_ATTACH_HIDE_MAIN_PASS | FLAGS_ATTACH_HIDE_SHADOW_PASS | FLAGS_ATTACH_HIDE_RECURSION);
        }
        else
        {
            m_AttFlags &= ~(FLAGS_ATTACH_HIDE_MAIN_PASS | FLAGS_ATTACH_HIDE_SHADOW_PASS | FLAGS_ATTACH_HIDE_RECURSION);
        }
    }
    virtual uint32 IsAttachmentHidden() { return m_AttFlags & FLAGS_ATTACH_HIDE_MAIN_PASS; }
    virtual void HideInRecursion(uint32 x)
    {
        if (x)
        {
            m_AttFlags |= FLAGS_ATTACH_HIDE_RECURSION;
        }
        else
        {
            m_AttFlags &= ~FLAGS_ATTACH_HIDE_RECURSION;
        }
    }
    virtual uint32 IsAttachmentHiddenInRecursion() { return m_AttFlags & FLAGS_ATTACH_HIDE_RECURSION; }
    virtual void HideInShadow(uint32 x)
    {
        if (x)
        {
            m_AttFlags |= FLAGS_ATTACH_HIDE_SHADOW_PASS;
        }
        else
        {
            m_AttFlags &= ~FLAGS_ATTACH_HIDE_SHADOW_PASS;
        }
    }
    virtual uint32 IsAttachmentHiddenInShadow() { return m_AttFlags & FLAGS_ATTACH_HIDE_SHADOW_PASS; }



    virtual void SetAttAbsoluteDefault(const QuatT& qt) {   };
    virtual void SetAttRelativeDefault(const QuatT& qt) {   };
    virtual const QuatT& GetAttAbsoluteDefault() const { return g_IdentityQuatT; };
    virtual const QuatT& GetAttRelativeDefault() const { return g_IdentityQuatT; };

    virtual const QuatT& GetAttModelRelative() const { return g_IdentityQuatT;  }; //this is relative to the animated bone
    virtual const QuatTS GetAttWorldAbsolute() const;
    virtual void UpdateAttModelRelative();

    virtual uint32 GetJointID() const { return -1; };
    virtual void AlignJointAttachment() {};
    virtual uint32 ProjectAttachment(const char* szJointName = 0)  { return 0; };

    virtual void Serialize(TSerialize ser);
    virtual size_t SizeOfThis();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void TriggerMeshStreaming(uint32 nDesiredRenderLOD, const SRenderingPassInfo& passInfo);

    void DrawAttachment(SRendParams& rParams, const SRenderingPassInfo& passInfo, const Matrix34& rWorldMat34, f32 fZoomFactor = 1);
    void RecreateDefaultSkeleton(CCharInstance* pInstanceSkel, uint32 nLoadingFlags);
    void UpdateRemapTable();

    // function to keep in sync ref counts on skins and cleanup of remap tables
    void ReleaseModelSkin();

    // Vertex Transformation
    SSkinningData* GetVertexTransformationData(const bool bVertexAnimation, uint8 nRenderLOD);
    _smart_ptr<IRenderMesh> CreateVertexAnimationRenderMesh(uint lod, uint id);
    void CullVertexFrames(const SRenderingPassInfo& passInfo, float fDistance);

#ifdef EDITOR_PCDEBUGCODE
    void DrawVertexDebug(IRenderMesh* pRenderMesh, const QuatT& location,    const SVertexAnimationJob* pVertexAnimation,    const SVertexSkinData& vertexSkinData);
    void DrawWireframeStatic(const Matrix34& m34, int nLOD, uint32 color);
    void SoftwareSkinningDQ_VS_Emulator(CModelMesh* pModelMesh, Matrix34 rRenderMat34, uint8 tang, uint8 binorm, uint8 norm, uint8 wire, const DualQuat* const pSkinningTransformations);
#endif

    //////////////////////////////////////////////////////////////////////////
    //IAttachmentSkin implementation
    //////////////////////////////////////////////////////////////////////////
    virtual IVertexAnimation* GetIVertexAnimation() { return &m_vertexAnimation; }
    virtual ISkin* GetISkin() { return m_pModelSkin; };
    virtual float GetExtent(EGeomForm eForm);
    virtual void GetRandomPos(PosNorm& ran, EGeomForm eForm) const;
    virtual void ComputeGeometricMean(SMeshLodInfo& lodInfo) const override;

    int GetGuid() const;

    //---------------------------------------------------------

    //just for skin-attachments
    DynArray<JointIdType> m_arrRemapTable;  // maps skin's bone indices to skeleton's bone indices
    _smart_ptr<CSkin> m_pModelSkin;
    _smart_ptr<IRenderMesh> m_pRenderMeshsSW[2];
    string m_sSoftwareMeshName;
    CVertexData m_vertexData;
    CVertexAnimation m_vertexAnimation;
    // history for skinning data, needed for motion blur
    static const int tripleBufferSize = 3;
    struct
    {
        SSkinningData* pSkinningData;
        int nNumBones;
        int nFrameID;
    } m_arrSkinningRendererData[tripleBufferSize];                                                      // triple buffered for motion blur

    //! Lock whenever creating/releasing bone remappings
    AZStd::mutex m_remapMutex;

    //! Keep track of whether the bone remapping has been done or not to prevent multiple remappings from occurring in multi-threaded situations
    AZStd::vector<bool> m_hasRemapped;
};

#endif // CRYINCLUDE_CRYANIMATION_ATTACHMENTSKIN_H
