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

// include required headers
#include "AttributeWidgets.h"
#include "MysticQtManager.h"
#include <QVBoxLayout>
#include <MCore/Source/Compare.h>
#include "PropertyWidget.h"

#define SPINNER_WIDTH 58

namespace MysticQt
{
    // constructor
    AttributeWidget::AttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : QWidget()
    {
        MCORE_UNUSED(creationMode);

        mReadOnly                   = readOnly;
        mAttributeSettings          = attributeSettings;
        mAttributes                 = attributes;
        mFirstAttribute             = nullptr;
        mCustomData                 = customData;
        mNeedsHeavyUpdate           = false;
        mNeedsObjectUpdate          = false;
        mNeedsAttributeWindowUpdate = false;
        mAttribChangedFunc          = func;

        if (attributeSettings)
        {
            mNeedsHeavyUpdate           = attributeSettings->GetFlag(MCore::AttributeSettings::FLAGINDEX_REINITGUI_ONVALUECHANGE);
            mNeedsObjectUpdate          = attributeSettings->GetFlag(MCore::AttributeSettings::FLAGINDEX_REINITOBJECT_ONVALUECHANGE);
            mNeedsAttributeWindowUpdate = attributeSettings->GetFlag(MCore::AttributeSettings::FLAGINDEX_REINIT_ATTRIBUTEWINDOW);
        }

        mAttributes.SetMemoryCategory(MEMCATEGORY_MYSTICQT);

        if (attributes.GetLength() > 0)
        {
            mFirstAttribute = attributes[0];
        }
    }


    // destructor
    AttributeWidget::~AttributeWidget()
    {
    }


    // called when an attribute inside the widget got changed, this internally calls the callbacks from the attribute widget factory
    void AttributeWidget::UpdateInterface()
    {
        AttributeWidgetFactory* widgetFactory = GetMysticQt()->GetAttributeWidgetFactory();

        widgetFactory->OnAttributeChanged();

        if (mNeedsHeavyUpdate)
        {
            widgetFactory->OnStructureChanged();
        }

        if (mNeedsObjectUpdate)
        {
            widgetFactory->OnObjectChanged();
        }

        if (mNeedsAttributeWindowUpdate)
        {
            widgetFactory->OnUpdateAttributeWindow();
        }
    }


    void AttributeWidget::CreateStandardLayout(QWidget* widget, const char* description)
    {
        //QLabel*           label       = new QLabel( mNameString.AsChar() );
        QVBoxLayout*        layout      = new QVBoxLayout();

        //layout->addWidget(label);
        layout->addWidget(widget);
        layout->setAlignment(Qt::AlignLeft);
        layout->setSizeConstraint(QLayout::SizeConstraint::SetMinimumSize);
        widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        setLayout(layout);

        //label->setMinimumWidth(ATTRIBUTEWIDGET_LABEL_WIDTH);
        //label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
        //layout->setAlignment(Qt::AlignTop);

        //layout->addSpacerItem( new QSpacerItem(1,1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding) );

        layout->setMargin(0);
        widget->setToolTip(description);
    }


    void AttributeWidget::CreateStandardLayout(QWidget* widget, MCore::AttributeSettings* attributeSettings)
    {
        //MCORE_UNUSED( attributeSettings );
        // TODO: check this, shouldn't we use attributeSettings here?
        MCORE_ASSERT(attributeSettings);
        CreateStandardLayout(widget, attributeSettings->GetDescription());
    }


    void AttributeWidget::CreateStandardLayout(QWidget* widgetA, QWidget* widgetB, MCore::AttributeSettings* attributeSettings)
    {
        // TODO: check this
        MCORE_ASSERT(mAttributeSettings);

        QWidget*        rightWidget = new QWidget();
        QHBoxLayout*    rightLayout = new QHBoxLayout();

        //rightLayout->setAlignment(Qt::AlignTop);

        rightLayout->addWidget(widgetA);
        rightLayout->addWidget(widgetB);
        rightLayout->setMargin(0);
        rightLayout->setAlignment(Qt::AlignLeft);

        rightWidget->setToolTip(mAttributeSettings->GetDescription());
        rightWidget->setLayout(rightLayout);
        rightWidget->setObjectName("TransparentWidget");

        CreateStandardLayout(rightWidget, attributeSettings);
    }

    //-----------------------------------------------------------------------------------------------------------------

    CheckBoxAttributeWidget::CheckBoxAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        mCheckBox = new QCheckBox();
        CreateStandardLayout(mCheckBox, attributeSettings);

