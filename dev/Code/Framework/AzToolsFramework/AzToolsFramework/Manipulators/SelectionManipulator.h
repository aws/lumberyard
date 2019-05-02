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
#include <AzToolsFramework/Manipulators/BaseManipulator.h>

namespace AzToolsFramework
{
    class ManipulatorView;

    /**
     * Represents a sphere that can be clicked on to trigger a particular behaviour
     * For example clicking a preview point to create a translation manipulator.
     */
    class SelectionManipulator
        : public BaseManipulator
    {
    public:
        AZ_RTTI(SelectionManipulator, "{966F44B7-E287-4C28-9734-5958F1A13A1D}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(SelectionManipulator, AZ::SystemAllocator, 0);

        SelectionManipulator(AZ::EntityId entityId, const AZ::Transform& worldFromLocal);
        ~SelectionManipulator() = default;

        /**
         * This is the function signature of callbacks that will be invoked
         * whenever a selection manipulator is clicked on.
         */
        using MouseActionCallback = AZStd::function<void(const ViewportInteraction::MouseInteraction&)>;

        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);
        void InstallRightMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallRightMouseUpCallback(const MouseActionCallback& onMouseUpCallback);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::EntityDebugDisplayRequests& display,
            const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetPosition(const AZ::Vector3& position) { m_position = position; }
        void SetSpace(const AZ::Transform& worldFromLocal) { m_worldFromLocal = worldFromLocal; }
        
        const AZ::Vector3& GetPosition() const { return m_position; }

        bool Selected() const { return m_selected; }
        void Select() { m_selected = true; }
        void Deselect() { m_selected = false; }
        void ToggleSelected() { m_selected = !m_selected; }

        void SetView(AZStd::unique_ptr<ManipulatorView>&& view);

    private:
        AZ_DISABLE_COPY_MOVE(SelectionManipulator)

        void OnLeftMouseDownImpl(
            const ViewportInteraction::MouseInteraction& interaction,
            float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnRightMouseDownImpl(
            const ViewportInteraction::MouseInteraction& interaction,
            float rayIntersectionDistance) override;
        void OnRightMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;

        void InvalidateImpl() override;
        void SetBoundsDirtyImpl() override;

        AZ::Vector3 m_position = AZ::Vector3::CreateZero(); ///< Position in local space.
        AZ::Transform m_worldFromLocal = AZ::Transform::CreateIdentity(); ///< Space the manipulator is in (identity is world space).
        
        bool m_selected = false;

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onRightMouseDownCallback = nullptr;
        MouseActionCallback m_onRightMouseUpCallback = nullptr;
        
        AZStd::unique_ptr<ManipulatorView> m_manipulatorView = nullptr; ///< Look of manipulator.
    };
} // namespace AzToolsFramework