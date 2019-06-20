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

#ifndef PROPERTY_INTSLIDER_CTRL
#define PROPERTY_INTSLIDER_CTRL

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include "PropertyEditorAPI.h"
#include <QtWidgets/QWidget>

#pragma once

class QSlider;
class QSpinBox;
class QToolButton;

namespace AzToolsFramework
{
    class DHPropertyIntSlider
        : public QWidget
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(DHPropertyIntSlider, AZ::SystemAllocator, 0);

        DHPropertyIntSlider(QWidget* pParent = NULL);
        virtual ~DHPropertyIntSlider();

        virtual void focusInEvent(QFocusEvent* e);

    public slots:
        void setValue(AZ::s64 val);
        void setMinimum(AZ::s64 val);
        void setMaximum(AZ::s64 val);
        void setStep(AZ::s64 val);
        void setMultiplier(AZ::s64 val);
        void setSoftMinimum(AZ::s64 val);
        void setSoftMaximum(AZ::s64 val);
        void setPrefix(QString val);
        void setSuffix(QString val);

        AZ::s64 minimum() const;
        AZ::s64 maximum() const;
        AZ::s64 step() const;
        AZ::s64 value() const;
        AZ::s64 softMinimum() const;
        AZ::s64 softMaximum() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

        void onChildSliderValueChange(int val);
        void onChildSpinboxValueChange(int val);
    signals:
        void valueChanged(AZ::s64 val);
        void sliderReleased();

    private:
        AZ::s64 m_multiplier;
        AZ::s64 m_softMinimum;
        AZ::s64 m_softMaximum;
        bool m_useSoftMinimum;
        bool m_useSoftMaximum;
        QSlider* m_pSlider;
        QSpinBox* m_pSpinBox;

        Q_DISABLE_COPY(DHPropertyIntSlider)
    };

    // note:  QT Objects cannot themselves be templates, else I would templatize creategui as well.
    template <class ValueType>
    class IntSliderHandlerCommon
        : public PropertyHandler<ValueType, DHPropertyIntSlider>
    {
        AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::Slider; }
        QWidget* GetFirstInTabOrder(DHPropertyIntSlider* widget) override { return widget->GetFirstInTabOrder(); }
        QWidget* GetLastInTabOrder(DHPropertyIntSlider* widget) override { return widget->GetLastInTabOrder(); }
        void UpdateWidgetInternalTabbing(DHPropertyIntSlider* widget) override { widget->UpdateTabOrder(); }
    };

    class s16PropertySliderHandler
        : QObject
        , public IntSliderHandlerCommon<AZ::s16>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(s16PropertySliderHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    class u16PropertySliderHandler
        : QObject
        , public IntSliderHandlerCommon<AZ::u16>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(u16PropertySliderHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };


    class s32PropertySliderHandler
        : QObject
        , public IntSliderHandlerCommon<AZ::s32>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(s32PropertySliderHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    class u32PropertySliderHandler
        : QObject
        , public IntSliderHandlerCommon<AZ::u32>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(u32PropertySliderHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    class s64PropertySliderHandler
        : QObject
        , public IntSliderHandlerCommon<AZ::s64>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(s64PropertySliderHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    class u64PropertySliderHandler
        : QObject
        , public IntSliderHandlerCommon<AZ::u64>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(u64PropertySliderHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(DHPropertyIntSlider* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, DHPropertyIntSlider* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, DHPropertyIntSlider* GUI, const property_t& instance, InstanceDataNode* node)  override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
    };

    void RegisterIntSliderHandlers();
};

#endif
