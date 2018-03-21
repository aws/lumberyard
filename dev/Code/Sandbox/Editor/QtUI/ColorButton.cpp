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
#include "ColorButton.h"

#include <QPainter>
#include <QColorDialog>

ColorButton::ColorButton(QWidget* parent /* = nullptr */)
    : QToolButton(parent)
{
    connect(this, &ColorButton::clicked, this, &ColorButton::OnClick);
}

void ColorButton::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_color);
    painter.setPen(Qt::black);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void ColorButton::OnClick()
{
    QColor color = QColorDialog::getColor(m_color, this, tr("Select Color"));

    if (color.isValid())
    {
        m_color = color;
        emit ColorChanged(m_color);
    }
}

#include <QtUI/ColorButton.moc>
