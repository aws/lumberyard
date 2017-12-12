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

#include "ManipulatorBus.h"
#include "BaseManipulator.h"

namespace AzToolsFramework
{
    /**
     * This is an aggregation of manipulators used to modify each of a Transform's three components, namely scale, rotation and translation.
     */
    class TransformManipulator
        : private ManipulatorManagerNotificationBus::Handler
    {
    public:

        AZ_RTTI(TransformManipulator, "{B2006A9B-A502-45E8-A49C-D6C458A13E83}");
        AZ_CLASS_ALLOCATOR(TransformManipulator, AZ::SystemAllocator, 0);

        explicit TransformManipulator(AZ::EntityId entityId);
        ~TransformManipulator();

        void Register(AzToolsFramework::ManipulatorManagerId manipulatorManagerId);
        void Unregister();

        //////////////////////////////////////////////////////////////////////////
        /// ManipulatorManagerNotificationBus::Handler
        void OnTransformManipulatorModeChanged(TransformManipulatorMode previousMode, TransformManipulatorMode currentMode) override;
        //////////////////////////////////////////////////////////////////////////

        using LinearManipulatorMouseActionCallback = AZStd::function<void(const LinearManipulationData&)>;
        using PlanarManipulatorMouseActionCallback = AZStd::function<void(const PlanarManipulationData&)>;
        using AngularManipulatorMouseActionCallback = AZStd::function<void(const AngularManipulationData&)>;

        void InstallTranslationLinearManipulatorMouseDownCallback(LinearManipulatorMouseActionCallback onMouseDownCallback);
        void InstallTranslationLinearManipulatorMouseMoveCallback(LinearManipulatorMouseActionCallback onMouseMoveCallback);
        void InstallTranslationPlanarManipulatorMouseDownCallback(PlanarManipulatorMouseActionCallback onMouseDownCallback);
        void InstallTranslationPlanarManipulatorMouseMoveCallback(PlanarManipulatorMouseActionCallback onMouseMoveCallback);

        void InstallScaleLinearManipulatorMouseDownCallback(LinearManipulatorMouseActionCallback onMouseDownCallback);
        void InstallScaleLinearManipulatorMouseMoveCallback(LinearManipulatorMouseActionCallback onMouseMoveCallback);

        void InstallRotationAngularManipulatorMouseDownCallback(AngularManipulatorMouseActionCallback onMouseDownCallback);
        void InstallRotationAngularManipulatorMouseMoveCallback(AngularManipulatorMouseActionCallback onMouseMoveCallback);

        void SetBoundsDirty();

    private:

        void SwitchManipulators(TransformManipulatorMode mode);

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

        ManipulatorManagerId m_manipulatorManagerId;
    };
} // namespace AzToolsFramework