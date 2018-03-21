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
#include "Water_precompiled.h"

#include <AzTest/AzTest.h>
#include <Mocks/ITimerMock.h>
#include <Mocks/ICryPakMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/std/chrono/clocks.h>

#include <Water/WaterSystemComponent.h>
#include <Water/WaterOceanComponent.h>
#include <Water/WaterModule.h>

struct MockGlobalEnvironment
{
    MockGlobalEnvironment()
    {
        m_stubEnv.pTimer = &m_stubTimer;
        m_stubEnv.pCryPak = &m_stubPak;
        m_stubEnv.pConsole = &m_stubConsole;
        m_stubEnv.pSystem = &m_stubSystem;
        m_stubEnv.p3DEngine = nullptr;
        gEnv = &m_stubEnv;
    }

    ~MockGlobalEnvironment()
    {
        gEnv = nullptr;
    }

private:
    SSystemGlobalEnvironment m_stubEnv;
    testing::NiceMock<TimerMock> m_stubTimer;
    testing::NiceMock<CryPakMock> m_stubPak;
    testing::NiceMock<ConsoleMock> m_stubConsole;
    testing::NiceMock<SystemMock> m_stubSystem;
};

TEST(WaterTest, ComponentsWithComponentApplication)
{
    AZ::ComponentApplication::Descriptor appDesc;
    appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
    appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
    appDesc.m_stackRecordLevels = 20;

    MockGlobalEnvironment mocks;

    AZ::ComponentApplication app;
    AZ::Entity* systemEntity = app.Create(appDesc);
    ASSERT_TRUE(systemEntity != nullptr);
    systemEntity->CreateComponent<Water::WaterSystemComponent>();

    systemEntity->Init();
    systemEntity->Activate();

    AZ::Entity* waterEntity = aznew AZ::Entity("water_entity");
    waterEntity->CreateComponent<Water::WaterOceanComponent>();
    app.AddEntity(waterEntity);

    app.Destroy();
    ASSERT_TRUE(true);
}

class WaterTestApp
    : public ::testing::Test
{
public:
    WaterTestApp()
        : m_application()
        , m_systemEntity(nullptr)
    {
    }

    void SetUp() override
    {
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
        appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
        appDesc.m_stackRecordLevels = 20;

        AZ::ComponentApplication::StartupParameters appStartup;
        appStartup.m_createStaticModulesCallback =
            [](AZStd::vector<AZ::Module*>& modules)
        {
            modules.emplace_back(new Water::WaterModule);
        };

        m_systemEntity = m_application.Create(appDesc, appStartup);
        m_systemEntity->Init();
        m_systemEntity->Activate();
    }

    void TearDown() override
    {
        m_application.Destroy();
    }

    AZ::ComponentApplication m_application;
    AZ::Entity* m_systemEntity;
    MockGlobalEnvironment m_mocks;
};

//////////////////////////////////////////////////////////////////////////
// testing class to inspect protected data members in the WaterOceanComponent
struct WaterOceanComponentTester : public Water::WaterOceanComponent
{
    const Water::WaterOceanComponentData& GetConfig() const { return m_data; }

    void AssertTrue(const Water::WaterOceanComponentData& cfg)
    {
        using Flags = AZ::OceanEnvironmentRequests::ReflectionFlags;
        ASSERT_TRUE(m_data.GetReflectRenderFlag(Flags::Entities) == cfg.GetReflectRenderFlag(Flags::Entities));
        ASSERT_TRUE(m_data.GetReflectRenderFlag(Flags::StaticObjects) == cfg.GetReflectRenderFlag(Flags::StaticObjects));
        ASSERT_TRUE(m_data.GetReflectRenderFlag(Flags::TerrainDetailMaterials) == cfg.GetReflectRenderFlag(Flags::TerrainDetailMaterials));
        ASSERT_TRUE(m_data.GetReflectRenderFlag(Flags::Particles) == cfg.GetReflectRenderFlag(Flags::Particles));
        ASSERT_TRUE(m_data.GetOceanMaterialName() == cfg.GetOceanMaterialName());
        ASSERT_TRUE(m_data.GetOceanLevel() == cfg.GetOceanLevel());
        ASSERT_TRUE(m_data.GetUseOceanBottom() == cfg.GetUseOceanBottom());
        ASSERT_TRUE(m_data.GetUseOceanBottom() == cfg.GetUseOceanBottom());
        ASSERT_TRUE(m_data.GetCausticsDistanceAttenuation() == cfg.GetCausticsDistanceAttenuation());
        ASSERT_TRUE(m_data.GetCausticsDepth() == cfg.GetCausticsDepth());
        ASSERT_TRUE(m_data.GetCausticsIntensity() == cfg.GetCausticsIntensity());
        ASSERT_TRUE(m_data.GetCausticsTiling() == cfg.GetCausticsTiling());
        ASSERT_TRUE(m_data.GetCausticsEnabled() == cfg.GetCausticsEnabled());
        ASSERT_TRUE(m_data.GetAnimationWavesAmount() == cfg.GetAnimationWavesAmount());
        ASSERT_TRUE(m_data.GetAnimationWavesSize() == cfg.GetAnimationWavesSize());
        ASSERT_TRUE(m_data.GetAnimationWavesSpeed() == cfg.GetAnimationWavesSpeed());
        ASSERT_TRUE(m_data.GetAnimationWindDirection()  == cfg.GetAnimationWindDirection());
        ASSERT_TRUE(m_data.GetAnimationWindSpeed() == cfg.GetAnimationWindSpeed());
        ASSERT_TRUE(m_data.GetReflectResolutionScale() == cfg.GetReflectResolutionScale());
        ASSERT_TRUE(m_data.GetReflectionAnisotropic() == cfg.GetReflectionAnisotropic());
        ASSERT_TRUE(m_data.GetFogColorMulitplier() == cfg.GetFogColorMulitplier());
        ASSERT_TRUE(m_data.GetFogColor() == cfg.GetFogColor());
        ASSERT_TRUE(m_data.GetFogColorMulitplier() == cfg.GetFogColorMulitplier());
        ASSERT_TRUE(m_data.GetFogDensity() == cfg.GetFogDensity());
        ASSERT_TRUE(m_data.GetWaterTessellationAmount() == cfg.GetWaterTessellationAmount());
        ASSERT_TRUE(m_data.GetGodRaysEnabled() == cfg.GetGodRaysEnabled());
        ASSERT_TRUE(m_data.GetUnderwaterDistortion() == cfg.GetUnderwaterDistortion());
    }
};

