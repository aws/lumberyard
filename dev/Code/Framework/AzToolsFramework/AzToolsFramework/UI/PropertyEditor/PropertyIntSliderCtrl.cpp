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
#include "PropertyIntSliderCtrl.hxx"
#include "DHQSpinbox.hxx"
#include "DHQSlider.hxx"
#include "PropertyQTConstants.h"
#include <QtWidgets/QHBoxLayout>

namespace AzToolsFramework
{
    DHPropertyIntSlider::DHPropertyIntSlider(QWidget* pParent)
        : QWidget(pParent)
        , m_multiplier(1)
        , m_softMinimum(0)
        , m_softMaximum(0)
        , m_useSoftMinimum(false)
        , m_useSoftMaximum(false)
        , m_pSlider(NULL)
        , m_pSpinBox(NULL)
    {
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        m_pSlider = aznew DHQSlider(Qt::Horizontal, this);
        m_pSpinBox = aznew DHQSpinbox(this);
        m_pSpinBox->setKeyboardTracking(false);  // do not send valueChanged every time a character is typed
        pLayout->setSpacing(4);
        pLayout->setContentsMargins(1, 0, 1, 0);
        pLayout->addWidget(m_pSpinBox);
        pLayout->addWidget(m_pSlider);
        setLayout(pLayout);

        InitializeSliderPropertyWidgets(m_pSlider, m_pSpinBox);
        setFocusPolicy(Qt::StrongFocus);
        setFocusProxy(m_pSpinBox);

        connect(m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(onChildSliderValueChange(int)));
        connect(m_pSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onChildSpinboxValueChange(int)));
    }

    DHPropertyIntSlider::~DHPropertyIntSlider()
    {
    }

    void DHPropertyIntSlider::focusInEvent(QFocusEvent* e)
    {
        QWidget::focusInEvent(e);
        m_pSpinBox->setFocus();
        m_pSpinBox->selectAll();
    }

    void DHPropertyIntSlider::onChildSliderValueChange(int newValue)
    {
        m_pSpinBox->setValue((int)(newValue));
        m_pSpinBox->selectAll();
    }

    void DHPropertyIntSlider::onChildSpinboxValueChange(int newValue)
    {
        m_pSlider->blockSignals(true);

        m_pSlider->setValue(newValue);

        m_pSlider->blockSignals(false);
        emit valueChanged((AZ::u64)newValue * m_multiplier);
    }

    void DHPropertyIntSlider::setMinimum(AZ::s64 min)
    {
        m_pSpinBox->blockSignals(true);
        m_pSlider->blockSignals(true);

        m_pSpinBox->setMinimum((int)(min / m_multiplier));
        AZ::s64 sliderMin = m_useSoftMinimum ? m_softMinimum : min;
        m_pSlider->setMinimum((int)(sliderMin / m_multiplier));

        m_pSlider->blockSignals(false);
        m_pSpinBox->blockSignals(false);
    }

    void DHPropertyIntSlider::setMaximum(AZ::s64 max)
    {
        m_pSpinBox->blockSignals(true);
        m_pSlider->blockSignals(true);

        m_pSpinBox->setMaximum((int)(max / m_multiplier));
        AZ::s64 sliderMax = m_useSoftMaximum ? m_softMaximum : max;
        m_pSlider->setMaximum((int)(sliderMax / m_multiplier));

        m_pSlider->blockSignals(false);
        m_pSpinBox->blockSignals(false);
    }

    void DHPropertyIntSlider::setStep(AZ::s64 val)
    {
        m_pSpinBox->blockSignals(true);
        m_pSlider->blockSignals(true);

        if ((val / m_multiplier) < 1)
        {
            val = m_multiplier;
        }

        m_pSpinBox->setSingleStep((int)(val / m_multiplier));
        m_pSlider->setSingleStep((int)(val / m_multiplier));

        m_pSpinBox->blockSignals(false);
        m_pSlider->blockSignals(false);
    }

    void DHPropertyIntSlider::setValue(AZ::s64 val)
    {
        m_pSpinBox->blockSignals(true);
        m_pSlider->blockSignals(true);

        m_pSpinBox->setValue((int)(val / m_multiplier));
        m_pSlider->setValue((int)(val / m_multiplier));

        m_pSpinBox->blockSignals(false);
        m_pSlider->blockSignals(false);
    }

    void DHPropertyIntSlider::setMultiplier(AZ::s64 val)
    {
        m_pSpinBox->blockSignals(true);
        m_pSlider->blockSignals(true);

        AZ::s64 currentVal = value();
        AZ::s64 currentMax = maximum();
        AZ::s64 currentMin = minimum();
        AZ::s64 currentStep = step();

        m_multiplier = val;

        if ((currentStep / m_multiplier) < 1)
        {
            currentStep = m_multiplier;
        }

        m_pSpinBox->setMinimum((int)(currentMin / m_multiplier));
        m_pSpinBox->setMaximum((int)(currentMax / m_multiplier));
        m_pSpinBox->setSingleStep((int)(currentStep / m_multiplier));
        m_pSpinBox->setValue((int)(currentVal / m_multiplier));

        if (m_useSoftMaximum)
        {
            currentMax = m_softMaximum;
        }
        if (m_useSoftMinimum)
        {
            currentMin = m_softMinimum;
        }

        m_pSlider->setMinimum((int)(currentMin / m_multiplier));
        m_pSlider->setMaximum((int)(currentMax / m_multiplier));
        m_pSlider->setSingleStep((int)(currentStep / m_multiplier));
        m_pSlider->setValue((int)(currentVal / m_multiplier));

        m_pSpinBox->blockSignals(false);
        m_pSlider->blockSignals(false);
    }

    void DHPropertyIntSlider::setSoftMinimum(AZ::s64 val)
    {
        if (val != m_softMinimum || !m_useSoftMinimum)
        {
            m_useSoftMinimum = true;
            m_softMinimum = val;

            m_pSpinBox->blockSignals(true);
            m_pSlider->blockSignals(true);

            m_pSlider->setMinimum((int)(m_softMinimum / m_multiplier));

            m_pSpinBox->blockSignals(false);
            m_pSlider->blockSignals(false);
        }
    }

    void DHPropertyIntSlider::setSoftMaximum(AZ::s64 val)
    {
        if (val != m_softMaximum || !m_useSoftMaximum)
        {
            m_useSoftMaximum = true;
            m_softMaximum = val;

            m_pSpinBox->blockSignals(true);
            m_pSlider->blockSignals(true);

            m_pSlider->setMaximum((int)(m_softMaximum / m_multiplier));

            m_pSpinBox->blockSignals(false);
            m_pSlider->blockSignals(false);
        }
    }

    void DHPropertyIntSlider::setPrefix(QString val)
    {
        m_pSpinBox->setPrefix(val);
    }

    void DHPropertyIntSlider::setSuffix(QString val)
    {
        m_pSpinBox->setSuffix(val);
    }

    AZ::s64 DHPropertyIntSlider::value() const
    {
        return (AZ::s64)m_pSpinBox->value() * m_multiplier;
    }

    AZ::s64 DHPropertyIntSlider::softMinimum() const
    {
        return m_softMaximum * m_multiplier;
    }

    AZ::s64 DHPropertyIntSlider::softMaximum() const
    {
        return m_softMaximum * m_multiplier;
    }

    AZ::s64 DHPropertyIntSlider::minimum() const
    {
        return (AZ::s64)m_pSpinBox->minimum() * m_multiplier;
    }

    AZ::s64 DHPropertyIntSlider::maximum() const
    {
        return (AZ::s64)m_pSpinBox->maximum() * m_multiplier;
    }

    AZ::s64 DHPropertyIntSlider::step() const
    {
        return (AZ::s64)m_pSpinBox->singleStep() * m_multiplier;
    }

    QWidget* DHPropertyIntSlider::GetFirstInTabOrder()
    {
        return m_pSpinBox;
    }
    QWidget* DHPropertyIntSlider::GetLastInTabOrder()
    {
        return m_pSlider;
    }

    void DHPropertyIntSlider::UpdateTabOrder()
    {
        setTabOrder(m_pSpinBox, m_pSlider);
    }

    // a common function to eat attribs, for all int handlers:
    static void ConsumeAttributeCommon(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        (void)debugName;
        AZ::s64 value;
        if (attrib == AZ::Edit::Attributes::Min)
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                GUI->setMinimum(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Min' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::Max)
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                GUI->setMaximum(value);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Max' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::SoftMin)
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                GUI->setSoftMinimum(value);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'SoftMin' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::SoftMax)
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                GUI->setSoftMaximum(value);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'SoftMax' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::Step)
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                GUI->setStep(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Step' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ_CRC("Multiplier", 0xa49aa95b))
        {
            if (attrValue->Read<AZ::s64>(value))
            {
                GUI->setMultiplier(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Multiplier' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::Suffix)
        {
            AZStd::string result;
            if (attrValue->Read<AZStd::string>(result))
            {
                GUI->setSuffix(result.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Suffix' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ_CRC("Prefix", 0x93b1868e))
        {
            AZStd::string result;
            if (attrValue->Read<AZStd::string>(result))
            {
                GUI->setPrefix(result.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Prefix' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
    }

    QWidget* s16PropertySliderHandler::CreateGUI(QWidget* pParent)
    {
        DHPropertyIntSlider* newCtrl = aznew DHPropertyIntSlider(pParent);
        connect(newCtrl, &DHPropertyIntSlider::valueChanged, this, [newCtrl]()
        {
            EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
        });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setMinimum(std::numeric_limits<AZ::s16>::min());
        newCtrl->setMaximum(std::numeric_limits<AZ::s16>::max());

        return newCtrl;
    }

    QWidget* u16PropertySliderHandler::CreateGUI(QWidget* pParent)
    {
        DHPropertyIntSlider* newCtrl = aznew DHPropertyIntSlider(pParent);
        connect(newCtrl, &DHPropertyIntSlider::valueChanged, this, [newCtrl]()
        {
            EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
        });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setMinimum(std::numeric_limits<AZ::u16>::min());
        newCtrl->setMaximum(std::numeric_limits<AZ::u16>::max());

        return newCtrl;
    }

    QWidget* s32PropertySliderHandler::CreateGUI(QWidget* pParent)
    {
        DHPropertyIntSlider* newCtrl = aznew DHPropertyIntSlider(pParent);
        connect(newCtrl, &DHPropertyIntSlider::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setMinimum(std::numeric_limits<AZ::s32>::min());
        newCtrl->setMaximum(std::numeric_limits<AZ::s32>::max());

        return newCtrl;
    }

    QWidget* u32PropertySliderHandler::CreateGUI(QWidget* pParent)
    {
        DHPropertyIntSlider* newCtrl = aznew DHPropertyIntSlider(pParent);
        connect(newCtrl, &DHPropertyIntSlider::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setMinimum(std::numeric_limits<AZ::u32>::min());
        newCtrl->setMaximum(std::numeric_limits<AZ::u32>::max());

        return newCtrl;
    }

    QWidget* s64PropertySliderHandler::CreateGUI(QWidget* pParent)
    {
        DHPropertyIntSlider* newCtrl = aznew DHPropertyIntSlider(pParent);
        connect(newCtrl, &DHPropertyIntSlider::valueChanged, this, [newCtrl]()
        {
            EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
        });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setMinimum(std::numeric_limits<AZ::s64>::min());
        newCtrl->setMaximum(std::numeric_limits<AZ::s64>::max());

        return newCtrl;
    }

    QWidget* u64PropertySliderHandler::CreateGUI(QWidget* pParent)
    {
        DHPropertyIntSlider* newCtrl = aznew DHPropertyIntSlider(pParent);
        connect(newCtrl, &DHPropertyIntSlider::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setMinimum(std::numeric_limits<AZ::u64>::min());
        // here intentionally use AZ::s64
        newCtrl->setMaximum(std::numeric_limits<AZ::s64>::max());

        return newCtrl;
    }

    void s16PropertySliderHandler::ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);

        const auto c_intMin = std::numeric_limits<AZ::s16>::min();
        const auto c_intMax = std::numeric_limits<AZ::s16>::min();

        if ((GUI->maximum() > c_intMax) || (GUI->maximum() < c_intMin) || (GUI->minimum() > c_intMax) || (GUI->minimum() < c_intMin))
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in s32 Slider Box exceeds Min/Max values", debugName, attrib);
        }
    }

    void u16PropertySliderHandler::ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);

        const auto c_uintMin = std::numeric_limits<AZ::u16>::min();
        const auto c_uintMax = std::numeric_limits<AZ::u16>::min();

        if ((GUI->maximum() > c_uintMax) || (GUI->maximum() < c_uintMin) || (GUI->minimum() > c_uintMax) || (GUI->minimum() < c_uintMin))
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in u32 Slider Box exceeds Min/Max values", debugName, attrib);
        }
    }

    void s32PropertySliderHandler::ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);

        const auto c_intMin = std::numeric_limits<AZ::s32>::min();
        const auto c_intMax = std::numeric_limits<AZ::s32>::min();

        if ((GUI->maximum() > c_intMax) || (GUI->maximum() < c_intMin) || (GUI->minimum() > c_intMax) || (GUI->minimum() < c_intMin))
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in s32 Slider Box exceeds Min/Max values", debugName, attrib);
        }
    }

    void u32PropertySliderHandler::ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);

        const auto c_uintMin = std::numeric_limits<AZ::u32>::min();
        const auto c_uintMax = std::numeric_limits<AZ::u32>::min();

        if ((GUI->maximum() > c_uintMax) || (GUI->maximum() < c_uintMin) || (GUI->minimum() > c_uintMax) || (GUI->minimum() < c_uintMin))
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in u32 Slider Box exceeds Min/Max values", debugName, attrib);
        }
    }

    void s64PropertySliderHandler::ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);
    }

    void u64PropertySliderHandler::ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);
        if ((GUI->maximum() < 0) || (GUI->minimum() < 0))
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in u64 Slider Box has a negative min or max attribute", debugName, attrib);
        }
    }

    void s16PropertySliderHandler::WriteGUIValuesIntoProperty(size_t /*index*/, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* /*node*/)
    {
        AZ::s64 val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    void u16PropertySliderHandler::WriteGUIValuesIntoProperty(size_t /*index*/, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* /*node*/)
    {
        AZ::s64 val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    void s32PropertySliderHandler::WriteGUIValuesIntoProperty(size_t /*index*/, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* /*node*/)
    {
        AZ::s64 val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    void u32PropertySliderHandler::WriteGUIValuesIntoProperty(size_t /*index*/, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* /*node*/)
    {
        AZ::s64 val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    void s64PropertySliderHandler::WriteGUIValuesIntoProperty(size_t /*index*/, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* /*node*/)
    {
        AZ::s64 val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    void u64PropertySliderHandler::WriteGUIValuesIntoProperty(size_t /*index*/, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* /*node*/)
    {
        AZ::s64 val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    bool s16PropertySliderHandler::ReadValuesIntoGUI(size_t /*index*/, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* /*node*/)
    {
        GUI->setValue(instance);
        return false;
    }

    bool u16PropertySliderHandler::ReadValuesIntoGUI(size_t /*index*/, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* /*node*/)
    {
        GUI->setValue(instance);
        return false;
    }

    bool s32PropertySliderHandler::ReadValuesIntoGUI(size_t /*index*/, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* /*node*/)
    {
        GUI->setValue(instance);
        return false;
    }

    bool u32PropertySliderHandler::ReadValuesIntoGUI(size_t /*index*/, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* /*node*/)
    {
        GUI->setValue(instance);
        return false;
    }

    bool s64PropertySliderHandler::ReadValuesIntoGUI(size_t /*index*/, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* /*node*/)
    {
        GUI->setValue(instance);
        return false;
    }

    bool u64PropertySliderHandler::ReadValuesIntoGUI(size_t /*index*/, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* /*node*/)
    {
        GUI->setValue(instance);
        return false;
    }

    template<typename T>
    bool ModifyTooltipCommonSigned(QWidget* widget, QString& toolTipString)
    {
        DHPropertyIntSlider* propertyControl = qobject_cast<DHPropertyIntSlider*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[";
            if (propertyControl->minimum() <= std::numeric_limits<T>::min())
            {
                toolTipString += "-" + QString::fromUtf8(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->minimum());
            }
            toolTipString += ", ";
            if (propertyControl->maximum() >= std::numeric_limits<T>::max())
            {
                toolTipString += QString::fromUtf8(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->maximum());
            }
            toolTipString += "]";
            return true;
        }
        return false;
    }

    template<typename T>
    bool ModifyTooltipCommonUnsigned(QWidget* widget, QString& toolTipString)
    {
        DHPropertyIntSlider* propertyControl = qobject_cast<DHPropertyIntSlider*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[" + QString::number(propertyControl->minimum()) + ", ";
            if (propertyControl->maximum() >= std::numeric_limits<T>::max())
            {
                toolTipString += QString::fromUtf8(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->maximum());
            }
            toolTipString += "]";
            return true;
        }
        return false;
    }

    bool s16PropertySliderHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        return ModifyTooltipCommonSigned<AZ::s16>(widget, toolTipString);
    }

    bool u16PropertySliderHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        return ModifyTooltipCommonUnsigned<AZ::u16>(widget, toolTipString);
    }

    bool s32PropertySliderHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        return ModifyTooltipCommonSigned<AZ::s32>(widget, toolTipString);
    }

    bool u32PropertySliderHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        return ModifyTooltipCommonUnsigned<AZ::u32>(widget, toolTipString);
    }

    bool s64PropertySliderHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        return ModifyTooltipCommonSigned<AZ::s64>(widget, toolTipString);
    }

    bool u64PropertySliderHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        // Although it is a u64 handler, the control is based on s64.
        return ModifyTooltipCommonUnsigned<AZ::s64>(widget, toolTipString);
    }

    void RegisterIntSliderHandlers()
    {
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew s16PropertySliderHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew u16PropertySliderHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew s32PropertySliderHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew u32PropertySliderHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew s64PropertySliderHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew u64PropertySliderHandler());
    }
}

#include <UI/PropertyEditor/PropertyIntSliderCtrl.moc>
