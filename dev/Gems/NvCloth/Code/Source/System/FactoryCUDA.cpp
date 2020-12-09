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

#include <System/FactoryCUDA.h>
#include <System/SystemComponent.h>

// NvCloth library includes
#include <NvCloth/Factory.h>

#ifdef CUDA_ENABLED
#include <cuda.h>
#include <ISystem.h> // Needed for HMODULE
#include <CryLibrary.h>
#endif

namespace NvCloth
{
    namespace
    {
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
#endif //CUDA_ENABLED
    }

    void FactoryCUDA::Init()
    {
#ifdef CUDA_ENABLED
        // Try to create a GPU(CUDA) NvCloth Factory
        ClothCudaContextManager = CudaContextManager::Create();
        if (ClothCudaContextManager)
        {
            m_nvFactory = NvFactoryUniquePtr(NvClothCreateFactoryCUDA(ClothCudaContextManager->GetContext()));
            AZ_Assert(m_nvFactory, "Failed to create CUDA cloth factory");

            if (SystemComponent::CheckLastClothError())
            {
                m_isUsingCUDA = true;
                AZ_Printf("Cloth", "NVIDIA NvCloth Gem using GPU (CUDA) for cloth simulation.\n");
            }
            else
            {
                m_nvFactory.reset(); // Destroy this CUDA factory that was initialized with errors
                ClothCudaContextManager.reset(); // Destroy CUDA Context Manager
                SystemComponent::ResetLastClothError(); // Reset error to give way to create a different type of factory
                AZ_Warning("Cloth", false,
                    "NvCloth library failed to create CUDA factory. "
                    "Please check CUDA is installed, the GPU has CUDA support and its drivers are up to date.");
            }
        }
#endif //CUDA_ENABLED

        // Fallback to CPU simulation in case CUDA is not available
        if (!m_isUsingCUDA)
        {
            Factory::Init();
        }
    }

    void FactoryCUDA::Destroy()
    {
        m_isUsingCUDA = false;

        Factory::Destroy();

#ifdef CUDA_ENABLED
        ClothCudaContextManager.reset();
#endif
    }

} // namespace NvCloth