TEST_F(WaterTestApp, Ocean_WaterOceanComponent)
{
    AZ::Entity* waterEntity = aznew AZ::Entity("water_entity");
    ASSERT_TRUE(waterEntity != nullptr);
    waterEntity->CreateComponent<Water::WaterOceanComponent>();
    m_application.AddEntity(waterEntity);

    Water::WaterOceanComponent* waterComp = waterEntity->FindComponent<Water::WaterOceanComponent>();
    ASSERT_TRUE(waterComp != nullptr);
    ASSERT_TRUE(reinterpret_cast<WaterOceanComponentTester*>(waterComp)->GetConfig().GetOceanLevel() == AZ::OceanConstants::s_DefaultHeight);
    ASSERT_TRUE(reinterpret_cast<WaterOceanComponentTester*>(waterComp)->GetConfig().GetWaterTessellationAmount() > 0);
}

TEST_F(WaterTestApp, Ocean_WaterOceanComponentMatchesConfiguration)
{
    AZ::Entity* waterEntity = aznew AZ::Entity("water_entity");
    ASSERT_TRUE(waterEntity != nullptr);

    AZ::SimpleLcgRandom rand(AZStd::GetTimeNowMicroSecond());
    using Flags = AZ::OceanEnvironmentRequests::ReflectionFlags;

    Water::WaterOceanComponentData cfg;
    cfg.SetOceanLevel(1.0);
    cfg.SetUseOceanBottom((rand.GetRandom() % 2) == 0);
    cfg.SetCausticsEnabled((rand.GetRandom() % 2) == 0);
    cfg.SetCausticsDistanceAttenuation(2.0f);
    cfg.SetCausticsDepth(3.0f);
    cfg.SetCausticsIntensity(4.0f);
    cfg.SetCausticsTiling(5.0f);
    cfg.SetAnimationWavesAmount(6.0f);
    cfg.SetAnimationWavesSize(7.0f);
    cfg.SetAnimationWavesSpeed(8.0f);
    cfg.SetAnimationWindDirection(9.0f);
    cfg.SetAnimationWindSpeed(10.0f);
    cfg.SetFogColor(AZ::Color((AZ::u8)12, (AZ::u8)13, (AZ::u8)14, (AZ::u8)15));
    cfg.SetFogColorMulitplier(11.0f);
    cfg.SetFogDensity(12.0f);
    cfg.SetReflectResolutionScale(0.123f);
    cfg.SetReflectRenderFlag(Flags::Entities, (rand.GetRandom() % 2) == 0);
    cfg.SetReflectRenderFlag(Flags::StaticObjects, (rand.GetRandom() % 2) == 0);
    cfg.SetReflectRenderFlag(Flags::TerrainDetailMaterials, (rand.GetRandom() % 2) == 0);
    cfg.SetReflectRenderFlag(Flags::Particles, (rand.GetRandom() % 2) == 0);
    cfg.SetReflectionAnisotropic((rand.GetRandom() % 2) == 0);
    cfg.SetWaterTessellationAmount(13);
    cfg.SetOceanMaterialName("fake.mtl");
    cfg.SetGodRaysEnabled((rand.GetRandom() % 2) == 0);
    cfg.SetUnderwaterDistortion(16.0f);

    waterEntity->CreateComponent<Water::WaterOceanComponent>(cfg);

    m_application.AddEntity(waterEntity);

    Water::WaterOceanComponent* waterComp = waterEntity->FindComponent<Water::WaterOceanComponent>();
    ASSERT_TRUE(waterComp != nullptr);
    reinterpret_cast<WaterOceanComponentTester*>(waterComp)->AssertTrue(cfg);
}

