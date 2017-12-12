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

#ifndef CRYINCLUDE_CRY3DENGINE_PARTICLEENVIRON_H
#define CRYINCLUDE_CRY3DENGINE_PARTICLEENVIRON_H
#pragma once

#include "ParticleParams.h"
#include "ParticleEffect.h"
#include "CryPtrArray.h"

class CParticleEmitter;

//////////////////////////////////////////////////////////////////////////
// Physical environment management.
//
struct SPhysEnviron
    : Cry3DEngineBase
{
    // PhysArea caching.
    SPhysForces             m_UniformForces;
    ETrinary                    m_tUnderWater;
    uint32                      m_nNonUniformFlags;     // EParticleEnviron flags of non-uniform areas found.
    uint32                      m_nNonCachedFlags;      // Env flags of areas requiring physics system access.

    // Nonuniform area support.
    struct SArea
    {
        const SPhysEnviron*
            m_pEnviron;                                                     // Parent environment.
        _smart_ptr<IPhysicalEntity>
        m_pArea;
        volatile int*           m_pLock;                            // Copy of lock for area.

        SPhysForces             m_Forces;
        AABB                            m_bbArea;                           // Bounds of area, for quick checks.
        uint32                      m_nFlags;
        bool                            m_bOutdoorOnly;             // Force only for outdoor areas.

        // Area params, for simple evaluation.
        bool                            m_bCacheForce;              // Quick cached force info.
        bool                            m_bRadial;                      // Forces are radial from center.
        uint8                           m_nGeomShape;                   // GEOM_BOX, GEOM_SPHERE, ...
        Vec3                            m_vCenter;                      // Area position (for radial forces).
        Matrix33                    m_matToLocal;                   // Convert to unit sphere space.
        float                           m_fFalloffScale;            // For scaling to inner/outer force bounds.

        SArea()
        { ZeroStruct(*this); }
        void GetForces(SPhysForces& forces, Vec3 const& vPos, uint32 nFlags) const;
        void GetForcesPhys(SPhysForces& forces, Vec3 const& vPos) const;
        float GetWaterPlane(Plane& plane, Vec3 const& vPos, float fMaxDist = -WATER_LEVEL_UNKNOWN) const;
        void GetMemoryUsage(ICrySizer* pSizer) const
        {}

        void AddRef()
        {
            m_nRefCount++;
        }
        void Release()
        {
            assert(m_nRefCount >= 0);
            if (--m_nRefCount == 0)
            {
                delete this;
            }
        }

    private:
        int                             m_nRefCount;
    };


    SPhysEnviron()
    {
        Clear();
    }

    // Phys areas
    void Clear();
    void FreeMemory();

    // Query world phys areas.
    void GetWorldPhysAreas(uint32 nFlags = ~0, bool bNonUniformAreas = true);

    // Query subset of phys areas.
    void GetPhysAreas(SPhysEnviron const& envSource, AABB const& box, bool bIndoors, uint32 nFlags = ~0, bool bNonUniformAreas = true, const CParticleEmitter* pEmitterSkip = 0);

    bool IsCurrent() const
    {
        return (m_nNonUniformFlags & EFF_LOADED) != 0;
    }

    void OnPhysAreaChange(const EventPhysAreaChange& event);

    void LockAreas(uint32 nFlags, int iLock) const
    {
        if (m_nNonCachedFlags & nFlags)
        {
            for_array (i, m_NonUniformAreas)
            {
                if (m_NonUniformAreas[i].m_nFlags & nFlags)
                {
                    if (!m_NonUniformAreas[i].m_bCacheForce)
                    {
                        CryInterlockedAdd(m_NonUniformAreas[i].m_pLock, iLock);
                    }
                }
            }
        }
    }

    void GetForces(SPhysForces& forces, Vec3 const& vPos, uint32 nFlags) const
    {
        forces = m_UniformForces;
        if (m_nNonUniformFlags & nFlags & ENV_PHYS_AREA)
        {
            GetNonUniformForces(forces, vPos, nFlags);
        }
    }

    float GetWaterPlane(Plane& plWater, Vec3 const& vPos, float fMaxDist = -WATER_LEVEL_UNKNOWN) const
    {
        plWater = m_UniformForces.plWater;
        float fDist = m_UniformForces.plWater.DistFromPlane(vPos);
        if (fDist > 0.f)
        {
            if (m_nNonUniformFlags & ENV_WATER)
            {
                fDist = GetNonUniformWaterPlane(plWater, vPos, min(fDist, fMaxDist));
            }
        }
        return fDist;
    }

    // Phys collision
    static bool PhysicsCollision(ray_hit& hit, Vec3 const& vStart, Vec3 const& vEnd, float fRadius, uint32 nEnvFlags, IPhysicalEntity* pThisEntity = 0);

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddContainer(m_NonUniformAreas);
    }

    const SmartPtrArray<SArea>& GetNonUniformAreas() const { return m_NonUniformAreas; }

protected:

    SmartPtrArray<SArea>    m_NonUniformAreas;

    void GetNonUniformForces(SPhysForces& forces, Vec3 const& vPos, uint32 nFlags) const;
    float GetNonUniformWaterPlane(Plane& plWater, Vec3 const& vPos, float fMaxDist = -WATER_LEVEL_UNKNOWN) const;
};

