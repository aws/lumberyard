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

#include <QEvent>
#include <QGraphicsItem>
#include <QGraphicsGridLayout>
#include <QGraphicsLayoutItem>
#include <QGraphicsScene>
#include <qgraphicssceneevent.h>
#include <QGraphicsWidget>
#include <QPainter>

#include <Components/Nodes/Comment/CommentTextGraphicsWidget.h>

#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/tools.h>

namespace GraphCanvas
{
    //////////////////////////////////
    // CommentNodeTextGraphicsWidget
    //////////////////////////////////
    CommentTextGraphicsWidget::CommentTextGraphicsWidget(const AZ::EntityId& targetId)
        : m_editable(false)
        , m_layoutLock(false)
        , m_pressed(false)
        , m_entityId(targetId)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setGraphicsItem(this);
        setFlag(ItemIsMovable, false);

        m_displayLabel = aznew GraphCanvasLabel();
        m_displayLabel->SetElide(false);
        m_displayLabel->SetWrap(true);
        m_displayLabel->setAcceptedMouseButtons(Qt::MouseButton::LeftButton);        

        m_proxyWidget = new QGraphicsProxyWidget();
        m_proxyWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);

        m_timer.setSingleShot(true);
        m_timer.setInterval(100);
        m_timer.stop();

        QObject::connect(&m_timer, &QTimer::timeout, [&]() { 
            m_textEdit->setFocus(Qt::FocusReason::MouseFocusReason);
            m_proxyWidget->setFocus(Qt::FocusReason::MouseFocusReason);
        });

        m_textEdit = aznew Internal::FocusableTextEdit();
        m_textEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_textEdit->setProperty("HasNoWindowDecorations", true);
        m_textEdit->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
        m_textEdit->setWordWrapMode(QTextOption::WrapMode::WordWrap);

        m_textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        m_textEdit->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);

        m_textEdit->setEnabled(true);

        QObject::connect(m_textEdit, &Internal::FocusableTextEdit::textChanged, [this]() { this->UpdateSizing(); });
        QObject::connect(m_textEdit, &Internal::FocusableTextEdit::OnFocusIn,   [this]() { this->m_layoutLock = true; });
        QObject::connect(m_textEdit, &Internal::FocusableTextEdit::OnFocusOut, [this]() { this->SubmitValue(); this->m_layoutLock = false; this->SetEditable(false); });

        m_proxyWidget->setWidget(m_textEdit);

        m_layout = new QGraphicsLinearLayout(Qt::Vertical);
        m_layout->setSpacing(0);

        m_layout->addItem(m_displayLabel);

        setLayout(m_layout);
        setData(GraphicsItemName, QStringLiteral("Comment/%1").arg(static_cast<AZ::u64>(GetEntityId()), 16, 16, QChar('0')));
    }

    void CommentTextGraphicsWidget::Activate()
    {
        CommentLayoutRequestBus::Handler::BusConnect(GetEntityId());
        StyleNotificationBus::Handler::BusConnect(GetEntityId());

        UpdateLayout();
    }

    void CommentTextGraphicsWidget::Deactivate()
    {
        StyleNotificationBus::Handler::BusDisconnect();
        CommentLayoutRequestBus::Handler::BusDisconnect();
    }

    void CommentTextGraphicsWidget::OnAddedToScene()
    {
        m_displayLabel->installSceneEventFilter(this);
    }

    void CommentTextGraphicsWidget::SetComment(const AZStd::string& comment)
    {
        if (!comment.empty())
        {
            m_displayLabel->SetLabel(comment);
            m_textEdit->setPlainText(comment.c_str());
        }
        else
        {
            // Hack to force the minimum height based on the style's font/size
            m_displayLabel->SetLabel(" ");
        }

        UpdateSizing();
    }

    AZStd::string CommentTextGraphicsWidget::GetComment() const
    {
        return m_textEdit->toPlainText().toUtf8().data();
    }
    
    void CommentTextGraphicsWidget::SetStyle(const AZStd::string& style)
    {
        m_style = style;
        OnStyleChanged();
    }

    void CommentTextGraphicsWidget::UpdateLayout()
    {
        if (m_layoutLock)
        {
            return;
        }

        QGraphicsScene* graphicsScene = nullptr;
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        SceneRequestBus::EventResult(graphicsScene, sceneId, &SceneRequests::AsQGraphicsScene);

        for (int i = m_layout->count() - 1; i >= 0; --i)
        {
            QGraphicsLayoutItem* layoutItem = m_layout->itemAt(i);
            m_layout->removeAt(i);
            layoutItem->setParentLayoutItem(nullptr);

            if (graphicsScene)
            {
                graphicsScene->removeItem(layoutItem->graphicsItem());
            }
        }

        if (m_editable)
        {
            // Adjust the size of the editable widget to match the label
            UpdateSizing();

            m_layout->addItem(m_proxyWidget);
        }
        else
        {
            m_layout->addItem(m_displayLabel);

            if (graphicsScene)
            {
                m_displayLabel->installSceneEventFilter(this);
            }
        }

        m_layout->invalidate();

        adjustSize();
        RefreshDisplay();
    }

    void CommentTextGraphicsWidget::UpdateStyles()
    {    
        m_displayLabel->SetStyle(GetEntityId(), m_style.c_str());

        Styling::StyleHelper styleHelper;

        styleHelper.SetStyle(GetEntityId(), m_style);
        QPen border = styleHelper.GetBorder();
        QColor color = styleHelper.GetColor(Styling::Attribute::Color);
        QFont font = styleHelper.GetFont();

        // We construct a stylesheet for the Qt widget based on our calculated style
        QStringList fields;

        fields.push_back("background-color: rgba(0,0,0,0)");
        
        fields.push_back(QString("border-width: %1").arg(border.width()));

        switch (border.style())
        {
        case Qt::PenStyle::SolidLine:
            fields.push_back("border-style: solid");
            break;
        case Qt::PenStyle::DashLine:
            fields.push_back("border-style: dashed");
            break;
        case Qt::PenStyle::DotLine:
            fields.push_back("border-style: dotted");
            break;
        default:
            fields.push_back("border-style: none");
        }

        fields.push_back(QString("border-color: rgba(%1,%2,%3,%4)").arg(border.color().red()).arg(border.color().green()).arg(border.color().blue()).arg(border.color().alpha()));
        fields.push_back(QString("border-radius: %1").arg(styleHelper.GetAttribute(Styling::Attribute::BorderRadius, 0)));

        fields.push_back("margin: 0");
        fields.push_back("padding: 0");
        
        fields.push_back(styleHelper.GetFontStyleSheet());

        m_textEdit->setStyleSheet(fields.join("; ").toUtf8().data());

        UpdateSizing();
    }

    void CommentTextGraphicsWidget::RefreshDisplay()
    {
        updateGeometry();
        update();
    }

    QGraphicsLayoutItem* CommentTextGraphicsWidget::GetGraphicsLayoutItem()
    {
        return this;
    }
	
	void CommentTextGraphicsWidget::OnStyleChanged()
    {
        UpdateStyles();
        RefreshDisplay();
    }

    void CommentTextGraphicsWidget::SetEditable(bool editable)
    {
        if (m_editable != editable)
        {
            m_editable = editable;
            UpdateLayout();

            if (m_editable)
            {
                UpdateSizing();

                StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Editing);                
                m_textEdit->selectAll();

                // Set the focus after a short delay to give Qt time to update
                m_timer.start();
            }
            else
            {
                StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Editing);
                m_layoutLock = false;
            }

            OnStyleChanged();
        }
    }

    void CommentTextGraphicsWidget::UpdateSizing()
    {
        AZStd::string value = GetComment();
        m_displayLabel->SetLabel(value);

        // As we update the label with the new contents, adjust the editable widget size to match
        m_textEdit->setMinimumSize(m_displayLabel->preferredSize().toSize());
        m_textEdit->setMaximumSize(m_displayLabel->preferredSize().toSize());

        adjustSize();
    }

    void CommentTextGraphicsWidget::SubmitValue()
    {
        CommentRequestBus::Event(GetEntityId(), &CommentRequests::SetComment, GetComment());
        UpdateSizing();
    }
    
    bool CommentTextGraphicsWidget::sceneEventFilter(QGraphicsItem*, QEvent* event)
    {
        bool consumeEvent = false;

        switch (event->type())
        {
            case QEvent::GraphicsSceneMousePress:
            {
                consumeEvent = true;
                QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
                m_initialClick = mouseEvent->screenPos();
                m_pressed = true;
                break;
            }
            case QEvent::GraphicsSceneMouseMove:
            {
                if (m_pressed)
                {
                    consumeEvent = true;
                    QGraphicsSceneMouseEvent* dragEvent = static_cast<QGraphicsSceneMouseEvent*>(event);

                    QPointF point = dragEvent->screenPos();

                    if ((m_initialClick - point).manhattanLength() > 5.0)
                    {
                        m_pressed = false;

                        AZ::EntityId sceneId;
                        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);
                        SceneRequestBus::Event(sceneId, &SceneRequests::ClearSelection);
                        NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::StartItemMove, m_initialClick, point);
                    }
                }
                break;
            }
            case QEvent::GraphicsSceneMouseRelease:
            {
                if (m_pressed)
                {
                    m_pressed = false;

                    // Need to queue this since if we change out display in the middle of processing input
                    // Things get sad.
                    QTimer::singleShot(0, [this]() { this->SetEditable(true); });
                }
                break;
            }
            default:
                break;
        }

        return consumeEvent;
    }
#include <Source/Components/Nodes/Comment/CommentTextGraphicsWidget.moc>
}