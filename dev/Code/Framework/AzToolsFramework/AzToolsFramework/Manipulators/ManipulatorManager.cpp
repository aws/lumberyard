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

#include "BaseManipulator.h"
#include "ManipulatorManager.h"

#include <AzToolsFramework/Picking/BoundInterface.h>

namespace AzToolsFramework
{
    ManipulatorManager::ManipulatorManager(ManipulatorManagerId managerId)
        : m_manipulatorManagerId(managerId)
        , m_nextManipulatorIdToGenerate(ManipulatorId(1))
        , m_boundManager(managerId)
    {
        ManipulatorManagerRequestBus::Handler::BusConnect(m_manipulatorManagerId);
        EditorEntityInfoNotificationBus::Handler::BusConnect();
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
        EditorEntityInfoNotificationBus::Handler::BusDisconnect();
    }

    void ManipulatorManager::RegisterManipulator(BaseManipulator& manipulator)
    {
        if (manipulator.Registered())
        {
            AZ_Assert(manipulator.GetManipulatorManagerId() == m_manipulatorManagerId,
                "This manipulator was registered with a different manipulator manager!");
            return;
        }

        const ManipulatorId manipulatorId = m_nextManipulatorIdToGenerate++;
        m_manipulatorIdToPtrMap[manipulatorId] = &manipulator;
        manipulator.m_manipulatorId = manipulatorId;
        manipulator.m_manipulatorManagerId = m_manipulatorManagerId;
    }

    void ManipulatorManager::UnregisterManipulator(BaseManipulator& manipulator)
    {
        m_manipulatorIdToPtrMap.erase(manipulator.GetManipulatorId());
        manipulator.Invalidate();
    }

    void ManipulatorManager::SetActiveManipulator(BaseManipulator* manipulator)
    {
        if (m_activeManipulator != manipulator)
        {
            m_activeManipulator = manipulator;
        }
    }

    Picking::RegisteredBoundId ManipulatorManager::UpdateBound(ManipulatorId manipulatorId,
        Picking::RegisteredBoundId boundId, const Picking::BoundRequestShapeBase& boundShapeData)
    {
        if (manipulatorId == InvalidManipulatorId)
        {
            return Picking::InvalidBoundId;
        }

        const auto manipulatorItr = m_manipulatorIdToPtrMap.find(manipulatorId);
        if (manipulatorItr == m_manipulatorIdToPtrMap.end())
        {
            return Picking::InvalidBoundId;
        }

        if (boundId != Picking::InvalidBoundId)
        {
            auto boundItr = m_boundIdToManipulatorIdMap.find(boundId);
            AZ_UNUSED(boundItr);
            AZ_Assert(boundItr != m_boundIdToManipulatorIdMap.end(), "Manipulator and its bounds are out of synchronization!");
            AZ_Assert(boundItr->second == manipulatorId, "Manipulator and its bounds are out of synchronization!");
        }

        const Picking::RegisteredBoundId newBoundId = m_boundManager.UpdateOrRegisterBound(boundShapeData, boundId);
        if (newBoundId != boundId)
        {
            m_boundIdToManipulatorIdMap[newBoundId] = manipulatorId;
        }

        return newBoundId;
    }

    void ManipulatorManager::SetManipulatorSpace(ManipulatorSpace manipulatorSpace)
    {
        if (m_manipulatorSpace != manipulatorSpace)
        {
            m_manipulatorSpace = manipulatorSpace;
        }
    }

