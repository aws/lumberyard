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
#include "EntityIdQLabel.hxx"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>

#include <QMouseEvent>

namespace AzToolsFramework
{
    EntityIdQLabel::EntityIdQLabel(QWidget* parent)
        : QLabel(parent)
    {
    }

    EntityIdQLabel::~EntityIdQLabel()
    {
    }

    void EntityIdQLabel::SetEntityId(AZ::EntityId newId, const AZStd::string_view& nameOverride)
    {
        m_entityId = newId;
        if (!m_entityId.IsValid())
        {
            setText(QString());
        }
        else
        {
            AZ::Entity* pEntity = NULL;
            EBUS_EVENT_RESULT(pEntity, AZ::ComponentApplicationBus, FindEntity, m_entityId);
            if (pEntity && nameOverride.empty())
            {
                setText(pEntity->GetName().c_str());
            }
            else if (!nameOverride.empty())
            {
                setText(nameOverride.data());
            }
            else
            {
                setText(tr("(Entity not found)"));
            }
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
        QLabel::mouseDoubleClickEvent(e);
        EBUS_EVENT(AzToolsFramework::EntityHighlightMessages::Bus, EntityStrongHighlightRequested, m_entityId);
        // The above EBus request calls EntityPropertyEditor::UpdateContents() down the code path, which in turn destroys 
        // the current EntityIdQLabel object, therefore calling any method of EntityIdQLabel at this point is invalid.
    }
}

#include <UI/PropertyEditor/EntityIdQLabel.moc>