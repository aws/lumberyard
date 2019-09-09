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

#include "StdAfx.h"

#include <platform_impl.h>      // include this once per module!
#include <IEngineModule.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include <AzCore/Memory/OSAllocator.h>

#include <IAudioSystem.h>
#include <AudioSystemImpl_NoSound.h>
#include <AudioSystemImplCVars_NoSound.h>

namespace Audio
{
    CAudioSystemImplCVars_nosound g_audioImplCVars_nosound;
    CAudioLogger g_audioImplLogger_nosound;
} // namespace Audio

///////////////////////////////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAudioImplNoSound
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule);
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAudioImplNoSound, "CryAudioImplNoSound", 0xF085C746B62A4589, 0x8322CD6589015BEA);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    const char* GetName() const override
    {
        return "CryAudioImplNoSound";
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
            const size_t poolSize = g_audioImplCVars_nosound.m_nPrimaryPoolSize << 10;

            Audio::AudioImplAllocator::Descriptor allocDesc;

            // Generic Allocator:
            allocDesc.m_allocationRecords = true;
            allocDesc.m_heap.m_numFixedMemoryBlocks = 1;
            allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0] = poolSize;

            allocDesc.m_heap.m_fixedMemoryBlocks[0] = AZ::AllocatorInstance<AZ::OSAllocator>::Get().Allocate(allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0], allocDesc.m_heap.m_memoryBlockAlignment);

            // Note: This allocator is destroyed in CAudioSystemImpl_NoSound::Release() after the impl object has been freed.
            AZ::AllocatorInstance<Audio::AudioImplAllocator>::Create(allocDesc);
        }

        auto implComponent = azcreate(CAudioSystemImpl_NoSound, (), Audio::AudioImplAllocator, "AudioSystemImpl_nosound");

        if (implComponent)
        {
            g_audioImplLogger_nosound.Log(eALT_ALWAYS, "%s loaded!", GetName());

            SAudioRequest oAudioRequestData;
            oAudioRequestData.nFlags = (eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING);

            SAudioManagerRequestData<eAMRT_INIT_AUDIO_IMPL> oAMData;
            oAudioRequestData.pData = &oAMData;
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

            bSuccess = true;
        }
        else
        {
            g_audioImplLogger_nosound.Log(eALT_ALWAYS, "%s failed to load!", GetName());
        }

        return bSuccess;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryAudioImplNoSound)

///////////////////////////////////////////////////////////////////////////////////////////////////
CEngineModule_CryAudioImplNoSound::CEngineModule_CryAudioImplNoSound()
{
    Audio::g_audioImplCVars_nosound.RegisterVariables();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CEngineModule_CryAudioImplNoSound::~CEngineModule_CryAudioImplNoSound()
{
}

#include <CrtDebugStats.h>
