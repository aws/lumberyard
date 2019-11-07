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

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <AzCore/UnitTest/TestTypes.h>

#include "AzToolsFrameworkTestHelpers.h"

using namespace AzToolsFramework;

namespace UnitTest
{
    class EditorVertexSelectionFixture
        : public ToolsApplicationFixture
    {
        void SetUpEditorFixtureImpl() override
        {
            m_entityId = CreateDefaultEditorEntity("Default");

            m_vertexSelection.Create(
                AZ::EntityComponentIdPair(m_entityId, AZ::InvalidComponentId),
                g_mainManipulatorManagerId, AZStd::make_unique<NullHoverSelection>(),
                TranslationManipulators::Dimensions::Three, ConfigureTranslationManipulatorAppearance3d);
        }

        void TearDownEditorFixtureImpl() override
        {
            m_vertexSelection.Destroy();
        }

    public:
        void PopulateVertices();
        void ClearVertices();

        static const AZ::u32 s_vertexCount = 4;

        AZ::EntityId m_entityId;
        AzToolsFramework::EditorVertexSelectionVariable<AZ::Vector3> m_vertexSelection;
    };

    const AZ::u32 EditorVertexSelectionFixture::s_vertexCount;

    void EditorVertexSelectionFixture::PopulateVertices()
    {
        for (size_t vertIndex = 0; vertIndex < 4; ++vertIndex)
        {
            InsertVertexAfter(
                AZ::EntityComponentIdPair(m_entityId, AZ::InvalidComponentId), 0, AZ::Vector3::CreateZero());
        }
    }
    void EditorVertexSelectionFixture::ClearVertices()
    {
        for (size_t vertIndex = 0; vertIndex < 4; ++vertIndex)
        {
            SafeRemoveVertex<AZ::Vector3>(
                AZ::EntityComponentIdPair(m_entityId, AZ::InvalidComponentId), 0);
        }
    }

    TEST_F(EditorVertexSelectionFixture, PropertyEditorEntityChangeAfterVertexAdded)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // connect before insert vertex
        EditorEntityComponentChangeDetector editorEntityComponentChangeDetector(m_entityId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        PopulateVertices();
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(editorEntityComponentChangeDetector.ChangeDetected());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorVertexSelectionFixture, PropertyEditorEntityChangeAfterVertexRemoved)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        PopulateVertices();

        // connect after insert vertex
        EditorEntityComponentChangeDetector editorEntityComponentChangeDetector(m_entityId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        ClearVertices();
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(editorEntityComponentChangeDetector.ChangeDetected());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorVertexSelectionFixture, PropertyEditorEntityChangeAfterTerrainSnap)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        PopulateVertices();

        // connect after insert vertex
        EditorEntityComponentChangeDetector editorEntityComponentChangeDetector(m_entityId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // just provide a placeholder mouse interaction event in this case
        m_vertexSelection.SnapVerticesToTerrain(ViewportInteraction::MouseInteractionEvent{});
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(editorEntityComponentChangeDetector.ChangeDetected());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
} // namespace UnitTest
