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

#include <PhysX_precompiled.h>

#include "ColliderComponentMode.h"
#include "ColliderSubComponentMode.h"
#include "ColliderOffsetMode.h"
#include "ColliderRotationMode.h"
#include "ColliderBoxMode.h"
#include "ColliderSphereMode.h"
#include "ColliderCapsuleMode.h"
#include "ColliderAssetScaleMode.h"

#include <PhysX/EditorColliderComponentRequestBus.h>

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzToolsFramework/ComponentModes/BoxComponentMode.h>
#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace PhysX
{
    namespace
    {
        //! Uri's for shortcut actions.
        const AZ::Crc32 SetDimensionsSubModeActionUri = AZ_CRC("com.amazon.action.physx.setdimensionssubmode", 0x77b70dd6);
        const AZ::Crc32 SetOffsetSubModeActionUri = AZ_CRC("com.amazon.action.physx.setoffsetsubmode", 0xc06132e5);
        const AZ::Crc32 SetRotationSubModeActionUri = AZ_CRC("com.amazon.action.physx.setrotationsubmode", 0xc4225918);
        const AZ::Crc32 ResetSubModeActionUri = AZ_CRC("com.amazon.action.physx.resetsubmode", 0xb70b120e);
    }

    AZ_CLASS_ALLOCATOR_IMPL(ColliderComponentMode, AZ::SystemAllocator, 0);

    ColliderComponentMode::ColliderComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        CreateSubModes();
        ColliderComponentModeRequestBus::Handler::BusConnect(entityComponentIdPair);
    }

    ColliderComponentMode::~ColliderComponentMode()
    {
        ColliderComponentModeRequestBus::Handler::BusDisconnect();
        m_subModes[m_subMode]->Teardown(GetEntityComponentIdPair());
    }

    void ColliderComponentMode::Refresh()
    {
        m_subModes[m_subMode]->Refresh(GetEntityComponentIdPair());
    }

    AZStd::vector<AzToolsFramework::ActionOverride> ColliderComponentMode::PopulateActionsImpl()
    {

        AzToolsFramework::ActionOverride setDimensionsModeAction;
        setDimensionsModeAction.SetUri(SetDimensionsSubModeActionUri);
        setDimensionsModeAction.SetKeySequence(QKeySequence(Qt::Key_1));
        setDimensionsModeAction.SetTitle("Set Resize Mode");
        setDimensionsModeAction.SetTip("Set resize mode");
        setDimensionsModeAction.SetEntityComponentIdPair(GetEntityComponentIdPair());
        setDimensionsModeAction.SetCallback([this]()
        {
            SetCurrentMode(SubMode::Dimensions);
        });

        AzToolsFramework::ActionOverride setOffsetModeAction;
        setOffsetModeAction.SetUri(SetOffsetSubModeActionUri);
        setOffsetModeAction.SetKeySequence(QKeySequence(Qt::Key_2));
        setOffsetModeAction.SetTitle("Set Offset Mode");
        setOffsetModeAction.SetTip("Set offset mode");
        setOffsetModeAction.SetEntityComponentIdPair(GetEntityComponentIdPair());
        setOffsetModeAction.SetCallback([this]()
        {
            SetCurrentMode(SubMode::Offset);
        });

        AzToolsFramework::ActionOverride setRotationModeAction;
        setRotationModeAction.SetUri(SetRotationSubModeActionUri);
        setRotationModeAction.SetKeySequence(QKeySequence(Qt::Key_3));
        setRotationModeAction.SetTitle("Set Rotation Mode");
        setRotationModeAction.SetTip("Set rotation mode");
        setRotationModeAction.SetEntityComponentIdPair(GetEntityComponentIdPair());
        setRotationModeAction.SetCallback([this]()
        {
            SetCurrentMode(SubMode::Rotation);
        });

        AzToolsFramework::ActionOverride resetModeAction;
        resetModeAction.SetUri(ResetSubModeActionUri);
        resetModeAction.SetKeySequence(QKeySequence(Qt::Key_R));
        resetModeAction.SetTitle("Reset Current Mode");
        resetModeAction.SetTip("Reset current mode");
        resetModeAction.SetEntityComponentIdPair(GetEntityComponentIdPair());
        resetModeAction.SetCallback([this]()
        {
            ResetCurrentMode();
        });

        return {setDimensionsModeAction, setOffsetModeAction, setRotationModeAction, resetModeAction };
    }

    void ColliderComponentMode::CreateSubModes()
    {
        Physics::ShapeType shapeType = Physics::ShapeType::Box;
        EditorColliderComponentRequestBus::EventResult(
            shapeType, GetEntityComponentIdPair(), &EditorColliderComponentRequests::GetShapeType);

        switch (shapeType)
        {
        case Physics::ShapeType::Box:
            m_subModes[SubMode::Dimensions] = AZStd::make_unique<ColliderBoxMode>();
            break;
        case Physics::ShapeType::Sphere:
            m_subModes[SubMode::Dimensions] = AZStd::make_unique<ColliderSphereMode>();
            break;
        case Physics::ShapeType::Capsule:
            m_subModes[SubMode::Dimensions] = AZStd::make_unique<ColliderCapsuleMode>();
            break;
        case Physics::ShapeType::PhysicsAsset:
            m_subModes[SubMode::Dimensions] = AZStd::make_unique<ColliderAssetScaleMode>();
            break;
        }
        m_subModes[SubMode::Offset] = AZStd::make_unique<ColliderOffsetMode>();
        m_subModes[SubMode::Rotation] = AZStd::make_unique<ColliderRotationMode>();
        m_subModes[m_subMode]->Setup(GetEntityComponentIdPair());
    }

    bool ColliderComponentMode::HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Wheel &&
            mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl())
        {
            int direction = MouseWheelDelta(mouseInteraction) > 0.0f ? 1 : -1;
            AZ::u32 currentModeIndex = static_cast<AZ::u32>(m_subMode);
            AZ::u32 numSubModes = static_cast<AZ::u32>(SubMode::NumModes);
            AZ::u32 nextModeIndex = (currentModeIndex + numSubModes + direction) % m_subModes.size();
            SubMode nextMode = static_cast<SubMode>(nextModeIndex);
            SetCurrentMode(nextMode);
            return true;
        }
        return false;
    }

    ColliderComponentMode::SubMode ColliderComponentMode::GetCurrentMode()
    {
        return m_subMode;
    }

    void ColliderComponentMode::SetCurrentMode(SubMode newMode)
    {
        AZ_Assert(m_subModes.count(newMode) > 0, "Submode not found:%d", newMode);
        m_subModes[m_subMode]->Teardown(GetEntityComponentIdPair());
        m_subMode = newMode;
        m_subModes[m_subMode]->Setup(GetEntityComponentIdPair());
    }

    void RefreshUI()
    {
        /// The reason this is in a free function is because ColliderComponentMode
        /// privately inherits from ToolsApplicationNotificationBus. Trying to invoke
        /// the bus inside the class scope causes the compiler to complain it's not accessible 
        /// to due private inheritence. 
        /// Using the global namespace operator :: should have fixed that, except there 
        /// is a bug in the microsoft compiler meaning it doesn't work. So this is a work around.
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_Values);
    }

    void ColliderComponentMode::ResetCurrentMode()
    {
        m_subModes[m_subMode]->ResetValues(GetEntityComponentIdPair());
        m_subModes[m_subMode]->Refresh(GetEntityComponentIdPair()); 
        RefreshUI();
    }
}