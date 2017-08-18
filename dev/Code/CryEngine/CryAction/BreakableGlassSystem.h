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

#ifndef CRYINCLUDE_CRYACTION_BREAKABLEGLASSSYSTEM_H
#define CRYINCLUDE_CRYACTION_BREAKABLEGLASSSYSTEM_H
#pragma once

#include <IBreakableGlassSystem.h>

// Forward decls
struct IBreakableGlassRenderNode;
struct SBreakableGlassCVars;
struct SBreakableGlassPhysData;

//==================================================================================================
// Name: CBreakableGlassSystem
// Desc: Management system for breakable glass nodes
// Authors: Chris Bunner
//==================================================================================================
class CBreakableGlassSystem
    : public IBreakableGlassSystem
{
public:
    CBreakableGlassSystem();
    virtual ~CBreakableGlassSystem();

    static int      HandleImpact(const EventPhys* pPhysEvent);
    static void     OnEnabledCVarChange(ICVar* pCVar);

    virtual void    Update(const float frameTime);
    virtual bool    BreakGlassObject(const EventPhysCollision& physEvent, const bool forceGlassBreak = false);
    virtual void    ResetAll();
    virtual void    GetMemoryUsage(ICrySizer* pSizer) const;

private:
    bool                    ExtractPhysDataFromEvent(const EventPhysCollision& physEvent, SBreakableGlassPhysData& data, SBreakableGlassInitParams& initParams);

    bool                    ValidatePhysMesh(mesh_data* pPhysMesh, const int thinAxis);
    bool                    ExtractPhysMesh(mesh_data* pPhysMesh, const int thinAxis, const primitives::box& bbox, TGlassDefFragmentArray& fragOutline);
    void                    MergeFragmentTriangles(mesh_data* pPhysMesh, const int tri, const Vec3& thinRow, TGlassFragmentIndexArray& touchedTris, TGlassFragmentIndexArray& frag);
    void                    ExtractUVCoords(IStatObj* const pStatObj, const primitives::box& bbox, const int thinAxis, SBreakableGlassPhysData& data);
    Vec2                    InterpolateUVCoords(const Vec4& uvPt0, const Vec4& uvPt1, const Vec4& uvPt2, const Vec2& pt);

    bool                    ValidateExtractedOutline(SBreakableGlassPhysData& data, SBreakableGlassInitParams& initParams);
    void                    CalculateGlassBounds(const phys_geometry* const pPhysGeom, Vec3& size, Matrix34& matrix);
    void                    CalculateGlassAnchors(SBreakableGlassPhysData& data, SBreakableGlassInitParams& initParams);
    void                    PassImpactToNode(IBreakableGlassRenderNode* pRenderNode, const EventPhysCollision* pPhysEvent);

    void                    ModifyEventToForceBreak(EventPhysCollision* pPhysEvent);
    void                    AssertUnusedIfDisabled();

    IBreakableGlassRenderNode*  InitGlassNode(const SBreakableGlassPhysData& physData, SBreakableGlassInitParams& initParams, const Matrix34& transMat);
    void                                                ReleaseGlassNodes();

    PodArray<IBreakableGlassRenderNode*> m_glassPlanes;

    SBreakableGlassCVars*               m_pGlassCVars;
    bool                                                m_enabled;
};//------------------------------------------------------------------------------------------------

#endif // CRYINCLUDE_CRYACTION_BREAKABLEGLASSSYSTEM_H
