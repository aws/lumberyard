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
#include "Water_precompiled.h"

#include "WaterOceanEditor.h"
#include <Water/WaterOceanComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <LmbrCentral/Physics/WaterNotificationBus.h>

#include "QtUtil.h"

/**
  * An editor component that prepares a component for the ocean
  * The authored runtime component is created during the BuildGameEntity() method
  */
namespace Water
{
    // static
    void WaterOceanEditor::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<WaterOceanEditor, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("data", &WaterOceanEditor::m_data)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<WaterOceanEditor>("Infinite Ocean", "Provides an infinite ocean.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Environment")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/InfiniteOcean.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/InfiniteOcean.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/console/lumberyard/userguide/infinite-ocean-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterOceanEditor::m_data, "m_data", "m_data desc")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
                    ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<WaterOceanEditor>()->RequestBus("OceanEnvironmentRequestBus");
        }
    }

    WaterOceanEditor::WaterOceanEditor()
    {
    }

    WaterOceanEditor::~WaterOceanEditor()
    {
    }

    // static
    void WaterOceanEditor::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("WaterOceanEditor", 0x487c0aa8));
        provided.push_back(AZ_CRC("WaterOceanService", 0x12a06661));
    }

    // static
    void WaterOceanEditor::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("WaterOceanEditor", 0x487c0aa8));
        incompatible.push_back(AZ_CRC("WaterOceanService", 0x12a06661));
    }

    // static
    void WaterOceanEditor::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    /**
      * Used when going into game mode from the editor, or when the game entity is being created for exporting a game package
      */      
    void WaterOceanEditor::BuildGameEntity(AZ::Entity* gameEntity)
    {
        const bool isActive = GetEntity() ? (GetEntity()->GetState() == AZ::Entity::ES_ACTIVE) : false;
        if (isActive)
        {
            m_data.Deactivate();
        }

        gameEntity->CreateComponent<Water::WaterOceanComponent>(m_data);

        if (isActive)
        {
            m_data.Activate();
        }
    }

    // called when Editor activates the component when loading a level OR when coming back from game mode
    void WaterOceanEditor::Activate()
    {
        m_data.Activate();

        // the ocean is visible if the entity (in editing mode) is also visible
        bool entityVisible = true;
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(entityVisible, GetEntityId(), &AzToolsFramework::EditorVisibilityRequests::GetCurrentVisibility);
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(GetEntityId());
        OnEntityVisibilityChanged(entityVisible);

        AzToolsFramework::Components::EditorComponentBase::Activate();

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

        AZ::Transform worldTransform = AZ::Transform::Identity();
        EBUS_EVENT_ID_RESULT(worldTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);
        m_data.SetOceanLevel(worldTransform.GetPosition().GetZ());

        // all data has been loaded and ocean height has been set, so notify the Editor that things have changed.
        GetIEditor()->SetModifiedFlag();
        GetIEditor()->SetModifiedModule(eModifiedTerrain);
        GetIEditor()->Notify(eNotify_OnTerrainRebuild);
        GetIEditor()->RegisterNotifyListener(this);
    }

    // called when Editor de-activates the component when going into Game Mode
    void WaterOceanEditor::Deactivate()
    {
        m_data.Deactivate();

        GetIEditor()->UnregisterNotifyListener(this);
        GetIEditor()->SetModifiedFlag();
        GetIEditor()->SetModifiedModule(eModifiedTerrain);
        GetIEditor()->Notify(eNotify_OnTerrainRebuild);

        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
    }

    void WaterOceanEditor::OnEditorNotifyEvent(EEditorNotifyEvent e)
    {
        if (eNotify_OnTerrainRebuild == e)
        {
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
        }
        else if (eNotify_OnEndGameMode == e)
        {
            AZ::Transform worldTransform = AZ::Transform::Identity();
            EBUS_EVENT_ID_RESULT(worldTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);
            m_data.SetOceanLevel(worldTransform.GetPosition().GetZ());
        }
    }

    void WaterOceanEditor::OnEntityVisibilityChanged(bool visibility)
    {
        gEnv->p3DEngine->EnableOceanRendering(visibility);
    }

    void WaterOceanEditor::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_data.SetOceanLevel(world.GetPosition().GetZ());
        LmbrCentral::WaterNotificationBus::Broadcast(&LmbrCentral::WaterNotificationBus::Events::OceanHeightChanged, world.GetPosition().GetZ());
    }
}
