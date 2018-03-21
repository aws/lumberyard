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
#include "PropertyVectorCtrl.hxx"
#include "DHQSpinbox.hxx"
#include "PropertyQTConstants.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Math/MathUtils.h> // for ConvertTransformToEulerDegrees
#include <cmath>

namespace AzToolsFramework
{
    //////////////////////////////////////////////////////////////////////////

    VectorElement::VectorElement(QWidget* pParent)
        : QObject(pParent)
        , m_wasValueEditedByUser(false)
    {
        m_spinBox = aznew DHQDoubleSpinbox(pParent);
        m_spinBox->setKeyboardTracking(false); // don't send valueChanged every time a character is typed (no need for undo/redo per digit of a double value)

        m_label = aznew QLabel(pParent);

        m_spinBox->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_spinBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        using OnValueChanged = void(QDoubleSpinBox::*)(double);
        auto valueChanged = static_cast<OnValueChanged>(&QDoubleSpinBox::valueChanged);
        connect(m_spinBox, valueChanged, this, &VectorElement::onValueChanged);
    }

    void VectorElement::SetLabel(const char* label)
    {
        m_label->setText(label);
        m_label->setObjectName(label);
    }

    void VectorElement::SetValue(double newValue)
    {
        m_value = newValue;
        m_spinBox->blockSignals(true);
        m_spinBox->setValue(newValue);
        m_spinBox->blockSignals(false);
        m_wasValueEditedByUser = false;
        Q_EMIT valueChanged(newValue);
    }

