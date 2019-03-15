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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/parallel/atomic.h>

#include <AzFramework/Asset/SimpleAsset.h>

#include <Integration/Assets/AssetCommon.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>

#include <IEntityRenderState.h>
#include <IIndexedMesh.h>

struct IStatObj; // Cry mesh wrapper.
struct SSkinningData;

namespace EMotionFX
{
    class Actor;
    class ActorInstance;

    namespace Integration
    {
        enum class SkinningMethod: AZ::u32
        {
            DualQuat = 0,
            Linear
        };

        /**
         * Represents an EMotionFX actor asset.
         * Each asset maintains storage of the original EMotionFX binary asset (via EMotionFXAsset base class).
         * Initialization of the asset constructs Lumberyard rendering objects, such as the render mesh and material,
         * directly from the instantiated EMotionFX actor.
         * An easy future memory optimization is to wipe the EMotionFXAsset buffer after the actor, render meshes,
         * and materials are created, since it's technically no longer necessary. At this stage it's worth keeping
         * around for testing.
         */
        class ActorAsset
            : public EMotionFXAsset
        {
        public:
            typedef AZStd::vector<AZ::u32> BoneIndexToNodeMap;
            using MaterialList = AZStd::vector<AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> >;

            struct Primitive
            {
                AZStd::vector<SMeshBoneMapping_uint16>  m_vertexBoneMappings;
                _smart_ptr<IRenderMesh> m_renderMesh;
                CMesh*                  m_mesh = nullptr;      // Non-null only until asset is finalized.
                bool                    m_isDynamic = false; // Indicates if the mesh is dynamic (e.g. has morph targets)
                EMotionFX::SubMesh*     m_subMesh = nullptr;

                Primitive() = default;
                ~Primitive()
                {
                    delete m_mesh;
                }
            };

            /// Holds render representation for a single LOD.
            struct MeshLOD
            {
                AZStd::vector<Primitive>    m_primitives;
                AZStd::atomic_bool          m_isReady;
                bool                        m_hasDynamicMeshes;

                MeshLOD()
                    : m_hasDynamicMeshes(false)
                {
                    m_isReady.store(false);
                }

                MeshLOD(MeshLOD&& rhs)
                {
                    m_primitives = AZStd::move(rhs.m_primitives);
                    m_hasDynamicMeshes = rhs.m_hasDynamicMeshes;
                    m_isReady.store(rhs.m_isReady.load());
                }

                ~MeshLOD() = default;
            };

            friend class ActorAssetHandler;

            AZ_RTTI(ActorAsset, "{F67CC648-EA51-464C-9F5D-4A9CE41A7F86}", EMotionFXAsset)
            AZ_CLASS_ALLOCATOR_DECL

            ActorAsset();
            ~ActorAsset() override;

            typedef EMotionFXPtr<EMotionFX::ActorInstance> ActorInstancePtr;
            ActorInstancePtr CreateInstance(AZ::Entity* entity);

            EMotionFXPtr<EMotionFX::Actor> GetActor() const { return m_emfxActor; }

            size_t GetNumLODs() const { return m_meshLODs.size(); }
            MeshLOD* GetMeshLOD(size_t lodIndex) { return m_meshLODs[lodIndex].m_isReady ? &m_meshLODs[lodIndex] : nullptr; }

        private:
            EMotionFXPtr<EMotionFX::Actor>  m_emfxActor;    ///< Pointer to shared EMotionFX actor
            AZStd::vector<MeshLOD>          m_meshLODs;     ///< Mesh render data (for CryRenderer)
        };

        /**
         * Asset handler for loading and initializing actor assets.
         * The OnInitAsset stage constructs Lumberyard render meshes and materials by extracting
         * said data from the EMotionFX actor.
         */
        class ActorAssetHandler
            : public EMotionFXAssetHandler<ActorAsset>
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            bool OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
            AZ::Data::AssetType GetAssetType() const override;
            void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
            AZ::Uuid GetComponentTypeId() const override;
            const char* GetAssetTypeDisplayName() const override;
            const char* GetBrowserIcon() const override;

        private:
            static bool BuildLODMeshes(const AZ::Data::Asset<ActorAsset>& asset);
            static void FinalizeActorAssetForRendering(const AZ::Data::Asset<ActorAsset>& asset);
        };

        /**
         * Render node for managing and rendering actor instances. Each Actor Component
         * creates an ActorRenderNode. The render node is responsible for drawing meshes and
         * passing skinning transforms to the skinning pipeline.
         */
        class ActorRenderNode
            : public IRenderNode
            , private AZ::TransformNotificationBus::Handler
            , private LmbrCentral::SkeletalHierarchyRequestBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            ActorRenderNode(AZ::EntityId entityId,
                const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
                const AZ::Data::Asset<ActorAsset>& asset,
                const AZ::Transform& worldTransform);
            ~ActorRenderNode() override;

            void SetActorAsset(const AZ::Data::Asset<ActorAsset>& asset);
            void SetMaterials(const ActorAsset::MaterialList& materialPerLOD);
            void BuildRenderMeshPerLOD();

