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
#include "LmbrCentral_precompiled.h"
#include "LmbrCentralEditor.h"
#include "LmbrCentralReflectionTest.h"
#include "Physics/EditorRigidPhysicsComponent.h"
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

using namespace LmbrCentral;

// Serialized legacy PhysicsComponent containing RigidPhysicsBehavior.
// PhysicsComponent is wrapped by a GenericComponentWrapper because it's being used by the editor.
// This should get converted to an EditorRigidPhysicsComponent.
const char kWrappedLegacyPhysicsComponentWithRigidBehavior[] =
R"DELIMITER(<ObjectStream version="1">
    <Class name="GenericComponentWrapper" type="{68D358CA-89B9-4730-8BA6-E181DEA28FDE}">
        <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
            <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                <Class name="AZ::u64" field="Id" value="11874523501682509824" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
            </Class>
        </Class>
        <Class name="PhysicsComponent" field="m_template" version="1" type="{A74FA374-8F68-495B-96C1-0BCC8D00EB61}">
            <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                <Class name="AZ::u64" field="Id" value="0" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
            </Class>
            <Class name="PhysicsConfiguration" field="Configuration" version="1" type="{3EE60668-D14C-458F-9E83-FEBC654C898E}">
                <Class name="bool" field="Proximity Triggerable" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                <Class name="AZStd::shared_ptr" field="Behavior" type="{D5B5ACA6-A81E-410E-8151-80C97B8CD2A0}">
                    <Class name="RigidPhysicsBehavior" field="element" version="1" type="{48366EA0-71FA-49CA-B566-426EE897468E}">
                        <Class name="RigidPhysicsConfiguration" field="Configuration" version="1" type="{4D4211C2-4539-444F-A8AC-B0C8417AA579}">
                            <Class name="bool" field="EnabledInitially" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                            <Class name="unsigned int" field="SpecifyMassOrDensity" value="0" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
                            <Class name="float" field="Mass" value="33.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                            <Class name="float" field="Density" value="555.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                            <Class name="bool" field="AtRestInitially" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                            <Class name="bool" field="RecordCollisions" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                            <Class name="int" field="MaxRecordedCollisions" value="3" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                            <Class name="float" field="SimulationDamping" value="0.1000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                            <Class name="float" field="SimulationMinEnergy" value="0.0030000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                            <Class name="float" field="BuoyancyDamping" value="0.3000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                            <Class name="float" field="BuoyancyDensity" value="1.3000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                            <Class name="float" field="BuoyancyResistance" value="1.6000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                        </Class>
                    </Class>
                </Class>
                <Class name="AZStd::vector" field="Child Colliders" type="{2BADE35A-6F1B-4698-B2BC-3373D010020C}">
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="16119116356122925873" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="16119118507901541169" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                </Class>
            </Class>
        </Class>
    </Class>
</ObjectStream>)DELIMITER";

class LoadEditorRigidPhysicsComponentFromLegacyData
    : public LoadReflectedObjectTest<AzToolsFramework::ToolsApplication, LmbrCentralEditorModule, AzToolsFramework::Components::GenericComponentWrapper>
{
protected:
    const char* GetSourceDataBuffer() const override { return kWrappedLegacyPhysicsComponentWithRigidBehavior; }

    void SetUp() override
    {
        LoadReflectedObjectTest::SetUp();

        if (m_object)
        {
            if (m_editorPhysicsComponent = azrtti_cast<EditorRigidPhysicsComponent*>(m_object->GetTemplate()))
            {
                m_editorPhysicsComponent->GetConfiguration(m_rigidPhysicsConfig);
            }
        }
    }

    EditorRigidPhysicsComponent* m_editorPhysicsComponent = nullptr;
    AzFramework::RigidPhysicsConfig m_rigidPhysicsConfig;

};

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, Application_IsRunning)
{
    ASSERT_NE(GetApplication(), nullptr);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, Components_Load)
{
    EXPECT_NE(m_object.get(), nullptr);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, EditorComponentWithinWrapper_Found)
{
    EXPECT_NE(m_editorPhysicsComponent, nullptr);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, EnabledInitially_MatchesSourceData)
{
    EXPECT_EQ(m_rigidPhysicsConfig.m_enabledInitially, false);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, SpecifyMassOrDensity_MatchesSourceData)
{
    EXPECT_EQ(m_rigidPhysicsConfig.m_specifyMassOrDensity, static_cast<AzFramework::RigidPhysicsConfig::MassOrDensity>(0));
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, Mass_MatchesSourceData)
{
    EXPECT_FLOAT_EQ(m_rigidPhysicsConfig.m_mass, 33.f);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, Density_MatchesSourceData)
{
    EXPECT_FLOAT_EQ(m_rigidPhysicsConfig.m_density, 555.f);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, AtRestInitially_MatchesSourceData)
{
    EXPECT_EQ(m_rigidPhysicsConfig.m_atRestInitially, false);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, InteractsWithTriggers_MatchesSourceData)
{
    EXPECT_EQ(m_rigidPhysicsConfig.m_interactsWithTriggers, false);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, RecordCollision_MatchesSourceData)
{
    EXPECT_EQ(m_rigidPhysicsConfig.m_recordCollisions, true);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, MaxRecordedCollision_MatchesSourceData)
{
    EXPECT_EQ(m_rigidPhysicsConfig.m_maxRecordedCollisions, 3);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, SimulationDamping_MatchesSourceData)
{
    EXPECT_FLOAT_EQ(m_rigidPhysicsConfig.m_simulationDamping, 0.1f);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, SimulationMinEnergy_MatchesSourceData)
{
    EXPECT_FLOAT_EQ(m_rigidPhysicsConfig.m_simulationMinEnergy, 0.003f);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, BuoyancyDamping_MatchesSourceData)
{
    EXPECT_FLOAT_EQ(m_rigidPhysicsConfig.m_buoyancyDamping, 0.3f);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, BuoyancyDensity_MatchesSourceData)
{
    EXPECT_FLOAT_EQ(m_rigidPhysicsConfig.m_buoyancyDensity, 1.3f);
}

