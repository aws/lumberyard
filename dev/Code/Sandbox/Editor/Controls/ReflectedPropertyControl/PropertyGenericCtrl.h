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

#ifndef CRYINCLUDE_EDITOR_UTILS_PROPERTYGENERICCTRL_H
#define CRYINCLUDE_EDITOR_UTILS_PROPERTYGENERICCTRL_H
#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/systemallocator.h>
#include <AzToolsFramework/Ui/PropertyEditor/PropertyEditorAPI.h>
#include "ReflectedVar.h"
#include "Util/VariablePropertyType.h"
#include <QtWidgets/QWidget>

class QStringListModel;
class QListView;
class QLabel;

class GenericPopupPropertyEditor
    : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(GenericPopupPropertyEditor, AZ::SystemAllocator, 0);
    GenericPopupPropertyEditor(QWidget* pParent = nullptr, bool showTwoButtons = false);

    void SetValue(const QString& value, bool notify = true);
    QString GetValue() const { return m_value; }
    void SetPropertyType(PropertyType type);
    PropertyType GetPropertyType() const { return m_propertyType; }

    //override in derived classes to show appropriate editor
    virtual void onEditClicked() {};
    virtual void onButton2Clicked() {};

signals:
    void ValueChanged(const QString& value);

private:
    QLabel* m_valueLabel;
    PropertyType m_propertyType;
    QString m_value;
};

template <class T, AZ::u32 CRC>
class GenericPopupWidgetHandler
    : public QObject
    , public AzToolsFramework::PropertyHandler < CReflectedVarGenericProperty, GenericPopupPropertyEditor >
{
public:
    AZ_CLASS_ALLOCATOR(GenericPopupWidgetHandler, AZ::SystemAllocator, 0);
    virtual bool IsDefaultHandler() const override { return false; }

    virtual AZ::u32 GetHandlerName(void) const override  { return CRC; }
    virtual QWidget* CreateGUI(QWidget* pParent) override
    {
        GenericPopupPropertyEditor* newCtrl = aznew T(pParent);
        connect(newCtrl, &GenericPopupPropertyEditor::ValueChanged, [newCtrl]()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        return newCtrl;
    }
    virtual void ConsumeAttribute(GenericPopupPropertyEditor* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override
    {
        Q_UNUSED(GUI);
        Q_UNUSED(attrib);
        Q_UNUSED(attrValue);
        Q_UNUSED(debugName);
    }
    virtual void WriteGUIValuesIntoProperty(size_t index, GenericPopupPropertyEditor* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        CReflectedVarGenericProperty val = instance;
        val.m_propertyType = GUI->GetPropertyType();
        val.m_value = GUI->GetValue().toUtf8().data();
        instance = static_cast<property_t>(val);
    }
    virtual bool ReadValuesIntoGUI(size_t index, GenericPopupPropertyEditor* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        CReflectedVarGenericProperty val = instance;
        GUI->SetPropertyType(val.m_propertyType);
        GUI->SetValue(val.m_value.c_str(), false);
        return false;
    }
};

class SOClassPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SOClassPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SOClassesPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SOClassesPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SOStatePropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SOStatePropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SOStatesPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SOStatesPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SOStatePatternPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SOStatePatternPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SOActionPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SOActionPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SOHelperPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SOHelperPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SOEventPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SOEventPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SOTemplatePropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SOTemplatePropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class ShaderPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    ShaderPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class MaterialPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    MaterialPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent, true){}
    void onEditClicked() override;
    void onButton2Clicked() override;
};

class AiBehaviorPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    AiBehaviorPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class AIAnchorPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    AIAnchorPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};


#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
class AiCharacterPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    AiCharacterPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};
#endif


class EquipPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    EquipPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class ReverbPresetPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    ReverbPresetPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class CustomActionPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    CustomActionPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class GameTokenPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    GameTokenPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class MissionObjPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    MissionObjPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SequencePropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SequencePropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SequenceIdPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SequenceIdPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class LocalStringPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    LocalStringPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class LightAnimationPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    LightAnimationPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class ParticleNamePropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    ParticleNamePropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent, true){}
    void onEditClicked() override;
    void onButton2Clicked() override;
};


