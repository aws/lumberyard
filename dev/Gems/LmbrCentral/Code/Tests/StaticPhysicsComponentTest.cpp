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
#include "LmbrCentral.h"
#include "LmbrCentralReflectionTest.h"
#include "Physics/StaticPhysicsComponent.h"
#include <AzCore/Component/ComponentApplication.h>

using namespace LmbrCentral;

// Serialized legacy PhysicsComponent containing StaticPhysicsBehavior.
// This should get converted to a StaticPhysicsComponent.
const char kLegacyPhysicsComponentWithStaticBehavior[] =
R"DELIMITER(<ObjectStream version="1">
    <Class name="PhysicsComponent" version="1" type="{A74FA374-8F68-495B-96C1-0BCC8D00EB61}">
        <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
            <Class name="AZ::u64" field="Id" value="123" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
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
</ObjectStream>)DELIMITER";

class LoadStaticPhysicsComponentFromLegacyData
    : public LoadReflectedObjectTest<AZ::ComponentApplication, LmbrCentralModule, StaticPhysicsComponent>
{
protected:
    void SetUp() override
    {
        LoadReflectedObjectTest::SetUp();
        if (m_object)
        {
            m_object->GetConfiguration(m_staticPhysicsConfig);
        }
    }

    const char* GetSourceDataBuffer() const override { return kLegacyPhysicsComponentWithStaticBehavior; }

    AzFramework::StaticPhysicsConfig m_staticPhysicsConfig;
};

TEST_F(LoadStaticPhysicsComponentFromLegacyData, Application_IsRunning)
{
    ASSERT_NE(GetApplication(), nullptr);
}

TEST_F(LoadStaticPhysicsComponentFromLegacyData, Component_Loads)
{
    EXPECT_NE(m_object.get(), nullptr);
}

TEST_F(LoadStaticPhysicsComponentFromLegacyData, ComponentId_MatchesSourceData)
{
    EXPECT_EQ(m_object->GetId(), 123);
}

TEST_F(LoadStaticPhysicsComponentFromLegacyData, EnabledInitially_MatchesSourceData)
{
    EXPECT_EQ(m_staticPhysicsConfig.m_enabledInitially, false);
}

// A legacy PhysicsComponent with no behavior should be converted to a StaticPhysicsComponent.
const char kLegacyPhysicsComponentLackingBehavior[] =
R"DELIMITER(<ObjectStream version="1">
    <Class name="PhysicsComponent" version="1" type="{A74FA374-8F68-495B-96C1-0BCC8D00EB61}">
        <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
            <Class name="AZ::u64" field="Id" value="951" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        </Class>
        <Class name="PhysicsConfiguration" field="Configuration" version="1" type="{3EE60668-D14C-458F-9E83-FEBC654C898E}">
            <Class name="bool" field="Proximity Triggerable" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name="AZStd::shared_ptr" field="Behavior" type="{D5B5ACA6-A81E-410E-8151-80C97B8CD2A0}"/>
            <Class name="AZStd::vector" field="Child Colliders" type="{2BADE35A-6F1B-4698-B2BC-3373D010020C}"/>
        </Class>
    </Class>
</ObjectStream>)DELIMITER";

class LoadStaticPhysicsComponentFromLegacyDataLackingBehavior
    : public LoadReflectedObjectTest<AZ::ComponentApplication, LmbrCentralModule, StaticPhysicsComponent>
{
    const char* GetSourceDataBuffer() const override { return kLegacyPhysicsComponentLackingBehavior; }
};

TEST_F(LoadStaticPhysicsComponentFromLegacyDataLackingBehavior, Component_Loads)
{
    EXPECT_NE(m_object, nullptr);
}

TEST_F(LoadStaticPhysicsComponentFromLegacyDataLackingBehavior, ComponentId_MatchesSourceData)
{
    EXPECT_EQ(m_object->GetId(), 951);
}
