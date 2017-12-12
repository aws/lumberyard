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

// include the required headers
#include "LODGenMesh.h"
#include "LODGenVertex.h"
#include "LODGenTriangle.h"
#include "Mesh.h"
#include "ActorInstance.h"
#include "TransformData.h"
#include "EventManager.h"
#include "SkinningInfoVertexAttributeLayer.h"
#include "MeshBuilder.h"
#include "MeshBuilderSkinningInfo.h"
#include "SubMesh.h"
#include "LODGenerator.h"
#include <MCore/Source/AABB.h>

namespace EMotionFX
{
    // update the cell, which means it tries to find its minimum cost vertex
    void LODGenCell::Update()
    {
        if (mNeedsUpdate == false || mVertices.GetLength() == 0)
        {
            return;
        }

        // find the minimum cost vertex
        LODGenVertex* curMinVertex = mVertices[0];
        const uint32 numVertices = mVertices.GetLength();
        for (uint32 i = 0; i < numVertices; ++i)
        {
            if (mVertices[i]->GetCollapseCost() < curMinVertex->GetCollapseCost())
            {
                curMinVertex = mVertices[i];
            }
        }

        // store it
        mMinCostVertex = curMinVertex;
        mNeedsUpdate = false;
    }

    //---------------------------------------------------------------------------------

    // default constructor
    LODGenMesh::LODGenMesh(LODGenerator* generator)
    {
        mGenerator          = generator;
        mActor              = nullptr;
        mMesh               = nullptr;
        mNodeIndex          = MCORE_INVALIDINDEX32;
        mStartVertex        = 0;
        mNumStartVertices   = 0;
        mNumVertices        = 0;
        mLowestCostVertex   = nullptr;
        mImportanceFactor   = 1.0f;
        mCanGenerate        = true;
    }


    // extended constructor
    LODGenMesh::LODGenMesh(LODGenerator* generator, Actor* actor, Mesh* mesh, uint32 nodeIndex, uint32 startVertex, float importanceFactor)
    {
        mGenerator          = generator;
        mMesh               = mesh;
        mActor              = actor;
        mNodeIndex          = nodeIndex;
        mStartVertex        = startVertex;
        mNumStartVertices   = 0;
        mNumVertices        = 0;
        mLowestCostVertex   = nullptr;
        mImportanceFactor   = importanceFactor;
    }


    // destructor
    LODGenMesh::~LODGenMesh()
    {
        Release();
    }


    // release memory
    void LODGenMesh::Release()
    {
        mTotalNumVertices.Clear();
        mMap.Clear();
        mPermutations.Clear();
        mIndexLookup.Clear();
        mVertices.Clear();

        mLowestCostVertex   = nullptr;
        mNumStartVertices   = 0;
        mNumVertices        = 0;
        mCanGenerate        = true;
    }


    // create
    LODGenMesh* LODGenMesh::Create(LODGenerator* generator)
    {
        return new LODGenMesh(generator);
    }


    // extended create
    LODGenMesh* LODGenMesh::Create(LODGenerator* generator, Actor* actor, Mesh* mesh, uint32 nodeIndex, uint32 startVertex, float importanceFactor)
    {
        return new LODGenMesh(generator, actor, mesh, nodeIndex, startVertex, importanceFactor);
    }