// AZ_CRC changed recently - it used to be evaluated by the preprocessor to AZ::u32(value); now it evaluates to Az::Crc32, and can't be used as a const template parameter
// So we use our own
#define CONST_AZ_CRC(name, value) AZ::u32(value)

using SOClassPropertyHandler = GenericPopupWidgetHandler<SOClassPropertyEditor, CONST_AZ_CRC("ePropertySOClass", 0x6d13d619)>;
using SOClassesPropertyHandler = GenericPopupWidgetHandler<SOClassesPropertyEditor, CONST_AZ_CRC("ePropertySOClasses", 0x64ef1e71)>;
using SOStatePropertyHandler = GenericPopupWidgetHandler<SOStatePropertyEditor, CONST_AZ_CRC("ePropertySOState", 0x23cb1d7d)>;
using SOStatesPropertyHandler = GenericPopupWidgetHandler<SOStatesPropertyEditor, CONST_AZ_CRC("ePropertySOStates", 0x35990997)>;
using SOStatePatternPropertyHandler = GenericPopupWidgetHandler<SOStatePatternPropertyEditor, CONST_AZ_CRC("ePropertySOStatePattern", 0xbd09853a) >;
using SOActionPropertyHandler = GenericPopupWidgetHandler<SOActionPropertyEditor, CONST_AZ_CRC("ePropertySOAction", 0x4397f248)>;
using SOHelperPropertyHandler = GenericPopupWidgetHandler<SOHelperPropertyEditor, CONST_AZ_CRC("ePropertySOHelper", 0x836c056a)>;
using SONavHelperPropertyHandler = GenericPopupWidgetHandler<SOHelperPropertyEditor, CONST_AZ_CRC("ePropertySONavHelper", 0x1abfbd59)>;
using SOAnimHelperPropertyHandler = GenericPopupWidgetHandler<SOHelperPropertyEditor, CONST_AZ_CRC("ePropertySOAnimHelper", 0x139a4d89)>;
using SOEventPropertyHandler = GenericPopupWidgetHandler<SOEventPropertyEditor, CONST_AZ_CRC("ePropertySOEvent", 0xbbf6c521)>;
using SOTemplatePropertyHandler = GenericPopupWidgetHandler<SOTemplatePropertyEditor, CONST_AZ_CRC("ePropertySOTemplate", 0x5b0a6a76)>;
using ShaderPropertyHandler = GenericPopupWidgetHandler<ShaderPropertyEditor, CONST_AZ_CRC("ePropertyShader", 0xc40932f1)>;
using MaterialPropertyHandler = GenericPopupWidgetHandler<MaterialPropertyEditor, CONST_AZ_CRC("ePropertyMaterial", 0xf324dffa)>;
using AiBehaviorPropertyHandler = GenericPopupWidgetHandler<AiBehaviorPropertyEditor, CONST_AZ_CRC("ePropertyAiBehavior", 0xa780fd1a)>;
using AIAnchorPropertyHandler = GenericPopupWidgetHandler<AIAnchorPropertyEditor, CONST_AZ_CRC("ePropertyAiAnchor", 0x3e446ccb)>;
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
using AiCharacterPropertyHandler = GenericPopupWidgetHandler<AiCharacterPropertyEditor, CONST_AZ_CRC("ePropertyAiCharacter", 0xa5e5d19f)>;
#endif
using EquipPropertyHandler = GenericPopupWidgetHandler<EquipPropertyEditor, CONST_AZ_CRC("ePropertyEquip", 0x66ffd290)>;
using ReverbPresetPropertyHandler = GenericPopupWidgetHandler<ReverbPresetPropertyEditor, CONST_AZ_CRC("ePropertyReverbPreset", 0x51469f38)>;
using CustomActionPropertyHandler = GenericPopupWidgetHandler<CustomActionPropertyEditor, CONST_AZ_CRC("ePropertyCustomAction", 0x4ffa5ba5)>;
using GameTokenPropertyHandler = GenericPopupWidgetHandler<GameTokenPropertyEditor, CONST_AZ_CRC("ePropertyGameToken", 0x34855b6f)>;
using MissionObjPropertyHandler = GenericPopupWidgetHandler<MissionObjPropertyEditor, CONST_AZ_CRC("ePropertyMissionObj", 0x4a2d0dc8)>;
using SequencePropertyHandler = GenericPopupWidgetHandler<SequencePropertyEditor, CONST_AZ_CRC("ePropertySequence", 0xdd1c7d44)>;
using SequenceIdPropertyHandler = GenericPopupWidgetHandler<SequenceIdPropertyEditor, CONST_AZ_CRC("ePropertySequenceId", 0x05983dcc)>;
using LocalStringPropertyHandler = GenericPopupWidgetHandler<LocalStringPropertyEditor, CONST_AZ_CRC("ePropertyLocalString", 0x0cd9609a)>;
using LightAnimationPropertyHandler = GenericPopupWidgetHandler<LightAnimationPropertyEditor, CONST_AZ_CRC("ePropertyLightAnimation", 0x277097da)>;
using ParticleNamePropertyHandler = GenericPopupWidgetHandler<ParticleNamePropertyEditor, CONST_AZ_CRC("ePropertyParticleName", 0xf44c7133)>;

