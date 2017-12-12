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

#include <QCoreApplication>
#include <QFont>
#include <QGraphicsItem>
#include <QPainter>

#include <AzCore/Serialization/EditContext.h>

#include <Widgets/GraphCanvasLabel.h>

#include <GraphCanvas/tools.h>
#include <Styling/StyleHelper.h>

namespace GraphCanvas
{
    /////////////////////
    // GraphCanvasLabel
    /////////////////////

    GraphCanvasLabel::GraphCanvasLabel(QGraphicsItem* parent)
        : QGraphicsWidget(parent)
        , m_defaultAlignment(Qt::AlignVCenter | Qt::AlignHCenter)
        , m_elide(true)
        , m_wrap(false)
        , m_maximumSize(0,0)
        , m_minimumSize(0,0)
    {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        setGraphicsItem(this);
        setFlag(ItemIsMovable, false);
    }

    void GraphCanvasLabel::SetLabel(const AZStd::string& label, const AZStd::string& translationContext, const AZStd::string& translationKey)
    {
        TranslationKeyedString keyedString(label, translationContext, translationKey);
        SetLabel(keyedString);
    }

    void GraphCanvasLabel::SetLabel(const TranslationKeyedString& value)
    {
        AZStd::string displayString = value.GetDisplayString();

        if (m_labelText.compare(QString(displayString.c_str())))
        {
            m_labelText = Tools::qStringFromUtf8(displayString);

            UpdateDisplayText();
            RefreshDisplay();
        }
    }

    void GraphCanvasLabel::SetSceneStyle(const AZ::EntityId& sceneId, const char* style)
    {
        m_styleHelper.SetScene(sceneId);
        m_styleHelper.SetStyle(style);
        UpdateDisplayText();
        RefreshDisplay();
    }

    void GraphCanvasLabel::SetStyle(const AZ::EntityId& entityId, const char* styleElement)
    {
        m_styleHelper.SetStyle(entityId, styleElement);
        UpdateDisplayText();
        RefreshDisplay();
    }

    void GraphCanvasLabel::RefreshDisplay()
    {
        UpdateDesiredBounds();
        updateGeometry();
        update();
    }

    void GraphCanvasLabel::SetElide(bool elide)
    {
        AZ_Warning("GraphCanvasLabel", !(elide && m_wrap), "GraphCanvasLabel doesn't support eliding text and word wrapping at the same time.");

        if (m_elide != elide)
        {
            m_elide = elide;
            RefreshDisplay();
        }
    }

    void GraphCanvasLabel::SetWrap(bool wrap)
    {
        AZ_Warning("GraphCanvasLabel", !(m_elide && wrap), "GraphCanvasLabel doesn't support eliding text and word wrapping at the same time.");

        if (m_wrap != wrap)
        {
            m_wrap = wrap;
            RefreshDisplay();
        }
    }

    void GraphCanvasLabel::SetDefaultAlignment(Qt::Alignment defaultAlignment)
    {
        m_defaultAlignment = defaultAlignment;
        update();
    }

    Styling::StyleHelper& GraphCanvasLabel::GetStyleHelper()
    {
        return m_styleHelper;
    }

    const Styling::StyleHelper& GraphCanvasLabel::GetStyleHelper() const
    {
        return m_styleHelper;
    }

    bool GraphCanvasLabel::event(QEvent* qEvent)
    {
        if (qEvent->type() == QEvent::GraphicsSceneResize)
        {
            UpdateDisplayText();
        }

        return QGraphicsWidget::event(qEvent);
    }

