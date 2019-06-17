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

#ifndef PROPERTYEDITOR_PROPERTY_VECTOR3CTRL_H
#define PROPERTYEDITOR_PROPERTY_VECTOR3CTRL_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>

#include "PropertyEditorAPI.h"

#pragma once

class QLabel;
class QLayout;

namespace AzToolsFramework
{
    class DHQDoubleSpinbox;

    //////////////////////////////////////////////////////////////////////////

    /*!
     * \class VectorElement
     * \brief All flexible vector GUI's are constructed using a number vector elements. Each Vector
     * element contains a label and a double spin box which show the label and the value respectively.
     */
    class VectorElement
        : public QObject
    {
        Q_OBJECT

    public:

        AZ_CLASS_ALLOCATOR(VectorElement, AZ::SystemAllocator, 0);

        explicit VectorElement(QWidget* pParent = nullptr);
        ~VectorElement() override {}

        /**
        * Sets the label for this vector element
        * @param The new label
        */
        void SetLabel(const char* label);

        /**
        * Sets the value for the spinbox in this vector element
        * @param The new value
        */
        void SetValue(double newValue);

        QLabel* GetLabel() const
        {
            return m_label;
        }

        double GetValue() const
        {
            return m_value;
        }

        DHQDoubleSpinbox* GetSpinBox() const
        {
            return m_spinBox;
        }

        inline bool WasValueEditedByUser() const
        {
            return m_wasValueEditedByUser;
        }

    Q_SIGNALS:
        void valueChanged(double);
        void editingFinished();

    public Q_SLOTS:
        void onValueChanged(double newValue);


    protected:
        // Each vector element contains a label and a spin box

        QLabel* m_label;
        DHQDoubleSpinbox* m_spinBox;

        //! Value being shown by the spin box
        double m_value;

        //! Indicates whether the value in the spin box has been edited by the user or not
        bool m_wasValueEditedByUser;
    };

    //////////////////////////////////////////////////////////////////////////

    /*!
     * \class PropertyVectorCtrl
     * \brief Qt Widget that control holds an array of VectorElements.
     * This control can be used to display any number of labeled float values with configurable row(s) & column(s)
     */
    class PropertyVectorCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyVectorCtrl, AZ::SystemAllocator, 0);

        /**
        * Configures and constructs a vector control
        * @param parent The Parent Qwidget
        * @param elementCount Number of elements being managed by this vector control
        * @param elementsPerRow Number of elements in every row
        * @param customLabels A string that has custom labels that are use by the Vector elements
        */
        PropertyVectorCtrl(QWidget* parent, int elementCount, int elementsPerRow = -1, AZStd::string customLabels = "");
        ~PropertyVectorCtrl() override;

        /**
        * Sets the label on the indicated Vector element
        * @param index Index of the element for which the label is to be set
        * @param label New label
        */
        void setLabel(int index, const AZStd::string& label);

        /**
        * Sets the style on the indicated Vector element
        * @param index Index of the element for which the label is to be styled
        * @param qss The Qt Style to be applied
        */
        void setLabelStyle(int index, const AZStd::string& qss);

        /**
        * Fetches all elements being managed by this Vector control
        * @return An array of VectorElement*
        */
        VectorElement** getElements()
        {
            return m_elements;
        }

        /**
        * Fetches the count of elements being managed by this control
        * @return the number of elements being managed by this control
        */
        int getSize() const
        {
            return m_elementCount;
        }

        /**
        * Sets the value on the indicated Vector element
        * @param value the new value
        * @param index Index of the element for which the value is to be set
        */
        void setValuebyIndex(double value, int index);

        /**
        * Sets the minimum value that all spinboxes being managed by this control can have
        * @param Minimum value
        */
        void setMinimum(double value);

        /**
        * Sets the maximum value that all spinboxes being managed by this control can have
        * @param Maximum value
        */
        void setMaximum(double value);

        /**
        * Sets the step value that all spinboxes being managed by this control can have
        * @param Step value
        */
        void setStep(double value);

        /**
        * Sets the number of decimals to to lock the spinboxes being managed by this control to
        * @param Decimals value
        */
        void setDecimals(int value);

        /**
        * Sets the number of display decimals to truncate the spinboxes being managed by this control to
        * @param DisplayDecimals value
        */
        void setDisplayDecimals(int value);

        void OnValueChangedInElement(double newValue, int elementIndex);

        /**
        * Sets the suffix that is appended to values in spin boxes
        * @param Suffix to be used
        */
        void setSuffix(const AZStd::string label);