class ListEditWidget : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(ListEditWidget, AZ::SystemAllocator, 0);
    ListEditWidget(QWidget *pParent = nullptr);

    void SetValue(const QString &value, bool notify = true);
    QString GetValue() const { return m_value; }

    QWidget* GetFirstInTabOrder();
    QWidget* GetLastInTabOrder();

signals:
    void ValueChanged(const QString &value);

private:
    void OnModelDataChange();
    virtual void OnEditClicked() {};

protected:
    QLineEdit *m_valueEdit;
    QString m_value;
    QListView *m_listView;
    QStringListModel *m_model;
};


class AIPFPropertiesListEdit : public ListEditWidget
{
public:
    AIPFPropertiesListEdit(QWidget *pParent = nullptr) : ListEditWidget(pParent){}
    void OnEditClicked() override;
};

class AIEntityClassesListEdit : public ListEditWidget
{
public:
    AIEntityClassesListEdit(QWidget *pParent = nullptr) : ListEditWidget(pParent){}
    void OnEditClicked() override;
};

template <class T, AZ::u32 CRC>
class ListEditWidgetHandler : public QObject, public AzToolsFramework::PropertyHandler < CReflectedVarGenericProperty, ListEditWidget >
{
public:
    AZ_CLASS_ALLOCATOR(ListEditWidgetHandler, AZ::SystemAllocator, 0);
    virtual bool IsDefaultHandler() const override { return false; }

    virtual AZ::u32 GetHandlerName(void) const override  { return CRC; }
    virtual QWidget* CreateGUI(QWidget *pParent) override
    {
        ListEditWidget* newCtrl = aznew T(pParent);
        connect(newCtrl, &ListEditWidget::ValueChanged, [newCtrl]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
        });
        return newCtrl;
    }
    virtual void ConsumeAttribute(ListEditWidget* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override {
        Q_UNUSED(GUI); 	Q_UNUSED(attrib); Q_UNUSED(attrValue); Q_UNUSED(debugName);
    }
    virtual void WriteGUIValuesIntoProperty(size_t index, ListEditWidget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        CReflectedVarGenericProperty val = instance;
        val.m_value = GUI->GetValue().toUtf8().data();
        instance = static_cast<property_t>(val);
    }
    virtual bool ReadValuesIntoGUI(size_t index, ListEditWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        CReflectedVarGenericProperty val = instance;
        GUI->SetValue(val.m_value.c_str(), false);
        return false;
    }
    QWidget* GetFirstInTabOrder(ListEditWidget* widget) override { return widget->GetFirstInTabOrder(); }
    QWidget* GetLastInTabOrder(ListEditWidget* widget) override {return widget->GetLastInTabOrder(); }

};

using AIPFPropertiesHandler = ListEditWidgetHandler<AIPFPropertiesListEdit, 0x9b406f43 /*AZ_CRC("ePropertyAiPFPropertiesList", 0x9b406f43 )*/>;
using AIEntityClassesHandler = ListEditWidgetHandler<AIEntityClassesListEdit, 0xd50f1b94 /* AZ_CRC("ePropertyAiEntityClasses", 0xd50f1b94 )*/>;



#endif // CRYINCLUDE_EDITOR_UTILS_PROPERTYGENERICCTRL_H
