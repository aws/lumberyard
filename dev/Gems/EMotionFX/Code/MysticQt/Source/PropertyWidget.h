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

#ifndef __MYSTICQT_PROPERTYWIDGET_H
#define __MYSTICQT_PROPERTYWIDGET_H

// include the required headers
#include <MCore/Source/StandardHeaders.h>
#include "MysticQtConfig.h"
#include "AttributeWidgets.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/Color.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/Quaternion.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeInt32.h>
#include <MCore/Source/AttributeColor.h>
#include <MCore/Source/AttributeSettings.h>


// forward declarations
QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QContextMenuEvent)


namespace MysticQt
{
    /**
     *
     *
     */
    class MYSTICQT_API PropertyWidget
        : public QTreeWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(PropertyWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS);

    public:
        class MYSTICQT_API Property
        {
            MCORE_MEMORYOBJECTCATEGORY(PropertyWidget::Property, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS);
        public:
            Property(QTreeWidgetItem* item, AttributeWidget* attributeWidget, MCore::Attribute* attributeValue, MCore::AttributeSettings* settings, bool autoDelete = true);
            virtual ~Property();

            MCORE_INLINE QTreeWidgetItem* GetTreeWidgetItem() const                         { return mItem; }
            MCORE_INLINE AttributeWidget* GetAttributeWidget() const                        { return mAttributeWidget; }
            MCORE_INLINE MCore::Attribute* GetAttributeValue() const                        { return mAttributeValue; }
            MCORE_INLINE MCore::AttributeSettings* GetAttributeSettings() const             { return mAttributeSettings; }
            MCORE_INLINE void SetAutoDelete(bool autoDelete)                                { mAutoDelete = autoDelete; }
            MCORE_INLINE bool GetAutoDelete() const                                         { return mAutoDelete; }

            bool AsBool() const;
            int32 AsInt() const;
            float AsFloat() const;
            MCore::String AsString() const;
            AZ::Vector2 AsVector2() const;
            MCore::Vector3 AsVector3() const;
            AZ::Vector4 AsVector4() const;
            MCore::RGBAColor AsColor() const;

        private:
            QTreeWidgetItem*            mItem;
            AttributeWidget*            mAttributeWidget;
            MCore::Attribute*           mAttributeValue;
            MCore::AttributeSettings*   mAttributeSettings;
            bool                        mAutoDelete;
        };

        PropertyWidget(QWidget* parent = nullptr, bool showHierarchicalNames = false);
        virtual ~PropertyWidget();

        void AddAttributeSetReadOnly(MCore::AttributeSet* set, bool clearFirst, const char* groupName);
        void Clear();

        // integer type
        Property* AddIntProperty(const char* groupName, const char* valueName, int32 value, int32 defaultValue, int32 min, int32 max, bool readOnly = false);
        Property* AddReadOnlyIntProperty(const char* groupName, const char* valueName, int32 value)                                                     { return AddIntProperty(groupName, valueName, value, value, -100000000, +100000000, true); }

        // float
        Property* AddFloatSpinnerProperty(const char* groupName, const char* valueName, float value, float defaultValue, float min, float max, bool readOnly = false);
        Property* AddReadOnlyFloatSpinnerProperty(const char* groupName, const char* valueName, float value)                { return AddFloatSpinnerProperty(groupName, valueName, value, value, -FLT_MAX, FLT_MAX, true); }

        // bool
        Property* AddBoolProperty(const char* groupName, const char* valueName, bool value, bool defaultValue, bool readOnly = false);
        Property* AddBoolProperty(const char* groupName, const char* valueName, bool value)                                 { return AddBoolProperty(groupName, valueName, value, value, false); }
        Property* AddReadOnlyBoolProperty(const char* groupName, const char* valueName, bool value)                         { return AddBoolProperty(groupName, valueName, value, value, true); }

        // string
        Property* AddStringProperty(const char* groupName, const char* valueName, const char* value, const char* defaultValue = nullptr, bool readOnly = false);
        Property* AddReadOnlyStringProperty(const char* groupName, const char* valueName, const char* value)                { return AddStringProperty(groupName, valueName, value, value, true); }

