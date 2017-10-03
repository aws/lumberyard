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

#include "TranslationManipulator.h"
#include "LinearManipulator.h"
#include "PlanarManipulator.h"

namespace AzToolsFramework
{
    TranslationManipulator::TranslationManipulator(AZ::EntityId entityId, Dimensions dimensions)
        : m_manipulatorIndices(dimensions)
        , m_dimensions(dimensions)
    {
        switch (dimensions)
        {
        case Dimensions::Two:
            m_manipulators.reserve(3);
            m_manipulators.emplace_back(aznew LinearManipulator(entityId));
            m_manipulators.emplace_back(aznew LinearManipulator(entityId));
            m_manipulators.emplace_back(aznew PlanarManipulator(entityId));
            break;
        case Dimensions::Three:
            m_manipulators.reserve(6);
            m_manipulators.emplace_back(aznew LinearManipulator(entityId));
            m_manipulators.emplace_back(aznew LinearManipulator(entityId));
            m_manipulators.emplace_back(aznew LinearManipulator(entityId));
            m_manipulators.emplace_back(aznew PlanarManipulator(entityId));
            m_manipulators.emplace_back(aznew PlanarManipulator(entityId));
            m_manipulators.emplace_back(aznew PlanarManipulator(entityId));
            break;
        default:
            AZ_Assert(false, "Invalid dimensions provided");
            break;
        }
    }

    TranslationManipulator::~TranslationManipulator()
    {
        for (AZStd::unique_ptr<BaseManipulator>& manipulatorPtr : m_manipulators)
        {
            if (manipulatorPtr->IsRegistered())
            {
                manipulatorPtr->Unregister();
            }
        }
    }

    void TranslationManipulator::Register(AzToolsFramework::ManipulatorManagerId manipulatorManagerId)
    {
        for (AZStd::unique_ptr<BaseManipulator>& manipulatorPtr : m_manipulators)
        {
            manipulatorPtr->Register(manipulatorManagerId);
        }
    }

    void TranslationManipulator::Unregister()
    {
        for (AZStd::unique_ptr<BaseManipulator>& manipulatorPtr : m_manipulators)
        {
            manipulatorPtr->Unregister();
        }
    }

    void TranslationManipulator::InstallLinearManipulatorMouseDownCallback(LinearManipulatorMouseActionCallback onMouseDownCallback)
    {
        for (size_t i = m_manipulatorIndices.m_linearBeginIndex; i < m_manipulatorIndices.m_linearEndIndex; ++i)
        {
            if (LinearManipulator* linearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                linearManipulator->InstallMouseDownCallback(onMouseDownCallback);
            }
        }
    }

    void TranslationManipulator::InstallLinearManipulatorMouseMoveCallback(LinearManipulatorMouseActionCallback onMouseMoveCallback)
    {
        for (size_t i = m_manipulatorIndices.m_linearBeginIndex; i < m_manipulatorIndices.m_linearEndIndex; ++i)
        {
            if (LinearManipulator* linearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                linearManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
            }
        }
    }

    void TranslationManipulator::InstallLinearManipulatorMouseUpCallback(LinearManipulatorMouseActionCallback onMouseUpCallback)
    {
        for (size_t i = m_manipulatorIndices.m_linearBeginIndex; i < m_manipulatorIndices.m_linearEndIndex; ++i)
        {
            if (LinearManipulator* linearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                linearManipulator->InstallMouseUpCallback(onMouseUpCallback);
            }
        }
    }

