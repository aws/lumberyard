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

#include <AzFramework/Physics/World.h>

#include <System/Cloth.h>
#include <System/ClothConfiguration.h>

#include <NvCloth/Factory.h>
#include <NvCloth/Fabric.h>
#include <NvCloth/Cloth.h>
#include <NvClothExt/ClothFabricCooker.h>

#include <NvCloth/SystemBus.h>

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

    Cloth::Cloth(const ClothConfiguration& config,
        const AZStd::vector<SimParticleType>& initialParticles,
        const AZStd::vector<SimIndexType>& initialIndices)
        : m_initialParticles(initialParticles)
        , m_initialIndices(initialIndices)
    {
        Create(config);
    }

    Cloth::~Cloth()
    {
        if (m_cloth)
        {
            ClothInstanceNotificationsBus::Handler::BusDisconnect();

            // Remove cloth from the system
            SystemRequestBus::Broadcast(&SystemRequestBus::Events::RemoveCloth, m_cloth.get());
        }
    }

    void Cloth::Create(const ClothConfiguration& config)
    {
        // Get the cloth factory from the cloth system
        nv::cloth::Factory* factory = nullptr;
        SystemRequestBus::BroadcastResult(factory, &SystemRequestBus::Events::GetClothFactory);
        AZ_Assert(factory, "Could not get cloth factory.");

        // Create cloth fabric
        {
            // Check if all the particles are static (inverse masses are all 0)
            const bool fullyStaticFabric = AZStd::all_of(m_initialParticles.cbegin(), m_initialParticles.cend(),
                [](const SimParticleType& particle)
                {
                    return particle.w == 0.0f;
                });

            const AZStd::vector<SimParticleType>* initialInvMasses = &m_initialParticles;
            if (fullyStaticFabric)
            {
                // NvCloth doesn't support cooking a fabric where all its simulation particles are static (inverse masses are all 0).
                // In this situation we will cook the fabric with the default inverse masses (all are 1) and the cloth instance will
                // override them by using the static particles. NvCloth does support the cloth instance to be fully static,
                // but not the fabric.
                initialInvMasses = new AZStd::vector<SimParticleType>(m_initialParticles.size(), SimParticleType(0.0f, 0.0f, 0.0f, 1.0f));
            }

            nv::cloth::ClothMeshDesc meshDesc;
            meshDesc.setToDefault();
            meshDesc.points = ToBoundedData(m_initialParticles.data(), sizeof(SimParticleType), m_initialParticles.size());
            meshDesc.invMasses = ToBoundedData(&initialInvMasses->front().w, sizeof(SimParticleType), initialInvMasses->size());
            meshDesc.triangles = ToBoundedData(m_initialIndices.data(), sizeof(SimIndexType) * 3, m_initialIndices.size() / 3);
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

            if (fullyStaticFabric)
            {
                delete initialInvMasses;
            }
        }

        // Create cloth
        {
            // Apply the mass modifier of this cloth instance
            if (!AZ::IsClose(config.m_mass, 1.0f, std::numeric_limits<float>::epsilon()))
            {
                float inverseMass = (config.m_mass > 0.0f) ? (1.0f / config.m_mass) : 0.0f;
                for (SimParticleType& particle : m_initialParticles)
                {
                    particle.w *= inverseMass;
                }
            }

            m_cloth = ClothUniquePtr(factory->createCloth(
                nv::cloth::Range<SimParticleType>(m_initialParticles.data(), m_initialParticles.data() + m_initialParticles.size()),
                *m_fabric));
            AZ_Assert(m_cloth, "Failed to create cloth");

            InitializeCloth(config);

            // Initialize simulation data
            m_simParticles = m_initialParticles;
        }

        // Add cloth to the system
        SystemRequestBus::Broadcast(&SystemRequestBus::Events::AddCloth, m_cloth.get());

        ClothInstanceNotificationsBus::Handler::BusConnect();
    }

    const AZStd::vector<SimParticleType>& Cloth::GetInitialParticles() const
    {
        return m_initialParticles;
    }

    const AZStd::vector<SimIndexType>& Cloth::GetInitialIndices() const
    {
        return m_initialIndices;
    }

    const AZStd::vector<SimParticleType>& Cloth::GetParticles() const
    {
        return m_simParticles;
    }

    void Cloth::SetParticles(const AZStd::vector<SimParticleType>& particles)
    {
        if (m_simParticles.size() != particles.size())
        {
            AZ_Warning("Cloth", false, "Unable to set cloth particles as it doesn't match the number of elements");
            return;
        }
        m_simParticles = particles;
        CopyParticlesToCloth();
    }

    void Cloth::SetParticles(AZStd::vector<SimParticleType>&& particles)
    {
        if (m_simParticles.size() != particles.size())
        {
            AZ_Warning("Cloth", false, "Unable to set cloth particles as it doesn't match the number of elements");
            return;
        }
        m_simParticles = AZStd::move(particles);
        CopyParticlesToCloth();
    }

    void Cloth::SetTransform(const AZ::Transform& transformWorld)
    {
        m_cloth->setTranslation(PxMathConvert(transformWorld.GetPosition()));
        m_cloth->setRotation(PxMathConvert(AZ::Quaternion::CreateFromTransform(transformWorld)));
    }

    void Cloth::SetSphereColliders(const AZStd::vector<physx::PxVec4>& spheres)
    {
        m_cloth->setSpheres(
            nv::cloth::Range<const physx::PxVec4>(spheres.data(), spheres.data() + spheres.size()),
            0, m_cloth->getNumSpheres());
    }

    void Cloth::SetCapsuleColliders(const AZStd::vector<uint32_t>& capsuleIndices)
    {
        m_cloth->setCapsules(
            nv::cloth::Range<const uint32_t>(capsuleIndices.data(), capsuleIndices.data() + capsuleIndices.size()),
            0, m_cloth->getNumCapsules());
    }

    void Cloth::ClearInertia()
    {
        m_cloth->clearInertia();
    }

    void Cloth::InitializeCloth(const ClothConfiguration& config)
    {
        // Fabric Phases
        SetupFabricPhases(config);

        // Gravity
        AZ::Vector3 gravity = config.m_customGravity;
        if (!config.m_useCustomGravity)
        {
            // Set default value of gravity in case GetGravity call to WorldRequestBus
            // doesn't set any value to it.
            gravity = AZ::Vector3(0.0f, 0.0f, -9.81f);

            // Obtain gravity from the default physics world
            Physics::WorldRequestBus::EventResult(gravity, Physics::DefaultPhysicsWorldId, &Physics::WorldRequests::GetGravity);
        }
        gravity *= config.m_gravityScale;
        m_cloth->setGravity(PxMathConvert(gravity));

        // Stiffness Frequency
        m_cloth->setStiffnessFrequency(config.m_stiffnessFrequency);

        // Damping parameters
        m_cloth->setDamping(PxMathConvert(config.m_damping));
        m_cloth->setLinearDrag(PxMathConvert(config.m_linearDrag));
        m_cloth->setAngularDrag(PxMathConvert(config.m_angularDrag));

        // Inertia parameters
        m_cloth->setLinearInertia(PxMathConvert(config.m_linearInteria));
        m_cloth->setAngularInertia(PxMathConvert(config.m_angularInteria));
        m_cloth->setCentrifugalInertia(PxMathConvert(config.m_centrifugalInertia));

        // Wind parameters
        m_cloth->setWindVelocity(PxMathConvert(config.m_windVelocity));
        const float airDragMaxValue = 0.97f;
        m_cloth->setDragCoefficient(airDragMaxValue * config.m_airDragCoefficient);
        const float airLiftMaxValue = 0.8f;
        m_cloth->setLiftCoefficient(airLiftMaxValue * config.m_airLiftCoefficient);
        m_cloth->setFluidDensity(config.m_fluidDensity);

        // Collision parameters
        m_cloth->setFriction(config.m_collisionFriction);
        m_cloth->setCollisionMassScale(config.m_collisionMassScale);
        m_cloth->enableContinuousCollision(config.m_continuousCollisionDetection);

        // Self Collision parameters
        m_cloth->setSelfCollisionDistance(config.m_selfCollisionDistance);
        m_cloth->setSelfCollisionStiffness(config.m_selfCollisionStiffness);

        // Tether Constraints parameters
        m_cloth->setTetherConstraintStiffness(config.m_tetherConstraintStiffness);
        m_cloth->setTetherConstraintScale(config.m_tetherConstraintScale);

        // Quality parameters
        m_cloth->setSolverFrequency(config.m_solverFrequency);
        m_cloth->setAcceleationFilterWidth(config.m_accelerationFilterIterations);
    }

    void Cloth::SetupFabricPhases(const ClothConfiguration& config)
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
                phases[phaseIndex].mStiffness = config.m_verticalStiffness;
                phases[phaseIndex].mStiffnessMultiplier = 1.0f - AZ::GetClamp(config.m_verticalStiffnessMultiplier, 0.0f, 1.0f);
                phases[phaseIndex].mCompressionLimit = 1.0f + config.m_verticalCompressionLimit; // A value of 1.0f is no compression inside nvcloth
                phases[phaseIndex].mStretchLimit = 1.0f + config.m_verticalStretchLimit; // A value of 1.0f is no stretch inside nvcloth
                break;
            case nv::cloth::ClothFabricPhaseType::eHORIZONTAL:
                phases[phaseIndex].mStiffness = config.m_horizontalStiffness;
                phases[phaseIndex].mStiffnessMultiplier = 1.0f - AZ::GetClamp(config.m_horizontalStiffnessMultiplier, 0.0f, 1.0f);
                phases[phaseIndex].mCompressionLimit = 1.0f + config.m_horizontalCompressionLimit;
                phases[phaseIndex].mStretchLimit = 1.0f + config.m_horizontalStretchLimit;
                break;
            case nv::cloth::ClothFabricPhaseType::eBENDING:
                phases[phaseIndex].mStiffness = config.m_bendingStiffness;
                phases[phaseIndex].mStiffnessMultiplier = 1.0f - AZ::GetClamp(config.m_bendingStiffnessMultiplier, 0.0f, 1.0f);
                phases[phaseIndex].mCompressionLimit = 1.0f + config.m_bendingCompressionLimit;
                phases[phaseIndex].mStretchLimit = 1.0f + config.m_bendingStretchLimit;
                break;
            case nv::cloth::ClothFabricPhaseType::eSHEARING:
                phases[phaseIndex].mStiffness = config.m_shearingStiffness;
                phases[phaseIndex].mStiffnessMultiplier = 1.0f - AZ::GetClamp(config.m_shearingStiffnessMultiplier, 0.0f, 1.0f);
                phases[phaseIndex].mCompressionLimit = 1.0f + config.m_shearingCompressionLimit;
                phases[phaseIndex].mStretchLimit = 1.0f + config.m_shearingStretchLimit;
                break;
            default:
                AZ_Error("ClothComponent", false, "Phase %d of fabric with unknown phase type %d", phaseIndex, phaseType);
                break;
            }
        }

        m_cloth->setPhaseConfig(
            nv::cloth::Range<nv::cloth::PhaseConfig>(phases.data(), phases.data() + phaseCount));
    }

    void Cloth::UpdateClothInstance(
        [[maybe_unused]] float deltaTime)
    {
        if (!RetrieveSimulationResults())
        {
            RestoreSimulation();
        }
    }

    bool Cloth::RetrieveSimulationResults()
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

    void Cloth::RestoreSimulation()
    {
        nv::cloth::MappedRange<SimParticleType> previousParticles = m_cloth->getPreviousParticles();
        nv::cloth::MappedRange<SimParticleType> currentParticles = m_cloth->getCurrentParticles();

        const AZ::u32 maxAttemptsToRestoreCloth = 15;

        if (m_numInvalidSimulations <= maxAttemptsToRestoreCloth)
        {
            // Leave the simulation particles in their last good position if
            // the simulation has gone wrong and it has stopped providing valid vertices.

            memcpy(&previousParticles[0], m_simParticles.data(), m_simParticles.size() * sizeof(SimParticleType));
            memcpy(&currentParticles[0], m_simParticles.data(), m_simParticles.size() * sizeof(SimParticleType));
        }
        else
        {
            // Reset simulation particles to their initial position if after a number of
            // attempts cloth has not been restored to a stable state.

            memcpy(&previousParticles[0], m_initialParticles.data(), m_initialParticles.size() * sizeof(SimParticleType));
            memcpy(&currentParticles[0], m_initialParticles.data(), m_initialParticles.size() * sizeof(SimParticleType));
        }

        m_cloth->clearInertia();
        m_cloth->clearInterpolation();

        m_numInvalidSimulations++;
    }

    void Cloth::CopyParticlesToCloth()
    {
        nv::cloth::MappedRange<physx::PxVec4> currentParticles = m_cloth->getCurrentParticles();
        memcpy(&currentParticles[0], m_simParticles.data(), m_simParticles.size() * sizeof(SimParticleType));
    }

    ClothTangentSpaces::ClothTangentSpaces(const ClothConfiguration& config,
        const AZStd::vector<SimParticleType>& initialParticles,
        const AZStd::vector<SimIndexType>& initialIndices,
        const AZStd::vector<SimUVType>& initialUVs)
        : Cloth(config, initialParticles, initialIndices)
        , m_initialUVs(initialUVs)
    {
    }

    const AZStd::vector<SimUVType>& ClothTangentSpaces::GetInitialUVs() const
    {
        return m_initialUVs;
    }

    const TangentSpaceCalculation& ClothTangentSpaces::GetTangentSpaces() const
    {
        return m_tangentSpaces;
    }

    void ClothTangentSpaces::UpdateClothInstance(float deltaTime)
    {
        Cloth::UpdateClothInstance(deltaTime);

        m_tangentSpaces.Calculate(m_simParticles, m_initialIndices, m_initialUVs);
    }
} // namespace NvCloth
