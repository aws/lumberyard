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
    const ManipulatorManagerId g_mainManipulatorManagerId = ManipulatorManagerId(AZ::Crc32("MainManipulatorManagerId"));

    ManipulatorManager::ManipulatorManager(const ManipulatorManagerId managerId)
        : m_manipulatorManagerId(managerId)
        , m_nextManipulatorIdToGenerate(ManipulatorId(1))
    {
        ManipulatorManagerRequestBus::Handler::BusConnect(m_manipulatorManagerId);
        EditorEntityInfoNotificationBus::Handler::BusConnect();
    }

    ManipulatorManager::~ManipulatorManager()
    {
        for (auto& pair : m_manipulatorIdToPtrMap)
        {
            pair.second->Invalidate();
        }

        m_manipulatorIdToPtrMap.clear();

        ManipulatorManagerRequestBus::Handler::BusDisconnect();
        EditorEntityInfoNotificationBus::Handler::BusDisconnect();
    }

    void ManipulatorManager::RegisterManipulator(AZStd::shared_ptr<BaseManipulator> manipulator)
    {
        if (!manipulator)
        {
            AZ_Error("Manipulators", false, "Attempting to register a null Manipulator");
            return;
        }

        if (manipulator->Registered())
        {
            AZ_Assert(manipulator->GetManipulatorManagerId() == m_manipulatorManagerId,
                "This manipulator was registered with a different manipulator manager!");
            return;
        }

        const ManipulatorId manipulatorId = m_nextManipulatorIdToGenerate++;
        manipulator->m_manipulatorId = manipulatorId;
        manipulator->m_manipulatorManagerId = m_manipulatorManagerId;
        m_manipulatorIdToPtrMap[manipulatorId] = AZStd::move(manipulator);
    }

    void ManipulatorManager::UnregisterManipulator(BaseManipulator* manipulator)
    {
        if (!manipulator)
        {
            AZ_Error("Manipulators", false, "Attempting to unregister a null Manipulator");
            return;
        }

        m_manipulatorIdToPtrMap.erase(manipulator->GetManipulatorId());
        manipulator->Invalidate();
    }

    Picking::RegisteredBoundId ManipulatorManager::UpdateBound(
        const ManipulatorId manipulatorId, const Picking::RegisteredBoundId boundId,
        const Picking::BoundRequestShapeBase& boundShapeData)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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

        const Picking::RegisteredBoundId newBoundId =
            m_boundManager.UpdateOrRegisterBound(boundShapeData, boundId);

        if (newBoundId != boundId)
        {
            m_boundIdToManipulatorIdMap[newBoundId] = manipulatorId;
        }

        return newBoundId;
    }

    void ManipulatorManager::SetBoundDirty(const Picking::RegisteredBoundId boundId)
    {
        if (boundId != Picking::InvalidBoundId)
        {
            m_boundManager.SetBoundValidity(boundId, false);
        }
    }

    void ManipulatorManager::DeleteManipulatorBound(const Picking::RegisteredBoundId boundId)
    {
        if (boundId != Picking::InvalidBoundId)
        {
            m_boundManager.UnregisterBound(boundId);
            m_boundIdToManipulatorIdMap.erase(boundId);
        }
    }

    void ManipulatorManager::CheckModifierKeysChanged(
        const ViewportInteraction::KeyboardModifiers keyboardModifiers,
        const ViewportInteraction::MousePick& mousePick)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_keyboardModifiers != keyboardModifiers)
        {
            if (!Interacting())
            {
                float rayIntersectionDistance = 0.0f;
                const AZStd::shared_ptr<BaseManipulator> manipulator = PerformRaycast(
                    mousePick.m_rayOrigin, mousePick.m_rayDirection, rayIntersectionDistance);
                const ManipulatorId manipulatorId = manipulator
                    ? manipulator->GetManipulatorId()
                    : InvalidManipulatorId;

                for (auto& pair : m_manipulatorIdToPtrMap)
                {
                    pair.second->UpdateMouseOver(manipulatorId);
                }
            }

            m_keyboardModifiers.m_keyModifiers = keyboardModifiers.m_keyModifiers;
        }
    }

    void ManipulatorManager::DrawManipulators(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        for (const auto& pair : m_manipulatorIdToPtrMap)
        {
            pair.second->Draw({ Interacting() }, debugDisplay, cameraState, mouseInteraction);
        }

        CheckModifierKeysChanged(mouseInteraction.m_keyboardModifiers, mouseInteraction.m_mousePick);
    }

    AZStd::shared_ptr<BaseManipulator> ManipulatorManager::PerformRaycast(
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        Picking::RaySelectInfo raySelection;
        raySelection.m_origin = rayOrigin;
        raySelection.m_direction = rayDirection;

        m_boundManager.RaySelect(raySelection);

        for (const auto& hitItr : raySelection.m_boundIdsHit)
        {
            const auto found = m_boundIdToManipulatorIdMap.find(hitItr.first);
            if (found != m_boundIdToManipulatorIdMap.end())
            {
                const auto manipulatorFound = m_manipulatorIdToPtrMap.find(found->second);
                AZ_Assert(manipulatorFound != m_manipulatorIdToPtrMap.end(),
                    "Found a bound without a corresponding Manipulator, "
                    "it's likely a bound was not cleaned up correctly");
                rayIntersectionDistance = hitItr.second;
                return manipulatorFound != m_manipulatorIdToPtrMap.end() ? manipulatorFound->second : nullptr;
            }
        }

        return nullptr;
    }

    bool ManipulatorManager::ConsumeViewportMousePress(const ViewportInteraction::MouseInteraction& interaction)
    {
        float rayIntersectionDistance = 0.0f;
        if (AZStd::shared_ptr<BaseManipulator> manipulator = PerformRaycast(
            interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, rayIntersectionDistance))
        {
            if (interaction.m_mouseButtons.Left())
            {
                if (manipulator->OnLeftMouseDown(interaction, rayIntersectionDistance))
                {
                    m_activeManipulator = manipulator;
                    return true;
                }
            }

            if (interaction.m_mouseButtons.Right())
            {
                if (manipulator->OnRightMouseDown(interaction, rayIntersectionDistance))
                {
                    m_activeManipulator = manipulator;
                    return true;
                }
            }
        }

        return false;
    }

    bool ManipulatorManager::ConsumeViewportMouseRelease(const ViewportInteraction::MouseInteraction& interaction)
    {
        // must have had a meaningful interaction in mouse down to have assigned an
        // active manipulator - only notify mouse up if this was the case
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

    ManipulatorManager::ConsumeMouseMoveResult ManipulatorManager::ConsumeViewportMouseMove(
        const ViewportInteraction::MouseInteraction& interaction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_activeManipulator)
        {
            m_activeManipulator->OnMouseMove(interaction);
            return ConsumeMouseMoveResult::Interacting;
        }

        float rayIntersectionDistance = 0.0f;
        const AZStd::shared_ptr<BaseManipulator> manipulator = PerformRaycast(
            interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, rayIntersectionDistance);
        const ManipulatorId manipulatorId = manipulator
            ? manipulator->GetManipulatorId()
            : InvalidManipulatorId;

        ConsumeMouseMoveResult mouseMoveResult = ConsumeMouseMoveResult::None;
        for (auto& pair : m_manipulatorIdToPtrMap)
        {
            if (pair.second->OnMouseOver(manipulatorId, interaction))
            {
                mouseMoveResult = ConsumeMouseMoveResult::Hovering;
            }
        }

        return mouseMoveResult;
    }

    bool ManipulatorManager::ConsumeViewportMouseWheel(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_activeManipulator)
        {
            m_activeManipulator->OnMouseWheel(interaction);
            return true;
        }

        float rayIntersectionDistance = 0.0f;
        const AZStd::shared_ptr<BaseManipulator> manipulator = PerformRaycast(
            interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, rayIntersectionDistance);
        const ManipulatorId manipulatorId = manipulator
            ? manipulator->GetManipulatorId()
            : InvalidManipulatorId;

        // when scrolling the mouse wheel, the view may change so the mouse falls over a manipulator
        // without actually moving, ensure we refresh its bounds and call OnMouseOver when this happens
        for (auto& pair : m_manipulatorIdToPtrMap)
        {
            pair.second->SetBoundsDirty();
            pair.second->OnMouseOver(manipulatorId, interaction);
        }

        return false;
    }

    void ManipulatorManager::OnEntityInfoUpdatedVisibility(const AZ::EntityId entityId, const bool visible)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        for (auto& pair : m_manipulatorIdToPtrMap)
        {
            // set all manipulator bounds on this entity to dirty so we cannot
            // interact with them (bounds will be refreshed when they are redrawn)
            for (const AZ::EntityId id : pair.second->EntityIds())
            {
                if (id == entityId && !visible)
                {
                    pair.second->SetBoundsDirty();
                    break;
                }
            }
        }
    }
}