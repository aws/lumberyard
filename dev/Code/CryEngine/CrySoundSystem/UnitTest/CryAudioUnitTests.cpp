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
#include "stdafx.h"
#include "AudioUnitTestData.h"

#include <SoundAllocator.h>
#include <IAudioSystem.h>
#include <platform_impl.h>
#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

namespace Audio
{
    CSoundAllocator g_audioMemoryPool_UnitTests;
} // namespace Audio

///////////////////////////////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAudioUnitTests
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAudioUnitTests, "EngineModule_CryAudioUnitTests", 0x394C85904E8A418B, 0x418BBB6CE507D3EF)

    ///////////////////////////////////////////////////////////////////////////////////////////////
    const char* GetName() const override {
        return "CryAudioUnitTests";
    }
    const char* GetCategory() const override { return "CryAudio"; }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
    {
        CryUnitTest::IUnitTestManager* pTestManager = gEnv->pSystem->GetITestSystem()->GetIUnitTestManager();
        if (pTestManager)
        {
            for (CryUnitTest::Test* pTest = CryUnitTest::Test::m_pFirst; pTest != nullptr; pTest = pTest->m_pNext)
            {
                pTest->m_unitTestInfo.module = GetName();
                pTestManager->CreateTest(pTest->m_unitTestInfo);
            }

            Audio::CAudioUnitTestData::Init();
            return true;
        }
        return false;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryAudioUnitTests)

///////////////////////////////////////////////////////////////////////////////////////////////////
CEngineModule_CryAudioUnitTests::CEngineModule_CryAudioUnitTests()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CEngineModule_CryAudioUnitTests::~CEngineModule_CryAudioUnitTests()
{
}