//////////////////////////////////////////////////////////////////////////
// Vis area management.
//
struct SVisEnviron
    : Cry3DEngineBase
{
    SVisEnviron()
    {
        Clear();
    }

    void Clear()
    {
        memset(this, 0, sizeof(*this));
    }

    void Invalidate()
    {
        m_bValid = false;
    }

    void Update(Vec3 const& vOrigin, AABB const& bbExtents);

    IVisArea* GetClipVisArea(IVisArea* pVisAreaCam, AABB const& bb) const;

    bool ClipVisAreas(IVisArea* pClipVisArea, Sphere& sphere, Vec3 const& vNormal) const
    {
        // Clip inside or outside specified area.
        return pClipVisArea->ClipToVisArea(pClipVisArea == m_pVisArea, sphere, vNormal);
    }

    bool OriginIndoors() const
    {
        return m_pVisArea != 0;
    }

    void OnVisAreaDeleted(IVisArea* pVisArea)
    {
        if (m_pVisArea == pVisArea)
        {
            Clear();
        }
    }

protected:
    bool                            m_bValid;                           // True if VisArea determination up to date.
    bool                            m_bCrossesVisArea;
    IVisArea*                   m_pVisArea;                     // VisArea emitter is in, if needed and if any.
    void*                           m_pVisNodeCache;
};

//////////////////////////////////////////////////////////////////////////
// GeomRef functions.

struct GeomRef;

//////////////////////////////////////////////////////////////////////////
// Compact version of GeomRef, with a union type to preserve space in CParticle.
// Also provides methods for attachment functionality.
// No automatic reference-counting, AddRef and Release provide explicit counting.
struct SEmitGeom
{
protected:

    enum
    {
        uIsCentered = 1, uIsCharacter = 2, uFlags = 3
    };                                                      // Flags stored in lower bits of StatObj ptr
    union
    {
        void*                               m_pMeshObj;                     // Render object attachment. Can hold an IStatObj* OR an ICharacterInstance*
        size_t                          m_uFlags;                           // 1 bit identifies which mesh type it is, another bit for centering
    };
    IPhysicalEntity*            m_pPhysEnt;                     // Physics object attachment.

public:

    SEmitGeom()
    {
        m_pMeshObj = 0;
        m_pPhysEnt = 0;
    }
    SEmitGeom(const SEmitGeom& o)
    {
        m_pMeshObj = o.m_pMeshObj;
        m_pPhysEnt = o.m_pPhysEnt;
    }
    SEmitGeom(const GeomRef& geom)
    {
        Set(geom.m_pStatObj, geom.m_pChar);
        Set(geom.m_pPhysEnt);
    }
    SEmitGeom(const GeomRef& geom, EGeomType eAttachType);

    void Set(IStatObj* pStatobj)
    {
        m_pMeshObj = pStatobj;
        assert(!(m_uFlags & uFlags));
    }
    void Set(ICharacterInstance* pChar)
    {
        m_pMeshObj = (IStatObj*)pChar;
        assert(!(m_uFlags & uFlags));
        m_uFlags |= uIsCharacter;
    }
    void Set(IStatObj* pStatobj, ICharacterInstance* pChar)
    {
        if (pChar)
        {
            Set(pChar);
        }
        else
        {
            Set(pStatobj);
        }
    }
    void Set(IPhysicalEntity* pPhys)
    {
        m_pPhysEnt = pPhys;
    }
    void SetCentered(bool bCenter)
    {
        if (bCenter)
        {
            m_uFlags |= uIsCentered;
        }
        else
        {
            m_uFlags &= ~uIsCentered;
        }
    }

    void AddRef() const;
    void Release() const;

    operator bool() const
    {
        return (m_uFlags & ~uFlags) || m_pPhysEnt;
    }
    bool operator == (const SEmitGeom& other) const
    { return m_pMeshObj == other.m_pMeshObj && m_pPhysEnt == other.m_pPhysEnt; }
    bool operator != (const SEmitGeom& other) const
    { return m_pMeshObj != other.m_pMeshObj || m_pPhysEnt != other.m_pPhysEnt; }

    IStatObj* GetStatObj() const
    { return !(m_uFlags & uIsCharacter) ? (IStatObj*)(m_uFlags & ~uFlags) : 0; }
    ICharacterInstance* GetCharacter() const
    { return (m_uFlags & uIsCharacter) ? (ICharacterInstance*)(m_uFlags & ~uFlags) : 0; }
    IPhysicalEntity* GetPhysicalEntity() const
    { return m_pPhysEnt; }
    bool IsCentered() const
    { return m_uFlags & uIsCentered; }

    void GetAABB(AABB& bb, QuatTS const& tLoc) const;
    float GetExtent(EGeomForm eForm) const;
    void GetRandomPos(PosNorm& ran, EGeomForm eForm, QuatTS const& tWorld) const;
};

#endif // CRYINCLUDE_CRY3DENGINE_PARTICLEENVIRON_H
