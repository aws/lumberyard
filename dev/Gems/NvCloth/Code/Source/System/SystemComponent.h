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

#include <NvCloth/SystemBus.h>
#include <CrySystemBus.h>

#ifdef NVCLOTH_EDITOR
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#endif //NVCLOTH_EDITOR

namespace nv
{
    namespace cloth
    {
        class Factory;
        class Solver;
        class Fabric;
        class Cloth;
    }
}

namespace NvCloth
{
    // Defines deleters for NvCloth types to destroy them appropriately,
    // allowing to handle them with unique pointers.
    struct NvClothTypesDeleter
    {
        void operator()(nv::cloth::Factory* factory) const;
        void operator()(nv::cloth::Solver* factory) const;
        void operator()(nv::cloth::Fabric* fabric) const;
        void operator()(nv::cloth::Cloth* cloth) const;
    };

    using FactoryUniquePtr = AZStd::unique_ptr<nv::cloth::Factory, NvClothTypesDeleter>;
    using SolverUniquePtr = AZStd::unique_ptr<nv::cloth::Solver, NvClothTypesDeleter>;
    using FabricUniquePtr = AZStd::unique_ptr<nv::cloth::Fabric, NvClothTypesDeleter>;
    using ClothUniquePtr = AZStd::unique_ptr<nv::cloth::Cloth, NvClothTypesDeleter>;

    class SystemComponent
        : public AZ::Component
        , public SystemRequestBus::Handler
        , protected CrySystemEventBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{89DF5C48-64AC-4B8E-9E61-0D4C7A7B5491}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

    protected:
        // AZ::Component overrides
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // CrySystemEventBus::Handler overrides
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;

        // NvCloth::SystemRequestBus::Handler overrides
        nv::cloth::Factory* GetClothFactory() override;

    private:
        void InitializeNvClothLibrary();
        void TearDownNvClothLibrary();

        // Cloth Factory that creates all other objects
        FactoryUniquePtr m_factory;

#ifdef NVCLOTH_EDITOR
        AZStd::vector<AzToolsFramework::PropertyHandlerBase*> m_propertyHandlers;
#endif //NVCLOTH_EDITOR
    };
} // namespace NvCloth
