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
#include "MeshBuilderVertexAttributeLayers.h"
#include "BaseObject.h"

#include <MCore/Source/Array.h>


namespace EMotionFX
{
    // forward declarations
    class MeshBuilderSubMesh;
    class MeshBuilderSkinningInfo;
    class Mesh;


    /*
     * Small usage tutorial:
     *
     * For all your vertex data types (position, normal, uvs etc)
     *    AddLayer( layer );
     *
     * For all polygons inside a mesh you want to export
     *    BeginPolygon( polyMaterialNr )
     *       For all added layers you added to the mesh
     *          layer->SetCurrentVertexValue( )
     *       AddPolygonVertex( originalVertexNr )
     *    EndPolygon()
     *
     * OptimizeMemoryUsage()
     * OptimizeTriangleList()
     *
     */
    class EMFX_API MeshBuilder
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class MeshBuilderSubMesh;

    public:
        static MeshBuilder* Create(uint32 nodeNumber, uint32 numOrgVerts, uint32 maxBonesPerSubMesh, uint32 maxSubMeshVertices, bool isCollisionMesh);
        static MeshBuilder* Create(uint32 nodeNumber, uint32 numOrgVerts, bool isCollisionMesh);

        // add a layer, all added layers will be deleted automatically by the destructor of this class
        void AddLayer(MeshBuilderVertexAttributeLayer* layer);
        MeshBuilderVertexAttributeLayer* FindLayer(uint32 layerID, uint32 occurrence = 0) const;
        uint32 GetNumLayers(uint32 layerID) const;

        void BeginPolygon(uint32 materialNr);       // begin a poly
        void AddPolygonVertex(uint32 orgVertexNr);  // add a vertex to it (do this n-times, for an n-gon)
        void EndPolygon();                          // end the polygon, after adding all polygon vertices

        uint32 CalcNumIndices() const;  // calculate the number of indices in the mesh
        uint32 CalcNumVertices() const; // calculate the number of vertices in the mesh

        void OptimizeMemoryUsage();     // call this after your mesh is filled with data and won't change anymore
        void OptimizeTriangleList();    // call this in order to optimize the index buffers on cache efficiency
        void ConvertEndian(MCore::Endian::EEndianType sourceEndian, MCore::Endian::EEndianType destEndian); // convert endian

        void LogContents();
        bool CheckIfIsTriangleMesh() const;
        bool CheckIfIsQuadMesh() const;
        Mesh* ConvertToEMotionFXMesh();

        // inline functions
        MCORE_INLINE uint32 GetNumOrgVerts() const                                  { return mNumOrgVerts; }
        MCORE_INLINE void SetSkinningInfo(MeshBuilderSkinningInfo* skinningInfo)    { mSkinningInfo = skinningInfo; }
        MCORE_INLINE MeshBuilderSkinningInfo* GetSkinningInfo() const               { return mSkinningInfo; }
        MCORE_INLINE uint32 GetMaxBonesPerSubMesh() const                           { return mMaxBonesPerSubMesh; }
        MCORE_INLINE uint32 GetMaxVerticesPerSubMesh() const                        { return mMaxSubMeshVertices; }
        MCORE_INLINE void SetMaxBonesPerSubMesh(uint32 maxBones)                    { mMaxBonesPerSubMesh = maxBones; }
        MCORE_INLINE uint32 GetNodeNumber() const                                   { return mNodeNumber; }
        MCORE_INLINE void SetNodeNumber(uint32 nodeNr)                              { mNodeNumber = nodeNr; }
        MCORE_INLINE bool GetIsCollisionMesh() const                                { return mIsCollisionMesh; }
        MCORE_INLINE uint32 GetNumLayers() const                                    { return mLayers.GetLength(); }
        MCORE_INLINE uint32 GetNumSubMeshes() const                                 { return mSubMeshes.GetLength(); }
        MCORE_INLINE MeshBuilderVertexAttributeLayer* GetLayer(uint32 index) const  { return mLayers[index]; }
        MCORE_INLINE MeshBuilderSubMesh* GetSubMesh(uint32 index) const             { return mSubMeshes[index]; }
        MCORE_INLINE uint32 GetNumPolygons() const                                  { return mPolyVertexCounts.GetLength(); }

        struct EMFX_API SubMeshVertex
        {
            uint32              mRealVertexNr;
            uint32              mDupeNr;
            MeshBuilderSubMesh* mSubMesh;
        };

        uint32 FindRealVertexNr(MeshBuilderSubMesh* subMesh, uint32 orgVtx, uint32 dupeNr);
        SubMeshVertex* FindSubMeshVertex(MeshBuilderSubMesh* subMesh, uint32 orgVtx, uint32 dupeNr);
        uint32 CalcNumVertexDuplicates(MeshBuilderSubMesh* subMesh, uint32 orgVtx) const;

        void GenerateSubMeshVertexOrders();

        void AddSubMeshVertex(uint32 orgVtx, const SubMeshVertex& vtx);
        uint32 GetNumSubMeshVertices(uint32 orgVtx);
        MCORE_INLINE SubMeshVertex* GetSubMeshVertex(uint32 orgVtx, uint32 index)       { return &mVertices[orgVtx][index]; }

    private:
        static const int s_defaultMaxBonesPerSubMesh;
        static const int s_defaultMaxSubMeshVertices;

        MCore::Array< MCore::Array<SubMeshVertex> > mVertices;

        MCore::Array<uint32>                mPolyBoneList;
        MCore::Array<MeshBuilderSubMesh*>   mSubMeshes;
        MCore::Array<MeshBuilderVertexAttributeLayer*>  mLayers;
        MeshBuilderSkinningInfo*            mSkinningInfo;
        MeshBuilderSubMesh*                 mLastSubMeshHit;
        uint32                              mNodeNumber;

        MCore::Array<MeshBuilderVertexLookup>   mPolyIndices;
        MCore::Array<uint32>                    mPolyOrgVertexNumbers;
        MCore::Array<uint32>                    mPolyVertexCounts;

        uint32                              mMaterialNr;
        uint32                              mMaxBonesPerSubMesh;
        uint32                              mMaxSubMeshVertices;
        uint32                              mNumOrgVerts;
        bool                                mIsCollisionMesh;

        MeshBuilderVertexLookup FindMatchingDuplicate(uint32 orgVertexNr);
        MeshBuilderVertexLookup AddVertex(uint32 orgVertexNr);
        MeshBuilderVertexLookup FindVertexIndex(uint32 orgVertexNr);
        MeshBuilderSubMesh* FindSubMeshForPolygon(const MCore::Array<uint32>& orgVertexNumbers, uint32 materialNr);
        void ExtractBonesForPolygon(const MCore::Array<uint32>& orgVertexNumbers, MCore::Array<uint32>* outPolyBoneList) const;
        void AddPolygon(const MCore::Array<MeshBuilderVertexLookup>& indices, const MCore::Array<uint32>& orgVertexNumbers, uint32 materialNr);

        MeshBuilder(uint32 nodeNumber, uint32 numOrgVerts, uint32 maxBonesPerSubMesh, uint32 maxSubMeshVertices, bool isCollisionMesh);
        ~MeshBuilder();
    };
} // namespace MeshExporter
