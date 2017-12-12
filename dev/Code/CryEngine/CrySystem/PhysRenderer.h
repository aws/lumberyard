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

// Description : declaration of a simple dedicated renderer for the physics subsystem


#ifndef CRYINCLUDE_CRYSYSTEM_PHYSRENDERER_H
#define CRYINCLUDE_CRYSYSTEM_PHYSRENDERER_H
#pragma once

#include <IPhysicsDebugRenderer.h>

struct SRayRec
{
    Vec3 origin;
    Vec3 dir;
    float time;
    int idxColor;
};

struct SGeomRec
{
    int itype;
    char buf[sizeof(primitives::box)];
    Vec3 offset;
    Matrix33 R;
    float scale;
    Vec3 sweepDir;
    float time;
};


class CPhysRenderer
    : public IPhysicsDebugRenderer
    , public IPhysRenderer
{
public:
    CPhysRenderer();
    ~CPhysRenderer();
    void Init();
    void DrawGeometry(IGeometry* pGeom, geom_world_data* pgwd, const ColorB& clr, const Vec3& sweepDir = Vec3(0));
    void DrawGeometry(int itype, const void* pGeomData, geom_world_data* pgwd, const ColorB& clr, const Vec3& sweepDir = Vec3(0));
    Vec3 SetOffset(const Vec3& offs = Vec3(ZERO)) { Vec3 prev = m_offset; m_offset = offs; return prev; }

    // IPhysRenderer
    virtual void DrawFrame(const Vec3& pnt, const Vec3* axes, const float scale, const Vec3* limits, const int axes_locked);
    virtual void DrawGeometry(IGeometry* pGeom, geom_world_data* pgwd, int idxColor = 0, int bSlowFadein = 0, const Vec3& sweepDir = Vec3(0));
    virtual void DrawLine(const Vec3& pt0, const Vec3& pt1, int idxColor = 0, int bSlowFadein = 0);
    virtual void DrawText(const Vec3& pt, const char* txt, int idxColor, float saturation = 0)
    {
        if (!m_camera.IsPointVisible(pt))
        {
            return;
        }
        float clr[4] = { min(saturation * 2, 1.0f), 0, 0, 1 };
        clr[1] = min((1 - saturation) * 2, 1.0f) * 0.5f;
        clr[idxColor + 1] = min((1 - saturation) * 2, 1.0f);
        gEnv->pRenderer->DrawLabelEx(pt, 1.5f, clr, true, true, txt);
    }
    static const char* GetPhysForeignName(PhysicsForeignData pForeignData, int iForeignData, int iForeignFlags);
    virtual const char* GetForeignName(PhysicsForeignData pForeignData, int iForeignData, int iForeignFlags)
    { return GetPhysForeignName(pForeignData, iForeignData, iForeignFlags); }
    // ^^^

    // IPhysicsDebugRenderer
    virtual void UpdateCamera(const CCamera& camera);
    virtual void DrawAllHelpers(IPhysicalWorld* world);
    virtual void DrawEntityHelpers(IPhysicalWorld* world, int entityId, int iDrawHelpers);
    virtual void DrawEntityHelpers(IPhysicalEntity* entity, int iDrawHelpers);
    virtual void Flush(float dt);
    // ^^^

    float m_cullDist, m_wireframeDist;
    float m_timeRayFadein;
    float m_rayPeakTime;
    int m_maxTris, m_maxTrisRange;

protected:
    CCamera m_camera;
    IRenderAuxGeom* m_pAuxRenderer;
    IRenderer* m_pRenderer;
    SRayRec* m_rayBuf;
    int m_szRayBuf, m_iFirstRay, m_iLastRay, m_nRays;
    SGeomRec* m_geomBuf;
    int m_szGeomBuf, m_iFirstGeom, m_iLastGeom, m_nGeoms;
    IGeometry* m_pRayGeom;
    primitives::ray* m_pRay;
    Vec3 m_offset;
    static ColorB g_colorTab[9];
    volatile int m_lockDrawGeometry;
};

#endif // CRYINCLUDE_CRYSYSTEM_PHYSRENDERER_H
