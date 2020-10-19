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

#include "WhiteBox_precompiled.h"

#include "Viewport/WhiteBoxManipulatorViews.h"
#include "WhiteBoxTestFixtures.h"
#include "WhiteBoxTestUtil.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

namespace UnitTest
{
    static const AzToolsFramework::ManipulatorManagerId TestManipulatorManagerId =
        AzToolsFramework::ManipulatorManagerId(AZ::Crc32("TestManipulatorManagerId"));

    class NullDebugDisplayRequests : public AzFramework::DebugDisplayRequests
    {
    };

    class WhiteBoxManipulatorFixture : public WhiteBoxTestFixture
    {
    public:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        AZStd::unique_ptr<AzToolsFramework::ManipulatorManager> m_manipulatorManager;
    };

    void WhiteBoxManipulatorFixture::SetUpEditorFixtureImpl()
    {
        m_manipulatorManager = AZStd::make_unique<AzToolsFramework::ManipulatorManager>(TestManipulatorManagerId);
    }

    void WhiteBoxManipulatorFixture::TearDownEditorFixtureImpl()
    {
        m_manipulatorManager.reset();
    }

    TEST_F(WhiteBoxManipulatorFixture, ManipulatorBoundsRefreshedAfterBeingMarkedDirty)
    {
        namespace Api = WhiteBox::Api;

        // create and register the manipulator with the test manipulator manager
        auto manipulator = AzToolsFramework::LinearManipulator::MakeShared(AZ::Transform::CreateIdentity());
        manipulator->Register(TestManipulatorManagerId);

        // create a simple white box mesh
        Api::InitializeAsUnitQuad(*m_whiteBox);

        // create polygon manipulator view from white box
        const Api::PolygonHandle polygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const Api::VertexPositionsCollection outlines = Api::PolygonBorderVertexPositions(*m_whiteBox, polygonHandle);
        const AZStd::vector<AZ::Vector3> triangles = Api::PolygonFacesPositions(*m_whiteBox, polygonHandle);
        auto polygonView = WhiteBox::CreateManipulatorViewPolygon(triangles, outlines);

        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(AZStd::move(polygonView));
        manipulator->SetViews(views);

        // position the manipulator offset down the y axis
        const AZ::Vector3& initialPosition = AZ::Vector3::CreateAxisY(10.0f);
        manipulator->SetLocalPosition(initialPosition);

        // simple callback to update the manipulator's current position
        manipulator->InstallMouseMoveCallback(
            [initialPosition, &manipulator](const AzToolsFramework::LinearManipulator::Action& action)
            {
                manipulator->SetLocalPosition(initialPosition + action.LocalPositionOffset());
            });

        // center the camera at the origin
        AzFramework::CameraState cameraState;
        cameraState.m_position = AZ::Vector3::CreateZero();

        // create a 'mock' mouse interaction simulating a ray firing down the positive y axis
        AzToolsFramework::ViewportInteraction::MouseInteraction mouseInteraction;
        mouseInteraction.m_mousePick.m_rayOrigin = AZ::Vector3::CreateZero();
        mouseInteraction.m_mousePick.m_rayDirection = AZ::Vector3::CreateAxisY();
        mouseInteraction.m_mouseButtons.m_mouseButtons =
            static_cast<AZ::u32>(AzToolsFramework::ViewportInteraction::MouseButton::Left);

        // dummy debug display bus
        NullDebugDisplayRequests nullDebugDisplayRequests;

        // 'draw' manipulators to refresh and register bounds
        m_manipulatorManager->DrawManipulators(nullDebugDisplayRequests, cameraState, mouseInteraction);
        // move to a valid position so the mouse pick ray intersects the manipulator bound (view)
        m_manipulatorManager->ConsumeViewportMouseMove(mouseInteraction);
        // verify precondition - the manipulator recognizes is has the mouse over it
        EXPECT_TRUE(manipulator->MouseOver());

        // simulate a click and drag motion (click and then move the camera to the right)
        m_manipulatorManager->ConsumeViewportMousePress(mouseInteraction);
        // next position
        mouseInteraction.m_mousePick.m_rayOrigin = AZ::Vector3::CreateAxisX(10.0f);
        mouseInteraction.m_mousePick.m_rayDirection = AZ::Vector3::CreateAxisY();
        // move with mouse button down
        m_manipulatorManager->ConsumeViewportMouseMove(mouseInteraction);

        // simulate refreshing the bound while rendering
        m_manipulatorManager->DrawManipulators(nullDebugDisplayRequests, cameraState, mouseInteraction);
        // mouse up after (ending drag)
        m_manipulatorManager->ConsumeViewportMouseRelease(mouseInteraction);
        // simulate event from Qt (immediate mouse move after mouse up)
        m_manipulatorManager->ConsumeViewportMouseMove(mouseInteraction);

        // potential bound refresh while rendering
        m_manipulatorManager->DrawManipulators(nullDebugDisplayRequests, cameraState, mouseInteraction);

        EXPECT_TRUE(manipulator->MouseOver());
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, EditorWhiteBoxComponentRespectsEntityHiddenVisibility)
    {
        // given (precondition)
        EXPECT_TRUE(m_whiteBoxComponent->HasRenderMesh());

        // when
        AzToolsFramework::SetEntityVisibility(m_whiteBoxEntityId, false);

        // then
        EXPECT_FALSE(m_whiteBoxComponent->HasRenderMesh());
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, EditorWhiteBoxComponentRespectsEntityHiddenVisibilityWhenActivated)
    {
        // given (precondition)
        EXPECT_TRUE(m_whiteBoxComponent->HasRenderMesh());
        m_whiteBoxComponent->Deactivate();

        // when
        AzToolsFramework::SetEntityVisibility(m_whiteBoxEntityId, false);

        // then
        m_whiteBoxComponent->Activate();
        EXPECT_FALSE(m_whiteBoxComponent->HasRenderMesh());
    }
} // namespace UnitTest
