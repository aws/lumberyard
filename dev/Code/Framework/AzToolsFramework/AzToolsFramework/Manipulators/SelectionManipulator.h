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

        explicit SelectionManipulator(AZ::EntityId entityId);
        ~SelectionManipulator();

        using MouseActionCallback = AZStd::function<void(const ViewportInteraction::MouseInteraction&)>;

        void InstallLeftMouseDownCallback(MouseActionCallback onMouseDownCallback);
        void InstallLeftMouseUpCallback(MouseActionCallback onMouseUpCallback);
        void InstallRightMouseDownCallback(MouseActionCallback onMouseDownCallback);
        void InstallRightMouseUpCallback(MouseActionCallback onMouseUpCallback);

        void OnLeftMouseDownImpl(
            const ViewportInteraction::MouseInteraction& interaction,
            float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnRightMouseDownImpl(
            const ViewportInteraction::MouseInteraction& interaction,
            float rayIntersectionDistance) override;
        void OnRightMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;

        void SetBoundsDirtyImpl() override;

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::EntityDebugDisplayRequests& display,
            const ViewportInteraction::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetPosition(const AZ::Vector3& position) { m_position = position; }
        const AZ::Vector3& GetPosition() const { return m_position; }

        bool Selected() const { return m_selected; }
        void Select() { m_selected = true; }
        void Deselect() { m_selected = false; }
        void ToggleSelected() { m_selected = !m_selected; }

        void SetView(AZStd::unique_ptr<ManipulatorView>&& view);
        
    protected:
        void InvalidateImpl() override;

    private:
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        bool m_selected = false;

        AZStd::unique_ptr<ManipulatorView> m_manipulatorView = nullptr; ///< look of manipulator

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onRightMouseDownCallback = nullptr;
        MouseActionCallback m_onRightMouseUpCallback = nullptr;
    };
}