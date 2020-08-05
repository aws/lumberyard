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

#include <NvCloth_precompiled.h>

#include <Cry_Geo.h> // Needed for AABB used in IIndexedMesh.h included from QTangent.h
#include <QTangent.h>
#include <VertexFormats.h>

#include <IRenderer.h> // Needed for IRenderMesh.h
#include <IRenderMesh.h>
#include <stridedptr.h>

#include <System/Cloth.h>

#include <Components/ClothComponentMesh/ActorClothColliders.h>
#include <Components/ClothComponentMesh/ActorClothSkinning.h>
#include <Components/ClothComponentMesh/ClothDebugDisplay.h>
#include <Components/ClothComponentMesh/ClothComponentMesh.h>

namespace NvCloth
{
    ClothComponentMesh::ClothComponentMesh(AZ::EntityId entityId, const ClothConfiguration& config)
    {
        Setup(entityId, config);
    }

    ClothComponentMesh::~ClothComponentMesh()
    {
        TearDown();
    }

    void ClothComponentMesh::Setup(AZ::EntityId entityId, const ClothConfiguration& config)
    {
        TearDown();

        AZStd::unique_ptr<AssetHelper> assetHelper = AssetHelper::CreateAssetHelper(entityId);
        if (assetHelper)
        {
            m_entityId = entityId;
            m_config = config;

            // Obtain cloth information
            AZStd::vector<SimParticleType> meshInitialParticles;
            AZStd::vector<SimIndexType> meshInitialIndices;
            bool clothInfoObtained = assetHelper->ObtainClothMeshNodeInfo(m_config.m_meshNode,
                m_meshNodeInfo, meshInitialParticles, meshInitialIndices, m_meshInitialUVs);

            if (clothInfoObtained)
            {
                // Create cloth instance
                m_clothSimulation = AZStd::make_unique<Cloth>(m_config,
                    meshInitialParticles, meshInitialIndices);

                // Set initial Position and Rotation
                AZ::Transform transform = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(transform, m_entityId, &AZ::TransformInterface::GetWorldTM);
                m_clothSimulation->SetTransform(transform);
                m_clothSimulation->ClearInertia();

                // Initialize render data
                m_renderDataBufferIndex = 0;
                m_renderDataBuffer[0].m_particles = m_clothSimulation->GetInitialParticles();
                RecalculateTangentSpaces();
                // Copy the first initialized element to the rest of the buffer
                for (int i = 1; i < RenderDataBufferSize; ++i)
                {
                    m_renderDataBuffer[i] = m_renderDataBuffer[0];
                }

                // It will return a valid instance if it's an actor with cloth colliders in it.
                m_actorClothColliders = ActorClothColliders::Create(m_entityId);

                // It will return a valid instance if it's an actor with skinning data.
                m_actorClothSkinning = ActorClothSkinning::Create(m_entityId, m_clothSimulation->GetInitialParticles(), m_config.m_meshNode);

#ifndef RELEASE
                m_clothDebugDisplay = AZStd::make_unique<ClothDebugDisplay>(this);
#endif

                // Notify the render component to start requesting mesh modifications
                for (const auto& subMesh : m_meshNodeInfo.m_subMeshes)
                {
                    LmbrCentral::MeshModificationRequestBus::Event(m_entityId,
                        &LmbrCentral::MeshModificationRequestBus::Events::RequireSendingRenderMeshForModification,
                        m_meshNodeInfo.m_lodLevel, subMesh.m_primitiveIndex);
                }

                AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
                SystemNotificationsBus::Handler::BusConnect();
                LmbrCentral::MeshModificationNotificationBus::Handler::BusConnect(m_entityId);
            }
            else
            {
                ClearData();
            }
        }
    }

