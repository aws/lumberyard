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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterCreateEditDialog.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ParameterEditorFactory.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ValueParameterEditor.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <MysticQt/Source/ComboBox.h>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>


namespace EMStudio
{
    int ParameterCreateEditDialog::m_parameterEditorMinWidth = 300;

    ParameterCreateEditDialog::ParameterCreateEditDialog(AnimGraphPlugin* plugin, QWidget* parent, const EMotionFX::Parameter* editParameter)
        : QDialog(parent)
        , m_plugin(plugin)
        , m_valueTypeCombo(nullptr)
        , m_valueParameterEditor(nullptr)
    {
        if (!editParameter)
        {
            setWindowTitle("Create Parameter");
            m_createButton = new QPushButton("Create");
        }
        else
        {
            m_parameter.reset(MCore::ReflectionSerializer::Clone(editParameter));
            setWindowTitle("Edit Parameter");
            m_createButton = new QPushButton("Apply");
            m_originalName = editParameter->GetName();
        }
        QVBoxLayout* mainLayout = new QVBoxLayout();
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

        // Value Type
        if (!editParameter ||
            (editParameter && editParameter->RTTI_IsTypeOf(azrtti_typeid<EMotionFX::ValueParameter>())))
        {
            QHBoxLayout* valueTypeLayout = new QHBoxLayout();
            QLabel* valueTypeLabel = new QLabel("Value type");
            valueTypeLabel->setFixedWidth(100);
            valueTypeLayout->addWidget(valueTypeLabel);
            m_valueTypeCombo = new MysticQt::ComboBox();
            m_valueTypeCombo->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            valueTypeLayout->addWidget(m_valueTypeCombo);
            valueTypeLayout->addItem(new QSpacerItem(2, 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
        	connect(m_valueTypeCombo, static_cast<void (MysticQt::ComboBox::*)(int)>(&MysticQt::ComboBox::currentIndexChanged), this, &ParameterCreateEditDialog::OnValueTypeChange);
            mainLayout->addItem(valueTypeLayout);
        }

        // Add the RPE for the parameter
        m_parameterEditorWidget = aznew AzToolsFramework::ReflectedPropertyEditor(this);
        m_parameterEditorWidget->SetAutoResizeLabels(false);
        m_parameterEditorWidget->setSizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::MinimumExpanding);
        m_parameterEditorWidget->SetSizeHintOffset(QSize(0, 0));
        m_parameterEditorWidget->SetLeafIndentation(0);
        m_parameterEditorWidget->setMinimumWidth(m_parameterEditorMinWidth);
        mainLayout->addWidget(m_parameterEditorWidget);

        // Add the preview information
        QHBoxLayout* previewLayout = new QHBoxLayout();
        m_previewFrame = new QFrame(this);
        m_previewFrame->setFrameShape(QFrame::Box);
        m_previewFrame->setObjectName("previewFrame");
        m_previewFrame->setStyleSheet("QFrame#previewFrame { border: 2px dashed #979797; background-color: #85858580; }");
        m_previewFrame->setLayout(new QHBoxLayout());
        QLabel* previewLabel = new QLabel("Preview", m_previewFrame);
        previewLabel->setAutoFillBackground(false);
        previewLabel->setStyleSheet("background: transparent");
        m_previewFrame->layout()->addWidget(previewLabel);
        m_previewWidget = new AzToolsFramework::ReflectedPropertyEditor(m_previewFrame);
        m_previewWidget->SetAutoResizeLabels(false);
        m_previewWidget->SetLeafIndentation(0);
        m_previewWidget->setStyleSheet("QFrame, .QWidget, QSlider, QCheckBox { background-color: transparent }");
        m_previewFrame->layout()->addWidget(m_previewWidget);
        previewLayout->addSpacerItem(new QSpacerItem(100, 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
        previewLayout->addWidget(m_previewFrame);
        mainLayout->addItem(previewLayout);
        mainLayout->addItem(new QSpacerItem(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed));

        // create the create or apply button and the cancel button
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* cancelButton = new QPushButton("Cancel");
        buttonLayout->addWidget(m_createButton);
        buttonLayout->addWidget(cancelButton);
        mainLayout->addItem(buttonLayout);
        connect(m_createButton, &QPushButton::clicked, this, &ParameterCreateEditDialog::OnValidate);
        connect(cancelButton, &QPushButton::clicked, this, &ParameterCreateEditDialog::reject);

        setLayout(mainLayout);
    }

    ParameterCreateEditDialog::~ParameterCreateEditDialog()
    {
        m_previewWidget->ClearInstances();
        if (m_valueParameterEditor)
        {
            delete m_valueParameterEditor;
        }
    }

    void ParameterCreateEditDialog::Init()
    {
        if (m_valueTypeCombo)
        {
            // We're editing a value parameter, allow to change the type to any other value parameter.
            const AZStd::vector<AZ::TypeId> parameterTypes = EMotionFX::ParameterFactory::GetValueParameterTypes();

            m_valueTypeCombo->blockSignals(true);

            // Register all children of EMotionFX::ValueParameter
            for (const AZ::TypeId& parameterType : parameterTypes)
            {
                AZStd::unique_ptr<EMotionFX::Parameter> param(EMotionFX::ParameterFactory::Create(parameterType));
                m_valueTypeCombo->addItem(QString(param->GetTypeDisplayName().c_str()), QString(parameterType.ToString<AZStd::string>().c_str()));
            }

            if (m_parameter)
            {
                m_valueTypeCombo->setCurrentText(m_parameter->GetTypeDisplayName().c_str());
            }

            m_valueTypeCombo->blockSignals(false);
        }

        if (!m_parameter)
        {
            OnValueTypeChange(0);
        }
        else
        {
            InitDynamicInterface(azrtti_typeid(m_parameter.get()));
        }
    }

    // init the dynamic part of the interface
    void ParameterCreateEditDialog::InitDynamicInterface(const AZ::TypeId& valueType)
    {
        if (valueType.IsNull())
        {
            return;
        }

        if (!m_parameter)
        {
            const AZStd::string uniqueParameterName = MCore::GenerateUniqueString("Parameter",
                    [&](const AZStd::string& value)
                    {
                        return (m_plugin->GetActiveAnimGraph()->FindParameterByName(value) == nullptr);
                    });
            m_parameter.reset(EMotionFX::ParameterFactory::Create(valueType));
            m_parameter->SetName(uniqueParameterName);
        }
        else
        {
            if (azrtti_typeid(m_parameter.get()) != valueType) // value type changed
            {
                const AZStd::string oldParameterName = m_parameter->GetName();
                const AZStd::string oldParameterDescription = m_parameter->GetDescription();
                m_parameter.reset(EMotionFX::ParameterFactory::Create(valueType));
                m_parameter->SetName(oldParameterName);
                m_parameter->SetDescription(oldParameterDescription);
            }
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return;
        }

        m_parameterEditorWidget->ClearInstances();
        m_parameterEditorWidget->AddInstance(m_parameter.get(), azrtti_typeid(m_parameter.get()));
        m_parameterEditorWidget->Setup(serializeContext, this, false, 100);
        m_parameterEditorWidget->show();
        m_parameterEditorWidget->ExpandAll();
        m_parameterEditorWidget->InvalidateAll();

        m_previewWidget->ClearInstances();
        if (m_valueParameterEditor)
        {
            delete m_valueParameterEditor;
            m_valueParameterEditor = nullptr;
        }

        if (azrtti_typeid(m_parameter.get()) == azrtti_typeid<EMotionFX::GroupParameter>())
        {
            m_previewFrame->setVisible(false);
        }
        else
        {
            m_valueParameterEditor = ParameterEditorFactory::Create(nullptr, static_cast<const EMotionFX::ValueParameter*>(m_parameter.get()), AZStd::vector<MCore::Attribute*>());
            m_previewWidget->AddInstance(m_valueParameterEditor, azrtti_typeid(m_valueParameterEditor));
            m_previewWidget->Setup(serializeContext, nullptr, false, 0);
            m_previewWidget->show();
            m_previewWidget->ExpandAll();
            m_previewWidget->InvalidateAll();
            m_previewFrame->setVisible(true);
        }

        adjustSize();
    }


    // when the value type changes
    void ParameterCreateEditDialog::OnValueTypeChange(int valueType)
    {
        if (m_valueTypeCombo && valueType != -1)
        {
            QVariant variantData = m_valueTypeCombo->itemData(valueType);
            AZ_Assert(variantData != QVariant::Invalid, "Expected valid variant data");
            const AZStd::string typeId = FromQtString(variantData.toString());
            InitDynamicInterface(AZ::TypeId::CreateString(typeId.c_str(), typeId.size()));
        }
    }


    // check if we can create this parameter
    void ParameterCreateEditDialog::OnValidate()
    {
        EMotionFX::AnimGraph* animGraph = m_plugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            MCore::LogWarning("ParameterCreateEditDialog::OnValidate() - No AnimGraph active!");
            return;
        }

        if (m_parameter->GetName().empty())
        {
            QMessageBox::warning(this, "Please Provide A Parameter Name", "The parameter name cannot be empty!");
            return;
        }

        // check if the name already exists
        if ((m_createButton->text() == "Create" && animGraph->FindParameterByName(m_parameter->GetName()))
            || (m_parameter->GetName() != m_originalName && animGraph->FindParameterByName(m_parameter->GetName())))
        {
            const AZStd::string errorString = AZStd::string::format("Parameter with name '<b>%s</b>' already exists in anim graph '<b>%s</b>'.<br><br><i>Please use a unique parameter name.</i>",
                    m_parameter->GetName().c_str(), animGraph->GetFileName());
            QMessageBox::warning(this, "Parameter name is not unique", errorString.c_str());
            return;
        }

        emit accept();
    }

    void ParameterCreateEditDialog::AfterPropertyModified(AzToolsFramework::InstanceDataNode*)
    {
        m_previewWidget->InvalidateAttributesAndValues();
        adjustSize();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterCreateEditDialog.moc>
