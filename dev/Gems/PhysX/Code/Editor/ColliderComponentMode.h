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

#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include "ColliderComponentModeBus.h"

namespace PhysX
{
    class ColliderSubComponentMode;

    //! ComponentMode for the Collider Component - Manages a list of Sub-Component Modes and 
    //! is responsible for switching between and activating them.
    class ColliderComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , public ColliderComponentModeRequestBus::Handler
    {
    public:

        AZ_CLASS_ALLOCATOR_DECL;

        ColliderComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~ColliderComponentMode();

        // EditorBaseComponentMode ...
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;

        // ColliderComponentModeBus ...
        SubMode GetCurrentMode() override;
        void SetCurrentMode(SubMode index) override;

    private:
        
        // AzToolsFramework::ViewportInteraction::ViewportSelectionRequests ...
        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        void CreateSubModes();
        void ResetCurrentMode();

        AZStd::unordered_map<SubMode, AZStd::unique_ptr<ColliderSubComponentMode>> m_subModes;
        SubMode m_subMode = SubMode::Dimensions;
    };
} // namespace PhysX