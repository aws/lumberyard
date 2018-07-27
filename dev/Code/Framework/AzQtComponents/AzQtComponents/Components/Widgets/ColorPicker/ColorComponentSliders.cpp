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

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorComponentSliders.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <QIntValidator>
#include <QLabel>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>

namespace AzQtComponents
{
    namespace
    {
        int GetRequiredWidth(const QWidget* widget)
        {
            const QString labels[] = {
                QObject::tr("H"),
                QObject::tr("S"),
                QObject::tr("L"),
                QObject::tr("V"),
                QObject::tr("R"),
                QObject::tr("G"),
                QObject::tr("B")
            };

            widget->ensurePolished();
            QFontMetrics metrics(widget->font());

            int widest = 0;
            for (const QString& label : labels)
            {
                widest = std::max(widest, metrics.width(label));
            }

            return widest;
        }
    }

ColorComponentEdit::ColorComponentEdit(const QString& labelText, int softMaximum, int hardMaximum, QWidget* parent)
    : QWidget(parent)
    , m_value(0)
    , m_spinBox(new SpinBox(this))
    , m_slider(new GradientSlider(this))
{
    auto layout = new QHBoxLayout(this);

    layout->setMargin(0);

    auto label = new QLabel(labelText, this);
    label->setFixedWidth(GetRequiredWidth(this));
    label->setAlignment(Qt::AlignCenter);
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(label);

    layout->addSpacing(2);

    m_spinBox->setRange(0, hardMaximum);
    m_spinBox->setFixedWidth(32);
    m_spinBox->setAlignment(Qt::AlignHCenter);
    connect(m_spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ColorComponentEdit::spinValueChanged);
    connect(m_spinBox, &SpinBox::valueChangeBegan, this, &ColorComponentEdit::valueChangeBegan);
    connect(m_spinBox, &SpinBox::valueChangeEnded, this, &ColorComponentEdit::valueChangeEnded);
    layout->addWidget(m_spinBox);

    layout->addSpacing(4);

    m_slider->setMinimum(0);
    m_slider->setMaximum(softMaximum);
    connect(m_slider, &QSlider::valueChanged, this, &ColorComponentEdit::sliderValueChanged);
    connect(m_slider, &QSlider::sliderPressed, this, &ColorComponentEdit::valueChangeBegan);
    connect(m_slider, &QSlider::sliderReleased, this, &ColorComponentEdit::valueChangeEnded);
    layout->addWidget(m_slider);
}

ColorComponentEdit::~ColorComponentEdit()
{
}

void ColorComponentEdit::setColorFunction(GradientSlider::ColorFunction colorFunction)
{
    m_slider->setColorFunction(colorFunction);
}

qreal ColorComponentEdit::value() const
{
    return m_value;
}

void ColorComponentEdit::setValue(qreal value)
{
    if (qFuzzyCompare(value, m_value))
    {
        return;
    }

    m_value = value;

    const QSignalBlocker sliderBlocker(m_slider);
    const QSignalBlocker spinBoxBlocker(m_spinBox);

    int sliderValue = qRound(m_value*m_slider->maximum());
    m_slider->setValue(sliderValue);
    m_spinBox->setValue (sliderValue);

    emit valueChanged(m_value);
}

void ColorComponentEdit::updateGradient()
{
    m_slider->updateGradient();
}

void ColorComponentEdit::spinValueChanged(int value)
{
    m_value = static_cast<qreal>(value) / m_slider->maximum();

    const QSignalBlocker b(m_slider);
    m_slider->setValue(value);

    emit valueChanged(m_value);
}

void ColorComponentEdit::sliderValueChanged(int value)
{
    m_value = static_cast<qreal>(value) / m_slider->maximum();

    const QSignalBlocker b(m_spinBox);
    m_spinBox->setValue(static_cast<int>(value));

    emit valueChanged(m_value);
}

HSLSliders::HSLSliders(QWidget* widget)
    : QWidget(widget)
    , m_mode(Mode::Hsl)
{
    auto layout = new QVBoxLayout(this);

    m_hueSlider = new ColorComponentEdit(tr("H"), 360, 360, this);
    layout->addWidget(m_hueSlider);

    m_saturationSlider = new ColorComponentEdit(tr("S"), 100, 100, this);
    layout->addWidget(m_saturationSlider);

    m_lightnessSlider = new ColorComponentEdit(tr("L"), 100, 1250, this);
    layout->addWidget(m_lightnessSlider);

    m_hueSlider->setColorFunction([this](qreal position) {
        return QColor::fromHslF(position, m_saturationSlider->value(), m_lightnessSlider->value());
    });

    m_saturationSlider->setColorFunction([this](qreal position) {
        return QColor::fromHslF(m_hueSlider->value(), position, m_lightnessSlider->value());
    });

    m_lightnessSlider->setColorFunction([this](qreal position) {
        return QColor::fromHslF(m_hueSlider->value(), m_saturationSlider->value(), position);
    });

    connect(m_hueSlider, &ColorComponentEdit::valueChanged, this, &HSLSliders::hueChanged);
    connect(m_hueSlider, &ColorComponentEdit::valueChanged, m_saturationSlider, &ColorComponentEdit::updateGradient);
    connect(m_hueSlider, &ColorComponentEdit::valueChanged, m_lightnessSlider, &ColorComponentEdit::updateGradient);
    connect(m_hueSlider, &ColorComponentEdit::valueChangeBegan, this, &HSLSliders::valueChangeBegan);
    connect(m_hueSlider, &ColorComponentEdit::valueChangeEnded, this, &HSLSliders::valueChangeEnded);

    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, this, &HSLSliders::saturationChanged);
    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, m_hueSlider, &ColorComponentEdit::updateGradient);
    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, m_lightnessSlider, &ColorComponentEdit::updateGradient);
    connect(m_saturationSlider, &ColorComponentEdit::valueChangeBegan, this, &HSLSliders::valueChangeBegan);
    connect(m_saturationSlider, &ColorComponentEdit::valueChangeEnded, this, &HSLSliders::valueChangeEnded);

    connect(m_lightnessSlider, &ColorComponentEdit::valueChanged, this, &HSLSliders::lightnessChanged);
    connect(m_lightnessSlider, &ColorComponentEdit::valueChanged, m_hueSlider, &ColorComponentEdit::updateGradient);
    connect(m_lightnessSlider, &ColorComponentEdit::valueChanged, m_saturationSlider, &ColorComponentEdit::updateGradient);
    connect(m_lightnessSlider, &ColorComponentEdit::valueChangeBegan, this, &HSLSliders::valueChangeBegan);
    connect(m_lightnessSlider, &ColorComponentEdit::valueChangeEnded, this, &HSLSliders::valueChangeEnded);

}

