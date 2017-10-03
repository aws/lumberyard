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

#include <MCore/Source/Array.h>
#include <MCore/Source/Vector.h>


namespace EMotionFX
{
    // forward declarations
    class LODGenVertex;
    class LODGenMesh;


    class EMFX_API LODGenTriangle
    {
        MCORE_MEMORYOBJECTCATEGORY(LODGenTriangle, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_LODGENERATOR);

    public:
        LODGenTriangle();
        LODGenTriangle(LODGenMesh* mesh, LODGenVertex* vertexA, LODGenVertex* vertexB, LODGenVertex* vertexC);
        ~LODGenTriangle();

        void Delete();

        void Init(LODGenMesh* mesh, LODGenVertex* vertexA, LODGenVertex* vertexB, LODGenVertex* vertexC);
        bool GetHasVertex(const LODGenVertex* vertex) const;
        void ReplaceVertex(LODGenVertex* oldVertex, LODGenVertex* newVertex);
        void CalcNormal();

        MCORE_INLINE const MCore::Vector3& GetNormal() const        { return mNormal; }
        MCORE_INLINE LODGenVertex* GetVertex(uint32 index) const    { return mVertices[index]; }

    private:
        LODGenVertex*   mVertices[3];
        LODGenMesh*     mMesh;
        MCore::Vector3  mNormal;
    };
}   // namespace EMotionFX
