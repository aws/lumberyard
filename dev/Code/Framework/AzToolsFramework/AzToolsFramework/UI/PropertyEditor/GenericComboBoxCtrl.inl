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

#pragma once

#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>
#include <AzToolsFramework/UI/PropertyEditor/DHQComboBox.hxx>
#include <QtWidgets/QHBoxLayout>
#include <QPushButton>

namespace
{
    template<typename T>
    struct Helper
    {
        static void ParseStringList(AzToolsFramework::GenericComboBoxCtrlBase* GUI, AzToolsFramework::PropertyAttributeReader* attrReader, AZStd::vector< AZStd::pair<T, AZStd::string> >& valueList)
        {
            AZ_UNUSED(GUI);
            AZ_UNUSED(attrReader);
            AZ_UNUSED(valueList);

            // Do nothing by default
        }
    };

    template<>
    struct Helper<AZStd::string>
    {
        static void ParseStringList(AzToolsFramework::GenericComboBoxCtrlBase* GUI, AzToolsFramework::PropertyAttributeReader* attrReader, AZStd::vector< AZStd::pair<AZStd::string, AZStd::string> >& valueList)
        {
            AZ_WarningOnce("AzToolsFramework", false, "Using Deprecated Attribute StringList. Please switch to GenericValueList");

            auto genericGUI = azrtti_cast<AzToolsFramework::GenericComboBoxCtrl<AZStd::string>*>(GUI);

            if (genericGUI)
            {
                AZStd::vector<AZStd::string> values;
                if (attrReader->Read<AZStd::vector<AZStd::string>>(values))
                {
                    valueList.reserve(values.size());
                    for (const AZStd::string& currentValue : values)
                    {
                        valueList.emplace_back(AZStd::make_pair(currentValue, currentValue));
                    }
                }
            }
        }
    };
}

namespace AzToolsFramework
{
    template<typename T>
    GenericComboBoxCtrl<T>::GenericComboBoxCtrl(QWidget* pParent)
        : GenericComboBoxCtrlBase(pParent)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        auto* pLayout = new QHBoxLayout(this);
        m_pComboBox = aznew AzToolsFramework::DHQComboBox(this);

        pLayout->setSpacing(4);
        pLayout->setContentsMargins(1, 0, 1, 0);

        pLayout->addWidget(m_pComboBox);

        m_pComboBox->setMinimumWidth(AzToolsFramework::PropertyQTConstant_MinimumWidth);
        m_pComboBox->setFixedHeight(AzToolsFramework::PropertyQTConstant_DefaultHeight);

        m_pComboBox->setFocusPolicy(Qt::StrongFocus);

        setLayout(pLayout);
        setFocusProxy(m_pComboBox);
        setFocusPolicy(m_pComboBox->focusPolicy());

