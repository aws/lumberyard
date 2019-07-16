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
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QStyle>

namespace AzQtComponents
{
    ElidingLabel::ElidingLabel(QWidget* parent /* = nullptr */)
        : QWidget(parent)
        , m_elideMode(Qt::ElideRight)
        , m_label(new QLabel(this))
    {
        auto layout = new QHBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(m_label);
    }

    void ElidingLabel::setText(const QString& text)
    {
        if (text == m_text)
        {
            return;
        }

        m_text = text;
        m_label->setText(m_text);
        m_elidedText.clear();
        update();
    }

    void ElidingLabel::setDescription(const QString& description)
    {
        m_description = description;
    }

    void ElidingLabel::setElideMode(Qt::TextElideMode mode)
    {
        if (m_elideMode == mode)
        {
            return;
        }

        m_elideMode = mode;
        m_elidedText.clear();
        update();
    }

    void ElidingLabel::paintEvent(QPaintEvent* event)
    {
        elide();
        QWidget::paintEvent(event);
    }

    void ElidingLabel::resizeEvent(QResizeEvent* event)
    {
        m_elidedText.clear();
        QWidget::resizeEvent(event);
    }

    void ElidingLabel::elide()
    {
        if (!m_elidedText.isEmpty())
        {
            return;
        }

        QPainter painter(this);
        QFontMetrics fontMetrics = painter.fontMetrics();
        m_elidedText = fontMetrics.elidedText(m_text, m_elideMode, m_label->contentsRect().width());
        m_label->setText(m_elidedText);

        if (m_elidedText != m_text)
        {
            if (m_description.isEmpty())
            {
                setToolTip(m_text);
            }
            else
            {
                setToolTip(QStringLiteral("%1\n%2").arg(m_text, m_description));
            }
        }
        else
        {
            setToolTip(m_description);
        }
    }

    void ElidingLabel::refreshStyle()
    {
        m_label->style()->unpolish(m_label);
        m_label->style()->polish(m_label);
        m_label->update();
    }

    QSize ElidingLabel::minimumSizeHint() const
    {
        // Override the QLabel minimumSizeHint width to let other
        // widgets know we can deal with less space.
        return QWidget::minimumSizeHint().boundedTo({0, std::numeric_limits<int>::max()});
    }

} // namespace AzQtComponents
#include <Components/Widgets/ElidingLabel.moc>
