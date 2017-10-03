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

#include <AzCore/Component/EntityId.h>

#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <Widgets/GraphCanvasLabel.h>
#include <Styling/StyleHelper.h>

namespace GraphCanvas
{
    namespace Internal
    {
        // Need to know when the text edit gets focus in order to
        // manage the layout display when the mouse hovers off, but the
        // widget still has focus. Qt does not expose focus events in any
        // signal way, so this exposes that functionality for me.
        class FocusableTextEdit
            : public QTextEdit
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(FocusableTextEdit, AZ::SystemAllocator, 0);
            FocusableTextEdit() = default;
            ~FocusableTextEdit() = default;

        signals:
            void OnFocusIn();
            void OnFocusOut();

        private:

            void focusInEvent(QFocusEvent* focusEvent)
            {
                QTextEdit::focusInEvent(focusEvent);
                emit OnFocusIn();
            }

            void focusOutEvent(QFocusEvent* focusEvent)
            {
                QTextEdit::focusOutEvent(focusEvent);
                emit OnFocusOut();
            }
        };
    }

    //! The QGraphicsWidget for displaying the comment text
    // This class is not meant to be serializable
    class CommentTextGraphicsWidget
        : public QGraphicsWidget
        , public CommentLayoutRequestBus::Handler
        , public StyleNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(CommentTextGraphicsWidget, "{1779F401-6A9F-42A8-B4B7-F7732DBEC462}");
        AZ_CLASS_ALLOCATOR(CommentTextGraphicsWidget, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context) = delete;

        CommentTextGraphicsWidget(const AZ::EntityId& targetId);
        ~CommentTextGraphicsWidget() override = default;

        void Activate();
        void Deactivate();

        void OnAddedToScene();

        void SetComment(const AZStd::string& comment);
        AZStd::string GetComment() const;
        
        void SetStyle(const AZStd::string& style);

        void UpdateLayout();

        void UpdateStyles();
        void RefreshDisplay();

        // CommentLayoutRequestBus
        QGraphicsLayoutItem* GetGraphicsLayoutItem() override;
        ////
		
		// StyleNotificationBus
        void OnStyleChanged() override;
        ////

    protected:

        void SetEditable(bool editable);

        void UpdateSizing();
        void SubmitValue();
        
        bool sceneEventFilter(QGraphicsItem*, QEvent* event);

        const AZ::EntityId& GetEntityId() const { return m_entityId; }

    private:
        CommentTextGraphicsWidget(const CommentTextGraphicsWidget&) = delete;

        bool m_editable;
        bool m_layoutLock;

        QGraphicsLinearLayout* m_layout;

        GraphCanvasLabel*               m_displayLabel;
        Internal::FocusableTextEdit*    m_textEdit;
        QGraphicsProxyWidget*           m_proxyWidget;

        Styling::StyleHelper m_styleHelper;
        AZStd::string        m_style;
        
        QPointF m_initialClick;
        bool m_pressed;

        QTimer m_timer;

        AZ::EntityId m_entityId;
    };
}