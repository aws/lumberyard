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
#include <System/Cloth.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>

#include <NvCloth/Callbacks.h>
#include <NvCloth/Factory.h>
#include <NvCloth/Solver.h>

#ifdef CUDA_ENABLED
#include <cuda.h>
#include <CryLibrary.h>
#endif

namespace NvCloth
{
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
                    m_lastError = code;
                    break;
                }
            }

            physx::PxErrorCode::Enum GetLastError() const
            {
                return m_lastError;
            }

            void ResetLastError()
            {
                m_lastError = physx::PxErrorCode::eNO_ERROR;
            }

        private:
            physx::PxErrorCode::Enum m_lastError = physx::PxErrorCode::eNO_ERROR;
        };

        // Implementation of the assert handler interface directing nvcloth asserts to Lumberyard assertion system.
        class AzClothAssertHandler
            : public nv::cloth::PxAssertHandler
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

#ifdef CUDA_ENABLED
        class CudaContextManager
        {
        public:
            static AZStd::unique_ptr<CudaContextManager> Create()
            {
                // CUDA requires nvcuda.dll to work, which is part of NVIDIA graphics card driver.
                // Loading the library before any cuda call to determine if it's available.
                HMODULE nvcudaLib = CryLoadLibrary("nvcuda.dll");
                if (!nvcudaLib)
                {
                    AZ_Warning("Cloth", false, "Cannot load nvcuda.dll. CUDA requires an NVIDIA GPU with latest drivers.");
                    return nullptr;
                }

                if (!NvClothCompiledWithCudaSupport())
                {
                    AZ_Warning("Cloth", false, "NvCloth library not built with CUDA support");
                    return nullptr;
                }

                // Initialize CUDA and get context.
                CUresult cudaResult = cuInit(0);
                if (cudaResult != CUDA_SUCCESS)
                {
                    AZ_Warning("Cloth", false, "CUDA didn't initialize correctly. Error code: %d", cudaResult);
                    return nullptr;
                }

                int cudaDeviceCount = 0;
                cudaResult = cuDeviceGetCount(&cudaDeviceCount);
                if (cudaResult != CUDA_SUCCESS || cudaDeviceCount <= 0)
                {
                    AZ_Warning("Cloth", false, "CUDA initialization didn't get any devices");
                    return nullptr;
                }

                CUcontext cudaContext = NULL;
                const int flags = 0;
                const CUdevice device = 0; // Use first device to create cuda context on
                cudaResult = cuCtxCreate(&cudaContext, flags, device);
                if (cudaResult != CUDA_SUCCESS)
                {
                    AZ_Warning("Cloth", false, "CUDA didn't create context correctly. Error code: %d", cudaResult);
                    return nullptr;
                }

                return AZStd::make_unique<CudaContextManager>(cudaContext);
            }

            CudaContextManager(CUcontext cudaContext)
                : m_cudaContext(cudaContext)
            {
                AZ_Assert(m_cudaContext, "Invalid cuda context");
            }

            ~CudaContextManager()
            {
                cuCtxDestroy(m_cudaContext);
            }

            CUcontext GetContext()
            {
                return m_cudaContext;
            }

        private:
            CUcontext m_cudaContext = NULL;
        };

        AZStd::unique_ptr<CudaContextManager> ClothCudaContextManager;

        int32_t g_clothEnableCuda = 0;
