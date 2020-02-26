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
#include <AzQtComponents/Components/Widgets/SliderCombo.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <QHBoxLayout>
#include <QSignalBlocker>

namespace AzQtComponents
{
SliderCombo::SliderCombo(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_spinbox = new SpinBox(this);
    m_spinbox->setMinimumWidth(66);
    layout->addWidget(m_spinbox);

    m_slider = new SliderInt(this);
    layout->addWidget(m_slider);

    connect(m_slider, &SliderInt::valueChanged, [this](int value) {
        m_spinbox->setInitialValueWasSetting(true);
        QSignalBlocker block(m_spinbox);
        m_spinbox->setValue(value);

    });
    connect(m_spinbox, static_cast<void(SpinBox::*)(int)>(&SpinBox::valueChanged), [this](int value) {
        QSignalBlocker block(m_slider);
        m_slider->setValue(value);
    });
    m_spinbox->setInitialValueWasSetting(false);
}

SliderCombo::~SliderCombo()
{
}

void SliderCombo::setValue(int value)
{
    if (m_slider->value() != value) {
        m_spinbox->setInitialValueWasSetting(true);
        m_slider->setValue(value);
        m_spinbox->setValue(value);
        Q_EMIT valueChanged();
    }
}

SliderInt* SliderCombo::slider() const
{
    return m_slider;
}

SpinBox* SliderCombo::spinbox() const
{
    return m_spinbox;
}


int SliderCombo::value() const
{
    return m_slider->value();
}

void SliderCombo::setMinimum(int min)
{
    m_slider->setMinimum(min);
    m_spinbox->setMinimum(min);
}

int SliderCombo::minimum() const
{
    return m_slider->minimum();
}

void SliderCombo::setMaximum(int max)
{
    m_slider->setMaximum(max);
    m_spinbox->setMaximum(max);
}

int SliderCombo::maximum() const
{
    return m_slider->maximum();
}

void SliderCombo::setRange(int min, int max)
{
    m_slider->setRange(min, max);
    m_spinbox->setRange(min, max);
}

SliderDoubleCombo::SliderDoubleCombo(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_spinbox = new DoubleSpinBox(this);
    m_spinbox->setMinimumWidth(66);
    layout->addWidget(m_spinbox);

    m_slider = new SliderDouble(this);
    layout->addWidget(m_slider);

    connect(m_slider, &SliderDouble::valueChanged, [this](double value) {
        m_spinbox->setInitialValueWasSetting(true);
        QSignalBlocker block(m_spinbox);
        m_spinbox->updateValue(value);
    });
    connect(m_spinbox, static_cast<void(DoubleSpinBox::*)(double)>(&DoubleSpinBox::valueChanged), [this](double value) {
        QSignalBlocker block(m_slider);
        m_slider->setValue(value);
    });
    m_spinbox->setInitialValueWasSetting(false);
}

SliderDoubleCombo::~SliderDoubleCombo()
{
}

void SliderDoubleCombo::setValue(double value)
{
    if (m_slider->value() != value) {
        m_spinbox->setInitialValueWasSetting(true);
        m_slider->setValue(value);
        m_spinbox->setValue(value);
        Q_EMIT valueChanged();
    }
}

SliderDouble* SliderDoubleCombo::slider() const
{
    return m_slider;
}

DoubleSpinBox* SliderDoubleCombo::spinbox() const
{
    return m_spinbox;
}

double SliderDoubleCombo::value() const
{
    return m_slider->value();
}

void SliderDoubleCombo::setMinimum(double min)
{
    m_slider->setMinimum(min);
    m_spinbox->setMinimum(min);
}

double SliderDoubleCombo::minimum() const
{
    return m_slider->minimum();
}

void SliderDoubleCombo::setMaximum(double max)
{
    m_slider->setMaximum(max);
    m_spinbox->setMaximum(max);
}

double SliderDoubleCombo::maximum() const
{
    return m_slider->maximum();
}

void SliderDoubleCombo::setRange(double min, double max)
{
    m_slider->setRange(min, max);
    m_spinbox->setRange(min, max);
}

int SliderDoubleCombo::numSteps() const
{
    return m_slider->numSteps();
}

void SliderDoubleCombo::setNumSteps(int steps)
{
    m_slider->setNumSteps(steps);
}

int SliderDoubleCombo::decimals() const
{
    return m_slider->decimals();
}

void SliderDoubleCombo::setDecimals(int decimals)
{
    m_slider->setDecimals(decimals);
}

}

#include <Components/Widgets/SliderCombo.moc>