TEST_F(WaterTestApp, Ocean_WaterFeatureToggle)
{
    bool bHasOceanComponent = false;
    AZ::OceanFeatureToggleBus::BroadcastResult(bHasOceanComponent, &AZ::OceanFeatureToggleBus::Events::OceanComponentEnabled);
    ASSERT_TRUE(bHasOceanComponent);
}

TEST_F(WaterTestApp, Ocean_ScriptingOceanEnvironmentRequestBus)
{
    static int s_checkCount;
    static int s_testCount;
    s_checkCount = 0;
    s_testCount = 0;
    auto fnScriptAssert = [](bool check)
    {
        s_testCount++;
        if (check)
        {
            s_checkCount++;
        }
        AZ_Assert(check, "script test assert at %d", s_testCount);
    };
    m_application.GetBehaviorContext()->Method("ScriptAssert", fnScriptAssert);

    AZ::Entity* waterEntity = aznew AZ::Entity("water_entity");
    ASSERT_TRUE(waterEntity != nullptr);
    waterEntity->CreateComponent<Water::WaterOceanComponent>();
    waterEntity->Init();
    waterEntity->Activate();

    // OceanEnvironmentRequestBus.Broadcast tests
    const char luaCode[] =
        R"LUA(
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetOceanLevel() == 0.0);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetFogDensity(2) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetFogDensity() == 2);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetWaterTessellationAmount(3) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetWaterTessellationAmount() == 3);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetAnimationWindDirection(4) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetAnimationWindDirection() == 4);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetAnimationWindSpeed(5) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetAnimationWindSpeed() == 5);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetAnimationWavesSpeed(6) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetAnimationWavesSpeed() == 6);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetAnimationWavesSize(7) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetAnimationWavesSize() == 7);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetAnimationWavesAmount(8) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetAnimationWavesAmount() == 8);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetReflectResolutionScale(9) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetReflectResolutionScale() == 9);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetReflectionAnisotropic(false) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetReflectionAnisotropic() == false);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetUseOceanBottom(false) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetUseOceanBottom() == false);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetGodRaysEnabled(false) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetGodRaysEnabled() == false);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetUnderwaterDistortion(10) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetUnderwaterDistortion() == 10);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetFogColor(Color(1.0,2.0,3.0)) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetFogColor() == Color(1.0,2.0,3.0));
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetFogColorMulitplier(2.0) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetFogColorMulitplier() == 2.0);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetFogColorPremultiplied() == Color(2.0,4.0,6.0,2.0));
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetNearFogColor(Color(1.1,2.2,3.3)) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetNearFogColor() == Color(1.1,2.2,3.3));
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetReflectRenderFlag(ReflectionFlags.TerrainDetailMaterials, false) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetReflectRenderFlag(ReflectionFlags.TerrainDetailMaterials)  == false);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetReflectRenderFlag(ReflectionFlags.StaticObjects, false) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetReflectRenderFlag(ReflectionFlags.StaticObjects)  == false);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetReflectRenderFlag(ReflectionFlags.Particles, false) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetReflectRenderFlag(ReflectionFlags.Particles)  == false);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetReflectRenderFlag(ReflectionFlags.Entities, false) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetReflectRenderFlag(ReflectionFlags.Entities)  == false);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetCausticsEnabled(false) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetCausticsEnabled()  == false);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetCausticsDepth(0.5) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetCausticsDepth() == 0.5);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetCausticsIntensity(0.25) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetCausticsIntensity()  == 0.25);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetCausticsTiling(1.50) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetCausticsTiling()  == 1.50);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetCausticsDistanceAttenuation(5.50) or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetCausticsDistanceAttenuation()  == 5.50);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.SetOceanMaterialName('fake.mtl') or true);
            ScriptAssert(OceanEnvironmentRequestBus.Broadcast.GetOceanMaterialName()  == 'fake.mtl');
        )LUA";

    AZ::ScriptContext script;
    script.BindTo(m_application.GetBehaviorContext());
    ASSERT_TRUE(script.Execute(luaCode));
    ASSERT_TRUE(s_checkCount == 52);
}


#if WATER_GEM_EDITOR
#include "../Source/WaterOceanEditor.h"

TEST_F(WaterTestApp, Ocean_EditorCreateGameEntity)
{
    AZ::Entity* infiniteOceanEntity = aznew AZ::Entity("infinite_ocean_entity");
    ASSERT_TRUE(infiniteOceanEntity != nullptr);

    Water::WaterOceanEditor editor;
    auto* editorBase = static_cast<AzToolsFramework::Components::EditorComponentBase*>(&editor);
    editorBase->BuildGameEntity(infiniteOceanEntity);

    // the new game entity's ocean component should look like the default one
    Water::WaterOceanComponentData cfg;

    Water::WaterOceanComponent* oceanComp = infiniteOceanEntity->FindComponent<Water::WaterOceanComponent>();
    ASSERT_TRUE(oceanComp != nullptr);
    reinterpret_cast<WaterOceanComponentTester*>(oceanComp)->AssertTrue(cfg);
}

#endif

AZ_UNIT_TEST_HOOK();
