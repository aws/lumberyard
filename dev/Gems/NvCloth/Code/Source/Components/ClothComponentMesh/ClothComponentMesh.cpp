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

#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/IConsole.h>

#include <Cry_Geo.h> // Needed for AABB used in IIndexedMesh.h included from QTangent.h
#include <QTangent.h>
#include <MathConversion.h>

#include <IRenderer.h> // Needed for IRenderMesh.h
#include <IRenderMesh.h>
#include <stridedptr.h>

#include <NvCloth/IClothSystem.h>
#include <NvCloth/IFabricCooker.h>
#include <NvCloth/IClothConfigurator.h>
#include <NvCloth/ITangentSpaceHelper.h>

#include <Components/ClothComponentMesh/ActorClothColliders.h>
#include <Components/ClothComponentMesh/ActorClothSkinning.h>
#include <Components/ClothComponentMesh/ClothConstraints.h>
#include <Components/ClothComponentMesh/ClothDebugDisplay.h>
#include <Components/ClothComponentMesh/ClothComponentMesh.h>

#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/WindBus.h>

namespace NvCloth
{
    AZ_CVAR(float, cloth_DistanceToTeleport, 0.5f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "The amount of meters the entity has to move in a frame to consider it a teleport for cloth.");

    ClothComponentMesh::ClothComponentMesh(AZ::EntityId entityId, const ClothConfiguration& config)
        : m_preSimulationEventHandler(
            [this](ClothId clothId, float deltaTime)
            {
                this->OnPreSimulation(clothId, deltaTime);
            })
        , m_postSimulationEventHandler(
            [this](ClothId clothId, float deltaTime, const AZStd::vector<SimParticleFormat>& updatedParticles)
            {
                this->OnPostSimulation(clothId, deltaTime, updatedParticles);
            })
    {
        Setup(entityId, config);
    }

    ClothComponentMesh::~ClothComponentMesh()
    {
        TearDown();
    }

    void ClothComponentMesh::UpdateConfiguration(AZ::EntityId entityId, const ClothConfiguration& config)
    {
        if (m_entityId != entityId ||
            m_config.m_meshNode != config.m_meshNode ||
            m_config.m_removeStaticTriangles != config.m_removeStaticTriangles)
        {
            Setup(entityId, config);
        }
        else if (m_cloth)
        {
            m_config = config;
            ApplyConfigurationToCloth();

            // Update the cloth constraints parameters
            m_clothConstraints->SetMotionConstraintMaxDistance(m_config.m_motionConstraintsMaxDistance);
            m_clothConstraints->SetBackstopMaxRadius(m_config.m_backstopRadius);
            m_clothConstraints->SetBackstopMaxOffsets(m_config.m_backstopBackOffset, m_config.m_backstopFrontOffset);
            UpdateSimulationConstraints();

            // Subscribe to WindNotificationsBus only if custom wind velocity flag is not set
            if (m_config.IsUsingWindBus())
            {
                Physics::WindNotificationsBus::Handler::BusConnect();
            }
            else
            {
                Physics::WindNotificationsBus::Handler::BusDisconnect();
            }
        }
    }

    void ClothComponentMesh::Setup(AZ::EntityId entityId, const ClothConfiguration& config)
    {
        TearDown();

        m_entityId = entityId;
        m_config = config;

        if (!CreateCloth())
        {
            TearDown();
            return;
        }

        // Initialize render data
        m_renderDataBufferIndex = 0;
        UpdateRenderData(m_cloth->GetParticles());
        // Copy the first initialized element to the rest of the buffer
        for (AZ::u32 i = 1; i < RenderDataBufferSize; ++i)
        {
            m_renderDataBuffer[i] = m_renderDataBuffer[0];
        }
        CopyInternalRenderData();

        // It will return a valid instance if it's an actor with cloth colliders in it.
        m_actorClothColliders = ActorClothColliders::Create(m_entityId);

        // It will return a valid instance if it's an actor with skinning data.
        m_actorClothSkinning = ActorClothSkinning::Create(m_entityId, m_config.m_meshNode, m_cloth->GetParticles().size(), m_meshRemappedVertices);
        m_numberOfClothSkinningUpdates = 0;

        m_clothConstraints = ClothConstraints::Create(
            m_meshClothInfo.m_motionConstraints,
            m_config.m_motionConstraintsMaxDistance,
            m_meshClothInfo.m_backstopData,
            m_config.m_backstopRadius,
            m_config.m_backstopBackOffset,
            m_config.m_backstopFrontOffset,
            m_cloth->GetParticles(),
            m_cloth->GetInitialIndices(),
            m_meshRemappedVertices);
        AZ_Assert(m_clothConstraints, "Failed to create cloth constraints");
        UpdateSimulationConstraints();

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
        m_cloth->ConnectPreSimulationEventHandler(m_preSimulationEventHandler);
        m_cloth->ConnectPostSimulationEventHandler(m_postSimulationEventHandler);
        LmbrCentral::MeshModificationNotificationBus::Handler::BusConnect(m_entityId);

        if (m_config.IsUsingWindBus())
        {
            Physics::WindNotificationsBus::Handler::BusConnect();
        }
    }

