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
#include <GraphCanvas/Widgets/AutoElidingLabel.h>

namespace GraphCanvas
{
    //////////////////////
    // AutoEllidingLabel
    //////////////////////

    AutoElidingLabel::AutoElidingLabel(QWidget* parent, Qt::WindowFlags flags)
        : QLabel(parent, flags)
        , m_elideMode(Qt::ElideRight)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    }

    AutoElidingLabel::~AutoElidingLabel()
    {
    }

    void AutoElidingLabel::SetElideMode(Qt::TextElideMode elideMode)
    {
        m_elideMode = elideMode;

        RefreshLabel();
    }

    Qt::TextElideMode AutoElidingLabel::GetElideMode() const
    {
        return m_elideMode;
    }

    QString AutoElidingLabel::fullText() const
    {
        return m_fullText;
    }

    void AutoElidingLabel::setFullText(const QString& text)
    {
        m_fullText = text;
        RefreshLabel();
    }

    void AutoElidingLabel::resizeEvent(QResizeEvent* resizeEvent)
    {
        RefreshLabel();
        QLabel::resizeEvent(resizeEvent);
    }

    QSize AutoElidingLabel::minimumSizeHint() const
    {
        QSize retVal = QLabel::minimumSizeHint();
        retVal.setWidth(0);

        return retVal;
    }

    QSize AutoElidingLabel::sizeHint() const
    {
        QSize retVal = QLabel::sizeHint();
        retVal.setWidth(0);

        return retVal;
    }

    void AutoElidingLabel::RefreshLabel()
    {
        QFontMetrics metrics(font());

        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;

        getContentsMargins(&left, &top, &right, &bottom);

        int labelWidth = width();

        QString elidedText = metrics.elidedText(m_fullText, m_elideMode, (width() - (left + right)));

        setText(elidedText);
    }

#include <StaticLib/GraphCanvas/Widgets/AutoElidingLabel.moc>
}