#endif //CUDA_ENABLED
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

    void SystemComponent::Activate()
    {
        CrySystemEventBus::Handler::BusConnect();

        // Initialization of NvCloth Library delayed until OnCrySystemInitialized
        // because that's when CVARs will be registered and available.
    }

    void SystemComponent::Deactivate()
    {
        CrySystemEventBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        SystemRequestBus::Handler::BusDisconnect();

        TearDownNvClothLibrary();
    }

    void SystemComponent::OnCrySystemInitialized(ISystem& system,
        [[maybe_unused]] const SSystemInitParams& systemInitParams)
    {
        SSystemGlobalEnvironment* globalEnv = system.GetGlobalEnvironment();
        if (globalEnv && globalEnv->pConsole)
        {
            // Can't use macros here because we have to use our pointer.
            // Still using legacy console cvars because they read the values from cfg files.
#ifdef CUDA_ENABLED
            globalEnv->pConsole->Register("cloth_EnableCuda", &g_clothEnableCuda, 0, VF_READONLY | VF_INVISIBLE,
                "If enabled, cloth simulation will run on GPU using CUDA on supported cards.");
#endif //CUDA_ENABLED
        }

        InitializeNvClothLibrary();

        SystemRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void SystemComponent::OnCrySystemShutdown(ISystem& system)
    {
        SSystemGlobalEnvironment* globalEnv = system.GetGlobalEnvironment();
        if (globalEnv && globalEnv->pConsole)
        {
#ifdef CUDA_ENABLED
            globalEnv->pConsole->UnregisterVariable("cloth_EnableCuda");
#endif //CUDA_ENABLED
        }
    }

    nv::cloth::Factory* SystemComponent::GetClothFactory()
    {
        return m_factory.get();
    }

    void SystemComponent::AddCloth(nv::cloth::Cloth* cloth)
    {
        if (cloth)
        {
            m_solver->addCloth(cloth);
        }
    }

    void SystemComponent::RemoveCloth(nv::cloth::Cloth* cloth)
    {
        if (cloth)
        {
            m_solver->removeCloth(cloth);
        }
    }

    void SystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        AZ_UNUSED(time);

        if (m_solver->getNumCloths() > 0)
        {
            SystemNotificationsBus::Broadcast(&SystemNotificationsBus::Events::OnPreUpdateClothSimulation, deltaTime);

            if (m_solver->beginSimulation(deltaTime))
            {
                const int simChunkCount = m_solver->getSimulationChunkCount();
                for (int i = 0; i < simChunkCount; ++i)
                {
                    m_solver->simulateChunk(i);
                }

                m_solver->endSimulation(); // Cloth inter collisions are performed here (when applicable)
            }

            ClothInstanceNotificationsBus::Broadcast(&ClothInstanceNotificationsBus::Events::UpdateClothInstance, deltaTime);

            SystemNotificationsBus::Broadcast(&SystemNotificationsBus::Events::OnPostUpdateClothSimulation, deltaTime);
        }
    }

    int SystemComponent::GetTickOrder()
    {
        // Using Physics tick order which guarantees to happen after the animation tick,
        // this is necessary for actor cloth colliders to be updated with the current pose.
        return AZ::TICK_PHYSICS;
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
        AZ_Assert(ClothErrorCallback->GetLastError() == physx::PxErrorCode::eNO_ERROR, "Failed to initialize NvCloth library");

        // Create Factory
        CreateNvClothFactory();

        // Create NvCloth Solver
        m_solver = SolverUniquePtr(m_factory->createSolver());
        AZ_Assert(m_solver, "Failed to create cloth solver");
    }

    void SystemComponent::TearDownNvClothLibrary()
    {
        // Destroy NvCloth Solver
        m_solver.reset();

        // Destroy Factory
        DestroyNvClothFactory();

        // NvCloth library doesn't need any destruction

        ClothAssertHandler.reset();
        ClothErrorCallback.reset();
        ClothAllocatorCallback.reset();

        AZ::AllocatorInstance<AzClothAllocator>::Destroy();
    }
    void SystemComponent::CreateNvClothFactory()
    {
#ifdef CUDA_ENABLED
        // Try to create a GPU(CUDA) NvCloth Factory
        if (g_clothEnableCuda)
        {
            ClothCudaContextManager = CudaContextManager::Create();
            if (ClothCudaContextManager)
            {
                m_factory = FactoryUniquePtr(NvClothCreateFactoryCUDA(ClothCudaContextManager->GetContext()));
                AZ_Assert(m_factory, "Failed to create CUDA cloth factory");

                if (ClothErrorCallback->GetLastError() != physx::PxErrorCode::eNO_ERROR)
                {
                    m_factory.reset(); // Destroy this CUDA factory that was initialized with errors
                    ClothCudaContextManager.reset(); // Destroy CUDA Context Manager
                    ClothErrorCallback->ResetLastError(); // Reset error to give way to create a different type of factory
                    AZ_Warning("Cloth", false,
                        "NvCloth library failed to create CUDA factory. "
                        "Please check CUDA is installed, the GPU has CUDA support and its drivers are up to date.");
                }
                else
                {
                    AZ_Printf("Cloth", "NVIDIA NvCloth Gem using GPU (CUDA) for cloth simulation.\n");
                }
            }
        }
#endif //CUDA_ENABLED

        // Create a CPU NvCloth Factory
        if (!m_factory)
        {
            m_factory = FactoryUniquePtr(NvClothCreateFactoryCPU());
            AZ_Assert(m_factory, "Failed to create CPU cloth factory");

            if (ClothErrorCallback->GetLastError() != physx::PxErrorCode::eNO_ERROR)
            {
                AZ_Error("Cloth", false, "NvCloth library failed to create CPU factory.");
            }
            else
            {
                AZ_Printf("Cloth", "NVIDIA NvCloth Gem using CPU for cloth simulation.\n");
            }
        }
    }

    void SystemComponent::DestroyNvClothFactory()
    {
        m_factory.reset();

#ifdef CUDA_ENABLED
        ClothCudaContextManager.reset();
#endif
    }
} // namespace NvCloth
