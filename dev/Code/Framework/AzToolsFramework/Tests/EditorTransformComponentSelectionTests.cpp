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

#include <AzCore/Math/ToString.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelection.h>
#include <AzCore/UnitTest/TestTypes.h>

#include "AzToolsFrameworkTestHelpers.h"

using namespace AzToolsFramework;

namespace UnitTest
{
    // Fixture to support testing EditorTransformComponentSelection functionality on an Entity selection.
    class EditorTransformComponentSelectionTest
        : public ToolsApplicationFixture
        , private NewViewportInteractionModelEnabledRequestBus::Handler
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            NewViewportInteractionModelEnabledRequestBus::Handler::BusConnect();
            m_editorActions.Connect();

            EditorInteractionSystemViewportSelectionRequestBus::Event(
                GetEntityContextId(), &EditorInteractionSystemViewportSelection::SetDefaultHandler);

            m_entityIds.push_back(CreateDefaultEditorEntity("Entity"));

            ToolsApplicationRequestBus::Broadcast(
                &ToolsApplicationRequests::SetSelectedEntities, m_entityIds);
        }

        void TearDownEditorFixtureImpl() override
        {
            m_editorActions.Disconnect();
            NewViewportInteractionModelEnabledRequestBus::Handler::BusDisconnect();
        }

        void ArrangeIndividualRotatedEntitySelection(const AZ::Quaternion& orientation);
        AZStd::optional<AZ::Transform> GetManipulatorTransform() const;
        void RefreshManipulators(EditorTransformComponentSelectionRequests::RefreshType refreshType);
        void SetTransformMode(EditorTransformComponentSelectionRequests::Mode transformMode);
        void OverrideManipulatorOrientation(const AZ::Quaternion& orientation);
        void OverrideManipulatorTranslation(const AZ::Vector3& translation);

    private:
        // NewViewportInteractionModelEnabledRequestBus ...
        bool IsNewViewportInteractionModelEnabled() override;

    public:
        EntityIdList m_entityIds;
        TestEditorActions m_editorActions;
    };

    bool EditorTransformComponentSelectionTest::IsNewViewportInteractionModelEnabled()
    {
        return true;
    }

    void EditorTransformComponentSelectionTest::ArrangeIndividualRotatedEntitySelection(
        const AZ::Quaternion& orientation)
    {
        for (const auto entityId : m_entityIds)
        {
            AZ::TransformBus::Event(
                entityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, orientation);
        }
    }

    AZStd::optional<AZ::Transform> EditorTransformComponentSelectionTest::GetManipulatorTransform() const
    {
        AZStd::optional<AZ::Transform> manipulatorTransform;
        EditorTransformComponentSelectionRequestBus::EventResult(
            manipulatorTransform, GetEntityContextId(),
            &EditorTransformComponentSelectionRequests::GetManipulatorTransform);
        return manipulatorTransform;
    }

    void EditorTransformComponentSelectionTest::RefreshManipulators(
        EditorTransformComponentSelectionRequests::RefreshType refreshType)
    {
        EditorTransformComponentSelectionRequestBus::Event(
            GetEntityContextId(), &EditorTransformComponentSelectionRequests::RefreshManipulators, refreshType);
    }

    void EditorTransformComponentSelectionTest::SetTransformMode(
        EditorTransformComponentSelectionRequests::Mode transformMode)
    {
        EditorTransformComponentSelectionRequestBus::Event(
            GetEntityContextId(), &EditorTransformComponentSelectionRequests::SetTransformMode,
            transformMode);
    }

    void EditorTransformComponentSelectionTest::OverrideManipulatorOrientation(
        const AZ::Quaternion& orientation)
    {
        EditorTransformComponentSelectionRequestBus::Event(
            GetEntityContextId(), &EditorTransformComponentSelectionRequests::OverrideManipulatorOrientation,
            orientation);
    }

    void EditorTransformComponentSelectionTest::OverrideManipulatorTranslation(
        const AZ::Vector3& translation)
    {
        EditorTransformComponentSelectionRequestBus::Event(
            GetEntityContextId(), &EditorTransformComponentSelectionRequests::OverrideManipulatorTranslation,
            translation);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EditorTransformComponentSelection Tests

    TEST_F(EditorTransformComponentSelectionTest, ManipulatorOrientationIsResetWhenEntityOrientationIsReset)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        ArrangeIndividualRotatedEntitySelection(AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)));
        RefreshManipulators(EditorTransformComponentSelectionRequests::RefreshType::All);

        SetTransformMode(EditorTransformComponentSelectionRequests::Mode::Rotation);

        const AZ::Transform manipulatorTransformBefore =
            GetManipulatorTransform().value_or(AZ::Transform::CreateIdentity());

        // check preconditions - manipulator transform matches entity transform
        EXPECT_THAT(manipulatorTransformBefore.GetBasisY(), IsClose(AZ::Vector3::CreateAxisZ()));
        EXPECT_THAT(manipulatorTransformBefore.GetBasisZ(), IsClose(-AZ::Vector3::CreateAxisY()));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // R - reset entity and manipulator orientation when in Rotation Mode
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_R);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        const AZ::Transform manipulatorTransformAfter =
            GetManipulatorTransform().value_or(AZ::Transform::CreateIdentity());

        // check postconditions - manipulator transform matches identity transform
        EXPECT_THAT(manipulatorTransformAfter.GetBasisY(), IsClose(AZ::Vector3::CreateAxisY()));
        EXPECT_THAT(manipulatorTransformAfter.GetBasisZ(), IsClose(AZ::Vector3::CreateAxisZ()));

        for (const auto entityId : m_entityIds)
        {
            AZ::Quaternion entityOrientation;
            AZ::TransformBus::EventResult(
                entityOrientation, entityId, &AZ::TransformBus::Events::GetLocalRotationQuaternion);

            // manipulator orientation matches entity orientation
            EXPECT_THAT(entityOrientation, IsClose(
                AZ::Quaternion::CreateRotationFromUnscaledTransform(manipulatorTransformAfter)));
        }
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorTransformComponentSelectionTest, EntityOrientationRemainsConstantWhenOnlyManipulatorOrientationIsReset)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        const AZ::Quaternion initialEntityOrientation = AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f));
        ArrangeIndividualRotatedEntitySelection(initialEntityOrientation);

        // assign new orientation to manipulator which does not match entity orientation
        OverrideManipulatorOrientation(AZ::Quaternion::CreateRotationZ(AZ::DegToRad(90.0f)));

        SetTransformMode(EditorTransformComponentSelectionRequests::Mode::Rotation);

        const AZ::Transform manipulatorTransformBefore =
            GetManipulatorTransform().value_or(AZ::Transform::CreateIdentity());

        // check preconditions - manipulator transform matches manipulator orientation override (not entity transform)
        EXPECT_THAT(manipulatorTransformBefore.GetBasisX(), IsClose(AZ::Vector3::CreateAxisY()));
        EXPECT_THAT(manipulatorTransformBefore.GetBasisY(), IsClose(-AZ::Vector3::CreateAxisX()));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // Ctrl+R - reset only manipulator orientation when in Rotation Mode
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_R, Qt::ControlModifier);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        const AZ::Transform manipulatorTransformAfter =
            GetManipulatorTransform().value_or(AZ::Transform::CreateIdentity());

        // check postconditions - manipulator transform matches entity transform (manipulator override was cleared)
        EXPECT_THAT(manipulatorTransformAfter.GetBasisY(), IsClose(AZ::Vector3::CreateAxisZ()));
        EXPECT_THAT(manipulatorTransformAfter.GetBasisZ(), IsClose(-AZ::Vector3::CreateAxisY()));

        for (const auto entityId : m_entityIds)
        {
            AZ::Quaternion entityOrientation;
            AZ::TransformBus::EventResult(
                entityOrientation, entityId, &AZ::TransformBus::Events::GetLocalRotationQuaternion);

            // entity transform matches initial (entity transform was not reset, only manipulator was)
            EXPECT_THAT(entityOrientation, IsClose(initialEntityOrientation));
        }
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorTransformComponentSelectionTest, TestComponentPropertyNotificationIsSentAfterModifyingSlice)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::Entity* grandParent = nullptr;
        AZ::Entity* parent = nullptr;
        AZ::Entity* child = nullptr;

        AZ::EntityId grandParentId = CreateDefaultEditorEntity("GrandParent", &grandParent);
        AZ::EntityId parentId = CreateDefaultEditorEntity("Parent", &parent);
        AZ::EntityId childId = CreateDefaultEditorEntity("Child", &child);

        AZ::TransformBus::Event(
            childId, &AZ::TransformInterface::SetParent, parentId);
        AZ::TransformBus::Event(
            parentId, &AZ::TransformInterface::SetParent, grandParentId);

        UnitTest::SliceAssets sliceAssets;
        const auto sliceAssetId = UnitTest::SaveAsSlice({ grandParent }, &m_app, sliceAssets);

        AzToolsFramework::EntityList instantiatedEntities =
            UnitTest::InstantiateSlice(sliceAssetId, sliceAssets);

        const AZ::EntityId entityIdToMove = instantiatedEntities.back()->GetId();
        EditorEntityComponentChangeDetector editorEntityChangeDetector(entityIdToMove);

        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::SetSelectedEntities, EntityIdList{ entityIdToMove } );
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        EditorTransformComponentSelectionRequestBus::Event(
            GetEntityContextId(),
            &EditorTransformComponentSelectionRequests::CopyOrientationToSelectedEntitiesIndividual,
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::DegToRad(90.0f)));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(editorEntityChangeDetector.ChangeDetected());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        UnitTest::DestroySlices(sliceAssets);
    }
} // namespace UnitTest
