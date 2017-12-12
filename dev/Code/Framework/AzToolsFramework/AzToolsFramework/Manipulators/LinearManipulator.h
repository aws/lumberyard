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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/Picking/ContextBoundAPI.h>

#include "BaseManipulator.h"

namespace AzToolsFramework
{
    /**
     * LinearManipulator provides a way to visually change properties of a component.
     * Like the gizmo tool that's typically used to modify translation values of a transform component, LinearManipulator
     * can be attached to any component, serving as a visual tool for mouse to interact with in the viewport. Instead of 
     * three axes, LinearManipulator displays only one axis whose direction and origin can be customized. 
     */
    class LinearManipulator 
        : public BaseManipulator
    {
    public:

        enum class DisplayType
        {
            Arrow = 0, ///< a cylinder with an arrow head
            Cube = 1, ///< a cylinder with a cube head
            SquarePoint = 2 ///< a square
        };

        AZ_RTTI(LinearManipulator, "{4AA805DA-7D3C-4AFA-8110-EECF32B8F530}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(LinearManipulator, AZ::SystemAllocator, 0);

        explicit LinearManipulator(AZ::EntityId entityId);
        ~LinearManipulator();

        /**
         * This is the function signature of callbacks that will be invoked whenever a manipulator
         * is being clicked on or dragged.
         */
        using MouseActionCallback = AZStd::function<void(const LinearManipulationData&)>;

        void InstallMouseDownCallback(MouseActionCallback onMouseDownCallback);
        void InstallMouseMoveCallback(MouseActionCallback onMouseDownCallback);
        void InstallMouseUpCallback(MouseActionCallback onMouseDownCallback);

        void SetBoundsDirty() override;

        void OnMouseDown(const ViewportInteraction::MouseInteraction& interaction, float t) override;
        void OnMouseMove(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseUp(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseOver(ManipulatorId manipulatorId)  override;

        void Draw(AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState) override;

        /**
         * Set whether the size of the manipulator should maintain the same in the viewport when the camera zooms in and out.
         */
        void SetSizeFixedInScreen(bool isFixed) { m_isSizeFixedInScreen = isFixed; }

        void SetColor(const AZ::Color& color) { m_color = color; }
        void SetMouseOverColor(const AZ::Color& color) { m_mouseOverColor = color; }

        void SetPosition(const AZ::Vector3& pos) { m_origin = pos; }
        void SetDirection(const AZ::Vector3& dir) { m_direction = dir; }
        void SetLength(float length) { m_length = length; }
        void SetWidth(float width) { m_width = width; }
        void SetSquarePointSize(float size) { m_squarePointSize = size; }

        const AZ::Vector3& GetPosition() const { return m_origin; }

        void SetUpDirection(const AZ::Vector3& up) { m_upDir = up; }
        void SetSideReferenceDirection(const AZ::Vector3& side) { m_sideDir = side; }

        void SetDisplayType(DisplayType displayType) { m_displayType = displayType; }

    protected:

        void Invalidate() override;

    private:

        LinearManipulationData GetManipulationData(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection);

    private:

        /* Shape and Color */

        // in entity's local space
        AZ::Vector3 m_origin = AZ::Vector3::CreateZero(); 
        AZ::Vector3 m_direction = AZ::Vector3::CreateAxisX();
        AZ::Vector3 m_upDir = AZ::Vector3::CreateAxisZ(); ///< only used for DisplayType::Cube
        AZ::Vector3 m_sideDir = AZ::Vector3::CreateAxisY(); ///< only used for DisplayType::Cube
        AZ::Color m_color = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); 
        AZ::Color m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;

        bool m_isSizeFixedInScreen = true;

        // If m_isSizeFixedInScreen is true, the following variables define the manipulator's shape 
        // in screen space, otherwise they describe the manipulator in the local space of its owing 
        // component. 
        float m_length = 80.0f;
        float m_width = 6.0f;
        float m_squarePointSize = 2.5f;

        // The closer the manipulator is shown in the camera, the smaller this number is.
        float m_screenToWorldMultiplier = 1.0f;

        DisplayType m_displayType = DisplayType::Arrow;
            
        /* Hit Detection */
        
        union 
        {
            Picking::RegisteredBoundId m_cylinderBoundId = Picking::InvalidBoundId;
            Picking::RegisteredBoundId m_squarePointBoundId;
        };
        union
        {
            Picking::RegisteredBoundId m_coneBoundId = Picking::InvalidBoundId;
            Picking::RegisteredBoundId m_cubeBoundId;
        };
        
        /* Mouse Actions */

        float m_cameraFarClip = 1.0f;
        AZ::Vector3 m_manipulatorOriginWorld = AZ::Vector3::CreateZero();
        AZ::Vector3 m_manipulatorDirectionWorld = AZ::Vector3::CreateZero();
        AZ::Vector3 m_manipulatorPlaneNormalWorld = AZ::Vector3::CreateZero();

        AZ::Vector3 m_startHitWorldPosition = AZ::Vector3::CreateZero();
        AZ::Vector3 m_currentHitWorldPosition = AZ::Vector3::CreateZero();

        MouseActionCallback m_onMouseDownCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;
        MouseActionCallback m_onMouseUpCallback = nullptr;
    };

    /**
     * Return local space translation of manipulator.
     * @param manipulationData Current state of linear manipulator.
     * @param inverseTransform Inverse transform of entity manipulator exists on.
     */
    AZ_FORCE_INLINE AZ::Vector3 CalculateLocalOffsetFromOrigin(const LinearManipulationData& manipulationData, const AZ::Transform& inverseTransform)
    {
        return Vector4ToVector3(inverseTransform
            * Vector3ToVector4(manipulationData.m_manipulationWorldDirection)) * manipulationData.m_totalTranslationWorldDelta;
    }

} // namespace AzToolsFramework