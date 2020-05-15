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

#ifndef PROPERTY_BOOLCHECKBOX_CTRL
#define PROPERTY_BOOLCHECKBOX_CTRL

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>
#include "PropertyEditorAPI.h"

#pragma once

class QCheckBox;
class QLineEdit;
class QPushButton;

namespace AzToolsFramework
{
    //just a test to see how it would work to pop a dialog

    class PropertyBoolCheckBoxCtrl : public QWidget
    {
        template<typename T> friend class PropertyCheckBoxHandlerCommon;
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyBoolCheckBoxCtrl, AZ::SystemAllocator, 0);

        PropertyBoolCheckBoxCtrl(QWidget* pParent = NULL);
        virtual ~PropertyBoolCheckBoxCtrl();

        bool value() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

        void SetCheckBoxToolTip(const char* text);

    signals:
        void valueChanged(bool newValue);

    public slots:
        void setValue(bool val);

    protected slots:
        void onStateChanged(int value);

    private:
        QCheckBox* m_pCheckBox;
    };

    template <class ValueType> class PropertyCheckBoxHandlerCommon : public PropertyHandler<ValueType, PropertyBoolCheckBoxCtrl>
    {
        virtual AZ::u32 GetHandlerName(void) const override { return AZ_CRC("CheckBox", 0x1e7b08ed); }
        virtual bool IsDefaultHandler() const override { return true; }
        virtual QWidget* GetFirstInTabOrder(PropertyBoolCheckBoxCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        virtual QWidget* GetLastInTabOrder(PropertyBoolCheckBoxCtrl* widget) override { return widget->GetLastInTabOrder(); }
        virtual void UpdateWidgetInternalTabbing(PropertyBoolCheckBoxCtrl* widget) override { widget->UpdateTabOrder(); }

        virtual void ConsumeAttribute(PropertyBoolCheckBoxCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
    };

    class BoolPropertyCheckBoxHandler : QObject, public PropertyCheckBoxHandlerCommon<bool>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(BoolPropertyCheckBoxHandler, AZ::SystemAllocator, 0);

        virtual QWidget* CreateGUI(QWidget *pParent) override;
 
        virtual void WriteGUIValuesIntoProperty(size_t index, PropertyBoolCheckBoxCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        virtual bool ReadValuesIntoGUI(size_t index, PropertyBoolCheckBoxCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    void RegisterBoolCheckBoxHandler();
};

#endif
