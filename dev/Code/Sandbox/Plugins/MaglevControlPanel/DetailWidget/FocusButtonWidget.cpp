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

#include "FocusButtonWidget.h"

#include <DetailWidget/FocusButtonWidget.moc>

FocusButtonWidget::FocusButtonWidget()
    : QPushButton{}
    , m_numFocusGains{0}
{}

FocusButtonWidget::FocusButtonWidget(const QString& title)
    : QPushButton{title}
    , m_numFocusGains{0}
{}

void FocusButtonWidget::enterEvent(QEvent* event)
{
    HandleGainFocus();

    QPushButton::enterEvent(event);
}

void FocusButtonWidget::leaveEvent(QEvent* event)
{
    HandleLoseFocus();

    QPushButton::leaveEvent(event);
}

void FocusButtonWidget::focusInEvent(QFocusEvent* event)
{
    HandleGainFocus();

    QPushButton::focusInEvent(event);
}

void FocusButtonWidget::focusOutEvent(QFocusEvent* event)
{
    HandleLoseFocus();

    QPushButton::focusOutEvent(event);
}

void FocusButtonWidget::HandleGainFocus()
{
    ++m_numFocusGains;
    if (m_numFocusGains == 1) // First focus
    {
        FocusGained();
    }
}

void FocusButtonWidget::HandleLoseFocus()
{
    --m_numFocusGains;
    if (m_numFocusGains == 0) // No sources of focus (e.g., mouse, keyboard) remain
    {
        FocusLost();
    }
}