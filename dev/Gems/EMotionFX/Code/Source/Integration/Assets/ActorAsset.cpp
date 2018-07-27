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

#include <Integration/Assets/ActorAsset.h>
#include <Integration/System/SystemCommon.h>

#include <I3DEngine.h>
#include <IRenderMesh.h>
#include <MathConversion.h>
#include <QTangent.h>


namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(ActorAsset, EMotionFXAllocator, 0)
        AZ_CLASS_ALLOCATOR_IMPL(ActorAssetHandler, EMotionFXAllocator, 0)
        AZ_CLASS_ALLOCATOR_IMPL(ActorRenderNode, EMotionFXAllocator, 0)

        ActorRenderNode::ActorRenderNode(AZ::EntityId entityId,
            const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
            const AZ::Data::Asset<ActorAsset>& asset,
            const AZ::Transform& worldTransform)
            : m_actorInstance(actorInstance)
            , m_worldTransform(worldTransform)
            , m_renderTransform(AZTransformToLYTransform(worldTransform))
            , m_worldBoundingBox(AABB::RESET)
            , m_entityId(entityId)
            , m_isRegisteredWithRenderer(false)
        {
            SetActorAsset(asset);
            memset(m_arrSkinningRendererData, 0, sizeof(m_arrSkinningRendererData));

            if (m_entityId.IsValid())
            {
                AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
                LmbrCentral::SkeletalHierarchyRequestBus::Handler::BusConnect(m_entityId);

                AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
                EBUS_EVENT_ID_RESULT(entityTransform, m_entityId, AZ::TransformBus, GetWorldTM);
                UpdateWorldTransform(entityTransform);
            }
        }

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

            DeregisterWithRenderer();

            if (m_entityId.IsValid())
            {
                LmbrCentral::SkeletalHierarchyRequestBus::Handler::BusDisconnect(m_entityId);
                AZ::TransformNotificationBus::Handler::BusDisconnect(m_entityId);
            }
        }

        void ActorRenderNode::SetActorAsset(const AZ::Data::Asset<ActorAsset>& asset)
        {
            m_actorAsset = asset;
        }

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

        void ActorRenderNode::RegisterWithRenderer()
        {
            if (!m_isRegisteredWithRenderer && gEnv && gEnv->p3DEngine)
            {
                SetRndFlags(ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS, true);

                UpdateWorldBoundingBox();

                gEnv->p3DEngine->RegisterEntity(this);

                m_isRegisteredWithRenderer = true;
            }
        }

        void ActorRenderNode::DeregisterWithRenderer()
        {
            if (m_isRegisteredWithRenderer && gEnv && gEnv->p3DEngine)
            {
                gEnv->p3DEngine->FreeRenderNodeState(this);
                m_isRegisteredWithRenderer = false;
            }
        }

        void ActorRenderNode::UpdateWorldTransform(const AZ::Transform& entityTransform)
        {
            m_worldTransform = entityTransform;

            m_renderTransform = AZTransformToLYTransform(m_worldTransform);

            UpdateWorldBoundingBox();
        }

        void ActorRenderNode::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
        {
            AZ_UNUSED(local);
            UpdateWorldTransform(world);
        }

        AZ::u32 ActorRenderNode::GetJointCount()
        {
            if (m_actorInstance)
            {
                return m_actorInstance->GetActor()->GetSkeleton()->GetNumNodes();
            }

            return 0;
        }

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

            ActorAsset::MeshLOD* meshLOD = data->GetMeshLOD(useLodIndex);
            if (meshLOD)
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

                IRenderMesh* renderMesh = meshLOD->m_renderMesh;
                if (meshLOD->m_IsDynamic && MorphTargetWeightsWereUpdated(useLodIndex))
                {
                    UpdateDynamicSkin(useLodIndex);
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

        SSkinningData* ActorRenderNode::GetSkinningData()
        {
            // Get data to fill.
            const int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
            const int nList = nFrameID % 3;
            const int nPrevList = (nFrameID - 1) % 3;

            // Before allocating new skinning date, check if we already have for this frame.
            if (m_arrSkinningRendererData[nList].nFrameID == nFrameID && m_arrSkinningRendererData[nList].pSkinningData)
            {
                return m_arrSkinningRendererData[nList].pSkinningData;
            }

            EMotionFXPtr<EMotionFX::Actor> actorPtr = m_actorAsset.Get()->GetActor();
            const EMotionFX::TransformData* transforms = m_actorInstance->GetTransformData();
            const MCore::Matrix* nodeMatrices = transforms->GetGlobalInclusiveMatrices();
            const MCore::Matrix* invBindPoseMatrices = actorPtr->GetInverseBindPoseGlobalMatrices().GetPtr();
            const AZ::u32 transformCount = transforms->GetNumTransforms();
            const AZ::Transform worldTransformInv = MCore::EmfxTransformToAzTransform(m_actorInstance->GetGlobalTransform()).GetInverseFull();

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

        bool ActorRenderNode::GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const
        {
            for (int lodIndex = 0; lodIndex < SMeshLodInfo::s_nMaxLodCount; ++lodIndex)
            {
                distances[lodIndex] = FLT_MAX;
            }

            return true;
        }

        EERType ActorRenderNode::GetRenderNodeType()
        {
            return eERType_RenderComponent;
        }

        const char* ActorRenderNode::GetName() const
        {
            return "ActorRenderNode";
        }

        const char* ActorRenderNode::GetEntityClassName() const
        {
            return "ActorRenderNode";
        }

        Vec3 ActorRenderNode::GetPos(bool bWorldOnly /* = true */) const
        {
            return m_renderTransform.GetTranslation();
        }

        const AABB ActorRenderNode::GetBBox() const
        {
            return m_worldBoundingBox;
        }

        void ActorRenderNode::GetLocalBounds(AABB& bbox)
        {
            const MCore::AABB& emfxAabb = m_actorInstance->GetStaticBasedAABB();
            bbox = AABB(AZVec3ToLYVec3(emfxAabb.GetMin()), AZVec3ToLYVec3(emfxAabb.GetMax()));
        }

        void ActorRenderNode::SetBBox(const AABB& WSBBox)
        {
            m_worldBoundingBox = WSBBox;
        }

        void ActorRenderNode::OffsetPosition(const Vec3& delta)
        {
            // Recalculate local transform
            AZ::Transform localTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(localTransform, m_entityId, AZ::TransformBus, GetLocalTM);

            localTransform.SetTranslation(localTransform.GetTranslation() + LYVec3ToAZVec3(delta));
            EBUS_EVENT_ID(m_entityId, AZ::TransformBus, SetLocalTM, localTransform);
        }

        struct IPhysicalEntity* ActorRenderNode::GetPhysics() const
        {
            return nullptr;
        }

        void ActorRenderNode::SetPhysics(IPhysicalEntity* /*pPhys*/)
        {
        }

        void ActorRenderNode::SetMaterial(_smart_ptr<IMaterial> /*pMat*/)
        {
        }

        _smart_ptr<IMaterial> ActorRenderNode::GetMaterial(Vec3* pHitPos /* = nullptr */)
        {
            AZ_UNUSED(pHitPos);

            if (!m_materialPerLOD.empty())
            {
                return m_materialPerLOD.front();
            }

            return nullptr;
        }

        _smart_ptr<IMaterial> ActorRenderNode::GetMaterialOverride()
        {
            return nullptr;
        }

        IStatObj* ActorRenderNode::GetEntityStatObj(unsigned int /*nPartId*/, unsigned int /*nSubPartId*/, Matrix34A* /*pMatrix*/, bool /*bReturnOnlyVisible*/)
        {
            return nullptr;
        }

        _smart_ptr<IMaterial> ActorRenderNode::GetEntitySlotMaterial(unsigned int /*nPartId*/, bool /*bReturnOnlyVisible*/, bool* /*pbDrawNear */)
        {
            return GetMaterial(nullptr);
        }

        ICharacterInstance* ActorRenderNode::GetEntityCharacter(unsigned int /*nSlot*/, Matrix34A* /*pMatrix*/, bool /*bReturnOnlyVisible*/)
        {
            return nullptr;
        }

        float ActorRenderNode::GetMaxViewDist()
        {
            return(100.f * GetViewDistanceMultiplier()); // \todo
        }

        void ActorRenderNode::GetMemoryUsage(class ICrySizer* /*pSizer*/) const
        {
        }

        void ActorRenderNode::SetSkinningMethod(SkinningMethod method)
        {
            m_skinningMethod = method;
        }

        bool ActorRenderNode::MorphTargetWeightsWereUpdated(uint32 lodLevel)
        {
            bool differentMorpthTargets = false;

            MorphSetupInstance* morphSetupInstance = m_actorInstance->GetMorphSetupInstance();
            if (morphSetupInstance)
            {
                // if there is no morph setup, we have nothing to do
                MorphSetup* morphSetup = m_actorAsset.Get()->GetActor()->GetMorphSetup(lodLevel);
                if (morphSetup)
                {
                    const uint32 numTargets = morphSetup->GetNumMorphTargets();

                    if (numTargets != m_lastMorphTargetWeights.size())
                    {
                        differentMorpthTargets = true;
                        m_lastMorphTargetWeights.resize(numTargets);
                    }
                    
                    for (uint32 i = 0; i < numTargets; ++i)
                    {
                        // get the morph target
                        MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);
                        MorphSetupInstance::MorphTarget* morphTargetInstance = morphSetupInstance->FindMorphTargetByID(morphTarget->GetID());
                        if (morphTargetInstance)
                        {
                            const float currentWeight = morphTargetInstance->GetWeight();
                            if (!AZ::IsClose(currentWeight, m_lastMorphTargetWeights[i], MCore::Math::epsilon))
                            {
                                m_lastMorphTargetWeights[i] = currentWeight;
                                differentMorpthTargets = true;
                            }
                        }
                    }
                }
                else if (!m_lastMorphTargetWeights.empty())
                {
                    differentMorpthTargets = true;
                    m_lastMorphTargetWeights.clear();
                }
            }
            else if (!m_lastMorphTargetWeights.empty())
            {
                differentMorpthTargets = true;
                m_lastMorphTargetWeights.clear();
            }
            return differentMorpthTargets;
        }

        void ActorRenderNode::UpdateDynamicSkin(AZ::u32 lodIndex)
        {
            ActorAsset::MeshLOD* meshLOD = m_actorAsset.Get()->GetMeshLOD(lodIndex);
            if (!meshLOD)
            {
                return;
            }

            _smart_ptr<IRenderMesh>& renderMesh = meshLOD->m_renderMesh;

            IRenderMesh::ThreadAccessLock lockRenderMesh(renderMesh);

            strided_pointer<Vec3> destVertices;
            strided_pointer<Vec3> destNormals;
            strided_pointer<SPipQTangents> destTangents;

            destVertices.data = reinterpret_cast<Vec3*>(renderMesh->GetPosPtr(destVertices.iStride, FSL_SYSTEM_UPDATE));
            destNormals.data = reinterpret_cast<Vec3*>(renderMesh->GetNormPtr(destNormals.iStride, FSL_SYSTEM_UPDATE));

            AZ_Assert(destVertices, "Unexpected null pointer for vertices");
            AZ_Assert(destNormals, "Unexpected null pointer for normals");

            ActorAsset* assetData = m_actorAsset.Get();
            AZ_Assert(assetData, "Invalid asset data");

            m_actorInstance->UpdateMorphMeshDeformers(0.0f);

            EMotionFX::Actor* actor = assetData->GetActor().get();
            const uint32 numNodes = actor->GetNumNodes();
            const uint32 numLODs = actor->GetNumLODLevels();
            EMotionFX::Skeleton* skeleton = actor->GetSkeleton();

            uint32 destVertexIndex = 0;

            for (uint32 n = 0; n < numNodes; ++n)
            {
                const EMotionFX::Node* node = skeleton->GetNode(n);
                EMotionFX::Mesh* mesh = actor->GetMesh(lodIndex, n);

                if (!mesh || mesh->GetIsCollisionMesh())
                {
                    continue;
                }

                const EMotionFX::Mesh::EMeshType meshType = mesh->ClassifyMeshType(0, actor, node->GetNodeIndex(), false, 4, 255);
                if (meshType == EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED)
                {
                    // Skin doesn't need to be updated, however, we need to compute the
                    // meshFirstVertexId to get the right offset when we process the dynamic case
                    const uint32 numSubMeshes = mesh->GetNumSubMeshes();
                    for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                    {
                        const EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);
                        destVertexIndex += subMesh->GetNumVertices();
                    }
                    continue;
                }
                else if (meshType == EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED)
                {
                    const AZ::PackedVector3f* sourcePositions = static_cast<AZ::PackedVector3f*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
                    const AZ::PackedVector3f* sourceNormals = static_cast<AZ::PackedVector3f*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_NORMALS));
                    const AZ::Vector4* sourceTangents = static_cast<AZ::Vector4*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));

                    const uint32 numSubMeshes = mesh->GetNumSubMeshes();
                    for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                    {
                        const EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);

                        const uint32 numVertices = subMesh->GetNumVertices();
                        for (uint32 i = 0; i < numVertices; ++i)
                        {
                            const AZ::PackedVector3f& sourcePosition = sourcePositions[subMesh->GetStartVertex() + i];
                            destVertices[destVertexIndex + i] = Vec3(sourcePosition.GetX(), sourcePosition.GetY(), sourcePosition.GetZ());
                        }
                        for (uint32 i = 0; i < numVertices; ++i)
                        {
                            const AZ::PackedVector3f& sourceNormal = sourceNormals[subMesh->GetStartVertex() + i];
                            destNormals[destVertexIndex + i] = Vec3(sourceNormal.GetX(), sourceNormal.GetY(), sourceNormal.GetZ());
                        }
                        if (sourceTangents)
                        {
                            if (!destTangents)
                            {
                                destTangents.data = reinterpret_cast<SPipQTangents*>(renderMesh->GetQTangentPtr(destTangents.iStride, FSL_SYSTEM_UPDATE));
                            }
                            AZ_Assert(static_cast<bool>(destTangents), "Expected a destination tangent buffer");

                            // We only need to update the tangents if they are in the mesh, otherwise they will be 0 or not
                            // be present at the destination
                            for (uint32 i = 0; i < numVertices; ++i)
                            {
                                const AZ::Vector4& sourceTangent = sourceTangents[subMesh->GetStartVertex() + i];
                                const AZ::Vector3 sourceNormal(sourceNormals[subMesh->GetStartVertex() + i]);
                                const AZ::Vector3 bitangent = AZ::Vector3(sourceTangent.GetX(), sourceTangent.GetY(), sourceTangent.GetZ()).Cross(sourceNormal) * sourceTangent.GetW();

                                SMeshTangents meshTangent(
                                    Vec3(sourceTangent.GetX(), sourceTangent.GetY(), sourceTangent.GetZ()),
                                    Vec3(bitangent.GetX(), bitangent.GetY(), bitangent.GetZ()),
                                    Vec3(sourceNormal.GetX(), sourceNormal.GetY(), sourceNormal.GetZ()));

                                Quat q = MeshTangentFrameToQTangent(meshTangent);

                                destTangents[destVertexIndex + i] = SPipQTangents(
                                        Vec4sf(
                                            PackingSNorm::tPackF2B(q.v.x),
                                            PackingSNorm::tPackF2B(q.v.y),
                                            PackingSNorm::tPackF2B(q.v.z),
                                            PackingSNorm::tPackF2B(q.w)));
                            }
                        }
                        destVertexIndex += numVertices;
                    }
                }
            }

            renderMesh->UnlockStream(VSF_GENERAL);
            if (destTangents)
            {
                renderMesh->UnlockStream(VSF_QTANGENTS);
            }
        }

        ActorAsset::ActorAsset()
        {
        }

        ActorAsset::~ActorAsset()
        {
        }

        ActorAsset::ActorInstancePtr ActorAsset::CreateInstance(AZ::EntityId entityId)
        {
            AZ_Assert(m_emfxActor, "Anim graph asset is not loaded");
            ActorInstancePtr actorInstance = ActorInstancePtr::MakeFromNew(EMotionFX::ActorInstance::Create(m_emfxActor.get(), entityId));
            if (actorInstance)
            {
                actorInstance->SetIsOwnedByRuntime(true);
            }
            return actorInstance;
        }

        bool ActorAssetHandler::BuildLODMeshes(const AZ::Data::Asset<ActorAsset>& asset)
        {
            ActorAsset* assetData = asset.Get();
            AZ_Assert(assetData, "Invalid asset data");

            EMotionFX::Actor* actor = assetData->m_emfxActor.get();
            const uint32 numNodes = actor->GetNumNodes();
            const uint32 numLODs = actor->GetNumLODLevels();

            const uint32 maxInfluences = AZ_ARRAY_SIZE(((SMeshBoneMapping_uint16*)nullptr)->boneIds);

            EMotionFX::Skeleton* skeleton = actor->GetSkeleton();

            assetData->m_meshLODs.reserve(numLODs);

            //
            // Process all LODs from the EMotionFX actor data.
            //
            for (uint32 lodIndex = 0; lodIndex < numLODs; ++lodIndex)
            {
                assetData->m_meshLODs.push_back(ActorAsset::MeshLOD());
                ActorAsset::MeshLOD& lod = assetData->m_meshLODs.back();

                lod.m_mesh = new CMesh();
                AZ_Assert(lod.m_vertexBoneMappings.empty(), "Expected empty vertex bone mappings");

                // Get the amount of vertices and indices
                // Get the meshes to process
                uint32 verticesCount = 0;
                uint32 indicesCount = 0;
                bool hasUVs = false;
                bool hasTangents = false;
                AZStd::vector<EMotionFX::Mesh*> meshesToProcess;

                for (uint32 n = 0; n < numNodes; ++n)
                {
                    const EMotionFX::Node* node = skeleton->GetNode(n);
                    EMotionFX::Mesh* mesh = actor->GetMesh(lodIndex, n);

                    if (!mesh || mesh->GetIsCollisionMesh())
                    {
                        continue;
                    }

                    const EMotionFX::Mesh::EMeshType meshType = mesh->ClassifyMeshType(0, actor, node->GetNodeIndex(), false, 4, 255);

                    lod.m_IsDynamic |= (meshType == EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED);

                    // For each sub-mesh within each mesh, we want to create a separate sub-piece.
                    const uint32 numSubMeshes = mesh->GetNumSubMeshes();
                    for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                    {
                        const EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);
                        indicesCount += subMesh->GetNumIndices();
                        verticesCount += subMesh->GetNumVertices();

                        hasUVs |= (mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0) != nullptr);
                        hasTangents |= (mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS) != nullptr);
                    }

                    meshesToProcess.push_back(mesh);
                }

                // Destination initialization. We are going to put all meshes and submeshes for one lod
                // into one destination mesh
                lod.m_vertexBoneMappings.resize(verticesCount);
                lod.m_mesh->SetIndexCount(indicesCount);
                lod.m_mesh->SetVertexCount(verticesCount);
                // Positions and normals are reallocated by the SetVertexCount
                //lod.m_mesh->ReallocStream(CMesh::POSITIONS, 0, verticesCount);
                //lod.m_mesh->ReallocStream(CMesh::NORMALS, 0, verticesCount);
                if (hasTangents)
                {
                    lod.m_mesh->ReallocStream(CMesh::TANGENTS, 0, verticesCount);
                }
                if (hasUVs)
                {
                    lod.m_mesh->ReallocStream(CMesh::TEXCOORDS, 0, verticesCount);
                }
                lod.m_mesh->m_pBoneMapping = lod.m_vertexBoneMappings.data();

                // Pointers to the destination
                vtx_idx* targetIndices = lod.m_mesh->GetStreamPtr<vtx_idx>(CMesh::INDICES);
                Vec3* destVertices = lod.m_mesh->GetStreamPtr<Vec3>(CMesh::POSITIONS);
                Vec3* destNormals = lod.m_mesh->GetStreamPtr<Vec3>(CMesh::NORMALS);
                SMeshTexCoord* destTexCoords = lod.m_mesh->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);
                SMeshTangents* destTangents = lod.m_mesh->GetStreamPtr<SMeshTangents>(CMesh::TANGENTS);
                SMeshBoneMapping_uint16* destBoneMapping = lod.m_vertexBoneMappings.data();

                // Since we will put all the vertex/indices for all meshes/submeshes in the same array, we need to keep an
                // offset to convert indices from submesh-local to global in the lod. To do this we are going to keep the
                // index id and vertex id for the mesh and count the indices/vertices for submeshes.
                uint32 meshFirstIndexId = 0;
                uint32 meshFirstVertexId = 0;

                for (EMotionFX::Mesh* mesh : meshesToProcess)
                {
                    // Pointers to the source
                    const AZ::PackedVector3f* sourcePositions = static_cast<AZ::PackedVector3f*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
                    const AZ::PackedVector3f* sourceNormals = static_cast<AZ::PackedVector3f*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_NORMALS));
                    const AZ::u32* sourceOriginalVertex = static_cast<AZ::u32*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS));
                    const AZ::Vector4* sourceTangents = static_cast<AZ::Vector4*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));
                    const AZ::Vector2* sourceUVs = static_cast<AZ::Vector2*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0));
                    EMotionFX::SkinningInfoVertexAttributeLayer* sourceSkinningInfo = static_cast<EMotionFX::SkinningInfoVertexAttributeLayer*>(mesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID));

                    uint32 meshIndicesCount = 0;
                    uint32 meshVerticesCount = 0;

                    // For each sub-mesh within each mesh, we want to create a separate sub-piece.
                    const uint32 numSubMeshes = mesh->GetNumSubMeshes();
                    for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                    {
                        const EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);

                        lod.m_mesh->m_subsets.push_back();
                        SMeshSubset& subset = lod.m_mesh->m_subsets.back();
                        subset.nFirstIndexId = meshFirstIndexId + meshIndicesCount;
                        subset.nNumIndices = subMesh->GetNumIndices();
                        subset.nFirstVertId = meshFirstVertexId + meshVerticesCount;
                        subset.nNumVerts = subMesh->GetNumVertices();
                        subset.nMatID = subMesh->GetMaterial();
                        subset.fTexelDensity = 0.f;
                        subset.nPhysicalizeType = -1;

                        // Process indices
                        {
                            const uint32* subMeshIndices = subMesh->GetIndices();
                            for (uint32 indexIndex = 0; indexIndex < subMesh->GetNumIndices(); ++indexIndex)
                            {
                                // here we convert the index since in EMFX the indices are per mesh and in ly/cry we
                                // put all the meshes of the node into one mesh. To get the index relative to the
                                // ly/cry mesh we need to add the vertex id of the current mesh
                                *targetIndices = subMeshIndices[indexIndex] + meshFirstVertexId;
                                ++targetIndices;
                            }
                            meshIndicesCount += subMesh->GetNumIndices();
                        }

                        // Process vertices
                        const uint32 untilVertex = subMesh->GetStartVertex() + subMesh->GetNumVertices();
                        for (uint32 vertexIndex = subMesh->GetStartVertex(); vertexIndex < untilVertex; ++vertexIndex)
                        {
                            const AZ::PackedVector3f& sourcePosition = sourcePositions[vertexIndex];
                            destVertices->x = sourcePosition.GetX();
                            destVertices->y = sourcePosition.GetY();
                            destVertices->z = sourcePosition.GetZ();
                            ++destVertices;
                        }
                        meshVerticesCount += subMesh->GetNumVertices();

                        // Process normals
                        for (uint32 vertexIndex = subMesh->GetStartVertex(); vertexIndex < untilVertex; ++vertexIndex)
                        {
                            const AZ::PackedVector3f& sourceNormal = sourceNormals[vertexIndex];
                            destNormals->x = sourceNormal.GetX();
                            destNormals->y = sourceNormal.GetY();
                            destNormals->z = sourceNormal.GetZ();
                            ++destNormals;
                        }

                        // Process UVs (TextCoords)
                        if (hasUVs)
                        {
                            if (sourceUVs)
                            {
                                for (uint32 vertexIndex = subMesh->GetStartVertex(); vertexIndex < untilVertex; ++vertexIndex)
                                {
                                    const AZ::Vector2& uv = sourceUVs[vertexIndex];
                                    *destTexCoords = SMeshTexCoord(uv.GetX(), uv.GetY());
                                    ++destTexCoords;
                                }
                            }
                            else
                            {
                                for (uint32 vertexIndex = subMesh->GetStartVertex(); vertexIndex < untilVertex; ++vertexIndex)
                                {
                                    *destTexCoords = SMeshTexCoord(0.f, 0.f);
                                    ++destTexCoords;
                                }
                            }
                        }

                        // Process tangents
                        if (hasTangents)
                        {
                            if (sourceTangents)
                            {
                                for (uint32 vertexIndex = subMesh->GetStartVertex(); vertexIndex < untilVertex; ++vertexIndex)
                                {
                                    const AZ::Vector4& sourceTangent = sourceTangents[vertexIndex];
                                    const AZ::Vector3 sourceNormal(sourceNormals[vertexIndex]);
                                    const AZ::Vector3 bitangent = (AZ::Vector3(sourceTangent.GetX(), sourceTangent.GetY(), sourceTangent.GetZ())
                                        .Cross(sourceNormal) * sourceTangent.GetW()).GetNormalizedExact();

                                    *destTangents = SMeshTangents(
                                            Vec3(sourceTangent.GetX(), sourceTangent.GetY(), sourceTangent.GetZ()),
                                            Vec3(bitangent.GetX(), bitangent.GetY(), bitangent.GetZ()),
                                            Vec3(sourceNormal.GetX(), sourceNormal.GetY(), sourceNormal.GetZ()));
                                    ++destTangents;
                                }
                            }
                            else
                            {
                                for (uint32 vertexIndex = subMesh->GetStartVertex(); vertexIndex < untilVertex; ++vertexIndex)
                                {
                                    *destTangents = SMeshTangents();
                                    ++destTangents;
                                }
                            }
                        }

                        // Process AABB
                        AABB localAabb(AABB::RESET);
                        for (uint32 vertexIndex = subMesh->GetStartVertex(); vertexIndex < untilVertex; ++vertexIndex)
                        {
                            const AZ::PackedVector3f& sourcePosition = sourcePositions[vertexIndex];
                            localAabb.Add(Vec3(sourcePosition.GetX(), sourcePosition.GetY(), sourcePosition.GetZ()));
                        }

                        subset.fRadius = localAabb.GetRadius();
                        subset.vCenter = localAabb.GetCenter();

                        lod.m_mesh->m_bbox.Add(localAabb.min);
                        lod.m_mesh->m_bbox.Add(localAabb.max);

                        // Process Skinning info
                        if (sourceSkinningInfo)
                        {
                            for (uint32 vertexIndex = subMesh->GetStartVertex(); vertexIndex < untilVertex; ++vertexIndex)
                            {
                                const AZ::u32 originalVertex = sourceOriginalVertex[vertexIndex];

                                const AZ::u32 influenceCount = AZ::GetMin<AZ::u32>(maxInfluences, sourceSkinningInfo->GetNumInfluences(originalVertex));
                                AZ::u32 influenceIndex = 0;
                                int weightError = 255;

                                for (; influenceIndex < influenceCount; ++influenceIndex)
                                {
                                    EMotionFX::SkinInfluence* influence = sourceSkinningInfo->GetInfluence(originalVertex, influenceIndex);
                                    destBoneMapping->boneIds[influenceIndex] = influence->GetNodeNr();
                                    destBoneMapping->weights[influenceIndex] = static_cast<AZ::u8>(AZ::GetClamp<float>(influence->GetWeight() * 255.f, 0.f, 255.f));
                                    weightError -= destBoneMapping->weights[influenceIndex];
                                }

                                destBoneMapping->weights[0] += weightError;

                                for (; influenceIndex < maxInfluences; ++influenceIndex)
                                {
                                    destBoneMapping->boneIds[influenceIndex] = 0;
                                    destBoneMapping->weights[influenceIndex] = 0;
                                }

                                ++destBoneMapping;
                            }
                        }
                    }  // subMeshes

                    meshFirstIndexId += meshIndicesCount;
                    meshFirstVertexId += meshVerticesCount;
                }

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

            EMotionFX::Actor* actor = assetData->m_emfxActor.get();
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

        bool ActorAssetHandler::OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            ActorAsset* assetData = asset.GetAs<ActorAsset>();
            assetData->m_emfxActor = EMotionFXPtr<EMotionFX::Actor>::MakeFromNew(EMotionFX::GetImporter().LoadActor(
                        assetData->m_emfxNativeData.data(),
                        assetData->m_emfxNativeData.size()));

            if (!assetData->m_emfxActor)
            {
                AZ_Error("EMotionFX", false, "Failed to initialize actor asset %s", asset.ToString<AZStd::string>().c_str());
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

        AZ::Data::AssetType ActorAssetHandler::GetAssetType() const
        {
            return azrtti_typeid<ActorAsset>();
        }

        void ActorAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back("actor");
        }

        AZ::Uuid ActorAssetHandler::GetComponentTypeId() const
        {
            // EditorActorComponent
            return AZ::Uuid("{A863EE1B-8CFD-4EDD-BA0D-1CEC2879AD44}");
        }

        const char* ActorAssetHandler::GetAssetTypeDisplayName() const
        {
            return "EMotion FX Actor";
        }
    } //namespace Integration
} // namespace EMotionFX