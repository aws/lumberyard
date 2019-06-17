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

#ifndef PROPERTY_INTSPINBOX_CTRL
#define PROPERTY_INTSPINBOX_CTRL

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>

#include "PropertyEditorAPI.h"

#pragma once

class QSpinBox;
class QPushButton;

namespace AzToolsFramework
{
    class PropertyIntSpinCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyIntSpinCtrl, AZ::SystemAllocator, 0);

        PropertyIntSpinCtrl(QWidget* pParent = NULL);
        virtual ~PropertyIntSpinCtrl();

        AZ::s64 value() const;
        AZ::s64 minimum() const;
        AZ::s64 maximum() const;
        AZ::s64 step() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

    signals:
        void valueChanged(AZ::s64 newValue);
        void editingFinished();

    public slots:
        void setValue(AZ::s64 val);
        void setMinimum(AZ::s64 val);
        void setMaximum(AZ::s64 val);
        void setStep(AZ::s64 val);
        void setMultiplier(AZ::s64 val);
        void setPrefix(QString val);
        void setSuffix(QString val);

    protected slots:
        void onChildSpinboxValueChange(int value);

    private:
        QSpinBox* m_pSpinBox;
        AZ::s64 m_multiplier;

    protected:
        virtual void focusInEvent(QFocusEvent* e);
    };

    // note:  QT Objects cannot themselves be templates, else I would templatize creategui as well.
    template <class ValueType>
    class IntSpinBoxHandlerCommon
        : public PropertyHandler<ValueType, PropertyIntSpinCtrl>
    {
        AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::SpinBox; }
        bool IsDefaultHandler() const override { return true; }
        QWidget* GetFirstInTabOrder(PropertyIntSpinCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        QWidget* GetLastInTabOrder(PropertyIntSpinCtrl* widget) override { return widget->GetLastInTabOrder(); }
        void UpdateWidgetInternalTabbing(PropertyIntSpinCtrl* widget) override { widget->UpdateTabOrder(); }
    };


    class s32PropertySpinboxHandler
        : QObject
        , public IntSpinBoxHandlerCommon<AZ::s32>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(s32PropertySpinboxHandler, AZ::SystemAllocator, 0);

        // common to all int spinners
        static void ConsumeAttributeCommon(PropertyIntSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName);
        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyIntSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyIntSpinCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyIntSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    class u32PropertySpinboxHandler
        : QObject
        , public IntSpinBoxHandlerCommon<AZ::u32>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(u32PropertySpinboxHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyIntSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyIntSpinCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyIntSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    class s64PropertySpinboxHandler
        : QObject
        , public IntSpinBoxHandlerCommon<AZ::s64>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(u32PropertySpinboxHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyIntSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyIntSpinCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyIntSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    class u64PropertySpinboxHandler
        : QObject
        , public IntSpinBoxHandlerCommon<AZ::u64>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(u32PropertySpinboxHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyIntSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyIntSpinCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyIntSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    void RegisterIntSpinBoxHandlers();
};

#endif
