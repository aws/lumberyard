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

#include <functional>

#include <QGraphicsItem>
#include <QGraphicsGridLayout>
#include <QGraphicsLayoutItem>
#include <QGraphicsScene>
#include <QGraphicsWidget>
#include <QPainter>

#include <AzCore/RTTI/TypeInfo.h>

#include <Components/Nodes/Comment/CommentNodeTextComponent.h>

#include <Components/Nodes/Comment/CommentTextGraphicsWidget.h>
#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <Components/Nodes/NodeBus.h>
#include <Components/Slots/SlotBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/tools.h>
#include <Styling/StyleHelper.h>

namespace GraphCanvas
{
    /////////////////////////////
    // CommentNodeTextComponent
    /////////////////////////////
    void CommentNodeTextComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CommentNodeTextComponent>()
                ->Version(1)
                ->Field("Comment", &CommentNodeTextComponent::m_comment)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<CommentNodeTextComponent>("Comment", "The node's customizable properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CommentNodeTextComponent::m_comment, "Comment", "The comment to display on this node")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CommentNodeTextComponent::OnCommentChanged)
                    ;
            }
        }
    }

    CommentNodeTextComponent::CommentNodeTextComponent()
        : m_commentTextWidget(nullptr)
    {
    }

    void CommentNodeTextComponent::Init()
    {
        GraphCanvasPropertyComponent::Init();
    }

    void CommentNodeTextComponent::Activate()
    {
        if (m_commentTextWidget == nullptr)
        {
            m_commentTextWidget = aznew CommentTextGraphicsWidget(GetEntityId());
            m_commentTextWidget->SetStyle(Styling::Elements::CommentText);
        }

        GraphCanvasPropertyComponent::Activate();

        CommentRequestBus::Handler::BusConnect(GetEntityId());
        CommentLayoutRequestBus::Handler::BusConnect(GetEntityId());

        NodeNotificationBus::Handler::BusConnect(GetEntityId());

        m_commentTextWidget->Activate();
    }

    void CommentNodeTextComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        CommentLayoutRequestBus::Handler::BusDisconnect();
        CommentRequestBus::Handler::BusDisconnect();

        m_commentTextWidget->Deactivate();
    }

    void CommentNodeTextComponent::OnAddedToScene(const AZ::EntityId&)
    {
        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetSnapToGrid, false);

        m_commentTextWidget->OnAddedToScene();
        OnCommentChanged();
    }

    void CommentNodeTextComponent::SetComment(const AZStd::string& comment)
    {
        if (m_comment.compare(comment) != 0)
        {
            m_comment = comment;
            m_commentTextWidget->SetComment(m_comment);

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);
            SceneUIRequestBus::Event(sceneId, &SceneUIRequests::RequestUndoPoint);
        }
    }

    const AZStd::string& CommentNodeTextComponent::GetComment() const
    {
        return m_comment;
    }

    QGraphicsLayoutItem* CommentNodeTextComponent::GetGraphicsLayoutItem()
    {
        return m_commentTextWidget;
    }

    void CommentNodeTextComponent::OnCommentChanged()
    {
        if (m_commentTextWidget)
        {
            m_commentTextWidget->SetComment(m_comment);
        }
    }
}