    void ClothComponentMesh::TearDown()
    {
        if (m_cloth)
        {
            Physics::WindNotificationsBus::Handler::BusDisconnect();
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            m_preSimulationEventHandler.Disconnect();
            m_postSimulationEventHandler.Disconnect();
            LmbrCentral::MeshModificationNotificationBus::Handler::BusDisconnect();

            // Notify the render component to stop requesting mesh modifications
            for (const auto& subMesh : m_meshNodeInfo.m_subMeshes)
            {
                LmbrCentral::MeshModificationRequestBus::Event(m_entityId,
                    &LmbrCentral::MeshModificationRequestBus::Events::StopSendingRenderMeshForModification, m_meshNodeInfo.m_lodLevel, subMesh.m_primitiveIndex);
            }

            AZ::Interface<IClothSystem>::Get()->RemoveCloth(m_cloth);
            AZ::Interface<IClothSystem>::Get()->DestroyCloth(m_cloth);
        }
        m_entityId.SetInvalid();
        m_renderDataBuffer = {};
        m_internalRenderData = {};
        m_meshRemappedVertices.clear();
        m_meshNodeInfo = {};
        m_meshClothInfo = {};
        m_actorClothColliders.reset();
        m_actorClothSkinning.reset();
        m_clothConstraints.reset();
        m_motionConstraints.clear();
        m_separationConstraints.clear();
        m_clothDebugDisplay.reset();
    }

    void ClothComponentMesh::OnPreSimulation(
        [[maybe_unused]] ClothId clothId,
        [[maybe_unused]] float deltaTime)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        UpdateSimulationCollisions();

