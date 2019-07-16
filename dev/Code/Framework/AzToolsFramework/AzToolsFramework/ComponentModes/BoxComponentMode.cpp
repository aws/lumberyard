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

#include "BoxComponentMode.h"

#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(BoxComponentMode, AZ::SystemAllocator, 0)

    // manipulator visual properties
    static const float s_manipulatorHandleSize = 0.05f;
    static const AZ::Color s_manipulatorHandleColor = AZ::Color(0.06275f, 0.1647f, 0.1647f, 1.0f);

    const AZStd::array<AZ::Vector3, 6> s_boxAxes =
    { {
        AZ::Vector3::CreateAxisX(), -AZ::Vector3::CreateAxisX(),
        AZ::Vector3::CreateAxisY(), -AZ::Vector3::CreateAxisY(),
        AZ::Vector3::CreateAxisZ(), -AZ::Vector3::CreateAxisZ()
    } };

    /// Pass a single axis, and return not of elements
    /// Example: In -> (1, 0, 0) Out -> (0, 1, 1)
    static AZ::Vector3 NotAxis(const AZ::Vector3& offset)
    {
        return AZ::Vector3::CreateOne() - AZ::Vector3(
            fabsf(Sign(offset.GetX())),
            fabsf(Sign(offset.GetY())),
            fabsf(Sign(offset.GetZ())));
    }

    BoxComponentMode::BoxComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        BoxManipulatorRequestBus::EventResult(
            worldFromLocal, entityComponentIdPair, &BoxManipulatorRequests::GetCurrentTransform);

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_linearManipulators.size(); ++manipulatorIndex)
        {
            auto& linearManipulator = m_linearManipulators[manipulatorIndex];

            if (linearManipulator == nullptr)
            {
                linearManipulator = LinearManipulator::MakeShared(worldFromLocal);

                linearManipulator->AddEntityId(entityComponentIdPair.GetEntityId());
                linearManipulator->SetAxis(s_boxAxes[manipulatorIndex]);

                ManipulatorViews views;
                views.emplace_back(CreateManipulatorViewQuadBillboard(
                    s_manipulatorHandleColor, s_manipulatorHandleSize));
                linearManipulator->SetViews(AZStd::move(views));

                linearManipulator->InstallMouseMoveCallback([this, entityComponentIdPair](
                    const LinearManipulator::Action& action)
                {
                    // calculate the amount of displacement along an axis this manipulator has moved
                    // clamp movement so it cannot go negative based on axis direction
                    const AZ::Vector3 axisDisplacement =
                        action.LocalPosition().GetAbs() * 2.0f
                        * action.LocalPosition().GetNormalized().Dot(action.m_fixed.m_axis).GetMax(AZ::VectorFloat::CreateZero());

                    AZ::Vector3 boxScale = AZ::Vector3::CreateOne();
                    BoxManipulatorRequestBus::EventResult(
                        boxScale, entityComponentIdPair, &BoxManipulatorRequests::GetBoxScale);

                    AZ::Vector3 boxDimensions = AZ::Vector3::CreateZero();
                    BoxManipulatorRequestBus::EventResult(
                        boxDimensions, entityComponentIdPair, &BoxManipulatorRequests::GetDimensions);

                    // ensure we take into account the entity scale using the axis displacement
                    const AZ::Vector3 scaledAxisDisplacement =
                        axisDisplacement / boxScale;

                    // update dimensions - preserve dimensions not effected by this
                    // axis, and update current axis displacement
                    BoxManipulatorRequestBus::Event(
                        entityComponentIdPair, &BoxManipulatorRequests::SetDimensions,
                        (NotAxis(action.m_fixed.m_axis) * boxDimensions).GetMax(scaledAxisDisplacement));

                    Refresh();

                    ToolsApplicationNotificationBus::Broadcast(
                        &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay,
                        Refresh_Values);
                });
            }

            linearManipulator->Register(g_mainManipulatorManagerId);
        }

        UpdateManipulators();
    }

    BoxComponentMode::~BoxComponentMode()
    {
        for (auto& linearManipulator : m_linearManipulators)
        {
            if (linearManipulator)
            {
                linearManipulator->Unregister();
                linearManipulator.reset();
            }
        }
    }

    void BoxComponentMode::UpdateManipulators()
    {
        AZ::Transform boxWorldFromLocal = AZ::Transform::CreateIdentity();
        BoxManipulatorRequestBus::EventResult(
            boxWorldFromLocal, GetComponentEntityIdPair(), &BoxManipulatorRequests::GetCurrentTransform);

        AZ::Vector3 boxScale = AZ::Vector3::CreateOne();
        BoxManipulatorRequestBus::EventResult(
            boxScale, GetComponentEntityIdPair(), &BoxManipulatorRequests::GetBoxScale);

        AZ::Vector3 boxDimensions = AZ::Vector3::CreateZero();
        BoxManipulatorRequestBus::EventResult(
            boxDimensions, GetComponentEntityIdPair(), &BoxManipulatorRequests::GetDimensions);

        // ensure we apply the entity scale to the box dimensions so
        // the manipulators appear in the correct location
        boxDimensions *= boxScale;

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_linearManipulators.size(); ++manipulatorIndex)
        {
            if (auto& linearManipulator = m_linearManipulators[manipulatorIndex])
            {
                linearManipulator->SetSpace(boxWorldFromLocal);
                linearManipulator->SetLocalTransform(
                    AZ::Transform::CreateTranslation(s_boxAxes[manipulatorIndex] * AZ::VectorFloat(0.5f) * boxDimensions));
                linearManipulator->SetBoundsDirty();
            }
        }
    }

    void BoxComponentMode::Refresh()
    {
        UpdateManipulators();
    }
} // namespace AzToolsFramework