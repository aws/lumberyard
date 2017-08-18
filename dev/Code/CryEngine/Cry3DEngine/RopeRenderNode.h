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

#ifndef CRYINCLUDE_CRY3DENGINE_ROPERENDERNODE_H
#define CRYINCLUDE_CRY3DENGINE_ROPERENDERNODE_H
#pragma once

#include <ISplines.h>
#include <IDeferredCollisionEvent.h>


class CRopeRenderNode
    : public IRopeRenderNode
    , public Cry3DEngineBase
{
public:
    static void StaticReset();

public:
    //////////////////////////////////////////////////////////////////////////
    // implements IRenderNode
    virtual void GetLocalBounds(AABB& bbox);
    virtual void SetMatrix(const Matrix34& mat);

    virtual EERType GetRenderNodeType();
    virtual const char* GetEntityClassName() const;
    virtual const char* GetName() const;
    virtual Vec3 GetPos(bool bWorldOnly = true) const;
    virtual void Render(const SRendParams& rParam, const SRenderingPassInfo& passInfo);
    virtual IPhysicalEntity* GetPhysics() const;
    virtual void SetPhysics(IPhysicalEntity*);
    virtual void Physicalize(bool bInstant = false);
    virtual void Dephysicalize(bool bKeepIfReferenced = false);
    void SetMaterial(_smart_ptr<IMaterial> pMat) override;
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = 0);
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return m_pMaterial; }
    virtual float GetMaxViewDist();
    virtual void Precache();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void LinkEndPoints();
    virtual const AABB GetBBox() const { return m_WSBBox; }
    virtual void SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; m_bNeedToReRegister = true; }
    virtual void FillBBox(AABB& aabb);
    virtual void OffsetPosition(const Vec3& delta);

    virtual void   SetEntityOwner(uint32 nEntityId) { m_nEntityOwnerId = nEntityId; }
    virtual uint32 GetEntityOwner() const { return m_nEntityOwnerId; }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IRopeRenderNode implementation
    //////////////////////////////////////////////////////////////////////////
    virtual void SetName(const char* sNodeName);
    virtual void SetParams(const SRopeParams& params);
    virtual const IRopeRenderNode::SRopeParams& GetParams() const;

    virtual void SetPoints(const Vec3* pPoints, int nCount);
    virtual int GetPointsCount() const;
    virtual const Vec3* GetPoints() const;

    virtual uint32 GetLinkedEndsMask() { return m_nLinkedEndsMask; };
    virtual void OnPhysicsPostStep();

    virtual void ResetPoints();

    virtual void LinkEndEntities(IPhysicalEntity* pStartEntity, IPhysicalEntity* pEndEntity);
    virtual void GetEndPointLinks(SEndPointLink* links);

    // Sound related
    virtual void SetRopeSound(char const* const pcSoundName, int unsigned const nSegmentToAttachTo, float const fOffset);
    virtual void StopRopeSound();
    virtual void ResetRopeSound();
    //////////////////////////////////////////////////////////////////////////

public:
    CRopeRenderNode();

    void CreateRenderMesh();
    void UpdateRenderMesh();
    void AnchorEndPoints(pe_params_rope& pr);
    void SyncWithPhysicalRope(bool bForce);
    bool RenderDebugInfo(const SRendParams& rParams, const SRenderingPassInfo& passInfo);

private:
    ~CRopeRenderNode();

private:
    string m_sName;
    uint32 m_nEntityOwnerId;
    Vec3 m_pos;
    AABB m_localBounds;
    Matrix34 m_worldTM;
    Matrix34 m_InvWorldTM;
    _smart_ptr<IMaterial>           m_pMaterial;
    _smart_ptr<IRenderMesh>     m_pRenderMesh;
    IPhysicalEntity* m_pPhysicalEntity;

    uint32 m_nLinkedEndsMask;

    // Flags
    uint32 m_bModified : 1;
    uint32 m_bRopeCreatedInsideVisArea : 1;
    uint32 m_bNeedToReRegister : 1;
    uint32 m_bStaticPhysics : 1;

    std::vector<Vec3> m_points;
    std::vector<Vec3> m_physicsPoints;

    SRopeParams m_params;

    typedef spline::CatmullRomSpline<Vec3> SplineType;
    SplineType m_spline;

    AABB m_WSBBox;

    // Sound related
    void UpdateSound();

    struct SRopeSoundData
    {
        SRopeSoundData()
            :   nSegementToAttachTo(1)
            , fOffset(0.0f){}
        //tSoundID nSoundID;    //DEPREC: [RopeRenderNode.h]
        int nSegementToAttachTo;
        float fOffset;
    } m_ropeSoundData;
};


#endif // CRYINCLUDE_CRY3DENGINE_ROPERENDERNODE_H