    // generate the connectivity info
    void LODGenMesh::Init(const LODGenerator::InitSettings& initSettings)
    {
        MCORE_ASSERT(mMesh);
        MCORE_ASSERT(mActor);
        MCORE_ASSERT(mNodeIndex != MCORE_INVALIDINDEX32);

        // release previous data
        Release();

        // only init for triangle meshes
        if (mMesh->CheckIfIsTriangleMesh() == false)
        {
            //MCore::LogWarning("EMotionFX::LODGenMesh::Init() - The mesh is not a triangle mesh. Currently only triangle meshes are supported.");
            mCanGenerate = false;
            return;
        }

        if (mMesh->GetIsCollisionMesh())
        {
            mCanGenerate = false;
            return;
        }

        // find the original vertex numbers in the mesh
        const uint32* orgVertices = (uint32*)mMesh->FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

        // get the vertex colors
        const uint32*               colors32    = (uint32*)mMesh->FindVertexData(Mesh::ATTRIB_COLORS32, initSettings.mColorLayerSetIndex);
        const MCore::RGBAColor*     colors128   = (MCore::RGBAColor*)mMesh->FindVertexData(Mesh::ATTRIB_COLORS128, initSettings.mColorLayerSetIndex);

        // generate the vertex and triangle connectivity data
        // start by preallocating space for the vertices
        const uint32 numVertices = mMesh->GetNumVertices();
        mVertices.Resize(numVertices);
        mNumVertices = numVertices;
        mNumStartVertices = numVertices;

        if (initSettings.mRandomizeSameCostVerts)
        {
            mSameCostVerts.Reserve(numVertices); // worst case, assume they all have the same cost
        }
        if ((initSettings.mUseVertexColors) && (colors32 || colors128))
        {
            for (uint32 v = 0; v < numVertices; ++v)
            {
                float vertexImportance = 1.0f;
                if (colors32 && colors128 == nullptr)
                {
                    vertexImportance = 1.0f + (MCore::ExtractGreen(colors32[v]) / 255.0f) * initSettings.mColorStrength;
                }

                if (colors128)
                {
                    vertexImportance = 1.0f + colors128[v].g * initSettings.mColorStrength;
                }

                mVertices[v].Init(this, v, mImportanceFactor * vertexImportance);
            }
        }
        else
        {
            for (uint32 v = 0; v < numVertices; ++v)
            {
                mVertices[v].Init(this, v, mImportanceFactor);
            }
        }

        // calculate the weight factor
        SkinningInfoVertexAttributeLayer* skinningLayer = (SkinningInfoVertexAttributeLayer*)mMesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
        if (skinningLayer)
        {
            for (uint32 v = 0; v < numVertices; ++v)
            {
                mVertices[v].SetSkinningCosts(static_cast<float>(skinningLayer->GetNumInfluences(orgVertices[v])));
            }
        }

        // add all triangles
        const uint32* indices = mMesh->GetIndices();
        const uint32 numIndices = mMesh->GetNumIndices();
        mTriangles.Resize(numIndices / 3);
        uint32 triangleIndex = 0;
        for (uint32 index = 0; index < numIndices; index += 3)
        {
            const uint32 indexA = indices[index];
            const uint32 indexB = indices[index + 1];
            const uint32 indexC = indices[index + 2];
            mTriangles[triangleIndex].Init(this, &mVertices[indexA], &mVertices[indexB], &mVertices[indexC]);
            triangleIndex++;
        }
        /*
            // iterate over all polygons, triangulate internally
            mTriangles.Resize( mMesh->CalcNumTriangles() );
            const uint32* indices       = mMesh->GetIndices();
            const uint8* vertCounts     = mMesh->GetPolygonVertexCounts();
            uint32 indexA, indexB, indexC;
            uint32 polyStartIndex = 0;
            uint32 triangleIndex = 0;
            const uint32 numPolygons = mMesh->GetNumPolygons();
            for (uint32 p=0; p<numPolygons; p++)
            {
                const uint32 numPolyVerts = vertCounts[p];

                // iterate over all triangles inside this polygon
                // explanation: numPolyVerts-2 == number of triangles
                // 3 verts=1 triangle, 4 verts=2 triangles, etc
                for (uint32 i=2; i<numPolyVerts; i++)
                {
                    indexA = indices[polyStartIndex];
                    indexB = indices[polyStartIndex + i];
                    indexC = indices[polyStartIndex + i - 1];

                    mTriangles[triangleIndex++].Init(this, &mVertices[indexA], &mVertices[indexB], &mVertices[indexC]);
                }

                polyStartIndex += numPolyVerts;
            }
        */
        // resize the permutation table
        mPermutations.Resize(mMesh->GetNumVertices());
        mMap.Resize(mMesh->GetNumVertices());
    }



    // remove a given vertex
    void LODGenMesh::RemoveVertex(LODGenVertex* vertex)
    {
        LODGenCell* cell = vertex->GetCell();
        cell->RemoveVertex(vertex);
        cell->Invalidate();
        cell->Update();
        mNumVertices--;
    }

    /*
    LODGenVertex* LODGenMesh::FindFirstVertex() const
    {
        const uint32 numCells = mCells.GetLength();
        for (uint32 i=0; i<numCells; ++i)
            if (mCells[i].GetNumVertices() > 0)
                return mCells[i].GetVertex(0);

        return nullptr;
    }*/