        // color
        Property* AddColorProperty(const char* groupName, const char* valueName, const MCore::RGBAColor& value, const MCore::RGBAColor& defaultValue, bool readOnly = false);
        Property* AddColorProperty(const char* groupName, const char* valueName, const MCore::RGBAColor& value)             { return AddColorProperty (groupName, valueName, value, value, false); }
        Property* AddReadOnlyColorProperty(const char* groupName, const char* valueName, const MCore::RGBAColor& value)     { return AddColorProperty (groupName, valueName, value, value, true); }

        // vector3
        Property* AddVector3Property(const char* groupName, const char* valueName, const MCore::Vector3& value, const MCore::Vector3& defaultValue, const MCore::Vector3& min, const MCore::Vector3& max, bool readOnly = false, bool useGizmo = true);
        Property* AddVector3Property(const char* groupName, const char* valueName, const MCore::Vector3& value, bool useGizmo = true)             { return AddVector3Property (groupName, valueName, value, value, MCore::Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX), MCore::Vector3(FLT_MAX, FLT_MAX, FLT_MAX), false, useGizmo); }
        Property* AddReadOnlyVector3Property(const char* groupName, const char* valueName, const MCore::Vector3& value)     { return AddVector3Property (groupName, valueName, value, value, MCore::Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX), MCore::Vector3(FLT_MAX, FLT_MAX, FLT_MAX), true, false); }

        // vector4
        Property* AddVector4Property(const char* groupName, const char* valueName, const AZ::Vector4& value, const AZ::Vector4& defaultValue, const AZ::Vector4& min, const AZ::Vector4& max, bool readOnly = false);
        Property* AddVector4Property(const char* groupName, const char* valueName, const AZ::Vector4& value)                { return AddVector4Property (groupName, valueName, value, value, AZ::Vector4(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX), AZ::Vector4(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX), false); }
        Property* AddReadOnlyVector4Property(const char* groupName, const char* valueName, const AZ::Vector4& value)        { return AddVector4Property (groupName, valueName, value, value, AZ::Vector4(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX), AZ::Vector4(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX), true); }

        // add an attribute set
        Property* AddAttributeSet(const char* groupName, MCore::AttributeSet* set, MCore::AttributeSettings* attributeSettings, bool readOnly);

        // expand or collapse groups
        void SetIsExpanded(const char* groupName, bool isExpanded);
        bool GetIsExpanded(const char* groupName);

        // helpers
        Property* FindProperty(MysticQt::AttributeWidget* attributeWidget) const;
        Property* FindProperty(QTreeWidgetItem* item) const;
        Property* AddProperty(const char* groupName, const char* propertyName, AttributeWidget* attributeWidget, MCore::Attribute* attributeValue, MCore::AttributeSettings* settings, bool autoDelete = true);
        Property* AddHierarchicalProperty(QTreeWidgetItem* parentItem, MCore::Attribute* attribute, MCore::AttributeSettings* settings, uint32 index = 0, bool readOnly = false);
        Property* AddProperty(QTreeWidgetItem* parentItem, const char* propertyName, AttributeWidget* attributeWidget, MCore::Attribute* attributeValue, MCore::AttributeSettings* settings, bool autoDelete);
        Property* AddProperty(const char* groupName, MCore::Attribute* attributeValue, MCore::AttributeSettings* settings, bool readOnly);

    signals:
        void ValueChanged(MysticQt::PropertyWidget::Property* property);

    private slots:
        void FireValueChangedSignal(Property* property)     { emit ValueChanged(property); }
        void OnValueChanged();
        void OnCopyNames();
        void OnCopyValues();
        void OnCopyInternalNames();
        void OnCopyHierarchicalNames();
        void OnCopyNamesAndValues();
        void OnCopyInternalNamesAndValues();
        void OnCopyHierarchicalNamesAndValues();

    private:
        MCore::Array<Property*> mProperties;
        bool                    mShowHierarchicalNames;

        void contextMenuEvent(QContextMenuEvent* event);
        QTreeWidgetItem* AddTreeWidgetItem(const QString& name, QTreeWidgetItem* parentItem = nullptr, const QString& valueText = "");
        QTreeWidgetItem* AddTreeWidgetItem(const QString& name, QTreeWidgetItem* parentItem, QWidget* widget);
        QTreeWidgetItem* GetGroupWidgetItem(const MCore::String& groupName);
        QTreeWidgetItem* GetGroupWidgetItem(MCore::Array<MCore::String>& groupNames, QTreeWidgetItem* parentItem);
    };
} // namespace MysticQt


#endif
