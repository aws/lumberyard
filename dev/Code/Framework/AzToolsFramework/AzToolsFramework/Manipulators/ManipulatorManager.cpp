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

#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include "BaseManipulator.h"
#include "ManipulatorManager.h"

namespace AzToolsFramework
{
    ManipulatorManager::ManipulatorManager(ManipulatorManagerId managerId)
        : m_manipulatorManagerId(managerId)
        , m_activeManipulator(nullptr)
        , m_nextManipulatorIDToGenerate(1)
        , m_boundManager(managerId)
    {
        ManipulatorManagerRequestBus::Handler::BusConnect(m_manipulatorManagerId);
    }

    ManipulatorManager::~ManipulatorManager()
    {
        AZStd::vector<BaseManipulator*> manipulatorsToRemove(m_manipulatorIdToPtrMap.size());
        for (auto& pair : m_manipulatorIdToPtrMap)
        {
            manipulatorsToRemove.push_back(pair.second);
        }
        for (BaseManipulator* manipulator : manipulatorsToRemove)
        {
            UnregisterManipulator(*manipulator);
        }

        ManipulatorManagerRequestBus::Handler::BusDisconnect();
    }

    void ManipulatorManager::RegisterManipulator(BaseManipulator& manipulator)
    {
        if (manipulator.IsRegistered())
        {
            AZ_Assert(manipulator.GetManipulatorManagerId() == m_manipulatorManagerId, "This manipulator was registered with a different manipulator manager!");
            return;
        }

        ManipulatorId newId = m_nextManipulatorIDToGenerate++;
        m_manipulatorIdToPtrMap[newId] = &manipulator;
        manipulator.SetManipulatorId(newId);
        manipulator.SetManipulatorManagerId(m_manipulatorManagerId);
    }

    void ManipulatorManager::UnregisterManipulator(BaseManipulator& manipulator)
    {
        ManipulatorId manipulatorId = manipulator.GetManipulatorId();
        m_manipulatorIdToPtrMap.erase(manipulatorId);
        manipulator.Invalidate();
    }

    void ManipulatorManager::SetActiveManipulator(BaseManipulator* ptrManip)
    {
        // This manipulator now gets all input until we call this again with nullptr.
        if (m_activeManipulator != ptrManip)
        {
            m_activeManipulator = ptrManip;
        }
    }

    Picking::RegisteredBoundId ManipulatorManager::UpdateManipulatorBound(ManipulatorId manipulatorId,
        Picking::RegisteredBoundId boundId, const Picking::BoundRequestShapeBase& boundShapeData)
    {
        if (manipulatorId == InvalidManipulatorId)
        {
            return Picking::InvalidBoundId;
        }
        auto manipulatorItr = m_manipulatorIdToPtrMap.find(manipulatorId);
        if (manipulatorItr == m_manipulatorIdToPtrMap.end())
        {
            return Picking::InvalidBoundId;
        }

        if (boundId != Picking::InvalidBoundId)
        {
            auto boundItr = m_boundIdToManipulatorIdMap.find(boundId);
            AZ_Assert(boundItr != m_boundIdToManipulatorIdMap.end(), "Manipulator and its bounds are out of synchronization!");
            AZ_Assert(boundItr->second == manipulatorId, "Manipulator and its bounds are out of synchronization!");
        }

        Picking::RegisteredBoundId newBoundId = m_boundManager.UpdateOrRegisterBound(boundShapeData, boundId);
        if (newBoundId != boundId)
        {
            m_boundIdToManipulatorIdMap[newBoundId] = manipulatorId;
        }

        return newBoundId;
    }

    void ManipulatorManager::SetTransformManipulatorMode(TransformManipulatorMode mode)
    {
        if (m_transformManipulatorMode != mode)
        {
            TransformManipulatorMode prevMode = m_transformManipulatorMode;
            m_transformManipulatorMode = mode;
            ManipulatorManagerNotificationBus::Event(m_manipulatorManagerId, 
                &ManipulatorManagerNotificationBus::Events::OnTransformManipulatorModeChanged, prevMode, m_transformManipulatorMode);
        }
    }

    TransformManipulatorMode ManipulatorManager::GetTransformManipulatorMode()
    {
        return m_transformManipulatorMode;
    }

    void ManipulatorManager::SetManipulatorReferenceSpace(ManipulatorReferenceSpace referenceSpace)
    {
        if (m_manipulatorReferenceSpace != referenceSpace)
        {
            m_manipulatorReferenceSpace = referenceSpace;
        }
    }

