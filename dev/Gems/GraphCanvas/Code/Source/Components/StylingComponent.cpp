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

#include "precompiled.h"

#include <QGraphicsItem>
#include <QGraphicsLayoutItem>

#include <Components/StylingComponent.h>

namespace GraphCanvas
{
    /////////////////////
    // StylingComponent
    /////////////////////
    void StylingComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<StylingComponent>()
            ->Version(1)
            ->Field("Parent", &StylingComponent::m_parentStyledEntity)
            ->Field("Element", &StylingComponent::m_element)
            ->Field("Class", &StylingComponent::m_class)
            ->Field("Id", &StylingComponent::m_id)
            ;
    }

    AZ::EntityId StylingComponent::CreateStyleEntity(const AZStd::string& element)
    {
        AZ::Entity* entity = aznew AZ::Entity("Style");
        entity->CreateComponent<StylingComponent>(element);

        entity->Init();
        entity->Activate();

        return entity->GetId();
    }

    StylingComponent::StylingComponent(const AZStd::string& element, const AZ::EntityId& parentStyledEntity, const AZStd::string& styleClass, const AZStd::string& id)
        : m_parentStyledEntity(parentStyledEntity)
        , m_element(element)
        , m_class(styleClass)
        , m_id(id)
    {
    }

    void StylingComponent::Activate()
    {
        m_selectedSelector = Styling::Selector::Get(Styling::States::Selected);
        m_disabledSelector = Styling::Selector::Get(Styling::States::Disabled);
        m_hoveredSelector = Styling::Selector::Get(Styling::States::Hovered);
        m_collapsedSelector = Styling::Selector::Get(Styling::States::Collapsed);
        m_highlightedSelector = Styling::Selector::Get(Styling::States::Highlighted);

        Styling::Selector elementSelector = Styling::Selector::Get(m_element);
        AZ_Assert(elementSelector.IsValid(), "The item's element selector (\"%s\") is not valid", m_element.c_str());
        m_coreSelectors.emplace_back(elementSelector);

        Styling::Selector classSelector = Styling::Selector::Get(m_class);
        if (classSelector.IsValid())
        {
            m_coreSelectors.emplace_back(classSelector);
        }

        Styling::Selector idSelector = Styling::Selector::Get(m_id);
        if (idSelector.IsValid())
        {
            m_coreSelectors.emplace_back(idSelector);
        }

        StyledEntityRequestBus::Handler::BusConnect(GetEntityId());
        VisualNotificationBus::Handler::BusConnect(GetEntityId());
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void StylingComponent::Deactivate()
    {
        SceneMemberNotificationBus::Handler::BusDisconnect();
        VisualNotificationBus::Handler::BusDisconnect();
        StyledEntityRequestBus::Handler::BusDisconnect();

        m_selectedSelector = Styling::Selector();
        m_disabledSelector = Styling::Selector();
        m_hoveredSelector = Styling::Selector();
        m_collapsedSelector = Styling::Selector();
        m_highlightedSelector = Styling::Selector();
        m_coreSelectors.clear();
    }

    void StylingComponent::OnHoverEnter(QGraphicsItem * item)
    {
        m_hovered = true;
        StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
    }

    void StylingComponent::OnHoverLeave(QGraphicsItem* item)
    {
        QGraphicsItem* root = nullptr;
        RootVisualRequestBus::EventResult(root, GetEntityId(), &RootVisualRequests::GetRootGraphicsItem);

        if (!root || item == root)
        {
            m_hovered = false;
            StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
        }
    }

    void StylingComponent::OnItemChange(const AZ::EntityId&, QGraphicsItem::GraphicsItemChange change, const QVariant&)
    {
        if (change == QGraphicsItem::GraphicsItemChange::ItemSelectedHasChanged)
        {
            StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
        }
    }

    AZ::EntityId StylingComponent::GetStyleParent() const
    {
        return m_parentStyledEntity;
    }

    Styling::SelectorVector StylingComponent::GetStyleSelectors() const
    {
        Styling::SelectorVector selectors = m_coreSelectors;

        QGraphicsItem* root = nullptr;
        RootVisualRequestBus::EventResult(root, GetEntityId(), &RootVisualRequests::GetRootGraphicsItem);

        if (!root)
        {
            return selectors;
        }

        if (m_hovered)
        {
            selectors.emplace_back(m_hoveredSelector);
        }

        if (root->isSelected())
        {
            selectors.emplace_back(m_selectedSelector);
        }

        if (!root->isEnabled())
        {
            selectors.emplace_back(m_disabledSelector);
        }

        // TODO collapsed and highlighted

        for (auto& mapPair : m_dynamicSelectors)
        {
            selectors.emplace_back(mapPair.second);
        }

        return selectors;
    }

    void StylingComponent::AddSelectorState(const char* selectorState)
    {
        auto insertResult = m_dynamicSelectors.insert(AZStd::pair<AZ::Crc32, Styling::Selector>(AZ::Crc32(selectorState), Styling::Selector::Get(selectorState)));

        if (insertResult.second)
        {
            StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
        }
        else
        {
            AZ_Assert(false, "Pushing the same state(%s) onto the selector stack twice. State cannot be correctly removed.", selectorState);
        }
    }

    void StylingComponent::RemoveSelectorState(const char* selectorState)
    {
        AZStd::size_t count = m_dynamicSelectors.erase(AZ::Crc32(selectorState));

        if (count != 0)
        {
            StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
        }
    }

    AZStd::string StylingComponent::GetElement() const
    {
        return m_element;
    }

    AZStd::string StylingComponent::GetClass() const
    {
        return m_class;
    }

    AZStd::string StylingComponent::GetId() const
    {
        return m_id;
    }

    void StylingComponent::OnSceneSet(const AZ::EntityId& scene)
    {
        SceneNotificationBus::Handler::BusConnect(scene);
        StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
    }

    void StylingComponent::OnSceneCleared(const AZ::EntityId& scene)
    {
        SceneNotificationBus::Handler::BusDisconnect();
    }

    void StylingComponent::OnStyleSheetChanged()
    {
        StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
    }
}