HSLSliders::Mode HSLSliders::mode() const
{
    return m_mode;
}

qreal HSLSliders::hue() const
{
    return m_hueSlider->value();
}

qreal HSLSliders::saturation() const
{
    return m_saturationSlider->value();
}

qreal HSLSliders::lightness() const
{
    return m_lightnessSlider->value();
}

void HSLSliders::setMode(Mode mode)
{
    if (mode == m_mode)
    {
        return;
    }

    m_mode = mode;

    m_lightnessSlider->setVisible(m_mode == Mode::Hsl);
    if (m_mode == Mode::Hs)
    {
        setLightness(1.0);
    }

    emit modeChanged(mode);
}

void HSLSliders::setHue(qreal hue)
{
    const QSignalBlocker b(m_hueSlider);
    m_hueSlider->setValue(hue);
    m_saturationSlider->updateGradient();
    m_lightnessSlider->updateGradient();
}

void HSLSliders::setSaturation(qreal saturation)
{
    const QSignalBlocker b(m_saturationSlider);
    m_saturationSlider->setValue(saturation);
    m_hueSlider->updateGradient();
    m_lightnessSlider->updateGradient();
}

void HSLSliders::setLightness(qreal lightness)
{
    const QSignalBlocker b(m_lightnessSlider);
    m_lightnessSlider->setValue(lightness);
    m_hueSlider->updateGradient();
    m_saturationSlider->updateGradient();
}

