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

// include required headers
#include "ParameterCreateEditDialog.h"
#include "AnimGraphPlugin.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>

#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <MysticQt/Source/AttributeWidgetFactory.h>


#define ATTRIBUTEWIDGET_LABEL_WIDTH 100

namespace EMStudio
{
    // constructor
    ParameterCreateEditDialog::ParameterCreateEditDialog(AnimGraphPlugin* plugin, QWidget* parent, bool editMode)
        : QDialog(parent)
    {
        // the value type we have chosen
        mPlugin                     = plugin;
        mEditMode                   = editMode;
        mParamContainer             = nullptr;
        mMaxStaticItemDescWidth     = 0;
        mMaxStaticItemValueWidth    = 0;
        mOrgAttributeSettings       = nullptr;

        setWindowTitle(mEditMode ? "Edit Parameter" : "Create Parameter");

        mAttributeSettings          = MCore::AttributeSettings::Create();

        mMainLayout = new QVBoxLayout();
        setLayout(mMainLayout);
        mMainLayout->setAlignment(Qt::AlignTop);

        mStaticLayout = new QGridLayout();
        mMainLayout->addLayout(mStaticLayout);

        // create the create or apply button and the cancel button
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mCreateButton = new QPushButton(mEditMode ? "Apply" : "Create");
        QPushButton* cancelButton = new QPushButton("Cancel");
        buttonLayout->addWidget(mCreateButton);
        buttonLayout->addWidget(cancelButton);
        mMainLayout->addLayout(buttonLayout);
        connect(mCreateButton, SIGNAL(clicked()), this, SLOT(OnValidate()));
        connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    }


    // destructor
    ParameterCreateEditDialog::~ParameterCreateEditDialog()
    {
        ClearAttributesArray();
        mAttributeSettings->Destroy();
    }


    // init the window
    void ParameterCreateEditDialog::Init()
    {
        uint32 i;

        if (mEditMode)
        {
            // get the anim graph
            EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
            if (!animGraph)
            {
                return;
            }

            mOrgAttributeSettings = animGraph->FindParameter(mName.c_str());
            mAttributeSettings->InitFrom(mOrgAttributeSettings);
        }
        else
        {
            mOrgAttributeSettings = nullptr;
            if (mAttributeSettings)
            {
                mAttributeSettings->Destroy();
            }
            mAttributeSettings = MCore::AttributeSettings::Create();
            mAttributeSettings->SetName("");
            mAttributeSettings->SetInternalName("");
            mAttributeSettings->SetInterfaceType(MCore::ATTRIBUTE_INTERFACETYPE_FLOATSLIDER);
        }

        // add the items
        QLabel* label = new QLabel("Parameter Name:");
        label->setMinimumWidth(ATTRIBUTEWIDGET_LABEL_WIDTH);
        //label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
        mStaticLayout->addWidget(label, 0, 0);
        mNameEdit = new QLineEdit();
        //mNameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mStaticLayout->addWidget(mNameEdit, 0, 1);
        mNameEdit->setText(mName.c_str());
        connect(mNameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(OnNameEdited(const QString&)));
        mMaxStaticItemDescWidth = MCore::Max<uint32>(mMaxStaticItemDescWidth, label->sizeHint().width());
        mMaxStaticItemValueWidth = MCore::Max<uint32>(mMaxStaticItemValueWidth, mNameEdit->sizeHint().width());

        label = new QLabel("Description:");
        label->setMinimumWidth(ATTRIBUTEWIDGET_LABEL_WIDTH);
        //label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        mStaticLayout->addWidget(label, 1, 0);
        mDescriptionEdit = new QTextEdit();
        //mDescriptionEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mDescriptionEdit->setText(mDescription.c_str());
        mStaticLayout->addWidget(mDescriptionEdit, 1, 1);
        //connect(mDescriptionEdit, SIGNAL(editingFinished()), this, SLOT(OnDescriptionChange()));
        mMaxStaticItemDescWidth = MCore::Max<uint32>(mMaxStaticItemDescWidth, label->sizeHint().width());
        mMaxStaticItemValueWidth = MCore::Max<uint32>(mMaxStaticItemValueWidth, mDescriptionEdit->sizeHint().width());

        label = new QLabel("Value Type:");
        label->setMinimumWidth(ATTRIBUTEWIDGET_LABEL_WIDTH);
        //label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
        mStaticLayout->addWidget(label, 2, 0);
        mValueTypeCombo = new MysticQt::ComboBox();
        //mValueTypeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        const uint32 numCreators = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->GetNumRegisteredCreators();
        
        for (i = 0; i < numCreators; ++i)
        {
            MysticQt::AttributeWidgetCreator* creator = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->GetRegisteredCreator(i);

            if (creator->CanBeParameter())
            {
                mValueTypeCombo->addItem(creator->GetTypeString(), QVariant(creator->GetType()));
            }
        }

        mStaticLayout->addWidget(mValueTypeCombo, 2, 1);
        connect(mValueTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(OnValueTypeChange(int)));

        mMaxStaticItemDescWidth = MCore::Max<uint32>(mMaxStaticItemDescWidth, label->sizeHint().width());
        mMaxStaticItemValueWidth = MCore::Max<uint32>(mMaxStaticItemValueWidth, mValueTypeCombo->sizeHint().width());

        // init the dynamic part, which changes based on the value data type combobox
        InitDynamicInterfaceOnType(mAttributeSettings->GetInterfaceType());

        // give the name edit field focus, the text is also selected
        mNameEdit->setFocus(Qt::TabFocusReason);
    }