        if (m_actorClothSkinning)
        {
            UpdateSimulationSkinning();

            UpdateSimulationConstraints();
        }
    }

    void ClothComponentMesh::OnPostSimulation(
        [[maybe_unused]] ClothId clothId,
        [[maybe_unused]] float deltaTime,
        const AZStd::vector<SimParticleFormat>& updatedParticles)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        // Next buffer index of the render data
        m_renderDataBufferIndex = (m_renderDataBufferIndex + 1) % RenderDataBufferSize;

        UpdateRenderData(updatedParticles);

        CopyInternalRenderData();
    }

    void ClothComponentMesh::ModifyMesh(size_t lodIndex, size_t primitiveIndex, IRenderMesh* renderMesh)
    {
        if (!renderMesh ||
            m_meshNodeInfo.m_lodLevel != static_cast<int>(lodIndex))
        {
            return;
        }

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

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

        const auto& internalRenderParticles = m_internalRenderData.m_particles;
        const auto& internalRenderNormals = m_internalRenderData.m_normals;
        const auto& internalRenderQuatTangents = m_internalRenderData.m_quatTangents;
        const auto& internalRenderTangents = m_internalRenderData.m_tangents;

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
        AZ_Assert((firstVertex + numVertices) <= static_cast<int>(internalRenderParticles.size()),
            "Submesh number of vertices (%d) reaches outside the particles (%zu)", (firstVertex + numVertices), internalRenderParticles.size());

        // Modify Render Mesh
        {
            IRenderMesh::ThreadAccessLock lockRenderMesh(renderMesh);

            strided_pointer<Vec3> destVertices;
            strided_pointer<Vec3f16> destNormals;
            strided_pointer<SPipQTangents> destQTangents;
            strided_pointer<SPipTangents> destTangents;
            strided_pointer<SVF_W4B_I4S> destSkinning;

            destVertices.data = reinterpret_cast<Vec3*>(renderMesh->GetPosPtr(destVertices.iStride, FSL_SYSTEM_UPDATE));
            destNormals.data = reinterpret_cast<Vec3f16*>(renderMesh->GetNormPtr(destNormals.iStride, FSL_SYSTEM_UPDATE));
            if (destNormals.data)
            {
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

                if (m_meshRemappedVertices[renderVertexIndex] < 0)
                {
                    // Removed Particle
                    continue;
                }

                destVertices[index] = internalRenderParticles[renderVertexIndex];

                if (destNormals.data)
                {
                    destNormals[index] = internalRenderNormals[renderVertexIndex];

                    if (destQTangents.data)
                    {
                        destQTangents[index] = internalRenderQuatTangents[renderVertexIndex];
                    }
                    else if (destTangents.data)
                    {
                        destTangents[index] = internalRenderTangents[renderVertexIndex];
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

    void ClothComponentMesh::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        // At the moment there is no way to distinguish "move" from "teleport".
        // As a workaround we will consider a teleport if the position has changed considerably.
        bool teleport = (m_worldPosition.GetDistance(world.GetTranslation()) >= cloth_DistanceToTeleport);

        if (teleport)
        {
            TeleportCloth(world);
        }
        else
        {
            MoveCloth(world);
        }
    }

    void ClothComponentMesh::OnGlobalWindChanged()
    {
        m_cloth->GetClothConfigurator()->SetWindVelocity(GetWindBusVelocity());
    }

    void ClothComponentMesh::OnWindChanged([[maybe_unused]] const AZ::Aabb& aabb)
    {
        OnGlobalWindChanged();
    }

    ClothComponentMesh::RenderData& ClothComponentMesh::GetRenderData()
    {
        return const_cast<RenderData&>(
            static_cast<const ClothComponentMesh&>(*this).GetRenderData());
    }

    const ClothComponentMesh::RenderData& ClothComponentMesh::GetRenderData() const
    {
        return m_renderDataBuffer[m_renderDataBufferIndex];
    }

    void ClothComponentMesh::UpdateSimulationCollisions()
    {
        if (m_actorClothColliders)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

            m_actorClothColliders->Update();

            const auto& spheres = m_actorClothColliders->GetSpheres();
            m_cloth->GetClothConfigurator()->SetSphereColliders(spheres);

            const auto& capsuleIndices = m_actorClothColliders->GetCapsuleIndices();
            m_cloth->GetClothConfigurator()->SetCapsuleColliders(capsuleIndices);
        }
    }

    void ClothComponentMesh::UpdateSimulationSkinning()
    {
        if (m_actorClothSkinning)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

            m_actorClothSkinning->UpdateSkinning();

            // Since component activation order is not trivial, the actor's pose might not be updated
            // immediately. Because of this cloth will receive a sudden impulse when changing from
            // T pose to animated pose. To avoid this undesired effect we will override cloth simulation during
            // a short amount of frames.
            const AZ::u32 numberOfTicksToDoFullSkinning = 10;
            m_numberOfClothSkinningUpdates++;

            // While the actor is not visible the skinned joints are not updated. Then when
            // it becomes visible the jump to the new skinned positions causes a sudden
            // impulse to cloth simulation. To avoid this undesired effect we will override cloth simulation during
            // a short amount of frames.
            m_actorClothSkinning->UpdateActorVisibility();
            if (!m_actorClothSkinning->WasActorVisible() &&
                m_actorClothSkinning->IsActorVisible())
            {
                m_numberOfClothSkinningUpdates = 0;
            }

            if (m_numberOfClothSkinningUpdates <= numberOfTicksToDoFullSkinning)
            {
                // Update skinning for all particles and apply it to cloth 
                AZStd::vector<SimParticleFormat> particles = m_cloth->GetParticles();
                m_actorClothSkinning->ApplySkinning(m_cloth->GetInitialParticles(), particles);
                m_cloth->SetParticles(AZStd::move(particles));
                m_cloth->DiscardParticleDelta();
            }
        }
    }

    void ClothComponentMesh::UpdateSimulationConstraints()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        m_motionConstraints = m_clothConstraints->GetMotionConstraints();
        m_separationConstraints = m_clothConstraints->GetSeparationConstraints();

        if (m_actorClothSkinning)
        {
            m_actorClothSkinning->ApplySkinning(m_clothConstraints->GetMotionConstraints(), m_motionConstraints);
            m_actorClothSkinning->ApplySkinning(m_clothConstraints->GetSeparationConstraints(), m_separationConstraints);
        }

        m_cloth->GetClothConfigurator()->SetMotionConstraints(m_motionConstraints);
        if (!m_separationConstraints.empty())
        {
            m_cloth->GetClothConfigurator()->SetSeparationConstraints(m_separationConstraints);
        }
    }

    void ClothComponentMesh::UpdateRenderData(const AZStd::vector<SimParticleFormat>& particles)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        // Calculate normals of the cloth particles (simplified mesh).
        AZStd::vector<AZ::Vector3> normals;
        bool normalsCalculated =
            AZ::Interface<ITangentSpaceHelper>::Get()->CalculateNormals(particles, m_cloth->GetInitialIndices(), normals);
        AZ_Assert(normalsCalculated, "Cloth component mesh failed to calculate normals.");

        // Copy particles and normals to render data.
        // Since cloth's vertices were welded together,
        // the full mesh will result in smooth normals.
        auto& renderData = GetRenderData();
        renderData.m_particles.resize_no_construct(m_meshRemappedVertices.size());
        renderData.m_normals.resize_no_construct(m_meshRemappedVertices.size());
        for (size_t index = 0; index < m_meshRemappedVertices.size(); ++index)
        {
            const int remappedIndex = m_meshRemappedVertices[index];
            if (remappedIndex < 0)
            {
                // Removed particle. Assign initial values to have something valid during tangents and bitangents calculation.
                renderData.m_particles[index] = m_meshClothInfo.m_particles[index];
                renderData.m_normals[index] = AZ::Vector3::CreateAxisZ();
            }
            else
            {
                renderData.m_particles[index] = particles[remappedIndex];
                renderData.m_normals[index] = normals[remappedIndex];
            }
        }

        // Calculate tangents and bitangents for the full mesh.
        bool tangentsAndBitangentsCalculated =
            AZ::Interface<ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
                renderData.m_particles, m_meshClothInfo.m_indices,
                m_meshClothInfo.m_uvs, renderData.m_normals,
                renderData.m_tangents, renderData.m_bitangents);
        AZ_Assert(tangentsAndBitangentsCalculated, "Cloth component mesh failed to calculate tangents and bitangents.");
    }

    void ClothComponentMesh::CopyInternalRenderData()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        // Previous buffer index of the render data
        const AZ::u32 previousBufferIndex = (m_renderDataBufferIndex + RenderDataBufferSize - 1) % RenderDataBufferSize;

        // Workaround to sync debug drawing with cloth rendering as
        // the Entity Debug Display Bus renders on the next frame.
        const bool isDebugDrawEnabled = m_clothDebugDisplay && m_clothDebugDisplay->IsDebugDrawEnabled();
        const RenderData& renderData = (isDebugDrawEnabled)
            ? m_renderDataBuffer[previousBufferIndex]
            : m_renderDataBuffer[m_renderDataBufferIndex];

        const auto& renderParticles = renderData.m_particles;
        const auto& renderNormals = renderData.m_normals;
        const auto& renderTangents = renderData.m_tangents;
        const auto& renderBitangents = renderData.m_bitangents;

        auto& internalRenderParticles = m_internalRenderData.m_particles;
        auto& internalRenderNormals = m_internalRenderData.m_normals;
        auto& internalRenderQuatTangents = m_internalRenderData.m_quatTangents;
        auto& internalRenderTangents = m_internalRenderData.m_tangents;

        internalRenderParticles.resize_no_construct(renderParticles.size());
        internalRenderNormals.resize_no_construct(renderNormals.size());
        internalRenderQuatTangents.resize_no_construct(renderTangents.size());
        internalRenderTangents.resize_no_construct(renderTangents.size());

        auto ToVec3 = [](const SimParticleFormat& particle)
        {
            return Vec3(particle.GetX(), particle.GetY(), particle.GetZ());
        };

        // Set based on current cloth particle positions
        for (size_t index = 0; index < renderParticles.size(); ++index)
        {
            if (m_meshRemappedVertices[index] < 0)
            {
                // Removed Particle
                continue;
            }

            internalRenderParticles[index] = ToVec3(renderParticles[index]);

            const Vec3 renderNormal = AZVec3ToLYVec3(renderNormals[index]);
            const Vec3 renderTangent = AZVec3ToLYVec3(renderTangents[index]);
            const Vec3 renderBitangent = AZVec3ToLYVec3(renderBitangents[index]);

            internalRenderNormals[index] = renderNormal; // Converts Vec3 to Vec3f16

            const SMeshTangents tangentSpace(renderTangent, renderBitangent, renderNormal);
            tangentSpace.ExportTo(internalRenderTangents[index]);

            SMeshQTangents quatTangentSpace(MeshTangentFrameToQTangent(tangentSpace));
            quatTangentSpace.ExportTo(internalRenderQuatTangents[index]);
        }
    }

    bool ClothComponentMesh::CreateCloth()
    {
        AZStd::unique_ptr<AssetHelper> assetHelper = AssetHelper::CreateAssetHelper(m_entityId);
        if (!assetHelper)
        {
            return false;
        }

        // Obtain cloth mesh info
        bool clothInfoObtained = assetHelper->ObtainClothMeshNodeInfo(m_config.m_meshNode,
            m_meshNodeInfo, m_meshClothInfo);
        if (!clothInfoObtained)
        {
            return false;
        }

        // Generate a simplified mesh for simulation
        AZStd::vector<SimParticleFormat> meshSimplifiedParticles;
        AZStd::vector<SimIndexType> meshSimplifiedIndices;
        AZ::Interface<IFabricCooker>::Get()->SimplifyMesh(
            m_meshClothInfo.m_particles, m_meshClothInfo.m_indices,
            meshSimplifiedParticles, meshSimplifiedIndices,
            m_meshRemappedVertices,
            m_config.m_removeStaticTriangles);
        if (meshSimplifiedParticles.empty() ||
            meshSimplifiedIndices.empty())
        {
            return false;
        }

        // Cook Fabric
        AZStd::optional<FabricCookedData> cookedData =
            AZ::Interface<IFabricCooker>::Get()->CookFabric(meshSimplifiedParticles, meshSimplifiedIndices);
        if (!cookedData)
        {
            return false;
        }

        // Create cloth instance
        m_cloth = AZ::Interface<IClothSystem>::Get()->CreateCloth(meshSimplifiedParticles, *cookedData);
        if (!m_cloth)
        {
            return false;
        }

        // Set initial Position and Rotation
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, m_entityId, &AZ::TransformInterface::GetWorldTM);
        TeleportCloth(transform);

        ApplyConfigurationToCloth();

        // Add cloth to default solver to be simulated
        AZ::Interface<IClothSystem>::Get()->AddCloth(m_cloth);

        return true;
    }

    void ClothComponentMesh::ApplyConfigurationToCloth()
    {
        IClothConfigurator* clothConfig = m_cloth->GetClothConfigurator();

        // Mass
        clothConfig->SetMass(m_config.m_mass);

        // Gravity and scale
        if (m_config.IsUsingWorldBusGravity())
        {
            AZ::Vector3 gravity(0.0f, 0.0f, -9.81f);
            Physics::WorldRequestBus::EventResult(gravity, Physics::DefaultPhysicsWorldId, &Physics::WorldRequests::GetGravity);
            clothConfig->SetGravity(gravity * m_config.m_gravityScale);
        }
        else
        {
            clothConfig->SetGravity(m_config.m_customGravity * m_config.m_gravityScale);
        }

        // Stiffness Frequency
        clothConfig->SetStiffnessFrequency(m_config.m_stiffnessFrequency);

        // Motion constraints parameters
        clothConfig->SetMotionConstraintsScale(m_config.m_motionConstraintsScale);
        clothConfig->SetMotionConstraintsBias(m_config.m_motionConstraintsBias);
        clothConfig->SetMotionConstraintsStiffness(m_config.m_motionConstraintsStiffness);

        // Damping parameters
        clothConfig->SetDamping(m_config.m_damping);
        clothConfig->SetDampingLinearDrag(m_config.m_linearDrag);
        clothConfig->SetDampingAngularDrag(m_config.m_angularDrag);

        // Inertia parameters
        clothConfig->SetLinearInertia(m_config.m_linearInteria);
        clothConfig->SetAngularInertia(m_config.m_angularInteria);
        clothConfig->SetCentrifugalInertia(m_config.m_centrifugalInertia);

        // Wind parameters
        if (m_config.IsUsingWindBus())
        {
            clothConfig->SetWindVelocity(GetWindBusVelocity());
        }
        else
        {
            clothConfig->SetWindVelocity(m_config.m_windVelocity);
        }
        clothConfig->SetWindDragCoefficient(m_config.m_airDragCoefficient);
        clothConfig->SetWindLiftCoefficient(m_config.m_airLiftCoefficient);
        clothConfig->SetWindFluidDensity(m_config.m_fluidDensity);

        // Collision parameters
        clothConfig->SetCollisionFriction(m_config.m_collisionFriction);
        clothConfig->SetCollisionMassScale(m_config.m_collisionMassScale);
        clothConfig->EnableContinuousCollision(m_config.m_continuousCollisionDetection);
        clothConfig->SetCollisionAffectsStaticParticles(m_config.m_collisionAffectsStaticParticles);

        // Self Collision parameters
        clothConfig->SetSelfCollisionDistance(m_config.m_selfCollisionDistance);
        clothConfig->SetSelfCollisionStiffness(m_config.m_selfCollisionStiffness);

        // Tether Constraints parameters
        clothConfig->SetTetherConstraintStiffness(m_config.m_tetherConstraintStiffness);
        clothConfig->SetTetherConstraintScale(m_config.m_tetherConstraintScale);

        // Quality parameters
        clothConfig->SetSolverFrequency(m_config.m_solverFrequency);
        clothConfig->SetAcceleationFilterWidth(m_config.m_accelerationFilterIterations);

        // Fabric Phases
        clothConfig->SetVerticalPhaseConfig(
            m_config.m_verticalStiffness,
            m_config.m_verticalStiffnessMultiplier,
            m_config.m_verticalCompressionLimit,
            m_config.m_verticalStretchLimit);
        clothConfig->SetHorizontalPhaseConfig(
            m_config.m_horizontalStiffness,
            m_config.m_horizontalStiffnessMultiplier,
            m_config.m_horizontalCompressionLimit,
            m_config.m_horizontalStretchLimit);
        clothConfig->SetBendingPhaseConfig(
            m_config.m_bendingStiffness,
            m_config.m_bendingStiffnessMultiplier,
            m_config.m_bendingCompressionLimit,
            m_config.m_bendingStretchLimit);
        clothConfig->SetShearingPhaseConfig(
            m_config.m_shearingStiffness,
            m_config.m_shearingStiffnessMultiplier,
            m_config.m_shearingCompressionLimit,
            m_config.m_shearingStretchLimit);
    }

    void ClothComponentMesh::MoveCloth(const AZ::Transform& worldTransform)
    {
        m_worldPosition = worldTransform.GetTranslation();

        m_cloth->GetClothConfigurator()->SetTransform(worldTransform);

        if (m_config.IsUsingWindBus())
        {
            // Wind velocity is affected by world position
            m_cloth->GetClothConfigurator()->SetWindVelocity(GetWindBusVelocity());
        }
    }

    void ClothComponentMesh::TeleportCloth(const AZ::Transform& worldTransform)
    {
        MoveCloth(worldTransform);

        // By clearing inertia the cloth won't be affected by the sudden translation caused when teleporting the entity.
        m_cloth->GetClothConfigurator()->ClearInertia();
    }

    AZ::Vector3 ClothComponentMesh::GetWindBusVelocity()
    {
        const Physics::WindRequests* windRequests = AZ::Interface<Physics::WindRequests>::Get();
        if (windRequests)
        {
            const AZ::Vector3 globalWind = windRequests->GetGlobalWind();
            const AZ::Vector3 localWind = windRequests->GetWind(m_worldPosition);
            return globalWind + localWind;
        }
        return AZ::Vector3::CreateZero();
    }
} // namespace NvCloth
