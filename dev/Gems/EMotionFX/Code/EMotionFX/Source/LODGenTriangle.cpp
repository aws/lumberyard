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
#include "LODGenTriangle.h"
#include "LODGenVertex.h"
#include "LODGenMesh.h"
#include "Mesh.h"


namespace EMotionFX
{
    // constructor
    LODGenTriangle::LODGenTriangle()
    {
        mMesh               = nullptr;
        mVertices[0]        = nullptr;
        mVertices[1]        = nullptr;
        mVertices[2]        = nullptr;
        //mVertexNumbers[0] = MCORE_INVALIDINDEX32;
        //mVertexNumbers[1] = MCORE_INVALIDINDEX32;
        //mVertexNumbers[2] = MCORE_INVALIDINDEX32;
        mNormal.Set(0.0f, 1.0f, 0.0f);
    }


    // extended constructor
    LODGenTriangle::LODGenTriangle(LODGenMesh* mesh, LODGenVertex* vertexA, LODGenVertex* vertexB, LODGenVertex* vertexC)
    {
        Init(mesh, vertexA, vertexB, vertexC);
    }


    // destructor
    LODGenTriangle::~LODGenTriangle()
    {
    }


    // act like we delete this triangle
    void LODGenTriangle::Delete()
    {
        // remove the triangle from the mesh
        //mMesh->RemoveTriangle( this );

        // unregister the triangle from the vertices
        for (uint32 v = 0; v < 3; ++v)
        {
            if (mVertices[v])
            {
                mVertices[v]->RemoveTriangle(this);
            }
        }

        // update vertex neighbor data
        for (uint32 v = 0; v < 3; ++v)
        {
            const uint32 v2 = (v + 1) % 3;
            if (mVertices[v] == nullptr || mVertices[v2] == nullptr)
            {
                continue;
            }
            mVertices[v ]->RemoveIfNonNeighbor(mVertices[v2]);
            mVertices[v2]->RemoveIfNonNeighbor(mVertices[v]);
        }
    }


    // calculate the normal
    void LODGenTriangle::CalcNormal()
    {
        const MCore::Vector3* positions = (MCore::Vector3*)mMesh->GetMesh()->FindVertexData(Mesh::ATTRIB_POSITIONS);
        const MCore::Vector3& posA = positions[ mVertices[0]->GetIndex() ];
        const MCore::Vector3& posB = positions[ mVertices[1]->GetIndex() ];
        const MCore::Vector3& posC = positions[ mVertices[2]->GetIndex() ];

        mNormal = (posB - posA).Cross(posC - posB).SafeNormalize();
    }


    // initialize
    void LODGenTriangle::Init(LODGenMesh* mesh, LODGenVertex* vertexA, LODGenVertex* vertexB, LODGenVertex* vertexC)
    {
        // make sure it are three different vertices
        MCORE_ASSERT(vertexA != vertexB && vertexB != vertexC && vertexC != vertexA);

        // init members
        mMesh        = mesh;
        mVertices[0] = vertexA;
        mVertices[1] = vertexB;
        mVertices[2] = vertexC;

        // inform the used vertices about this triangle
        for (uint32 v = 0; v < 3; ++v)
        {
            mVertices[v]->AddTriangle(this);

            // and register the neighbors
            for (uint32 j = 0; j < 3; ++j)
            {
                if (v != j)
                {
                    mVertices[v]->AddNeighborUnique(mVertices[j]);
                }
            }
        }

        // calculate the triangle normal
        CalcNormal();
    }


    // check if the given vertex is used by this triangle
    bool LODGenTriangle::GetHasVertex(const LODGenVertex* vertex) const
    {
        return (mVertices[0] == vertex || mVertices[1] == vertex || mVertices[2] == vertex);
    }


    // replace vertex u with v
    void LODGenTriangle::ReplaceVertex(LODGenVertex* oldVertex, LODGenVertex* newVertex)
    {
        MCORE_ASSERT(oldVertex && newVertex);
        MCORE_ASSERT(oldVertex == mVertices[0] || oldVertex == mVertices[1] || oldVertex == mVertices[2]);
        MCORE_ASSERT(newVertex != mVertices[0] && newVertex != mVertices[1] && newVertex != mVertices[2]);

        if (oldVertex == mVertices[0])
        {
            mVertices[0] = newVertex;
        }
        else
        if (oldVertex == mVertices[1])
        {
            mVertices[1] = newVertex;
        }
        else
        {
            MCORE_ASSERT(oldVertex == mVertices[2]);
            mVertices[2] = newVertex;
        }

        oldVertex->RemoveTriangle(this);
        MCORE_ASSERT(newVertex->GetHasTriangle(this) == false);

        newVertex->AddTriangle(this);

        for (uint32 i = 0; i < 3; ++i)
        {
            oldVertex->RemoveIfNonNeighbor(mVertices[i]);
            mVertices[i]->RemoveIfNonNeighbor(oldVertex);
        }

        for (uint32 i = 0; i < 3; ++i)
        {
            MCORE_ASSERT(mVertices[i]->GetHasTriangle(this));
            for (uint32 j = 0; j < 3; ++j)
            {
                if (i != j)
                {
                    mVertices[i]->AddNeighborUnique(mVertices[j]);
                }
            }
        }

        CalcNormal();
    }
}   // namespace EMotionFX
