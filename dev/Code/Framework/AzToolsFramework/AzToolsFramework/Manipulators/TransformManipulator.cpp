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

#include "AngularManipulator.h"
#include "LinearManipulator.h"
#include "PlanarManipulator.h"
#include "TransformManipulator.h"

namespace AzToolsFramework
{
    TransformManipulator::TransformManipulator(AZ::EntityId entityId)
    {
        m_manipulators.reserve(s_manipulatorCount);
        for (int i = s_translationLinearManipulatorBeginIndex; i < s_translationLinearManipulatorEndIndex; ++i)
        {
            m_manipulators.emplace_back(AZStd::make_unique<LinearManipulator>(entityId));
        }
        for (int i = s_translationPlanarManipulatorBeginIndex; i < s_translationPlanarManipulatorEndIndex; ++i)
        {
            m_manipulators.emplace_back(AZStd::make_unique<PlanarManipulator>(entityId));
        }
        for (int i = s_scaleLinearManipulatorBeginIndex; i < s_scaleLinearManipulatorEndIndex; ++i)
        {
            m_manipulators.emplace_back(AZStd::make_unique<LinearManipulator>(entityId));
        }
        for (int i = s_rotationAngularManipulatorBeginIndex; i < s_rotationAngularManipulatorEndIndex; ++i)
        {
            m_manipulators.emplace_back(AZStd::make_unique<AngularManipulator>(entityId));
        }
    }

    TransformManipulator::~TransformManipulator()
    {
        Unregister();
    }

    void TransformManipulator::Register(AzToolsFramework::ManipulatorManagerId manipulatorManagerId)
    {
        m_manipulatorManagerId = manipulatorManagerId;
        ManipulatorManagerNotificationBus::Handler::BusConnect(m_manipulatorManagerId);
    }

    void TransformManipulator::Unregister()
    {
        for (int i = 0; i < s_manipulatorCount; ++i)
        {
            if (m_manipulators[i] != nullptr)
            {
                m_manipulators[i]->Unregister();
            }
        }

        ManipulatorManagerNotificationBus::Handler::BusDisconnect();
    }

    void TransformManipulator::InstallTranslationLinearManipulatorLeftMouseDownCallback(LinearManipulator::MouseActionCallback onMouseDownCallback)
    {
        for (int i = s_translationLinearManipulatorBeginIndex; i < s_translationLinearManipulatorEndIndex; ++i)
        {
            if (LinearManipulator* translationLinearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                translationLinearManipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
            }
        }
    }

    void TransformManipulator::InstallTranslationLinearManipulatorMouseMoveCallback(LinearManipulator::MouseActionCallback onMouseMoveCallback)
    {
        for (int i = s_translationLinearManipulatorBeginIndex; i < s_translationLinearManipulatorEndIndex; ++i)
        {
            if (LinearManipulator* translationLinearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                translationLinearManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
            }
        }
    }