TEST_F(LoadEditorRigidPhysicsComponentFromLegacyData, BuoyancyResistance_MatchesSourceData)
{
    EXPECT_FLOAT_EQ(m_rigidPhysicsConfig.m_buoyancyResistance, 1.6f);
}

// Serialized EditorRigidPhysicsConfiguration version 1.
// This version was accidentally reflected as a templated class.
const char kEditorRigidPhysicsConfigurationV1[] = 
R"DELIMITER(<ObjectStream version="2">
    <Class name="EditorRigidPhysicsConfiguration&lt;RigidPhysicsConfiguration &gt;" field="Configuration" type="{B2FA5441-9B99-5EFA-A606-82752CA23EE8}" version="1" specializationTypeId="{B2FA5441-9B99-5EFA-A606-82752CA23EE8}">
        <Class name="RigidPhysicsConfiguration" field="BaseClass1" type="{4D4211C2-4539-444F-A8AC-B0C8417AA579}" version="1" specializationTypeId="{4D4211C2-4539-444F-A8AC-B0C8417AA579}">
            <Class name="bool" field="EnabledInitially" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}" value="true" specializationTypeId="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name="unsigned int" field="SpecifyMassOrDensity" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}" value="1" specializationTypeId="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
            <Class name="float" field="Mass" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}" value="10.0000000" specializationTypeId="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            <Class name="float" field="Density" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}" value="500.0000000" specializationTypeId="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            <Class name="bool" field="AtRestInitially" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}" value="false" specializationTypeId="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name="bool" field="EnableCollisionResponse" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}" value="true" specializationTypeId="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name="bool" field="InteractsWithTriggers" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}" value="true" specializationTypeId="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name="bool" field="RecordCollisions" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}" value="true" specializationTypeId="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name="int" field="MaxRecordedCollisions" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}" value="1" specializationTypeId="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
            <Class name="float" field="SimulationDamping" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}" value="0.0000000" specializationTypeId="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            <Class name="float" field="SimulationMinEnergy" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}" value="0.0020000" specializationTypeId="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            <Class name="float" field="BuoyancyDamping" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}" value="0.0000000" specializationTypeId="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            <Class name="float" field="BuoyancyDensity" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}" value="1.0000000" specializationTypeId="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            <Class name="float" field="BuoyancyResistance" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}" value="1.0000000" specializationTypeId="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        </Class>
    </Class>
</ObjectStream>)DELIMITER";

class LoadEditorRigidPhysicsConfigurationV1
    : public LoadReflectedObjectTest<AzToolsFramework::ToolsApplication, LmbrCentralEditorModule, EditorRigidPhysicsConfig>
{
    const char* GetSourceDataBuffer() const override { return kEditorRigidPhysicsConfigurationV1; }
};

TEST_F(LoadEditorRigidPhysicsConfigurationV1, Load_Succeeds)
{
    EXPECT_NE(m_object.get(), nullptr);
}
