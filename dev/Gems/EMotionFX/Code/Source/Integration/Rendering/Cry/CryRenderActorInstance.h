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

#include <AzCore/RTTI/RTTI.h>

#include <Integration/Rendering/Cry/CryRenderBackendCommon.h>
#include <Integration/Rendering/RenderActorInstance.h>

#include <LmbrCentral/Rendering/MaterialOwnerBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/MeshModificationBus.h>
#include <LmbrCentral/Rendering/RenderBoundsBus.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>

#include <LmbrCentral/Rendering/Utils/MaterialOwnerRequestBusHandlerImpl.h>

namespace EMotionFX
{
    class Actor;
    class ActorInstance;

    namespace Integration
    {
        class CryRenderActor;

        /**
         * Render node for managing and rendering actor instances. Each Actor Component
         * creates an ActorRenderNode. The render node is responsible for drawing meshes and
         * passing skinning transforms to the skinning pipeline.
         */
        class CryRenderActorInstance
            : public IRenderNode
            , private AZ::TransformNotificationBus::Handler
            , private LmbrCentral::MeshComponentRequestBus::Handler
            , private LmbrCentral::SkeletalHierarchyRequestBus::Handler
            , private LmbrCentral::RenderBoundsRequestBus::Handler
            , private LmbrCentral::RenderNodeRequestBus::Handler
            , public RenderActorInstance
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL
            AZ_RTTI(EMotionFX::Integration::CryRenderActorInstance, "{9C41129F-E448-4C2A-B428-0E4E624734CF}", EMotionFX::Integration::RenderActorInstance)

            CryRenderActorInstance(AZ::EntityId entityId,
                const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
                const AZ::Data::Asset<ActorAsset>& asset,
                const AZ::Transform& worldTransform);
            ~CryRenderActorInstance() override;

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

            //////////////////////////////////////////////////////////////////////////
            // AZ::TransformNotificationBus::Handler interface implementation
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            //////////////////////////////////////////////////////////////////////////
            // SkeletalHierarchyRequestBus::Handler
            AZ::u32 GetJointCount() override;
            const char* GetJointNameByIndex(AZ::u32 jointIndex) override;
            AZ::s32 GetJointIndexByName(const char* jointName) override;
            AZ::Transform GetJointTransformCharacterRelative(AZ::u32 jointIndex) override;

            //////////////////////////////////////////////////////////////////////////
            // RenderNodeRequestBus::Handler
            IRenderNode* GetRenderNode() override;
            float GetRenderNodeRequestBusOrder() const override;
            static const float s_renderNodeRequestBusOrder;

            //////////////////////////////////////////////////////////////////////////
            // RenderBoundsRequestBus::Handler
            AZ::Aabb GetWorldBounds() override { return GetWorldAABB(); }
            AZ::Aabb GetLocalBounds() override { return GetLocalAABB(); }

            //////////////////////////////////////////////////////////////////////////
            // LmbrCentral::MeshComponentRequestBus::Handler
            bool GetVisibility() override;
            void SetVisibility(bool isVisible) override;
            //AZ::Aabb GetWorldBounds() override; // Already overridden by RenderBoundsRequestBus::Handler
            //AZ::Aabb GetLocalBounds() override; // Already overridden by RenderBoundsRequestBus::Handler
            void SetMeshAsset(const AZ::Data::AssetId& id) override { (void)id; }
            AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() override { return m_actorAsset; }

            //////////////////////////////////////////////////////////////////////////
            // MaterialOwnerRequestBus interface implementation

            // Helper class needed as we inherit a SetMaterial() function from the IRenderNode
            // as well as the MaterialOwnerRequestBus
            class MaterialOwner
                : public LmbrCentral::MaterialOwnerRequestBusHandlerImpl
            {
            public:
                AZ_CLASS_ALLOCATOR_DECL

                MaterialOwner(CryRenderActorInstance* renderActorInstance, AZ::EntityId entityId);
                ~MaterialOwner();
                
                void SetMaterial(_smart_ptr<IMaterial>) override;
                _smart_ptr<IMaterial> GetMaterial() override;

            private:
                CryRenderActorInstance* m_renderActorInstance = nullptr;
            };
            friend class MaterialOwner;

            AZStd::unique_ptr<MaterialOwner> m_materialOwner;

            //////////////////////////////////////////////////////////////////////////
            // RenderActorInstance
            void OnTick(float timeDelta) override;
            void UpdateBounds() override;
            void DebugDraw(const DebugOptions& debugOptions) override;
            void SetMaterials(const ActorAsset::MaterialList& materialPerLOD) override;
            void SetIsVisible(bool isVisible) override;

            // Helpers
            void DrawAABB();
            void DrawSkeleton();
            void DrawRootTransform(const AZ::Transform& worldTransform);
            void EmfxDebugDraw();

            CryRenderActor* GetRenderActor() const;

            void UpdateWorldBoundingBox();
            void RegisterWithRenderer();
            void DeregisterWithRenderer();
            void UpdateWorldTransform(const AZ::Transform& entityTransform);
            SSkinningData* GetSkinningData();

            bool IsInCameraFrustum() const override;

            // Determines if the morph target weights were updated since the last call.
            // It is used to avoid calling UpdateDynamicSkin if the weights have not been
            // updated.
            bool MorphTargetWeightsWereUpdated(uint32 lodLevel);

            // Updates the vertices, normals and tangents buffers in cry based on the emfx
            // mesh. This is used to update morph targets in the ly viewport.
            void UpdateDynamicSkin(size_t lodIndex, size_t primitiveIndex);

            Matrix34 m_renderTransform;
            AABB m_worldBoundingBox;

            AZStd::vector<_smart_ptr<IMaterial>> m_materialPerLOD;

            bool m_isRegisteredWithRenderer = false;

            AZStd::vector<float> m_lastMorphTargetWeights;

            // history for skinning data, needed for motion blur
            struct
            {
                SSkinningData* pSkinningData = nullptr;
                int nFrameID;
            } m_arrSkinningRendererData[3];  // triple buffered for motion blur

            // Helper to store indices for meshes to be modified by other components.
            LmbrCentral::MeshModificationRequestHelper m_modificationHelper;

            // If our actor has dynamic skin, we need each actor instance to have its own render mesh so we can send separate
            // meshes to cry to render. If they don't have dynamic skin, the render mesh will be the same as the one in the
            // actor asset
            AZStd::vector<AZStd::vector<_smart_ptr<IRenderMesh>>> m_renderMeshesPerLOD; // Index as: [lod][primitiveNr]

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

            bool m_materialReadyEventSent = false; ///< Tracks whether OnMaterialOwnerReady has been sent yet. - TBD
        };
    } // namespace Integration
} // namespace EMotionFX