        connect(m_pComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &GenericComboBoxCtrl<T>::onChildComboBoxValueChange);
    }

    template<typename T>
    const T& GenericComboBoxCtrl<T>::value() const
    {
        AZ_Error("AzToolsFramework", m_pComboBox->currentIndex() >= 0 &&
            m_pComboBox->currentIndex() < static_cast<int>(m_values.size()), "Out of range combo box index %d", m_pComboBox->currentIndex());
        return m_values[m_pComboBox->currentIndex()].first;
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::setValue(const T& value)
    {
        QSignalBlocker signalBlocker(m_pComboBox);
        bool indexWasFound = m_values.empty(); // Combo box might just be empty don't warn in that case
        for (size_t genericValIndex = 0; genericValIndex < m_values.size(); ++genericValIndex)
        {
            if (m_values[genericValIndex].first == value)
            {
                m_pComboBox->setCurrentIndex(static_cast<int>(genericValIndex));
                indexWasFound = true;
                break;
            }
        }
        if (!indexWasFound)
        {
            m_pComboBox->setCurrentIndex(-1);
        }

        AZ_Warning("AzToolsFramework", indexWasFound == true, "GenericValue could not be found in the combo box");

    }

    template<typename T>
    void GenericComboBoxCtrl<T>::setElements(const AZStd::vector<AZStd::pair<T, AZStd::string>>& genericValues)
    {
        m_pComboBox->clear();
        m_values.clear();
        addElements(genericValues);
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::addElements(const AZStd::vector<AZStd::pair<T, AZStd::string>>& genericValues)
    {
        QSignalBlocker signalBlocker(m_pComboBox);
        for (auto&& genericValue : genericValues)
        {
            addElementImpl(genericValue);            
        }
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::addElement(const AZStd::pair<T, AZStd::string>& genericValue)
    {
        QSignalBlocker signalBlocker(m_pComboBox);
        addElementImpl(genericValue);
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::addElementImpl(const AZStd::pair<T, AZStd::string>& genericValue)
    {
        if (AZStd::find(m_values.begin(), m_values.end(), genericValue) == m_values.end())
        {
            m_values.push_back(genericValue);
            m_pComboBox->addItem(genericValue.second.data());
        }
    }

    template<typename T>
    inline void GenericComboBoxCtrl<T>::onChildComboBoxValueChange(int comboBoxIndex)
    {
        if (comboBoxIndex < 0)
        {
            return;
        }

        AZ_Error("AzToolsFramework", comboBoxIndex >= 0 &&
            comboBoxIndex < static_cast<int>(m_values.size()), "Out of range combo box index %d", comboBoxIndex);

        emit valueChanged(m_values[comboBoxIndex].second);
    }

    template<typename T>
    inline QWidget* GenericComboBoxCtrl<T>::GetFirstInTabOrder()
    {
        return m_pComboBox;
    }

    template<typename T>
    inline QWidget* GenericComboBoxCtrl<T>::GetLastInTabOrder()
    {
        return GetFirstInTabOrder();
    }

    template<typename T>
    inline void GenericComboBoxCtrl<T>::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }

    // Property handler

    template<typename T>
    QWidget* GenericComboBoxHandler<T>::CreateGUI(QWidget* pParent)
    {
        auto newCtrl = aznew GenericComboBoxCtrl<T>(pParent);
        connect(newCtrl, &GenericComboBoxCtrl<T>::valueChanged, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });
        return newCtrl;
    }

    template<typename T>
    void GenericComboBoxHandler<T>::ConsumeAttribute(ComboBoxCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrReader, const char* debugName)
    {
        AZ_UNUSED(debugName);

        auto genericGUI = azrtti_cast<GenericComboBoxCtrl<T>*>(GUI);        
        using ValueType = AZStd::pair<typename GenericComboBoxHandler::property_t, AZStd::string>;
        using ValueTypeContainer = AZStd::vector<ValueType>;

        if (attrib == AZ::Edit::Attributes::StringList)
        {
            ValueTypeContainer valueContainer;
            Helper<T>::ParseStringList(GUI, attrReader, valueContainer);
            genericGUI->setElements(valueContainer);
        }
        else if (attrib == AZ::Edit::Attributes::GenericValue)
        {
            ValueType value;
            if (attrReader->Read<ValueType>(value))
            {
                genericGUI->addElement(value);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'GenericValue' attribute from property '%s' into generic combo box. Expected a pair<%s, string>.", debugName, AZ::AzTypeInfo<typename GenericComboBoxHandler::property_t>::Name());
            }
        }
        else if (attrib == AZ::Edit::Attributes::GenericValueList)
        {
            ValueTypeContainer values;
            if (attrReader->Read<ValueTypeContainer>(values))
            {
                genericGUI->setElements(values);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'GenericValueList' attribute from property '%s' into generic combo box. Expected a vector of pair<%s, string>.", debugName, AZ::AzTypeInfo<typename GenericComboBoxHandler::property_t>::Name());
            }
        }
        else if (attrib == AZ::Edit::Attributes::PostChangeNotify)
        {
            if (auto notifyCallback = azrtti_cast<AZ::AttributeFunction<void(const T&)>*>(attrReader->GetAttribute()))
            {
                genericGUI->m_postChangeNotifyCB = notifyCallback;
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'PostChangeNotify' attribute from property '%s' into generic combo box.", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::ComboBoxEditable)
        {
            bool value;
            if (attrReader->Read<bool>(value))
            {
                genericGUI->m_pComboBox->setEditable(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'ComboBoxEditable' attribute from property '%s' into generic combo box", debugName);
            }
        }
    }

    template<typename T>
    void GenericComboBoxHandler<T>::WriteGUIValuesIntoProperty(size_t index, ComboBoxCtrl* GUI, typename GenericComboBoxHandler::property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        auto genericGUI = azrtti_cast<GenericComboBoxCtrl<T>*>(GUI);
        const auto oldValue = instance;
        instance = genericGUI->value();
        InvokePostChangeNotify(index, genericGUI, oldValue, node);
    }

    template<typename T>
    bool GenericComboBoxHandler<T>::ReadValuesIntoGUI(size_t, ComboBoxCtrl* GUI, const typename GenericComboBoxHandler::property_t& instance, AzToolsFramework::InstanceDataNode*)
    {
        auto genericGUI = azrtti_cast<GenericComboBoxCtrl<T>*>(GUI);
        genericGUI->setValue(instance);
        return true;
    }

    template<typename T>
    void GenericComboBoxHandler<T>::InvokePostChangeNotify(size_t index, GenericComboBoxCtrl<T>* genericGUI, const typename GenericComboBoxHandler::property_t& oldValue, AzToolsFramework::InstanceDataNode* node) const
    {
        if (genericGUI->m_postChangeNotifyCB)
        {
            void* notifyInstance = nullptr;
            const AZ::Uuid handlerTypeId = genericGUI->m_postChangeNotifyCB->GetInstanceType();
            if (!handlerTypeId.IsNull())
            {
                do
                {
                    if (handlerTypeId == node->GetClassMetadata()->m_typeId || (node->GetClassMetadata()->m_azRtti && node->GetClassMetadata()->m_azRtti->IsTypeOf(handlerTypeId)))
                    {
                        notifyInstance = node->GetInstance(index);
                        break;
                    }
                } while (node = node->GetParent());
            }

            genericGUI->m_postChangeNotifyCB->Invoke(notifyInstance, oldValue);
        }
    }
}
