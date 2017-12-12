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

// include required headers
#include "LODGenVertex.h"
#include "LODGenMesh.h"
#include "LODGenTriangle.h"
#include "Mesh.h"


namespace EMotionFX
{
    // default constructor
    LODGenVertex::LODGenVertex()
    {
    }


    // constructor
    LODGenVertex::LODGenVertex(LODGenMesh* mesh, uint32 vertexIndex, float importanceFactor)
    {
        mVertexIndex        = vertexIndex;
        mMesh               = mesh;
        mCollapse           = nullptr;
        mCosts              = 0.0f;
        mIsBorderVertex     = false;
        mSkinningCosts      = 1.0f;
        mCell               = nullptr;
        mImportanceFactor   = importanceFactor;
    }


    // destructor
    LODGenVertex::~LODGenVertex()
    {
    }


    // init the vertex
    void LODGenVertex::Init(LODGenMesh* mesh, uint32 vertexIndex, float importanceFactor)
    {
        mVertexIndex        = vertexIndex;
        mMesh               = mesh;
        mCollapse           = nullptr;
        mCosts              = 0.0f;
        mIsBorderVertex     = false;
        mSkinningCosts      = 1.0f;
        mImportanceFactor   = importanceFactor;

        mTriangles.Reserve(4);
        mNeighbors.Reserve(6);
    }


    // delete the vertex
    void LODGenVertex::Delete()
    {
        MCORE_ASSERT(mTriangles.GetLength() == 0);
        while (mNeighbors.GetLength() > 0)
        {
            const uint32 index = mNeighbors[0]->mNeighbors.Find(this);
            if (index != MCORE_INVALIDINDEX32)
            {
                mNeighbors[0]->mNeighbors.SwapRemove(index);
            }
            //mNeighbors[0]->mNeighbors.RemoveByValue( this );
            //mNeighbors.RemoveByValue( mNeighbors[0] );
            mNeighbors.SwapRemove(0);
        }

        mMesh->RemoveVertex(this);
    }


    // remove a vertex if it is not a neighbor
    void LODGenVertex::RemoveIfNonNeighbor(LODGenVertex* v)
    {
        // it's not a neighbor, do nothing
        const uint32 index = mNeighbors.Find(v);
        if (index == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // if one of the triangles has this vertex, do nothing
        const uint32 numTriangles = mTriangles.GetLength();
        for (uint32 t = 0; t < numTriangles; t++)
        {
            if (mTriangles[t]->GetHasVertex(v))
            {
                return;
            }
        }

        // remove it from the neighbor list
        mNeighbors.SwapRemove(index);//RemoveByValue( v );
    }


    // precalculate the edge collapse cost
    void LODGenVertex::CalcEdgeCollapseCosts(const LODGenerator::InitSettings& settings)
    {
        // this vertex has no neighbors, in that case it costs nothing to collapse it
        const uint32 numNeighbors = mNeighbors.GetLength();
        if (numNeighbors == 0)
        {
            mCollapse   = nullptr;
            mCosts      = 0.0f;
            return;
        }

        // init
        mCosts      = FLT_MAX - 1.0f;
        mCollapse   = nullptr;

        // try to find the edge to collapse, which has the lowest cost, so the least visual error
        for (uint32 n = 0; n < numNeighbors; ++n)
        {
            const float cost = CalcEdgeCollapseCosts(mNeighbors[n], settings);
            if (cost < mCosts)
            {
                mCollapse   = mNeighbors[n];
                mCosts      = cost;
            }
        }

        if (mIsBorderVertex && settings.mPreserveBorderEdges)
        {
            mCosts = FLT_MAX - 1.0f - (1.0f / (mCosts + 1.0f));
        }
    }


    // calculate the costs when collapsing this vertex onto another given neighbor
    float LODGenVertex::CalcEdgeCollapseCosts(LODGenVertex* n, const LODGenerator::InitSettings& settings)
    {
        const MCore::Vector3* positions = (MCore::Vector3*)mMesh->GetMesh()->FindVertexData(Mesh::ATTRIB_POSITIONS);
        const MCore::Vector3& posU = positions[ mVertexIndex ] * mMesh->GetGlobalMatrix();
        const MCore::Vector3& posV = positions[ n->mVertexIndex ] * mMesh->GetGlobalMatrix();
        const float edgeLength = (posU - posV).SafeLength();

        // find the triangles used by the edge
        MCore::Array<LODGenTriangle*>& sides = mMesh->GetSidesBuffer();
        sides.Clear(false);
        const uint32 numFaces = mTriangles.GetLength();
        for (uint32 i = 0; i < numFaces; ++i)
        {
            if (mTriangles[i]->GetHasVertex(n))
            {
                sides.Add(mTriangles[i]);
            }
        }

        // use the triangle facing most away from the sides to determine our curvature term
        float curvature = 0.0f;
        for (uint32 i = 0; i < numFaces; ++i)
        {
            float minCurv = 1.0f;

            // process all edges
            const uint32 numSides = sides.GetLength();
            for (uint32 j = 0; j < numSides; ++j)
            {
                // calculte the dot product between the two face normals
                const float dotProduct = mTriangles[i]->GetNormal().Dot(sides[j]->GetNormal());

                // calculate the minimum curvature value
                minCurv = MCore::Min<float>(minCurv, (1.0f - dotProduct) * 0.5f);
            }

            // find the maximum curvature
            curvature = MCore::Max<float>(curvature, minCurv);

            // prevent the cost from being completely zero on planar surfaces
            // as then the edge length is basically ignored
            if (curvature < MCore::Math::epsilon)
            {
                curvature = 0.00001f;
            }
        }

        // prevent collapsing border edges
        if (sides.GetLength() == 1)
        {
            //n->mIsBorderVertex    = true;
            mIsBorderVertex     = true;
        }


        float cost =    (edgeLength * settings.mEdgeLengthFactor) *
            (curvature * settings.mCurvatureFactor) *
            (mSkinningCosts * settings.mSkinningFactor)
            * mImportanceFactor;

        return cost;
    }

    /*
    // find a neighbor with a given original vertex number
    LODGenVertex* LODGenVertex::FindNeighbor(uint32 orgVertexNr) const
    {
        const uint32 numNeighbors = mNeighbors.GetLength();
        for (uint32 n=0; n<numNeighbors; ++n)
            if (mNeighbors[n]->GetOrgVertexNumber() == orgVertexNr)
                return mNeighbors[n];

        return nullptr;
    }


    // check if this is a manifold edge (an edge along a border)
    bool LODGenVertex::IsManifoldEdgeWith(const LODGenVertex* other) const
    {
        uint32 count = 0;

        const uint32 numFaces = mTriangles.GetLength();
        for (uint32 i=0; i<numFaces; ++i)
            if (mTriangles[i]->HasVertex(other))
                count++;

        return (count == 1);
    }
    */
}   // namespace EMotionFX
