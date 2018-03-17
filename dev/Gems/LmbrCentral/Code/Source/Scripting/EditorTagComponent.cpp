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
#include "LmbrCentral_precompiled.h"

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
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-tag.html")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorTagComponent::m_tags, "Tags", "The tags that will be on this entity by default");
            }
        }
    }

    //=========================================================================
    // EditorTagComponentRequestBus::Handler
    //=========================================================================
    bool EditorTagComponent::HasTag(const char* tag)
    {
        return AZStd::find(m_tags.begin(), m_tags.end(), tag) != m_tags.end();
    }

    void EditorTagComponent::AddTag(const char* tag)
    {
        if (!HasTag(tag))
        {
            m_tags.push_back(tag);
            EBUS_EVENT_ID(GetEntityId(), TagComponentNotificationsBus, OnTagAdded, Tag(tag));
            EBUS_EVENT_ID(Tag(tag), TagGlobalNotificationBus, OnEntityTagAdded, GetEntityId());
            TagGlobalRequestBus::MultiHandler::BusConnect(Tag(tag));
        }
    }

    void EditorTagComponent::RemoveTag(const  char* tag)
    {
        AZStd::size_t prevSize = m_tags.size();
        m_tags.erase(AZStd::remove_if(m_tags.begin(), m_tags.end(), [tag](const AZStd::string& target) { return target == tag; }));
        if (m_tags.size() != prevSize)
        {
            EBUS_EVENT_ID(GetEntityId(), TagComponentNotificationsBus, OnTagRemoved, Tag(tag));
            EBUS_EVENT_ID(Tag(tag), TagGlobalNotificationBus, OnEntityTagRemoved, GetEntityId());
            TagGlobalRequestBus::MultiHandler::BusDisconnect(Tag(tag));
        }

    }
    //////////////////////////////////////////////////////////////////////////

    //=========================================================================
    // AZ::Component
    //=========================================================================
    void EditorTagComponent::Activate()
    {
        EditorComponentBase::Activate();

        for (const AZStd::string& tag : m_tags)
        {
            TagGlobalRequestBus::MultiHandler::BusConnect(Tag(tag.c_str()));
            EBUS_EVENT_ID(Tag(tag.c_str()), TagGlobalNotificationBus, OnEntityTagAdded, GetEntityId());
        }
        EditorTagComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void EditorTagComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();

        EditorTagComponentRequestBus::Handler::BusDisconnect();
        for (const AZStd::string& tag : m_tags)
        {
            TagGlobalRequestBus::MultiHandler::BusDisconnect(Tag(tag.c_str()));
            EBUS_EVENT_ID(Tag(tag.c_str()), TagGlobalNotificationBus, OnEntityTagRemoved, GetEntityId());
        }
    }

    //=========================================================================
    // AzToolsFramework::Components::EditorComponentBase
    //=========================================================================
    void EditorTagComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (TagComponent* tagComponent = gameEntity->CreateComponent<TagComponent>())
        {
            Tags newTagList;
            for (const AZStd::string& tag : m_tags)
            {
                newTagList.insert(Tag(tag.c_str()));
            }
            tagComponent->EditorSetTags(AZStd::move(newTagList));
        }
    }

} // namespace LmbrCentral