HSVSliders::HSVSliders(QWidget* widget)
    : QWidget(widget)
{
    auto layout = new QVBoxLayout(this);

    m_hueSlider = new ColorComponentEdit(tr("H"), 360, 360, this);
    layout->addWidget(m_hueSlider);

    m_saturationSlider = new ColorComponentEdit(tr("S"), 100, 100, this);
    layout->addWidget(m_saturationSlider);

    m_valueSlider = new ColorComponentEdit(tr("V"), 100, 1250, this);
    layout->addWidget(m_valueSlider);

    m_hueSlider->setColorFunction([this](qreal position) {
        return QColor::fromHsvF(position, m_saturationSlider->value(), m_valueSlider->value());
    });

    m_saturationSlider->setColorFunction([this](qreal position) {
        return QColor::fromHsvF(m_hueSlider->value(), position, m_valueSlider->value());
    });

    m_valueSlider->setColorFunction([this](qreal position) {
        return QColor::fromHsvF(m_hueSlider->value(), m_saturationSlider->value(), position);
    });

    connect(m_hueSlider, &ColorComponentEdit::valueChanged, this, &HSVSliders::hueChanged);
    connect(m_hueSlider, &ColorComponentEdit::valueChanged, m_saturationSlider, &ColorComponentEdit::updateGradient);
    connect(m_hueSlider, &ColorComponentEdit::valueChanged, m_valueSlider, &ColorComponentEdit::updateGradient);
    connect(m_hueSlider, &ColorComponentEdit::valueChangeBegan, this, &HSVSliders::valueChangeBegan);
    connect(m_hueSlider, &ColorComponentEdit::valueChangeEnded, this, &HSVSliders::valueChangeEnded);

    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, this, &HSVSliders::saturationChanged);
    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, m_hueSlider, &ColorComponentEdit::updateGradient);
    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, m_valueSlider, &ColorComponentEdit::updateGradient);
    connect(m_saturationSlider, &ColorComponentEdit::valueChangeBegan, this, &HSVSliders::valueChangeBegan);
    connect(m_saturationSlider, &ColorComponentEdit::valueChangeEnded, this, &HSVSliders::valueChangeEnded);

    connect(m_valueSlider, &ColorComponentEdit::valueChanged, this, &HSVSliders::valueChanged);
    connect(m_valueSlider, &ColorComponentEdit::valueChanged, m_hueSlider, &ColorComponentEdit::updateGradient);
    connect(m_valueSlider, &ColorComponentEdit::valueChanged, m_saturationSlider, &ColorComponentEdit::updateGradient);
    connect(m_valueSlider, &ColorComponentEdit::valueChangeBegan, this, &HSVSliders::valueChangeBegan);
    connect(m_valueSlider, &ColorComponentEdit::valueChangeEnded, this, &HSVSliders::valueChangeEnded);
}

qreal HSVSliders::hue() const
{
    return m_hueSlider->value();
}

qreal HSVSliders::saturation() const
{
    return m_saturationSlider->value();
}

qreal HSVSliders::value() const
{
    return m_valueSlider->value();
}

void HSVSliders::setHue(qreal hue)
{
    const QSignalBlocker b(m_hueSlider);
    m_hueSlider->setValue(hue);
    m_saturationSlider->updateGradient();
    m_valueSlider->updateGradient();
}

void HSVSliders::setSaturation(qreal saturation)
{
    const QSignalBlocker b(m_saturationSlider);
    m_saturationSlider->setValue(saturation);
    m_hueSlider->updateGradient();
    m_valueSlider->updateGradient();
}

void HSVSliders::setValue(qreal value)
{
    const QSignalBlocker b(m_valueSlider);
    m_valueSlider->setValue(value);
    m_hueSlider->updateGradient();
    m_saturationSlider->updateGradient();
}