    void ParameterCreateEditDialog::InitDynamicInterfaceOnType(uint32 interfaceTypeID)
    {
        // init the dynamic part, which changes based on the value data type combobox
        mValueTypeCombo->setCurrentIndex(-1);

        int comboIndex = -1;
        const int numComboItems = mValueTypeCombo->count();
        for (int32 i = 0; i < numComboItems; ++i)
        {
            QVariant variantData = mValueTypeCombo->itemData(i);
            MCORE_ASSERT(variantData != QVariant::Invalid);
            if (static_cast<uint32>(variantData.toInt()) == interfaceTypeID)
            {
                comboIndex = i;
                break;
            }
        }

        mValueTypeCombo->setCurrentIndex(comboIndex);
    }


    void ParameterCreateEditDialog::OnNameEdited(const QString& text)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            return;
        }

        mName = text.toUtf8().data();

        // Is the name empty? The parameter name has to be a non-empty.
        if (mName.empty())
        {
            GetManager()->SetWidgetAsInvalidInput(mNameEdit);
            mCreateButton->setEnabled(false);
            return;
        }

        // find the parameter
        MCore::AttributeSettings* parameterFoundByName = animGraph->FindParameter(mName.c_str());

        // create mode
        if (!mEditMode)
        {
            // check if a parameter with the same name is not found
            if (!parameterFoundByName)
            {
                mNameEdit->setStyleSheet("");
                mCreateButton->setEnabled(true);
            }
            else
            {
                GetManager()->SetWidgetAsInvalidInput(mNameEdit);
                mCreateButton->setEnabled(false);
            }
        }
        else // edit mode
        {
            // check if the name is not a duplicate
            // also check if the new param is the same as orginal to enable if the name is the same
            if (!parameterFoundByName || parameterFoundByName == mOrgAttributeSettings)
            {
                mNameEdit->setStyleSheet("");
                mCreateButton->setEnabled(true);
            }
            else
            {
                GetManager()->SetWidgetAsInvalidInput(mNameEdit);
                mCreateButton->setEnabled(false);
            }
        }
    }


    QLayout* ParameterCreateEditDialog::CreateNameLabel(QWidget* widget, MCore::AttributeSettings* attributeSettings)
    {
        AZStd::string   labelString = attributeSettings->GetName();
        labelString += ":";

        QLabel*         label       = new QLabel(labelString.c_str());
        QHBoxLayout*    layout      = new QHBoxLayout();

        layout->addWidget(label);
        layout->addWidget(widget);

        label->setMinimumWidth(ATTRIBUTEWIDGET_LABEL_WIDTH);
        //label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
        //widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        //layout->setAlignment(label, Qt::AlignTop);

        layout->setMargin(0);
        label->setToolTip(attributeSettings->GetDescription());
        widget->setToolTip(attributeSettings->GetDescription());

        return layout;
    }


    // delete items in the attributes array
    void ParameterCreateEditDialog::ClearAttributesArray()
    {
        for (MCore::Attribute* attribute : mAttributes)
        {
            MCore::GetAttributePool().Free(attribute);
        }

        mAttributes.clear();
    }


    // init the dynamic part of the interface
    void ParameterCreateEditDialog::InitDynamicInterface(uint32 valueType)
    {
        if (mEditMode == false)
        {
            ClearAttributesArray();
        }

        if (valueType == MCORE_INVALIDINDEX32)
        {
            return;
        }

        const uint32 creatorIndex = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->FindCreatorIndexByType(valueType);
        if (creatorIndex == MCORE_INVALIDINDEX32)
        {
            return;
        }

        delete mParamContainer;
        mParamContainer = new QWidget();
        mParamContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mDynamicLayout = new QVBoxLayout();
        mDynamicLayout->setMargin(0);
        mParamContainer->setLayout(mDynamicLayout);
        mMainLayout->insertWidget(1, mParamContainer);
        mDynamicLayout->setAlignment(Qt::AlignLeft);

        MysticQt::AttributeWidgetCreator* creator = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->GetRegisteredCreator(creatorIndex);
        mAttributeSettings->SetInterfaceType(creator->GetType());

        //AZ_Printf("EMotionFX", "InterfaceType: %i, InterfaceTypeString:%s, DataType: %i", creator->GetType(), creator->GetTypeString(), creator->GetDataType());

        bool readOnly = false;
        const AZStd::string oldAttributeName = mAttributeSettings->GetName();

        MCore::Array<MCore::Attribute*> defaultAttribute;
        MCore::Array<MCore::Attribute*> minAttribute;
        MCore::Array<MCore::Attribute*> maxAttribute;
        mAttributeSettings->SetName("Default");
        if (mEditMode == false)
        {
            defaultAttribute.Add(nullptr);
        }
        else
        {
            defaultAttribute.Add(mAttributes[VALUE_DEFAULT]);
        }
        QWidget* widget = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->CreateAttributeWidget(defaultAttribute, mAttributeSettings, nullptr, readOnly, true, true,  MysticQt::AttributeWidgetFactory::ATTRIBUTE_DEFAULT, true);
        widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        mDynamicLayout->addLayout(CreateNameLabel(widget, mAttributeSettings));

        if (creator->GetHasMinMaxValues())
        {
            if (mEditMode == false)
            {
                minAttribute.Add(nullptr);
                maxAttribute.Add(nullptr);
            }
            else
            {
                minAttribute.Add(mAttributes[VALUE_MINIMUM]);
                maxAttribute.Add(mAttributes[VALUE_MAXIMUM]);
            }

            mAttributeSettings->SetName("Minimum");
            widget = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->CreateAttributeWidget(minAttribute, mAttributeSettings, nullptr, readOnly, false, !mEditMode,  MysticQt::AttributeWidgetFactory::ATTRIBUTE_MIN, true);
            widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
            mDynamicLayout->addLayout(CreateNameLabel(widget, mAttributeSettings));

            mAttributeSettings->SetName("Maximum");
            widget = MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->CreateAttributeWidget(maxAttribute, mAttributeSettings, nullptr, readOnly, false, !mEditMode,  MysticQt::AttributeWidgetFactory::ATTRIBUTE_MAX, true);
            widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
            mDynamicLayout->addLayout(CreateNameLabel(widget, mAttributeSettings));
        }
        else
        {
            if (mEditMode == false)
            {
                minAttribute.Add(nullptr);
                maxAttribute.Add(nullptr);
            }
            else
            {
                minAttribute.Add(mAttributes[VALUE_MINIMUM]);
                maxAttribute.Add(mAttributes[VALUE_MAXIMUM]);
            }

            MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->InitAttributes(minAttribute, mAttributeSettings, false,  MysticQt::AttributeWidgetFactory::ATTRIBUTE_MIN);
            MysticQt::GetMysticQt()->GetAttributeWidgetFactory()->InitAttributes(maxAttribute, mAttributeSettings, false,  MysticQt::AttributeWidgetFactory::ATTRIBUTE_MAX);
        }

        // scalable checkbox
        QLabel*         checkLabel          = new QLabel("Scalable:");
        QHBoxLayout*    checkBoxLayout      = new QHBoxLayout();
        mScaleBox = new QCheckBox();
        checkBoxLayout->addWidget(checkLabel);
        checkBoxLayout->addWidget(mScaleBox);
        mScaleBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        checkLabel->setMinimumWidth(ATTRIBUTEWIDGET_LABEL_WIDTH);
        checkBoxLayout->setMargin(0);
        if (mEditMode)
        {
            mScaleBox->setChecked(mAttributeSettings->GetCanScale());
        }
        checkLabel->setToolTip("If checked, the default, min and max values are scalable. If marked as scalable the values will be scaled when applying a Scale AnimGraph Data command.");
        mScaleBox->setToolTip("If checked, the default, min and max values are scalable. If marked as scalable the values will be scaled when applying a Scale AnimGraph Data command.");
        mDynamicLayout->addLayout(checkBoxLayout);

        // clear the existing attributes
        if (mEditMode == false)
        {
            ClearAttributesArray();
        }

        // set the new attributes
        mAttributes.resize(3);
        mAttributes[VALUE_DEFAULT] = defaultAttribute[0];
        mAttributes[VALUE_MINIMUM] = (minAttribute.GetLength() > 0) ? minAttribute[0] : nullptr;
        mAttributes[VALUE_MAXIMUM] = (maxAttribute.GetLength() > 0) ? maxAttribute[0] : nullptr;

        mAttributeSettings->SetName(oldAttributeName.c_str());
    }


    // when the value type changes
    void ParameterCreateEditDialog::OnValueTypeChange(int valueType)
    {
        assert(sender()->inherits("QComboBox"));
        MysticQt::ComboBox* comboBox = qobject_cast<MysticQt::ComboBox*>(sender());

        if (valueType != -1)
        {
            QVariant variantData = comboBox->itemData(valueType);
            assert(variantData != QVariant::Invalid);
            InitDynamicInterface(variantData.toInt());
        }
    }


    // check if we can create this parameter
    void ParameterCreateEditDialog::OnValidate()
    {
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            MCore::LogWarning("ParameterCreateEditDialog::OnValidate() - No AnimGraph active!");
            return;
        }

        if (mName.empty())
        {
            QMessageBox::warning(this, "Please Provide A Parameter Name", "The parameter name cannot be empty!");
            return;
        }

        // check if the name already exists
        MCore::AttributeSettings* paramInfo = animGraph->FindParameter(mName.c_str());
        if (paramInfo && mOrgAttributeSettings != paramInfo)
        {
            const AZStd::string errorString = AZStd::string::format("Parameter with name '<b>%s</b>' already exists in anim graph '<b>%s</b>'.<br><br><i>Please use a unique parameter name.</i>", paramInfo->GetName(), animGraph->GetName());
            QMessageBox::warning(this, "Parameter Name Not Unique", errorString.c_str());
            return;
        }

        // Store the description.
        mDescription = mDescriptionEdit->toPlainText().toUtf8().data();

        emit accept();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterCreateEditDialog.moc>
