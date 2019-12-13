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

#ifdef LY_TERRAIN_RUNTIME

#ifndef CRYINCLUDE_CRY3DENGINE_TERRAIN_RENDER_NODE_H
#define CRYINCLUDE_CRY3DENGINE_TERRAIN_RENDER_NODE_H
#pragma once

#include <IMaterial.h>
#include <IRenderMesh.h>
#include <IEntityRenderState.h>
#include <Cry3DEngineBase.h>

struct ShadowMapFrustum;
struct SRendParams;
struct IMaterial;

namespace Terrain
{
    class CRETerrain;

    struct TerrainRenderNode
        : public Cry3DEngineBase
        , public IRenderNode
    {
    public:
        explicit TerrainRenderNode(AZStd::string_view terrainSystemMaterialName);
        virtual ~TerrainRenderNode();

        CRETerrain* getTerrainRE() const { return m_terrainRE; }

        // called directly from COctreeNode::RenderTerrainSystemObjects and ObjectsTree_MT
        void Render(const struct SRendParams& EntDrawParams, const SRenderingPassInfo& passInfo) override;



        // *****************
        // from IRenderNode, many are nops to cover abstracts

        bool CanExecuteRenderAsJob() override { return true; }

        const char* GetName() const override { return "TERRAIN_SYSTEM"; }
        const char* GetEntityClassName() const override { return "CRETerrain"; }

        Vec3 GetPos(bool bWorldOnly = true) const override { return Vec3(ZERO); }
        const AABB GetBBox() const override
        {
            // we really want a signal to say we're infinite, but we'll settle for biggest
            // return AABB(MAX_BB);
            // you would think it's the above, but later checks enforce the below
            // from ObjMan.h, which was too complex to include here #define MAX_VALID_OBJECT_VOLUME (10000000000.f)
            float max_valid_object_volume = 10000000000.f;
            return AABB(floorf(sqrtf(max_valid_object_volume / 3.0f) / 2.0f));
        }
        void SetBBox(const AABB& WSBBox) override {}
        void OffsetPosition(const Vec3& delta) override {}

        struct IPhysicalEntity* GetPhysics() const override { return nullptr; }
        void SetPhysics(IPhysicalEntity* pPhys) override {}

        void SetMaterial(_smart_ptr<IMaterial> pMat) override { m_material = pMat; };
        _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = nullptr) override { return m_material; }
        _smart_ptr<IMaterial> GetMaterialOverride() override { return nullptr; }

        float GetMaxViewDist() override { return FLT_MAX; }
        EERType GetRenderNodeType() override { return eERType_TerrainSystem; }

        void GetMemoryUsage(ICrySizer* pSizer) const override;

    private:
        void SetupRenderObject(CRenderObject* pObj, const SRenderingPassInfo& passInfo);

        CRETerrain* m_terrainRE;
        _smart_ptr<IMaterial> m_material;

        IConsole* m_console;
        IRenderer* m_renderer;
        I3DEngine* m_3dengine;
    };
}

#endif // CRYINCLUDE_CRY3DENGINE_TERRAIN_RENDER_NODE_H

#endif
