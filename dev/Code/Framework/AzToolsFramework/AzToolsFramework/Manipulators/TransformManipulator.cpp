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

        TransformManipulatorMode currentMode = TransformManipulatorMode::Translation;
        ManipulatorManagerRequestBus::EventResult(currentMode, m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::GetTransformManipulatorMode);

        SwitchManipulators(currentMode);
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

    void TransformManipulator::OnTransformManipulatorModeChanged(TransformManipulatorMode /*previousMode*/, TransformManipulatorMode currentMode)
    {
        SwitchManipulators(currentMode);
    }

    void TransformManipulator::InstallTranslationLinearManipulatorMouseDownCallback(LinearManipulatorMouseActionCallback onMouseDownCallback)
    {
        for (int i = s_translationLinearManipulatorBeginIndex; i < s_translationLinearManipulatorEndIndex; ++i)
        {
            if (LinearManipulator* translationLinearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                translationLinearManipulator->InstallMouseDownCallback(onMouseDownCallback);
            }
        }
    }

    void TransformManipulator::InstallTranslationLinearManipulatorMouseMoveCallback(LinearManipulatorMouseActionCallback onMouseMoveCallback)
    {
        for (int i = s_translationLinearManipulatorBeginIndex; i < s_translationLinearManipulatorEndIndex; ++i)
        {
            if (LinearManipulator* translationLinearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                translationLinearManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
            }
        }
    }

    void TransformManipulator::InstallTranslationPlanarManipulatorMouseDownCallback(PlanarManipulatorMouseActionCallback onMouseDownCallback)
    {
        for (int i = s_translationPlanarManipulatorBeginIndex; i < s_translationPlanarManipulatorEndIndex; ++i)
        {
            if (PlanarManipulator* translationPlanarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[i].get()))
            {
                translationPlanarManipulator->InstallMouseDownCallback(onMouseDownCallback);
            }
        }
    }

    void TransformManipulator::InstallTranslationPlanarManipulatorMouseMoveCallback(PlanarManipulatorMouseActionCallback onMouseMoveCallback)
    {
        for (int i = s_translationPlanarManipulatorBeginIndex; i < s_translationPlanarManipulatorEndIndex; ++i)
        {
            if (PlanarManipulator* translationPlanarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[i].get()))
            {
                translationPlanarManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
            }
        }
    }

    void TransformManipulator::InstallScaleLinearManipulatorMouseDownCallback(LinearManipulatorMouseActionCallback onMouseDownCallback)
    {
        for (int i = s_scaleLinearManipulatorBeginIndex; i < s_scaleLinearManipulatorEndIndex; ++i)
        {
            if (LinearManipulator* scaleLinearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                scaleLinearManipulator->InstallMouseDownCallback(onMouseDownCallback);
            }
        }
    }

    void TransformManipulator::InstallScaleLinearManipulatorMouseMoveCallback(LinearManipulatorMouseActionCallback onMouseMoveCallback)
    {
        for (int i = s_scaleLinearManipulatorBeginIndex; i < s_scaleLinearManipulatorEndIndex; ++i)
        {
            if (LinearManipulator* scaleLinearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                scaleLinearManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
            }
        }
    }

    void TransformManipulator::InstallRotationAngularManipulatorMouseDownCallback(AngularManipulatorMouseActionCallback onMouseDownCallback)
    {
        for (int i = s_rotationAngularManipulatorBeginIndex; i < s_rotationAngularManipulatorEndIndex; ++i)
        {
            if (AngularManipulator* rotationAngularManipulator = azdynamic_cast<AngularManipulator*>(m_manipulators[i].get()))
            {
                rotationAngularManipulator->InstallMouseDownCallback(onMouseDownCallback);
            }
        }
    }

    void TransformManipulator::InstallRotationAngularManipulatorMouseMoveCallback(AngularManipulatorMouseActionCallback onMouseMoveCallback)
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
            if (manipulator != nullptr && manipulator->IsRegistered())
            {
                manipulator->SetBoundsDirty();
            }
        }
    }

    void TransformManipulator::SwitchManipulators(TransformManipulatorMode mode)
    {
        for (int i = 0; i < s_manipulatorCount; ++i)
        {
            BaseManipulator* manipulator = m_manipulators[i].get();
            if (manipulator != nullptr && manipulator->IsRegistered())
            {
                manipulator->Unregister();
            }
        }

        switch (mode)
        {
            case TransformManipulatorMode::None:
            {
                return;
            } break;

            case TransformManipulatorMode::Translation:
            {
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
                        int axisIndex = i - s_translationLinearManipulatorBeginIndex;
                        int colorIndex = axisIndex;
                        translationLinearManipulator->SetDirection(translationAxes[axisIndex]);
                        translationLinearManipulator->SetColor(translationAxesColors[colorIndex]);
                        translationLinearManipulator->SetLength(130.0f);
                    }
                }

                const int translationPlanarManipulatorCount = s_translationPlanarManipulatorEndIndex - s_translationPlanarManipulatorBeginIndex;
                AZ::Color translationPlanesColors[translationPlanarManipulatorCount] = {
                    AZ::Color(1.0f, 0.0f, 0.0f, 0.5f), AZ::Color(0.0f, 1.0f, 0.0f, 0.5f), AZ::Color(0.0f, 0.0f, 1.0f, 0.5f)
                };

                for (int i = s_translationPlanarManipulatorBeginIndex; i < s_translationPlanarManipulatorEndIndex; ++i)
                {
                    if (PlanarManipulator* translationPlanarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[i].get()))
                    {
                        translationPlanarManipulator->Register(m_manipulatorManagerId);
                        translationPlanarManipulator->SetPosition(AZ::Vector3::CreateZero());
                        int planeAxis1Index = (i - s_translationPlanarManipulatorBeginIndex) % translationLinearManipulatorCount;
                        int planeAxis2Index = (i - s_translationPlanarManipulatorBeginIndex + 1) % translationLinearManipulatorCount;
                        translationPlanarManipulator->SetAxes(translationAxes[planeAxis1Index], translationAxes[planeAxis2Index]);
                        int colorIndex = i - s_translationPlanarManipulatorBeginIndex;
                        translationPlanarManipulator->SetColor(translationPlanesColors[colorIndex]);
                        translationPlanarManipulator->SetAxesLength(70.0f, 70.0f);
                    }
                }

            } break;

            case TransformManipulatorMode::Scale:
            {
                const int scaleLinearManipulatorCount =  s_scaleLinearManipulatorEndIndex - s_scaleLinearManipulatorBeginIndex;
                AZ::Vector3 scaleAxes[scaleLinearManipulatorCount] = {
                    AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ()
                };

                AZ::Color scaleAxesColors[scaleLinearManipulatorCount] = {
                    AZ::Color(1.0f, 0.0f, 0.0f, 1.0f), AZ::Color(0.0f, 1.0f, 0.0f, 1.0f), AZ::Color(0.0f, 0.0f, 1.0f, 1.0f)
                };

                for (int i = s_scaleLinearManipulatorBeginIndex; i < s_scaleLinearManipulatorEndIndex; ++i)
                {
                    if (LinearManipulator* scaleLinearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
                    {
                        scaleLinearManipulator->Register(m_manipulatorManagerId);
                        scaleLinearManipulator->SetPosition(AZ::Vector3::CreateZero());
                        int axisIndex = i - s_scaleLinearManipulatorBeginIndex;
                        int colorIndex = axisIndex;
                        scaleLinearManipulator->SetDirection(scaleAxes[axisIndex]);
                        scaleLinearManipulator->SetColor(scaleAxesColors[colorIndex]);
                        scaleLinearManipulator->SetLength(130.0f);

                        scaleLinearManipulator->SetDisplayType(LinearManipulator::DisplayType::Cube);
                        int upAxisIndex = (axisIndex + 1) % scaleLinearManipulatorCount;
                        int sideAxisIndex = (axisIndex + 2) % scaleLinearManipulatorCount;
                        scaleLinearManipulator->SetUpDirection(scaleAxes[upAxisIndex]);
                        scaleLinearManipulator->SetSideReferenceDirection(scaleAxes[sideAxisIndex]);
                    }
                }
            } break;

            case TransformManipulatorMode::Rotation:
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
                        rotationManipulator->Register(m_manipulatorManagerId);
                        rotationManipulator->SetPosition(AZ::Vector3::CreateZero());
                        int rotationAxisIndex = i - s_rotationAngularManipulatorBeginIndex;
                        int colorIndex = rotationAxisIndex;
                        rotationManipulator->SetRotationAxis(rotationAxes[rotationAxisIndex]);
                        rotationManipulator->SetRadius(180.0f);
                        rotationManipulator->SetColor(rotationCircleColors[colorIndex]);
                    }
                }
            } break;
        }
    }
} // namespace AzToolsFramework