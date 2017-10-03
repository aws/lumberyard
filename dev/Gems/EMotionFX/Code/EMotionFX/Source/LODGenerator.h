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

#include <MCore/Source/Array.h>
#include <MCore/Source/Matrix4.h>


namespace EMotionFX
{
    // forward declarations
    class LODGenMesh;
    class Actor;
    class LODGenVertex;
    class ActorInstance;
    class MorphMeshDeformer;
    class Node;
    class Mesh;

    /**
     *
     *
     *
     */
    class EMFX_API LODGenerator
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(LODGenerator, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_LODGENERATOR);
        friend class LODGenMesh;

    public:
        struct EMFX_API NodeSettings
        {
            uint32      mNodeIndex;             /**< The node for which these settings are. */
            float       mImportanceFactor;      /**< How important is it to preserve detail in this node? Higher values will preserve detail longer. Lower values cause this node to lose detail faster. The default value is 1.*/
            bool        mForceRemoveMesh;       /**< Set to true if you like the mesh of this node to be removed in any case. */

            NodeSettings()
            {
                mNodeIndex          = MCORE_INVALIDINDEX32;
                mImportanceFactor   = 1.0f;
                mForceRemoveMesh    = false;
            };

            NodeSettings(uint32 nodeIndex, float importanceFactor, bool forceRemoveMesh)
                : mNodeIndex(nodeIndex)
                , mImportanceFactor(importanceFactor)
                , mForceRemoveMesh(forceRemoveMesh) {}
        };

        struct EMFX_API InitSettings
        {
            float   mCurvatureFactor;           /**< How important is the curvature of the surface? Higher values preserve curvatures longer. The default value is 1. */
            float   mEdgeLengthFactor;          /**< How important is the length of an edge? Higher values preserve long edges longer. The default value is 1. */
            float   mSkinningFactor;            /**< How important are the skin deformations? Higher values preserve vertices with higher number of vertex influences longer. The default value is 1. */
            float   mAABBThreshold;             /**< Mesh bounding boxes with the extent length smaller than this value will be automatically removed. This is used to remove small meshes automatically. The default value is 0. */
            float   mColorStrength;             /**< The vertex color multiplier, which is used in case we use vertex colors to vertex paint areas that should be preserved as long as possible. */
            uint32  mInputLOD;                  /**< The input LOD level to generate the LOD from. The default value is 0, which is the highest detail model. */
            uint32  mColorLayerSetIndex;        /**< The color layer to use when we want to use vertex color support to preserve given areas in the mesh longer. The default value is 0, which is the first color layer stored. */
            bool    mUseVertexColors;           /**< Do we want to use vertex colors to determine how important it is to preserve the given vertex? Higher green values prevent vertices from being collapsed longer. The default value is true. */
            bool    mPreserveBorderEdges;       /**< Prevent border edges from collapsing as long as possible? The default is true. */
            bool    mRandomizeSameCostVerts;    /**< If there are multiple vertices that have the same collapse cost, should we pick a random one? This can result in longer generation times. The default is false. */
            MCore::Array<NodeSettings>  mNodeSettings;  /**< The per node settings. */

            InitSettings()
            {
                mCurvatureFactor        = 1.0f;
                mEdgeLengthFactor       = 1.0f;
                mSkinningFactor         = 1.0f;
                mInputLOD               = 0;
                mColorLayerSetIndex     = 0;
                mAABBThreshold          = 0.0f;
                mColorStrength          = 1.0f;
                mUseVertexColors        = true;
                mPreserveBorderEdges    = true;
                mRandomizeSameCostVerts = false;
            }
        };

        struct EMFX_API GenerateSettings
        {
            float   mDetailPercentage;              /**< The percentage of detail of the input model. This must be a value between 0..100, where 100 means full detail, 75 means 75 percent detail of the input model, etc. */
            uint32  mMaxBonesPerSubMesh;            /**< The maximum number of bones per submesh. The default value is 55. This basically represents how many bones fit in the constant registers of your vertex skinning shader. */
            uint32  mMaxVertsPerSubMesh;            /**< The maximum vertices per submesh, which is 65535 on default. */
            uint32  mMaxSkinInfluences;             /**< The maximum number of skinning influences per vertex. The default is 3. */
            float   mWeightRemoveThreshold;         /**< Remove vertex skinning influences that have a weight value smaller than this threshold. The default is 0.0001.*/
            bool    mGenerateNormals;               /**< Recalculate the normals after generation of the new mesh? The default is false. */
            bool    mSmoothNormals;                 /**< Smooth the normals? This only has influence when recalculating the normals is enabled. */
            bool    mGenerateTangents;              /**< Recalculate the tangents after generation of the new mesh? The default is false. */
            bool    mOptimizeTriList;               /**< Optimize triangle lists for vertex cache efficiency? This can slow down generation a lot on high detail models. The default is false. */
            bool    mDualQuatSkinning;              /**< Use dual quaternion skinning? This is high quality volume preserving, but much slower vertex skinning. The default is false. */
            MCore::Array<uint32>    mMorphTargetIDsToRemove;    /**< The ID values of the morph targets that shall be excluded in this LOD. */

            GenerateSettings()
            {
                mDetailPercentage       = 80.0f;
                mMaxBonesPerSubMesh     = 55;
                mMaxVertsPerSubMesh     = 65535;
                mMaxSkinInfluences      = 3;
                mWeightRemoveThreshold  = 0.0001f;
                mGenerateNormals        = false;
                mSmoothNormals          = false;
                mGenerateTangents       = false;
                mDualQuatSkinning       = false;
                mOptimizeTriList        = false;
            }
        };

        static LODGenerator* Create();

        void Release();
        void Init(Actor* sourceActor, const InitSettings& initSettings);

        void GenerateMeshes(const GenerateSettings& settings);
        void AddLastGeneratedLODToActor(Actor* actor);
        void ReplaceLODLevelWithLastGeneratedActor(Actor* actor, uint32 lodLevel);
        void DebugRender(ActorInstance* instance, uint32 maxTotalVerts, const MCore::Vector3& visualOffset, const MCore::Vector3& camPos, bool colorBorders);

        MCORE_INLINE uint32 GetTotalNumVerts() const            { return mTotalNumVerts; }
        MCORE_INLINE Actor* GetPreviewActor() const             { return mActor; }

    private:
        MCore::Array<LODGenMesh*>   mMeshes;
        MCore::Array<LODGenVertex*> mTemp;
        MCore::Array<MCore::Matrix> mBindPoseMatrices;
        MCore::Array<NodeSettings>  mNodeSettings;
        Actor*                      mClone;
        Actor*                      mActor;
        uint32                      mTotalNumVerts;
        uint32                      mNumVerts;

        LODGenerator();
        ~LODGenerator();

        void Reinit(const InitSettings& initSettings);
        void CalcEdgeCollapseCosts(const InitSettings& settings);
        void Collapse(LODGenVertex* u, LODGenVertex* v, const InitSettings& settings);
        void UpdateMorphSetup(const GenerateSettings& settings);
        void CopyMorphSetup(Actor* sourceActor, Actor* targetActor, uint32 sourceLOD, uint32 targetLOD) const;
        LODGenVertex* FindMinimumCostVertex() const;
        const NodeSettings* FindNodeSettings(const InitSettings& settings, uint32 nodeIndex) const;
        uint32 CalcTotalNumVertices() const;

        MCORE_INLINE const MCore::Matrix& GetBindPoseMatrix(uint32 nodeIndex) const     { return mBindPoseMatrices[nodeIndex]; }
    };
}   // namespace EMotionFX
