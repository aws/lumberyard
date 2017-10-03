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

// include the required headers
#include "PropertyWidget.h"
#include "AttributeWidgetFactory.h"
#include "MysticQtManager.h"
#include <MCore/Source/AttributePool.h>
#include <MCore/Source/MCoreSystem.h>
#include <QtWidgets/QWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMenu>
#include <QContextMenuEvent>
#include <QTreeWidgetItem>
#include <QClipboard>
#include <QApplication>
#include <MCore/Source/AttributeFloat.h>

namespace MysticQt
{
    // constructor
    PropertyWidget::Property::Property(QTreeWidgetItem* item, AttributeWidget* attributeWidget, MCore::Attribute* attributeValue, MCore::AttributeSettings* settings, bool autoDelete)
    {
        mItem               = item;
        mAttributeValue     = attributeValue;
        mAttributeWidget    = attributeWidget;
        mAttributeSettings  = settings;
        mAutoDelete         = autoDelete;
    }


    // destructor
    PropertyWidget::Property::~Property()
    {
        if (mAutoDelete)
        {
            if (mAttributeValue)
            {
                mAttributeValue->Destroy();
            }

            if (mAttributeSettings)
            {
                mAttributeSettings->Destroy();
            }
        }
    }


    // add an attribute set
    void PropertyWidget::AddAttributeSetReadOnly(MCore::AttributeSet* set, bool clearFirst, const char* groupName)
    {
        if (clearFirst)
        {
            Clear();
        }

        const uint32 numAttribs = set->GetNumAttributes();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            Property* property = AddProperty(groupName, set->GetAttributeValue(i), set->GetAttributeSettings(i), true);
            property->SetAutoDelete(false);
        }
    }


    // return as boolean
    bool PropertyWidget::Property::AsBool() const
    {
        if (mAttributeValue->GetType() == MCore::AttributeFloat::TYPE_ID)
        {
            return (static_cast<MCore::AttributeFloat*>(mAttributeValue)->GetValue() > MCore::Math::epsilon);
        }

        MCORE_ASSERT(false);
        MCore::LogWarning("Property::AsBool(): Cannot convert attribute. Attribute type incorrect");
        return false;
    }


    // return as integer
    int32 PropertyWidget::Property::AsInt() const
    {
        if (mAttributeValue->GetType() == MCore::AttributeFloat::TYPE_ID)
        {
            return (int32) static_cast<MCore::AttributeFloat*>(mAttributeValue)->GetValue();
        }

        MCORE_ASSERT(false);
        MCore::LogWarning("Property::AsInt(): Cannot convert attribute. Attribute type incorrect");
        return 0;
    }


    // return as float
    float PropertyWidget::Property::AsFloat() const
    {
        if (mAttributeValue->GetType() == MCore::AttributeFloat::TYPE_ID)
        {
            return static_cast<MCore::AttributeFloat*>(mAttributeValue)->GetValue();
        }

        MCORE_ASSERT(false);
        MCore::LogWarning("Property::AsFloat(): Cannot convert attribute. Attribute type incorrect");
        return 0.0f;
    }


    // return as string
    MCore::String PropertyWidget::Property::AsString() const
    {
        if (mAttributeValue->GetType() == MCore::AttributeString::TYPE_ID)
        {
            return static_cast<MCore::AttributeString*>(mAttributeValue)->GetValue();
        }

        MCORE_ASSERT(false);
        MCore::LogWarning("Property::AsString(): Cannot convert attribute. Attribute type incorrect");
        return MCore::String();
    }


    // return as two component vector
    AZ::Vector2 PropertyWidget::Property::AsVector2() const
    {
        if (mAttributeValue->GetType() == MCore::AttributeVector2::TYPE_ID)
        {
            return static_cast<MCore::AttributeVector2*>(mAttributeValue)->GetValue();
        }

        MCORE_ASSERT(false);
        MCore::LogWarning("Property::AsVector2(): Cannot convert attribute. Attribute type incorrect");
        return AZ::Vector2(0.0f, 0.0f);
    }


    // return as three component vector
    MCore::Vector3 PropertyWidget::Property::AsVector3() const
    {
        if (mAttributeValue->GetType() == MCore::AttributeVector3::TYPE_ID)
        {
            return static_cast<MCore::AttributeVector3*>(mAttributeValue)->GetValue();
        }

        MCORE_ASSERT(false);
        MCore::LogWarning("Property::AsVector3(): Cannot convert attribute. Attribute type incorrect");
        return MCore::Vector3(0.0f, 0.0f, 0.0f);
    }


    // return as four component vector
    AZ::Vector4 PropertyWidget::Property::AsVector4() const
    {
        if (mAttributeValue->GetType() == MCore::AttributeVector4::TYPE_ID)
        {
            return static_cast<MCore::AttributeVector4*>(mAttributeValue)->GetValue();
        }

        MCORE_ASSERT(false);
        MCore::LogWarning("Property::AsVector4(): Cannot convert attribute. Attribute type incorrect");
        return AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    }


    // return as color
    MCore::RGBAColor PropertyWidget::Property::AsColor() const
    {
        if (mAttributeValue->GetType() == MCore::AttributeColor::TYPE_ID)
        {
            return static_cast<MCore::AttributeColor*>(mAttributeValue)->GetValue();
        }

        MCORE_ASSERT(false);
        MCore::LogWarning("Property::AsColor(): Cannot convert attribute. Attribute type incorrect");
        return MCore::RGBAColor(0.0f, 0.0f, 0.0f, 1.0f);
    }


    // constructor
    PropertyWidget::PropertyWidget(QWidget* parent, bool showHierarchicalNames)
        : QTreeWidget(parent)
    {
        mShowHierarchicalNames = showHierarchicalNames;

        // setup the behavior
        setSelectionBehavior(QAbstractItemView::SelectRows);
        //  setSelectionMode( QAbstractItemView::NoSelection );
        setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
        setAlternatingRowColors(true);
        setExpandsOnDoubleClick(true);
        setAnimated(true);

        // setup the headers
        QStringList headerList;
        headerList.append("Name");
        headerList.append("Value");
        if (showHierarchicalNames)
        {
            headerList.append("Hierarchical Name");
        }

        setHeaderLabels(headerList);
        //header()->setSectionResizeMode( QHeaderView::ResizeToContents );
        setColumnWidth(0, 150);
        setColumnWidth(1, 300);
        if (showHierarchicalNames)
        {
            setColumnWidth(2, 300);
        }

        header()->setStretchLastSection(true);
        header()->setSectionsMovable(false);

        setObjectName("PropertyWidget");
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    }


    // destructor
    PropertyWidget::~PropertyWidget()
    {
        Clear();
    }


    // clear the contents
    void PropertyWidget::Clear()
    {
        clear();

        // get the number of properties and iterate through them
        const uint32 numProperties = mProperties.GetLength();
        for (uint32 i = 0; i < numProperties; ++i)
        {
            delete mProperties[i];
        }
        mProperties.Clear();
    }


    // find the tree widget item for the given group name, create it if it doesn't exist yet
    QTreeWidgetItem* PropertyWidget::GetGroupWidgetItem(const MCore::String& groupName)
    {
        // split the groun names into their different hierarchy levels
        MCore::Array<MCore::String> groupNames = groupName.Split(MCore::UnicodeCharacter('.'));
        if (groupNames.GetIsEmpty())
        {
            return nullptr;
        }

        // get the group root level name and directly remove it from the array
        MCore::String rootLevelName = groupNames[0];
        groupNames.RemoveFirst();

        // get the number of top level items and iterate through them
        const int32 numTopLevelItems = QTreeWidget::topLevelItemCount();
        for (int32 i = 0; i < numTopLevelItems; ++i)
        {
            QTreeWidgetItem* topLevelItem = QTreeWidget::topLevelItem(i);

            // compare the names of the existing top level items with the one we are searching, if it already exists, just return it
            if (topLevelItem->text(0) == rootLevelName.AsChar())
            {
                return GetGroupWidgetItem(groupNames, topLevelItem);
            }
        }

        // if we reach this line it means that the root level item does not exist, so create it
        QTreeWidgetItem* newItem = new QTreeWidgetItem();
        newItem->setText(0, rootLevelName.AsChar());
        addTopLevelItem(newItem);

        // recursive call
        return GetGroupWidgetItem(groupNames, newItem);
    }


    // find the tree widget item for the given group name, create it if it doesn't exist yet
    QTreeWidgetItem* PropertyWidget::GetGroupWidgetItem(MCore::Array<MCore::String>& groupNames, QTreeWidgetItem* parentItem)
    {
        // if there are no more levels to deal with, return directly
        if (groupNames.GetIsEmpty())
        {
            return parentItem;
        }

        // get the first of the group name levels and directly remove it from the array
        MCore::String currentLevelName = groupNames[0];
        groupNames.RemoveFirst();

        // get the number of child items and iterate through them
        const int32 numChilds = parentItem->childCount();
        for (int32 i = 0; i < numChilds; ++i)
        {
            QTreeWidgetItem* childItem = parentItem->child(i);

            // compare the names of the existing top level items with the one we are searching, if it already exists, just return it
            if (childItem->text(0) == currentLevelName.AsChar())
            {
                return GetGroupWidgetItem(groupNames, childItem);
            }
        }

        // if we reach this line it means that the item does not exist, so create it
        QTreeWidgetItem* newItem = new QTreeWidgetItem();
        newItem->setText(0, currentLevelName.AsChar());
        parentItem->addChild(newItem);

        // recursive call
        return GetGroupWidgetItem(groupNames, newItem);
    }


    // set the expanded flag to a given item
    void PropertyWidget::SetIsExpanded(const char* groupName, bool isExpanded)
    {
        // get the tree widget item based on the group name, and return directly if it doesn't exist
        QTreeWidgetItem* treeWidgetItem = GetGroupWidgetItem(groupName);
        if (treeWidgetItem == nullptr)
        {
            MCORE_ASSERT(false);
            return;
        }

        treeWidgetItem->setExpanded(isExpanded);
    }


    // get the expanded flag from a given item
    bool PropertyWidget::GetIsExpanded(const char* groupName)
    {
        // get the tree widget item based on the group name, and return directly if it doesn't exist
        QTreeWidgetItem* treeWidgetItem = GetGroupWidgetItem(groupName);
        if (treeWidgetItem == nullptr)
        {
            return false;
        }

        return treeWidgetItem->isExpanded();
    }


    // build a property with some readonly text as value
    QTreeWidgetItem* PropertyWidget::AddTreeWidgetItem(const QString& name, QTreeWidgetItem* parentItem, const QString& valueText)
    {
        // create the new item
        QTreeWidgetItem* newItem;
        if (parentItem == nullptr)
        {
            newItem = new QTreeWidgetItem(this);
        }
        else
        {
            newItem = new QTreeWidgetItem(parentItem);
        }

        // set the name
        newItem->setText(0, name);

        // if we specified some text to use as value, set it
        if (valueText.length() > 0)
        {
            newItem->setText(1, valueText);
        }

        // return the new item
        return newItem;
    }


    // add a property with a given widget
    QTreeWidgetItem* PropertyWidget::AddTreeWidgetItem(const QString& name, QTreeWidgetItem* parentItem, QWidget* widget)
    {
        // create the new item
        QTreeWidgetItem* newItem = AddTreeWidgetItem(name, parentItem);

        // set the item widget
        setItemWidget(newItem, 1, widget);

        // return the new item
        return newItem;
    }


    // add a new property
    PropertyWidget::Property* PropertyWidget::AddProperty(QTreeWidgetItem* parentItem, const char* propertyName, AttributeWidget* attributeWidget, MCore::Attribute* attributeValue, MCore::AttributeSettings* settings, bool autoDelete)
    {
        // create the tree widget item and put the attribute widget in it
        QTreeWidgetItem* treeWidgetItem = AddTreeWidgetItem(propertyName, parentItem, attributeWidget);

        // create a new property
        Property* property = new Property(treeWidgetItem, attributeWidget, attributeValue, settings, autoDelete);
        if (mShowHierarchicalNames)
        {
            treeWidgetItem->setText(2, attributeValue->BuildHierarchicalName().AsChar());
        }
        mProperties.Add(property);

        if (settings)
        {
            // set the tooltip
            MCore::String tempString;
            settings->BuildToolTipString(tempString, attributeValue);
            property->GetTreeWidgetItem()->setToolTip(0, tempString.AsChar());

            if (settings->GetReferencesOtherAttribute())
            {
                property->GetTreeWidgetItem()->setBackgroundColor(0, QColor(45, 45, 45));
            }
        }

        return property;
    }


    // add a new property
    PropertyWidget::Property* PropertyWidget::AddProperty(const char* groupName, const char* propertyName, AttributeWidget* attributeWidget, MCore::Attribute* attributeValue, MCore::AttributeSettings* settings, bool autoDelete)
    {
        // find the corresponding parent item based on the group name
        QTreeWidgetItem* parentItem = GetGroupWidgetItem(groupName);

        // create the tree widget item and put the attribute widget in it
        QTreeWidgetItem* treeWidgetItem = AddTreeWidgetItem(propertyName, parentItem, attributeWidget);

        // create a new property
        Property* property = new Property(treeWidgetItem, attributeWidget, attributeValue, settings, autoDelete);
        mProperties.Add(property);

        if (settings)
        {
            MCore::String tempString;
            settings->BuildToolTipString(tempString, attributeValue);
            property->GetTreeWidgetItem()->setToolTip(0, tempString.AsChar());

            if (settings->GetReferencesOtherAttribute())
            {
                property->GetTreeWidgetItem()->setBackgroundColor(0, QColor(45, 45, 45));
            }
        }

        return property;
    }


    // add a new property
    PropertyWidget::Property* PropertyWidget::AddProperty(const char* groupName, MCore::Attribute* attributeValue, MCore::AttributeSettings* settings, bool readOnly)
    {
        Property* result = nullptr;
        if (attributeValue->GetCanHaveChildren() == false)
        {
            AttributeWidget* attributeWidget = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->CreateAttributeWidget(attributeValue, settings, nullptr, readOnly);
            if (attributeWidget)
            {
                if (readOnly == false)
                {
                    connect(attributeWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
                }
                attributeWidget->SetReadOnly(readOnly);
            }
            result = AddProperty(groupName, settings->GetName(), attributeWidget, attributeValue, settings);
        }
        else
        {
            result = AddHierarchicalProperty(GetGroupWidgetItem(groupName), attributeValue, settings, 0, readOnly);
        }

        // set the tooltip
        MCore::String tempString;
        settings->BuildToolTipString(tempString, attributeValue);
        result->GetTreeWidgetItem()->setToolTip(0, tempString.AsChar());

        return result;
    }


    // find the property based on the given attribute widget
    PropertyWidget::Property* PropertyWidget::FindProperty(MysticQt::AttributeWidget* attributeWidget) const
    {
        // get the number of properties and iterate through them
        const uint32 numProperties = mProperties.GetLength();
        for (uint32 i = 0; i < numProperties; ++i)
        {
            // get access to the current property and compare it against the given one, if they are the same return directly
            Property* property = mProperties[i];
            if (property->GetAttributeWidget() == attributeWidget)
            {
                return property;
            }
        }

        // attribute widget not found, return failure
        return nullptr;
    }


    // called when a value of a attribute widget got changed
    void PropertyWidget::OnValueChanged()
    {
        // get the attribute widget that fired the value change signal
        AttributeWidget* attributeWidget = static_cast<AttributeWidget*>(sender());

        // try to find the property based on the attribute widget
        Property* property = FindProperty(attributeWidget);

        // if the property is valid, fire the value changed signal
        if (property)
        {
            FireValueChangedSignal(property);
        }
    }


    // create an integer property
    PropertyWidget::Property* PropertyWidget::AddIntProperty(const char* groupName, const char* valueName, int32 value, int32 defaultValue, int32 min, int32 max, bool readOnly)
    {
        MCore::AttributeFloat*      attributeValue      = MCore::AttributeFloat::Create();
        MCore::AttributeSettings*   attributeSettings   = MCore::AttributeSettings::Create();

        attributeValue->SetValue(value);
        attributeSettings->SetName(valueName);
        attributeSettings->SetInternalName(valueName);
        attributeSettings->SetDefaultValue(MCore::AttributeFloat::Create(defaultValue));
        attributeSettings->SetMinValue(MCore::AttributeFloat::Create(min));
        attributeSettings->SetMaxValue(MCore::AttributeFloat::Create(max));
        attributeSettings->SetInterfaceType(MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER);

        // create and return the property
        return AddProperty(groupName, attributeValue, attributeSettings, readOnly);
    }


    // create a float property
    PropertyWidget::Property* PropertyWidget::AddFloatSpinnerProperty(const char* groupName, const char* valueName, float value, float defaultValue, float min, float max, bool readOnly)
    {
        MCore::AttributeFloat*      attributeValue      = MCore::AttributeFloat::Create();
        MCore::AttributeSettings*   attributeSettings   = MCore::AttributeSettings::Create();

        attributeValue->SetValue(value);
        attributeSettings->SetName(valueName);
        attributeSettings->SetInternalName(valueName);
        attributeSettings->SetDefaultValue(MCore::AttributeFloat::Create(defaultValue));
        attributeSettings->SetMinValue(MCore::AttributeFloat::Create(min));
        attributeSettings->SetMaxValue(MCore::AttributeFloat::Create(max));
        attributeSettings->SetInterfaceType(MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);

        // create and return the property
        return AddProperty(groupName, attributeValue, attributeSettings, readOnly);
    }


    // create a string property
    PropertyWidget::Property* PropertyWidget::AddStringProperty(const char* groupName, const char* valueName, const char* value, const char* defaultValue, bool readOnly)
    {
        MCore::AttributeString*     attributeValue      = MCore::AttributeString::Create();
        MCore::AttributeSettings*   attributeSettings   = MCore::AttributeSettings::Create();

        attributeValue->SetValue(value);
        attributeSettings->SetName(valueName);
        attributeSettings->SetInternalName(valueName);
        attributeSettings->SetInterfaceType(MCore::ATTRIBUTE_INTERFACETYPE_STRING);

        // in case no default value got specified, just use the value as default
        //if (attributeSettings->GetDefaultValue())
        attributeSettings->SetDefaultValue(MCore::AttributeString::Create(defaultValue));
        //else
        //static_cast<MCore::AttributeString*>(attributeSettings->GetDefaultValue())->SetValue( defaultValue );

        // create and return the property
        return AddProperty(groupName, attributeValue, attributeSettings, readOnly);
    }


    // create a bool property
    PropertyWidget::Property* PropertyWidget::AddBoolProperty(const char* groupName, const char* valueName, bool value, bool defaultValue, bool readOnly)
    {
        MCore::AttributeFloat*      attributeValue      = MCore::AttributeFloat::Create();
        MCore::AttributeSettings*   attributeSettings   = MCore::AttributeSettings::Create();

        attributeValue->SetValue(value);
        attributeSettings->SetName(valueName);
        attributeSettings->SetInternalName(valueName);
        attributeSettings->SetInterfaceType(MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);

        // in case no default value got specified, just use the value as default
        //if (attributeSettings->GetDefaultValue() == nullptr)
        attributeSettings->SetDefaultValue(MCore::AttributeFloat::Create(defaultValue ? 1.0f : 0.0f));
        //else
        //static_cast<MCore::AttributeFloat*>(attributeSettings->GetDefaultValue())->SetValue( defaultValue ? 1.0f : 0.0f );

        // create and return the property
        return AddProperty(groupName, attributeValue, attributeSettings, readOnly);
    }


    // create a color property
    PropertyWidget::Property* PropertyWidget::AddColorProperty(const char* groupName, const char* valueName, const MCore::RGBAColor& value, const MCore::RGBAColor& defaultValue, bool readOnly)
    {
        MCore::AttributeColor*      attributeValue      = MCore::AttributeColor::Create();
        MCore::AttributeSettings*   attributeSettings   = MCore::AttributeSettings::Create();

        attributeValue->SetValue(value);
        attributeSettings->SetName(valueName);
        attributeSettings->SetInternalName(valueName);
        attributeSettings->SetInterfaceType(MCore::ATTRIBUTE_INTERFACETYPE_COLOR);

        // in case no default value got specified, just use the value as default
        //if (attributeSettings->GetDefaultValue() == nullptr)
        attributeSettings->SetDefaultValue(MCore::AttributeColor::Create(defaultValue));
        //else
        //static_cast<MCore::AttributeColor*>(attributeSettings->GetDefaultValue())->SetValue( defaultValue );

        // create and return the property
        return AddProperty(groupName, attributeValue, attributeSettings, readOnly);
    }


    // create a vector3 property
    PropertyWidget::Property* PropertyWidget::AddVector3Property(const char* groupName, const char* valueName, const MCore::Vector3& value, const MCore::Vector3& defaultValue, const MCore::Vector3& min, const MCore::Vector3& max, bool readOnly, bool useGizmo)
    {
        MCore::AttributeVector3*    attributeValue      = MCore::AttributeVector3::Create();
        MCore::AttributeSettings*   attributeSettings   = MCore::AttributeSettings::Create();

        attributeValue->SetValue(value);
        attributeSettings->SetName(valueName);
        attributeSettings->SetInternalName(valueName);

        if (useGizmo)
        {
            attributeSettings->SetInterfaceType(MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO);
        }
        else
        {
            attributeSettings->SetInterfaceType(MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3);
        }

        attributeSettings->SetMinValue(MCore::AttributeVector3::Create(min));
        attributeSettings->SetMaxValue(MCore::AttributeVector3::Create(max));

        // in case no default value got specified, just use the value as default
        //if (attributeSettings->GetDefaultValue() == nullptr)
        attributeSettings->SetDefaultValue(MCore::AttributeVector3::Create(defaultValue));
        //else
        //attributeSettings->SetDefaultValue( MCore::AttributeVector3::Create(defaultValue) );

        // create and return the property
        return AddProperty(groupName, attributeValue, attributeSettings, readOnly);
    }


    // create a vector4 property
    PropertyWidget::Property* PropertyWidget::AddVector4Property(const char* groupName, const char* valueName, const AZ::Vector4& value, const AZ::Vector4& defaultValue, const AZ::Vector4& min, const AZ::Vector4& max, bool readOnly)
    {
        MCore::AttributeVector4*    attributeValue      = MCore::AttributeVector4::Create();
        MCore::AttributeSettings*   attributeSettings   = MCore::AttributeSettings::Create();

        attributeValue->SetValue(value);
        attributeSettings->SetName(valueName);
        attributeSettings->SetInternalName(valueName);
        attributeSettings->SetInterfaceType(MCore::ATTRIBUTE_INTERFACETYPE_VECTOR4);
        attributeSettings->SetMinValue(MCore::AttributeVector4::Create(min));
        attributeSettings->SetMaxValue(MCore::AttributeVector4::Create(max));

        // in case no default value got specified, just use the value as default
        //if (defaultValue == nullptr)
        attributeSettings->SetDefaultValue(MCore::AttributeVector4::Create(defaultValue));
        //else
        //attributeSettings->SetDefaultValue( MCore::AttributeVector4::Create(defaultValue) );

        // create and return the property
        return AddProperty(groupName, attributeValue, attributeSettings, readOnly);
    }


    // add an attribute set
    PropertyWidget::Property* PropertyWidget::AddAttributeSet(const char* groupName, MCore::AttributeSet* set, MCore::AttributeSettings* attributeSettings, bool readOnly)
    {
        // create and return the property
        Property* result = AddProperty(groupName, set, attributeSettings, readOnly);
        result->SetAutoDelete(false);
        return result;
    }


    // add an hierarchical property
    PropertyWidget::Property* PropertyWidget::AddHierarchicalProperty(QTreeWidgetItem* parentItem, MCore::Attribute* attribute, MCore::AttributeSettings* settings, uint32 index, bool readOnly)
    {
        // create a new root item first, for hierarchical attributes
        if (attribute->GetCanHaveChildren())
        {
            QTreeWidgetItem* finalParent = (parentItem) ?  new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(this);
            finalParent->setText(0, settings->GetName());

            if (mShowHierarchicalNames)
            {
                finalParent->setText(2, attribute->BuildHierarchicalName().AsChar());
            }

            // create a new property
            Property* rootProperty = new Property(parentItem, nullptr, attribute, settings, false);
            mProperties.Add(rootProperty);

            const uint32 numChildren = attribute->GetNumChildAttributes();
            for (uint32 i = 0; i < numChildren; ++i)
            {
                MCore::Attribute*           childAttribute          = attribute->GetChildAttribute(i);
                MCore::AttributeSettings*   childAttributeSettings  = attribute->GetChildAttributeSettings(i);
                AddHierarchicalProperty(finalParent, childAttribute, childAttributeSettings, i, readOnly);
            }

            if (attribute->GetType() == MCore::AttributeArray::TYPE_ID && attribute->GetNumChildAttributes() > 5)
            {
                finalParent->setExpanded(false);
            }
            else
            {
                finalParent->setExpanded(true);
            }

            return rootProperty;
        }
        else
        {
            AttributeWidget* attributeWidget = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->CreateAttributeWidget(attribute, settings, nullptr, readOnly);
            if (attributeWidget)
            {
                if (readOnly == false)
                {
                    connect(attributeWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
                }
                attributeWidget->SetReadOnly(readOnly);
            }

            Property* result = nullptr;
            if (strlen(settings->GetName()) > 0)
            {
                result = AddProperty(parentItem, settings->GetName(), attributeWidget, attribute, settings, false);
            }
            else
            {
                MCore::String tempString = "Index #";
                tempString += MCore::String((uint32)index);
                result = AddProperty(parentItem, tempString.AsChar(), attributeWidget, attribute, settings, false);
            }

            return result;
        }
    }


    // context menu event
    void PropertyWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        // create the context menu
        QMenu menu(this);

        QAction* copyValuesAction = menu.addAction("Copy Values");
        copyValuesAction->setIcon(GetMysticQt()->FindIcon("/Images/Icons/Copy.png"));
        connect(copyValuesAction, SIGNAL(triggered()), this, SLOT(OnCopyValues()));

        QAction* copyNamesAction = menu.addAction("Copy Names");
        copyNamesAction->setIcon(GetMysticQt()->FindIcon("/Images/Icons/Copy.png"));
        connect(copyNamesAction, SIGNAL(triggered()), this, SLOT(OnCopyNames()));

        QAction* copyInternalNamesAction = menu.addAction("Copy Internal Names");
        copyInternalNamesAction->setIcon(GetMysticQt()->FindIcon("/Images/Icons/Copy.png"));
        connect(copyInternalNamesAction, SIGNAL(triggered()), this, SLOT(OnCopyInternalNames()));

        if (mShowHierarchicalNames)
        {
            QAction* copyHierarchicalNamesAction = menu.addAction("Copy Hierarchical Names");
            copyHierarchicalNamesAction->setIcon(GetMysticQt()->FindIcon("/Images/Icons/Copy.png"));
            connect(copyHierarchicalNamesAction, SIGNAL(triggered()), this, SLOT(OnCopyHierarchicalNames()));
        }

        QAction* copyNamesAndValuesAction = menu.addAction("Copy Names And Values");
        copyNamesAndValuesAction->setIcon(GetMysticQt()->FindIcon("/Images/Icons/Copy.png"));
        connect(copyNamesAndValuesAction, SIGNAL(triggered()), this, SLOT(OnCopyNamesAndValues()));

        QAction* copyInternalNamesAndValuesAction = menu.addAction("Copy Internal Names And Values");
        copyInternalNamesAndValuesAction->setIcon(GetMysticQt()->FindIcon("/Images/Icons/Copy.png"));
        connect(copyInternalNamesAndValuesAction, SIGNAL(triggered()), this, SLOT(OnCopyInternalNamesAndValues()));

        if (mShowHierarchicalNames)
        {
            QAction* copyHierarchicalNamesAndValuesAction = menu.addAction("Copy Hierarchical Names And Values");
            copyHierarchicalNamesAndValuesAction->setIcon(GetMysticQt()->FindIcon("/Images/Icons/Copy.png"));
            connect(copyHierarchicalNamesAndValuesAction, SIGNAL(triggered()), this, SLOT(OnCopyHierarchicalNamesAndValues()));
        }

        // show the menu at the given position
        menu.exec(event->globalPos());
    }


    // find a given property by an item pointer
    PropertyWidget::Property* PropertyWidget::FindProperty(QTreeWidgetItem* item) const
    {
        const uint32 numProperties = mProperties.GetLength();
        for (uint32 i = 0; i < numProperties; ++i)
        {
            if (mProperties[i]->GetTreeWidgetItem() == item)
            {
                return mProperties[i];
            }
        }

        return nullptr;
    }


    // copy the UI names
    void PropertyWidget::OnCopyNames()
    {
        QString finalString;

        const QList<QTreeWidgetItem*> selected = selectedItems();
        const uint32 numSelected = selected.count();
        for (uint32 i = 0; i < numSelected; ++i)
        {
            Property* curProperty = FindProperty(selected[i]);
            if (curProperty == nullptr)
            {
                continue;
            }

            finalString += selected[i]->text(0);
            if (i < (numSelected - 1))
            {
                finalString += "\n";
            }
        }

        QClipboard* clipBoard = QApplication::clipboard();
        clipBoard->setText(finalString);
    }


    // copy the UI values
    void PropertyWidget::OnCopyValues()
    {
        QString finalString;

        const QList<QTreeWidgetItem*> selected = selectedItems();
        const uint32 numSelected = selected.count();
        for (uint32 i = 0; i < numSelected; ++i)
        {
            Property* curProperty = FindProperty(selected[i]);
            if (curProperty == nullptr)
            {
                continue;
            }

            if (curProperty->GetAttributeValue())
            {
                MCore::String valueToString;
                curProperty->GetAttributeValue()->ConvertToString(valueToString);

                finalString += valueToString.AsChar();
                if (i < (numSelected - 1))
                {
                    finalString += "\n";
                }
            }
        }

        QClipboard* clipBoard = QApplication::clipboard();
        clipBoard->setText(finalString);
    }


    // copy the internal names
    void PropertyWidget::OnCopyInternalNames()
    {
        QString finalString;

        const QList<QTreeWidgetItem*> selected = selectedItems();
        const uint32 numSelected = selected.count();
        for (uint32 i = 0; i < numSelected; ++i)
        {
            Property* curProperty = FindProperty(selected[i]);
            if (curProperty == nullptr)
            {
                continue;
            }

            if (curProperty->GetAttributeSettings())
            {
                finalString += curProperty->GetAttributeSettings()->GetInternalName();
                if (i < (numSelected - 1))
                {
                    finalString += "\n";
                }
            }
        }

        QClipboard* clipBoard = QApplication::clipboard();
        clipBoard->setText(finalString);
    }


    // copy the hierarchical names
    void PropertyWidget::OnCopyHierarchicalNames()
    {
        if (mShowHierarchicalNames == false)
        {
            return;
        }

        QString finalString;

        const QList<QTreeWidgetItem*> selected = selectedItems();
        const uint32 numSelected = selected.count();
        for (uint32 i = 0; i < numSelected; ++i)
        {
            Property* curProperty = FindProperty(selected[i]);
            if (curProperty == nullptr)
            {
                continue;
            }

            finalString += selected[i]->text(2);
            if (i < (numSelected - 1))
            {
                finalString += "\n";
            }
        }

        QClipboard* clipBoard = QApplication::clipboard();
        clipBoard->setText(finalString);
    }


    // copy the names and values
    void PropertyWidget::OnCopyNamesAndValues()
    {
        QString finalString;

        const QList<QTreeWidgetItem*> selected = selectedItems();
        const uint32 numSelected = selected.count();
        for (uint32 i = 0; i < numSelected; ++i)
        {
            Property* curProperty = FindProperty(selected[i]);
            if (curProperty == nullptr)
            {
                continue;
            }

            finalString += selected[i]->text(0);
            finalString += '=';

            if (curProperty->GetAttributeValue())
            {
                MCore::String valueToString;
                curProperty->GetAttributeValue()->ConvertToString(valueToString);

                finalString += valueToString.AsChar();
            }

            if (i < (numSelected - 1))
            {
                finalString += "\n";
            }
        }

        QClipboard* clipBoard = QApplication::clipboard();
        clipBoard->setText(finalString);
    }


    // copy the internal names and values
    void PropertyWidget::OnCopyInternalNamesAndValues()
    {
        QString finalString;

        const QList<QTreeWidgetItem*> selected = selectedItems();
        const uint32 numSelected = selected.count();
        for (uint32 i = 0; i < numSelected; ++i)
        {
            Property* curProperty = FindProperty(selected[i]);
            if (curProperty == nullptr)
            {
                continue;
            }

            if (curProperty->GetAttributeSettings())
            {
                finalString += curProperty->GetAttributeSettings()->GetInternalName();
            }

            finalString += '=';

            if (curProperty->GetAttributeValue())
            {
                MCore::String valueToString;
                curProperty->GetAttributeValue()->ConvertToString(valueToString);

                finalString += valueToString.AsChar();
            }

            if (i < (numSelected - 1))
            {
                finalString += "\n";
            }
        }

        QClipboard* clipBoard = QApplication::clipboard();
        clipBoard->setText(finalString);
    }


    // copy the hierarchical names and values
    void PropertyWidget::OnCopyHierarchicalNamesAndValues()
    {
        QString finalString;

        const QList<QTreeWidgetItem*> selected = selectedItems();
        const uint32 numSelected = selected.count();
        for (uint32 i = 0; i < numSelected; ++i)
        {
            Property* curProperty = FindProperty(selected[i]);
            if (curProperty == nullptr)
            {
                continue;
            }

            finalString += selected[i]->text(2);
            finalString += '=';

            if (curProperty->GetAttributeValue())
            {
                MCore::String valueToString;
                curProperty->GetAttributeValue()->ConvertToString(valueToString);

                finalString += valueToString.AsChar();
            }

            if (i < (numSelected - 1))
            {
                finalString += "\n";
            }
        }

        QClipboard* clipBoard = QApplication::clipboard();
        clipBoard->setText(finalString);
    }
} // namespace MysticQt

#include <MysticQt/Source/PropertyWidget.moc>