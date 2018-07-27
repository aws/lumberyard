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
#include "PropertyIntSpinCtrl.hxx"
#include "DHQSpinbox.hxx"
#include "PropertyQTConstants.h"
#include <QtWidgets/QSlider>
#include <QtWidgets/QHBoxLayout>
#include <QFocusEvent>

namespace AzToolsFramework
{
    PropertyIntSpinCtrl::PropertyIntSpinCtrl(QWidget* pParent)
        : QWidget(pParent)
        , m_multiplier(1)

    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        setFocusPolicy(Qt::StrongFocus);
        QHBoxLayout* pLayout = aznew QHBoxLayout(this);
        m_pSpinBox = aznew DHQSpinbox(this);

        m_pSpinBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_pSpinBox->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_pSpinBox->setFixedHeight(PropertyQTConstant_DefaultHeight);

        pLayout->setSpacing(4);
        pLayout->setContentsMargins(1, 0, 1, 0);

        pLayout->addWidget(m_pSpinBox);

        setLayout(pLayout);

        m_pSpinBox->setKeyboardTracking(false);
        m_pSpinBox->setFocusPolicy(Qt::StrongFocus);

        connect(m_pSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onChildSpinboxValueChange(int)));
    };

    QWidget* PropertyIntSpinCtrl::GetFirstInTabOrder()
    {
        return m_pSpinBox;
    }
    QWidget* PropertyIntSpinCtrl::GetLastInTabOrder()
    {
        return m_pSpinBox;
    }

    void PropertyIntSpinCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }


    void PropertyIntSpinCtrl::setValue(AZ::s64 value)
    {
        m_pSpinBox->blockSignals(true);
        m_pSpinBox->setValue((int)(value / m_multiplier));
        m_pSpinBox->blockSignals(false);
    }

    void PropertyIntSpinCtrl::focusInEvent(QFocusEvent* e)
    {
        ((QAbstractSpinBox*)m_pSpinBox)->event(e);
        ((QAbstractSpinBox*)m_pSpinBox)->selectAll();
    }

    void PropertyIntSpinCtrl::setMinimum(AZ::s64 value)
    {
        m_pSpinBox->blockSignals(true);
        m_pSpinBox->setMinimum((int)(value / m_multiplier));
        m_pSpinBox->blockSignals(false);
    }

    void PropertyIntSpinCtrl::setMaximum(AZ::s64 value)
    {
        m_pSpinBox->blockSignals(true);
        m_pSpinBox->setMaximum((int)(value / m_multiplier));
        m_pSpinBox->blockSignals(false);
    }

    void PropertyIntSpinCtrl::setStep(AZ::s64 value)
    {
        m_pSpinBox->blockSignals(true);

        if ((value / m_multiplier) < 1)
        {
            value = m_multiplier;
        }


        m_pSpinBox->setSingleStep((int)(value / m_multiplier));
        m_pSpinBox->blockSignals(false);
    }

    void PropertyIntSpinCtrl::setMultiplier(AZ::s64 val)
    {
        m_pSpinBox->blockSignals(true);

        AZ::s64 singleStep = step();
        AZ::s64 currValue = value();
        AZ::s64 minVal = minimum();
        AZ::s64 maxVal = maximum();

        m_multiplier = val;

        if ((singleStep / m_multiplier) < 1)
        {
            singleStep = m_multiplier;
        }

        m_pSpinBox->setMinimum((int)(minVal / m_multiplier));
        m_pSpinBox->setMaximum((int)(maxVal / m_multiplier));

        m_pSpinBox->setSingleStep((int)(singleStep / m_multiplier));
        m_pSpinBox->setValue((int)(currValue / m_multiplier));


        m_pSpinBox->blockSignals(false);
    }

    void PropertyIntSpinCtrl::setPrefix(QString val)
    {
        m_pSpinBox->setPrefix(val);
    }

    void PropertyIntSpinCtrl::setSuffix(QString val)
    {
        m_pSpinBox->setSuffix(val);
    }

    AZ::s64 PropertyIntSpinCtrl::value() const
    {
        return (AZ::s64)m_pSpinBox->value() * m_multiplier;
    }

    AZ::s64 PropertyIntSpinCtrl::minimum() const
    {
        return (AZ::s64)m_pSpinBox->minimum() * m_multiplier;
    }

    AZ::s64 PropertyIntSpinCtrl::maximum() const
    {
        return (AZ::s64)m_pSpinBox->maximum() * m_multiplier;
    }

    AZ::s64 PropertyIntSpinCtrl::step() const
    {
        return (AZ::s64)m_pSpinBox->singleStep() * m_multiplier;
    }

    void PropertyIntSpinCtrl::onChildSpinboxValueChange(int newValue)
    {
        //  this->setValue(newValue); // addition validation?
        emit valueChanged((AZ::s64)(newValue * m_multiplier));
    }

    PropertyIntSpinCtrl::~PropertyIntSpinCtrl()
    {
    }

    // a common function to eat attribs, for all int handlers:
    void s32PropertySpinboxHandler::ConsumeAttributeCommon(PropertyIntSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
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
            AZStd::string stringValue;
            if (attrValue->Read<AZStd::string>(stringValue))
            {
                GUI->setSuffix(stringValue.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Suffix' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ_CRC("Prefix", 0x93b1868e))
        {
            AZStd::string stringValue;
            if (attrValue->Read<AZStd::string>(stringValue))
            {
                GUI->setPrefix(stringValue.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Prefix' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
    }

    QWidget* s32PropertySpinboxHandler::CreateGUI(QWidget* pParent)
    {
        PropertyIntSpinCtrl* newCtrl = aznew PropertyIntSpinCtrl(pParent);
        connect(newCtrl, &PropertyIntSpinCtrl::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setMinimum(INT_MIN);
        newCtrl->setMaximum(INT_MAX);

        return newCtrl;
    }


    QWidget* u32PropertySpinboxHandler::CreateGUI(QWidget* pParent)
    {
        PropertyIntSpinCtrl* newCtrl = aznew PropertyIntSpinCtrl(pParent);
        connect(newCtrl, &PropertyIntSpinCtrl::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        newCtrl->setMinimum(0);
        newCtrl->setMaximum(INT_MAX);

        return newCtrl;
    }


    QWidget* u64PropertySpinboxHandler::CreateGUI(QWidget* pParent)
    {
        PropertyIntSpinCtrl* newCtrl = aznew PropertyIntSpinCtrl(pParent);
        connect(newCtrl, &PropertyIntSpinCtrl::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        newCtrl->setMinimum(0);
        newCtrl->setMaximum(INT_MAX);

        return newCtrl;
    }

    QWidget* s64PropertySpinboxHandler::CreateGUI(QWidget* pParent)
    {
        PropertyIntSpinCtrl* newCtrl = aznew PropertyIntSpinCtrl(pParent);
        connect(newCtrl, &PropertyIntSpinCtrl::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        newCtrl->setMinimum(INT_MIN);
        newCtrl->setMaximum(INT_MAX);

        return newCtrl;
    }

    void s32PropertySpinboxHandler::ConsumeAttribute(PropertyIntSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        s32PropertySpinboxHandler::ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);

        if ((GUI->maximum() > INT_MAX) || (GUI->maximum() < INT_MIN) || (GUI->minimum() > INT_MAX) || (GUI->minimum() < INT_MIN))
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in s32 Spin Box exceeds Min/Max values", debugName, attrib);
        }
    }

    void u32PropertySpinboxHandler::ConsumeAttribute(PropertyIntSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        s32PropertySpinboxHandler::ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);

        if ((GUI->maximum() > UINT_MAX) || (GUI->maximum() < 0) || (GUI->minimum() > UINT_MAX) || (GUI->minimum() < 0))
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in u32 Spin Box exceeds Min/Max values [%lli, %lli]\n", debugName, attrib, GUI->minimum(), GUI->maximum());
        }
    }

    void s64PropertySpinboxHandler::ConsumeAttribute(PropertyIntSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        s32PropertySpinboxHandler::ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);
    }

    void u64PropertySpinboxHandler::ConsumeAttribute(PropertyIntSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        s32PropertySpinboxHandler::ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);
        if ((GUI->maximum() < 0) || (GUI->minimum() < 0))
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in u64 has a negative min or max attribute", debugName, attrib);
        }
    }


    void s32PropertySpinboxHandler::WriteGUIValuesIntoProperty(size_t index, PropertyIntSpinCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        AZ::s64 val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    void u32PropertySpinboxHandler::WriteGUIValuesIntoProperty(size_t index, PropertyIntSpinCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        AZ::s64 val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    void s64PropertySpinboxHandler::WriteGUIValuesIntoProperty(size_t index, PropertyIntSpinCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        AZ::s64 val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    void u64PropertySpinboxHandler::WriteGUIValuesIntoProperty(size_t index, PropertyIntSpinCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        AZ::s64 val = GUI->value();
        instance = static_cast<property_t>(val);
    }


    bool s32PropertySpinboxHandler::ReadValuesIntoGUI(size_t index, PropertyIntSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        GUI->setValue(instance);
        return false;
    }


    bool u32PropertySpinboxHandler::ReadValuesIntoGUI(size_t index, PropertyIntSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        GUI->setValue(instance);
        return false;
    }

    bool s64PropertySpinboxHandler::ReadValuesIntoGUI(size_t index, PropertyIntSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        GUI->setValue(instance);
        return false;
    }


    bool u64PropertySpinboxHandler::ReadValuesIntoGUI(size_t index, PropertyIntSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        GUI->setValue(instance);
        return false;
    }

    bool s32PropertySpinboxHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        PropertyIntSpinCtrl* propertyControl = qobject_cast<PropertyIntSpinCtrl*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[";
            if (propertyControl->minimum() <= std::numeric_limits<AZ::s32>::min())
            {
                toolTipString += "-" + tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->minimum());
            }
            toolTipString += ", ";
            if (propertyControl->maximum() >= std::numeric_limits<AZ::s32>::max())
            {
                toolTipString += tr(PropertyQTConstant_InfinityString);
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

    bool u32PropertySpinboxHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        PropertyIntSpinCtrl* propertyControl = qobject_cast<PropertyIntSpinCtrl*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[" + QString::number(propertyControl->minimum()) + ", ";
            if (propertyControl->maximum() >= std::numeric_limits<AZ::u32>::max())
            {
                toolTipString += tr(PropertyQTConstant_InfinityString);
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

    bool s64PropertySpinboxHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        PropertyIntSpinCtrl* propertyControl = qobject_cast<PropertyIntSpinCtrl*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[";
            if (propertyControl->minimum() <= std::numeric_limits<AZ::s64>::min())
            {
                toolTipString += "-" + tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->minimum());
            }
            toolTipString += ", ";
            if (propertyControl->maximum() >= std::numeric_limits<AZ::s64>::max())
            {
                toolTipString += tr(PropertyQTConstant_InfinityString);
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

    bool u64PropertySpinboxHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        PropertyIntSpinCtrl* propertyControl = qobject_cast<PropertyIntSpinCtrl*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[" + QString::number(propertyControl->minimum()) + ", ";
            if (propertyControl->maximum() >= std::numeric_limits<AZ::s64>::max())  //  Although it is a u64 handler, the control is based on s64.
            {
                toolTipString += tr(PropertyQTConstant_InfinityString);
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

    void RegisterIntSpinBoxHandlers()
    {
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew u32PropertySpinboxHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew s32PropertySpinboxHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew u64PropertySpinboxHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew s64PropertySpinboxHandler());
    }
}

#include <UI/PropertyEditor/PropertyIntSpinCtrl.moc>