    ManipulatorReferenceSpace ManipulatorManager::GetManipulatorReferenceSpace()
    {
        return m_manipulatorReferenceSpace;
    }

    void ManipulatorManager::SetBoundDirty(Picking::RegisteredBoundId boundId)
    {
        m_boundManager.SetBoundValidity(boundId, false);
    }

    void ManipulatorManager::DeleteManipulatorBound(Picking::RegisteredBoundId boundId)
    {
        m_boundManager.UnregisterBound(boundId);
        m_boundIdToManipulatorIdMap.erase(boundId);
    }

    void ManipulatorManager::SetAllManipulatorBoundsDirty()
    {
        for (auto maniputlatorItr = m_manipulatorIdToPtrMap.begin(); maniputlatorItr != m_manipulatorIdToPtrMap.end(); ++maniputlatorItr)
        {
            maniputlatorItr->second->SetBoundsDirty();
        }
    }

    void ManipulatorManager::DrawManipulators(AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState)
    {
        for (auto it = m_manipulatorIdToPtrMap.begin(); it != m_manipulatorIdToPtrMap.end(); ++it)
        {
            it->second->Draw(display, cameraState);
        }
    }

    BaseManipulator* ManipulatorManager::PerformRaycast(const AZ::Vector3& rayOrigin, const AZ::Vector3 rayDirection, float& t)
    {

        Picking::RaySelectInfo raySelection;
        raySelection.m_origin = rayOrigin;
        raySelection.m_direction = rayDirection;
        raySelection.m_boundIDsHit.clear();

        m_boundManager.RaySelect(raySelection);

        for (auto hitItr = raySelection.m_boundIDsHit.begin(); hitItr != raySelection.m_boundIDsHit.end(); ++hitItr)
        {
            auto found = m_boundIdToManipulatorIdMap.find(hitItr->first);
            if (found != m_boundIdToManipulatorIdMap.end())
            {
                auto manipulatorFound = m_manipulatorIdToPtrMap.find(found->second);
                AZ_Assert(manipulatorFound != m_manipulatorIdToPtrMap.end(), "Out of sync lists!");
                t = hitItr->second;
                return manipulatorFound->second;
            }
        }

        return nullptr;
    }

    bool ManipulatorManager::ConsumeViewportMousePress(const ViewportInteraction::MouseInteraction &interaction)
    {
        if (interaction.m_keyModifiers == ViewportInteraction::Modifiers::MI_LEFT_BUTTON)
        {
            float t = 0.0f;
            BaseManipulator* manipulator = PerformRaycast(interaction.m_rayOrigin, interaction.m_rayDirection, t);

            if (manipulator)
            {
                m_activeManipulator = manipulator;
                manipulator->OnMouseDown(interaction, t);
                return true;
            }
        }
        
        return false;
    }

    bool ManipulatorManager::ConsumeViewportMouseMove(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_activeManipulator)
        {
            m_activeManipulator->OnMouseMove(interaction);
            return true;
        }
        else
        {
            float t = 0.0f;
            BaseManipulator* manipulator = PerformRaycast(interaction.m_rayOrigin, interaction.m_rayDirection, t);
            ManipulatorId manipulatorId = manipulator ? manipulator->GetManipulatorId() : InvalidManipulatorId;

            for (auto it = m_manipulatorIdToPtrMap.begin(); it != m_manipulatorIdToPtrMap.end(); ++it)
            {
                it->second->OnMouseOver(manipulatorId);
            }
            return false;
        }
    }

    bool ManipulatorManager::ConsumeViewportMouseRelease(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (interaction.m_keyModifiers == ViewportInteraction::Modifiers::MI_LEFT_BUTTON)
        {
            if (m_activeManipulator)
            {
                m_activeManipulator->OnMouseUp(interaction);
                m_activeManipulator = nullptr;
                return true;
            }
        }

        return false;
    }

    bool ManipulatorManager::ConsumeViewportMouseWheel(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_activeManipulator)
        {
            m_activeManipulator->OnMouseWheel(interaction);
            return true;
        }
        else
        {
            float t = 0.0f;
            BaseManipulator* manipulator = PerformRaycast(interaction.m_rayOrigin, interaction.m_rayDirection, t);
            ManipulatorId manipulatorId = manipulator ? manipulator->GetManipulatorId() : InvalidManipulatorId;

            for (auto it = m_manipulatorIdToPtrMap.begin(); it != m_manipulatorIdToPtrMap.end(); ++it)
            {
                it->second->SetBoundsDirty();
                it->second->OnMouseOver(manipulatorId);
            }
            return false;
        }
    }
}