    ManipulatorSpace ManipulatorManager::GetManipulatorSpace()
    {
        return m_manipulatorSpace;
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

    void ManipulatorManager::SetAllBoundsDirty()
    {
        for (auto& pair : m_manipulatorIdToPtrMap)
        {
            pair.second->SetBoundsDirty();
        }
    }

    void ManipulatorManager::CheckModifierKeysChanged(
        ViewportInteraction::KeyboardModifiers keyboardModifiers,
        const ViewportInteraction::MousePick& mousePick)
    {
        if (m_keyboardModifiers != keyboardModifiers)
        {
            float rayIntersectionDistance = 0.0f;
            BaseManipulator* manipulator = PerformRaycast(
                mousePick.m_rayOrigin, mousePick.m_rayDirection, rayIntersectionDistance);
            const ManipulatorId manipulatorId = manipulator ? manipulator->GetManipulatorId() : InvalidManipulatorId;

            for (auto& pair : m_manipulatorIdToPtrMap)
            {
                pair.second->UpdateMouseOver(manipulatorId);
            }

            m_keyboardModifiers.m_keyModifiers = keyboardModifiers.m_keyModifiers;
        }
    }

    void ManipulatorManager::DrawManipulators(
        AzFramework::EntityDebugDisplayRequests& display,
        const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        for (const auto& pair : m_manipulatorIdToPtrMap)
        {
            // check if the entity is currently visible
            bool visible = false;
            EditorEntityInfoRequestBus::EventResult(
                visible, pair.second->GetEntityId(), &EditorEntityInfoRequestBus::Events::IsVisible);

            if (visible)
            {
                pair.second->Draw({ Interacting() }, display, cameraState, mouseInteraction);
            }
        }

        CheckModifierKeysChanged(mouseInteraction.m_keyboardModifiers, mouseInteraction.m_mousePick);
    }

    BaseManipulator* ManipulatorManager::PerformRaycast(
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
    {
        Picking::RaySelectInfo raySelection;
        raySelection.m_origin = rayOrigin;
        raySelection.m_direction = rayDirection;

        m_boundManager.RaySelect(raySelection);

        for (auto hitItr = raySelection.m_boundIDsHit.begin(); hitItr != raySelection.m_boundIDsHit.end(); ++hitItr)
        {
            const auto found = m_boundIdToManipulatorIdMap.find(hitItr->first);
            if (found != m_boundIdToManipulatorIdMap.end())
            {
                const auto manipulatorFound = m_manipulatorIdToPtrMap.find(found->second);
                AZ_Assert(manipulatorFound != m_manipulatorIdToPtrMap.end(), "Manipulator bound and id lists are out of sync");
                rayIntersectionDistance = hitItr->second;
                return manipulatorFound != m_manipulatorIdToPtrMap.end() ? manipulatorFound->second : nullptr;
            }
        }

        return nullptr;
    }

    bool ManipulatorManager::ConsumeViewportMousePress(const ViewportInteraction::MouseInteraction& interaction)
    {
        float rayIntersectionDistance = 0.0f;
        if (BaseManipulator* manipulator = PerformRaycast(
            interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, rayIntersectionDistance))
        {
            if (interaction.m_mouseButtons.Left())
            {
                if (manipulator->OnLeftMouseDown(interaction, rayIntersectionDistance))
                {
                    m_activeManipulator = manipulator;
                }

                return true;
            }

            if (interaction.m_mouseButtons.Right())
            {
                if (manipulator->OnRightMouseDown(interaction, rayIntersectionDistance))
                {
                    m_activeManipulator = manipulator;
                }

                return true;
            }
        }

        return false;
    }

    bool ManipulatorManager::ConsumeViewportMouseRelease(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_activeManipulator)
        {
            if (interaction.m_mouseButtons.Left())
            {
                m_activeManipulator->OnLeftMouseUp(interaction);
                m_activeManipulator = nullptr;
                return true;
            }

            if (interaction.m_mouseButtons.Right())
            {
                m_activeManipulator->OnRightMouseUp(interaction);
                m_activeManipulator = nullptr;
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

        float rayIntersectionDistance = 0.0f;
        const BaseManipulator* manipulator = PerformRaycast(interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, rayIntersectionDistance);
        const ManipulatorId manipulatorId = manipulator ? manipulator->GetManipulatorId() : InvalidManipulatorId;

        for (auto& pair : m_manipulatorIdToPtrMap)
        {
            pair.second->OnMouseOver(manipulatorId, interaction);
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

        float rayIntersectionDistance = 0.0f;
        const BaseManipulator* manipulator = PerformRaycast(interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, rayIntersectionDistance);
        const ManipulatorId manipulatorId = manipulator ? manipulator->GetManipulatorId() : InvalidManipulatorId;

        // when scrolling the mouse wheel, the view may change so the mouse falls over a manipulator
        // without actually moving, ensure we refresh its bounds and call OnMouseOver when this happens
        for (auto& pair : m_manipulatorIdToPtrMap)
        {
            pair.second->SetBoundsDirty();
            pair.second->OnMouseOver(manipulatorId, interaction);
        }

        return false;
    }

    void ManipulatorManager::OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool visible)
    {
        for (auto& pair : m_manipulatorIdToPtrMap)
        {
            // set all manipulator bounds on this entity to dirty so we cannot
            // interact with them (bounds will be refreshed when they are redrawn)
            if (pair.second->GetEntityId() == entityId && !visible)
            {
                pair.second->SetBoundsDirty();
            }
        }
    }
}