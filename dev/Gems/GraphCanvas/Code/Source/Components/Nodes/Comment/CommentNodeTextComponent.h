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

#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsWidget>
#include <QPlainTextEdit>
#include <QTimer>

#include <AzCore/Component/Component.h>

#include <Components/Nodes/General/GeneralNodeLayoutComponent.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <Widgets/GraphCanvasLabel.h>
#include <Widgets/NodePropertyDisplayWidget.h>

class QGraphicsGridLayout;

namespace GraphCanvas
{
    class CommentTextGraphicsWidget;

    class CommentNodeTextComponent
        : public GraphCanvasPropertyComponent
        , public NodeNotificationBus::Handler
        , public CommentRequestBus::Handler
        , public CommentLayoutRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CommentNodeTextComponent, "{15C568B0-425C-4655-814D-0A299341F757}", GraphCanvasPropertyComponent);

        static void Reflect(AZ::ReflectContext*);

        CommentNodeTextComponent();
        ~CommentNodeTextComponent() = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GraphCanvas_CommentTextService", 0xb650db99));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incombatible)
        {
            incombatible.push_back(AZ_CRC("GraphCanvas_CommentTextService", 0xb650db99));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_StyledGraphicItemService", 0xeae4cdf4));
            required.push_back(AZ_CRC("GraphCanvas_SceneMemberService", 0xe9759a2d));
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // NodeNotification
        void OnAddedToScene(const AZ::EntityId&);
        ////

        // CommentRequestBus
        void SetComment(const AZStd::string& comment) override;
        const AZStd::string& GetComment() const override;        
        ////

        // CommentLayoutRequestBus
        QGraphicsLayoutItem* GetGraphicsLayoutItem() override;
        ////

        void OnCommentChanged();

    private:
        CommentNodeTextComponent(const CommentNodeTextComponent&) = delete;

        AZStd::string m_comment;
        CommentTextGraphicsWidget* m_commentTextWidget;
    };
}