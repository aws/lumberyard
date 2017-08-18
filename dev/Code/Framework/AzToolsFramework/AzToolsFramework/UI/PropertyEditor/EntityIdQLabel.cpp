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
#include "stdafx.h"
#include "EntityIdQLabel.hxx"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>

namespace AzToolsFramework
{
    EntityIdQLabel::EntityIdQLabel(QWidget* parent)
        : QLabel(parent)
    {
    }

    EntityIdQLabel::~EntityIdQLabel()
    {
    }

    void EntityIdQLabel::SetEntityId(AZ::EntityId newId)
    {
        m_entityId = newId;

        AZ::Entity* pEntity = NULL;
        EBUS_EVENT_RESULT(pEntity, AZ::ComponentApplicationBus, FindEntity, m_entityId);
        if (pEntity)
        {
            setText(pEntity->GetName().c_str());
        }
        else if(newId.IsValid())
        {
            setText(tr("(Entity not found)"));
        }
        else
        {
            setText(QString());
        }
    }

    void EntityIdQLabel::mousePressEvent(QMouseEvent* e)
    {
        if (e->modifiers() & Qt::ControlModifier)
        {
            emit RequestPickObject();
        }
        else
        {
            EBUS_EVENT(AzToolsFramework::EntityHighlightMessages::Bus, EntityHighlightRequested, m_entityId);
        }

        QLabel::mousePressEvent(e);
    }

    void EntityIdQLabel::mouseDoubleClickEvent(QMouseEvent* e)
    {
        EBUS_EVENT(AzToolsFramework::EntityHighlightMessages::Bus, EntityStrongHighlightRequested, m_entityId);
        QLabel::mouseDoubleClickEvent(e);
    }
}

#include <UI/PropertyEditor/EntityIdQLabel.moc>