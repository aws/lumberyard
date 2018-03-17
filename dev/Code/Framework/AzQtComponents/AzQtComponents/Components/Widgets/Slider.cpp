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

#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Style.h>

#include <QStyleFactory>

#include <QPainter>
#include <QStyleOption>
#include <QVariant>
#include <QSettings>
#include <QVBoxLayout>
#include <QEvent>

namespace AzQtComponents
{

static QString g_midPointStyleClass = QStringLiteral("MidPoint");

Slider::Slider(QWidget* parent)
    : Slider(Qt::Horizontal, parent)
{
}

Slider::Slider(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* noFrameLayout = new QVBoxLayout(this);
    noFrameLayout->setMargin(0);
    noFrameLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(noFrameLayout);
    
    m_slider = new QSlider(orientation, this);
    noFrameLayout->addWidget(m_slider);

    m_slider->installEventFilter(this);

    connect(m_slider, &QSlider::sliderPressed, this, &Slider::sliderPressed);
    connect(m_slider, &QSlider::sliderMoved, this, &Slider::sliderMoved);
    connect(m_slider, &QSlider::sliderReleased, this, &Slider::sliderReleased);
    connect(m_slider, &QSlider::actionTriggered, this, &Slider::actionTriggered);
}

QSize Slider::sizeHint() const
{
    return m_slider->sizeHint();
}

QSize Slider::minimumSizeHint() const
{
    return m_slider->minimumSizeHint();
}

void Slider::setTracking(bool enable)
{
    m_slider->setTracking(enable);
}

bool Slider::hasTracking() const
{
    return m_slider->hasTracking();
}

void Slider::applyMidPointStyle(Slider* slider)
{
    slider->setProperty("class", g_midPointStyleClass);
    slider->m_slider->setProperty("class", g_midPointStyleClass);
}

void Slider::setToolTipFormatting(QString prefix, QString postFix)
{
    m_toolTipPrefix = prefix;
    m_toolTipPostfix = postFix;
}

Slider::Config Slider::loadConfig(QSettings& settings)
{
    Q_UNUSED(settings);

    Config config = defaultConfig();

    return config;
}

Slider::Config Slider::defaultConfig()
{
    Config config;

    return config;
}

bool Slider::eventFilter(QObject* watched, QEvent* event)
{
    switch (event->type())
    {
        case QEvent::ToolTip:
            // call hoverValueText() to figure out what the tooltip text should be
            // detect when the mouse is at the right place...
            // or just change the tooltip text when the value changes...
            //QString toolTipText = QStringLiteral("%1%2%%3").arg(m_toolTipPrefix, hoverValueText()).arg(m_toolTipPostfix);
            break;
    }

    return QObject::eventFilter(watched, event);
}

bool Slider::polish(Style* style, QWidget* widget, const Slider::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(widget);
    Q_UNUSED(config);

    const bool widgetPolished = false;
    return widgetPolished;
}

bool Slider::drawSlider(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Slider::Config& config)
{
    Q_UNUSED(config);
    Q_UNUSED(option);
    Q_UNUSED(painter);

    const QSlider* slider = qobject_cast<const QSlider*>(widget);
    if (slider != nullptr)
    {
        if (style->hasClass(widget, g_midPointStyleClass))
        {

        }
        else
        {

        }
    }

    return false;
}




SliderInt::SliderInt(QWidget* parent)
    : SliderInt(Qt::Horizontal, parent)
{
}

SliderInt::SliderInt(Qt::Orientation orientation, QWidget* parent)
    : Slider(orientation, parent)
{
    connect(m_slider, &QSlider::valueChanged, this, &SliderInt::valueChanged);
    connect(m_slider, &QSlider::rangeChanged, this, &SliderInt::rangeChanged);
}

void SliderInt::setValue(int value)
{
    m_slider->setValue(value);
}

int SliderInt::value() const
{
    return m_slider->value();
}

void SliderInt::setMinimum(int min)
{
    m_slider->setMinimum(min);
}

int SliderInt::minimum() const
{
    return m_slider->minimum();
}

void SliderInt::setMaximum(int max)
{
    m_slider->setMaximum(max);
}

int SliderInt::maximum() const
{
    return m_slider->maximum();
}

void SliderInt::setRange(int min, int max)
{
    m_slider->setRange(min, max);
}

QString SliderInt::hoverValueText() const
{
    return QStringLiteral("%1").arg(m_slider->value());
}





SliderDouble::SliderDouble(QWidget* parent)
    : SliderDouble(Qt::Horizontal, parent)
{
}

SliderDouble::SliderDouble(Qt::Orientation orientation, QWidget* parent)
    : Slider(orientation, parent)
{
    connect(m_slider, &QSlider::valueChanged, this, [this](int newValue) {
        double denormalizedValue = ((static_cast<double>(newValue) / static_cast<double>(m_numSteps)) * (m_maximum - m_minimum)) + m_minimum;
        Q_EMIT valueChanged(denormalizedValue);
    });
}

void SliderDouble::setValue(double value)
{
    int normalizedValue = static_cast<int>(((value - m_minimum) / (m_maximum - m_minimum)) * static_cast<double>(m_numSteps));
    m_slider->setValue(normalizedValue);
}

double SliderDouble::value() const
{
    return m_slider->value();
}

double SliderDouble::minimum() const
{
    return m_minimum;
}

void SliderDouble::setMinimum(double min)
{
    setRange(min, m_maximum, m_numSteps);
}

double SliderDouble::maximum() const
{
    return m_maximum;
}

void SliderDouble::setMaximum(double max)
{
    setRange(m_minimum, max, m_numSteps);
}

int SliderDouble::numSteps() const
{
    return m_numSteps;
}

void SliderDouble::setNumSteps(int steps)
{
    setRange(m_minimum, m_maximum, steps);
}

void SliderDouble::setRange(double min, double max, int numSteps)
{
    m_slider->setRange(0, numSteps - 1);

    m_maximum = max;
    m_minimum = min;
    m_numSteps = numSteps;

    Q_ASSERT(m_numSteps != 0);
    if (m_numSteps == 0)
    {
        m_numSteps = 1;
    }

    Q_EMIT rangeChanged(min, max, numSteps);
}

QString SliderDouble::hoverValueText() const
{
    // maybe format this, max number of digits?
    return QStringLiteral("%1").arg(value());
}

} // namespace AzQtComponents
