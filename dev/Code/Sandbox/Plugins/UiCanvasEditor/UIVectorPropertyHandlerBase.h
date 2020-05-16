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

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyVectorCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <LyShine/UiBase.h>

template <class TypeBeingHandled>
class UIVectorPropertyHandlerBase
    : public AzToolsFramework::PropertyHandler < TypeBeingHandled, AzToolsFramework::PropertyVectorCtrl >
{
protected:
    AzToolsFramework::VectorPropertyHandlerCommon m_common;
public:
    UIVectorPropertyHandlerBase(int elementCount, int elementsPerRow = -1)
        : m_common(elementCount, elementsPerRow)
    {
    }

    AZ::u32 GetHandlerName(void) const override
    {
        return AZ_CRC("UI_Property_Handler", 0x29f0eee2);
    }

    bool IsDefaultHandler() const override
    {
        return true;
    }

    QWidget* GetFirstInTabOrder(AzToolsFramework::PropertyVectorCtrl* widget) override
    {
        return widget->GetFirstInTabOrder();
    }

    QWidget* GetLastInTabOrder(AzToolsFramework::PropertyVectorCtrl* widget) override
    {
        return widget->GetLastInTabOrder();
    }

    void UpdateWidgetInternalTabbing(AzToolsFramework::PropertyVectorCtrl* widget) override
    {
        widget->UpdateTabOrder();
    }

    QWidget* CreateGUI(QWidget* pParent) override
    {
        return m_common.ConstructGUI(pParent);
    }

    virtual void ConsumeAttribute(AzToolsFramework::PropertyVectorCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override
    {
        m_common.ConsumeAttributes(GUI, attrib, attrValue, debugName);
    }

    static TypeBeingHandled ExtractValuesFromGUI(AzToolsFramework::PropertyVectorCtrl* GUI)
    {
        AzToolsFramework::VectorElement** elements = GUI->getElements();
        TypeBeingHandled values;

        values.m_left = aznumeric_cast<decltype(values.m_left)>(elements[0]->GetValue());
        values.m_top = aznumeric_cast<decltype(values.m_top)>(elements[1]->GetValue());
        values.m_right = aznumeric_cast<decltype(values.m_right)>(elements[2]->GetValue());
        values.m_bottom = aznumeric_cast<decltype(values.m_bottom)>(elements[3]->GetValue());

        return values;
    }

    virtual void WriteGUIValuesIntoProperty(size_t, AzToolsFramework::PropertyVectorCtrl* GUI, TypeBeingHandled& instance, AzToolsFramework::InstanceDataNode*) override
    {
        instance = ExtractValuesFromGUI(GUI);
    }

    static void InsertValuesIntoGUI(AzToolsFramework::PropertyVectorCtrl* GUI, TypeBeingHandled values)
    {
        GUI->setValuebyIndex(values.m_left, 0);
        GUI->setValuebyIndex(values.m_top, 1);
        GUI->setValuebyIndex(values.m_right, 2);
        GUI->setValuebyIndex(values.m_bottom, 3);
    }

    virtual bool ReadValuesIntoGUI(size_t, AzToolsFramework::PropertyVectorCtrl* GUI, const TypeBeingHandled& instance, AzToolsFramework::InstanceDataNode*) override
    {
        GUI->blockSignals(true);
        {
            InsertValuesIntoGUI(GUI, instance);
        }
        GUI->blockSignals(false);

        return false;
    }
};
