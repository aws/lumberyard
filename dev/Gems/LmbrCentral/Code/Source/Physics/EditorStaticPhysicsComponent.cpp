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
#include "StdAfx.h"
#include "EditorStaticPhysicsComponent.h"

#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void EditorStaticPhysicsConfiguration::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorStaticPhysicsConfiguration, StaticPhysicsConfiguration>()
                ->Version(1)
            ;
            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<StaticPhysicsConfiguration>(
                    "Static Physics Configuration", "Configuration for Static physics object")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->DataElement(0, &StaticPhysicsConfiguration::m_enabledInitially, "Enabled initially", "Whether the entity is initially enabled in the physics simulation.")
                ;
            }
        }
    }

    void EditorStaticPhysicsComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorStaticPhysicsConfiguration::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorStaticPhysicsComponent, EditorPhysicsComponent>()
                ->Field("Configuration", &EditorStaticPhysicsComponent::m_configuration)
                ->Version(1)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorStaticPhysicsComponent>(
                    "Static Physics", "The Static Mesh component is the primary method of adding static visual geometry to entities")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/StaticPhysics.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/StaticPhysics.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorStaticPhysicsComponent::m_configuration,
                        "Configuration", "Static physics configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    EditorStaticPhysicsComponent::EditorStaticPhysicsComponent(const EditorStaticPhysicsConfiguration& configuration)
        : m_configuration(configuration)
    {
    }

    void EditorStaticPhysicsComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<StaticPhysicsComponent>(m_configuration);
    }

} // namespace LmbrCentral