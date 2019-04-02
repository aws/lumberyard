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
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>

namespace AzToolsFramework
{
    /**
     * ScaleManipulators is an aggregation of 3 linear manipulators for each basis axis who share
     * the same transform, and a single linear manipulator at the center of the transform whose
     * axis is world up (z).
     */
    class ScaleManipulators
        : public Manipulators
    {
    public:
        AZ_RTTI(ScaleManipulators, "{5D1F1D47-1D5B-4E42-B47E-23F108F8BF7D}")
        AZ_CLASS_ALLOCATOR(ScaleManipulators, AZ::SystemAllocator, 0)

        ScaleManipulators(AZ::EntityId entityId, const AZ::Transform& worldFromLocal);

        void InstallAxisLeftMouseDownCallback(const LinearManipulator::MouseActionCallback& onMouseDownCallback);
        void InstallAxisMouseMoveCallback(const LinearManipulator::MouseActionCallback& onMouseMoveCallback);
        void InstallAxisLeftMouseUpCallback(const LinearManipulator::MouseActionCallback& onMouseUpCallback);

        void InstallUniformLeftMouseDownCallback(const LinearManipulator::MouseActionCallback& onMouseDownCallback);
        void InstallUniformMouseMoveCallback(const LinearManipulator::MouseActionCallback& onMouseMoveCallback);
        void InstallUniformLeftMouseUpCallback(const LinearManipulator::MouseActionCallback& onMouseUpCallback);

        void SetLocalTransform(const AZ::Transform& localTransform);
        void SetSpace(const AZ::Transform& worldFromLocal);

        void SetAxes(
            const AZ::Vector3& axis1,
            const AZ::Vector3& axis2,
            const AZ::Vector3& axis3);

        void ConfigureView(
            float axisLength,
            const AZ::Color& axis1Color,
            const AZ::Color& axis2Color,
            const AZ::Color& axis3Color);

    private:
        AZ_DISABLE_COPY_MOVE(ScaleManipulators)

        // Manipulators
        void ProcessManipulators(const AZStd::function<void(BaseManipulator*)>&) override;

        AZStd::vector<AZStd::unique_ptr<LinearManipulator>> m_axisScaleManipulators;
        AZStd::unique_ptr<LinearManipulator> m_uniformScaleManipulator;
    };
} // namespace AzToolsFramework