        SetValue(mFirstAttribute);

        mCheckBox->setEnabled(!readOnly);
        connect(mCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnCheckBox(int)));
    }


    void CheckBoxAttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        if (attribute)
        {
            const float value = static_cast<MCore::AttributeFloat*>(attribute)->GetValue();
            if (MCore::Math::Abs(value) > MCore::Math::epsilon)
            {
                if (mCheckBox->checkState() != Qt::Checked)
                {
                    mCheckBox->setCheckState(Qt::Checked);
                }
            }
            else
            {
                if (mCheckBox->checkState() != Qt::Unchecked)
                {
                    mCheckBox->setCheckState(Qt::Unchecked);
                }
            }
        }
        else
        {
            if (mCheckBox->checkState() != Qt::Unchecked)
            {
                mCheckBox->setCheckState(Qt::Unchecked);
            }
        }
    }


    void CheckBoxAttributeWidget::SetReadOnly(bool readOnly)
    {
        mCheckBox->setDisabled(readOnly);
    }


    void CheckBoxAttributeWidget::OnCheckBox(int state)
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (state == Qt::Checked)
            {
                static_cast<MCore::AttributeFloat*>(mAttributes[i])->SetValue(1.0f);
            }
            else
            {
                static_cast<MCore::AttributeFloat*>(mAttributes[i])->SetValue(0.0f);
            }

            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    FloatSpinnerAttributeWidget::FloatSpinnerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        mSpinBox = new MysticQt::DoubleSpinBox();
        mSpinBox->setMinimumWidth(SPINNER_WIDTH);
        mSpinBox->setMaximumWidth(SPINNER_WIDTH);

        CreateStandardLayout(mSpinBox, attributeSettings);

        MCore::Attribute* minAttribute = attributeSettings->GetMinValue();
        MCore::Attribute* maxAttribute = attributeSettings->GetMaxValue();

        float minValue  = (minAttribute) ? static_cast<MCore::AttributeFloat*>(minAttribute)->GetValue() : std::numeric_limits<float>::lowest();
        float maxValue  = (maxAttribute) ? static_cast<MCore::AttributeFloat*>(maxAttribute)->GetValue() : std::numeric_limits<float>::max();
        float stepSize  = 0.01f;

        mSpinBox->setRange(minValue, maxValue);
        mSpinBox->setSingleStep(stepSize);
        mSpinBox->setDecimals(6);
        mSpinBox->setEnabled(!readOnly);

        SetValue(mFirstAttribute);

        connect(mSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinner(double)));
    }


    void FloatSpinnerAttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        float value = (attribute) ? static_cast<MCore::AttributeFloat*>(attribute)->GetValue() : 0.0f;

        if (mSpinBox->value() != value)
        {
            mSpinBox->setValue(value);
        }
    }


    void FloatSpinnerAttributeWidget::SetReadOnly(bool readOnly)
    {
        mSpinBox->setDisabled(readOnly);
    }


    // double spinner
    void FloatSpinnerAttributeWidget::OnDoubleSpinner(double value)
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeFloat*>(mAttributes[i])->SetValue((float)value);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    IntSpinnerAttributeWidget::IntSpinnerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        mSpinBox = new MysticQt::IntSpinBox();
        mSpinBox->setMinimumWidth(SPINNER_WIDTH);
        mSpinBox->setMaximumWidth(SPINNER_WIDTH);
        CreateStandardLayout(mSpinBox, attributeSettings);

        int value       = (mFirstAttribute) ? static_cast<MCore::AttributeFloat*>(mFirstAttribute)->GetValue() : 0;
        int minValue    = (attributeSettings->GetMinValue()) ? static_cast<MCore::AttributeFloat*>(attributeSettings->GetMinValue())->GetValue() : -1000000000;
        int maxValue    = (attributeSettings->GetMaxValue()) ? static_cast<MCore::AttributeFloat*>(attributeSettings->GetMaxValue())->GetValue() : +1000000000;
        int stepSize    = 1;

        mSpinBox->setRange(minValue, maxValue);
        mSpinBox->setValue(value);
        mSpinBox->setSingleStep(stepSize);
        mSpinBox->setEnabled(!readOnly);

        connect(mSpinBox, SIGNAL(valueChanged(int)), this, SLOT(OnIntSpinner(int)));
    }


    // int spinner
    void IntSpinnerAttributeWidget::OnIntSpinner(int value)
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeFloat*>(mAttributes[i])->SetValue(value);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void IntSpinnerAttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        float value = (attribute) ? static_cast<MCore::AttributeFloat*>(attribute)->GetValue() : 0;

        const int32 newValue = (int32)value;
        if (mSpinBox->value() != newValue)
        {
            mSpinBox->setValue(newValue);
        }
    }


    void IntSpinnerAttributeWidget::SetReadOnly(bool readOnly)
    {
        mSpinBox->setDisabled(readOnly);
    }


    //-----------------------------------------------------------------------------------------------------------------

    FloatSliderAttributeWidget::FloatSliderAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        mSlider = new MysticQt::Slider(Qt::Horizontal);
        CreateStandardLayout(mSlider, attributeSettings);

        mSlider->setRange(0, 1000);
        mSlider->setEnabled(!readOnly);

        SetValue(mFirstAttribute);
        //slider->setSingleStep( stepSize );

        connect(mSlider, SIGNAL(valueChanged(int)), this, SLOT(OnFloatSlider(int)));
    }


    void FloatSliderAttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        float value     = (attribute) ? static_cast<MCore::AttributeFloat*>(attribute)->GetValue() : 0.0f;
        float minValue  = (mAttributeSettings->GetMinValue()) ? static_cast<MCore::AttributeFloat*>(mAttributeSettings->GetMinValue())->GetValue() : 0.0f;
        float maxValue  = (mAttributeSettings->GetMaxValue()) ? static_cast<MCore::AttributeFloat*>(mAttributeSettings->GetMaxValue())->GetValue() : 1.0f;

        const int32 sliderValue = ((value - minValue) / (maxValue - minValue)) * 1000;

        if (mSlider->value() != sliderValue)
        {
            mSlider->setValue(sliderValue);
        }
    }


    void FloatSliderAttributeWidget::SetReadOnly(bool readOnly)
    {
        mSlider->setDisabled(readOnly);
    }


    // float slider value change
    void FloatSliderAttributeWidget::OnFloatSlider(int value)
    {
        float minValue  = (mAttributeSettings->GetMinValue()) ? static_cast<MCore::AttributeFloat*>(mAttributeSettings->GetMinValue())->GetValue() : 0.0f;
        float maxValue  = (mAttributeSettings->GetMaxValue()) ? static_cast<MCore::AttributeFloat*>(mAttributeSettings->GetMaxValue())->GetValue() : 1.0f;
        const float floatValue = minValue + (maxValue - minValue) * (value / 1000.0f);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeFloat*>(mAttributes[i])->SetValue(floatValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    IntSliderAttributeWidget::IntSliderAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        mSlider = new MysticQt::Slider(Qt::Horizontal);
        CreateStandardLayout(mSlider, attributeSettings);

        int value       = (mFirstAttribute) ? static_cast<MCore::AttributeFloat*>(mFirstAttribute)->GetValue() : 0;
        int minValue    = (attributeSettings->GetMinValue()) ? static_cast<MCore::AttributeFloat*>(attributeSettings->GetMinValue())->GetValue() : 0;
        int maxValue    = (attributeSettings->GetMaxValue()) ? static_cast<MCore::AttributeFloat*>(attributeSettings->GetMaxValue())->GetValue() : 100;

        mSlider->setRange(0, 1000);
        mSlider->setEnabled(!readOnly);

        int stepSize = 1;
        const int32 sliderValue = ((value - minValue) / (float)(maxValue - minValue)) * 1000.0f;
        mSlider->setValue(sliderValue);
        mSlider->setSingleStep(stepSize);

        connect(mSlider, SIGNAL(valueChanged(int)), this, SLOT(OnIntSlider(int)));
    }


    // int slider value change
    void IntSliderAttributeWidget::OnIntSlider(int value)
    {
        const int32 minValue = (mAttributeSettings->GetMinValue()) ? static_cast<MCore::AttributeFloat*>(mAttributeSettings->GetMinValue())->GetValue() : 0;
        const int32 maxValue = (mAttributeSettings->GetMaxValue()) ? static_cast<MCore::AttributeFloat*>(mAttributeSettings->GetMaxValue())->GetValue() : 100;
        const int32 intValue = minValue + (maxValue - minValue) * (value / 1000.0f);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeFloat*>(mAttributes[i])->SetValue(intValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void IntSliderAttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        const float value       = (attribute) ? static_cast<MCore::AttributeFloat*>(attribute)->GetValue() : 0.0f;
        const int32 minValue = (mAttributeSettings->GetMinValue()) ? static_cast<MCore::AttributeFloat*>(mAttributeSettings->GetMinValue())->GetValue() : 0;
        const int32 maxValue = (mAttributeSettings->GetMaxValue()) ? static_cast<MCore::AttributeFloat*>(mAttributeSettings->GetMaxValue())->GetValue() : 100;

        const int32 sliderValue = ((value - minValue) / (float)(maxValue - minValue)) * 1000;
        if (mSlider->value() != sliderValue)
        {
            mSlider->setValue(sliderValue);
        }
    }


    void IntSliderAttributeWidget::SetReadOnly(bool readOnly)
    {
        mSlider->setDisabled(readOnly);
    }



    //-----------------------------------------------------------------------------------------------------------------

    ComboBoxAttributeWidget::ComboBoxAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        mComboBox = new MysticQt::ComboBox();
        CreateStandardLayout(mComboBox, attributeSettings);

        int value = (mFirstAttribute) ? static_cast<MCore::AttributeFloat*>(mFirstAttribute)->GetValue() : 0;

        const uint32 numComboValues = attributeSettings->GetNumComboValues();
        for (uint32 i = 0; i < numComboValues; ++i)
        {
            mComboBox->addItem(attributeSettings->GetComboValueString(i).AsChar());
        }

        mComboBox->setCurrentIndex(value);
        mComboBox->setEditable(false);
        mComboBox->setEnabled(!readOnly);

        connect(mComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnComboBox(int)));
    }


    // adjust a combobox value
    void ComboBoxAttributeWidget::OnComboBox(int value)
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeFloat*>(mAttributes[i])->SetValue(value);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void ComboBoxAttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        float value     = (attribute) ? static_cast<MCore::AttributeFloat*>(attribute)->GetValue() : 0.0f;
        const int32 newIndex = (int32)value;

        if (mComboBox->currentIndex() != newIndex)
        {
            mComboBox->setCurrentIndex(newIndex);
        }
    }


    void ComboBoxAttributeWidget::SetReadOnly(bool readOnly)
    {
        mComboBox->setDisabled(readOnly);
    }


    //-----------------------------------------------------------------------------------------------------------------

    Vector2AttributeWidget::Vector2AttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        QWidget*                    widget      = new QWidget();
        QHBoxLayout*                hLayout     = new QHBoxLayout();
        MysticQt::DoubleSpinBox*    spinBoxX    = new MysticQt::DoubleSpinBox();
        MysticQt::DoubleSpinBox*    spinBoxY    = new MysticQt::DoubleSpinBox();

        mSpinBoxX = spinBoxX;
        mSpinBoxY = spinBoxY;

        spinBoxX->setMinimumWidth(SPINNER_WIDTH);
        spinBoxX->setMaximumWidth(SPINNER_WIDTH);

        spinBoxY->setMinimumWidth(SPINNER_WIDTH);
        spinBoxY->setMaximumWidth(SPINNER_WIDTH);

        hLayout->addWidget(spinBoxX);
        hLayout->addWidget(spinBoxY);
        hLayout->setMargin(0);
        hLayout->setSpacing(1);
        hLayout->setAlignment(Qt::AlignLeft);
        widget->setLayout(hLayout);
        widget->setObjectName("TransparentWidget");
        CreateStandardLayout(widget, attributeSettings);

        AZ::Vector2 value       = (mFirstAttribute) ? static_cast<MCore::AttributeVector2*>(mFirstAttribute)->GetValue() : AZ::Vector2(0.0f, 0.0f);
        AZ::Vector2 minValue    = (attributeSettings->GetMinValue()) ? static_cast<MCore::AttributeVector2*>(attributeSettings->GetMinValue())->GetValue() : AZ::Vector2(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
        AZ::Vector2 maxValue    = (attributeSettings->GetMaxValue()) ? static_cast<MCore::AttributeVector2*>(attributeSettings->GetMaxValue())->GetValue() : AZ::Vector2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        float           stepSize    = 0.1f;

        spinBoxX->setRange(minValue.GetX(), maxValue.GetX());
        spinBoxX->setValue(value.GetX());
        spinBoxX->setSingleStep(stepSize);
        //spinBoxX->setPrefix("x: ");
        spinBoxX->setEnabled(!readOnly);
        spinBoxX->setDecimals(6);

        spinBoxY->setRange(minValue.GetY(), maxValue.GetY());
        spinBoxY->setValue(value.GetY());
        spinBoxY->setSingleStep(stepSize);
        //spinBoxY->setPrefix("y: ");
        spinBoxY->setEnabled(!readOnly);
        spinBoxY->setDecimals(6);

        widget->setTabOrder(spinBoxX->GetLineEdit(), spinBoxY->GetLineEdit());

        connect(spinBoxX, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerX(double)));
        connect(spinBoxY, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerY(double)));
    }


    void Vector2AttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        AZ::Vector2 value = (attribute) ? static_cast<MCore::AttributeVector2*>(attribute)->GetValue() : AZ::Vector2(0.0f, 0.0f);

        if (MCore::Compare<float>::CheckIfIsClose(value.GetX(), mSpinBoxX->value(), 0.000001f) == false)
        {
            mSpinBoxX->setValue(value.GetX());
        }

        if (MCore::Compare<float>::CheckIfIsClose(value.GetY(), mSpinBoxY->value(), 0.000001f) == false)
        {
            mSpinBoxY->setValue(value.GetY());
        }
    }


    void Vector2AttributeWidget::SetReadOnly(bool readOnly)
    {
        mSpinBoxX->setDisabled(readOnly);
        mSpinBoxY->setDisabled(readOnly);
    }


    void Vector2AttributeWidget::OnDoubleSpinnerX(double value)
    {
        AZ::Vector2 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector2*>(mFirstAttribute)->GetValue() : AZ::Vector2(mSpinBoxX->value(), mSpinBoxY->value());

        curValue.SetX(value);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector2*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void Vector2AttributeWidget::OnDoubleSpinnerY(double value)
    {
        AZ::Vector2 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector2*>(mFirstAttribute)->GetValue() : AZ::Vector2(mSpinBoxX->value(), mSpinBoxY->value());
        curValue.SetY(value);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector2*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    Vector3AttributeWidget::Vector3AttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        QWidget*                    widget      = new QWidget();
        QHBoxLayout*                hLayout     = new QHBoxLayout();
        MysticQt::DoubleSpinBox*    spinBoxX    = new MysticQt::DoubleSpinBox();
        MysticQt::DoubleSpinBox*    spinBoxY    = new MysticQt::DoubleSpinBox();
        MysticQt::DoubleSpinBox*    spinBoxZ    = new MysticQt::DoubleSpinBox();

        mSpinBoxX = spinBoxX;
        mSpinBoxY = spinBoxY;
        mSpinBoxZ = spinBoxZ;

        spinBoxX->setMinimumWidth(SPINNER_WIDTH);
        spinBoxX->setMaximumWidth(SPINNER_WIDTH);

        spinBoxY->setMinimumWidth(SPINNER_WIDTH);
        spinBoxY->setMaximumWidth(SPINNER_WIDTH);

        spinBoxZ->setMinimumWidth(SPINNER_WIDTH);
        spinBoxZ->setMaximumWidth(SPINNER_WIDTH);

        hLayout->addWidget(spinBoxX);
        hLayout->addWidget(spinBoxY);
        hLayout->addWidget(spinBoxZ);
        hLayout->setMargin(0);
        hLayout->setSpacing(1);
        hLayout->setAlignment(Qt::AlignLeft);
        widget->setLayout(hLayout);
        widget->setObjectName("TransparentWidget");
        CreateStandardLayout(widget, attributeSettings);

        MCore::Vector3  value       = (mFirstAttribute) ? static_cast<MCore::AttributeVector3*>(mFirstAttribute)->GetValue() : MCore::Vector3(0.0f, 0.0f, 0.0f);
        MCore::Vector3  minValue    = (attributeSettings->GetMinValue()) ? static_cast<MCore::AttributeVector3*>(attributeSettings->GetMinValue())->GetValue() : MCore::Vector3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
        MCore::Vector3  maxValue    = (attributeSettings->GetMaxValue()) ? static_cast<MCore::AttributeVector3*>(attributeSettings->GetMaxValue())->GetValue() : MCore::Vector3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        float           stepSize    = 0.1f;

        spinBoxX->setRange(minValue.x, maxValue.x);
        spinBoxX->setValue(value.x);
        spinBoxX->setSingleStep(stepSize);
        //spinBoxX->setPrefix("x: ");
        spinBoxX->setEnabled(!readOnly);
        spinBoxX->setDecimals(6);

        spinBoxY->setRange(minValue.y, maxValue.y);
        spinBoxY->setValue(value.y);
        spinBoxY->setSingleStep(stepSize);
        //spinBoxY->setPrefix("y: ");
        spinBoxY->setEnabled(!readOnly);
        spinBoxY->setDecimals(6);

        spinBoxZ->setRange(minValue.z, maxValue.z);
        spinBoxZ->setValue(value.z);
        spinBoxZ->setSingleStep(stepSize);
        //spinBoxZ->setPrefix("z: ");
        spinBoxZ->setEnabled(!readOnly);
        spinBoxZ->setDecimals(6);

        widget->setTabOrder(spinBoxX->GetLineEdit(), spinBoxY->GetLineEdit());
        widget->setTabOrder(spinBoxY->GetLineEdit(), spinBoxZ->GetLineEdit());

        connect(spinBoxX, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerX(double)));
        connect(spinBoxY, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerY(double)));
        connect(spinBoxZ, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerZ(double)));
    }


    void Vector3AttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        MCore::Vector3 value = (attribute) ? static_cast<MCore::AttributeVector3*>(attribute)->GetValue() : MCore::Vector3(0.0f, 0.0f, 0.0f);

        if (MCore::Compare<float>::CheckIfIsClose(value.x, mSpinBoxX->value(), 0.000001f) == false)
        {
            mSpinBoxX->setValue(value.x);
        }

        if (MCore::Compare<float>::CheckIfIsClose(value.y, mSpinBoxY->value(), 0.000001f) == false)
        {
            mSpinBoxY->setValue(value.y);
        }

        if (MCore::Compare<float>::CheckIfIsClose(value.z, mSpinBoxZ->value(), 0.000001f) == false)
        {
            mSpinBoxZ->setValue(value.z);
        }
    }


    void Vector3AttributeWidget::SetReadOnly(bool readOnly)
    {
        mSpinBoxX->setDisabled(readOnly);
        mSpinBoxY->setDisabled(readOnly);
        mSpinBoxZ->setDisabled(readOnly);
    }


    void Vector3AttributeWidget::OnDoubleSpinnerX(double value)
    {
        MCore::Vector3 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector3*>(mFirstAttribute)->GetValue() : MCore::Vector3(mSpinBoxX->value(), mSpinBoxY->value(), mSpinBoxZ->value());
        curValue.x = value;

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector3*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void Vector3AttributeWidget::OnDoubleSpinnerY(double value)
    {
        MCore::Vector3 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector3*>(mFirstAttribute)->GetValue() : MCore::Vector3(mSpinBoxX->value(), mSpinBoxY->value(), mSpinBoxZ->value());
        curValue.y = value;

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector3*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void Vector3AttributeWidget::OnDoubleSpinnerZ(double value)
    {
        MCore::Vector3 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector3*>(mFirstAttribute)->GetValue() : MCore::Vector3(mSpinBoxX->value(), mSpinBoxY->value(), mSpinBoxZ->value());
        curValue.z = value;

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector3*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    Vector4AttributeWidget::Vector4AttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        QWidget*                    widget      = new QWidget();
        QHBoxLayout*                hLayout     = new QHBoxLayout();
        MysticQt::DoubleSpinBox*    spinBoxX    = new MysticQt::DoubleSpinBox();
        MysticQt::DoubleSpinBox*    spinBoxY    = new MysticQt::DoubleSpinBox();
        MysticQt::DoubleSpinBox*    spinBoxZ    = new MysticQt::DoubleSpinBox();
        MysticQt::DoubleSpinBox*    spinBoxW    = new MysticQt::DoubleSpinBox();

        mSpinBoxX = spinBoxX;
        mSpinBoxY = spinBoxY;
        mSpinBoxZ = spinBoxZ;
        mSpinBoxW = spinBoxW;

        spinBoxX->setMinimumWidth(SPINNER_WIDTH);
        spinBoxX->setMaximumWidth(SPINNER_WIDTH);

        spinBoxY->setMinimumWidth(SPINNER_WIDTH);
        spinBoxY->setMaximumWidth(SPINNER_WIDTH);

        spinBoxZ->setMinimumWidth(SPINNER_WIDTH);
        spinBoxZ->setMaximumWidth(SPINNER_WIDTH);

        spinBoxW->setMinimumWidth(SPINNER_WIDTH);
        spinBoxW->setMaximumWidth(SPINNER_WIDTH);

        hLayout->addWidget(spinBoxX);
        hLayout->addWidget(spinBoxY);
        hLayout->addWidget(spinBoxZ);
        hLayout->addWidget(spinBoxW);
        hLayout->setMargin(0);
        hLayout->setSpacing(1);
        hLayout->setAlignment(Qt::AlignLeft);
        widget->setLayout(hLayout);
        widget->setObjectName("TransparentWidget");
        CreateStandardLayout(widget, attributeSettings);

        AZ::Vector4 value       = (mFirstAttribute) ? static_cast<MCore::AttributeVector4*>(mFirstAttribute)->GetValue() : AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
        AZ::Vector4 minValue    = (attributeSettings->GetMinValue()) ? static_cast<MCore::AttributeVector4*>(attributeSettings->GetMinValue())->GetValue() : AZ::Vector4(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
        AZ::Vector4 maxValue    = (attributeSettings->GetMaxValue()) ? static_cast<MCore::AttributeVector4*>(attributeSettings->GetMaxValue())->GetValue() : AZ::Vector4(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        float           stepSize    = 0.1f;

        spinBoxX->setRange(minValue.GetX(), maxValue.GetX());
        spinBoxX->setValue(value.GetX());
        spinBoxX->setSingleStep(stepSize);
        //spinBoxX->setPrefix("x: ");
        spinBoxX->setEnabled(!readOnly);
        spinBoxX->setDecimals(6);

        spinBoxY->setRange(minValue.GetY(), maxValue.GetY());
        spinBoxY->setValue(value.GetY());
        spinBoxY->setSingleStep(stepSize);
        //spinBoxY->setPrefix("y: ");
        spinBoxY->setEnabled(!readOnly);
        spinBoxY->setDecimals(6);

        spinBoxZ->setRange(minValue.GetZ(), maxValue.GetZ());
        spinBoxZ->setValue(value.GetZ());
        spinBoxZ->setSingleStep(stepSize);
        //spinBoxZ->setPrefix("z: ");
        spinBoxZ->setEnabled(!readOnly);
        spinBoxZ->setDecimals(6);

        spinBoxW->setRange(minValue.GetW(), maxValue.GetW());
        spinBoxW->setValue(value.GetW());
        spinBoxW->setSingleStep(stepSize);
        //spinBoxW->setPrefix("w: ");
        spinBoxW->setEnabled(!readOnly);
        spinBoxW->setDecimals(6);

        widget->setTabOrder(spinBoxX->GetLineEdit(), spinBoxY->GetLineEdit());
        widget->setTabOrder(spinBoxY->GetLineEdit(), spinBoxZ->GetLineEdit());
        widget->setTabOrder(spinBoxZ->GetLineEdit(), spinBoxW->GetLineEdit());

        connect(spinBoxX, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerX(double)));
        connect(spinBoxY, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerY(double)));
        connect(spinBoxZ, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerZ(double)));
        connect(spinBoxW, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerW(double)));
    }


    void Vector4AttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        AZ::Vector4 value = (attribute) ? static_cast<MCore::AttributeVector4*>(attribute)->GetValue() : AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f);

        if (MCore::Compare<float>::CheckIfIsClose(value.GetX(), mSpinBoxX->value(), 0.000001f) == false)
        {
            mSpinBoxX->setValue(value.GetX());
        }

        if (MCore::Compare<float>::CheckIfIsClose(value.GetY(), mSpinBoxY->value(), 0.000001f) == false)
        {
            mSpinBoxY->setValue(value.GetY());
        }

        if (MCore::Compare<float>::CheckIfIsClose(value.GetZ(), mSpinBoxZ->value(), 0.000001f) == false)
        {
            mSpinBoxZ->setValue(value.GetZ());
        }

        if (MCore::Compare<float>::CheckIfIsClose(value.GetW(), mSpinBoxW->value(), 0.000001f) == false)
        {
            mSpinBoxW->setValue(value.GetW());
        }
    }


    void Vector4AttributeWidget::SetReadOnly(bool readOnly)
    {
        mSpinBoxX->setDisabled(readOnly);
        mSpinBoxY->setDisabled(readOnly);
        mSpinBoxZ->setDisabled(readOnly);
        mSpinBoxW->setDisabled(readOnly);
    }


    void Vector4AttributeWidget::OnDoubleSpinnerX(double value)
    {
        AZ::Vector4 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector4*>(mFirstAttribute)->GetValue() : AZ::Vector4(mSpinBoxX->value(), mSpinBoxY->value(), mSpinBoxZ->value(), mSpinBoxW->value());
        curValue.SetX(value);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector4*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void Vector4AttributeWidget::OnDoubleSpinnerY(double value)
    {
        AZ::Vector4 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector4*>(mFirstAttribute)->GetValue() : AZ::Vector4(mSpinBoxX->value(), mSpinBoxY->value(), mSpinBoxZ->value(), mSpinBoxW->value());
        curValue.SetY(value);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector4*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void Vector4AttributeWidget::OnDoubleSpinnerZ(double value)
    {
        AZ::Vector4 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector4*>(mFirstAttribute)->GetValue() : AZ::Vector4(mSpinBoxX->value(), mSpinBoxY->value(), mSpinBoxZ->value(), mSpinBoxW->value());
        curValue.SetZ(value);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector4*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void Vector4AttributeWidget::OnDoubleSpinnerW(double value)
    {
        AZ::Vector4 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector4*>(mFirstAttribute)->GetValue() : AZ::Vector4(mSpinBoxX->value(), mSpinBoxY->value(), mSpinBoxZ->value(), mSpinBoxW->value());
        curValue.SetW(value);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector4*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    StringAttributeWidget::StringAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        QLineEdit* lineEdit = new QLineEdit();
        mLineEdit = lineEdit;
        CreateStandardLayout(lineEdit, attributeSettings);

        const char* value = (mFirstAttribute) ? static_cast<MCore::AttributeString*>(mFirstAttribute)->AsChar() : "";
        lineEdit->setText(value);
        lineEdit->setReadOnly(readOnly);

        connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(OnStringChange()));
    }


    // a string changed
    void StringAttributeWidget::OnStringChange()
    {
        assert(sender()->inherits("QLineEdit"));
        QLineEdit* widget = qobject_cast<QLineEdit*>(sender());

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(FromQtString(widget->text()));
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void StringAttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        QString value;
        if (attribute)
        {
            value = static_cast<MCore::AttributeString*>(attribute)->GetValue().AsChar();
        }

        if (value != mLineEdit->text())
        {
            mLineEdit->setText(value);
        }
    }


    void StringAttributeWidget::SetReadOnly(bool readOnly)
    {
        mLineEdit->setReadOnly(readOnly);
    }


    //-----------------------------------------------------------------------------------------------------------------

    ColorAttributeWidget::ColorAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        const MCore::RGBAColor& color = (mFirstAttribute) ? static_cast<MCore::AttributeColor*>(mFirstAttribute)->GetValue() : MCore::RGBAColor(0.0f, 0.0f, 0.0f, 0.0f);

        ColorLabel* colorLabel = new ColorLabel(color, nullptr, nullptr, !readOnly);
        colorLabel->setMaximumWidth(57);
        mColorLabel = colorLabel;
        CreateStandardLayout(colorLabel, attributeSettings);

        connect(colorLabel, SIGNAL(ColorChangeEvent()), this, SLOT(OnColorChanged()));
    }


    // a string changed
    void ColorAttributeWidget::OnColorChanged()
    {
        ColorLabel* widget = qobject_cast<ColorLabel*>(sender());

        const MCore::RGBAColor& newColor = widget->GetRGBAColor();

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeColor*> (mAttributes[i])->SetValue(newColor);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void ColorAttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        const MCore::RGBAColor value = (attribute) ? static_cast<MCore::AttributeColor*>(attribute)->GetValue() : MCore::RGBAColor(0.5f, 0.5f, 0.5f, 1.0f);
        const uint32 colorCode = value.ToInt();
        mColorLabel->ColorChanged(QColor(MCore::ExtractRed(colorCode), MCore::ExtractGreen(colorCode), MCore::ExtractBlue(colorCode), MCore::ExtractAlpha(colorCode)));
    }


    void ColorAttributeWidget::SetReadOnly(bool readOnly)
    {
        mColorLabel->setDisabled(readOnly);
    }


    //-----------------------------------------------------------------------------------------------------------------

    //
    AttributeSetAttributeWidget::AttributeSetAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const AttributeChangedFunction& func)
        : AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        mPropertyWidget = new PropertyWidget();
        mPropertyWidget->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);
        mPropertyWidget->setRootIsDecorated(false);
        if (mFirstAttribute)
        {
            SetValue(mFirstAttribute);
        }
        CreateStandardLayout(mPropertyWidget, attributeSettings);
    }


    //
    void AttributeSetAttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        MCORE_ASSERT(attribute->GetType() == MCore::AttributeSet::TYPE_ID);
        MCore::AttributeSet* attributeSet = static_cast<MCore::AttributeSet*>(attribute);
        mPropertyWidget->AddAttributeSetReadOnly(attributeSet, true, "");
    }


    //
    void AttributeSetAttributeWidget::SetReadOnly(bool readOnly)
    {
        MCORE_UNUSED(readOnly);
        //mPropertyWidget->SetReadOnly(true);
    }
} // namespace MysticQt

#include <MysticQt/Source/AttributeWidgets.moc>