    void TransformManipulator::InstallTranslationPlanarManipulatorLeftMouseDownCallback(PlanarManipulator::MouseActionCallback onMouseDownCallback)
    {
        for (int i = s_translationPlanarManipulatorBeginIndex; i < s_translationPlanarManipulatorEndIndex; ++i)
        {
            if (PlanarManipulator* translationPlanarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[i].get()))
            {
                translationPlanarManipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
            }
        }
    }

    void TransformManipulator::InstallTranslationPlanarManipulatorMouseMoveCallback(PlanarManipulator::MouseActionCallback onMouseMoveCallback)
    {
        for (int i = s_translationPlanarManipulatorBeginIndex; i < s_translationPlanarManipulatorEndIndex; ++i)
        {
            if (PlanarManipulator* translationPlanarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[i].get()))
            {
                translationPlanarManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
            }
        }
    }

    void TransformManipulator::InstallScaleLinearManipulatorLeftMouseDownCallback(LinearManipulator::MouseActionCallback onMouseDownCallback)
    {
        for (int i = s_scaleLinearManipulatorBeginIndex; i < s_scaleLinearManipulatorEndIndex; ++i)
        {
            if (LinearManipulator* scaleLinearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                scaleLinearManipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
            }
        }
    }

    void TransformManipulator::InstallScaleLinearManipulatorMouseMoveCallback(LinearManipulator::MouseActionCallback onMouseMoveCallback)
    {
        for (int i = s_scaleLinearManipulatorBeginIndex; i < s_scaleLinearManipulatorEndIndex; ++i)
        {
            if (LinearManipulator* scaleLinearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                scaleLinearManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
            }
        }
    }

    void TransformManipulator::InstallRotationAngularManipulatorLeftMouseDownCallback(AngularManipulator::MouseActionCallback onMouseDownCallback)
    {
        for (int i = s_rotationAngularManipulatorBeginIndex; i < s_rotationAngularManipulatorEndIndex; ++i)
        {
            if (AngularManipulator* rotationAngularManipulator = azdynamic_cast<AngularManipulator*>(m_manipulators[i].get()))
            {
                rotationAngularManipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
            }
        }
    }

    void TransformManipulator::InstallRotationAngularManipulatorMouseMoveCallback(AngularManipulator::MouseActionCallback onMouseMoveCallback)
    {
        for (int i = s_rotationAngularManipulatorBeginIndex; i < s_rotationAngularManipulatorEndIndex; ++i)
        {
            if (AngularManipulator* rotationAngularManipulator = azdynamic_cast<AngularManipulator*>(m_manipulators[i].get()))
            {
                rotationAngularManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
            }
        }
    }

    void TransformManipulator::SetBoundsDirty()
    {
        for (int i = 0; i < s_manipulatorCount; ++i)
        {
            BaseManipulator* manipulator = m_manipulators[i].get();
            if (manipulator != nullptr && manipulator->Registered())
            {
                manipulator->SetBoundsDirty();
            }
        }
    }

    void TransformManipulator::SwitchManipulators(Mode mode)
    {
        for (int i = 0; i < s_manipulatorCount; ++i)
        {
            BaseManipulator* manipulator = m_manipulators[i].get();
            if (manipulator != nullptr && manipulator->Registered())
            {
                manipulator->Unregister();
            }
        }
        
        const float lineLength = 2.0f;
        const float lineWidth = 0.05f;

        switch (mode)
        {
        case Mode::Translation:
            {
                const float coneLength = 0.28f;
                const float coneRadius = 0.07f;
        
                const int translationLinearManipulatorCount = s_translationLinearManipulatorEndIndex - s_translationLinearManipulatorBeginIndex;
                AZ::Vector3 translationAxes[translationLinearManipulatorCount] = { 
                    AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ() 
                };

                AZ::Color translationAxesColors[translationLinearManipulatorCount] = {
                    AZ::Color(1.0f, 0.0f, 0.0f, 1.0f), AZ::Color(0.0f, 1.0f, 0.0f, 1.0f), AZ::Color(0.0f, 0.0f, 1.0f, 1.0f)
                };

                for (int i = s_translationLinearManipulatorBeginIndex; i < s_translationLinearManipulatorEndIndex; ++i)
                {
                    if (LinearManipulator* translationLinearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
                    {
                        translationLinearManipulator->Register(m_manipulatorManagerId);
                        translationLinearManipulator->SetPosition(AZ::Vector3::CreateZero());
                        const int axisIndex = i - s_translationLinearManipulatorBeginIndex;
                        translationLinearManipulator->SetAxis(translationAxes[axisIndex]);

                        ManipulatorViews views;
                        views.emplace_back(CreateManipulatorViewLine(
                            *translationLinearManipulator, translationAxesColors[axisIndex], lineLength, lineWidth));
                        views.emplace_back(CreateManipulatorViewCone(
                            *translationLinearManipulator, translationAxesColors[axisIndex], translationLinearManipulator->GetAxis() * (lineLength - coneLength), coneLength, coneRadius));
                        translationLinearManipulator->SetViews(AZStd::move(views));
                    }
                }

                const int translationPlanarManipulatorCount = s_translationPlanarManipulatorEndIndex - s_translationPlanarManipulatorBeginIndex;
                AZ::Color translationPlanesColors[translationPlanarManipulatorCount] = {
                    AZ::Color(1.0f, 0.0f, 0.0f, 0.5f), AZ::Color(0.0f, 1.0f, 0.0f, 0.5f), AZ::Color(0.0f, 0.0f, 1.0f, 0.5f)
                };

                const float planeSize = 0.6f;
                for (int i = s_translationPlanarManipulatorBeginIndex; i < s_translationPlanarManipulatorEndIndex; ++i)
                {
                    if (PlanarManipulator* translationPlanarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[i].get()))
                    {
                        translationPlanarManipulator->Register(m_manipulatorManagerId);
                        translationPlanarManipulator->SetPosition(AZ::Vector3::CreateZero());
                        const int planeAxis1Index = (i - s_translationPlanarManipulatorBeginIndex) % translationLinearManipulatorCount;
                        const int planeAxis2Index = (i - s_translationPlanarManipulatorBeginIndex + 1) % translationLinearManipulatorCount;
                        translationPlanarManipulator->SetAxes(translationAxes[planeAxis1Index], translationAxes[planeAxis2Index]);
                        const int colorIndex = i - s_translationPlanarManipulatorBeginIndex;

                        translationPlanarManipulator->SetView(
                            CreateManipulatorViewQuad(*translationPlanarManipulator, translationPlanesColors[colorIndex], translationPlanesColors[colorIndex], planeSize));
                    }
                }
            }
            break;
        case Mode::Scale:
            {
                const int scaleLinearManipulatorCount =  s_scaleLinearManipulatorEndIndex - s_scaleLinearManipulatorBeginIndex;
                AZ::Vector3 scaleAxes[scaleLinearManipulatorCount] = {
                    AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ()
                };

                AZ::Color scaleAxesColors[scaleLinearManipulatorCount] = {
                    AZ::Color(1.0f, 0.0f, 0.0f, 1.0f), AZ::Color(0.0f, 1.0f, 0.0f, 1.0f), AZ::Color(0.0f, 0.0f, 1.0f, 1.0f)
                };

                const float boxSize = 0.1f;
                for (int i = s_scaleLinearManipulatorBeginIndex; i < s_scaleLinearManipulatorEndIndex; ++i)
                {
                    if (LinearManipulator* scaleLinearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
                    {
                        scaleLinearManipulator->Register(m_manipulatorManagerId);
                        scaleLinearManipulator->SetPosition(AZ::Vector3::CreateZero());
                        const int axisIndex = i - s_scaleLinearManipulatorBeginIndex;
                        scaleLinearManipulator->SetAxis(scaleAxes[axisIndex]);

                        ManipulatorViews views;
                        views.emplace_back(CreateManipulatorViewLine(*scaleLinearManipulator, scaleAxesColors[axisIndex], lineLength, lineWidth));
                        views.emplace_back(CreateManipulatorViewBox(AZ::Transform::CreateIdentity(), scaleAxesColors[axisIndex], scaleLinearManipulator->GetAxis() * (lineLength - boxSize), AZ::Vector3(boxSize, boxSize, boxSize)));
                        scaleLinearManipulator->SetViews(AZStd::move(views));
                    }
                }
            } 
            break;
        case Mode::Rotation:
            {
                const int rotationAngularManipulatorCount = s_rotationAngularManipulatorEndIndex - s_rotationAngularManipulatorBeginIndex;
                AZ::Vector3 rotationAxes[rotationAngularManipulatorCount] = {
                    AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ()
                };

                AZ::Color rotationCircleColors[rotationAngularManipulatorCount] = {
                    AZ::Color(1.0f, 0.0f, 0.0f, 1.0f), AZ::Color(0.0f, 1.0f, 0.0f, 1.0f), AZ::Color(0.0f, 0.0f, 1.0f, 1.0f)
                };

                for (int i = s_rotationAngularManipulatorBeginIndex; i < s_rotationAngularManipulatorEndIndex; ++i)
                {
                    if (AngularManipulator* rotationManipulator = azdynamic_cast<AngularManipulator*>(m_manipulators[i].get()))
                    {
                        size_t rotationAxisIndex = i - s_rotationAngularManipulatorBeginIndex;

                        rotationManipulator->Register(m_manipulatorManagerId);
                        rotationManipulator->SetTransform(AZ::Transform::CreateIdentity());
                        rotationManipulator->SetAxis(rotationAxes[rotationAxisIndex]);

                        rotationManipulator->SetView(
                            CreateManipulatorViewCircle(
                                *rotationManipulator, rotationCircleColors[rotationAxisIndex],
                                lineLength, lineWidth));
                    }
                }
            } 
            break;
        }
    }
} // namespace AzToolsFramework