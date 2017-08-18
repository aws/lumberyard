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

#ifndef CRYINCLUDE_CRY3DENGINE_BREAKABLEGLASSRENDERNODE_H
#define CRYINCLUDE_CRY3DENGINE_BREAKABLEGLASSRENDERNODE_H
#pragma once

// Includes
#include "CREBreakableGlass.h"
#include "CREBreakableGlassHelpers.h"

// Forward declares
class CREBreakableGlass;
struct IParticleEffect;

//==================================================================================================
// Name: CBreakableGlassRenderNode
// Desc: Breakable glass sim render node
// Author: Chris Bunner
//==================================================================================================
class CBreakableGlassRenderNode
    : public IBreakableGlassRenderNode
{
public:
    CBreakableGlassRenderNode();
    virtual ~CBreakableGlassRenderNode();

    // IBreakableGlassRenderNode interface
    virtual bool        InitialiseNode(const SBreakableGlassInitParams& params, const Matrix34& matrix);
    virtual void        ReleaseNode(bool bImmediate);

    virtual void        SetId(const uint16 id);
    virtual uint16  GetId();

    virtual void        Update                          (SBreakableGlassUpdateParams& params);
    virtual bool        HasGlassShattered       ();
    virtual bool        HasActiveFragments  ();
    virtual void        ApplyImpactToGlass  (const EventPhysCollision* pPhysEvent);
    virtual void        ApplyExplosionToGlass(const EventPhysCollision* pPhysEvent);
    virtual void        DestroyPhysFragment (SGlassPhysFragment* pPhysFrag);

    virtual void        SetCVars(const SBreakableGlassCVars* pCVars);

    // IRenderNode interface
    virtual const char*             GetName                         () const;
    virtual EERType                     GetRenderNodeType       ();
    virtual const char*             GetEntityClassName  () const;
    virtual void                            GetMemoryUsage          (ICrySizer* pSizer) const;
    void                            SetMaterial (_smart_ptr<IMaterial> pMaterial) override;
    virtual _smart_ptr<IMaterial>              GetMaterial                 (Vec3* pHitPos = NULL);
    virtual void                            SetMatrix                       (const Matrix34& matrix);
    virtual Vec3                            GetPos                          (bool worldOnly = true) const;
    virtual const AABB              GetBBox                         () const;
    virtual void                            SetBBox                         (const AABB& worldSpaceBoundingBox);
    virtual void                            FillBBox                        (AABB& aabb);
    virtual void                            OffsetPosition(const Vec3& delta);
    virtual float                           GetMaxViewDist          ();
    virtual IPhysicalEntity*    GetPhysics                  () const;
    virtual void                            SetPhysics                  (IPhysicalEntity* pPhysics);
    virtual void                            Render                          (const SRendParams& renderParams, const SRenderingPassInfo& passInfo);
    virtual _smart_ptr<IMaterial>              GetMaterialOverride ();

private:
    void        PhysicalizeGlass();
    void        DephysicalizeGlass();

    void        PhysicalizeGlassFragment(SGlassPhysFragment& physFrag, const Vec3& centerOffset);
    void        DephysicalizeGlassFragment(SGlassPhysFragment& physFrag);

    void        CalculateImpactPoint(const Vec3& pt, Vec2& impactPt);
    void        UpdateGlassState(const EventPhysCollision* pPhysEvent);
    void        SetParticleEffectColours(IParticleEffect* pEffect, const Vec4& rgba);
    void        PlayBreakageEffect(const EventPhysCollision* pPhysEvent);

    TGlassPhysFragmentArray     m_physFrags;
    TGlassPhysFragmentIdArray   m_deadPhysFrags;

    SBreakableGlassInitParams   m_glassParams;
    SBreakableGlassState            m_lastGlassState;

    AABB                                            m_planeBBox; // Plane's world space bounding box
    AABB                                            m_fragBBox;  // Phys fragments' world space bounding box
    Matrix34                                    m_matrix;
    float                                           m_maxViewDist;
    CREBreakableGlass*              m_pBreakableGlassRE;
    IPhysicalEntity*                    m_pPhysEnt;

    static const SBreakableGlassCVars*  s_pCVars;

    Vec4                                            m_glassTintColour;
    uint16                                      m_id;
    uint8                                           m_state;
    uint8                                           m_nextPhysFrag;
};//------------------------------------------------------------------------------------------------

#endif // CRYINCLUDE_CRY3DENGINE_BREAKABLEGLASSRENDERNODE_H
