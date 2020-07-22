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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <CrySystemBus.h>

#include <NvCloth/SystemBus.h>

#include <System/DataTypes.h>

namespace NvCloth
{
    class SystemComponent
        : public AZ::Component
        , protected CrySystemEventBus::Handler
        , public SystemRequestBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{89DF5C48-64AC-4B8E-9E61-0D4C7A7B5491}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

    protected:
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;

        // CrySystemEventBus overrides
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;

        // NvCloth::SystemRequestBus::Handler overrides
        nv::cloth::Factory* GetClothFactory() override;
        void AddCloth(nv::cloth::Cloth* cloth) override;
        void RemoveCloth(nv::cloth::Cloth* cloth) override;

        // AZ::TickBus::Handler overrides
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;

    private:
        void InitializeNvClothLibrary();
        void TearDownNvClothLibrary();

        void CreateNvClothFactory();
        void DestroyNvClothFactory();

        // Cloth Factory that creates all other objects
        FactoryUniquePtr m_factory;

        // Cloth Solver that contains all cloths
        SolverUniquePtr m_solver;
    };
} // namespace NvCloth
