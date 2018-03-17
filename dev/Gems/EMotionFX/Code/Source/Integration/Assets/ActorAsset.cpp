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

#ifdef _DEBUG
#define AZ_NUMERICCAST_ENABLED 1
#endif

#include <EMotionFX_precompiled.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Jobs/LegacyJobExecutor.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Transform.h>

#include <LmbrCentral/Rendering/MeshComponentBus.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <Integration/Assets/ActorAsset.h>

#include <I3DEngine.h>
#include <IRenderMesh.h>
#include <MathConversion.h>
#include <QTangent.h>


namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        ActorRenderNode::ActorRenderNode(AZ::EntityId entityId,
            const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
            const AZ::Data::Asset<ActorAsset>& asset,
            const AZ::Transform& worldTransform)
            : m_actorInstance(actorInstance)
            , m_worldTransform(worldTransform)
            , m_renderTransform(AZTransformToLYTransform(worldTransform))
            , m_worldBoundingBox(AABB::RESET)
            , m_isRegisteredWithRenderer(false)
        {
            SetActorAsset(asset);
            AttachToEntity(entityId);
            memset(m_arrSkinningRendererData, 0, sizeof(m_arrSkinningRendererData));
        }

        //////////////////////////////////////////////////////////////////////////
        ActorRenderNode::~ActorRenderNode()
        {
            int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
            int nList = nFrameID % 3;
            if (m_arrSkinningRendererData[nList].nFrameID == nFrameID && m_arrSkinningRendererData[nList].pSkinningData)
            {
                AZ::LegacyJobExecutor* pAsyncDataJobExecutor = m_arrSkinningRendererData[nList].pSkinningData->pAsyncDataJobExecutor;
                if (pAsyncDataJobExecutor)
                {
                    pAsyncDataJobExecutor->WaitForCompletion();
                }
            }

            RegisterWithRenderer(false);
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::SetActorAsset(const AZ::Data::Asset<ActorAsset>& asset)
        {
            m_actorAsset = asset;
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::SetMaterials(const ActorAsset::MaterialList& materialPerLOD)
        {
            // Initialize materials from input paths.
            // Once materials are converted to real AZ assets, this conversion can be completely removed.
            m_materialPerLOD.clear();
            m_materialPerLOD.reserve(materialPerLOD.size());
            for (auto& materialReference : materialPerLOD)
            {
                const AZStd::string& path = materialReference.GetAssetPath();

                // Create render material. If it fails or isn't specified, use the material from base LOD.
                _smart_ptr<IMaterial> material = path.empty() ? nullptr :
                    gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(path.c_str());

                if (!material && m_materialPerLOD.size() > 0)
                {
                    material = m_materialPerLOD.front();
                }

                m_materialPerLOD.emplace_back(material);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::UpdateWorldBoundingBox()
        {
            const MCore::AABB& emfxAabb = m_actorInstance->GetStaticBasedAABB();

            m_worldBoundingBox = AABB(AZVec3ToLYVec3(emfxAabb.GetMin()), AZVec3ToLYVec3(emfxAabb.GetMax()));
            m_worldBoundingBox.SetTransformedAABB(m_renderTransform, m_worldBoundingBox);

            if (m_isRegisteredWithRenderer)
            {
                gEnv->p3DEngine->RegisterEntity(this);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::RegisterWithRenderer(bool registerWithRenderer)
        {
            if (gEnv && gEnv->p3DEngine)
            {
                if (registerWithRenderer)
                {
                    if (!m_isRegisteredWithRenderer)
                    {
                        SetRndFlags(ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS, true);

                        UpdateWorldBoundingBox();

                        gEnv->p3DEngine->RegisterEntity(this);

                        m_isRegisteredWithRenderer = true;
                    }
                }
                else
                {
                    if (m_isRegisteredWithRenderer)
                    {
                        gEnv->p3DEngine->FreeRenderNodeState(this);
                        m_isRegisteredWithRenderer = false;
                    }
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::AttachToEntity(AZ::EntityId id)
        {
            LmbrCentral::SkeletalHierarchyRequestBus::Handler::BusDisconnect();
            AZ::TransformNotificationBus::Handler::BusDisconnect(m_entityId);

            if (id.IsValid())
            {
                AZ::TransformNotificationBus::Handler::BusConnect(id);
                LmbrCentral::SkeletalHierarchyRequestBus::Handler::BusConnect(id);

                AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
                EBUS_EVENT_ID_RESULT(entityTransform, id, AZ::TransformBus, GetWorldTM);
                UpdateWorldTransform(entityTransform);
            }

            m_entityId = id;
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::UpdateWorldTransform(const AZ::Transform& entityTransform)
        {
            m_worldTransform = entityTransform;

            m_renderTransform = AZTransformToLYTransform(m_worldTransform);

            UpdateWorldBoundingBox();
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
        {
            (void)local;
            UpdateWorldTransform(world);
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::u32 ActorRenderNode::GetJointCount()
        {
            if (m_actorInstance)
            {
                return m_actorInstance->GetActor()->GetSkeleton()->GetNumNodes();
            }

            return 0;
        }

        //////////////////////////////////////////////////////////////////////////
        const char* ActorRenderNode::GetJointNameByIndex(AZ::u32 jointIndex)
        {
            if (m_actorInstance)
            {
                EMotionFX::Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
                const AZ::u32 numNodes = skeleton->GetNumNodes();
                if (jointIndex < numNodes)
                {
                    return skeleton->GetNode(jointIndex)->GetName();
                }
            }

            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::s32 ActorRenderNode::GetJointIndexByName(const char* jointName)
        {
            if (m_actorInstance)
            {
                EMotionFX::Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
                const AZ::u32 numNodes = skeleton->GetNumNodes();
                for (AZ::u32 nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
                {
                    if (0 == azstricmp(jointName, skeleton->GetNode(nodeIndex)->GetName()))
                    {
                        return nodeIndex;
                    }
                }
            }

            return -1;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Transform ActorRenderNode::GetJointTransformCharacterRelative(AZ::u32 jointIndex)
        {
            if (m_actorInstance)
            {
                const EMotionFX::TransformData* transforms = m_actorInstance->GetTransformData();
                if (transforms && jointIndex < transforms->GetNumTransforms())
                {
                    return MCore::EmfxTransformToAzTransform(transforms->GetCurrentPose()->GetGlobalTransform(jointIndex));
                }
            }

            return AZ::Transform::CreateIdentity();
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo)
        {
            ActorAsset* data = m_actorAsset.Get();

            if (!data)
            {
                // Asset is not loaded.
                AZ_WarningOnce("ActorRenderNode", false, "Actor asset is not loaded. Rendering aborted.");
                return;
            }

            if (!m_renderTransform.IsValid())
            {
                AZ_Warning("ActorRenderNode", false, "Render node has no valid transform.");
                return;
            }

            if (data->GetNumLODs() == 0)
            {
                return;
            }

            AZ::u32 useLodIndex = 0; // \todo - choose LOD

            SRendParams rParams(inRenderParams);

            rParams.fAlpha = 1.f;

            IMaterial* previousMaterial = rParams.pMaterial;
            const int previousObjectFlags = rParams.dwFObjFlags;

            rParams.dwFObjFlags |= FOB_DYNAMIC_OBJECT;

            rParams.pMatrix = &m_renderTransform;

            rParams.lodValue = rParams.lodValue.LodA();

            CRenderObject* pObj = gEnv->pRenderer->EF_GetObject_Temp(passInfo.ThreadID());

            pObj->m_fSort = rParams.fCustomSortOffset;
            pObj->m_fAlpha = rParams.fAlpha;
            pObj->m_fDistance = rParams.fDistance;
            pObj->m_II.m_AmbColor = rParams.AmbientColor;

            SRenderObjData* pD = gEnv->pRenderer->EF_GetObjData(pObj, true, passInfo.ThreadID());

            if (rParams.pShaderParams && rParams.pShaderParams->size() > 0)
            {
                pD->SetShaderParams(rParams.pShaderParams);
            }

            pD->m_uniqueObjectId = reinterpret_cast<uintptr_t>(this);

            rParams.pMatrix = &m_renderTransform;

            pObj->m_II.m_Matrix = *rParams.pMatrix;
            pObj->m_nClipVolumeStencilRef = rParams.nClipVolumeStencilRef;
            pObj->m_nTextureID = rParams.nTextureID;

            rParams.dwFObjFlags |= rParams.dwFObjFlags;
            rParams.dwFObjFlags &= ~FOB_NEAREST;

            pObj->m_nMaterialLayers = rParams.nMaterialLayersBlend;

            pD->m_nHUDSilhouetteParams = rParams.nHUDSilhouettesParams;

            pD->m_nCustomData = rParams.nCustomData;
            pD->m_nCustomFlags = rParams.nCustomFlags;

            pObj->m_DissolveRef = rParams.nDissolveRef;

            pObj->m_nSort = fastround_positive(rParams.fDistance * 2.0f);

            if (SSkinningData* skinningData = GetSkinningData())
            {
                pD->m_pSkinningData = skinningData;
                pObj->m_ObjFlags |= FOB_SKINNED;
                pObj->m_ObjFlags |= FOB_DYNAMIC_OBJECT;
                pObj->m_ObjFlags |= FOB_MOTION_BLUR;

                // Shader code is associating this with skin offset - this parameter is currently not used by our skeleton
                pD->m_fTempVars[0] = pD->m_fTempVars[1] = pD->m_fTempVars[2] = 0;
            }

            IRenderMesh* renderMesh = data->GetMesh(useLodIndex);
            if (renderMesh)
            {
                IMaterial* pMaterial = rParams.pMaterial;

                // Grab material for this LOD.
                if (!pMaterial && !m_materialPerLOD.empty())
                {
                    const AZ::u32 materialIndex = AZ::GetClamp<AZ::u32>(useLodIndex, 0, m_materialPerLOD.size() - 1);
                    pMaterial = m_materialPerLOD[materialIndex];
                }

                // Otherwise, fall back to default material.
                if (!pMaterial)
                {
                    pMaterial = gEnv->p3DEngine->GetMaterialManager()->GetDefaultMaterial();
                }

                if (renderMesh)
                {
                    renderMesh->Render(rParams, pObj, pMaterial, passInfo);
                }
            }

            // Restore previous state.
            rParams.pMaterial = previousMaterial;
            rParams.dwFObjFlags = previousObjectFlags;
        }

        //////////////////////////////////////////////////////////////////////////
        SSkinningData* ActorRenderNode::GetSkinningData()
        {
            ActorAsset* assetData = m_actorAsset.Get();
            auto actorPtr = assetData->GetActor();
            const EMotionFX::TransformData* transforms = m_actorInstance->GetTransformData();
            const MCore::Matrix* nodeMatrices = transforms->GetGlobalInclusiveMatrices();
            const MCore::Matrix* invBindPoseMatrices = actorPtr->GetInverseBindPoseGlobalMatrices().GetPtr();
            const AZ::u32 transformCount = transforms->GetNumTransforms();

            // Get the base transformations from the actor instance so that the bone transformations and the world transformations are being derived from the same place.
            const AZ::Quaternion entityOrientation = MCore::EmfxQuatToAzQuat(m_actorInstance->GetGlobalRotation());
            const AZ::Vector3 entityPosition = m_actorInstance->GetGlobalPosition();
            const AZ::Transform worldTransformInv = MCore::EmfxTransformToAzTransform(m_actorInstance->GetGlobalTransform()).GetInverseFull();

            //---------------------------------------
            // Get data to fill.
            const int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
            const int nList = nFrameID % 3;
            const int nPrevList = (nFrameID - 1) % 3;

            // Before allocating new skinning date, check if we already have for this frame.
            if (m_arrSkinningRendererData[nList].nFrameID == nFrameID && m_arrSkinningRendererData[nList].pSkinningData)
            {
                return m_arrSkinningRendererData[nList].pSkinningData;
            }

            SSkinningData* renderSkinningData = gEnv->pRenderer->EF_CreateSkinningData(transformCount, false, m_skinningMethod == SkinningMethod::Linear);

            if (m_skinningMethod == SkinningMethod::Linear)
            {
                Matrix34* renderTransforms = renderSkinningData->pBoneMatrices;

                for (AZ::u32 transformIndex = 0; transformIndex < transformCount; ++transformIndex)
                {
                    const AZ::Transform bindTransform = EmfxTransformToAzTransform(invBindPoseMatrices[transformIndex]);
                    AZ::Transform boneTransform = EmfxTransformToAzTransform(nodeMatrices[transformIndex]);
                    boneTransform = worldTransformInv * boneTransform * bindTransform;

                    renderTransforms[transformIndex] = AZTransformToLYTransform(boneTransform);
                }
            }
            else if (m_skinningMethod == SkinningMethod::DualQuat)
            {
                DualQuat* renderTransforms = renderSkinningData->pBoneQuatsS;

                const AZ::Transform worldTransformInvNoScale = AZ::Transform::CreateFromQuaternionAndTranslation(entityOrientation, entityPosition).GetInverseFull();
                for (AZ::u32 transformIndex = 0; transformIndex < transformCount; ++transformIndex)
                {
                    const AZ::Transform bindTransform = EmfxTransformToAzTransform(invBindPoseMatrices[transformIndex]);
                    AZ::Transform boneTransform = EmfxTransformToAzTransform(nodeMatrices[transformIndex]);
                    boneTransform = worldTransformInv * boneTransform * bindTransform;

                    renderTransforms[transformIndex] = DualQuat(AZTransformToLYTransform(boneTransform));
                }
            }

            // Set data for motion blur.
            if (m_arrSkinningRendererData[nPrevList].nFrameID == (nFrameID - 1) && m_arrSkinningRendererData[nPrevList].pSkinningData)
            {
                renderSkinningData->nHWSkinningFlags |= eHWS_MotionBlured;
                renderSkinningData->pPreviousSkinningRenderData = m_arrSkinningRendererData[nPrevList].pSkinningData;
                AZ::LegacyJobExecutor* pAsyncDataJobExecutor = renderSkinningData->pPreviousSkinningRenderData->pAsyncDataJobExecutor;
                if (pAsyncDataJobExecutor)
                {
                    pAsyncDataJobExecutor->WaitForCompletion();
                }
            }
            else
            {
                // If we don't have motion blur data, use the some as for the current frame.
                renderSkinningData->pPreviousSkinningRenderData = renderSkinningData;
            }

            m_arrSkinningRendererData[nList].nFrameID = nFrameID;
            m_arrSkinningRendererData[nList].pSkinningData = renderSkinningData;

            return renderSkinningData;
        }

        //////////////////////////////////////////////////////////////////////////
        bool ActorRenderNode::GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const
        {
            for (int lodIndex = 0; lodIndex < SMeshLodInfo::s_nMaxLodCount; ++lodIndex)
            {
                distances[lodIndex] = FLT_MAX;
            }

            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        EERType ActorRenderNode::GetRenderNodeType()
        {
            return eERType_RenderComponent;
        }

        //////////////////////////////////////////////////////////////////////////
        const char* ActorRenderNode::GetName() const
        {
            return "ActorRenderNode";
        }

        //////////////////////////////////////////////////////////////////////////
        const char* ActorRenderNode::GetEntityClassName() const
        {
            return "ActorRenderNode";
        }

        //////////////////////////////////////////////////////////////////////////
        Vec3 ActorRenderNode::GetPos(bool bWorldOnly /* = true */) const
        {
            return m_renderTransform.GetTranslation();
        }

        //////////////////////////////////////////////////////////////////////////
        const AABB ActorRenderNode::GetBBox() const
        {
            return m_worldBoundingBox;
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::SetBBox(const AABB& WSBBox)
        {
            m_worldBoundingBox = WSBBox;
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::OffsetPosition(const Vec3& delta)
        {
            // Recalculate local transform
            AZ::Transform localTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(localTransform, m_entityId, AZ::TransformBus, GetLocalTM);

            localTransform.SetTranslation(localTransform.GetTranslation() + LYVec3ToAZVec3(delta));
            EBUS_EVENT_ID(m_entityId, AZ::TransformBus, SetLocalTM, localTransform);
        }

        //////////////////////////////////////////////////////////////////////////
        struct IPhysicalEntity* ActorRenderNode::GetPhysics() const
        {
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::SetPhysics(IPhysicalEntity* /*pPhys*/)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::SetMaterial(_smart_ptr<IMaterial> /*pMat*/)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        _smart_ptr<IMaterial> ActorRenderNode::GetMaterial(Vec3* pHitPos /* = nullptr */)
        {
            (void)pHitPos;

            if (!m_materialPerLOD.empty())
            {
                return m_materialPerLOD.front();
            }

            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        _smart_ptr<IMaterial> ActorRenderNode::GetMaterialOverride()
        {
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        IStatObj* ActorRenderNode::GetEntityStatObj(unsigned int /*nPartId*/, unsigned int /*nSubPartId*/, Matrix34A* /*pMatrix*/, bool /*bReturnOnlyVisible*/)
        {
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        _smart_ptr<IMaterial> ActorRenderNode::GetEntitySlotMaterial(unsigned int /*nPartId*/, bool /*bReturnOnlyVisible*/, bool* /*pbDrawNear */)
        {
            return GetMaterial(nullptr);
        }

        //////////////////////////////////////////////////////////////////////////
        ICharacterInstance* ActorRenderNode::GetEntityCharacter(unsigned int /*nSlot*/, Matrix34A* /*pMatrix*/, bool /*bReturnOnlyVisible*/)
        {
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        float ActorRenderNode::GetMaxViewDist()
        {
            return(100.f * GetViewDistanceMultiplier()); // \todo
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::GetMemoryUsage(class ICrySizer* /*pSizer*/) const
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorRenderNode::SetSkinningMethod(SkinningMethod method)
        {
            m_skinningMethod = method;
        }

        //////////////////////////////////////////////////////////////////////////
        ActorAsset::ActorAsset()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        ActorAsset::~ActorAsset()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        ActorAsset::ActorInstancePtr ActorAsset::CreateInstance(AZ::EntityId entityId)
        {
            AZ_Assert(m_emfxActor, "Anim graph asset is not loaded");
            auto actorInstance = ActorInstancePtr::MakeFromNew(EMotionFX::ActorInstance::Create(m_emfxActor.get(), entityId));
            if (actorInstance)
            {
                actorInstance->SetIsOwnedByRuntime(true);
            }
            return actorInstance;
        }

        //////////////////////////////////////////////////////////////////////////
        bool ActorAssetHandler::BuildLODMeshes(const AZ::Data::Asset<ActorAsset>& asset)
        {
            ActorAsset* assetData = asset.Get();
            AZ_Assert(assetData, "Invalid asset data");

            auto* actor = assetData->m_emfxActor.get();
            const uint32 numNodes = actor->GetNumNodes();
            const AZ::u32 numLODs = actor->GetNumLODLevels();

            const AZ::u32 maxInfluences = AZ_ARRAY_SIZE(((SMeshBoneMapping_uint16*)nullptr)->boneIds);

            EMotionFX::Skeleton* skeleton = actor->GetSkeleton();

            //
            // Buffers re-used during the construction of all LODs.
            //

            AZStd::vector<AZ::Vector3> vertexBuffer;
            AZStd::vector<AZ::Vector3> vertexNormals;
            AZStd::vector<AZ::Vector4> vertexTangents;
            AZStd::vector<AZ::Vector2> vertexUVs;
            AZStd::vector<vtx_idx> indexBuffer;

            //
            // Process all LODs from the EMotionFX actor data.
            //

            for (AZ::u32 lodIndex = 0; lodIndex < numLODs; ++lodIndex)
            {
                vertexBuffer.clear();
                vertexNormals.clear();
                vertexTangents.clear();
                vertexUVs.clear();
                indexBuffer.clear();

                assetData->m_meshLODs.emplace_back();
                ActorAsset::MeshLOD& lod = assetData->m_meshLODs.back();

                lod.m_mesh = new CMesh();
                lod.m_vertexBoneMappings.clear();

                //
                // Populate vertex streams and bone influences.
                //

                for (uint32 n = 0; n < numNodes; ++n)
                {
                    const EMotionFX::Node* node = skeleton->GetNode(n);
                    const EMotionFX::Mesh* mesh = actor->GetMesh(lodIndex, n);

                    if (!mesh || mesh->GetIsCollisionMesh())
                    {
                        continue;
                    }

                    const EMotionFX::Mesh::EMeshType meshType = mesh->ClassifyMeshType(0, actor, node->GetNodeIndex(), false, 4, 255);
                    if (meshType != EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED)
                    {
                        continue;
                    }

                    // Initial reserves to minimize first-time allocs.
                    const size_t numVertices = mesh->GetNumVertices();
                    vertexBuffer.reserve(numVertices);
                    vertexNormals.reserve(numVertices);
                    vertexTangents.reserve(numVertices);
                    vertexUVs.reserve(numVertices);
                    lod.m_vertexBoneMappings.reserve(numVertices);
                    indexBuffer.reserve(mesh->GetNumIndices());

                    const AZ::PackedVector3f* positions = static_cast<AZ::PackedVector3f*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
                    const AZ::PackedVector3f* normals = static_cast<AZ::PackedVector3f*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_NORMALS));
                    const AZ::u32* orgVerts = static_cast<AZ::u32*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS));
                    const AZ::Vector4* tangents = static_cast<AZ::Vector4*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));
                    const AZ::Vector2* uvsA = static_cast<AZ::Vector2*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0));
                    EMotionFX::SkinningInfoVertexAttributeLayer* skinningInfo = static_cast<EMotionFX::SkinningInfoVertexAttributeLayer*>(mesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID));

                    const AZ::u32 meshFirstVertex = vertexBuffer.size();

                    // For each sub-mesh within each mesh, we want to create a separate sub-piece.
                    const AZ::u32 numSubMeshes = mesh->GetNumSubMeshes();
                    for (AZ::u32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                    {
                        const EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);

                        const AZ::u32 numSubMeshIndices = subMesh->GetNumIndices();
                        const AZ::u32 numSubMeshVertices = subMesh->GetNumVertices();

                        lod.m_mesh->m_subsets.push_back();
                        SMeshSubset& subset = lod.m_mesh->m_subsets.back();
                        subset.nFirstIndexId = indexBuffer.size();
                        subset.nNumIndices = numSubMeshIndices;
                        subset.nFirstVertId = vertexBuffer.size();
                        subset.nNumVerts = numSubMeshVertices;
                        subset.nMatID = subMesh->GetMaterial();
                        subset.fTexelDensity = 0.f;
                        subset.nPhysicalizeType = -1;

                        // Subset's index values are relative to its own vertex block.
                        const uint32* subMeshIndices = subMesh->GetIndices();
                        for (AZ::u32 i = 0; i < numSubMeshIndices; ++i)
                        {
                            const uint32 meshIndex = subMeshIndices[i] + meshFirstVertex;
                            indexBuffer.push_back(aznumeric_cast<vtx_idx>(meshIndex));
                        }

                        AABB localAabb(AABB::RESET);

                        // Populate vertex streams and bone influences.
                        for (AZ::u32 v = 0; v < numSubMeshVertices; ++v)
                        {
                            const uint32 meshVertIndex = v + subMesh->GetStartVertex();
                            const uint32 orgVertex = orgVerts[meshVertIndex];

                            const AZ::Vector3 position = AZ::Vector3(positions[meshVertIndex]);
                            localAabb.Add(Vec3(position.GetX(), position.GetY(), position.GetZ()));

                            vertexBuffer.push_back(position);
                            vertexNormals.push_back(AZ::Vector3(normals[meshVertIndex]));

                            vertexTangents.push_back(tangents ? tangents[meshVertIndex] : AZ::Vector4::CreateZero());
                            vertexUVs.push_back(uvsA ? uvsA[meshVertIndex] : AZ::Vector2::CreateZero());

                            lod.m_vertexBoneMappings.emplace_back();
                            auto& boneMapping = lod.m_vertexBoneMappings.back();

                            const AZ::u32 influenceCount = skinningInfo ? AZ::GetMin<AZ::u32>(maxInfluences, skinningInfo->GetNumInfluences(orgVertex)) : 0;
                            AZ::u32 influenceIndex = 0;

                            if (skinningInfo)
                            {
                                for (; influenceIndex < influenceCount; ++influenceIndex)
                                {
                                    EMotionFX::SkinInfluence* influence = skinningInfo->GetInfluence(orgVertex, influenceIndex);
                                    boneMapping.boneIds[influenceIndex] = influence->GetNodeNr();
                                    boneMapping.weights[influenceIndex] = static_cast<AZ::u8>(AZ::GetClamp<float>(influence->GetWeight() * 255.f, 0.f, 255.f));
                                }
                                for (; influenceIndex < maxInfluences; ++influenceIndex)
                                {
                                    boneMapping.boneIds[influenceIndex] = 0;
                                    boneMapping.weights[influenceIndex] = 0;
                                }
                            }
                        }

                        subset.fRadius = localAabb.GetRadius();
                        subset.vCenter = localAabb.GetCenter();
                    }
                }

                //
                // Populate CMesh for this LOD, which is used to construct the RenderMesh.
                //

                lod.m_mesh->SetIndexCount(indexBuffer.size());
                lod.m_mesh->SetVertexCount(vertexBuffer.size());
                lod.m_mesh->ReallocStream(CMesh::POSITIONS, 0, vertexBuffer.size());
                lod.m_mesh->ReallocStream(CMesh::TEXCOORDS, 0, vertexUVs.size());
                lod.m_mesh->ReallocStream(CMesh::NORMALS, 0, vertexNormals.size());
                lod.m_mesh->ReallocStream(CMesh::TANGENTS, 0, vertexTangents.size());
                Vec3* pVertices = lod.m_mesh->GetStreamPtr<Vec3>(CMesh::POSITIONS);
                SMeshTexCoord* pTexCoords = lod.m_mesh->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);
                Vec3* pNormals = lod.m_mesh->GetStreamPtr<Vec3>(CMesh::NORMALS);
                SMeshTangents* pTangents = lod.m_mesh->GetStreamPtr<SMeshTangents>(CMesh::TANGENTS);

                lod.m_mesh->m_pBoneMapping = lod.m_vertexBoneMappings.data();

                for (AZ::u32 vertIndex = 0, vertCount = vertexBuffer.size(); vertIndex < vertCount; ++vertIndex)
                {
                    const AZ::Vector3& vertex = vertexBuffer[vertIndex];
                    const AZ::Vector3& normal = vertexNormals[vertIndex];
                    const AZ::Vector4& tangent = vertexTangents[vertIndex];

                    const AZ::Vector2& uv = vertexUVs[vertIndex];

                    AZ::Vector3 bitangent = AZ::Vector3(tangent.GetX(), tangent.GetY(), tangent.GetZ()).Cross(normal) * tangent.GetW();

                    lod.m_mesh->m_bbox.Add(Vec3(vertex.GetX(), vertex.GetY(), vertex.GetZ()));

                    *pVertices++ = Vec3(vertex.GetX(), vertex.GetY(), vertex.GetZ());
                    *pNormals++ = Vec3(normal.GetX(), normal.GetY(), normal.GetZ());
                    *pTangents++ = SMeshTangents(
                            Vec3(tangent.GetX(), tangent.GetY(), tangent.GetZ()),
                            Vec3(bitangent.GetX(), bitangent.GetY(), bitangent.GetZ()),
                            Vec3(normal.GetX(), normal.GetY(), normal.GetZ()));
                    *pTexCoords++ = SMeshTexCoord(uv.GetX(), uv.GetY());
                }

                // Indices can be copied directly.
                vtx_idx* targetIndices = lod.m_mesh->GetStreamPtr<vtx_idx>(CMesh::INDICES);
                memcpy(targetIndices, indexBuffer.data(), sizeof(vtx_idx) * indexBuffer.size());

                //////////////////////////////////////////////////////////////////////////
                // Hacks lifted from legacy character code.
                uint32 numMeshSubsets = lod.m_mesh->m_subsets.size();
                for (uint32 i = 0; i < numMeshSubsets; i++)
                {
                    lod.m_mesh->m_subsets[i].FixRanges(lod.m_mesh->m_pIndices);
                }

                // Convert tangent frame from matrix to quaternion based.
                // Without this, materials do NOT render correctly on skinned characters.
                if (lod.m_mesh->m_pTangents && !lod.m_mesh->m_pQTangents)
                {
                    lod.m_mesh->m_pQTangents = (SMeshQTangents*)lod.m_mesh->m_pTangents;
                    MeshTangentsFrameToQTangents(
                        lod.m_mesh->m_pTangents, sizeof(lod.m_mesh->m_pTangents[0]), lod.m_mesh->GetVertexCount(),
                        lod.m_mesh->m_pQTangents, sizeof(lod.m_mesh->m_pQTangents[0]));
                }
                //////////////////////////////////////////////////////////////////////////
            }

            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorAssetHandler::FinalizeActorAssetForRendering(const AZ::Data::Asset<ActorAsset>& asset)
        {
            //
            // The CMesh, which contains vertex streams, indices, uvs, bone influences, etc, is computed within
            // the job thread.
            // However, the render mesh and material need to be constructed on the main thread, as imposed by
            // the renderer. Naturally this is undesirable, but a limitation of the engine at the moment.
            //
            // The material also cannot be constructed natively. Materials only seem to be fully valid
            // if loaded from Xml data. Attempts to build procedurally, outside of the renderer code, have
            // been unsuccessful due to some aspects of the data being inaccessible.
            // Jumping through this hoop is acceptable for now since we'll soon be generating the material asset
            // in the asset pipeline and loading it via the game, as opposed to extracting the data here.
            //

            ActorAsset* assetData = asset.Get();
            AZ_Assert(assetData, "Invalid asset data");

            AZStd::string assetPath;
            EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());

            AZStd::string assetName;
            AZStd::string assetFolder;
            AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), assetName);
            AzFramework::StringFunc::Path::GetFolderPath(assetPath.c_str(), assetFolder);

            auto* actor = assetData->m_emfxActor.get();
            const uint32 numNodes = actor->GetNumNodes();
            const AZ::u32 numLODs = actor->GetNumLODLevels();

            //
            // Process materials all LODs from the EMotionFX actor data.
            //

            for (AZ::u32 lodIndex = 0; lodIndex < numLODs; ++lodIndex)
            {
                ActorAsset::MeshLOD& lod = assetData->m_meshLODs[lodIndex];

                // Create and initialize render mesh.
                lod.m_renderMesh = gEnv->pRenderer->CreateRenderMesh("EMotion FX Actor", assetPath.c_str(), nullptr, eRMT_Dynamic);
                const AZ::u32 renderMeshFlags = FSM_ENABLE_NORMALSTREAM | FSM_VERTEX_VELOCITY;
                lod.m_renderMesh->SetMesh(*lod.m_mesh, 0, renderMeshFlags, false);

                // Free temporary load objects & buffers.
                delete lod.m_mesh;
                lod.m_mesh = nullptr;
                lod.m_vertexBoneMappings.resize(0);

                // It's now safe to use this LOD.
                lod.m_isReady = true;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        bool ActorAssetHandler::OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            ActorAsset* assetData = asset.GetAs<ActorAsset>();
            assetData->m_emfxActor = EMotionFXPtr<EMotionFX::Actor>::MakeFromNew(EMotionFX::GetImporter().LoadActor(
                        assetData->m_emfxNativeData.data(),
                        assetData->m_emfxNativeData.size()));

            if (!assetData->m_emfxActor)
            {
                AZ_Error("EMotionFX", false, "Failed to initialize actor asset %s", asset.GetId().ToString<AZStd::string>().c_str());
                return false;
            }

            assetData->m_emfxActor->SetIsOwnedByRuntime(true);

            // Populate CMeshes on the job thread, so we can at least build data streams asynchronously.
            if (!BuildLODMeshes(asset))
            {
                return false;
            }

            AZStd::function<void()> finalizeOnMainThread = [asset]()
                {
                    // RenderMesh creation must be performed on the main thread,
                    // as required by the renderer.
                    FinalizeActorAssetForRendering(asset);
                };

            AZ::SystemTickBus::QueueFunction(finalizeOnMainThread);

            return (assetData->m_emfxActor);
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Data::AssetType ActorAssetHandler::GetAssetType() const
        {
            return AZ::AzTypeInfo<ActorAsset>::Uuid();
        }

        //////////////////////////////////////////////////////////////////////////
        void ActorAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back("actor");
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Uuid ActorAssetHandler::GetComponentTypeId() const
        {
            // EditorActorComponent
            return AZ::Uuid("{A863EE1B-8CFD-4EDD-BA0D-1CEC2879AD44}");
        }

        //////////////////////////////////////////////////////////////////////////
        const char* ActorAssetHandler::GetAssetTypeDisplayName() const
        {
            return "EMotionFX Actor";
        }
    } //namespace Integration
} // namespace EMotionFX