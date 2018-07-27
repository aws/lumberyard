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

#ifndef CRYINCLUDE_CRYANIMATION_ATTACHMENTMANAGER_H
#define CRYINCLUDE_CRYANIMATION_ATTACHMENTMANAGER_H
#pragma once

#include "Vertex/VertexData.h"
#include "Vertex/VertexAnimation.h"
#include "ModelSkin.h"
#include "Skeleton.h"

#include "AttachmentFace.h"
#include "AttachmentBone.h"
#include "AttachmentSkin.h"
#include "AttachmentProxy.h"
#include "GeomQuery.h"

struct CharacterAttachment;

class CAttachmentManager
    : public IAttachmentManager
{
    friend class CAttachmentBONE;
    friend class CAttachmentFACE;
    friend class CAttachmentSKIN;

public:
    CAttachmentManager()
    {
        m_pSkelInstance = NULL;
        m_nHaveEntityAttachments = 0;
        m_numRedirectionWithAttachment = 0;
        m_fZoomDistanceSq = 0;
        m_arrAttachments.reserve(0x20);
        m_TypeSortingRequired = 0;
        m_nDrawProxies = 0;
        memset(&m_sortop, 0, sizeof(m_sortop));
        m_fTurbulenceGlobal = 0;
        m_fTurbulenceLocal = 0;
    };

    uint32 LoadAttachmentList(const char* pathname);
    static uint32 ParseXMLAttachmentList(CharacterAttachment* parrAttachments, uint32 numAttachments, XmlNodeRef nodeAttachements);
    void InitAttachmentList(const DynArray<CharacterAttachment>& parrAttachments, const AZStd::string& pathname, uint32 nLoadingFlags, int nKeepModelInMemory);


    IAttachment* CreateAttachment(const char* szAttName, uint32 type, const char* szFirstJointName = 0, bool bCallProject = true, const char* szSecondJointName = 0);

    int32 GetAttachmentCount() const { return m_arrAttachments.size(); };

    IAttachment* GetInterfaceByIndex(uint32 c) const;
    IAttachment* GetInterfaceByName(const char* szName) const;
    IAttachment* GetInterfaceByNameCRC(uint32 nameCRC) const;

    int32 GetIndexByName(const char* szName) const;
    int32 GetIndexByNameCRC(uint32 nameCRC) const;

    void AddEntityAttachment()
    {
        m_nHaveEntityAttachments++;
    }
    void RemoveEntityAttachment()
    {
        if (m_nHaveEntityAttachments > 0)
        {
            m_nHaveEntityAttachments--;
        }
    }

    bool NeedsHierarchicalUpdate();
#ifdef EDITOR_PCDEBUGCODE
    void Verification();
#endif

    void UpdateAllRemapTables();
    void UpdateAllRedirectedTransformations(Skeleton::CPoseData& pPoseData);
    void UpdateAllLocations(Skeleton::CPoseData& pPoseData);

    void UpdateAllLocationsFast(Skeleton::CPoseData& pPoseData);
    void ProcessAllAttachedObjectsFast();

    void DrawAttachments(const SRendParams& rRendParams, const Matrix34& m, const SRenderingPassInfo& passInfo);

    void RemoveAttachmentByIndex(uint32 n);
    int32 RemoveAttachmentByInterface(const IAttachment* ptr);
    int32 RemoveAttachmentByName(const char* szName);
    int32 RemoveAttachmentByNameCRC(uint32 nameCRC);
    uint32 RemoveAllAttachments();
    uint32 ProjectAllAttachment();

    void PhysicalizeAttachment(int idx, int nLod, IPhysicalEntity* pent, const Vec3& offset);
    int UpdatePhysicalizedAttachment(int idx, IPhysicalEntity* pent, const QuatT& offset);
    int UpdatePhysAttachmentHideState(int idx, IPhysicalEntity* pent, const Vec3& offset);

    virtual void PhysicalizeAttachment(int idx, IPhysicalEntity* pent = 0, int nLod = 0);
    virtual void DephysicalizeAttachment(int idx, IPhysicalEntity* pent = 0);

    void Serialize(TSerialize ser);


    virtual ICharacterInstance* GetSkelInstance() const;
    ILINE bool IsFastUpdateType(IAttachmentObject::EType eAttachmentType) const
    {
        if (eAttachmentType == IAttachmentObject::eAttachment_Entity)
        {
            return true;
        }
        if (eAttachmentType == IAttachmentObject::eAttachment_Effect)
        {
            return true;
        }
        return false;
    }

    virtual IProxy* CreateProxy(const char* szName, const char* szJointName);
    virtual int32 GetProxyCount() const { return m_arrProxies.size(); }

    virtual IProxy* GetProxyInterfaceByIndex(uint32 c) const;
    virtual IProxy* GetProxyInterfaceByName(const char* szName) const;
    virtual IProxy* GetProxyInterfaceByCRC(uint32 nameCRC) const;

    virtual int32 GetProxyIndexByName(const char* szName) const;
    virtual int32 GetProxyIndexByCRC(uint32 nameCRC) const;

    virtual int32 RemoveProxyByInterface(const IProxy* ptr);
    virtual int32 RemoveProxyByName(const char* szName);
    virtual int32 RemoveProxyByNameCRC(uint32 nameCRC);
    void RemoveProxyByIndex(uint32 n);
    virtual void DrawProxies(uint32 enable){(enable & 0x80) ? m_nDrawProxies &= enable : m_nDrawProxies |= enable; }
    void VerifyProxyLinks();

    virtual uint32 GetProcFunctionCount() const { return 5; }
    virtual const char* GetProcFunctionName(uint32 idx) const;

    virtual void ClearAttachmentData();
    virtual void RebindAttachments(uint32 nLoadingFlags);

    const char* ExecProcFunction(uint32 nCRC32, Skeleton::CPoseData* pPoseData, const char* pstrFunction = 0) const;

    float GetExtent(EGeomForm eForm);
    void GetRandomPos(PosNorm& ran, EGeomForm eForm) const;
#if !defined(_RELEASE)
    float DebugDrawAttachment(IAttachment* pAttachment, ISkin* pSkin, Vec3 drawLoc, _smart_ptr<IMaterial> pMaterial, float drawScale);
#endif

public:
    uint32 m_TypeSortingRequired;
    void SortByType();
    size_t SizeOfAllAttachments();
    void GetMemoryUsage(ICrySizer* pSizer) const;
    ILINE uint32 GetMinJointAttachments() { return m_sortop[0][bs]; }
    ILINE uint32 GetMaxJointAttachments() { return m_sortop[1][bx]; }
    ILINE uint32 GetRedirectedJointCount() { return m_sortop[1][br] - m_sortop[0][br]; }

    CGeomExtents m_Extents;
    CCharInstance* m_pSkelInstance;
    DynArray<_smart_ptr<IAttachment> > m_arrAttachments;
    DynArray<CProxy> m_arrProxies;
    DynArray<_smart_ptr<CAttachmentBONE> > m_arrProcExec;
    uint32 m_nDrawProxies;
    f32 m_fTurbulenceGlobal, m_fTurbulenceLocal;
private:
    f32 m_fZoomDistanceSq;
    uint32 m_nHaveEntityAttachments;
    uint32 m_numRedirectionWithAttachment;

    //initial research (what it looks like these represent)
    //AttachmentBone R-edirect
    //AttachmentBone E-mpty, AttachmentBone S-tatic, AttachmentBone eX-ecute
    //AttachmentFace E-mpty, AttachmentFace S-tatic, AttachmentFace eX-ecute
    //Skinned Mesh,Vertex Cloth, bc not used 
    enum
    {
        br, be, bs, bx, fe, fs, fx,  sm, vc, bc,  x
    };
    uint16 m_sortop[2][x];
};



#endif // CRYINCLUDE_CRYANIMATION_ATTACHMENTMANAGER_H
