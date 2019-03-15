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

#include "ScaleManipulators.h"

namespace AzToolsFramework
{
    ScaleManipulators::ScaleManipulators(AZ::EntityId entityId, const AZ::Transform& worldFromLocal)
    {
        m_axisScaleManipulators.reserve(3);
        for (size_t i = 0; i < 3; ++i)
        {
            m_axisScaleManipulators.emplace_back(AZStd::make_unique<LinearManipulator>(entityId, worldFromLocal));
        }

        m_uniformScaleManipulator = AZStd::make_unique<LinearManipulator>(entityId, worldFromLocal);
    }

    void ScaleManipulators::InstallAxisLeftMouseDownCallback(
        const LinearManipulator::MouseActionCallback& onMouseDownCallback)
    {
        for (AZStd::unique_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
        }
    }

    void ScaleManipulators::InstallAxisMouseMoveCallback(
        const LinearManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        for (AZStd::unique_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->InstallMouseMoveCallback(onMouseMoveCallback);
        }
    }

    void ScaleManipulators::InstallAxisLeftMouseUpCallback(
        const LinearManipulator::MouseActionCallback& onMouseUpCallback)
    {
        for (AZStd::unique_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
        }
    }

    void ScaleManipulators::InstallUniformLeftMouseDownCallback(
        const LinearManipulator::MouseActionCallback& onMouseDownCallback)
    {
        m_uniformScaleManipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
    }

    void ScaleManipulators::InstallUniformMouseMoveCallback(
        const LinearManipulator::MouseActionCallback& onMouseMoveCallback)
    {
         m_uniformScaleManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
    }

    void ScaleManipulators::InstallUniformLeftMouseUpCallback(
        const LinearManipulator::MouseActionCallback& onMouseUpCallback)
    {
         m_uniformScaleManipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
    }

    void ScaleManipulators::SetLocalTransform(const AZ::Transform& localTransform)
    {
        for (AZStd::unique_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->SetLocalTransform(localTransform);
        }

        m_uniformScaleManipulator->SetLocalTransform(localTransform);
    }

    void ScaleManipulators::SetSpace(const AZ::Transform& worldFromLocal)
    {
        for (AZStd::unique_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulator->SetSpace(worldFromLocal);
        }

        m_uniformScaleManipulator->SetSpace(worldFromLocal);
    }

    void ScaleManipulators::SetAxes(
        const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3)
    {
         AZ::Vector3 axes[] = { axis1, axis2, axis3 };

        for (size_t i = 0; i < m_axisScaleManipulators.size(); ++i)
        {
           m_axisScaleManipulators[i]->SetAxis(axes[i]);
        }

        // uniform scale manipulator uses Z axis for scaling (always in world space)
        m_uniformScaleManipulator->SetAxis(AZ::Vector3::CreateAxisZ());
        m_uniformScaleManipulator->OverrideInteractiveManipulatorSpace([](){ return ManipulatorSpace::World; });
    }

    void ScaleManipulators::ConfigureView(
        const float axisLength, const AZ::Color& axis1Color,
        const AZ::Color& axis2Color, const AZ::Color& axis3Color)
    {
        const float boxSize = 0.1f;
        const float lineWidth = 0.05f;

        const AZ::Color colors[] = {
            axis1Color, axis2Color, axis3Color
        };

        for (size_t i = 0; i < m_axisScaleManipulators.size(); ++i)
        {
            ManipulatorViews views;
            views.emplace_back(CreateManipulatorViewLine(
                *m_axisScaleManipulators[i], colors[i], axisLength, lineWidth));
            views.emplace_back(CreateManipulatorViewBox(
                AZ::Transform::CreateIdentity(), colors[i],
                m_axisScaleManipulators[i]->GetAxis() * (axisLength - boxSize),
                AZ::Vector3(boxSize)));
            m_axisScaleManipulators[i]->SetViews(AZStd::move(views));
        }

        ManipulatorViews views;
        views.emplace_back(CreateManipulatorViewBox(
            AZ::Transform::CreateIdentity(), AZ::Color::CreateOne(),
            AZ::Vector3::CreateZero(), AZ::Vector3(boxSize)));
        m_uniformScaleManipulator->SetViews(AZStd::move(views));
    }

    void ScaleManipulators::ProcessManipulators(const AZStd::function<void(BaseManipulator*)>& manipulatorFn)
    {
        for (AZStd::unique_ptr<LinearManipulator>& manipulator : m_axisScaleManipulators)
        {
            manipulatorFn(manipulator.get());
        }

        manipulatorFn(m_uniformScaleManipulator.get());
    }
}