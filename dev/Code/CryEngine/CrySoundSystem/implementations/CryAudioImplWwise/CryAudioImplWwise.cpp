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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"

#include <platform_impl.h>      // include this once per module!
#include <IEngineModule.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include <AzCore/Memory/OSAllocator.h>

#include <IAudioSystem.h>
#include <AudioSystemImplCVars.h>
#include <AudioSystemImpl_wwise.h>


namespace Audio
{
    // Define global objects.
    CAudioWwiseImplCVars g_audioImplCVars_wwise;
    CAudioLogger g_audioImplLogger_wwise;

#if AZ_TRAIT_CRYAUDIOIMPLWWISE_PROVIDE_IMPL_SECONDARY_POOL
    tMemoryPoolReferenced g_audioImplMemoryPoolSecondary_wwise;
#endif // AZ_TRAIT_CRYAUDIOIMPLWWISE_PROVIDE_IMPL_SECONDARY_POOL

    namespace Platform
    {
        void* InitializeSecondaryMemoryPool(size_t& secondarySize, CAudioWwiseImplCVars& audioImplCVars_wwise);
    }

} // namespace Audio

///////////////////////////////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAudioImplWwise
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule);
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAudioImplWwise, "CryAudioImplWwise", 0xB4971E5DD02442C5, 0xB34A9AC0B4ABFFFD);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    const char* GetName() const override
    {
        return "CryAudioImplWwise";
    }
    const char* GetCategory() const override
    {
        return "CryAudio";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
    {
        using namespace Audio;
        bool bSuccess = false;

        // initialize memory pools
        if (!AZ::AllocatorInstance<Audio::AudioImplAllocator>::IsReady())
        {
            const size_t poolSize = g_audioImplCVars_wwise.m_nPrimaryMemoryPoolSize << 10;

            Audio::AudioImplAllocator::Descriptor allocDesc;

            // Generic Allocator:
            allocDesc.m_allocationRecords = true;
            allocDesc.m_heap.m_numFixedMemoryBlocks = 1;
            allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0] = poolSize;

            allocDesc.m_heap.m_fixedMemoryBlocks[0] = AZ::AllocatorInstance<AZ::OSAllocator>::Get().Allocate(allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0], allocDesc.m_heap.m_memoryBlockAlignment);

            // Note: This allocator is destroyed in CAudioSystemImpl_wwise::Release() after the impl object has been freed.
            AZ::AllocatorInstance<Audio::AudioImplAllocator>::Create(allocDesc);
        }

    #if AZ_TRAIT_CRYAUDIOIMPLWWISE_PROVIDE_IMPL_SECONDARY_POOL
        size_t nSecondarySize = 0;
        void* pSecondaryMemory = Platform::InitializeSecondaryMemoryPool(nSecondarySize, g_audioImplCVars_wwise);

        g_audioImplMemoryPoolSecondary_wwise.InitMem(nSecondarySize, (uint8*)pSecondaryMemory);
    #endif // AZ_TRAIT_CRYAUDIOIMPLWWISE_PROVIDE_IMPL_SECONDARY_POOL

        auto implComponent = azcreate(CAudioSystemImpl_wwise, (), Audio::AudioImplAllocator, "AudioSystemImpl_wwise");

        if (implComponent)
        {
            g_audioImplLogger_wwise.Log(eALT_ALWAYS, "%s loaded!", GetName());

            SAudioRequest oAudioRequestData;
            oAudioRequestData.nFlags = (eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING);

            SAudioManagerRequestData<eAMRT_INIT_AUDIO_IMPL> oAMData;
            oAudioRequestData.pData = &oAMData;
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

            bSuccess = true;
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ALWAYS, "%s failed to load!", GetName());
        }

        return bSuccess;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryAudioImplWwise)

///////////////////////////////////////////////////////////////////////////////////////////////////
CEngineModule_CryAudioImplWwise::CEngineModule_CryAudioImplWwise()
{
    Audio::g_audioImplCVars_wwise.RegisterVariables();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CEngineModule_CryAudioImplWwise::~CEngineModule_CryAudioImplWwise()
{
}

#include <CrtDebugStats.h>
