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

// include the required headers
#include "IntSlider.h"
#include <QtWidgets/QHBoxLayout>


namespace MysticQt
{
    // the constructor
    IntSlider::IntSlider(QWidget* parent)
        : QWidget(parent)
    {
        // create the layout
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        // create the slider
        mSlider = new Slider(Qt::Horizontal);
        mSlider->setMinimumWidth(50);

        // create the spinbox
        mSpinBox = new IntSpinBox();

        // connect signals and slots
        connect(mSpinBox,  static_cast<void (MysticQt::IntSpinBox::*)(int)>(&MysticQt::IntSpinBox::valueChanged),                  this,   &MysticQt::IntSlider::OnSpinBoxChanged);
        connect(mSpinBox,  &MysticQt::IntSpinBox::ValueChangedMouseReleased,     this,   &MysticQt::IntSlider::OnSpinBoxEditingFinished);
        connect(mSpinBox,  &MysticQt::IntSpinBox::ValueChangedByTextFinished,    this,   &MysticQt::IntSlider::OnSpinBoxEditingFinished);
        connect(mSlider,   &MysticQt::Slider::valueChanged,                  this,   &MysticQt::IntSlider::OnSliderChanged);
        connect(mSlider,   &MysticQt::Slider::sliderReleased,                   this,   &MysticQt::IntSlider::OnSliderReleased);

        hLayout->addWidget(mSlider);
        hLayout->addWidget(mSpinBox);

        // set the layout
        setLayout(hLayout);
    }


    // destructor
    IntSlider::~IntSlider()
    {
    }


    // enable or disable the interface items
    void IntSlider::SetEnabled(bool isEnabled)
    {
        mSpinBox->setEnabled(isEnabled);
        mSlider->setEnabled(isEnabled);
    }


    // set the range of the slider
    void IntSlider::SetRange(int min, int max)
    {
        mSpinBox->setRange(min, max);
        mSlider->setRange(min, max);
    }


    // set the value for both the spinbox and the slider
    void IntSlider::SetValue(int value)
    {
        // disable signal emitting
        mSpinBox->blockSignals(true);
        mSlider->blockSignals(true);

        mSpinBox->setValue(value);
        mSlider->setValue(value);

        // enable signal emitting
        mSpinBox->blockSignals(false);
        mSlider->blockSignals(false);
    }


    // gets called when the spinbox value changes
    void IntSlider::OnSpinBoxChanged(int value)
    {
        SetValue(value);
        emit ValueChanged(value);
        emit ValueChanged();
    }


    // gets called when the spinbox finished to edit the value
    void IntSlider::OnSpinBoxEditingFinished(int value)
    {
        emit FinishedValueChange(value);
    }


    // gets called when the slider got moved
    void IntSlider::OnSliderChanged(int value)
    {
        SetValue(value);
        emit ValueChanged(value);
        emit ValueChanged();
    }


    // gets called when the slider was moved and then the mouse button got released
    void IntSlider::OnSliderReleased()
    {
        emit FinishedValueChange(mSlider->value());
    }


    // enable or disable signal emitting
    void IntSlider::BlockSignals(bool flag)
    {
        mSpinBox->blockSignals(flag);
        mSlider->blockSignals(flag);
    }


    //==============================================================================================================================

    // the constructor
    FloatSlider::FloatSlider(QWidget* parent)
        : QWidget(parent)
    {
        // create the layout
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        // create the slider
        mSlider = new Slider(Qt::Horizontal);
        mSlider->setMinimumWidth(50);
        mSlider->setRange(0, 1000);

        // create the spinbox
        mSpinBox = new DoubleSpinBox();

        // connect signals and slots
        connect(mSpinBox,  static_cast<void (MysticQt::DoubleSpinBox::*)(double)>(&MysticQt::DoubleSpinBox::valueChanged),               this,   &MysticQt::FloatSlider::OnSpinBoxChanged);
        connect(mSpinBox,  &MysticQt::DoubleSpinBox::ValueChangedMouseReleased,  this,   &MysticQt::FloatSlider::OnSpinBoxEditingFinished);
        connect(mSpinBox,  &MysticQt::DoubleSpinBox::ValueChangedByTextFinished, this,   &MysticQt::FloatSlider::OnSpinBoxEditingFinished);
        connect(mSlider,   &MysticQt::Slider::valueChanged,                  this,   &MysticQt::FloatSlider::OnSliderChanged);
        connect(mSlider,   &MysticQt::Slider::sliderReleased,                   this,   &MysticQt::FloatSlider::OnSliderReleased);

        hLayout->addWidget(mSlider);
        hLayout->addWidget(mSpinBox);

        // set the layout
        setLayout(hLayout);
    }


    // destructor
    FloatSlider::~FloatSlider()
    {
    }


    // enable or disable the interface items
    void FloatSlider::SetEnabled(bool isEnabled)
    {
        mSpinBox->setEnabled(isEnabled);
        mSlider->setEnabled(isEnabled);
    }


    // set the range of the spin box
    void FloatSlider::SetRange(float min, float max)
    {
        mSpinBox->setRange(min, max);
    }


    // set the value for both the spinbox and the slider
    void FloatSlider::SetValue(float value)
    {
        // disable signal emitting
        mSpinBox->blockSignals(true);
        mSlider->blockSignals(true);

        mSpinBox->setValue(value);

        // remap range and set the value
        const float minInput = mSpinBox->minimum();
        const float maxInput = mSpinBox->maximum();
        const float minOutput = 0;
        const float maxOutput = 1000;
        const float result = ((value - minInput) / (maxInput - minInput)) * (maxOutput - minOutput) + minOutput;
        mSlider->setValue((int)result);

        // enable signal emitting
        mSpinBox->blockSignals(false);
        mSlider->blockSignals(false);
    }


    // set the value for both the spinbox and the slider
    void FloatSlider::SetNormalizedValue(float value)
    {
        // disable signal emitting
        mSpinBox->blockSignals(true);
        mSlider->blockSignals(true);

        // remap range and set the value
        const float min = mSpinBox->minimum();
        const float max = mSpinBox->maximum();
        const float result = min + (max - min) * value;
        mSpinBox->setValue(result);

        mSlider->setValue((int)(value * 1000.0));

        // enable signal emitting
        mSpinBox->blockSignals(false);
        mSlider->blockSignals(false);
    }


    // gets called when the spinbox value changes
    void FloatSlider::OnSpinBoxChanged(double value)
    {
        SetValue(value);
        emit ValueChanged(value);
        emit ValueChanged();
    }


    // gets called when the spinbox finished to edit the value
    void FloatSlider::OnSpinBoxEditingFinished(double value)
    {
        emit FinishedValueChange(value);
    }


    // gets called when the slider got moved
    void FloatSlider::OnSliderChanged(int value)
    {
        const float normalizedValue = (float)value / 1000.0f;
        SetNormalizedValue(normalizedValue);
        emit ValueChanged(mSpinBox->value());
        emit ValueChanged();
    }


    // gets called when the slider was moved and then the mouse button got released
    void FloatSlider::OnSliderReleased()
    {
        emit FinishedValueChange(mSpinBox->value());
    }


    // enable or disable signal emitting
    void FloatSlider::BlockSignals(bool flag)
    {
        mSpinBox->blockSignals(flag);
        mSlider->blockSignals(flag);
    }
} // namespace MysticQt

#include <MysticQt/Source/IntSlider.moc>