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
#include <Integration/System/SystemComponent.h>

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
            , m_renderTransform(AZTransformToLYTransform(worldTransform))
            , m_worldBoundingBox(AABB::RESET)
            , m_entityId(entityId)
            , m_isRegisteredWithRenderer(false)
            , m_renderNodeLifetime(new int)
        {
            SetActorAsset(asset);
            memset(m_arrSkinningRendererData, 0, sizeof(m_arrSkinningRendererData));

            AZStd::shared_ptr<int>& renderNodeLifetime = m_renderNodeLifetime;
            AZStd::function<void()> finalizeOnMainThread = [this, renderNodeLifetime]()
                {
                    // RenderMesh creation must be performed on the main thread,
                    // as required by the renderer.
                    if (renderNodeLifetime.use_count() != 1)
                    {
                        BuildRenderMeshPerLOD();
                    }
                };
            AZ::SystemTickBus::QueueFunction(finalizeOnMainThread);

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
            if (gEnv)
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
            if (gEnv && gEnv->p3DEngine)
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
        }

        void ActorRenderNode::UpdateWorldBoundingBox()
        {
            const MCore::AABB& emfxAabb = m_actorInstance->GetAABB();
            m_worldBoundingBox = AABB(AZVec3ToLYVec3(emfxAabb.GetMin()), AZVec3ToLYVec3(emfxAabb.GetMax()));
            
            if (m_isRegisteredWithRenderer)
            {
                gEnv->p3DEngine->RegisterEntity(this);
            }
        }

        void ActorRenderNode::RegisterWithRenderer()
        {
            if (!m_isRegisteredWithRenderer && gEnv && gEnv->p3DEngine)
            {
                SetRndFlags(ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS | ERF_COMPONENT_ENTITY, true);

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
            m_renderTransform = AZTransformToLYTransform(entityTransform);
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
            if (m_actorInstance && jointName)
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
                    return MCore::EmfxTransformToAzTransform(transforms->GetCurrentPose()->GetModelSpaceTransform(jointIndex));
                }
            }

            return AZ::Transform::CreateIdentity();
        }

        Matrix34 MCoreMatrixToCryMatrix(const MCore::Matrix& matrix)
        {
            return Matrix34(
                matrix.m44[0][0],
                matrix.m44[1][0],
                matrix.m44[2][0],
                matrix.m44[3][0],
                matrix.m44[0][1],
                matrix.m44[1][1],
                matrix.m44[2][1],
                matrix.m44[3][1],
                matrix.m44[0][2],
                matrix.m44[1][2],
                matrix.m44[2][2],
                matrix.m44[3][2]);
        }

        DualQuat MCoreMatrixToCryDualQuat(const MCore::Matrix& matrix)
        {
            Matrix34 cryMatrix = MCoreMatrixToCryMatrix(matrix);
            cryMatrix.OrthonormalizeFast();
            return DualQuat(cryMatrix);
        }

        void ActorRenderNode::Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo)
        {
            if (!SystemComponent::emfx_actorRenderEnabled)
            {
                return;
            }

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

            if (data->GetNumLODs() == 0 || m_renderMeshesPerLOD.empty())
            {
                return; // not ready for rendering
            }

            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Animation, "ActorRenderNode::Render");

            AZ::u32 useLodIndex = m_actorInstance->GetLODLevel();

            SRendParams rParams(inRenderParams);
            rParams.fAlpha = 1.f;
            IMaterial* previousMaterial = rParams.pMaterial;
            const int previousObjectFlags = rParams.dwFObjFlags;
            rParams.dwFObjFlags |= FOB_DYNAMIC_OBJECT;
            rParams.pMatrix = &m_renderTransform;
            rParams.lodValue = useLodIndex;

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
            pObj->m_ObjFlags |= rParams.dwFObjFlags;
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
                if (meshLOD->m_hasDynamicMeshes)
                {
                    m_actorInstance->UpdateMorphMeshDeformers(0.0f);
                }

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

                const bool morphsUpdated = MorphTargetWeightsWereUpdated(useLodIndex);
                const size_t numPrimitives = meshLOD->m_primitives.size();
                for (size_t prim = 0; prim < numPrimitives; ++prim)
                {
                    const ActorAsset::Primitive& primitive = meshLOD->m_primitives[prim];
                    if (primitive.m_isDynamic && morphsUpdated)
                    {
                        UpdateDynamicSkin(useLodIndex, prim);
                    }

                    if (useLodIndex < m_renderMeshesPerLOD.size() && prim < m_renderMeshesPerLOD[useLodIndex].size())
                    {
                        IRenderMesh* renderMesh = m_renderMeshesPerLOD[useLodIndex][prim];
                        if (renderMesh)
                        {
                            renderMesh->Render(rParams, pObj, pMaterial, passInfo);
                        }
                    }
                }
            }

            // Restore previous state.
            rParams.pMaterial = previousMaterial;
            rParams.dwFObjFlags = previousObjectFlags;
        }

        SSkinningData* ActorRenderNode::GetSkinningData()
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Animation, "ActorRenderNode::GetSkinningData");
            // Get data to fill.
            const int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
            const int nList = nFrameID % 3;
            const int nPrevList = (nFrameID - 1) % 3;

            // Before allocating new skinning date, check if we already have for this frame.
            if (m_arrSkinningRendererData[nList].nFrameID == nFrameID && m_arrSkinningRendererData[nList].pSkinningData)
            {
                return m_arrSkinningRendererData[nList].pSkinningData;
            }

            const EMotionFX::TransformData* transforms = m_actorInstance->GetTransformData();
            const MCore::Matrix* skinningMatrices = transforms->GetSkinningMatrices();
            const AZ::u32 transformCount = transforms->GetNumTransforms();

            SSkinningData* renderSkinningData = gEnv->pRenderer->EF_CreateSkinningData(transformCount, false, m_skinningMethod == SkinningMethod::Linear);

            if (m_skinningMethod == SkinningMethod::Linear)
            {
                Matrix34* renderTransforms = renderSkinningData->pBoneMatrices;

                for (AZ::u32 transformIndex = 0; transformIndex < transformCount; ++transformIndex)
                {
                    renderTransforms[transformIndex] = MCoreMatrixToCryMatrix(skinningMatrices[transformIndex]);
                }
            }
            else if (m_skinningMethod == SkinningMethod::DualQuat)
            {
                DualQuat* renderTransforms = renderSkinningData->pBoneQuatsS;

                for (AZ::u32 transformIndex = 0; transformIndex < transformCount; ++transformIndex)
                {
                    renderTransforms[transformIndex] = MCoreMatrixToCryDualQuat(skinningMatrices[transformIndex]);
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

        void ActorRenderNode::SetMaterial(_smart_ptr<IMaterial> pMat)
        {
            AZ_Assert(m_materialPerLOD.size() < 2, "Attempting to override actor's multiple LOD materials with a single material");
            m_materialPerLOD.clear();
            m_materialPerLOD.emplace_back(pMat);
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


        void ActorRenderNode::UpdateDynamicSkin(size_t lodIndex, size_t primitiveIndex)
        {
            ActorAsset::MeshLOD* meshLOD = m_actorAsset.Get()->GetMeshLOD(lodIndex);
            if (!meshLOD)
            {
                return;
            }

            const ActorAsset::Primitive& primitive = meshLOD->m_primitives[primitiveIndex];
            _smart_ptr<IRenderMesh>& renderMesh = m_renderMeshesPerLOD[lodIndex][primitiveIndex];

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

            const EMotionFX::SubMesh*   subMesh = primitive.m_subMesh;
            const EMotionFX::Mesh*      mesh = subMesh->GetParentMesh();
            const AZ::PackedVector3f*   sourcePositions = static_cast<AZ::PackedVector3f*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
            const AZ::PackedVector3f*   sourceNormals   = static_cast<AZ::PackedVector3f*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_NORMALS));  // TODO: this shouldn't use the original data, this is a bug, but left here on purpose to hide an issue.
            const AZ::PackedVector3f*   sourceBitangents = static_cast<AZ::PackedVector3f*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_BITANGENTS));   // Due to time constraints we will fix this later.
            const AZ::Vector4*          sourceTangents  = static_cast<AZ::Vector4*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));

            if (!destTangents)
            {
                destTangents.data = reinterpret_cast<SPipQTangents*>(renderMesh->GetQTangentPtr(destTangents.iStride, FSL_SYSTEM_UPDATE));
            }
            AZ_Assert(static_cast<bool>(destTangents), "Expected a destination tangent buffer");

            const AZ::u32 startVertex = subMesh->GetStartVertex();
            const size_t numSubMeshVertices = subMesh->GetNumVertices();
            for (size_t i = 0; i < numSubMeshVertices; ++i)
            {
                const AZ::u32 vertexIndex = startVertex + i;

                const AZ::PackedVector3f& sourcePosition = sourcePositions[vertexIndex];
                destVertices[i] = Vec3(sourcePosition.GetX(), sourcePosition.GetY(), sourcePosition.GetZ());

                const AZ::PackedVector3f& sourceNormal = sourceNormals[vertexIndex];
                destNormals[i] = Vec3(sourceNormal.GetX(), sourceNormal.GetY(), sourceNormal.GetZ());

                if (sourceTangents)
                {
                    // We only need to update the tangents if they are in the mesh, otherwise they will be 0 or not
                    // be present at the destination
                    const AZ::Vector4& sourceTangent = sourceTangents[vertexIndex];
                    const AZ::Vector3 sourceNormalV3(sourceNormals[vertexIndex]);

                    AZ::Vector3 bitangent;
                    if (sourceBitangents)
                    {
                        bitangent = AZ::Vector3(sourceBitangents[vertexIndex]);
                    }
                    else
                    {
                        bitangent = sourceNormalV3.Cross(sourceTangent.GetAsVector3()) * sourceTangent.GetW();
                    }

                    const SMeshTangents meshTangent(
                        Vec3(sourceTangent.GetX(), sourceTangent.GetY(), sourceTangent.GetZ()),
                        Vec3(bitangent.GetX(), bitangent.GetY(), bitangent.GetZ()),
                        Vec3(sourceNormalV3.GetX(), sourceNormalV3.GetY(), sourceNormalV3.GetZ()));

                    const Quat q = MeshTangentFrameToQTangent(meshTangent);
                    destTangents[i] = SPipQTangents(
                            Vec4sf(
                                PackingSNorm::tPackF2B(q.v.x),
                                PackingSNorm::tPackF2B(q.v.y),
                                PackingSNorm::tPackF2B(q.v.z),
                                PackingSNorm::tPackF2B(q.w)));
                } // if (sourceTangents)
            } // for all vertices

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

        ActorAsset::ActorInstancePtr ActorAsset::CreateInstance(AZ::Entity* entity)
        {
            AZ_Assert(m_emfxActor, "Anim graph asset is not loaded");
            ActorInstancePtr actorInstance = ActorInstancePtr::MakeFromNew(EMotionFX::ActorInstance::Create(m_emfxActor.get(), entity));
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
            EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
            const uint32 numNodes = actor->GetNumNodes();
            const uint32 numLODs = actor->GetNumLODLevels();
            const uint32 maxInfluences = AZ_ARRAY_SIZE(((SMeshBoneMapping_uint16*)nullptr)->boneIds);

            assetData->m_meshLODs.clear();
            assetData->m_meshLODs.reserve(numLODs);

            //
            // Process all LODs from the EMotionFX actor data.
            //
            for (uint32 lodIndex = 0; lodIndex < numLODs; ++lodIndex)
            {
                assetData->m_meshLODs.push_back(ActorAsset::MeshLOD());
                ActorAsset::MeshLOD& lod = assetData->m_meshLODs.back();

                // Get the amount of vertices and indices
                // Get the meshes to process
                bool hasUVs = false;
                bool hasUVs2 = false;
                bool hasTangents = false;
                bool hasBitangents = false;

                // Find the number of submeshes in the full actor.
                // This will be the number of primitives.
                size_t numPrimitives = 0;
                for (uint32 n = 0; n < numNodes; ++n)
                {
                    EMotionFX::Mesh* mesh = actor->GetMesh(lodIndex, n);
                    if (!mesh || mesh->GetIsCollisionMesh())
                    {
                        continue;
                    }

                    numPrimitives += mesh->GetNumSubMeshes();
                }

                lod.m_primitives.resize(numPrimitives);

                bool hasDynamicMeshes = false;

                size_t primitiveIndex = 0;
                for (uint32 n = 0; n < numNodes; ++n)
                {
                    EMotionFX::Mesh* mesh = actor->GetMesh(lodIndex, n);
                    if (!mesh || mesh->GetIsCollisionMesh())
                    {
                        continue;
                    }

                    const EMotionFX::Node* node = skeleton->GetNode(n);
                    const EMotionFX::Mesh::EMeshType meshType = mesh->ClassifyMeshType(lodIndex, actor, node->GetNodeIndex(), false, 4, 255);

                    hasUVs  = (mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0) != nullptr);
                    hasUVs2 = (mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 1) != nullptr);
                    hasTangents = (mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS) != nullptr);

                    const AZ::PackedVector3f*   sourcePositions     = static_cast<AZ::PackedVector3f*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
                    const AZ::PackedVector3f*   sourceNormals       = static_cast<AZ::PackedVector3f*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_NORMALS));
                    const AZ::u32*              sourceOriginalVertex = static_cast<AZ::u32*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS));
                    const AZ::Vector4*          sourceTangents      = static_cast<AZ::Vector4*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));
                    const AZ::PackedVector3f*   sourceBitangents    = static_cast<AZ::PackedVector3f*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_BITANGENTS));
                    const AZ::Vector2*          sourceUVs           = static_cast<AZ::Vector2*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0));
                    const AZ::Vector2*          sourceUVs2          = static_cast<AZ::Vector2*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 1));
                    EMotionFX::SkinningInfoVertexAttributeLayer* sourceSkinningInfo = static_cast<EMotionFX::SkinningInfoVertexAttributeLayer*>(mesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID));

                    // For each sub-mesh within each mesh, we want to create a separate sub-piece.
                    const uint32 numSubMeshes = mesh->GetNumSubMeshes();
                    for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                    {
                        EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);

                        AZ_Assert(primitiveIndex < numPrimitives, "Unexpected primitive index");

                        ActorAsset::Primitive& primitive = lod.m_primitives[primitiveIndex++];
                        primitive.m_mesh = new CMesh();
                        primitive.m_isDynamic = (meshType == EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED);
                        primitive.m_subMesh = subMesh;

                        if (primitive.m_isDynamic)
                        {
                            hasDynamicMeshes = true;
                        }

                        // Destination initialization. We are going to put all meshes and submeshes for one lod
                        // into one destination mesh
                        primitive.m_vertexBoneMappings.resize(subMesh->GetNumVertices());
                        primitive.m_mesh->SetIndexCount(subMesh->GetNumIndices());
                        primitive.m_mesh->SetVertexCount(subMesh->GetNumVertices());

                        // Positions and normals are reallocated by the SetVertexCount
                        if (hasTangents)
                        {
                            primitive.m_mesh->ReallocStream(CMesh::TANGENTS, 0, subMesh->GetNumVertices());
                        }
                        if (hasUVs)
                        {
                            primitive.m_mesh->ReallocStream(CMesh::TEXCOORDS, 0, subMesh->GetNumVertices());
                        }
                        if (hasUVs2)
                        {
                            primitive.m_mesh->ReallocStream(CMesh::TEXCOORDS, 1, subMesh->GetNumVertices());
                        }

                        primitive.m_mesh->m_pBoneMapping = primitive.m_vertexBoneMappings.data();

                        // Pointers to the destination
                        vtx_idx* targetIndices = primitive.m_mesh->GetStreamPtr<vtx_idx>(CMesh::INDICES);
                        Vec3* destVertices = primitive.m_mesh->GetStreamPtr<Vec3>(CMesh::POSITIONS);
                        Vec3* destNormals = primitive.m_mesh->GetStreamPtr<Vec3>(CMesh::NORMALS);
                        SMeshTexCoord* destTexCoords = primitive.m_mesh->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);
                        SMeshTexCoord* destTexCoords2 = primitive.m_mesh->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS, 1);
                        SMeshTangents* destTangents = primitive.m_mesh->GetStreamPtr<SMeshTangents>(CMesh::TANGENTS);
                        SMeshBoneMapping_uint16* destBoneMapping = primitive.m_vertexBoneMappings.data();

                        primitive.m_mesh->m_subsets.push_back();
                        SMeshSubset& subset     = primitive.m_mesh->m_subsets.back();
                        subset.nFirstIndexId    = 0;
                        subset.nNumIndices      = subMesh->GetNumIndices();
                        subset.nFirstVertId     = 0;
                        subset.nNumVerts        = subMesh->GetNumVertices();
                        subset.nMatID           = subMesh->GetMaterial();
                        subset.fTexelDensity    = 0.0f;
                        subset.nPhysicalizeType = -1;

                        const uint32* subMeshIndices = subMesh->GetIndices();
                        const uint32 numSubMeshIndices = subMesh->GetNumIndices();
                        const uint32 subMeshStartVertex = subMesh->GetStartVertex();
                        for (uint32 index = 0; index < numSubMeshIndices; ++index)
                        {
                            targetIndices[index] = subMeshIndices[index] - subMeshStartVertex;
                        }

                        // Process vertices
                        const uint32 numSubMeshVertices = subMesh->GetNumVertices();
                        const AZ::PackedVector3f* subMeshPositions = &sourcePositions[subMesh->GetStartVertex()];
                        for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                        {
                            const AZ::PackedVector3f& sourcePosition = subMeshPositions[vertexIndex];
                            destVertices->x = sourcePosition.GetX();
                            destVertices->y = sourcePosition.GetY();
                            destVertices->z = sourcePosition.GetZ();
                            ++destVertices;
                        }

                        // Process normals
                        const AZ::PackedVector3f* subMeshNormals = &sourceNormals[subMesh->GetStartVertex()];
                        for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                        {
                            const AZ::PackedVector3f& sourceNormal = subMeshNormals[vertexIndex];
                            destNormals->x = sourceNormal.GetX();
                            destNormals->y = sourceNormal.GetY();
                            destNormals->z = sourceNormal.GetZ();
                            ++destNormals;
                        }

                        // Process UVs (TextCoords)
                        // First UV set.
                        if (hasUVs)
                        {
                            if (sourceUVs)
                            {
                                const AZ::Vector2* subMeshUVs = &sourceUVs[subMeshStartVertex];
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    const AZ::Vector2& uv = subMeshUVs[vertexIndex];
                                    *destTexCoords = SMeshTexCoord(uv.GetX(), uv.GetY());
                                    ++destTexCoords;
                                }
                            }
                            else
                            {
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    *destTexCoords = SMeshTexCoord(0.0f, 0.0f);
                                    ++destTexCoords;
                                }
                            }
                        }

                        // Second UV set.
                        if (hasUVs2)
                        {
                            if (sourceUVs2)
                            {
                                const AZ::Vector2* subMeshUVs = &sourceUVs2[subMeshStartVertex];
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    const AZ::Vector2& uv2 = subMeshUVs[vertexIndex];
                                    *destTexCoords2 = SMeshTexCoord(uv2.GetX(), uv2.GetY());
                                    ++destTexCoords2;
                                }
                            }
                            else
                            {
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    *destTexCoords2 = SMeshTexCoord(0.0f, 0.0f);
                                    ++destTexCoords2;
                                }
                            }
                        }

                        // Process tangents
                        if (hasTangents)
                        {
                            if (sourceTangents)
                            {
                                const AZ::Vector4* subMeshTangents = &sourceTangents[subMeshStartVertex];
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    const AZ::Vector4& sourceTangent = subMeshTangents[vertexIndex];
                                    const AZ::Vector3 sourceNormal(subMeshNormals[vertexIndex]);

                                    AZ::Vector3 bitangent;
                                    if (sourceBitangents)
                                    {
                                        bitangent = AZ::Vector3(sourceBitangents[vertexIndex + subMeshStartVertex]);
                                    }
                                    else
                                    {
                                        bitangent = sourceNormal.Cross(sourceTangent.GetAsVector3()) * sourceTangent.GetW();
                                    }

                                    *destTangents = SMeshTangents(
                                            Vec3(sourceTangent.GetX(), sourceTangent.GetY(), sourceTangent.GetZ()),
                                            Vec3(bitangent.GetX(), bitangent.GetY(), bitangent.GetZ()),
                                            Vec3(sourceNormal.GetX(), sourceNormal.GetY(), sourceNormal.GetZ()));
                                    ++destTangents;
                                }
                            }
                            else
                            {
                                for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                                {
                                    *destTangents = SMeshTangents();
                                    ++destTangents;
                                }
                            }
                        }

                        // Process AABB
                        AABB localAabb(AABB::RESET);
                        for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                        {
                            const AZ::PackedVector3f& sourcePosition = subMeshPositions[vertexIndex];
                            localAabb.Add(Vec3(sourcePosition.GetX(), sourcePosition.GetY(), sourcePosition.GetZ()));
                        }
                        subset.fRadius = localAabb.GetRadius();
                        subset.vCenter = localAabb.GetCenter();
                        primitive.m_mesh->m_bbox.Add(localAabb.min);
                        primitive.m_mesh->m_bbox.Add(localAabb.max);

                        // Process Skinning info
                        if (sourceSkinningInfo)
                        {
                            for (uint32 vertexIndex = 0; vertexIndex < numSubMeshVertices; ++vertexIndex)
                            {
                                const AZ::u32 originalVertex = sourceOriginalVertex[vertexIndex + subMesh->GetStartVertex()];
                                const AZ::u32 influenceCount = AZ::GetMin<AZ::u32>(maxInfluences, sourceSkinningInfo->GetNumInfluences(originalVertex));
                                AZ::u32 influenceIndex = 0;
                                int weightError = 255;

                                for (; influenceIndex < influenceCount; ++influenceIndex)
                                {
                                    EMotionFX::SkinInfluence* influence = sourceSkinningInfo->GetInfluence(originalVertex, influenceIndex);
                                    destBoneMapping->boneIds[influenceIndex] = influence->GetNodeNr();
                                    destBoneMapping->weights[influenceIndex] = static_cast<AZ::u8>(AZ::GetClamp<float>(influence->GetWeight() * 255.0f, 0.0f, 255.0f));
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

                        // Legacy index buffer fix.
                        primitive.m_mesh->m_subsets[0].FixRanges(primitive.m_mesh->m_pIndices);

                        // Convert tangent frame from matrix to quaternion based.
                        // Without this, materials do NOT render correctly on skinned characters.
                        if (primitive.m_mesh->m_pTangents && !primitive.m_mesh->m_pQTangents)
                        {
                            primitive.m_mesh->m_pQTangents = (SMeshQTangents*)primitive.m_mesh->m_pTangents;
                            MeshTangentsFrameToQTangents(
                                primitive.m_mesh->m_pTangents, sizeof(primitive.m_mesh->m_pTangents[0]), primitive.m_mesh->GetVertexCount(),
                                primitive.m_mesh->m_pQTangents, sizeof(primitive.m_mesh->m_pQTangents[0]));
                        }
                    }   // for all submeshes
                } // for all meshes

                lod.m_hasDynamicMeshes = hasDynamicMeshes;
            } // for all lods

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

            if (!gEnv)
            {
                return;
            }

            ActorAsset* assetData = asset.Get();
            AZ_Assert(assetData, "Invalid asset data");

            AZStd::string assetPath;
            EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());

            EMotionFX::Actor* actor = assetData->m_emfxActor.get();
            const AZ::u32 numLODs = actor->GetNumLODLevels();

            // Process all LODs from the EMotionFX actor data.
            for (AZ::u32 lodIndex = 0; lodIndex < numLODs; ++lodIndex)
            {
                ActorAsset::MeshLOD& lod = assetData->m_meshLODs[lodIndex];

                for (ActorAsset::Primitive& primitive : lod.m_primitives)
                {
                    // Create and initialize render mesh.
                    primitive.m_renderMesh = gEnv->pRenderer->CreateRenderMesh("EMotion FX Actor", assetPath.c_str(), nullptr, eRMT_Dynamic);

                    const AZ::u32 renderMeshFlags = FSM_ENABLE_NORMALSTREAM | FSM_VERTEX_VELOCITY;
                    primitive.m_renderMesh->SetMesh(*primitive.m_mesh, 0, renderMeshFlags, false);

                    // Free temporary load objects & buffers.
                    primitive.m_vertexBoneMappings.resize(0);
                }

                // It's now safe to use this LOD.
                lod.m_isReady = true;
            }
        }


        void ActorRenderNode::BuildRenderMeshPerLOD()
        {
            // Populate m_renderMeshesPerLOD. If the mesh is not dynamic, we reuse the render mesh from the actor. If the
            // mesh is dynamic, we create a copy of the actor's render mesh since this actor instance will be modifying it.
            ActorAsset* data = m_actorAsset.Get();
            if (data)
            {
                AZStd::string assetPath;
                EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, m_actorAsset.GetId());

                const size_t lodCount = data->GetNumLODs();
                m_renderMeshesPerLOD.resize(lodCount);
                for (size_t i = 0; i < lodCount; ++i)
                {
                    ActorAsset::MeshLOD* meshLOD = data->GetMeshLOD(i);
                    if (meshLOD)
                    {
                        const size_t numPrims = meshLOD->m_primitives.size();
                        m_renderMeshesPerLOD[i].resize(numPrims);
                        for (size_t primIndex = 0; primIndex < numPrims; ++primIndex)
                        {
                            ActorAsset::Primitive& primitive = meshLOD->m_primitives[primIndex];
                            if (!primitive.m_isDynamic)
                            {
                                // Reuse the same render mesh
                                m_renderMeshesPerLOD[i][primIndex] = primitive.m_renderMesh;
                            }
                            else
                            {
                                // Create a copy since each actor instance can be deforming differently and we need to send
                                // different meshes to cry to render
                                _smart_ptr<IRenderMesh>& clonedMesh = m_renderMeshesPerLOD[i][primIndex];
                                clonedMesh = gEnv->pRenderer->CreateRenderMesh("EMotion FX Actor", assetPath.c_str(), nullptr, eRMT_Dynamic);
                                const AZ::u32 renderMeshFlags = FSM_ENABLE_NORMALSTREAM | FSM_VERTEX_VELOCITY;
                                clonedMesh->SetMesh(*primitive.m_mesh, 0, renderMeshFlags, false);
                            }
                        }
                    }
                }
            }
        }


        bool ActorAssetHandler::OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            ActorAsset* assetData = asset.GetAs<ActorAsset>();
            assetData->m_emfxActor = EMotionFXPtr<EMotionFX::Actor>::MakeFromNew(EMotionFX::GetImporter().LoadActor(
                        assetData->m_emfxNativeData.data(),
                        assetData->m_emfxNativeData.size(),
                        nullptr, ""));

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

        const char* ActorAssetHandler::GetBrowserIcon() const
        {
            return "Editor/Images/AssetBrowser/Actor_16.png";
        }
    } //namespace Integration
} // namespace EMotionFX