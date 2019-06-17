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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelection.h>
#include <Tests/TestTypes.h>

namespace UnitTest
{
    class EditorTransformComponentSelectionTest
        : public AllocatorsTestFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;

    public:
        void SetUp() override
        {
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(
                AzToolsFramework::Components::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));

            AZStd::unique_ptr<AzToolsFramework::EditorTransformComponentSelection>
                editorTransformComponentSelection = AZStd::make_unique<AzToolsFramework::EditorTransformComponentSelection>();

            AZ_UNUSED(editorTransformComponentSelection);
        }

        void TearDown() override
        {
            m_serializeContext.reset();
            m_transformComponentDescriptor.reset();
        }
    };

    TEST_F(EditorTransformComponentSelectionTest, TestSelection)
    {
    }
}