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
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/BaseManipulator.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>

namespace AzToolsFramework
{
    /**
     * This is an aggregation of manipulators used to modify each of a Transform's
     * three components, namely scale, rotation and translation.
     */
    class TransformManipulator
        : private ManipulatorManagerNotificationBus::Handler
    {
    public:
        AZ_RTTI(TransformManipulator, "{B2006A9B-A502-45E8-A49C-D6C458A13E83}");
        AZ_CLASS_ALLOCATOR(TransformManipulator, AZ::SystemAllocator, 0);

        enum class Mode
        {
            Translation,
            Scale,
            Rotation
        };

        explicit TransformManipulator(AZ::EntityId entityId);
        ~TransformManipulator();

        void Register(ManipulatorManagerId manipulatorManagerId);
        void Unregister();

        void InstallTranslationLinearManipulatorLeftMouseDownCallback(LinearManipulator::MouseActionCallback onMouseDownCallback);
        void InstallTranslationLinearManipulatorMouseMoveCallback(LinearManipulator::MouseActionCallback onMouseMoveCallback);
        void InstallTranslationPlanarManipulatorLeftMouseDownCallback(PlanarManipulator::MouseActionCallback onMouseDownCallback);
        void InstallTranslationPlanarManipulatorMouseMoveCallback(PlanarManipulator::MouseActionCallback onMouseMoveCallback);
        void InstallScaleLinearManipulatorLeftMouseDownCallback(LinearManipulator::MouseActionCallback onMouseDownCallback);
        void InstallScaleLinearManipulatorMouseMoveCallback(LinearManipulator::MouseActionCallback onMouseMoveCallback);
        void InstallRotationAngularManipulatorLeftMouseDownCallback(AngularManipulator::MouseActionCallback onMouseDownCallback);
        void InstallRotationAngularManipulatorMouseMoveCallback(AngularManipulator::MouseActionCallback onMouseMoveCallback);

        void SetBoundsDirty();

    private:
        void SwitchManipulators(Mode mode);

        const static int s_translationLinearManipulatorBeginIndex = 0;
        const static int s_translationLinearManipulatorEndIndex = 3;
        const static int s_translationPlanarManipulatorBeginIndex = 3;
        const static int s_translationPlanarManipulatorEndIndex = 6;
        const static int s_translationManipulatorBeginIndex = 0;
        const static int s_translationManipulatorEndIndex = 6;
        const static int s_scaleLinearManipulatorBeginIndex = 6;
        const static int s_scaleLinearManipulatorEndIndex = 9;
        const static int s_rotationAngularManipulatorBeginIndex = 9;
        const static int s_rotationAngularManipulatorEndIndex = 12;
        const static int s_manipulatorCount = 12;

        AZStd::vector<AZStd::unique_ptr<BaseManipulator>> m_manipulators;

        ManipulatorManagerId m_manipulatorManagerId = InvalidManipulatorManagerId;
    };
} // namespace AzToolsFramework