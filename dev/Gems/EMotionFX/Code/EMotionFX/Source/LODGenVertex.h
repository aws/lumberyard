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

#pragma once

// include required headers
#include "EMotionFXConfig.h"
#include "MemoryCategories.h"
#include <MCore/Source/Array.h>
#include "LODGenMesh.h"


namespace EMotionFX
{
    // forward declarations
    class LODGenTriangle;
    class LODGenCell;


    class EMFX_API LODGenVertex
    {
        MCORE_MEMORYOBJECTCATEGORY(LODGenVertex, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_LODGENERATOR);

    public:
        LODGenVertex();
        LODGenVertex(LODGenMesh* mesh, uint32 vertexIndex, float importanceFactor);
        ~LODGenVertex();

        void Init(LODGenMesh* mesh, uint32 vertexIndex, float importanceFactor);

        MCORE_INLINE void AddTriangle(LODGenTriangle* triangle)     { mTriangles.Add(triangle); }
        MCORE_INLINE void AddNeighborUnique(LODGenVertex* vertex)
        {
            if (mNeighbors.Contains(vertex) == false)
            {
                mNeighbors.Add(vertex);
            }
        }
        MCORE_INLINE void RemoveTriangle(LODGenTriangle* triangle)  { mTriangles.RemoveByValue(triangle); }
        MCORE_INLINE uint32 GetNumNeighbors() const                 { return mNeighbors.GetLength(); }

        MCORE_INLINE void SetCell(LODGenCell* cell)                 { mCell = cell; }
        MCORE_INLINE LODGenCell* GetCell() const                    { return mCell; }

        MCORE_INLINE uint32 GetNumTriangles() const                             { return mTriangles.GetLength(); }
        MCORE_INLINE LODGenTriangle* GetTriangle(uint32 index) const            { return mTriangles[index]; }
        MCORE_INLINE const MCore::Array<LODGenVertex*>& GetNeighbors() const    { return mNeighbors; }

        MCORE_INLINE void SetCollapseCost(float costs)              { mCosts = costs; }
        MCORE_INLINE float GetCollapseCost() const                  { return mCosts; }
        MCORE_INLINE LODGenVertex* GetCollapseVertex() const        { return mCollapse; }

        MCORE_INLINE LODGenMesh* GetMesh() const                    { return mMesh; }

        MCORE_INLINE uint32 GetIndex() const                        { return mVertexIndex; }
        MCORE_INLINE void SetSkinningCosts(float costs)             { mSkinningCosts = costs; }

        MCORE_INLINE bool GetHasTriangle(LODGenTriangle* triangle) const    { return mTriangles.Contains(triangle); }
        MCORE_INLINE bool GetIsBorderVertex() const                 { return mIsBorderVertex; }

        MCORE_INLINE void SetImportanceFactor(float factor)         { mImportanceFactor = factor; }
        MCORE_INLINE float GetImportanceFactor() const              { return mImportanceFactor; }

        void RemoveIfNonNeighbor(LODGenVertex* v);
        void CalcEdgeCollapseCosts(const LODGenerator::InitSettings& settings);

        void Delete();

    private:
        MCore::Array<LODGenVertex*>     mNeighbors;     // the neighboring vertices, so basically a list of edges
        MCore::Array<LODGenTriangle*>   mTriangles;     // triangles it belongs to
        float                           mCosts;
        float                           mImportanceFactor;
        LODGenVertex*                   mCollapse;
        bool                            mIsBorderVertex;
        float                           mSkinningCosts;
        uint32                          mVertexIndex;   // the vertex number
        LODGenMesh*                     mMesh;
        LODGenCell*                     mCell;

        float CalcEdgeCollapseCosts(LODGenVertex* n, const LODGenerator::InitSettings& settings);
    };
}   // namespace EMotionFX
