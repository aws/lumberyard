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

#include <AzQtComponents/Components/Widgets/GradientSlider.h>
#include <AzQtComponents/Components/Style.h>

#include <QPainter>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QKeyEvent>

namespace AzQtComponents
{

const char* GradientSlider::s_gradientSliderClass = "GradientSlider";

GradientSlider::GradientSlider(Qt::Orientation orientation, QWidget* parent)
    : QSlider(orientation, parent)
{
    setProperty("class", s_gradientSliderClass);
}

GradientSlider::GradientSlider(QWidget* parent)
    : GradientSlider(Qt::Horizontal, parent)
{
}

GradientSlider::~GradientSlider()
{
}

void GradientSlider::setColorFunction(ColorFunction colorFunction)
{
    m_colorFunction = colorFunction;
    updateGradient();
}

void GradientSlider::updateGradient()
{
    initGradientColors();
    update();
}

void GradientSlider::initGradientColors()
{
    if (m_colorFunction)
    {
        const int numGradientStops = 12;

        QGradientStops stops;
        stops.reserve(numGradientStops);

        for (int i = 0; i < numGradientStops; ++i)
        {
            const auto position = static_cast<qreal>(i)/(numGradientStops - 1);
            stops.append({ position, m_colorFunction(position) });
        }

        m_gradient.setStops(stops);
    }
}

void GradientSlider::resizeEvent(QResizeEvent* e)
{
    QSlider::resizeEvent(e);

    const QRect r = contentsRect();
    m_gradient.setStart(r.bottomLeft());
    m_gradient.setFinalStop(orientation() == Qt::Horizontal ? r.bottomRight() : r.topLeft());
}

void GradientSlider::keyPressEvent(QKeyEvent * e)
{
    switch (e->key())
    {
    case Qt::Key_Up:
    case Qt::Key_Right:
        if (e->modifiers() & Qt::ShiftModifier)
        {
            setValue(value() + pageStep());
            return;
        }
        break;
    case Qt::Key_Down:
    case Qt::Key_Left:
        if (e->modifiers() & Qt::ShiftModifier)
        {
            setValue(value() - pageStep());
            return;
        }
        break;
    }

    QSlider::keyPressEvent(e);
}

int GradientSlider::sliderThickness(const Style* style, const QStyleOption* option, const QWidget* widget, const Slider::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(option);
    Q_UNUSED(widget);

    return config.gradientSlider.thickness;
}

int GradientSlider::sliderLength(const Style* style, const QStyleOption* option, const QWidget* widget, const Slider::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(option);
    Q_UNUSED(widget);

    return config.gradientSlider.length;
}

QRect GradientSlider::sliderHandleRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Slider::Config& config)
{
    // start from slider handle rect from common style and change dimensions to our desired thickness/length
    QRect rect = style->QCommonStyle::subControlRect(QStyle::CC_Slider, option, QStyle::SC_SliderHandle, widget);

    if (option->orientation == Qt::Horizontal)
    {
        rect.setHeight(config.gradientSlider.thickness);
        rect.setWidth(config.gradientSlider.length);
        rect.moveTop(option->rect.center().y() - rect.height() / 2 + 1);
    }
    else
    {
        rect.setWidth(config.gradientSlider.thickness);
        rect.setHeight(config.gradientSlider.length);
        rect.moveLeft(option->rect.center().x() - rect.width() / 2 + 1);
    }

    return rect;
}

QRect GradientSlider::sliderGrooveRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Slider::Config& config)
{
    // start from slider groove rect from common style and change to our desired thickness
    QRect rect = style->QCommonStyle::subControlRect(QStyle::CC_Slider, option, QStyle::SC_SliderGroove, widget);

    if (option->orientation == Qt::Horizontal)
    {
        rect.setHeight(config.gradientSlider.thickness);
    }
    else
    {
        rect.setWidth(config.gradientSlider.thickness);
    }

    rect.moveCenter(option->rect.center());

    return rect;
}

bool GradientSlider::drawSlider(const Style* style, const QStyleOptionSlider* option, QPainter* painter, const QWidget* widget, const Slider::Config& config)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // groove
    {
        const auto& border = config.gradientSlider.grooveBorder;

        const QPen pen(border.color, border.thickness);

        const qreal w = 0.5*pen.widthF();
        const auto r = QRectF(style->subControlRect(QStyle::CC_Slider, option, QStyle::SC_SliderGroove, widget)).adjusted(w, w, -w, -w);

        painter->setPen(Qt::NoPen);
        painter->setBrush(Style::cachedPixmap(QStringLiteral(":/stylesheet/img/UI20/alpha-background.png")));
        painter->drawRoundedRect(r, border.radius, border.radius);

        painter->setPen(pen);

        const QBrush brush = static_cast<const GradientSlider*>(widget)->gradientBrush();
        painter->setBrush(brush);

        painter->drawRoundedRect(r, border.radius, border.radius);
    }

    // handle
    {
        const auto& border = config.gradientSlider.handleBorder;

        const QPen pen(border.color, border.thickness);

        const qreal w = 0.5*pen.widthF();
        const auto r = QRectF(style->subControlRect(QStyle::CC_Slider, option, QStyle::SC_SliderHandle, widget)).adjusted(w, w, -w, -w);

        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(r, border.radius, border.radius);
    }

    painter->restore();

    return true;
}

QBrush GradientSlider::gradientBrush() const
{
    return m_gradient;
}

} // namespace AzQtComponents
#include <Components/Widgets/GradientSlider.moc>
