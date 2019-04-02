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

#ifndef PROPERTY_ENUMCOMBOBOX_CTRL
#define PROPERTY_ENUMCOMBOBOX_CTRL

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Casting/numeric_cast.h>
#include "PropertyEditorAPI.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <QWidget>

#pragma once

class QComboBox;
class QLineEdit;
class QPushButton;

namespace AzToolsFramework
{
    class PropertyEnumComboBoxCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyEnumComboBoxCtrl, AZ::SystemAllocator, 0);

        PropertyEnumComboBoxCtrl(QWidget* pParent = NULL);
        virtual ~PropertyEnumComboBoxCtrl();

        AZ::s64 value() const;
        void addEnumValue(AZStd::pair<AZ::s64, AZStd::string>& val);
        void setEnumValues(AZStd::vector<AZStd::pair<AZ::s64, AZStd::string> >& vals);

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

    signals:
        void valueChanged(AZ::s64 newValue);

    public slots:
        void setValue(AZ::s64 val);

    protected slots:
        void onChildComboBoxValueChange(int comboBoxIndex);

    private:
        QComboBox* m_pComboBox;
        AZStd::vector< AZStd::pair<AZ::s64, AZStd::string>  > m_enumValues;
    };

    template <class ValueType>
    class EnumPropertyComboBoxHandlerCommon
        : public PropertyHandler<ValueType, PropertyEnumComboBoxCtrl>
    {
        virtual AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::ComboBox; }
        virtual QWidget* GetFirstInTabOrder(PropertyEnumComboBoxCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        virtual QWidget* GetLastInTabOrder(PropertyEnumComboBoxCtrl* widget) override { return widget->GetLastInTabOrder(); }
        virtual void UpdateWidgetInternalTabbing(PropertyEnumComboBoxCtrl* widget) override { widget->UpdateTabOrder(); }

        virtual void ConsumeAttribute(PropertyEnumComboBoxCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override
        {
            (void)debugName;

            // Forward EnumValue fields from the registered enum specified by EnumType
            if (attrib == AZ::Edit::InternalAttributes::EnumType)
            {
                AZ::Uuid typeId;
                if (attrValue->Read<AZ::Uuid>(typeId))
                {
                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

                    if (serializeContext)
                    {
                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            const AZ::Edit::ElementData* data = editContext->GetEnumElementData(typeId);
                            if (data)
                            {
                                for (const AZ::AttributePair& attributePair : data->m_attributes)
                                {
                                    PropertyAttributeReader reader(attrValue->GetInstancePointer(), attributePair.second);
                                    ConsumeAttribute(GUI, attributePair.first, &reader, debugName);
                                }
                            }
                        }
                    }
                }
            }
            else if (attrib == AZ_CRC("EnumValue", 0xe4f32eed))
            {
                AZStd::pair<AZ::s64, AZStd::string>  guiEnumValue;
                AZStd::pair<ValueType, AZStd::string>  enumValue;
                AZ::Edit::EnumConstant<ValueType> enumConstant;
                if (attrValue->Read<AZ::Edit::EnumConstant<ValueType>>(enumConstant))
                {
                    guiEnumValue.first = static_cast<AZ::s64>(enumConstant.m_value);
                    guiEnumValue.second = enumConstant.m_description;
                    GUI->addEnumValue(guiEnumValue);
                }
                else
                {
                    // Legacy path. Support temporarily for compatibility.
                    if (attrValue->Read< AZStd::pair<ValueType, AZStd::string> >(enumValue))
                    {
                        guiEnumValue = static_cast<AZStd::pair<AZ::s64, AZStd::string>>(enumValue);
                        GUI->addEnumValue(guiEnumValue);
                    }
                    else
                    {
                        AZStd::pair<ValueType, const char*>  enumValueConst;
                        if (attrValue->Read< AZStd::pair<ValueType, const char*> >(enumValueConst))
                        {
                            AZStd::string consted = enumValueConst.second;
                            guiEnumValue = AZStd::make_pair(static_cast<AZ::s64>(enumValueConst.first), consted);
                            GUI->addEnumValue(guiEnumValue);
                        }
                        else
                        {
                            AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'EnumValue' attribute from property '%s' into enum combo box. Expected pair<IntegerType, char*> or pair<IntegerType, AZStd::string>, where IntegerType is int or u32 ", debugName);
                        }
                    }
                }
            }
            else if (attrib == AZ::Edit::Attributes::EnumValues)
            {
                AZStd::vector<AZStd::pair<AZ::s64, AZStd::string>>  guiEnumValues;
                AZStd::vector<AZStd::pair<ValueType, AZStd::string> > enumValues;
                AZStd::vector<AZ::Edit::EnumConstant<ValueType>> enumConstantValues;
                if (attrValue->Read<AZStd::vector<AZ::Edit::EnumConstant<ValueType>>>(enumConstantValues))
                {
                    for (const AZ::Edit::EnumConstant<ValueType>& constantValue : enumConstantValues)
                    {
                        guiEnumValues.push_back();
                        auto& enumValue = guiEnumValues.back();
                        enumValue.first = static_cast<AZ::s64>(constantValue.m_value);
                        enumValue.second = constantValue.m_description;
                    }

                    GUI->setEnumValues(guiEnumValues);
                }
                else
                {
                    // Legacy path. Support temporarily for compatibility.
                    if (attrValue->Read< AZStd::vector<AZStd::pair<ValueType, AZStd::string> > >(enumValues))
                    {
                        for (auto it = enumValues.begin(); it != enumValues.end(); ++it)
                        {
                            guiEnumValues.push_back(static_cast<AZStd::pair<AZ::s64, AZStd::string> >(*it));
                        }
                        GUI->setEnumValues(guiEnumValues);
                    }
                    else
                    {
                        AZStd::vector<AZStd::pair<ValueType, const char*> > attempt2;
                        if (attrValue->Read<AZStd::vector<AZStd::pair<ValueType, const char*> > >(attempt2))
                        {
                            for (auto it = attempt2.begin(); it != attempt2.end(); ++it)
                            {
                                guiEnumValues.push_back(AZStd::make_pair<AZ::s64, AZStd::string>(static_cast<AZ::s64>(it->first), it->second));
                            }
                            GUI->setEnumValues(guiEnumValues);
                        }
                        else
                        {
                            // emit a warning!
                            AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'EnumValue' attribute from property '%s' into enum combo box", debugName);
                        }
                    }
                }
            }
        }
    };

    template <class EnumUnderlyingType, class Derived>
    class EnumClassPropertyComboBoxHandler
        : public EnumPropertyComboBoxHandlerCommon<EnumUnderlyingType>
    {
    public:
        void WriteGUIValuesIntoProperty(size_t index, PropertyEnumComboBoxCtrl* GUI, EnumUnderlyingType& instance, InstanceDataNode* node) override
        {
            (int)index;
            (void)node;
            AZ::s64 val = GUI->value();
            instance = static_cast<EnumUnderlyingType>(val);
        }

        bool ReadValuesIntoGUI(size_t index, PropertyEnumComboBoxCtrl* GUI, const EnumUnderlyingType& instance, InstanceDataNode* node)  override
        {
            (int)index;
            (void)node;
            AZ::s64 val = aznumeric_cast<AZ::s64>(instance);
            GUI->setValue(val);
            return false;
        }

        QWidget* CreateGUI(QWidget* pParent) override
        {
            PropertyEnumComboBoxCtrl* newCtrl = aznew PropertyEnumComboBoxCtrl(pParent);
            static_cast<Derived*>(this)->connect(newCtrl, &PropertyEnumComboBoxCtrl::valueChanged, static_cast<Derived*>(this), [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
            return newCtrl;
        }
    };

    class charEnumPropertyComboBoxHandler
        : public QObject
        , public EnumClassPropertyComboBoxHandler<char, charEnumPropertyComboBoxHandler>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(charEnumPropertyComboBoxHandler, AZ::SystemAllocator, 0);
    };

    class s8EnumPropertyComboBoxHandler
        : public QObject
        , public EnumClassPropertyComboBoxHandler<AZ::s8, s8EnumPropertyComboBoxHandler>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(s8EnumPropertyComboBoxHandler, AZ::SystemAllocator, 0);
    };

    class u8EnumPropertyComboBoxHandler
        : public QObject
        , public EnumClassPropertyComboBoxHandler<AZ::u8, u8EnumPropertyComboBoxHandler>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(u8EnumPropertyComboBoxHandler, AZ::SystemAllocator, 0);
    };

    class s16EnumPropertyComboBoxHandler
        : public QObject
        , public EnumClassPropertyComboBoxHandler<AZ::s16, s16EnumPropertyComboBoxHandler>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(s16EnumPropertyComboBoxHandler, AZ::SystemAllocator, 0);
    };

    class u16EnumPropertyComboBoxHandler
        : public QObject
        , public EnumClassPropertyComboBoxHandler<AZ::u16, u16EnumPropertyComboBoxHandler>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(u16EnumPropertyComboBoxHandler, AZ::SystemAllocator, 0);
    };

    // Default: int
    class defaultEnumPropertyComboBoxHandler
        : public QObject
        , public EnumClassPropertyComboBoxHandler<int, defaultEnumPropertyComboBoxHandler>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(defaultEnumPropertyComboBoxHandler, AZ::SystemAllocator, 0);
    };

    class u32EnumPropertyComboBoxHandler
        : public QObject
        , public EnumClassPropertyComboBoxHandler<AZ::u32, u32EnumPropertyComboBoxHandler>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(u32EnumPropertyComboBoxHandler, AZ::SystemAllocator, 0);
    };

    class s64EnumPropertyComboBoxHandler
        : public QObject
        , public EnumClassPropertyComboBoxHandler<AZ::s64, s64EnumPropertyComboBoxHandler>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(s64EnumPropertyComboBoxHandler, AZ::SystemAllocator, 0);
    };

    class u64EnumPropertyComboBoxHandler
        : public QObject
        , public EnumClassPropertyComboBoxHandler<AZ::u64, u64EnumPropertyComboBoxHandler>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(u64EnumPropertyComboBoxHandler, AZ::SystemAllocator, 0);
    };

    void RegisterEnumComboBoxHandler();
};

#endif
