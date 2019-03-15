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

#include "BoxManipulators.h"
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>

namespace AzToolsFramework
{
    const static AZStd::array<AZ::Vector3, 6> s_boxAxes =
    { {
        AZ::Vector3::CreateAxisX(), -AZ::Vector3::CreateAxisX(),
        AZ::Vector3::CreateAxisY(), -AZ::Vector3::CreateAxisY(),
        AZ::Vector3::CreateAxisZ(), -AZ::Vector3::CreateAxisZ()
    } };


    /**
    * Pass a single axis, and return not of elements
    * Example: In -> (1, 0, 0) Out -> (0, 1, 1)
    */
    static AZ::Vector3 NotAxis(const AZ::Vector3& offset)
    {
        return AZ::Vector3::CreateOne() - AZ::Vector3(
            fabsf(AzToolsFramework::Sign(offset.GetX())),
            fabsf(AzToolsFramework::Sign(offset.GetY())),
            fabsf(AzToolsFramework::Sign(offset.GetZ())));
    }

    void BoxManipulator::OnSelect(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        CreateManipulators();
    }

    void BoxManipulator::OnDeselect()
    {
        DestroyManipulators();
    }

    void BoxManipulator::SetHandler(BoxManipulatorHandler* handler)
    {
        m_handler = handler;
    }

    void BoxManipulator::RefreshManipulators()
    {
        UpdateManipulators();
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_Values);
    }

    void BoxManipulator::CreateManipulators()
    {
        bool selected = false;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selected, &AzToolsFramework::ToolsApplicationRequests::IsSelected, m_entityId);

        if (!selected || !m_handler)
        {
            return;
        }

        const AzToolsFramework::ManipulatorManagerId manipulatorManagerId = AzToolsFramework::ManipulatorManagerId(1);
        for (size_t i = 0; i < m_linearManipulators.size(); ++i)
        {
            AZStd::shared_ptr<AzToolsFramework::LinearManipulator>& linearManipulator = m_linearManipulators[i];

            if (linearManipulator == nullptr)
            {
                linearManipulator = AZStd::make_shared<AzToolsFramework::LinearManipulator>(
                    m_entityId, m_handler->GetCurrentTransform());
                linearManipulator->SetAxis(s_boxAxes[i]);

                AzToolsFramework::ManipulatorViews views;
                views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(
                    AZ::Color(0.06275f, 0.1647f, 0.1647f, 1.0f), 0.05f));
                linearManipulator->SetViews(AZStd::move(views));

                linearManipulator->InstallMouseMoveCallback([this](
                    const AzToolsFramework::LinearManipulator::Action& action)
                {
                    OnMouseMoveManipulator(action);
                });
            }

            linearManipulator->Register(manipulatorManagerId);
        }

        UpdateManipulators();
    }

    void BoxManipulator::DestroyManipulators()
    {
        for (AZStd::shared_ptr<AzToolsFramework::LinearManipulator>& linearManipulator : m_linearManipulators)
        {
            if (linearManipulator)
            {
                linearManipulator->Unregister();
                linearManipulator.reset();
            }
        }
    }

    void BoxManipulator::UpdateManipulators()
    {
        const AZ::Vector3 boxDimensions = m_handler->GetDimensions();
        for (size_t i = 0; i < m_linearManipulators.size(); ++i)
        {
            if (AZStd::shared_ptr<AzToolsFramework::LinearManipulator>& linearManipulator = m_linearManipulators[i])
            {
                linearManipulator->SetSpace(m_handler->GetCurrentTransform());
                linearManipulator->SetLocalTransform(
                    AZ::Transform::CreateTranslation(s_boxAxes[i] * AZ::VectorFloat(0.5f) * boxDimensions));
                linearManipulator->SetBoundsDirty();
            }
        }
    }

    void BoxManipulator::OnMouseMoveManipulator(
        const AzToolsFramework::LinearManipulator::Action& action)
    {
        // calculate the amount of displacement along an axis this manipulator has moved
        // clamp movement so it cannot go negative based on axis direction
        const AZ::Vector3 axisDisplacement =
            action.LocalPosition().GetAbs() * 2.0f
                * action.LocalPosition().GetNormalized().Dot(action.m_fixed.m_axis).GetMax(AZ::VectorFloat::CreateZero());

        // update dimensions - preserve dimensions not effected by this
        // axis, and update current axis displacement
        m_handler->SetDimensions((NotAxis(action.m_fixed.m_axis) *
            m_handler->GetDimensions()).GetMax(axisDisplacement));
        
        RefreshManipulators();
    }
}