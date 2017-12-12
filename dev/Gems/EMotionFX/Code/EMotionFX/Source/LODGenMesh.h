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
#include "LODGenerator.h"
#include "LODGenVertex.h"
#include "LODGenTriangle.h"

#include <MCore/Source/Array.h>
#include <MCore/Source/Matrix4.h>


namespace EMotionFX
{
    // forward declarations
    class Mesh;
    class Actor;
    class ActorInstance;


    class EMFX_API LODGenCell
    {
    public:
        LODGenCell()
        {
            mMinCost        = FLT_MAX;
            mNeedsUpdate    = true;
            mMinCostVertex  = nullptr;
            mMesh           = nullptr;
        }

        ~LODGenCell() {}

        MCORE_INLINE void ReserveVertices(uint32 numVerts)          { mVertices.Reserve(numVerts); }
        MCORE_INLINE void RemoveVertex(LODGenVertex* vertex)        { mVertices.SwapRemove(mVertices.Find(vertex)); }
        MCORE_INLINE void AddVertex(LODGenVertex* vertex)           { mVertices.Add(vertex); }
        MCORE_INLINE float GetMinCost() const                       { return mMinCost; }
        MCORE_INLINE uint32 GetNumVertices() const                  { return mVertices.GetLength(); }
        MCORE_INLINE void Invalidate()                              { mNeedsUpdate = true; }
        MCORE_INLINE LODGenVertex* GetMinCostVertex() const         { return mMinCostVertex; }
        MCORE_INLINE void SetMesh(LODGenMesh* mesh)                 { mMesh = mesh; }
        MCORE_INLINE LODGenVertex* GetVertex(uint32 index) const    { return mVertices[index]; }

        void Update();

    private:
        float                       mMinCost;
        MCore::Array<LODGenVertex*> mVertices;
        bool                        mNeedsUpdate;
        LODGenVertex*               mMinCostVertex;
        LODGenMesh*                 mMesh;
    };


    //
    class EMFX_API LODGenMesh
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(LODGenMesh, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_LODGENERATOR);

    public:
        static LODGenMesh* Create(LODGenerator* generator);
        static LODGenMesh* Create(LODGenerator* generator, Actor* actor, Mesh* mesh, uint32 nodeIndex, uint32 startVertex, float importanceFactor);

        void Release();
        void Init(const LODGenerator::InitSettings& initSettings);
        void RemoveVertex(LODGenVertex* vertex);
        void GenerateCells();
        void DebugRender(ActorInstance* instance, uint32 maxTotalVerts, const MCore::Vector3& visualOffset, const MCore::Vector3& scale, const MCore::Vector3& camPos, bool colorBorders);
        void PrepareMap();
        void BuildSameCostVertexArray(float costs);
        void UpdateCells();
        LODGenVertex* FindLowestCostVertexLinear() const;
        LODGenVertex* FindLowestCostVertex() const;
        Mesh* GenerateMesh(uint32 maxTotalVerts, const LODGenerator::GenerateSettings& settings);
        uint32 MapVertex(uint32 index, uint32 maxVerts);

        MCORE_INLINE void SetTotalNumVertices(uint32 numVerts)                              { mTotalNumVertices.Resize(numVerts + 1); }
        MCORE_INLINE uint32 GetNumVertices() const                                          { return mNumVertices; }
        MCORE_INLINE uint32 GetNumStartVertices() const                                     { return mNumStartVertices; }
        MCORE_INLINE uint32 GetNodeIndex() const                                            { return mNodeIndex; }
        MCORE_INLINE Actor* GetActor() const                                                { return mActor; }
        MCORE_INLINE Mesh* GetMesh() const                                                  { return mMesh; }
        MCORE_INLINE LODGenVertex* GetStartVertex(uint32 index)                             { return &mVertices[index]; }
        MCORE_INLINE const MCore::Matrix& GetGlobalMatrix() const                           { return mGlobalMatrix; }
        MCORE_INLINE void SetGlobalMatrix(const MCore::Matrix& mat)                         { mGlobalMatrix = mat; }
        MCORE_INLINE uint32 GetStartVertex() const                                          { return mStartVertex; }
        MCORE_INLINE MCore::Array<LODGenTriangle*>& GetSidesBuffer()                        { return mSides; }
        MCORE_INLINE LODGenVertex* GetLowestCostVertex() const                              { return mLowestCostVertex; }
        MCORE_INLINE void SetLowestCostVertex(LODGenVertex* vertex)                         { mLowestCostVertex = vertex; }
        MCORE_INLINE void SetMapValue(uint32 index, uint32 value)                           { mMap[index] = value; }
        MCORE_INLINE void SetPermutationValue(uint32 index, uint32 value)                   { mPermutations[index] = value; }
        MCORE_INLINE void SetVertexTotals(uint32 totalNumVertices, uint32 numVertices)      { mTotalNumVertices[totalNumVertices] = numVertices; }
        MCORE_INLINE void SetImportanceFactor(float factor)                                 { mImportanceFactor = factor; }
        MCORE_INLINE float GetImportanceFactor() const                                      { return mImportanceFactor; }
        MCORE_INLINE LODGenVertex* GetRandomSameCostVertex() const                          { MCORE_ASSERT(mSameCostVerts.GetLength() > 0); return mSameCostVerts[rand() % mSameCostVerts.GetLength()]; }

    private:
        LODGenVertex*                   mLowestCostVertex;
        MCore::Array<LODGenVertex>      mVertices;
        MCore::Array<LODGenTriangle>    mTriangles;
        MCore::Array<LODGenTriangle*>   mSides;
        MCore::Array<LODGenCell>        mCells;
        MCore::Array<LODGenVertex*>     mSameCostVerts;
        float                           mImportanceFactor;
        uint32                          mNumVertices;
        uint32                          mNumStartVertices;
        Mesh*                           mMesh;
        Actor*                          mActor;
        uint32                          mNodeIndex;
        uint32                          mStartVertex;
        MCore::Array<uint32>            mMap;
        MCore::Array<uint32>            mIndexLookup;
        MCore::Array<uint32>            mPermutations;
        MCore::Array<uint32>            mTotalNumVertices;
        LODGenerator*                   mGenerator;
        MCore::Matrix                   mGlobalMatrix;
        bool                            mCanGenerate;

        LODGenMesh(LODGenerator* generator);
        LODGenMesh(LODGenerator* generator, Actor* actor, Mesh* mesh, uint32 nodeIndex, uint32 startVertex, float importanceFactor);
        ~LODGenMesh();

        void UpdateVertexNeighbors();
        void LogInfo();
    };
}   // namespace EMotionFX
