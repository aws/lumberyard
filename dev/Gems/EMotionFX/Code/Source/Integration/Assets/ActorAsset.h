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
        enum class SkinningMethod:AZ::u32
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
        class ActorAsset : public EMotionFXAsset
        {
        public:

            typedef AZStd::vector<AZ::u32> BoneIndexToNodeMap;

            using MaterialList = AZStd::vector<AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset>>;

            /// Holds render representation for a single LOD.
            struct MeshLOD
            {
                _smart_ptr<IRenderMesh>                 m_renderMesh;

                CMesh*                                  m_mesh;      // Non-null only until asset is finalized.
                AZStd::vector<SMeshBoneMapping_uint16>  m_vertexBoneMappings;
                AZStd::atomic_bool                      m_isReady;

                MeshLOD()
                    : m_mesh(nullptr)
                {
                    m_isReady.store(false);
                }

                MeshLOD(MeshLOD&& rhs)
                {
                    m_mesh = rhs.m_mesh;
                    m_renderMesh = rhs.m_renderMesh;
                    m_vertexBoneMappings = AZStd::move(rhs.m_vertexBoneMappings);
                    m_isReady.store(rhs.m_isReady.load());
                    rhs.m_mesh = nullptr;
                }
            };

            friend class ActorAssetHandler;

            AZ_CLASS_ALLOCATOR(ActorAsset, EMotionFXAllocator, 0);
            AZ_RTTI(ActorAsset, "{F67CC648-EA51-464C-9F5D-4A9CE41A7F86}", EMotionFXAsset);

            ActorAsset();
            ~ActorAsset() override;

            typedef EMotionFXPtr<EMotionFX::ActorInstance> ActorInstancePtr;
            ActorInstancePtr CreateInstance(AZ::EntityId entityId);

            EMotionFXPtr<EMotionFX::Actor> GetActor() const { return m_emfxActor; }

            size_t GetNumLODs() const { return m_meshLODs.size(); }
            IRenderMesh* GetMesh(AZ::u32 lodIndex) const { return m_meshLODs[lodIndex].m_isReady ? m_meshLODs[lodIndex].m_renderMesh : nullptr; }

        private:

            EMotionFXPtr<EMotionFX::Actor>  m_emfxActor;    ///< Pointer to shared EMotionFX actor
            AZStd::vector<MeshLOD>          m_meshLODs;     ///< Mesh render data (for CryRenderer)
        };

        /**
         * Asset handler for loading and initializing actor assets.
         * The OnInitAsset stage constructs Lumberyard render meshes and materials by extracting
         * said data from the EMotionFX actor.
         */
        class ActorAssetHandler : public EMotionFXAssetHandler<ActorAsset>
        {
        public:
            AZ_CLASS_ALLOCATOR(ActorAssetHandler, EMotionFXAllocator, 0);

            bool OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
            AZ::Data::AssetType GetAssetType() const override;
            void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
            AZ::Uuid GetComponentTypeId() const override;
            const char* GetAssetTypeDisplayName() const;

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

            AZ_CLASS_ALLOCATOR(ActorRenderNode, EMotionFXAllocator, 0);

            ActorRenderNode(AZ::EntityId entityId,
                const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
                const AZ::Data::Asset<ActorAsset>& asset,
                const AZ::Transform& worldTransform);
            ~ActorRenderNode() override;

            void SetActorAsset(const AZ::Data::Asset<ActorAsset>& asset);
            void SetMaterials(const ActorAsset::MaterialList& materialPerLOD);

            //////////////////////////////////////////////////////////////////////////
            // IRenderNode interface implementation
            void Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo) override;
            bool GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const override;
            EERType GetRenderNodeType() override;
            const char* GetName() const override;
            const char* GetEntityClassName() const override;
            Vec3 GetPos(bool bWorldOnly = true) const override;
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
            void RegisterWithRenderer(bool registerWithRenderer);
            void AttachToEntity(AZ::EntityId id);
            void UpdateWorldTransform(const AZ::Transform& entityTransform);
            SSkinningData* GetSkinningData();
            void SetSkinningMethod(SkinningMethod method);

            AZ::Data::Asset<ActorAsset>             m_actorAsset;
            EMotionFXPtr<EMotionFX::ActorInstance>  m_actorInstance;

            AZ::Transform                           m_worldTransform;
            Matrix34                                m_renderTransform;
            AABB                                    m_worldBoundingBox;
            AZ::EntityId                            m_entityId;

            AZStd::vector<_smart_ptr<IMaterial>>    m_materialPerLOD;

            bool                                    m_isRegisteredWithRenderer;
            SkinningMethod                          m_skinningMethod;

            // history for skinning data, needed for motion blur
            struct
            {
                SSkinningData* pSkinningData;
                int nFrameID;
            } m_arrSkinningRendererData[3];  // triple buffered for motion blur
        };
    } // namespace Integration
    
} // namespace EMotionFX

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::EMotionFXPtr<EMotionFX::Integration::ActorAsset>, "{3F60D391-F1C8-4A40-9946-A2637D088C48}");
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::EMotionFXPtr<EMotionFX::ActorInstance>, "{169ACF47-3DEF-482A-AB7D-4CC11934D932}");
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::ActorInstance, "{280A0170-EB6A-4E90-B2F1-E18D8EAEFB36}");
}
