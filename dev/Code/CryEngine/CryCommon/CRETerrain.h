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

#ifndef CRYINCLUDE_CRETERRAIN_H
#define CRYINCLUDE_CRETERRAIN_H
#pragma once

#ifdef LY_TERRAIN_RUNTIME

#include "VertexFormats.h"

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <vector>

#include "RenderBus.h"

class CDLODQuadTree;
class LODSelection;
struct SelectedNode;

namespace Terrain
{
    class TerrainTextureCache;
    class TerrainRenderingParameters;

    // Utility class used specifically for rendering terrain patch nodes
    class TerrainPatchMesh
    {
    public:
        TerrainPatchMesh();
        TerrainPatchMesh(int meshResolution);

        void RenderFullPatch() const;
        void RenderNode(const SelectedNode& nodeSel, const Vec4& morphConstants) const;

        unsigned int GetMeshResolution() const { return m_uMeshResolution; }

    private:
        unsigned int m_uMeshResolution;
        unsigned int m_renderPatchStartIndices[4];
        unsigned int m_uRenderPatchIndexCount;

        _smart_ptr<IRenderMesh> m_renderMesh;
    };

    class CRETerrain
        : public CRendElementBase
        , public AZ::RenderThreadEventsBus::Handler
    {
    public:
        CRETerrain();
        virtual ~CRETerrain();

        void GetMemoryUsage(ICrySizer* pSizer) const override
        {
            pSizer->AddObject(this, sizeof(*this));
        }
        bool mfDraw(CShader* shader, SShaderPass* shaderPass);
        void mfPrepare(bool bCheckOverflow) override;

        Vec3 GetPositionAt(float x, float y) const;

        void Release(bool bForce = false) override;

        ////////////////////////////////////////////////////////////
        // RenderThreadEventsBus Impl
        void OnRenderThreadRenderSceneBegin() override;
        ////////////////////////////////////////////////////////////

    protected:

        // Updates quad tree according to terrain cvars
        void UpdateQuadTree();

        // Render terrain mesh for a given CDLODQuadtree LODSelection result
        void RenderNodes(const LODSelection* selection) const;

        // Update heightmap and texture material caches
        void UpdateTextureCaches();

        // Debug Rendering
        void RenderNodesWireframe(const LODSelection* selection) const;
        void RenderNodeAABBs(const LODSelection* selection) const;

        // CDLOD Quadtree
        AZStd::unique_ptr<CDLODQuadTree> m_quadTree;

        // Terrain texture compositing quad tree
        // This quadtree is used for identifying the terrain texture tiles that need to be generated
        AZStd::unique_ptr<CDLODQuadTree> m_terrainTextureQuadTree;

        // Terrain rendering quad
        TerrainPatchMesh m_terrainRenderMesh;

        // Terrain texture caching
        AZStd::shared_ptr<TerrainTextureCache> m_terrainTextureCache;

        // Debugging
        AZ::Plane m_savedFrustumPlanes[FRUSTUM_PLANES];
        AZ::Vector3 m_savedCameraPosition;

        // Guards against the case that terrain gets initialized between mfPrepare and mfDraw calls
        bool m_dataPrepared{ false };
    };
}

#endif


#endif // CRYINCLUDE_CRETERRAIN_H
