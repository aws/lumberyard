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
#include "Physics/EditorStaticPhysicsComponent.h"
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

using namespace LmbrCentral;

// Serialized legacy PhysicsComponent containing StaticPhysicsBehavior.
// PhysicsComponent is wrapped by a GenericComponentWrapper because it's being used by the editor.
// This should get converted to an EditorStaticPhysicsComponent.
const char kWrappedLegacyPhysicsComponentWithStaticBehavior[] =
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
                <Class name="bool" field="Proximity Triggerable" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                <Class name="AZStd::shared_ptr" field="Behavior" type="{D5B5ACA6-A81E-410E-8151-80C97B8CD2A0}">
                    <Class name="StaticPhysicsBehavior" field="element" version="1" type="{BC0600CC-5EF5-4753-A8BE-E28194149CA5}">
                        <Class name="StaticPhysicsConfiguration" field="Configuration" version="1" type="{E87BB4E0-D771-4477-83C2-02EFE0016EC7}">
                            <Class name="bool" field="EnabledInitially" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                        </Class>
                    </Class>
                </Class>
                <Class name="AZStd::vector" field="Child Colliders" type="{2BADE35A-6F1B-4698-B2BC-3373D010020C}"/>
            </Class>
        </Class>
    </Class>
</ObjectStream>)DELIMITER";

/**
 * m_object points to GenericComponentWrapper.
 * m_editorPhysicsComponent points to component within the wrapper.
 */
class LoadEditorStaticPhysicsComponentFromLegacyData
    : public LoadReflectedObjectTest<AzToolsFramework::ToolsApplication, LmbrCentralEditorModule, AzToolsFramework::Components::GenericComponentWrapper>
{
protected:
    const char* GetSourceDataBuffer() const override { return kWrappedLegacyPhysicsComponentWithStaticBehavior; }

    void SetUp() override
    {
        LoadReflectedObjectTest::SetUp();

        if (m_object)
        {
            if (m_editorPhysicsComponent = azrtti_cast<EditorStaticPhysicsComponent*>(m_object->GetTemplate()))
            {
                m_editorPhysicsComponent->GetConfiguration(m_editorPhysicsConfig);
            }
        }
    }

    EditorStaticPhysicsComponent* m_editorPhysicsComponent = nullptr;
    EditorStaticPhysicsConfig m_editorPhysicsConfig;
};

TEST_F(LoadEditorStaticPhysicsComponentFromLegacyData, Application_IsRunning)
{
    ASSERT_NE(GetApplication(), nullptr);
}

TEST_F(LoadEditorStaticPhysicsComponentFromLegacyData, Components_Load)
{
    EXPECT_NE(m_object.get(), nullptr);
}

TEST_F(LoadEditorStaticPhysicsComponentFromLegacyData, EditorComponentWithinWrapper_Found)
{
    EXPECT_NE(m_editorPhysicsComponent, nullptr);
}

TEST_F(LoadEditorStaticPhysicsComponentFromLegacyData, EnabledInitially_MatchesSourceData)
{
    EXPECT_EQ(m_editorPhysicsConfig.m_enabledInitially, false);
}

// Serialized kEditorStaticPhysicsConfigurationV1 version 1.
// This version was accidentally reflected as a templated class.
const char kEditorStaticPhysicsConfigurationV1[] =
R"DELIMITER(<ObjectStream version="2">
    <Class name="EditorStaticPhysicsConfiguration&lt;StaticPhysicsConfiguration &gt;" field="Configuration" type="{8309995F-A628-57DA-AAFE-2E04A257EC40}" version="1" specializationTypeId="{8309995F-A628-57DA-AAFE-2E04A257EC40}">
        <Class name="StaticPhysicsConfiguration" field="BaseClass1" type="{2129576B-A548-4F3E-A2A1-87851BF48838}" version="1" specializationTypeId="{2129576B-A548-4F3E-A2A1-87851BF48838}">
            <Class name="bool" field="EnabledInitially" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}" value="true" specializationTypeId="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        </Class>
    </Class>
</ObjectStream>)DELIMITER";

class LoadEditorStaticPhysicsConfigurationV1
    : public LoadReflectedObjectTest<AzToolsFramework::ToolsApplication, LmbrCentralEditorModule, EditorStaticPhysicsConfig>
{
    const char* GetSourceDataBuffer() const override { return kEditorStaticPhysicsConfigurationV1; }
};

TEST_F(LoadEditorStaticPhysicsConfigurationV1, Load_Succeeds)
{
    EXPECT_NE(m_object.get(), nullptr);
}