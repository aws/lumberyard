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

#include <System/DataTypes.h>

#include <NvCloth/Allocator.h> // Needed for nv::cloth::Vector

#include <Utils/TangentSpaceCalculation.h>

namespace NvCloth
{
    struct ClothConfiguration;

    class ClothInstanceNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual void UpdateClothInstance(float deltaTime) = 0;
    };
    using ClothInstanceNotificationsBus = AZ::EBus<ClothInstanceNotifications>;

    class Cloth
        : protected ClothInstanceNotificationsBus::Handler
    {
    public:
        AZ_RTTI(Cloth, "{D9DEED18-FEF2-440B-8639-A080F8C1F6DB}");

        explicit Cloth(const ClothConfiguration& config,
            const AZStd::vector<SimParticleType>& initialParticles,
            const AZStd::vector<SimIndexType>& initialIndices);
        virtual ~Cloth();

        AZ_DISABLE_COPY_MOVE(Cloth);

        const AZStd::vector<SimParticleType>& GetInitialParticles() const;
        const AZStd::vector<SimIndexType>& GetInitialIndices() const;

        const AZStd::vector<SimParticleType>& GetParticles() const;
        void SetParticles(const AZStd::vector<SimParticleType>& particles);
        void SetParticles(AZStd::vector<SimParticleType>&& particles);

        void SetTransform(const AZ::Transform& transformWorld);

        void SetSphereColliders(const AZStd::vector<physx::PxVec4>& spheres);
        void SetCapsuleColliders(const AZStd::vector<uint32_t>& capsuleIndices);

        void ClearInertia();

    protected:
        // Functions to setup the NvCloth elements
        void Create(const ClothConfiguration& config);
        void InitializeCloth(const ClothConfiguration& config);
        void SetupFabricPhases(const ClothConfiguration& config);

        // ClothInstanceNotificationsBus overrides
        void UpdateClothInstance(float deltaTime) override;

        bool RetrieveSimulationResults();
        void RestoreSimulation();

        void CopyParticlesToCloth();

        // Original cloth data when cloth was created.
        AZStd::vector<SimParticleType> m_initialParticles;
        AZStd::vector<SimIndexType> m_initialIndices;

        // Simulation particles. Stores the results of the cloth simulation.
        AZStd::vector<SimParticleType> m_simParticles;

        AZ::u32 m_numInvalidSimulations = 0;

        // NvCloth elements
        ClothUniquePtr m_cloth;
        FabricUniquePtr m_fabric;
        nv::cloth::Vector<int32_t>::Type m_fabricPhaseTypes;
    };

    class ClothTangentSpaces
        : public Cloth
    {
    public:
        AZ_RTTI(ClothTangentSpaces, "{2EDEB9C2-27FC-4348-BDDC-C4E03BA68F0E}", Cloth);

        ClothTangentSpaces(const ClothConfiguration& config,
            const AZStd::vector<SimParticleType>& initialParticles,
            const AZStd::vector<SimIndexType>& initialIndices,
            const AZStd::vector<SimUVType>& initialUVs);

        const AZStd::vector<SimUVType>& GetInitialUVs() const;

        const TangentSpaceCalculation& GetTangentSpaces() const;

    protected:
        // ClothInstanceNotificationsBus overrides
        void UpdateClothInstance(float deltaTime) override;

        AZStd::vector<SimUVType> m_initialUVs;

        TangentSpaceCalculation m_tangentSpaces;
    };
} // namespace NvCloth
