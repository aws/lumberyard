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

#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/Attribute.h>
#include "../StandardPluginsConfig.h"
#include "AttributesWindow.h"
#include <QDialog>
#include <QWidget>
#include <QGridLayout>
#include <QTextEdit>


namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;

    class ParameterCreateEditDialog
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ParameterCreateEditDialog, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        enum
        {
            VALUE_DEFAULT = 0,
            VALUE_MINIMUM = 1,
            VALUE_MAXIMUM = 2
        };

        ParameterCreateEditDialog(AnimGraphPlugin* plugin, QWidget* parent, bool editMode = false);
        ~ParameterCreateEditDialog();

        void Init();
        void InitDynamicInterfaceOnType(uint32 interfaceTypeID);

        const AZStd::string& GetName() const                                                    { return mName; }
        const AZStd::string& GetDescription() const                                             { return mDescription; }
        const MCore::Attribute* GetMinValue() const                                             { return mAttributes[VALUE_MINIMUM]; }
        const MCore::Attribute* GetMaxValue() const                                             { return mAttributes[VALUE_MAXIMUM]; }
        const MCore::Attribute* GetDefaultValue() const                                         { return mAttributes[VALUE_DEFAULT]; }
        MCore::AttributeSettings* GetAttributeSettings() const                                  { return mAttributeSettings; }
        bool GetIsScalable() const                                                              { return mScaleBox->isChecked(); }

        void SetName(const char* name)                                                          { mName = name; }
        void SetDescription(const char* description)                                            { mDescription = description; }
        void SetMinValue(MCore::Attribute* value)                                               { mAttributes[VALUE_MINIMUM] = value; }
        void SetMaxValue(MCore::Attribute* value)                                               { mAttributes[VALUE_MAXIMUM] = value; }
        void SetDefaultValue(MCore::Attribute* value)                                           { mAttributes[VALUE_DEFAULT] = value; }

        void SetNumAttributes(uint32 numAttribs)
        {
            ClearAttributesArray();
            mAttributes.resize(numAttribs);
            for (uint32 i = 0; i < numAttribs; ++i)
            {
                mAttributes[i] = nullptr;
            }
        }

    protected slots:
        void OnNameEdited(const QString& text);
        void OnValueTypeChange(int valueType);
        void OnValidate();

    private:
        void InitDynamicInterface(uint32 valueType);
        QLayout* CreateNameLabel(QWidget* widget, MCore::AttributeSettings* attributeSettings);
        void ClearAttributesArray();

    private:
        AnimGraphPlugin*                    mPlugin;
        QVBoxLayout*                        mMainLayout;
        QGridLayout*                        mStaticLayout;
        QVBoxLayout*                        mDynamicLayout;
        MysticQt::ComboBox*                 mValueTypeCombo;
        QWidget*                            mParamContainer;
        QTextEdit*                          mDescriptionEdit;
        QLineEdit*                          mNameEdit;
        QPushButton*                        mCreateButton;
        QCheckBox*                          mScaleBox;
        AZStd::vector<MCore::Attribute*>    mAttributes;
        AZStd::string                       mName;
        AZStd::string                       mDescription;
        MCore::AttributeSettings*           mAttributeSettings;
        MCore::AttributeSettings*           mOrgAttributeSettings;
        bool                                mEditMode;
        uint32                              mMaxStaticItemValueWidth;
        uint32                              mMaxStaticItemDescWidth;
    };
} // namespace EMStudio