    void VectorElement::onValueChanged(double newValue)
    {
        if (!AZ::IsClose(m_value, newValue, std::numeric_limits<double>::epsilon()))
        {
            m_value = newValue;
            m_wasValueEditedByUser = true;
            Q_EMIT valueChanged(newValue);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    PropertyVectorCtrl::PropertyVectorCtrl(QWidget* parent, int elementCount, int elementsPerRow, AZStd::string customLabels)
        : QWidget(parent)
    {
        // Set up Qt layout
        QGridLayout* pLayout = new QGridLayout(this);
        pLayout->setMargin(0);
        pLayout->setSpacing(2);
        pLayout->setContentsMargins(1, 0, 1, 0);

        AZ_Warning("Property Editor", elementCount >= 2,
            "Vector controls with less than 2 elements are not supported");

        if (elementCount < 2)
        {
            elementCount = 2;
        }

        m_elementCount = elementCount;
        m_elements = new VectorElement*[m_elementCount];

        // Setting up custom labels
        AZStd::string labels = customLabels.empty() ? "XYZW" : customLabels;

        // Adding elements to the layout
        int numberOfElementsRemaining = m_elementCount;
        int numberOfRowsInLayout = elementsPerRow <= 0 ? 1 : std::ceil(static_cast<float>(m_elementCount) / elementsPerRow);
        int actualElementsPerRow = elementsPerRow <= 0 ? m_elementCount : elementsPerRow;

        for (int rowIdx = 0; rowIdx < numberOfRowsInLayout; rowIdx++)
        {
            QHBoxLayout* layout = new QHBoxLayout();
            for (int columnIdx = 0; columnIdx < actualElementsPerRow; columnIdx++)
            {
                if (numberOfElementsRemaining > 0)
                {
                    int elementIndex = rowIdx * elementsPerRow + columnIdx;

                    m_elements[elementIndex] = new VectorElement(this);
                    AZStd::string label(labels.substr(elementIndex, 1));
                    m_elements[elementIndex]->SetLabel(label.c_str());

                    layout->addWidget(m_elements[elementIndex]->GetLabel());
                    layout->addWidget(m_elements[elementIndex]->GetSpinBox());

                    using OnValueChanged = void(QDoubleSpinBox::*)(double);
                    auto valueChanged = static_cast<OnValueChanged>(&QDoubleSpinBox::valueChanged);
                    connect(m_elements[elementIndex]->GetSpinBox(), valueChanged, this, [elementIndex, this](double value)
                        {
                            OnValueChangedInElement(value, elementIndex);
                        });

                    numberOfElementsRemaining--;
                }
            }

            // Add internal layout to the external grid layout
            pLayout->addLayout(layout, rowIdx, 0);
        }
    }

    PropertyVectorCtrl::~PropertyVectorCtrl()
    {
        delete[] m_elements;
    }

    void PropertyVectorCtrl::setLabel(int index, const AZStd::string& label)
    {
        AZ_Warning("PropertyGrid", index < m_elementCount,
            "This control handles only %i controls", m_elementCount)
        if (index < m_elementCount)
        {
            m_elements[index]->SetLabel(label.c_str());
        }
    }

    void PropertyVectorCtrl::setLabelStyle(int index, const AZStd::string& qss)
    {
        AZ_Warning("PropertyGrid", index < m_elementCount,
            "This control handles only %i controls", m_elementCount)
        if (index < m_elementCount)
        {
            m_elements[index]->GetLabel()->setStyleSheet(qss.c_str());
        }
    }

    void PropertyVectorCtrl::setValuebyIndex(double value, int elementIndex)
    {
        AZ_Warning("PropertyGrid", elementIndex < m_elementCount,
            "This control handles only %i controls", m_elementCount)
        if (elementIndex < m_elementCount)
        {
            m_elements[elementIndex]->SetValue(value);
        }
    }

    void PropertyVectorCtrl::setMinimum(double value)
    {
        for (size_t i = 0; i < m_elementCount; ++i)
        {
            m_elements[i]->GetSpinBox()->blockSignals(true);
            m_elements[i]->GetSpinBox()->setMinimum(value);
            m_elements[i]->GetSpinBox()->blockSignals(false);
        }
    }

    void PropertyVectorCtrl::setMaximum(double value)
    {
        for (size_t i = 0; i < m_elementCount; ++i)
        {
            m_elements[i]->GetSpinBox()->blockSignals(true);
            m_elements[i]->GetSpinBox()->setMaximum(value);
            m_elements[i]->GetSpinBox()->blockSignals(false);
        }
    }

    void PropertyVectorCtrl::setStep(double value)
    {
        for (size_t i = 0; i < m_elementCount; ++i)
        {
            m_elements[i]->GetSpinBox()->blockSignals(true);
            m_elements[i]->GetSpinBox()->setSingleStep(value);
            m_elements[i]->GetSpinBox()->blockSignals(false);
        }
    }

    void PropertyVectorCtrl::setDecimals(int value)
    {
        for (size_t i = 0; i < m_elementCount; ++i)
        {
            m_elements[i]->GetSpinBox()->blockSignals(true);
            m_elements[i]->GetSpinBox()->setDecimals(value);
            m_elements[i]->GetSpinBox()->blockSignals(false);
        }
    }

    void PropertyVectorCtrl::setDisplayDecimals(int value)
    {
        for (size_t i = 0; i < m_elementCount; ++i)
        {
            m_elements[i]->GetSpinBox()->blockSignals(true);
            m_elements[i]->GetSpinBox()->SetDisplayDecimals(value);
            m_elements[i]->GetSpinBox()->blockSignals(false);
        }
    }

    void PropertyVectorCtrl::OnValueChangedInElement(double newValue, int elementIndex)
    {
        Q_EMIT valueChanged(newValue);
        Q_EMIT valueAtIndexChanged(elementIndex, newValue);
    }

    void PropertyVectorCtrl::setSuffix(const AZStd::string label)
    {
        for (size_t i = 0; i < m_elementCount; ++i)
        {
            m_elements[i]->GetSpinBox()->blockSignals(true);
            m_elements[i]->GetSpinBox()->setSuffix(label.c_str());
            m_elements[i]->GetSpinBox()->blockSignals(false);
        }
    }

    QWidget* PropertyVectorCtrl::GetFirstInTabOrder()
    {
        return m_elements[0]->GetSpinBox();
    }

    QWidget* PropertyVectorCtrl::GetLastInTabOrder()
    {
        return m_elements[m_elementCount - 1]->GetSpinBox();
    }

    void PropertyVectorCtrl::UpdateTabOrder()
    {
        for (int i = 0; i < m_elementCount - 1; i++)
        {
            setTabOrder(m_elements[i]->GetSpinBox(), m_elements[i + 1]->GetSpinBox());
        }
    }

    //////////////////////////////////////////////////////////////////////////

    PropertyVectorCtrl* VectorPropertyHandlerCommon::ConstructGUI(QWidget* parent) const
    {
        PropertyVectorCtrl* newCtrl = aznew PropertyVectorCtrl(parent, m_elementCount, m_elementsPerRow, m_customLabels);
        QObject::connect(newCtrl, &PropertyVectorCtrl::valueChanged, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });

        newCtrl->setMinimum(-std::numeric_limits<float>::max());
        newCtrl->setMaximum(std::numeric_limits<float>::max());

        return newCtrl;
    }

    void VectorPropertyHandlerCommon::ConsumeAttributes(PropertyVectorCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) const
    {
        if (attrib == AZ::Edit::Attributes::Suffix)
        {
            AZStd::string label;
            if (attrValue->Read<AZStd::string>(label))
            {
                GUI->setSuffix(label);
            }
        }
        else if (attrib == AZ::Edit::Attributes::LabelForX)
        {
            AZStd::string label;
            if (attrValue->Read<AZStd::string>(label))
            {
                GUI->setLabel(0, label);
            }
        }
        else if (attrib == AZ::Edit::Attributes::LabelForY)
        {
            AZStd::string label;
            if (attrValue->Read<AZStd::string>(label))
            {
                GUI->setLabel(1, label);
            }
        }
        else if (attrib == AZ::Edit::Attributes::StyleForX)
        {
            AZStd::string qss;
            if (attrValue->Read<AZStd::string>(qss))
            {
                GUI->setLabelStyle(0, qss);
            }
        }
        else if (attrib == AZ::Edit::Attributes::StyleForY)
        {
            AZStd::string qss;
            if (attrValue->Read<AZStd::string>(qss))
            {
                GUI->setLabelStyle(1, qss);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Min)
        {
            double value;
            if (attrValue->Read<double>(value))
            {
                GUI->setMinimum(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Max)
        {
            double value;
            if (attrValue->Read<double>(value))
            {
                GUI->setMaximum(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Step)
        {
            double value;
            if (attrValue->Read<double>(value))
            {
                GUI->setStep(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Decimals)
        {
            int intValue = 0;
            if (attrValue->Read<int>(intValue))
            {
                GUI->setDecimals(intValue);
            }
            else
            {
                // debugName is unused in release...
                Q_UNUSED(debugName);

                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Decimals' attribute from property '%s' into Vector", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::DisplayDecimals)
        {
            int intValue = 0;
            if (attrValue->Read<int>(intValue))
            {
                GUI->setDisplayDecimals(intValue);
            }
            else
            {
                // debugName is unused in release...
                Q_UNUSED(debugName);

                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'DisplayDecimals' attribute from property '%s' into Vector", debugName);
            }
            return;
        }

        if (m_elementCount > 2)
        {
            if (attrib == AZ::Edit::Attributes::LabelForZ)
            {
                AZStd::string label;
                if (attrValue->Read<AZStd::string>(label))
                {
                    GUI->setLabel(2, label);
                }
            }
            else if (attrib == AZ::Edit::Attributes::StyleForZ)
            {
                AZStd::string qss;
                if (attrValue->Read<AZStd::string>(qss))
                {
                    GUI->setLabelStyle(2, qss);
                }
            }

            if (m_elementCount > 3)
            {
                if (attrib == AZ::Edit::Attributes::LabelForW)
                {
                    AZStd::string label;
                    if (attrValue->Read<AZStd::string>(label))
                    {
                        GUI->setLabel(3, label);
                    }
                }
                else if (attrib == AZ::Edit::Attributes::StyleForW)
                {
                    AZStd::string qss;
                    if (attrValue->Read<AZStd::string>(qss))
                    {
                        GUI->setLabelStyle(3, qss);
                    }
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void QuaternionPropertyHandler::WriteGUIValuesIntoProperty(size_t, PropertyVectorCtrl* GUI, AZ::Quaternion& instance, InstanceDataNode*)
    {
        VectorElement** elements = GUI->getElements();

        AZ::Vector3 eulerRotation(elements[0]->GetValue(), elements[1]->GetValue(), elements[2]->GetValue());
        AZ::Transform eulerRepresentation = AzFramework::ConvertEulerDegreesToTransform(eulerRotation);
        AZ::Quaternion newValue = AZ::Quaternion::CreateFromTransform(eulerRepresentation);

        instance = static_cast<AZ::Quaternion>(newValue);
    }

    bool QuaternionPropertyHandler::ReadValuesIntoGUI(size_t, PropertyVectorCtrl* GUI, const AZ::Quaternion& instance, InstanceDataNode*)
    {
        GUI->blockSignals(true);

        AZ::Vector3 eulerRotation = AzFramework::ConvertTransformToEulerDegrees(AZ::Transform::CreateFromQuaternion(instance));

        for (int idx = 0; idx < m_common.GetElementCount(); ++idx)
        {
            GUI->setValuebyIndex(eulerRotation.GetElement(idx), idx);
        }

        GUI->blockSignals(false);
        return false;
    }

    //////////////////////////////////////////////////////////////////////////

    void RegisterVectorHandlers()
    {
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew Vector2PropertyHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew Vector3PropertyHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew Vector4PropertyHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew QuaternionPropertyHandler());
    }
}
#include <UI/PropertyEditor/PropertyVectorCtrl.moc>
