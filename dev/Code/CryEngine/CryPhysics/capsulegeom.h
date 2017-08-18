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

#ifndef CRYINCLUDE_CRYPHYSICS_CAPSULEGEOM_H
#define CRYINCLUDE_CRYPHYSICS_CAPSULEGEOM_H
#pragma once

class CCapsuleGeom
    : public CCylinderGeom
{
public:
    CCapsuleGeom() {}
    CCapsuleGeom* CreateCapsule(capsule* pcaps);
    virtual int GetType() { return GEOM_CAPSULE; }
    virtual void SetData(const primitive* pcaps) { CreateCapsule((capsule*)pcaps); }
    virtual int PreparePrimitive(geom_world_data* pgwd, primitive*& pprim, int iCaller = 0)
    {
        CCylinderGeom::PreparePrimitive(pgwd, pprim, iCaller);
        return capsule::type;
    }
    virtual int CalcPhysicalProperties(phys_geometry* pgeom);
    virtual int FindClosestPoint(geom_world_data* pgwd, int& iPrim, int& iFeature, const Vec3& ptdst0, const Vec3& ptdst1,
        Vec3* ptres, int nMaxIters);
    virtual int PointInsideStatus(const Vec3& pt);
    virtual float CalculateBuoyancy(const plane* pplane, const geom_world_data* pgwd, Vec3& massCenter);
    virtual void CalculateMediumResistance(const plane* pplane, const geom_world_data* pgwd, Vec3& dPres, Vec3& dLres);
    virtual int DrawToOcclusionCubemap(const geom_world_data* pgwd, int iStartPrim, int nPrims, int iPass, SOcclusionCubeMap* cubeMap);
    virtual int UnprojectSphere(Vec3 center, float r, float rsep, contact* pcontact);
    virtual float GetVolume() { return sqr(m_cyl.r) * m_cyl.hh * (g_PI * 2) + (4.0f / 3 * g_PI) * cube(m_cyl.r); }
    virtual int PrepareForIntersectionTest(geometry_under_test* pGTest, CGeometry* pCollider, geometry_under_test* pGTestColl, bool bKeepPrevContacts = false);
    virtual int GetUnprojectionCandidates(int iop, const contact* pcontact, primitive*& pprim, int*& piFeature, geometry_under_test* pGTest);
};

#endif // CRYINCLUDE_CRYPHYSICS_CAPSULEGEOM_H