Q_SIGNALS:
        void valueChanged(double);
        void valueAtIndexChanged(int elementIndex, double newValue);
        void editingFinished();

    public Q_SLOTS:
        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

    private:

        //! Max size of the element vector
        int m_elementCount;

        //! Array that holds any number of Vector elements, each of which represents one float value and a label in the UI
        VectorElement** m_elements;
    };

    //////////////////////////////////////////////////////////////////////////

    /*!
     * \class VectorPropertyHandlerCommon
     * \brief Common functionality that is needed by handlers that need to handle a configurable
     * number of floats
     */
    class VectorPropertyHandlerCommon
    {
    public:

        /**
        * @param elementCount Number of elements being managed by this vector control
        * @param elementsPerRow Number of elements in every row
        * @param customLabels A string that has custom labels that are shown in front of elements
        */
        VectorPropertyHandlerCommon(int elementCount, int elementsPerRow = -1, AZStd::string customLabels = "")
            : m_elementCount(elementCount)
            , m_elementsPerRow(elementsPerRow)
            , m_customLabels(customLabels)
        {
        }

        // Constructs a Vector controller GUI attached to the provided parent
        PropertyVectorCtrl* ConstructGUI(QWidget* parent) const;

        /**
        * Consumes up to four attributes and configures the GUI
        * [IMPORTANT] Although the GUI and the handler can be configured to use more than four
        * elements, this method only supports attribute consumption up to four. If your handler/gui
        * uses more than four elements, this functionality will need to be augmented
        * @param GUI that will be configured according to the attributes
        * @param The attribute in consideration
        * @param attribute reader
        * @param unused
        */
        void ConsumeAttributes(PropertyVectorCtrl* GUI, AZ::u32 attrib,
            PropertyAttributeReader* attrValue, const char* debugName) const;

        int GetElementCount() const
        {
            return m_elementCount;
        }

        int GetElementsPerRow() const
        {
            return m_elementsPerRow;
        }

    private:
        //! stores the number of Vector elements being managed by this property handler
        int m_elementCount;

        //! stores the number of Vector elements per row
        int m_elementsPerRow;

        //! stores the custom labels to be used by controls
        AZStd::string m_customLabels;
    };

    template <typename TypeBeingHandled>
    class VectorPropertyHandlerBase
        : public PropertyHandler < TypeBeingHandled, PropertyVectorCtrl >
    {
    protected:
        VectorPropertyHandlerCommon m_common;
    public:
        /**
        * @param elementCount Number of elements being managed by this vector control
        * @param elementsPerRow Number of elements in every row
        * @param customLabels A string that has custom labels that are shown in front of elements
        */
        VectorPropertyHandlerBase(int elementCount, int elementsPerRow = -1, AZStd::string customLabels = "")
            : m_common(elementCount, elementsPerRow, customLabels)
        {
        }

        AZ::u32 GetHandlerName(void) const override
        {
            return AZ_CRC("Vector_Flex_Handler", 0x47a7620c);
        }

        bool IsDefaultHandler() const override
        {
            return true;
        }

        QWidget* GetFirstInTabOrder(PropertyVectorCtrl* widget) override
        {
            return widget->GetFirstInTabOrder();
        }

        QWidget* GetLastInTabOrder(PropertyVectorCtrl* widget) override
        {
            return widget->GetLastInTabOrder();
        }

        void UpdateWidgetInternalTabbing(PropertyVectorCtrl* widget) override
        {
            widget->UpdateTabOrder();
        }

        QWidget* CreateGUI(QWidget* pParent) override
        {
            return m_common.ConstructGUI(pParent);
        }

        void ConsumeAttribute(PropertyVectorCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override
        {
            m_common.ConsumeAttributes(GUI, attrib, attrValue, debugName);
        }

        void WriteGUIValuesIntoProperty(size_t, PropertyVectorCtrl* GUI, TypeBeingHandled& instance, InstanceDataNode*) override
        {
            VectorElement** elements = GUI->getElements();
            TypeBeingHandled actualValue = instance;
            for (int idx = 0; idx < m_common.GetElementCount(); ++idx)
            {
                if (elements[idx]->WasValueEditedByUser())
                {
                    actualValue.SetElement(idx, elements[idx]->GetValue());
                }
            }
            instance = actualValue;
        }

        bool ReadValuesIntoGUI(size_t, PropertyVectorCtrl* GUI, const TypeBeingHandled& instance, InstanceDataNode*) override
        {
            GUI->blockSignals(true);

            for (int idx = 0; idx < m_common.GetElementCount(); ++idx)
            {
                GUI->setValuebyIndex(instance.GetElement(idx), idx);
            }

            GUI->blockSignals(false);
            return false;
        }
    };

    class Vector2PropertyHandler
        : public VectorPropertyHandlerBase<AZ::Vector2>
    {
    public:

        Vector2PropertyHandler()
            : VectorPropertyHandlerBase(2)
        {
        };

        AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::Vector2; }
    };

    class Vector3PropertyHandler
        : public VectorPropertyHandlerBase<AZ::Vector3>
    {
    public:

        Vector3PropertyHandler()
            : VectorPropertyHandlerBase(3)
        {
        };

        AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::Vector3; }
    };

    class Vector4PropertyHandler
        : public VectorPropertyHandlerBase<AZ::Vector4>
    {
    public:

        Vector4PropertyHandler()
            : VectorPropertyHandlerBase(4)
        {
        };

        AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::Vector4; }
    };

    class QuaternionPropertyHandler
        : public VectorPropertyHandlerBase<AZ::Quaternion>
    {
    public:
        QuaternionPropertyHandler()
            : VectorPropertyHandlerBase(3)
        {
        };

        AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::Quaternion; }

        void WriteGUIValuesIntoProperty(size_t index, PropertyVectorCtrl* GUI, AZ::Quaternion& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyVectorCtrl* GUI, const AZ::Quaternion& instance, InstanceDataNode* node)  override;
    };
}


#endif // PROPERTYEDITOR_PROPERTY_VECTOR3CTRL_H
