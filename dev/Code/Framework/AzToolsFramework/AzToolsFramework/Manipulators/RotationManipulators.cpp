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

#include "RotationManipulators.h"

#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    RotationManipulators::RotationManipulators(AZ::EntityId entityId, const AZ::Transform& worldFromLocal)
    {
        m_angularManipulators.reserve(3);
        for (size_t i = 0; i < 3; ++i)
        {
            m_angularManipulators.emplace_back(AZStd::make_unique<AngularManipulator>(entityId, worldFromLocal));
        }
    }

    void RotationManipulators::InstallLeftMouseDownCallback(
        const AngularManipulator::MouseActionCallback& onMouseDownCallback)
    {
        for (AZStd::unique_ptr<AngularManipulator>& manipulator : m_angularManipulators)
        {
            manipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
        }
    }

    void RotationManipulators::InstallMouseMoveCallback(
        const AngularManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        for (AZStd::unique_ptr<AngularManipulator>& manipulator : m_angularManipulators)
        {
            manipulator->InstallMouseMoveCallback(onMouseMoveCallback);
        }
    }

    void RotationManipulators::InstallLeftMouseUpCallback(
        const AngularManipulator::MouseActionCallback& onMouseUpCallback)
    {
        for (AZStd::unique_ptr<AngularManipulator>& manipulator : m_angularManipulators)
        {
            manipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
        }
    }

    void RotationManipulators::SetLocalTransform(const AZ::Transform& localTransform)
    {
        for (AZStd::unique_ptr<AngularManipulator>& manipulator : m_angularManipulators)
        {
            manipulator->SetLocalTransform(localTransform);
        }
    }

    void RotationManipulators::SetSpace(const AZ::Transform& worldFromLocal)
    {
        for (AZStd::unique_ptr<AngularManipulator>& manipulator : m_angularManipulators)
        {
            manipulator->SetSpace(worldFromLocal);
        }
    }

    void RotationManipulators::SetAxes(
        const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3)
    {
         AZ::Vector3 axes[] = { axis1, axis2, axis3 };

        for (size_t i = 0; i < m_angularManipulators.size(); ++i)
        {
           m_angularManipulators[i]->SetAxis(axes[i]);
        }
    }

    void RotationManipulators::ConfigureView(
        const float radius, const AZ::Color& axis1Color,
        const AZ::Color& axis2Color, const AZ::Color& axis3Color)
    {
        const AZ::Color colors[] = {
            axis1Color, axis2Color, axis3Color
        };

        for (size_t i = 0; i < m_angularManipulators.size(); ++i)
        {
            m_angularManipulators[i]->SetView(
                CreateManipulatorViewCircle(
                    *m_angularManipulators[i], colors[i],
                    radius, 0.05f));
        }
    }

    void RotationManipulators::ProcessManipulators(const AZStd::function<void(BaseManipulator*)>& manipulatorFn)
    {
        for (AZStd::unique_ptr<AngularManipulator>& manipulator : m_angularManipulators)
        {
            manipulatorFn(manipulator.get());
        }
    }
}