            //////////////////////////////////////////////////////////////////////////
            // IRenderNode interface implementation
            void Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo) override;
            bool GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const override;
            EERType GetRenderNodeType() override;
            const char* GetName() const override;
            const char* GetEntityClassName() const override;
            Vec3 GetPos(bool bWorldOnly = true) const override;
            void GetLocalBounds(AABB& bbox) override;
            const AABB GetBBox() const override;
            void SetBBox(const AABB& WSBBox) override;
            void OffsetPosition(const Vec3& delta) override;
            struct IPhysicalEntity* GetPhysics() const override;
            void SetPhysics(IPhysicalEntity* pPhys) override;
            void SetMaterial(_smart_ptr<IMaterial> pMat) override;
            _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = nullptr) override;
            _smart_ptr<IMaterial> GetMaterialOverride() override;
            IStatObj* GetEntityStatObj(unsigned int nPartId = 0, unsigned int nSubPartId = 0, Matrix34A* pMatrix = nullptr, bool bReturnOnlyVisible = false) override;
            _smart_ptr<IMaterial> GetEntitySlotMaterial(unsigned int nPartId, bool bReturnOnlyVisible = false, bool* pbDrawNear = nullptr) override;
            ICharacterInstance* GetEntityCharacter(unsigned int nSlot = 0, Matrix34A* pMatrix = nullptr, bool bReturnOnlyVisible = false) override;
            float GetMaxViewDist() override;
            void GetMemoryUsage(class ICrySizer* pSizer) const override;
            ////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AZ::TransformNotificationBus::Handler interface implementation
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // SkeletalHierarchyRequestBus::Handler
            AZ::u32 GetJointCount() override;
            const char* GetJointNameByIndex(AZ::u32 jointIndex) override;
            AZ::s32 GetJointIndexByName(const char* jointName) override;
            AZ::Transform GetJointTransformCharacterRelative(AZ::u32 jointIndex) override;
            //////////////////////////////////////////////////////////////////////////

            void UpdateWorldBoundingBox();
            void RegisterWithRenderer();
            void DeregisterWithRenderer();
            void UpdateWorldTransform(const AZ::Transform& entityTransform);
            SSkinningData* GetSkinningData();
            void SetSkinningMethod(SkinningMethod method);

            // Determines if the morph target weights were updated since the last call.
            // It is used to avoid calling UpdateDynamicSkin if the weights have not been
            // updated.
            bool MorphTargetWeightsWereUpdated(uint32 lodLevel);

            // Updates the vertices, normals and tangents buffers in cry based on the emfx
            // mesh. This is used to update morph targets in the ly viewport.
            void UpdateDynamicSkin(size_t lodIndex, size_t primitiveIndex);

            AZ::Data::Asset<ActorAsset>             m_actorAsset;
            EMotionFXPtr<EMotionFX::ActorInstance>  m_actorInstance;

            Matrix34                                m_renderTransform;
            AABB                                    m_worldBoundingBox;
            AZ::EntityId                            m_entityId;

            AZStd::vector<_smart_ptr<IMaterial> >    m_materialPerLOD;

            bool                                    m_isRegisteredWithRenderer;
            SkinningMethod                          m_skinningMethod;

            AZStd::vector<float>                    m_lastMorphTargetWeights;

            // history for skinning data, needed for motion blur
            struct
            {
                SSkinningData* pSkinningData;
                int nFrameID;
            } m_arrSkinningRendererData[3];  // triple buffered for motion blur

            // If our actor has dynamic skin, we need each actor instance to have its own render mesh so we can send separate
            // meshes to cry to render. If they don't have dynamic skin, the render mesh will be the same as the one in the
            // actor asset
            AZStd::vector<AZStd::vector<_smart_ptr<IRenderMesh> > > m_renderMeshesPerLOD; // Index as: [lod][primitiveNr]

            // We are using this shared_ptr as a mean to detect this object is still alive by the time BuildRenderMeshPerLOD
            // is about to be called. We are deferring BuildRenderMeshPerLOD to be executed in the main thread because it
            // is required by Cry.
            // It could happen that this ActorRenderNode is destroyed by the time the the deferred method is called.
            // To solve this situation we use a shared pointer to detect when the object got deleted. The deferred lambda 
            // holds a copy to this shared pointer (so we have to clients). 
            // If the counter of the shared_ptr.use_count == 1 when we are about to execute BuildRenderMeshPerLOD, that means 
            // only the lambda is holding it and the render node was already deleted. 
            // If the shared_ptr.use_count == 2, that means this object is still alive.
            AZStd::shared_ptr<int> m_renderNodeLifetime;
        };
    } // namespace Integration
} // namespace EMotionFX

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::EMotionFXPtr<EMotionFX::Integration::ActorAsset>, "{3F60D391-F1C8-4A40-9946-A2637D088C48}");
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::EMotionFXPtr<EMotionFX::ActorInstance>, "{169ACF47-3DEF-482A-AB7D-4CC11934D932}");
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::ActorInstance, "{280A0170-EB6A-4E90-B2F1-E18D8EAEFB36}");
}