    void TranslationManipulator::InstallPlanarManipulatorMouseDownCallback(PlanarManipulatorMouseActionCallback onMouseDownCallback)
    {
        for (size_t i = m_manipulatorIndices.m_planarBeginIndex; i < m_manipulatorIndices.m_planarEndIndex; ++i)
        {
            if (PlanarManipulator* planarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[i].get()))
            {
                planarManipulator->InstallMouseDownCallback(onMouseDownCallback);
            }
        }
    }

    void TranslationManipulator::InstallPlanarManipulatorMouseMoveCallback(PlanarManipulatorMouseActionCallback onMouseMoveCallback)
    {
        for (size_t i = m_manipulatorIndices.m_planarBeginIndex; i < m_manipulatorIndices.m_planarEndIndex; ++i)
        {
            if (PlanarManipulator* planarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[i].get()))
            {
                planarManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
            }
        }
    }

    void TranslationManipulator::InstallPlanarManipulatorMouseUpCallback(PlanarManipulatorMouseActionCallback onMouseUpCallback)
    {
        for (size_t i = m_manipulatorIndices.m_planarBeginIndex; i < m_manipulatorIndices.m_planarEndIndex; ++i)
        {
            if (PlanarManipulator* planarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[i].get()))
            {
                planarManipulator->InstallMouseUpCallback(onMouseUpCallback);
            }
        }
    }

    void TranslationManipulator::SetBoundsDirty()
    {
        for (AZStd::unique_ptr<BaseManipulator>& manipulatorPtr : m_manipulators)
        {
            manipulatorPtr->SetBoundsDirty();
        }
    }

    void TranslationManipulator::SetPosition(const AZ::Vector3& origin)
    {
        for (size_t i = m_manipulatorIndices.m_linearBeginIndex; i < m_manipulatorIndices.m_linearEndIndex; ++i)
        {
            if (LinearManipulator* linearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get()))
            {
                linearManipulator->SetPosition(origin);
            }
        }

        for (size_t i = m_manipulatorIndices.m_planarBeginIndex; i < m_manipulatorIndices.m_planarEndIndex; ++i)
        {
            if (PlanarManipulator* planarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[i].get()))
            {
                planarManipulator->SetPosition(origin);
            }
        }

        m_origin = origin;
    }

    void TranslationManipulator::SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3 /*= AZ::Vector3::CreateAxisZ()*/)
    {
        if (LinearManipulator* linearManipulator1 = azdynamic_cast<LinearManipulator*>(m_manipulators[m_manipulatorIndices.m_linearBeginIndex].get()))
        {
            linearManipulator1->SetDirection(axis1);
        }

        if (LinearManipulator* linearManipulator2 = azdynamic_cast<LinearManipulator*>(m_manipulators[m_manipulatorIndices.m_linearBeginIndex + 1].get()))
        {
            linearManipulator2->SetDirection(axis2);
        }

        if (PlanarManipulator* planarManipulator1 = azdynamic_cast<PlanarManipulator*>(m_manipulators[m_manipulatorIndices.m_planarBeginIndex].get()))
        {
            planarManipulator1->SetAxes(axis1, axis2);
        }

        if (m_dimensions == Dimensions::Three)
        {
            if (LinearManipulator* linearManipulator3 = azdynamic_cast<LinearManipulator*>(m_manipulators[m_manipulatorIndices.m_linearBeginIndex + 2].get()))
            {
                linearManipulator3->SetDirection(axis3);
            }

            if (PlanarManipulator* planarManipulator2 = azdynamic_cast<PlanarManipulator*>(m_manipulators[m_manipulatorIndices.m_planarBeginIndex + 1].get()))
            {
                planarManipulator2->SetAxes(axis2, axis3);
            }
            if (PlanarManipulator* planarManipulator3 = azdynamic_cast<PlanarManipulator*>(m_manipulators[m_manipulatorIndices.m_planarBeginIndex + 2].get()))
            {
                planarManipulator3->SetAxes(axis3, axis1);
            }
        }
    }

    void TranslationManipulator::SetAxesColor(const AZ::Color& axis1Color, const AZ::Color& axis2Color, const AZ::Color& axis3Color /*= AZ::Color(0.0f, 0.0f, 1.0f, 0.5f)*/)
    {
        if (LinearManipulator* linearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[m_manipulatorIndices.m_linearBeginIndex].get()))
        {
            linearManipulator->SetColor(axis1Color);
        }
        
        if (LinearManipulator* linearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[m_manipulatorIndices.m_linearBeginIndex + 1].get()))
        {
            linearManipulator->SetColor(axis2Color);
        }

        if (m_dimensions == Dimensions::Three)
        {   
            if (LinearManipulator* linearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[m_manipulatorIndices.m_linearBeginIndex + 2].get()))
            {
                linearManipulator->SetColor(axis3Color);
            }
        }
    }

    void TranslationManipulator::SetAxisLength(float axisLength)
    {
        for (size_t i = m_manipulatorIndices.m_linearBeginIndex; i < m_manipulatorIndices.m_linearEndIndex; ++i)
        {
            LinearManipulator* linearManipulator = azdynamic_cast<LinearManipulator*>(m_manipulators[i].get());
            linearManipulator->SetLength(axisLength);
        }
    }

    void TranslationManipulator::SetPlanesColor(const AZ::Color& plane1Color, const AZ::Color& plane2Color /*= AZ::Color(0.0f, 1.0f, 0.0f, 0.5f)*/, const AZ::Color& plane3Color /*= AZ::Color(0.0f, 0.0f, 1.0f, 0.5f)*/)
    {
        if (PlanarManipulator* planarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[m_manipulatorIndices.m_planarBeginIndex].get()))
        {
            planarManipulator->SetColor(plane1Color);
        }

        if (m_dimensions == Dimensions::Three)
        {
            if (PlanarManipulator* planarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[m_manipulatorIndices.m_planarBeginIndex + 1].get()))
            {
                planarManipulator->SetColor(plane2Color);
            }
            if (PlanarManipulator* planarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[m_manipulatorIndices.m_planarBeginIndex + 2].get()))
            {
                planarManipulator->SetColor(plane3Color);
            }
        }
    }

    void TranslationManipulator::SetPlaneSize(float size)
    {
        for (size_t i = m_manipulatorIndices.m_planarBeginIndex; i < m_manipulatorIndices.m_planarEndIndex; ++i)
        {
            PlanarManipulator* planarManipulator = azdynamic_cast<PlanarManipulator*>(m_manipulators[i].get());
            planarManipulator->SetAxesLength(size, size);
        }
    }
} // namespace AzToolsFramework