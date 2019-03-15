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
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Picking/Manipulators/ManipulatorBoundManager.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzFramework
{
    class EntityDebugDisplayRequests;
}

namespace AzToolsFramework
{
    namespace Picking
    {
        class DefaultContextBoundManager;
    }

    namespace ViewportInteraction
    {
        struct CameraState;
        struct MouseInteraction;
    }

    class BaseManipulator;
    class LinearManipulator;

    /**
     * State of overall manipulator manager.
     */
    struct ManipulatorManagerState
    {
        bool m_interacting;
    };

    /**
     * This class serves to manage all relevant mouse events and coordinate all registered manipulators to function properly.
     * ManipulatorManager does not manage the life cycle of specific manipulators. The users of manipulators are responsible
     * for creating and deleting them at right time, as well as registering and unregistering accordingly.
     */
    class ManipulatorManager final
        : private ManipulatorManagerRequestBus::Handler
        , private EditorEntityInfoNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorManager, AZ::SystemAllocator, 0)

        explicit ManipulatorManager(ManipulatorManagerId managerId);
        ~ManipulatorManager();

        // NOTE: These are NOT EBus messages. They are called by the owner of the manipulator manager and they will return TRUE
        // if they have gobbled up the interaction. If this is the case you should not process it yourself.
        bool ConsumeViewportMousePress(const ViewportInteraction::MouseInteraction&);
        bool ConsumeViewportMouseMove(const ViewportInteraction::MouseInteraction&);
        bool ConsumeViewportMouseRelease(const ViewportInteraction::MouseInteraction&);
        bool ConsumeViewportMouseWheel(const ViewportInteraction::MouseInteraction&);

        /// ManipulatorManagerRequestBus::Handler
        void RegisterManipulator(AZStd::shared_ptr<BaseManipulator> manipulator) override;
        void UnregisterManipulator(BaseManipulator* manipulator) override;
        void DeleteManipulatorBound(Picking::RegisteredBoundId boundId) override;
        void SetBoundDirty(Picking::RegisteredBoundId boundId) override;
        void SetAllBoundsDirty() override;
        Picking::RegisteredBoundId UpdateBound(ManipulatorId manipulatorId,
            Picking::RegisteredBoundId boundId, const Picking::BoundRequestShapeBase& boundShapeData) override;
        void SetManipulatorSpace(ManipulatorSpace manipulatorSpace) override;
        ManipulatorSpace GetManipulatorSpace() override;

        void DrawManipulators(
            AzFramework::EntityDebugDisplayRequests& display,
            const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction);

        /**
         * Check if the modifier key state has changed - if so we may need to refresh
         * certain manipulator bounds.
         */
        void CheckModifierKeysChanged(
            ViewportInteraction::KeyboardModifiers keyboardModifiers,
            const ViewportInteraction::MousePick& mousePick);

        bool Interacting() const { return m_activeManipulator != nullptr; }

    private:
        /**
         * @param rayOrigin The origin of the ray to test intersection with.
         * @param rayDirection The direction of the ray to test intersection with.
         * @param[out] rayIntersectionDistance The result intersecting point equals "rayOrigin + rayIntersectionDistance * rayDirection".
         * @return A pointer to a manipulator that the ray intersects. Null pointer if no intersection is detected.
         */
        AZStd::shared_ptr<BaseManipulator> PerformRaycast(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance);

        // EditorEntityInfoNotifications
        void OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool visible) override;

        ManipulatorManagerId m_manipulatorManagerId; ///< This manipulator manager's id.
        ManipulatorId m_nextManipulatorIdToGenerate; ///< Id to use for the next manipulator that is registered with this manager.

        AZStd::unordered_map<ManipulatorId, AZStd::shared_ptr<BaseManipulator>> m_manipulatorIdToPtrMap; ///< Mapping from a manipulatorId to the corresponding manipulator.
        AZStd::unordered_map<Picking::RegisteredBoundId, ManipulatorId> m_boundIdToManipulatorIdMap; ///< Mapping from a boundId to the corresponding manipulatorId.

        AZStd::shared_ptr<BaseManipulator> m_activeManipulator = nullptr; ///< The manipulator we are currently interacting with.
        Picking::ManipulatorBoundManager m_boundManager; ///< All active manipulator bounds that could be interacted with.

        ViewportInteraction::KeyboardModifiers m_keyboardModifiers; ///< Our recorded state of the modifier keys.

        ManipulatorSpace m_manipulatorSpace = ManipulatorSpace::Local; ///< Will manipulator movements happen in World or Local space.
    };
} // namespace AzToolsFramework