RGBSliders::RGBSliders(QWidget* widget)
    : QWidget(widget)
{
    auto layout = new QVBoxLayout(this);

    m_redSlider = new ColorComponentEdit(tr("R"), 255, 3187, this);
    layout->addWidget(m_redSlider);

    m_greenSlider = new ColorComponentEdit(tr("G"), 255, 3187, this);
    layout->addWidget(m_greenSlider);

    m_blueSlider = new ColorComponentEdit(tr("B"), 255, 3187, this);
    layout->addWidget(m_blueSlider);

    m_redSlider->setColorFunction([this](qreal position) {
        return QColor::fromRgbF(position, m_greenSlider->value(), m_blueSlider->value());
    });

    m_greenSlider->setColorFunction([this](qreal position) {
        return QColor::fromRgbF(m_redSlider->value(), position, m_blueSlider->value());
    });

    m_blueSlider->setColorFunction([this](qreal position) {
        return QColor::fromRgbF(m_redSlider->value(), m_greenSlider->value(), position);
    });

    connect(m_redSlider, &ColorComponentEdit::valueChanged, this, &RGBSliders::redChanged);
    connect(m_redSlider, &ColorComponentEdit::valueChanged, m_greenSlider, &ColorComponentEdit::updateGradient);
    connect(m_redSlider, &ColorComponentEdit::valueChanged, m_blueSlider, &ColorComponentEdit::updateGradient);
    connect(m_redSlider, &ColorComponentEdit::valueChangeBegan, this, &RGBSliders::valueChangeBegan);
    connect(m_redSlider, &ColorComponentEdit::valueChangeEnded, this, &RGBSliders::valueChangeEnded);

    connect(m_greenSlider, &ColorComponentEdit::valueChanged, this, &RGBSliders::greenChanged);
    connect(m_greenSlider, &ColorComponentEdit::valueChanged, m_redSlider, &ColorComponentEdit::updateGradient);
    connect(m_greenSlider, &ColorComponentEdit::valueChanged, m_blueSlider, &ColorComponentEdit::updateGradient);
    connect(m_greenSlider, &ColorComponentEdit::valueChangeBegan, this, &RGBSliders::valueChangeBegan);
    connect(m_greenSlider, &ColorComponentEdit::valueChangeEnded, this, &RGBSliders::valueChangeEnded);

    connect(m_blueSlider, &ColorComponentEdit::valueChanged, this, &RGBSliders::blueChanged);
    connect(m_blueSlider, &ColorComponentEdit::valueChanged, m_redSlider, &ColorComponentEdit::updateGradient);
    connect(m_blueSlider, &ColorComponentEdit::valueChanged, m_greenSlider, &ColorComponentEdit::updateGradient);
    connect(m_blueSlider, &ColorComponentEdit::valueChangeBegan, this, &RGBSliders::valueChangeBegan);
    connect(m_blueSlider, &ColorComponentEdit::valueChangeEnded, this, &RGBSliders::valueChangeEnded);
}

qreal RGBSliders::red() const
{
    return m_redSlider->value();
}

qreal RGBSliders::green() const
{
    return m_greenSlider->value();
}

qreal RGBSliders::blue() const
{
    return m_blueSlider->value();
}

void RGBSliders::setRed(qreal red)
{
    const QSignalBlocker b(m_redSlider);
    m_redSlider->setValue(red);
    m_greenSlider->updateGradient();
    m_blueSlider->updateGradient();
}

void RGBSliders::setGreen(qreal green)
{
    const QSignalBlocker b(m_greenSlider);
    m_greenSlider->setValue(green);
    m_redSlider->updateGradient();
    m_blueSlider->updateGradient();
}

void RGBSliders::setBlue(qreal blue)
{
    const QSignalBlocker b(m_blueSlider);
    m_blueSlider->setValue(blue);
    m_redSlider->updateGradient();
    m_greenSlider->updateGradient();
}


} // namespace AzQtComponents
#include <Components/Widgets/ColorPicker/ColorComponentSliders.moc>