    // prepare the map
    void LODGenMesh::PrepareMap()
    {
        if (mCanGenerate == false)
        {
            return;
        }

        const uint32 numVertices = mMap.GetLength();
        for (uint32 i = 0; i < numVertices; ++i)
        {
            const uint32 oldValue = mMap[i];
            mMap[i] = (oldValue == MCORE_INVALIDINDEX32) ? 0 : mPermutations[ oldValue ];
        }

        // build the index lookup table
        mIndexLookup.Resize(numVertices);
        for (uint32 i = 0; i < numVertices; ++i)
        {
            const uint32 index = mPermutations[i];
            mIndexLookup[index] = i;
        }

        // update the indices already
        uint32* indices = mMesh->GetIndices();
        const uint32 numIndices = mMesh->GetNumIndices();
        for (uint32 i = 0; i < numIndices; ++i)
        {
            indices[i] = mPermutations[ indices[i] ];
        }
    }


    // map a given vertex
    uint32 LODGenMesh::MapVertex(uint32 index, uint32 maxVerts)
    {
        if (maxVerts == 0)
        {
            return 0;
        }

        uint32 result = index;
        while (result >= maxVerts)
        {
            result = mMap[ result ];
        }

        return result;
    }


    // debug render the edges
    void LODGenMesh::DebugRender(ActorInstance* instance, uint32 maxTotalVerts, const MCore::Vector3& visualOffset, const MCore::Vector3& scale, const MCore::Vector3& camPos, bool colorBorders)
    {
        MCORE_UNUSED(instance);

        // draw the regular full detail mesh if we cannot generate
        if (mCanGenerate == false)
        {
            MCore::Vector3*     positions   = (MCore::Vector3*)mMesh->FindVertexData(Mesh::ATTRIB_POSITIONS);
            uint32*             indices     = (uint32*)mMesh->GetIndices();
            uint8*              vtxCounts   = (uint8*)mMesh->GetPolygonVertexCounts();

            const MCore::Matrix& globalTM = mGenerator->GetBindPoseMatrix(mNodeIndex);
            const uint32 color = MCore::RGBA(255, 0, 0);

            uint32 indexA;
            uint32 indexB;
            uint32 curIndex = 0;
            MCore::Vector3 posA;
            MCore::Vector3 posB;
            const uint32 numPolygons = mMesh->GetNumPolygons();
            for (uint32 p = 0; p < numPolygons; ++p)
            {
                const uint32 numPolyVerts = vtxCounts[p];
                for (uint32 i = 0; i < numPolyVerts; ++i)
                {
                    indexA = indices[curIndex + i];
                    indexB = (i < numPolyVerts - 1) ? indices[curIndex + i + 1] : indices[curIndex];
                    posA = ((positions[indexA] * globalTM) * scale) + visualOffset;
                    posB = ((positions[indexB] * globalTM) * scale) + visualOffset;
                    GetEventManager().OnDrawLine(posA, posB, color);
                }

                curIndex += numPolyVerts;
            }

            return;
        }

        const uint32 maxVerts = mTotalNumVertices[maxTotalVerts];

        const uint32* indices = mMesh->GetIndices();
        const MCore::Vector3* positions = (MCore::Vector3*)mMesh->FindVertexData(Mesh::ATTRIB_POSITIONS);

        const MCore::Matrix& globalTM = mGenerator->GetBindPoseMatrix(mNodeIndex);
        const uint32 color = MCore::RGBA(210, 210, 210);

        MCore::Vector3 normal;
        const uint32 numIndices = mMesh->GetNumIndices();

        if (colorBorders)
        {
            const uint32 borderColor = MCore::RGBA(255, 255, 0);
            for (uint32 i = 0; i < numIndices; i += 3)
            {
                const uint32 indexA = mIndexLookup[ MapVertex(indices[i  ], maxVerts) ];
                const uint32 indexB = mIndexLookup[ MapVertex(indices[i + 1], maxVerts) ];
                const uint32 indexC = mIndexLookup[ MapVertex(indices[i + 2], maxVerts) ];

                if (indexA == indexB || indexB == indexC || indexC == indexA)
                {
                    continue;
                }

                const MCore::Vector3 posA = ((positions[indexA] * globalTM) * scale) + visualOffset;
                const MCore::Vector3 posB = ((positions[indexB] * globalTM) * scale) + visualOffset;
                const MCore::Vector3 posC = ((positions[indexC] * globalTM) * scale) + visualOffset;

                // calculate the direction from center of the triangle to the viewer
                MCore::Vector3 dir = camPos - ((posA + posB + posC) * 0.3333f); // center of the triangle
                dir.SafeNormalize();

                // backface cull
                normal = (posB - posA).Cross(posC - posB).SafeNormalize();
                if (normal.Dot(dir) < 0.0f)
                {
                    continue;
                }

                if (mVertices[indexA].GetIsBorderVertex() == false || mVertices[indexB].GetIsBorderVertex() == false)
                {
                    GetEventManager().OnDrawLine(posA, posB, color);
                }
                else
                {
                    GetEventManager().OnDrawLine(posA, posB, borderColor);
                }

                if (mVertices[indexB].GetIsBorderVertex() == false || mVertices[indexC].GetIsBorderVertex() == false)
                {
                    GetEventManager().OnDrawLine(posB, posC, color);
                }
                else
                {
                    GetEventManager().OnDrawLine(posB, posC, borderColor);
                }

                if (mVertices[indexA].GetIsBorderVertex() == false || mVertices[indexC].GetIsBorderVertex() == false)
                {
                    GetEventManager().OnDrawLine(posA, posC, color);
                }
                else
                {
                    GetEventManager().OnDrawLine(posA, posC, borderColor);
                }
            }
        }
        else
        {
            for (uint32 i = 0; i < numIndices; i += 3)
            {
                const uint32 indexA = mIndexLookup[ MapVertex(indices[i  ], maxVerts) ];
                const uint32 indexB = mIndexLookup[ MapVertex(indices[i + 1], maxVerts) ];
                const uint32 indexC = mIndexLookup[ MapVertex(indices[i + 2], maxVerts) ];

                if (indexA == indexB || indexB == indexC || indexC == indexA)
                {
                    continue;
                }

                const MCore::Vector3 posA = ((positions[indexA] * globalTM) * scale) + visualOffset;
                const MCore::Vector3 posB = ((positions[indexB] * globalTM) * scale) + visualOffset;
                const MCore::Vector3 posC = ((positions[indexC] * globalTM) * scale) + visualOffset;

                // calculate the direction from center of the triangle to the viewer
                MCore::Vector3 dir = camPos - ((posA + posB + posC) * 0.3333f); // center of the triangle
                dir.SafeNormalize();

                // backface cull
                normal = (posB - posA).Cross(posC - posB).SafeNormalize();
                if (normal.Dot(dir) < 0.0f)
                {
                    continue;
                }

                GetEventManager().OnDrawLine(posA, posB, color);
                GetEventManager().OnDrawLine(posB, posC, color);
                GetEventManager().OnDrawLine(posA, posC, color);
            }
        }
    }


