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

#include <ISystem.h>
#include <IConsole.h>

#include <Utils/Allocators.h>
#include <System/SystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>

#include <NvCloth/Callbacks.h>
#include <NvCloth/Factory.h>
#include <NvCloth/Solver.h>
#include <NvCloth/Fabric.h>
#include <NvCloth/Cloth.h>

#ifdef NVCLOTH_EDITOR
#include <Editor/PropertyTypes.h>
#endif //NVCLOTH_EDITOR

namespace NvCloth
{
    const char* g_clothDebugDrawCVAR = "cloth_DebugDraw";
    const char* g_clothDebugDrawNormalsCVAR = "cloth_DebugDrawNormals";
    const char* g_clothDebugDrawCollidersCVAR = "cloth_DebugDrawColliders";

    namespace
    {
        // Implementation of the memory allocation callback interface using nvcloth allocator.
        class AzClothAllocatorCallback
            : public physx::PxAllocatorCallback
        {
            // NvCloth requires 16-byte alignment
            static const size_t alignment = 16;

            void* allocate(size_t size, const char* typeName, const char* filename, int line) override
            {
                void* ptr = AZ::AllocatorInstance<AzClothAllocator>::Get().Allocate(size, alignment, 0, "NvCloth", filename, line);
                AZ_Assert((reinterpret_cast<size_t>(ptr) & (alignment-1)) == 0, "NvCloth requires %d-byte aligned memory allocations.", alignment);
                return ptr;
            }

            void deallocate(void* ptr) override
            {
                AZ::AllocatorInstance<AzClothAllocator>::Get().DeAllocate(ptr);
            }
        };

        // Implementation of the error callback interface directing nvcloth library errors to Lumberyard error output.
        class AzClothErrorCallback
            : public physx::PxErrorCallback
        {
        public:
            void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override
            {
                switch (code)
                {
                case physx::PxErrorCode::eDEBUG_INFO:
                case physx::PxErrorCode::eNO_ERROR:
                    AZ_TracePrintf("NvCloth", "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                    break;

                case physx::PxErrorCode::eDEBUG_WARNING:
                case physx::PxErrorCode::ePERF_WARNING:
                    AZ_Warning("NvCloth", false, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                    break;

                default:
                    AZ_Error("NvCloth", false, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                    break;
                }
            }
        };

        // Implementation of the assert handler interface directing nvcloth asserts to Lumberyard assertion system.
        class AzClothAssertHandler
            : public physx::PxAssertHandler
        {
        public:
            void operator()(const char* exp, const char* file, int line, bool& ignore) override
            {
                AZ_UNUSED(ignore);
                AZ_Assert(false, "NvCloth library assertion failed in file %s:%d: %s", file, line, exp);
            }
        };

        AZStd::unique_ptr<AzClothAllocatorCallback> ClothAllocatorCallback;
        AZStd::unique_ptr<AzClothErrorCallback> ClothErrorCallback;
        AZStd::unique_ptr<AzClothAssertHandler> ClothAssertHandler;
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SystemComponent, AZ::Component>()
                ->Version(0);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SystemComponent>("NvCloth", "Provides functionality for simulating cloth using NvCloth")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("NvClothService", 0x7bb289b6));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("NvClothService", 0x7bb289b6));
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void SystemComponent::Init()
    {
    }

    void SystemComponent::Activate()
    {
        InitializeNvClothLibrary();

        CrySystemEventBus::Handler::BusConnect();
        SystemRequestBus::Handler::BusConnect();

#ifdef NVCLOTH_EDITOR
        m_propertyHandlers = NvCloth::Editor::RegisterPropertyTypes();
#endif //NVCLOTH_EDITOR
    }

    void SystemComponent::Deactivate()
    {
#ifdef NVCLOTH_EDITOR
        NvCloth::Editor::UnregisterPropertyTypes(m_propertyHandlers);
#endif //NVCLOTH_EDITOR

        SystemRequestBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();

        TearDownNvClothLibrary();
    }

    void SystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
    {
        AZ_UNUSED(systemInitParams);

#if !defined(AZ_MONOLITHIC_BUILD)
        // When module is linked dynamically, we must set our gEnv pointer.
        // When module is linked statically, we'll share the application's gEnv pointer.
        gEnv = system.GetGlobalEnvironment();
#else
        AZ_UNUSED(system);
#endif

        REGISTER_INT(g_clothDebugDrawCVAR, 0, VF_NULL,
            "Draw cloth wireframe mesh:\n"
            " 0 - Disabled\n"
            " 1 - Cloth wireframe and particle weights");

        REGISTER_INT(g_clothDebugDrawNormalsCVAR, 0, VF_NULL,
            "Draw cloth normals:\n"
            " 0 - Disabled\n"
            " 1 - Cloth normals\n"
            " 2 - Cloth normals, tangents and bitangents");

        REGISTER_INT(g_clothDebugDrawCollidersCVAR, 0, VF_NULL,
            "Draw cloth colliders:\n"
            " 0 - Disabled\n"
            " 1 - Cloth colliders");
    }

    void SystemComponent::OnCrySystemShutdown(ISystem& system)
    {
        AZ_UNUSED(system);

        UNREGISTER_CVAR(g_clothDebugDrawCVAR);
        UNREGISTER_CVAR(g_clothDebugDrawNormalsCVAR);
        UNREGISTER_CVAR(g_clothDebugDrawCollidersCVAR);

#if !defined(AZ_MONOLITHIC_BUILD)
        gEnv = nullptr;
#endif
    }

    nv::cloth::Factory* SystemComponent::GetClothFactory()
    {
        return m_factory.get();
    }

    void SystemComponent::InitializeNvClothLibrary()
    {
        AZ::AllocatorInstance<AzClothAllocator>::Create();

        ClothAllocatorCallback = AZStd::make_unique<AzClothAllocatorCallback>();
        ClothErrorCallback = AZStd::make_unique<AzClothErrorCallback>();
        ClothAssertHandler = AZStd::make_unique<AzClothAssertHandler>();

        nv::cloth::InitializeNvCloth(
            ClothAllocatorCallback.get(), 
            ClothErrorCallback.get(), 
            ClothAssertHandler.get(), 
            nullptr/*profilerCallback*/);

        // Create a CPU NvCloth Factory
        m_factory = FactoryUniquePtr(NvClothCreateFactoryCPU());
        AZ_Assert(m_factory, "Failed to create CPU cloth factory");
    }

    void SystemComponent::TearDownNvClothLibrary()
    {
        // Destroy NvCloth Factory
        m_factory.reset();

        // NvCloth library doesn't need any destruction

        ClothAssertHandler.reset();
        ClothErrorCallback.reset();
        ClothAllocatorCallback.reset();

        AZ::AllocatorInstance<AzClothAllocator>::Destroy();
    }

    void NvClothTypesDeleter::operator()(nv::cloth::Factory* factory) const
    {
        NvClothDestroyFactory(factory);
    }

    void NvClothTypesDeleter::operator()(nv::cloth::Solver* solver) const
    {
        // Any cloth instance remaining in the solver must be removed before deleting it.
        while (solver->getNumCloths() > 0)
        {
            solver->removeCloth(*solver->getClothList());
        }
        NV_CLOTH_DELETE(solver);
    }

    void NvClothTypesDeleter::operator()(nv::cloth::Fabric* fabric) const
    {
        fabric->decRefCount();
    }

    void NvClothTypesDeleter::operator()(nv::cloth::Cloth* cloth) const
    {
        NV_CLOTH_DELETE(cloth);
    }
} // namespace NvCloth
