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

#include <AzToolsFramework/Manipulators/LinearManipulator.h>

namespace AzToolsFramework
{
    /**
     * Interface for handling box manipulator updates.
     */
    class BoxManipulatorHandler
    {
    public:
        virtual ~BoxManipulatorHandler() = default;
        virtual AZ::Vector3 GetDimensions() = 0;
        virtual void SetDimensions(const AZ::Vector3& dimensions) = 0;
        virtual AZ::Transform GetCurrentTransform() = 0;
    };

    /**
     * Class that encapsulates linear manipulators for editing box dimensions.
     * A handler should be set that provides the underlying dimensions being
     * manipulated.
     */
    class BoxManipulator
    {
    public:
        void OnSelect(AZ::EntityId entityId);
        void OnDeselect();
        void SetHandler(BoxManipulatorHandler* handler);
        void RefreshManipulators();

    private:
        void CreateManipulators();
        void DestroyManipulators();
        void UpdateManipulators();
        void OnMouseMoveManipulator(const AzToolsFramework::LinearManipulator::Action& action);

        AZ::EntityId m_entityId; ///< The entity id of the selected object.
        AZStd::array<AZStd::shared_ptr<AzToolsFramework::LinearManipulator>, 6> m_linearManipulators; ///< Manipulators for editing box size.
        BoxManipulatorHandler* m_handler = nullptr; ///< Callback to invoke when the manipulators change the dimensions.
    };
}