    void GraphCanvasLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget /*= nullptr*/)
    {
        painter->save();

        // Background
        {
            qreal borderRadius = m_styleHelper.GetAttribute(Styling::Attribute::BorderRadius, 0);

            if (borderRadius == 0)
            {
                painter->fillRect(boundingRect(), m_styleHelper.GetBrush(Styling::Attribute::BackgroundColor));

                if (m_styleHelper.HasAttribute(Styling::Attribute::BorderWidth))
                {
                    QPen restorePen = painter->pen();
                    painter->setPen(m_styleHelper.GetBorder());
                    painter->drawRect(boundingRect());
                    painter->setPen(restorePen);
                }
            }
            else
            {
                QPainterPath path;
                path.addRoundedRect(boundingRect(), borderRadius, borderRadius);
                painter->fillPath(path, m_styleHelper.GetBrush(Styling::Attribute::BackgroundColor));

                if (m_styleHelper.HasAttribute(Styling::Attribute::BorderWidth))
                {
                    QPen restorePen = painter->pen();
                    painter->setPen(m_styleHelper.GetBorder());
                    painter->drawPath(path);
                    painter->setPen(restorePen);
                }
            }
        }

        // Text.
        if (!m_labelText.isEmpty())
        {
            qreal padding = m_styleHelper.GetAttribute(Styling::Attribute::Padding, 2.0f);

            QRectF innerBounds = boundingRect();
            innerBounds = innerBounds.adjusted(padding, padding, -padding, -padding);

            painter->setPen(m_styleHelper.GetColor(Styling::Attribute::Color));
            painter->setBrush(QBrush());
            painter->setFont(m_styleHelper.GetFont());

            Qt::Alignment textAlignment = m_styleHelper.GetTextAlignment(m_defaultAlignment);

            QTextOption opt(textAlignment);
            opt.setFlags(QTextOption::IncludeTrailingSpaces);

            if (m_wrap)
            {
                opt.setWrapMode(QTextOption::WordWrap);
            }
            else
            {
                opt.setWrapMode(QTextOption::NoWrap);
            }

            painter->drawText(innerBounds, m_displayText, opt);
        }

        painter->restore();
    }

    QSizeF GraphCanvasLabel::sizeHint(Qt::SizeHint which, const QSizeF& constraint /*= QSizeF()*/) const
    {
        switch (which)
        {
        case Qt::MinimumSize:
            return m_minimumSize;            
        case Qt::PreferredSize:
            return m_desiredBounds.size();
        case Qt::MaximumSize:
            return m_maximumSize;
        case Qt::MinimumDescent:
            //         {
            //             QFont FONT = QFont();
            //             QFontMetricsF metrics(FONT);
            //             return{ 0, qMin(constraint.height(), metrics.descent()) };
            //         }
        default:
            return QSizeF();
        }
    }

    void GraphCanvasLabel::UpdateDisplayText()
    {
        qreal padding = m_styleHelper.GetAttribute(Styling::Attribute::Padding, 2.0f);
        QFontMetrics metrics(m_styleHelper.GetFont());

        QRectF innerBounds = boundingRect();
        innerBounds = innerBounds.adjusted(padding, padding, -padding, -padding);

        if (m_elide)
        {
            m_displayText = metrics.elidedText(m_labelText, Qt::ElideRight, innerBounds.width());
        }
        else
        {
            m_displayText = m_labelText;
        }
    }

    void GraphCanvasLabel::UpdateDesiredBounds()
    {
        qreal padding = m_styleHelper.GetAttribute(Styling::Attribute::Padding, 2.0f);

        QFontMetricsF metrics = QFontMetricsF(m_styleHelper.GetFont());

        int flags = m_defaultAlignment;
        if (m_wrap)
        {
            flags = flags | Qt::TextWordWrap;
        }

        QRectF maxInnerBounds(rect().topLeft(), maximumSize());
        maxInnerBounds = maxInnerBounds.adjusted(padding, padding, -padding, -padding);

        // Horizontal Padding:
        // 1 padding for the left side
        // 1 padding for the right side
        //
        // Vertical Padding:
        // 1 padding for the top
        // 1 padding for the bottom.
        m_desiredBounds = metrics.boundingRect(maxInnerBounds, flags, m_labelText).adjusted(0.0f, 0.0f, padding * 2.0f, padding * 2.0f);
        
        // Seem so be an off by 1 error here. So adding one in each direction to my desired size to stop text from being clipped.
        m_desiredBounds.adjust(-1.0, -1.0, 1.0, 1.0);

        m_minimumSize = m_styleHelper.GetMinimumSize(m_desiredBounds.size());
        m_maximumSize = m_styleHelper.GetMaximumSize();

        adjustSize();
    }
}
