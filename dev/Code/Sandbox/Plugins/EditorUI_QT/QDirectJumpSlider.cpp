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
#include "EditorUI_QT_Precompiled.h"
#include "QDirectJumpSlider.h"

QDirectJumpSlider::QDirectJumpSlider(QWidget* parent)
    : QSlider(parent)
{
}
QDirectJumpSlider::QDirectJumpSlider(Qt::Orientation orientation, QWidget* parent)
    : QSlider(orientation, parent)
{
}


void QDirectJumpSlider::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        int pressedPos;

        if (orientation() == Qt::Vertical)
        {
            pressedPos = minimum() + ((maximum() - minimum()) * (height() - event->y())) / height();
        }
        else
        {
            pressedPos = minimum() + ((maximum() - minimum()) * event->x()) / width();
        }

        setValue(pressedPos);
        setSliderPosition(pressedPos);
        event->accept();
    }
    QSlider::mousePressEvent(event);
}

#include <QDirectJumpSlider.moc>