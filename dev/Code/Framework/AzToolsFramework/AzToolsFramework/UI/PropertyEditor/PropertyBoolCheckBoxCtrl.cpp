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
#include "PropertyBoolCheckBoxCtrl.hxx"
#include "PropertyQTConstants.h"
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>

//just a test to see how it would work to pop a dialog

namespace AzToolsFramework
{
    PropertyBoolCheckBoxCtrl::PropertyBoolCheckBoxCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        m_pCheckBox = new QCheckBox(this);

        pLayout->setContentsMargins(0, 0, 0, 0);

        pLayout->addWidget(m_pCheckBox);

        m_pCheckBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_pCheckBox->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_pCheckBox->setFixedHeight(PropertyQTConstant_DefaultHeight);

        m_pCheckBox->setFocusPolicy(Qt::StrongFocus);

        setLayout(pLayout);
        setFocusProxy(m_pCheckBox);
        setFocusPolicy(m_pCheckBox->focusPolicy());

        connect(m_pCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onStateChanged(int)));
    };

    void PropertyBoolCheckBoxCtrl::setValue(bool value)
    {
        m_pCheckBox->blockSignals(true);
        m_pCheckBox->setCheckState(value ? Qt::Checked : Qt::Unchecked);
        m_pCheckBox->blockSignals(false);
    }

    bool PropertyBoolCheckBoxCtrl::value() const
    {
        return m_pCheckBox->checkState() == Qt::Checked;
    }

    void PropertyBoolCheckBoxCtrl::onStateChanged(int newValue)
    {
        emit valueChanged(newValue == Qt::Unchecked ? false : true);
    }

    PropertyBoolCheckBoxCtrl::~PropertyBoolCheckBoxCtrl()
    {
    }

    QWidget* PropertyBoolCheckBoxCtrl::GetFirstInTabOrder()
    {
        return m_pCheckBox;
    }
    QWidget* PropertyBoolCheckBoxCtrl::GetLastInTabOrder()
    {
        return m_pCheckBox;
    }

    void PropertyBoolCheckBoxCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }

    template<class ValueType>
    void PropertyCheckBoxHandlerCommon<ValueType>::ConsumeAttribute(PropertyBoolCheckBoxCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        Q_UNUSED(GUI);
        Q_UNUSED(attrib);
        Q_UNUSED(attrValue);
        Q_UNUSED(debugName);
    }

    QWidget* BoolPropertyCheckBoxHandler::CreateGUI(QWidget* pParent)
    {
        PropertyBoolCheckBoxCtrl* newCtrl = aznew PropertyBoolCheckBoxCtrl(pParent);
        connect(newCtrl, &PropertyBoolCheckBoxCtrl::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });
        return newCtrl;
    }

    void BoolPropertyCheckBoxHandler::WriteGUIValuesIntoProperty(size_t index, PropertyBoolCheckBoxCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        bool val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    bool BoolPropertyCheckBoxHandler::ReadValuesIntoGUI(size_t index, PropertyBoolCheckBoxCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        bool val = instance;
        GUI->setValue(val);
        return false;
    }

    void RegisterBoolCheckBoxHandler()
    {
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew BoolPropertyCheckBoxHandler());
    }
}

#include <UI/PropertyEditor/PropertyBoolCheckBoxCtrl.moc>
