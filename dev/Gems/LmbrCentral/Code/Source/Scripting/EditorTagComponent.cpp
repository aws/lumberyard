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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include "EditorTagComponent.h"
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    //=========================================================================
    // Component Descriptor
    //=========================================================================
    void EditorTagComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorTagComponent, AZ::Component>()
                ->Version(1)
                ->Field("Tags", &EditorTagComponent::m_tags);


            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorTagComponent>("Tag", "The Tag component allows you to apply one or more labels to an entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Tag.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Tag.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorTagComponent::m_tags, "Tags", "The tags that will be on this entity by default");
            }
        }
    }

    //=========================================================================
    // AZ::Component
    //=========================================================================
    void EditorTagComponent::Activate()
    {
        EditorComponentBase::Activate();
    }

    void EditorTagComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();
    }

    //=========================================================================
    // AzToolsFramework::Components::EditorComponentBase
    //=========================================================================
    void EditorTagComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (TagComponent* tagComponent = gameEntity->CreateComponent<TagComponent>())
        {
            tagComponent->EditorSetTags(m_tags);
        }
    }

} // namespace LmbrCentral
