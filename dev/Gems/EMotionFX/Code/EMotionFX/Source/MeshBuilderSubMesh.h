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
#include "BaseObject.h"
#include "MeshBuilderVertexAttributeLayers.h"
#include "Importer/ActorFileFormat.h"
#include <MCore/Source/Array.h>


namespace EMotionFX
{
    // forward declarations
    class MeshBuilder;

    // a submesh
    class MeshBuilderSubMesh
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        static MeshBuilderSubMesh* Create(uint32 materialNr, MeshBuilder* mesh);

        MCORE_INLINE uint32 GetNumIndices() const                               { return mIndices.GetLength(); }
        MCORE_INLINE uint32 GetNumPolygons() const                              { return mPolyVertexCounts.GetLength(); }
        MCORE_INLINE uint32 GetNumBones() const                                 { return mBoneList.GetLength(); }
        MCORE_INLINE uint32 GetMaterialIndex() const                            { return mMaterialIndex; }
        MCORE_INLINE uint32 GetNumVertices() const                              { return mNumVertices; }
        MCORE_INLINE uint32 GetBone(uint32 index) const                         { return mBoneList[index]; }
        MCORE_INLINE uint8 GetPolygonVertexCount(uint32 polyIndex) const        { return mPolyVertexCounts[polyIndex]; }
        MCORE_INLINE MeshBuilderVertexLookup& GetVertex(uint32 index)
        {
            if (mVertexOrder.GetLength() != mNumVertices)
            {
                GenerateVertexOrder();
            }
            return mVertexOrder[index];
        }
        MCORE_INLINE MeshBuilder* GetMesh() const                               { return mMesh; }
        uint32 GetIndex(uint32 index);

        void GenerateVertexOrder();

        void SetBones(const MCore::Array<uint32>& boneList)                     { mBoneList = boneList; }
        MCore::Array<uint32> GetBones() const                                   { return mBoneList; }

        void Optimize();
        void AddPolygon(const MCore::Array<MeshBuilderVertexLookup>& indices, const MCore::Array<uint32>& boneList);
        bool CanHandlePolygon(const MCore::Array<uint32>& orgVertexNumbers, uint32 materialNr, MCore::Array<uint32>* outBoneList) const;

        uint32 CalcNumSimilarBones(const MCore::Array<uint32>& boneList) const;

    private:
        MCore::Array<MeshBuilderVertexLookup>   mIndices;
        MCore::Array<MeshBuilderVertexLookup>   mVertexOrder;
        MCore::Array<uint32>                    mBoneList;
        MCore::Array<uint8>                     mPolyVertexCounts;
        uint32                                  mMaterialIndex;
        uint32                                  mNumVertices;
        MeshBuilder*                            mMesh;

        MeshBuilderSubMesh(uint32 materialNr, MeshBuilder* mesh);
        ~MeshBuilderSubMesh();

        bool CheckIfHasVertex(const MeshBuilderVertexLookup& vertex);
    };
}   // namespace EMotionFX