    // log some infos
    void LODGenMesh::LogInfo()
    {
        const uint32 numVertices = mVertices.GetLength();
        for (uint32 v = 0; v < numVertices; ++v)
        {
            const uint32 numNeighbors = mVertices[v].GetNumNeighbors();
            MCore::LogInfo("Vertex #%d: numNeighbors=%d numTriangles=%d", v, numNeighbors, mVertices[v].GetNumTriangles());
        }
    }


    // generate the new mesh
    Mesh* LODGenMesh::GenerateMesh(uint32 maxTotalVerts, const LODGenerator::GenerateSettings& settings)
    {
        if (mCanGenerate == false)
        {
            Mesh* clonedMesh = mMesh->Clone();
            return clonedMesh;
        }

        // create the mesh builder
        const uint32 numOrgVerts = mMesh->GetNumOrgVertices();
        MeshBuilder* meshBuilder = MeshBuilder::Create(mNodeIndex, numOrgVerts, settings.mMaxBonesPerSubMesh, settings.mMaxVertsPerSubMesh, false);

        // build the skinning info if there is any
        SkinningInfoVertexAttributeLayer* skinLayer = (SkinningInfoVertexAttributeLayer*)mMesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
        if (skinLayer)
        {
            MeshBuilderSkinningInfo* skinningInfo = MeshBuilderSkinningInfo::Create(numOrgVerts);
            meshBuilder->SetSkinningInfo(skinningInfo);

            MCore::Array<MeshBuilderSkinningInfo::Influence> influences;

            // pass all skinning info
            for (uint32 v = 0; v < numOrgVerts; ++v)
            {
                uint32 numInfluences = skinLayer->GetNumInfluences(v);
                influences.Resize(numInfluences);
                for (uint32 i = 0; i < numInfluences; ++i)
                {
                    influences[i].mNodeNr = skinLayer->GetInfluence(v, i)->GetNodeNr();
                    ;
                    influences[i].mWeight = skinLayer->GetInfluence(v, i)->GetWeight();
                }

                // optimize the influences
                skinningInfo->SortInfluences(influences);
                skinningInfo->OptimizeSkinningInfluences(influences, settings.mWeightRemoveThreshold, settings.mMaxSkinInfluences);

                // add them to the skinning info
                numInfluences = influences.GetLength();
                for (uint32 i = 0; i < numInfluences; ++i)
                {
                    skinningInfo->AddInfluence(v, influences[i]);
                }
            }

            // optimize memory usage
            skinningInfo->OptimizeMemoryUsage();
        }

        // create all layers
        // original vertex numbers
        MeshBuilderVertexAttributeLayerUInt32* orgVtxLayer = MeshBuilderVertexAttributeLayerUInt32::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS);
        meshBuilder->AddLayer(orgVtxLayer);

