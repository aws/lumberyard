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
#include "QCustomGradientWidget.h"

#include <QGradientStops>
#include <QPixmap>
#include <QIcon>

QCustomGradientWidget::QCustomGradientWidget(QWidget* parent)
    : QWidget(parent)
{
}

QCustomGradientWidget::~QCustomGradientWidget()
{
    while (gradients.count() > 0)
    {
        gradients.takeAt(0);
    }
}

void QCustomGradientWidget::paintEvent(QPaintEvent*)
{
    QPixmap pxr(width(), height());
    pxr.fill(Qt::transparent);
    QPainter painter(&pxr);
    painter.setPen(Qt::NoPen);
    setAutoFillBackground(true);


    for (Gradient* gradient : gradients)
    {
        painter.setCompositionMode(gradient->m_mode);
        painter.setBrush(*gradient->m_gradient);
        painter.drawRect(rect());
    }

    QPainter m_painter(this);
    m_painter.fillRect(rect(), QBrush(background));
    m_painter.drawPixmap(0, 0, pxr);
}

unsigned int QCustomGradientWidget::AddGradient(QGradient* gradient, QPainter::CompositionMode mode)
{
    unsigned int index = gradients.count();
    gradients.push_back(new Gradient(gradient, mode));
    update();
    return index;
}

void QCustomGradientWidget::RemoveGradient(unsigned int id)
{
    if (id < 0 || !(id < gradients.count()))
    {
        return;
    }
    gradients.remove(id);
    update();
}

void QCustomGradientWidget::SetGradientStops(unsigned int id, QGradientStops stops)
{
    if (id < 0 || !(id < gradients.count()))
    {
        return;
    }
    gradients[id]->m_gradient->setStops(stops);
    update();
}

void QCustomGradientWidget::AddGradientStop(unsigned int id, QGradientStop stop)
{
    if (id < 0 || !(id < gradients.count()))
    {
        return;
    }
    gradients[id]->m_gradient->setColorAt(stop.first, stop.second);
    update();
}

void QCustomGradientWidget::AddGradientStop(unsigned int id, qreal stop, QColor color)
{
    gradients[id]->m_gradient->setColorAt(stop, color);
    update();
}

void QCustomGradientWidget::SetBackgroundImage(QString str)
{
    background.load(str);
}
