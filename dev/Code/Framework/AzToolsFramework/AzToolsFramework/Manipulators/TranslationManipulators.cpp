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

#include "TranslationManipulators.h"

#include <AzCore/Math/VectorConversions.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    TranslationManipulators::TranslationManipulators(
        const AZ::EntityId entityId, const Dimensions dimensions, const AZ::Transform& worldFromLocal)
        : m_dimensions(dimensions)
    {
        switch (dimensions)
        {
        case Dimensions::Two:
            m_linearManipulators.reserve(2);
            for (size_t i = 0; i < 2; ++i)
            {
                m_linearManipulators.emplace_back(AZStd::make_shared<LinearManipulator>(entityId, worldFromLocal));
            }
            m_planarManipulators.emplace_back(AZStd::make_shared<PlanarManipulator>(entityId, worldFromLocal));
            break;
        case Dimensions::Three:
            m_linearManipulators.reserve(3);
            m_planarManipulators.reserve(3);
            for (size_t i = 0; i < 3; ++i)
            {
                m_linearManipulators.emplace_back(AZStd::make_shared<LinearManipulator>(entityId, worldFromLocal));
                m_planarManipulators.emplace_back(AZStd::make_shared<PlanarManipulator>(entityId, worldFromLocal));
            }
            m_surfaceManipulator = AZStd::make_shared<SurfaceManipulator>(entityId, worldFromLocal);
            break;
        default:
            AZ_Assert(false, "Invalid dimensions provided");
            break;
        }
    }

    void TranslationManipulators::InstallLinearManipulatorMouseDownCallback(
        const LinearManipulator::MouseActionCallback& onMouseDownCallback)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
        }
    }

    void TranslationManipulators::InstallLinearManipulatorMouseMoveCallback(
        const LinearManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->InstallMouseMoveCallback(onMouseMoveCallback);
        }
    }

    void TranslationManipulators::InstallLinearManipulatorMouseUpCallback(
        const LinearManipulator::MouseActionCallback& onMouseUpCallback)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
        }
    }

    void TranslationManipulators::InstallPlanarManipulatorMouseDownCallback(
        const PlanarManipulator::MouseActionCallback& onMouseDownCallback)
    {
        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
        }
    }

    void TranslationManipulators::InstallPlanarManipulatorMouseMoveCallback(
        const PlanarManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->InstallMouseMoveCallback(onMouseMoveCallback);
        }
    }

    void TranslationManipulators::InstallPlanarManipulatorMouseUpCallback(
        const PlanarManipulator::MouseActionCallback& onMouseUpCallback)
    {
        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
        }
    }

    void TranslationManipulators::InstallSurfaceManipulatorMouseDownCallback(
        const SurfaceManipulator::MouseActionCallback& onMouseDownCallback)
    {
        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
        }
    }

    void TranslationManipulators::InstallSurfaceManipulatorMouseUpCallback(
        const SurfaceManipulator::MouseActionCallback& onMouseUpCallback)
    {
        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
        }
    }

    void TranslationManipulators::InstallSurfaceManipulatorMouseMoveCallback(
        const SurfaceManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
        }
    }

    void TranslationManipulators::SetLocalTransform(const AZ::Transform& localTransform)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->SetLocalTransform(localTransform);
        }

        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->SetLocalTransform(localTransform);
        }

        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->SetPosition(localTransform.GetTranslation());
        }

        m_position = localTransform.GetTranslation();
    }

    void TranslationManipulators::SetSpace(const AZ::Transform& worldFromLocal)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->SetSpace(worldFromLocal);
        }

        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->SetSpace(worldFromLocal);
        }

        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->SetSpace(worldFromLocal);
        }
    }

    void TranslationManipulators::SetAxes(
        const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3 /*= AZ::Vector3::CreateAxisZ()*/)
    {
        AZ::Vector3 axes[] = { axis1, axis2, axis3 };

        for (size_t i = 0; i < m_linearManipulators.size(); ++i)
        {
            m_linearManipulators[i]->SetAxis(axes[i]);
        }

        for (size_t i = 0; i < m_planarManipulators.size(); ++i)
        {
            m_planarManipulators[i]->SetAxes(axes[i], axes[(i + 1) % 3]);
        }
    }

    void TranslationManipulators::ConfigureLinearView(
        float axisLength, const AZ::Color& axis1Color, const AZ::Color& axis2Color,
        const AZ::Color& axis3Color /*= AZ::Color(0.0f, 0.0f, 1.0f, 0.5f)*/)
    {
        const float coneLength = 0.28f;
        const float coneRadius = 0.07f;
        const float lineWidth = 0.05f;

        const AZ::Color axesColor[] = { axis1Color, axis2Color, axis3Color };

        const auto configureLinearView = [lineWidth, coneLength, axisLength, coneRadius](
            LinearManipulator* linearManipulator, const AZ::Color& color)
        {
            ManipulatorViews views;
            views.emplace_back(CreateManipulatorViewLine(
                *linearManipulator, color, axisLength, lineWidth));
            views.emplace_back(CreateManipulatorViewCone(
                *linearManipulator, color, linearManipulator->GetAxis() * (axisLength - coneLength),
                coneLength, coneRadius));
            linearManipulator->SetViews(AZStd::move(views));
        };

        for (size_t i = 0; i < m_linearManipulators.size(); ++i)
        {
            configureLinearView(m_linearManipulators[i].get(), axesColor[i]);
        }
    }

    void TranslationManipulators::ConfigurePlanarView(
        const AZ::Color& plane1Color, const AZ::Color& plane2Color /*= AZ::Color(0.0f, 1.0f, 0.0f, 0.5f)*/,
        const AZ::Color& plane3Color /*= AZ::Color(0.0f, 0.0f, 1.0f, 0.5f)*/)
    {
        const float planeSize = 0.6f;
        const AZ::Color planesColor[] = { plane1Color, plane2Color, plane3Color };

        for (size_t i = 0; i < m_planarManipulators.size(); ++i)
        {
            m_planarManipulators[i]->SetView(
                CreateManipulatorViewQuad(
                    *m_planarManipulators[i],planesColor[i],
                    planesColor[(i + 1) % 3],
                    planeSize)
            );
        }
    }

    void TranslationManipulators::ConfigureSurfaceView(
        const float radius, const AZ::Color& color)
    {
        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->SetView(CreateManipulatorViewSphere(color, radius,
                [](const ViewportInteraction::MouseInteraction& /*mouseInteraction*/,
                    bool mouseOver, const AZ::Color& defaultColor) -> AZ::Color
            {
                const AZ::Color color[2] =
                {
                    defaultColor, Vector3ToVector4(BaseManipulator::s_defaultMouseOverColor.GetAsVector3(), 0.75f)
                };

                return color[mouseOver];
            }));
        }
    }

    void TranslationManipulators::ProcessManipulators(const AZStd::function<void(BaseManipulator*)>& manipulatorFn)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulatorFn(manipulator.get());
        }

        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulatorFn(manipulator.get());
        }

        if (m_surfaceManipulator)
        {
            manipulatorFn(m_surfaceManipulator.get());
        }
    }
} // namespace AzToolsFramework