        // positions
        MeshBuilderVertexAttributeLayerVector3* posLayer = MeshBuilderVertexAttributeLayerVector3::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_POSITIONS, false, true);
        meshBuilder->AddLayer(posLayer);

        // normals
        MeshBuilderVertexAttributeLayerVector3* normalLayer = MeshBuilderVertexAttributeLayerVector3::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_NORMALS, false, true);
        meshBuilder->AddLayer(normalLayer);

        // vertex lookup layer, used to find out which vertex before the LOD is now representing the current vertex
        MeshBuilderVertexAttributeLayerUInt32* vertexLookupLayer = MeshBuilderVertexAttributeLayerUInt32::Create(numOrgVerts, 0xFADE);
        meshBuilder->AddLayer(vertexLookupLayer);

        // uv layers
        MCore::Array<MeshBuilderVertexAttributeLayerVector2*>   layersUV;
        MCore::Array<AZ::Vector2*>  dataUV;
        uint32 uvLayerIndex = 0;
        AZ::Vector2* uvs = static_cast<AZ::Vector2*>(mMesh->FindOriginalVertexData(Mesh::ATTRIB_UVCOORDS, uvLayerIndex++));
        while (uvs)
        {
            dataUV.Add(uvs);
            MeshBuilderVertexAttributeLayerVector2* uvLayer = MeshBuilderVertexAttributeLayerVector2::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_UVCOORDS, false, false);
            VertexAttributeLayer* layer = mMesh->FindVertexAttributeLayer(Mesh::ATTRIB_UVCOORDS, uvLayerIndex - 1);
            if (layer)
            {
                uvLayer->SetName(layer->GetName());
            }

            layersUV.Add(uvLayer);
            meshBuilder->AddLayer(uvLayer);

            uvs = static_cast<AZ::Vector2*>(mMesh->FindVertexData(Mesh::ATTRIB_UVCOORDS, uvLayerIndex++));
        }

        // tangent layers
        MCore::Array<AZ::Vector4*>  dataTangent;
        MCore::Array<MeshBuilderVertexAttributeLayerVector4*>   layersTangent;
        uint32 tangentLayerIndex = 0;
        AZ::Vector4* tangents = static_cast<AZ::Vector4*>(mMesh->FindOriginalVertexData(Mesh::ATTRIB_TANGENTS, tangentLayerIndex++));
        while (tangents)
        {
            dataTangent.Add(tangents);
            MeshBuilderVertexAttributeLayerVector4* layer = MeshBuilderVertexAttributeLayerVector4::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_TANGENTS, false, true);
            VertexAttributeLayer* meshLayer = mMesh->FindVertexAttributeLayer(Mesh::ATTRIB_TANGENTS, tangentLayerIndex - 1);
            if (meshLayer)
            {
                layer->SetName(meshLayer->GetName());
            }

            layersTangent.Add(layer);
            meshBuilder->AddLayer(layer);
            tangents = static_cast<AZ::Vector4*>(mMesh->FindVertexData(Mesh::ATTRIB_TANGENTS, tangentLayerIndex++));
        }

        // 32 bit colors
        MCore::Array<uint32*>   dataColor32;
        MCore::Array<MeshBuilderVertexAttributeLayerUInt32*>    layersColor32;
        uint32 color32LayerIndex = 0;
        uint32* colors32 = (uint32*)mMesh->FindOriginalVertexData(Mesh::ATTRIB_COLORS32, color32LayerIndex++);
        while (colors32)
        {
            dataColor32.Add(colors32);
            MeshBuilderVertexAttributeLayerUInt32* layer = MeshBuilderVertexAttributeLayerUInt32::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_COLORS32);
            VertexAttributeLayer* meshLayer = mMesh->FindVertexAttributeLayer(Mesh::ATTRIB_COLORS32, color32LayerIndex - 1);
            if (meshLayer)
            {
                layer->SetName(meshLayer->GetName());
            }

            layersColor32.Add(layer);
            meshBuilder->AddLayer(layer);
            colors32 = (uint32*)mMesh->FindVertexData(Mesh::ATTRIB_COLORS32, color32LayerIndex++);
        }

        // 128 bit colors
        MCore::Array<AZ::Vector4*>  dataColor128;
        MCore::Array<MeshBuilderVertexAttributeLayerVector4*>   layersColor128;
        uint32 color128LayerIndex = 0;
        AZ::Vector4* colors128 = static_cast<AZ::Vector4*>(mMesh->FindOriginalVertexData(Mesh::ATTRIB_COLORS128, color128LayerIndex++));
        while (colors128)
        {
            dataColor128.Add(colors128);
            MeshBuilderVertexAttributeLayerVector4* layer = MeshBuilderVertexAttributeLayerVector4::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_COLORS128);
            VertexAttributeLayer* meshLayer = mMesh->FindVertexAttributeLayer(Mesh::ATTRIB_COLORS128, color128LayerIndex - 1);
            if (meshLayer)
            {
                layer->SetName(meshLayer->GetName());
            }

            layersColor128.Add(layer);
            meshBuilder->AddLayer(layer);
            colors128 = static_cast<AZ::Vector4*>(mMesh->FindVertexData(Mesh::ATTRIB_COLORS128, color128LayerIndex++));
        }

        // pass all triangles and vertices to the builder
        uint32* orgVertices = (uint32*)mMesh->FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);
        MCore::Vector3* positions = (MCore::Vector3*)mMesh->FindOriginalVertexData(Mesh::ATTRIB_POSITIONS);
        MCore::Vector3* normals   = (MCore::Vector3*)mMesh->FindOriginalVertexData(Mesh::ATTRIB_NORMALS);

        MCORE_ASSERT((int32)maxTotalVerts - 1 > 0);

        const uint32* indices = mMesh->GetIndices();
        const uint32 maxVerts = mTotalNumVertices[maxTotalVerts - 1];
        const uint32 numIndices = mMesh->GetNumIndices();
        uint32 triVerts[3];
        for (uint32 i = 0; i < numIndices; i += 3)
        {
            // get the triangle indices
            triVerts[0] = mIndexLookup[ MapVertex(indices[i  ], maxVerts) ];
            triVerts[1] = mIndexLookup[ MapVertex(indices[i + 1], maxVerts) ];
            triVerts[2] = mIndexLookup[ MapVertex(indices[i + 2], maxVerts) ];

            // if its a degenerate triangle, we can remove it
            if (triVerts[0] == triVerts[1] || triVerts[1] == triVerts[2] || triVerts[2] == triVerts[0])
            {
                continue;
            }

            // get the material for the triangle
            uint32 subMeshIndex = MCORE_INVALIDINDEX32;
            const uint32 numSubMeshes = mMesh->GetNumSubMeshes();
            for (uint32 s = 0; s < numSubMeshes && subMeshIndex == MCORE_INVALIDINDEX32; ++s)
            {
                const SubMesh* subMesh = mMesh->GetSubMesh(s);
                if (triVerts[0] >= subMesh->GetStartVertex() && triVerts[0] < subMesh->GetStartVertex() + subMesh->GetNumVertices())
                {
                    subMeshIndex = s;
                }
            }

            if (subMeshIndex == MCORE_INVALIDINDEX32)
            {
                subMeshIndex = 0;
            }

            // start the triangle
            meshBuilder->BeginPolygon(mMesh->GetSubMesh(subMeshIndex)->GetMaterial());

            // add all vertices of the triangle
            for (uint32 v = 0; v < 3; ++v)
            {
                const uint32 vertexIndex = triVerts[v];

                // set the orgvertex, position and normal
                orgVtxLayer->SetCurrentVertexValue(&orgVertices[vertexIndex]);
                posLayer->SetCurrentVertexValue(&positions[vertexIndex]);
                normalLayer->SetCurrentVertexValue(&normals[vertexIndex]);

                // set the uv coordinates
                for (uint32 j = 0; j < layersUV.GetLength(); ++j)
                {
                    layersUV[j]->SetCurrentVertexValue(&dataUV[j][vertexIndex]);
                }

                // set the tangents
                for (uint32 j = 0; j < layersTangent.GetLength(); ++j)
                {
                    layersTangent[j]->SetCurrentVertexValue(&dataTangent[j][vertexIndex]);
                }

                // set the 32 bit colors
                for (uint32 j = 0; j < layersColor32.GetLength(); ++j)
                {
                    layersColor32[j]->SetCurrentVertexValue(&dataColor32[j][vertexIndex]);
                }

                // set the 128 bit colors
                for (uint32 j = 0; j < layersColor128.GetLength(); ++j)
                {
                    layersColor128[j]->SetCurrentVertexValue(&dataColor128[j][vertexIndex]);
                }

                // set the temp vertex numbers, used for the morph targets later on, to know which vertex before LOD was applied is linked to this vertex
                uint32 fullDetailVertexIndex = vertexIndex;
                vertexLookupLayer->SetCurrentVertexValue(&fullDetailVertexIndex);

                // add the vertex
                meshBuilder->AddPolygonVertex(orgVertices[vertexIndex]);
            }

            // end the triangle
            meshBuilder->EndPolygon();
        }

        // optimize memory usage
        meshBuilder->OptimizeMemoryUsage();

        // optimize triangle lists
        if (settings.mOptimizeTriList)
        {
            meshBuilder->OptimizeTriangleList();
        }

        //meshBuilder.LogContents();

        Mesh* mesh = meshBuilder->ConvertToEMotionFXMesh();
        meshBuilder->Destroy();

        // convert into an EMotion FX mesh
        return mesh;
    }



    // generate the cells for this mesh
    void LODGenMesh::GenerateCells()
    {
        if (mVertices.GetLength() == 0)
        {
            return;
        }

        // first calculate the bounding box, in global space
        MCore::AABB box;
        mMesh->CalcAABB(&box, mGlobalMatrix);

        // calculate the number of grid cells we want
        uint32 gridCells = (uint32)(MCore::Math::Sqrt(MCore::Math::Sqrt((float)mVertices.GetLength())) / 2.5f);
        if (gridCells == 0)
        {
            gridCells = 1;
        }

        if (gridCells > 25)
        {
            gridCells = 25;
        }

        //MCore::LogInfo("LOD Generation Cells = %d", gridCells );

        // set the number of cells
        const uint32 numX = gridCells;
        const uint32 numY = gridCells;
        const uint32 numZ = gridCells;
        mCells.Resize(numX * numY * numZ);

        const float sizeX = box.CalcWidth() / (float)numX;
        const float sizeY = box.CalcHeight() / (float)numY;
        const float sizeZ = box.CalcDepth() / (float)numZ;

        //MCore::LogInfo("%s -    numX=%d, numY=%d, numZ=%d     sizeX=%f, sizeY=%f, sizeZ=%f", mActor->GetNode(mNodeIndex)->GetName(), numX, numY, numZ, sizeX, sizeY, sizeZ);

        uint32 cellX;
        uint32 cellY;
        uint32 cellZ;

        // reserve vertices in the cells
        for (uint32 i = 0; i < mCells.GetLength(); ++i)
        {
            mCells[i].ReserveVertices((uint32)(mVertices.GetLength() / ((numX * numY * numZ) * 0.5f)));
            mCells[i].SetMesh(this);
        }

        // fill the cells with vertices
        const MCore::Vector3* positions = (MCore::Vector3*)mMesh->FindVertexData(Mesh::ATTRIB_POSITIONS);
        const uint32 numVertices = mVertices.GetLength();
        for (uint32 i = 0; i < numVertices; ++i)
        {
            MCore::Vector3 pos = positions[i] * mGlobalMatrix;
            if (sizeX > MCore::Math::epsilon)
            {
                cellX = (uint32)MCore::Math::Floor((pos.x - box.GetMin().x) / (float)sizeX); // TODO: floor really needed?
            }
            else
            {
                cellX = 0;
            }

            if (sizeY > MCore::Math::epsilon)
            {
                cellY = (uint32)MCore::Math::Floor((pos.y - box.GetMin().y) / (float)sizeY);
            }
            else
            {
                cellY = 0;
            }

            if (sizeZ > MCore::Math::epsilon)
            {
                cellZ = (uint32)MCore::Math::Floor((pos.z - box.GetMin().z) / (float)sizeZ);
            }
            else
            {
                cellZ = 0;
            }

            if (cellX >= numX)
            {
                cellX = numX - 1;
            }
            if (cellY >= numY)
            {
                cellY = numY - 1;
            }
            if (cellZ >= numZ)
            {
                cellZ = numZ - 1;
            }

            LODGenCell* cell = &mCells[cellX + (cellY * numX) + (cellZ * numX * numY)];
            mVertices[i].SetCell(cell);
            cell->AddVertex(&mVertices[i]);
        }

        //MCore::LogInfo("CELL INFO FOR: %s", mActor->GetNode(mNodeIndex)->GetName());
        //for (uint32 i=0; i<mCells.GetLength(); ++i)
        //MCore::LogInfo("cell %d - numVerts = %d", i, mCells[i].GetNumVertices());

        // update the cells
        UpdateCells();
    }


    // update all cells (their minimum cost values)
    void LODGenMesh::UpdateCells()
    {
        const uint32 numCells = mCells.GetLength();
        for (uint32 i = 0; i < numCells; ++i)
        {
            mCells[i].Update();
        }
    }


    // find the lowest cost vetex
    LODGenVertex* LODGenMesh::FindLowestCostVertexLinear() const
    {
        LODGenVertex* result = nullptr;
        float lowest = FLT_MAX;

        const uint32 numCells = mCells.GetLength();
        for (uint32 i = 0; i < numCells; ++i)
        {
            const uint32 numVerts = mCells[i].GetNumVertices();
            for (uint32 v = 0; v < numVerts; ++v)
            {
                if (mCells[i].GetVertex(v)->GetCollapseCost() <= lowest)
                {
                    result = mCells[i].GetVertex(v);
                    lowest = mCells[i].GetVertex(v)->GetCollapseCost();
                }
            }
        }

        return result;
    }


    // find the vertex with lowest cost
    LODGenVertex* LODGenMesh::FindLowestCostVertex() const
    {
        LODGenVertex* result = nullptr;
        float lowest = FLT_MAX;

        const uint32 numCells = mCells.GetLength();
        for (uint32 i = 0; i < numCells; ++i)
        {
            const LODGenCell* cell = &mCells[i];
            if (cell->GetNumVertices() == 0)
            {
                continue;
            }

            LODGenVertex* cellMinVertex = cell->GetMinCostVertex();
            if (cellMinVertex->GetCollapseCost() <= lowest)
            {
                lowest = cellMinVertex->GetCollapseCost();
                result = cellMinVertex;
            }
        }

        return result;
    }


    // build the array of same cost vertices
    void LODGenMesh::BuildSameCostVertexArray(float costs)
    {
        mSameCostVerts.Clear(false);

        const uint32 numCells = mCells.GetLength();
        for (uint32 i = 0; i < numCells; ++i)
        {
            const LODGenCell* cell = &mCells[i];

            // skip cells that are empty or
            if ((cell->GetNumVertices() == 0) || (cell->GetMinCostVertex() && cell->GetMinCostVertex()->GetCollapseCost() > costs))
            {
                continue;
            }

            const uint32 numVerts = cell->GetNumVertices();
            for (uint32 v = 0; v < numVerts; ++v)
            {
                const float vertexCost = cell->GetVertex(v)->GetCollapseCost();
                if (MCore::Math::Abs(vertexCost - costs) < MCore::Math::epsilon)
                {
                    mSameCostVerts.Add(cell->GetVertex(v));
                }
            }
        }
    }
}   // namespace EMotionFX