    void ClothComponentMesh::TearDown()
    {
        if (m_clothSimulation)
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            SystemNotificationsBus::Handler::BusDisconnect();
            LmbrCentral::MeshModificationNotificationBus::Handler::BusDisconnect();

            // Notify the render component to stop requesting mesh modifications
            for (const auto& subMesh : m_meshNodeInfo.m_subMeshes)
            {
                LmbrCentral::MeshModificationRequestBus::Event(m_entityId,
                    &LmbrCentral::MeshModificationRequestBus::Events::StopSendingRenderMeshForModification, m_meshNodeInfo.m_lodLevel, subMesh.m_primitiveIndex);
            }
        }
        ClearData();
    }

    void ClothComponentMesh::OnPreUpdateClothSimulation(
        [[maybe_unused]] float deltaTime)
    {
        // Next buffer index of the render data
        m_renderDataBufferIndex = (m_renderDataBufferIndex + 1) % RenderDataBufferSize;

        // Update Cloth Simulation particles
        if (!IsClothFullyAnimated())
        {
            UpdateSimulationCollisions();

            UpdateSimulationStaticParticles();
        }
    }

    void ClothComponentMesh::OnPostUpdateClothSimulation(
        [[maybe_unused]] float deltaTime)
    {
        // Gather results of Cloth Simulation particles
        if (!IsClothFullyAnimated())
        {
            auto& renderParticles = GetRenderParticles();
            renderParticles = m_clothSimulation->GetParticles();
        }

        // Update Cloth particles for rendering
        {
            BlendSkinningAnimation();

            RecalculateTangentSpaces();
        }
    }

    void ClothComponentMesh::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
        AZ_UNUSED(local);
        m_clothSimulation->SetTransform(world);
    }

    void ClothComponentMesh::ModifyMesh(size_t lodIndex, size_t primitiveIndex, IRenderMesh* renderMesh)
    {
        if (!renderMesh ||
            m_meshNodeInfo.m_lodLevel != static_cast<int>(lodIndex))
        {
            return;
        }

        // Is it one of the meshes we have to modify?
        auto subMeshIt = AZStd::find_if(m_meshNodeInfo.m_subMeshes.cbegin(), m_meshNodeInfo.m_subMeshes.cend(), 
            [primitiveIndex](const MeshNodeInfo::SubMesh& subMesh)
            {
                return subMesh.m_primitiveIndex == static_cast<int>(primitiveIndex);
            });
        if (subMeshIt == m_meshNodeInfo.m_subMeshes.cend())
        {
            return;
        }

        // Previous buffer index of the render data
        const AZ::u32 previousBufferIndex = (m_renderDataBufferIndex + RenderDataBufferSize - 1) % RenderDataBufferSize;

        // Workaround to sync debug drawing with cloth rendering as
        // the Entity Debug Display Bus renders on the next frame.
        const bool isDebugDrawEnabled = m_clothDebugDisplay && m_clothDebugDisplay->IsDebugDrawEnabled();
        const RenderData& renderData = (isDebugDrawEnabled)
            ? m_renderDataBuffer[previousBufferIndex]
            : m_renderDataBuffer[m_renderDataBufferIndex];

        const auto& renderParticles = renderData.m_particles;
        const auto& renderTangentSpaces = renderData.m_tangentSpaces;
        bool validTangentSpaces = (renderTangentSpaces.GetBaseCount() == renderParticles.size());

        int numVertices = subMeshIt->m_numVertices;
        int firstVertex = subMeshIt->m_verticesFirstIndex;
        if (renderMesh->GetNumVerts() != numVertices)
        {
            AZ_Error("ClothComponent", false, 
                "Render mesh to be modified doesn't have the same number of vertices (%d) as the cloth's submesh (%d).",
                renderMesh->GetNumVerts(),
                numVertices)
            return;
        }
        AZ_Assert(firstVertex >= 0, "Invalid first vertex index %d", firstVertex);
        AZ_Assert((firstVertex + numVertices) <= renderParticles.size(),
            "Submesh number of vertices (%d) reaches outside the particles (%d)", (firstVertex + numVertices), renderParticles.size());

        // Modify Render Mesh
        {
            IRenderMesh::ThreadAccessLock lockRenderMesh(renderMesh);

            strided_pointer<Vec3> destVertices;
            strided_pointer<Vec3f16> destNormals;
            strided_pointer<SPipQTangents> destQTangents;
            strided_pointer<SPipTangents> destTangents;
            strided_pointer<SVF_W4B_I4S> destSkinning;

            destVertices.data = reinterpret_cast<Vec3*>(renderMesh->GetPosPtr(destVertices.iStride, FSL_SYSTEM_UPDATE));
            if (validTangentSpaces)
            {
                destNormals.data = reinterpret_cast<Vec3f16*>(renderMesh->GetNormPtr(destNormals.iStride, FSL_SYSTEM_UPDATE));
                destQTangents.data = reinterpret_cast<SPipQTangents*>(renderMesh->GetQTangentPtr(destQTangents.iStride, FSL_SYSTEM_UPDATE));
                if (!destQTangents.data)
                {
                    destTangents.data = reinterpret_cast<SPipTangents*>(renderMesh->GetTangentPtr(destTangents.iStride, FSL_SYSTEM_UPDATE));
                }
            }
            destSkinning.data = reinterpret_cast<SVF_W4B_I4S*>(renderMesh->GetHWSkinPtr(destSkinning.iStride, FSL_SYSTEM_UPDATE));

            // Set based on current cloth particle positions
            for (int index = 0; index < numVertices; ++index)
            {
                const int renderVertexIndex = firstVertex + index;

                const auto& renderParticle = renderParticles[renderVertexIndex];

                destVertices[index].x = renderParticle.x;
                destVertices[index].y = renderParticle.y;
                destVertices[index].z = renderParticle.z;

                if (destNormals.data)
                {
                    Vec3 renderNormal;
                    Vec3 renderTangent;
                    Vec3 renderBitangent;
                    renderTangentSpaces.GetBase(renderVertexIndex, renderTangent, renderBitangent, renderNormal);

                    destNormals[index] = renderNormal; // Converts Vec3 to Vec3f16

                    if (destQTangents.data)
                    {
                        const SMeshTangents tangentSpace(renderTangent, renderBitangent, renderNormal);
                        const Quat tangentSpaceQuat = MeshTangentFrameToQTangent(tangentSpace);

                        destQTangents[index] = SPipQTangents(
                            Vec4sf(
                                PackingSNorm::tPackF2B(tangentSpaceQuat.v.x),
                                PackingSNorm::tPackF2B(tangentSpaceQuat.v.y),
                                PackingSNorm::tPackF2B(tangentSpaceQuat.v.z),
                                PackingSNorm::tPackF2B(tangentSpaceQuat.w)));
                    }

                    if (destTangents.data)
                    {
                        const SMeshTangents tangentSpace(renderTangent, renderBitangent, renderNormal);
                        destTangents[index] = SPipTangents(renderTangent, renderBitangent, tangentSpace.GetR());
                    }
                }

                if (destSkinning.data)
                {
                    // Clear skinning weights so transforms are not applied later.
                    destSkinning[index].weights.dcolor = 0;
                }
            }

            renderMesh->UnlockStream(VSF_GENERAL);
#if ENABLE_NORMALSTREAM_SUPPORT
            if (destNormals.data)
            {
                renderMesh->UnlockStream(VSF_NORMALS);
            }
#endif
            if (destQTangents.data)
            {
                renderMesh->UnlockStream(VSF_QTANGENTS);
            }
            if (destTangents.data)
            {
                renderMesh->UnlockStream(VSF_TANGENTS);
            }
            if (destSkinning.data)
            {
                renderMesh->UnlockStream(VSF_HWSKIN_INFO);
            }
        }
    }

    AZStd::vector<SimParticleType>& ClothComponentMesh::GetRenderParticles()
    {
        return const_cast<AZStd::vector<SimParticleType>&>(
            static_cast<const ClothComponentMesh&>(*this).GetRenderParticles());
    }

    const AZStd::vector<SimParticleType>& ClothComponentMesh::GetRenderParticles() const
    {
        return m_renderDataBuffer[m_renderDataBufferIndex].m_particles;
    }

    TangentSpaceCalculation& ClothComponentMesh::GetRenderTangentSpaces()
    {
        return const_cast<TangentSpaceCalculation&>(
            static_cast<const ClothComponentMesh&>(*this).GetRenderTangentSpaces());
    }

    const TangentSpaceCalculation& ClothComponentMesh::GetRenderTangentSpaces() const
    {
        return m_renderDataBuffer[m_renderDataBufferIndex].m_tangentSpaces;
    }

    void ClothComponentMesh::ClearData()
    {
        m_entityId.SetInvalid();
        m_config = {};
        m_clothSimulation.reset();
        m_meshInitialUVs.clear();
        m_renderDataBufferIndex = 0;
        m_renderDataBuffer = {};
        m_meshNodeInfo = {};
        m_actorClothColliders.reset();
        m_actorClothSkinning.reset();
        m_clothDebugDisplay.reset();
    }

    void ClothComponentMesh::UpdateSimulationCollisions()
    {
        if (m_actorClothColliders)
        {
            m_actorClothColliders->Update();

            const auto& spheres = m_actorClothColliders->GetSpheres();
            m_clothSimulation->SetSphereColliders(spheres);

            const auto& capsuleIndices = m_actorClothColliders->GetCapsuleIndices();
            m_clothSimulation->SetCapsuleColliders(capsuleIndices);
        }
    }

    void ClothComponentMesh::UpdateSimulationStaticParticles()
    {
        if (m_actorClothSkinning)
        {
            AZStd::vector<SimParticleType> particles = m_clothSimulation->GetParticles();

            m_actorClothSkinning->UpdateStaticParticles(m_clothSimulation->GetInitialParticles(), particles);

            m_clothSimulation->SetParticles(AZStd::move(particles));
        }
    }

    void ClothComponentMesh::BlendSkinningAnimation()
    {
        if (IsClothFullySimulated())
        {
            return;
        }

        if (m_actorClothSkinning)
        {
            auto& renderParticles = GetRenderParticles();

            if (IsClothFullyAnimated())
            {
                m_actorClothSkinning->UpdateParticles(m_clothSimulation->GetInitialParticles(), renderParticles);
            }
            else
            {
                m_actorClothSkinning->UpdateDynamicParticles(m_config.m_animationBlendFactor, m_clothSimulation->GetInitialParticles(), renderParticles);
            }
        }
    }

    void ClothComponentMesh::RecalculateTangentSpaces()
    {
        const auto& renderParticles = GetRenderParticles();
        auto& renderTangentSpaces = GetRenderTangentSpaces();
        renderTangentSpaces.Calculate(renderParticles, m_clothSimulation->GetInitialIndices(), m_meshInitialUVs);
    }

    bool ClothComponentMesh::IsClothFullySimulated() const
    {
        return !m_actorClothSkinning || m_config.m_animationBlendFactor <= 0.0f;
    }

    bool ClothComponentMesh::IsClothFullyAnimated() const
    {
        return m_actorClothSkinning && m_config.m_animationBlendFactor >= 1.0f;
    }
} // namespace NvCloth
