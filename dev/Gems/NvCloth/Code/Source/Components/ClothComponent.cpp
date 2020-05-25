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

#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Physics/World.h>

#include <IRenderer.h> // Needed for IRenderMesh.h
#include <IRenderMesh.h>
#include <stridedptr.h>

#include <NvCloth/Factory.h>
#include <NvCloth/Solver.h>
#include <NvCloth/Fabric.h>
#include <NvCloth/Cloth.h>

#include <NvClothExt/ClothFabricCooker.h>

#include <Components/ClothComponent.h>
#include <Utils/MathConversion.h>

namespace NvCloth
{
    namespace
    {
        template <typename T>
        nv::cloth::BoundedData ToBoundedData(const T* data, size_t stride, size_t count)
        {
            nv::cloth::BoundedData boundedData;
            boundedData.data = data;
            boundedData.stride = static_cast<physx::PxU32>(stride);
            boundedData.count = static_cast<physx::PxU32>(count);
            return boundedData;
        }
    }

    void ClothConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ClothConfiguration>()
                ->Version(0)
                ->Field("Mesh Node", &ClothConfiguration::m_meshNode)
                ->Field("Mass", &ClothConfiguration::m_mass)
                ->Field("Use Custom Gravity", &ClothConfiguration::m_useCustomGravity)
                ->Field("Custom Gravity", &ClothConfiguration::m_customGravity)
                ->Field("Gravity Scale", &ClothConfiguration::m_gravityScale)
                ->Field("Animation Blend Factor", &ClothConfiguration::m_animationBlendFactor)
                ->Field("Stiffness Frequency", &ClothConfiguration::m_stiffnessFrequency)
                ->Field("Damping", &ClothConfiguration::m_damping)
                ->Field("Linear Drag", &ClothConfiguration::m_linearDrag)
                ->Field("Angular Drag", &ClothConfiguration::m_angularDrag)
                ->Field("Linear Inertia", &ClothConfiguration::m_linearInteria)
                ->Field("Angular Inertia", &ClothConfiguration::m_angularInteria)
                ->Field("Centrifugal Inertia", &ClothConfiguration::m_centrifugalInertia)
                ->Field("Wind Velocity", &ClothConfiguration::m_windVelocity)
                ->Field("Air Drag Coefficient", &ClothConfiguration::m_airDragCoefficient)
                ->Field("Air Lift Coefficient", &ClothConfiguration::m_airLiftCoefficient)
                ->Field("Fluid Density", &ClothConfiguration::m_fluidDensity)
                ->Field("Collision Friction", &ClothConfiguration::m_collisionFriction)
                ->Field("Collision Mass Scale", &ClothConfiguration::m_collisionMassScale)
                ->Field("Continuous Collision Detection", &ClothConfiguration::m_continuousCollisionDetection)
                ->Field("Self Collision Distance", &ClothConfiguration::m_selfCollisionDistance)
                ->Field("Self Collision Stiffness", &ClothConfiguration::m_selfCollisionStiffness)
                ->Field("Horizontal Stiffness", &ClothConfiguration::m_horizontalStiffness)
                ->Field("Horizontal Stiffness Multiplier", &ClothConfiguration::m_horizontalStiffnessMultiplier)
                ->Field("Horizontal Compression Limit", &ClothConfiguration::m_horizontalCompressionLimit)
                ->Field("Horizontal Stretch Limit", &ClothConfiguration::m_horizontalStretchLimit)
                ->Field("Vertical Stiffness", &ClothConfiguration::m_verticalStiffness)
                ->Field("Vertical Stiffness Multiplier", &ClothConfiguration::m_verticalStiffnessMultiplier)
                ->Field("Vertical Compression Limit", &ClothConfiguration::m_verticalCompressionLimit)
                ->Field("Vertical Stretch Limit", &ClothConfiguration::m_verticalStretchLimit)
                ->Field("Bending Stiffness", &ClothConfiguration::m_bendingStiffness)
                ->Field("Bending Stiffness Multiplier", &ClothConfiguration::m_bendingStiffnessMultiplier)
                ->Field("Bending Compression Limit", &ClothConfiguration::m_bendingCompressionLimit)
                ->Field("Bending Stretch Limit", &ClothConfiguration::m_bendingStretchLimit)
                ->Field("Shearing Stiffness", &ClothConfiguration::m_shearingStiffness)
                ->Field("Shearing Stiffness Multiplier", &ClothConfiguration::m_shearingStiffnessMultiplier)
                ->Field("Shearing Compression Limit", &ClothConfiguration::m_shearingCompressionLimit)
                ->Field("Shearing Stretch Limit", &ClothConfiguration::m_shearingStretchLimit)
                ->Field("Tether Constraint Stiffness", &ClothConfiguration::m_tetherConstraintStiffness)
                ->Field("Tether Constraint Scale", &ClothConfiguration::m_tetherConstraintScale)
                ->Field("Solver Frequency", &ClothConfiguration::m_solverFrequency)
                ->Field("Acceleration Filter Iterations", &ClothConfiguration::m_accelerationFilterIterations)
                ;
        }
    }

    MeshNodeList ClothConfiguration::PopulateMeshNodeList()
    {
        if (m_populateMeshNodeListCallback)
        {
            return m_populateMeshNodeListCallback();
        }
        return {};
    }

    bool ClothConfiguration::ShowAnimationBlendFactor()
    {
        if (m_showAnimationBlendFactorCallback)
        {
            return m_showAnimationBlendFactorCallback();
        }
        return false;
    }

    AZ::EntityId ClothConfiguration::GetEntityId()
    {
        if (m_getEntityIdCallback)
        {
            return m_getEntityIdCallback();
        }
        return AZ::EntityId();
    }

    void ClothComponent::Reflect(AZ::ReflectContext* context)
    {
        ClothConfiguration::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ClothComponent, AZ::Component>()
                ->Version(0)
                ->Field("ClothConfiguration", &ClothComponent::m_config)
                ;
        }
    }

    ClothComponent::ClothComponent(const ClothConfiguration& config)
        : m_config(config)
    {
    }
    
    void ClothComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ClothMeshService", 0x6ffcbca5));
    }
    
    void ClothComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("MeshService", 0x71d8a455));
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void ClothComponent::Activate()
    {
        LmbrCentral::MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void ClothComponent::Deactivate()
    {
        LmbrCentral::MeshComponentNotificationBus::Handler::BusDisconnect();

        TearDownComponent();
    }

    AZStd::vector<SimParticleType>& ClothComponent::GetRenderParticles()
    {
        return const_cast<AZStd::vector<SimParticleType>&>(
            static_cast<const ClothComponent&>(*this).GetRenderParticles());
    }

    const AZStd::vector<SimParticleType>& ClothComponent::GetRenderParticles() const
    {
        return m_renderDataBuffer[m_renderDataBufferIndex].m_particles;
    }

    TangentSpaceCalculation& ClothComponent::GetRenderTangentSpaces()
    {
        return const_cast<TangentSpaceCalculation&>(
            static_cast<const ClothComponent&>(*this).GetRenderTangentSpaces());
    }

    const TangentSpaceCalculation& ClothComponent::GetRenderTangentSpaces() const
    {
        return m_renderDataBuffer[m_renderDataBufferIndex].m_tangentSpaces;
    }

    void ClothComponent::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        if (!asset.IsReady())
        {
            return;
        }

        SetupComponent();
    }

    void ClothComponent::OnMeshDestroyed()
    {
        TearDownComponent();
    }

    void ClothComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        AZ_UNUSED(time);

        // Next buffer index of the render data
        m_renderDataBufferIndex = (m_renderDataBufferIndex + 1) % RenderDataBufferSize;

        // Update Cloth Simulation particles
        if (!IsClothFullyAnimated())
        {
            UpdateSimulationCollisions();

            UpdateSimulationStaticParticles();

            UpdateSimulation(deltaTime);

            if (!RetrieveSimulationResults())
            {
                RestoreSimulation();
            }

            auto& renderParticles = GetRenderParticles();
            renderParticles = m_simParticles;
        }

        // Update Cloth particles for rendering
        {
            BlendSkinningAnimation();

            RecalculateTangentSpaces();
        }
    }

    int ClothComponent::GetTickOrder()
    {
        // Using Physics tick order which guarantees to happen after the animation tick,
        // this is necessary for actor cloth colliders to be updated with the current pose.
        return AZ::TICK_PHYSICS;
    }

    void ClothComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
        AZ_UNUSED(local);
        UpdateSimulationTransform(world);
    }

    void ClothComponent::ModifyMesh(size_t lodIndex, size_t primitiveIndex, IRenderMesh* renderMesh)
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

    void ClothComponent::SetupComponent()
    {
        TearDownComponent();

        AZStd::unique_ptr<AssetHelper> assetHelper = AssetHelper::CreateAssetHelper(GetEntityId());
        if (assetHelper)
        {
            // Obtain cloth information
            bool clothInfoObtained = assetHelper->ObtainClothMeshNodeInfo(m_config.m_meshNode, 
                m_meshNodeInfo, m_clothInitialParticles, m_clothInitialIndices, m_clothInitialUVs, m_fullyStaticFabric);

            if (clothInfoObtained)
            {
                // Create cloth instance
                CreateCloth();

                // It will return a valid instance if it's an actor with cloth colliders in it.
                m_actorClothColliders = ActorClothColliders::Create(GetEntityId());

                // It will return a valid instance if it's an actor with skinning data.
                m_actorClothSkinning = ActorClothSkinning::Create(GetEntityId(), m_clothInitialParticles, m_config.m_meshNode);

#ifndef RELEASE
                m_clothDebugDisplay = AZStd::make_unique<ClothDebugDisplay>(this);
#endif

                // Notify the render component to start requesting mesh modifications
                for (const auto& subMesh : m_meshNodeInfo.m_subMeshes)
                {
                    LmbrCentral::MeshModificationRequestBus::Event(GetEntityId(),
                        &LmbrCentral::MeshModificationRequestBus::Events::RequireSendingRenderMeshForModification,
                        m_meshNodeInfo.m_lodLevel, subMesh.m_primitiveIndex);
                }

                AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
                AZ::TickBus::Handler::BusConnect();
                LmbrCentral::MeshModificationNotificationBus::Handler::BusConnect(GetEntityId());
            }
            else
            {
                ClearData();
            }
        }
    }

    void ClothComponent::TearDownComponent()
    {
        if (m_cloth)
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
            LmbrCentral::MeshModificationNotificationBus::Handler::BusDisconnect();

            // Notify the render component to stop requesting mesh modifications
            for (const auto& subMesh : m_meshNodeInfo.m_subMeshes)
            {
                LmbrCentral::MeshModificationRequestBus::Event(GetEntityId(),
                    &LmbrCentral::MeshModificationRequestBus::Events::StopSendingRenderMeshForModification, m_meshNodeInfo.m_lodLevel, subMesh.m_primitiveIndex);
            }

            DestroyCloth();
        }
        ClearData();
    }

    void ClothComponent::ClearData()
    {
        m_clothInitialParticles.clear();
        m_clothInitialIndices.clear();
        m_clothInitialUVs.clear();
        m_simParticles.clear();
        m_renderDataBufferIndex = 0;
        m_renderDataBuffer = {};
        m_meshNodeInfo = {};
        m_actorClothColliders.reset();
        m_actorClothSkinning.reset();
        m_clothDebugDisplay.reset();
    }

    void ClothComponent::CreateCloth()
    {
        // Get the cloth factory from the cloth system
        nv::cloth::Factory* factory = nullptr;
        SystemRequestBus::BroadcastResult(factory, &SystemRequestBus::Events::GetClothFactory);
        AZ_Assert(factory, "Could not get cloth factory.");

        // Create cloth fabric
        {
            const AZStd::vector<SimParticleType>* clothInitialInvMasses = &m_clothInitialParticles;
            if (m_fullyStaticFabric)
            {
                // NvCloth doesn't support cooking a fabric where all its simulation particles are static (inverse masses are all 0).
                // In this situation we will cook the fabric with the default inverse masses (all are 1) and the cloth instance will
                // override them by using the static particles. NvCloth does support the cloth instance to be fully static,
                // but not the fabric.
                clothInitialInvMasses = new AZStd::vector<SimParticleType>(m_clothInitialParticles.size(), SimParticleType(0.0f, 0.0f, 0.0f, 1.0f));
            }

            nv::cloth::ClothMeshDesc meshDesc;
            meshDesc.setToDefault();
            meshDesc.points = ToBoundedData(m_clothInitialParticles.data(), sizeof(SimParticleType), m_clothInitialParticles.size());
            meshDesc.invMasses = ToBoundedData(&clothInitialInvMasses->front().w, sizeof(SimParticleType), clothInitialInvMasses->size());
            meshDesc.triangles = ToBoundedData(m_clothInitialIndices.data(), sizeof(SimIndexType) * 3, m_clothInitialIndices.size() / 3);
            meshDesc.flags = (sizeof(SimIndexType) == 2) ? nv::cloth::MeshFlag::e16_BIT_INDICES : 0;

            // Gravity used to cook the fabric. 
            // It's important that the gravity vector used for cooking is in the same space as the mesh.
            // The phase constrains won't be properly created if the up vector is not the Z axis.
            const AZ::Vector3 fabricGravity(0.0f, 0.0f, -9.81f);
            // True to compute tether constraints (created when cooking with static particles)
            // using geodesic distance (using triangle adjacencies), otherwise it uses linear distance.
            const bool useGeodesicTether = true;

            m_fabric = FabricUniquePtr(
                NvClothCookFabricFromMesh(factory, meshDesc, PxMathConvert(fabricGravity), &m_fabricPhaseTypes, useGeodesicTether));
            AZ_Assert(m_fabric, "Failed to create cloth fabric");

            if (m_fullyStaticFabric)
            {
                delete clothInitialInvMasses;
            }
        }

        // Create cloth
        {
            // Apply the mass modifier of this cloth instance
            if (!AZ::IsClose(m_config.m_mass, 1.0f, std::numeric_limits<float>::epsilon()))
            {
                float inverseMass = (m_config.m_mass > 0.0f) ? (1.0f / m_config.m_mass) : 0.0f;
                for (SimParticleType& particle : m_clothInitialParticles)
                {
                    particle.w *= inverseMass;
                }
            }

            m_cloth = ClothUniquePtr(factory->createCloth(
                nv::cloth::Range<SimParticleType>(m_clothInitialParticles.data(), m_clothInitialParticles.data() + m_clothInitialParticles.size()),
                *m_fabric));
            AZ_Assert(m_cloth, "Failed to create cloth");

            InitializeCloth();

            // Initialize simulation and render data
            m_simParticles = m_clothInitialParticles;
            m_renderDataBufferIndex = 0;
            m_renderDataBuffer[0].m_particles = m_clothInitialParticles;
            RecalculateTangentSpaces();
            for (int i = 1; i < RenderDataBufferSize; ++i)
            {
                m_renderDataBuffer[i] = m_renderDataBuffer[0];
            }
        }

        // Create a NvCloth solver
        {
            m_solver = SolverUniquePtr(factory->createSolver());
            AZ_Assert(m_solver, "Failed to create cloth solver");

            // Add cloth to solver
            m_solver->addCloth(m_cloth.get());
        }
    }

    void ClothComponent::DestroyCloth()
    {
        // Destroy NvCloth Solver
        m_solver.reset();

        // Destroy Cloth instance
        m_cloth.reset();

        // Destroy NvCloth fabric
        m_fabric.reset();
        m_fabricPhaseTypes.clear();
    }

    void ClothComponent::InitializeCloth()
    {
        // Fabric Phases
        SetupFabricPhases();

        UpdateSimulationGravity();

        m_cloth->setStiffnessFrequency(m_config.m_stiffnessFrequency);

        // Damping parameters
        m_cloth->setDamping(PxMathConvert(m_config.m_damping));
        m_cloth->setLinearDrag(PxMathConvert(m_config.m_linearDrag));
        m_cloth->setAngularDrag(PxMathConvert(m_config.m_angularDrag));

        // Inertia parameters
        m_cloth->setLinearInertia(PxMathConvert(m_config.m_linearInteria));
        m_cloth->setAngularInertia(PxMathConvert(m_config.m_angularInteria));
        m_cloth->setCentrifugalInertia(PxMathConvert(m_config.m_centrifugalInertia));

        // Wind parameters
        m_cloth->setWindVelocity(PxMathConvert(m_config.m_windVelocity));
        m_cloth->setDragCoefficient(m_config.m_airDragCoefficient);
        m_cloth->setLiftCoefficient(m_config.m_airLiftCoefficient);
        m_cloth->setFluidDensity(m_config.m_fluidDensity);

        // Collision parameters
        m_cloth->setFriction(m_config.m_collisionFriction);
        m_cloth->setCollisionMassScale(m_config.m_collisionMassScale);
        m_cloth->enableContinuousCollision(m_config.m_continuousCollisionDetection);

        // Self Collision parameters
        m_cloth->setSelfCollisionDistance(m_config.m_selfCollisionDistance);
        m_cloth->setSelfCollisionStiffness(m_config.m_selfCollisionStiffness);

        // Tether Constraints parameters
        m_cloth->setTetherConstraintStiffness(m_config.m_tetherConstraintStiffness);
        m_cloth->setTetherConstraintScale(m_config.m_tetherConstraintScale);

        // Quality parameters
        m_cloth->setSolverFrequency(m_config.m_solverFrequency);
        m_cloth->setAcceleationFilterWidth(m_config.m_accelerationFilterIterations);

        // Initial Position and Rotation
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
        UpdateSimulationTransform(transform);
        m_cloth->clearInertia();
    }

    void ClothComponent::SetupFabricPhases()
    {
        const uint32_t phaseCount = m_fabric->getNumPhases();
        if (phaseCount == 0)
        {
            return;
        }

        AZ_Assert(phaseCount == m_fabricPhaseTypes.size(), "Fabric has %d phases but %d phase types.", phaseCount, m_fabricPhaseTypes.size());

        AZStd::vector<nv::cloth::PhaseConfig> phases(phaseCount);
        for (uint32_t phaseIndex = 0; phaseIndex < phaseCount; ++phaseIndex)
        {
            // Set index to the corresponding set (constraint group)
            phases[phaseIndex].mPhaseIndex = static_cast<uint16_t>(phaseIndex);

            const int32_t phaseType = m_fabricPhaseTypes[phaseIndex];
            switch (phaseType)
            {
            case nv::cloth::ClothFabricPhaseType::eINVALID:
                AZ_Error("ClothComponent", false, "Phase %d of fabric with invalid phase type", phaseIndex);
                break;
            case nv::cloth::ClothFabricPhaseType::eVERTICAL:
                phases[phaseIndex].mStiffness = m_config.m_verticalStiffness;
                phases[phaseIndex].mStiffnessMultiplier = 1.0f - AZ::GetClamp(m_config.m_verticalStiffnessMultiplier, 0.0f, 1.0f);
                phases[phaseIndex].mCompressionLimit = 1.0f + m_config.m_verticalCompressionLimit; // A value of 1.0f is no compression inside nvcloth
                phases[phaseIndex].mStretchLimit = 1.0f + m_config.m_verticalStretchLimit; // A value of 1.0f is no stretch inside nvcloth
                break;
            case nv::cloth::ClothFabricPhaseType::eHORIZONTAL:
                phases[phaseIndex].mStiffness = m_config.m_horizontalStiffness;
                phases[phaseIndex].mStiffnessMultiplier = 1.0f - AZ::GetClamp(m_config.m_horizontalStiffnessMultiplier, 0.0f, 1.0f);
                phases[phaseIndex].mCompressionLimit = 1.0f + m_config.m_horizontalCompressionLimit;
                phases[phaseIndex].mStretchLimit = 1.0f + m_config.m_horizontalStretchLimit;
                break;
            case nv::cloth::ClothFabricPhaseType::eBENDING:
                phases[phaseIndex].mStiffness = m_config.m_bendingStiffness;
                phases[phaseIndex].mStiffnessMultiplier = 1.0f - AZ::GetClamp(m_config.m_bendingStiffnessMultiplier, 0.0f, 1.0f);
                phases[phaseIndex].mCompressionLimit = 1.0f + m_config.m_bendingCompressionLimit;
                phases[phaseIndex].mStretchLimit = 1.0f + m_config.m_bendingStretchLimit;
                break;
            case nv::cloth::ClothFabricPhaseType::eSHEARING:
                phases[phaseIndex].mStiffness = m_config.m_shearingStiffness;
                phases[phaseIndex].mStiffnessMultiplier = 1.0f - AZ::GetClamp(m_config.m_shearingStiffnessMultiplier, 0.0f, 1.0f);
                phases[phaseIndex].mCompressionLimit = 1.0f + m_config.m_shearingCompressionLimit;
                phases[phaseIndex].mStretchLimit = 1.0f + m_config.m_shearingStretchLimit;
                break;
            default:
                AZ_Error("ClothComponent", false, "Phase %d of fabric with unknown phase type %d", phaseIndex, phaseType);
                break;
            }
        }

        m_cloth->setPhaseConfig(
            nv::cloth::Range<nv::cloth::PhaseConfig>(phases.data(), phases.data() + phaseCount));
    }

    void ClothComponent::UpdateSimulationCollisions()
    {
        if (m_actorClothColliders)
        {
            m_actorClothColliders->Update();

            const auto& spheres = m_actorClothColliders->GetSpheres();
            m_cloth->setSpheres(
                nv::cloth::Range<const physx::PxVec4>(spheres.data(), spheres.data() + spheres.size()),
                0, m_cloth->getNumSpheres());

            const auto& capsuleIndices = m_actorClothColliders->GetCapsuleIndices();
            m_cloth->setCapsules(
                nv::cloth::Range<const uint32_t>(capsuleIndices.data(), capsuleIndices.data() + capsuleIndices.size()),
                0, m_cloth->getNumCapsules());
        }
    }

    void ClothComponent::UpdateSimulationStaticParticles()
    {
        if (m_actorClothSkinning)
        {
            m_actorClothSkinning->UpdateStaticParticles(m_clothInitialParticles, m_simParticles);

            // Update cloth copy now that the anchor particles have been skinned
            nv::cloth::MappedRange<physx::PxVec4> particles = m_cloth->getCurrentParticles();
            memcpy(&particles[0], m_simParticles.data(), m_simParticles.size() * sizeof(SimParticleType));
        }
    }

    void ClothComponent::UpdateSimulation(float deltaTime)
    {
        if (m_solver->beginSimulation(deltaTime))
        {
            for (int i = 0; i < m_solver->getSimulationChunkCount(); ++i)
            {
                m_solver->simulateChunk(i);
            }

            m_solver->endSimulation(); // Cloth inter collisions are performed here (when applicable)
        }
    }

    bool ClothComponent::RetrieveSimulationResults()
    {
        const nv::cloth::Cloth* clothConst = m_cloth.get(); // Use a const pointer to call the read-only version of getCurrentParticles()
        const nv::cloth::MappedRange<const SimParticleType> particles = clothConst->getCurrentParticles();

        // NOTE: using type MappedRange<const SimParticleType> is important to call the read-only version of getCurrentParticles(),
        // otherwise when MappedRange gets out of scope it will wake up the cloth to account for the possibility that particles were changed.

        bool validCloth =
            AZStd::all_of(particles.begin(), particles.end(), [](const SimParticleType& particle)
                {
                    return particle.isFinite();
                })
            &&
            // On some platforms when cloth simulation gets corrupted it puts all particles' position to (0,0,0)
            AZStd::any_of(particles.begin(), particles.end(), [](const SimParticleType& particle)
                {
                    return particle.x != 0.0f || particle.y != 0.0f || particle.z != 0.0f;
                });

        if (validCloth)
        {
            AZ_Assert(m_simParticles.size() == particles.size(),
                "Mismatch in number of cloth particles. Expected: %d Updated: %d", m_simParticles.size(), particles.size());

            memcpy(m_simParticles.data(), &particles[0], particles.size() * sizeof(SimParticleType));

            m_numInvalidSimulations = 0;
        }

        return validCloth;
    }

    void ClothComponent::RestoreSimulation()
    {
        const AZ::u32 maxAttemptsToRestoreCloth = 15;

        if (m_numInvalidSimulations <= maxAttemptsToRestoreCloth)
        {
            // Leave the simulation particles in their last good position if
            // the simulation has gone wrong and it has stopped providing valid vertices.

            const nv::cloth::Cloth* clothConst = m_cloth.get(); // Use a const pointer to call the read-only version of getPreviousParticles()
            const nv::cloth::MappedRange<const SimParticleType> previousParticles = clothConst->getPreviousParticles();
            nv::cloth::MappedRange<SimParticleType> currentParticles = m_cloth->getCurrentParticles();

            memcpy(&currentParticles[0], &previousParticles[0], previousParticles.size() * sizeof(SimParticleType));
        }
        else
        {
            // Reset simulation particles to their initial position if after a number of
            // attempts cloth has not been restored to a stable state.

            nv::cloth::MappedRange<SimParticleType> previousParticles = m_cloth->getPreviousParticles();
            nv::cloth::MappedRange<SimParticleType> currentParticles = m_cloth->getCurrentParticles();

            memcpy(&previousParticles[0], m_clothInitialParticles.data(), m_clothInitialParticles.size() * sizeof(SimParticleType));
            memcpy(&currentParticles[0], m_clothInitialParticles.data(), m_clothInitialParticles.size() * sizeof(SimParticleType));
        }

        m_cloth->clearInertia();
        m_cloth->clearInterpolation();

        m_numInvalidSimulations++;
    }

    void ClothComponent::BlendSkinningAnimation()
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
                m_actorClothSkinning->UpdateParticles(m_clothInitialParticles, renderParticles);
            }
            else
            {
                m_actorClothSkinning->UpdateDynamicParticles(m_config.m_animationBlendFactor, m_clothInitialParticles, renderParticles);
            }
        }
    }

    void ClothComponent::RecalculateTangentSpaces()
    {
        AZStd::string errorMessage;
        auto& renderTangentSpaces = GetRenderTangentSpaces();
        TangentSpaceCalculation::Error error = renderTangentSpaces.CalculateTangentSpace(*this, errorMessage);

        AZ_Error("ClothComponent", error == TangentSpaceCalculation::Error::NoErrors, 
            "Tangent space calculation failed: '%s'", errorMessage.c_str());

        const auto& renderParticles = GetRenderParticles();
        AZ_Error("ClothComponent", renderTangentSpaces.GetBaseCount() == renderParticles.size(),
            "Number of tangent spaces (%d) doesn't match with the number of particles (%d).",
            renderTangentSpaces.GetBaseCount(), renderParticles.size());
    }

    void ClothComponent::UpdateSimulationTransform(const AZ::Transform& transformWorld)
    {
        m_cloth->setTranslation(PxMathConvert(transformWorld.GetPosition()));
        m_cloth->setRotation(PxMathConvert(AZ::Quaternion::CreateFromTransform(transformWorld)));
    }

    void ClothComponent::UpdateSimulationGravity()
    {
        AZ::Vector3 gravity = m_config.m_customGravity;
        if (!m_config.m_useCustomGravity)
        {
            // Set default value of gravity in case GetGravity call to WorldRequestBus
            // doesn't set any value to it.
            gravity = AZ::Vector3(0.0f, 0.0f, -9.81f);

            // Obtain gravity from the default physics world
            Physics::WorldRequestBus::EventResult(gravity, Physics::DefaultPhysicsWorldId, &Physics::WorldRequests::GetGravity);
        }
        gravity *= m_config.m_gravityScale;

        m_cloth->setGravity(PxMathConvert(gravity));
    }

    bool ClothComponent::IsClothFullySimulated() const
    {
        return !m_actorClothSkinning || m_config.m_animationBlendFactor <= 0.0f;
    }

    bool ClothComponent::IsClothFullyAnimated() const
    {
        return m_actorClothSkinning && m_config.m_animationBlendFactor >= 1.0f;
    }

    AZ::u32 ClothComponent::GetTriangleCount() const
    {
        return m_clothInitialIndices.size() / 3;
    }

    AZ::u32 ClothComponent::GetVertexCount() const
    {
        return m_clothInitialParticles.size();
    }

    TriangleIndices ClothComponent::GetTriangleIndices(AZ::u32 index) const
    {
        AZ_Assert(index < m_clothInitialIndices.size() / 3, "Triangle index %d outside range [0,%d]", index, m_clothInitialIndices.size() - 1);
        TriangleIndices triangleIndices;
        for (int i = 0; i < 3; ++i)
        {
            triangleIndices[i] = m_clothInitialIndices[index * 3 + i];
        }
        return triangleIndices;
    }

    Vec3 ClothComponent::GetPosition(AZ::u32 index) const
    {
        const auto& renderParticles = GetRenderParticles();
        AZ_Assert(index < renderParticles.size(), "Particle index %d outside range [0,%d]", index, renderParticles.size() - 1);
        const SimParticleType& renderParticle = renderParticles[index];
        return Vec3(renderParticle.x, renderParticle.y, renderParticle.z);
    }

    Vec2 ClothComponent::GetUV(AZ::u32 index) const
    {
        AZ_Assert(index < m_clothInitialUVs.size(), "UV index %d outside range [0,%d]", index, m_clothInitialUVs.size() - 1);
        const SimUVType& uv = m_clothInitialUVs[index];
        return Vec2(uv.GetX(), uv.GetY());
    }
} // namespace NvCloth
