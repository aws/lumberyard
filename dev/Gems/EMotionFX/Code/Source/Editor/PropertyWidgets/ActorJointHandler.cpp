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

#include <Editor/PropertyWidgets/ActorJointHandler.h>
#include <Editor/ActorEditorBus.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/AnimGraphEditorBus.h>
#include <Editor/JointSelectionDialog.h>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorJointPicker, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(ActorSingleJointHandler, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(ActorMultiJointHandler, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(ActorMultiWeightedJointHandler, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(ActorJointElementWidget, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(ActorJointElementHandler, AZ::SystemAllocator, 0)

    ActorJointPicker::ActorJointPicker(bool singleSelection, const char* dialogTitle, const char* dialogDescriptionLabelText, QWidget* parent)
        : QWidget(parent)
        , m_dialogTitle(dialogTitle)
        , m_dialogDescriptionLabelText(dialogDescriptionLabelText)
    {
        m_singleSelection = singleSelection;

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &ActorJointPicker::OnPickClicked);
        hLayout->addWidget(m_pickButton);

        m_resetButton = new QPushButton(this);
        EMStudio::EMStudioManager::MakeTransparentButton(m_resetButton, "/Images/Icons/Clear.png", "Reset selection");
        connect(m_resetButton, &QPushButton::clicked, this, &ActorJointPicker::OnResetClicked);
        hLayout->addWidget(m_resetButton);

        setLayout(hLayout);
    }

    void ActorJointPicker::AddDefaultFilter(const QString& category, const QString& displayName)
    {
        m_defaultFilters.emplace_back(category, displayName);
    }

    void ActorJointPicker::OnResetClicked()
    {
        if (m_weightedJointNames.empty())
        {
            return;
        }

        SetWeightedJointNames(AZStd::vector<AZStd::pair<AZStd::string, float> >());
        emit SelectionChanged();
    }

    void ActorJointPicker::SetJointName(const AZStd::string& jointName)
    {
        AZStd::vector<AZStd::pair<AZStd::string, float> > weightedJointNames;

        if (!jointName.empty())
        {
            weightedJointNames.emplace_back(jointName, 0.0f);
        }

        SetWeightedJointNames(weightedJointNames);
    }

    AZStd::string ActorJointPicker::GetJointName() const
    {
        if (m_weightedJointNames.empty())
        {
            return AZStd::string();
        }

        return m_weightedJointNames[0].first;
    }

    void ActorJointPicker::SetJointNames(const AZStd::vector<AZStd::string>& jointNames)
    {
        AZStd::vector<AZStd::pair<AZStd::string, float> > weightedJointNames;

        const size_t numJointNames = jointNames.size();
        weightedJointNames.resize(numJointNames);

        for (size_t i = 0; i < numJointNames; ++i)
        {
            weightedJointNames[i] = AZStd::make_pair<AZStd::string, float>(jointNames[i], 0.0f);
        }

        SetWeightedJointNames(weightedJointNames);
    }

    AZStd::vector<AZStd::string> ActorJointPicker::GetJointNames() const
    {
        AZStd::vector<AZStd::string> result;

        const size_t numJoints = m_weightedJointNames.size();
        result.resize(numJoints);

        for (size_t i = 0; i < numJoints; ++i)
        {
            result[i] = m_weightedJointNames[i].first;
        }

        return result;
    }

    void ActorJointPicker::UpdateInterface()
    {
        // Set the picker button name based on the number of joints.
        const size_t numJoints = m_weightedJointNames.size();
        if (numJoints == 0)
        {
            if (m_singleSelection)
            {
                m_pickButton->setText("Select joint");
            }
            else
            {
                m_pickButton->setText("Select joints");
            }

            m_resetButton->setVisible(false);
        }
        else if (numJoints == 1)
        {
            m_pickButton->setText(m_weightedJointNames[0].first.c_str());
            m_resetButton->setVisible(true);
        }
        else
        {
            m_pickButton->setText(QString("%1 joints").arg(numJoints));
            m_resetButton->setVisible(true);
        }

        // Build and set the tooltip containing all joints.
        QString tooltip;
        for (size_t i = 0; i < numJoints; ++i)
        {
            tooltip += m_weightedJointNames[i].first.c_str();
            if (i != numJoints - 1)
            {
                tooltip += "\n";
            }
        }
        m_pickButton->setToolTip(tooltip);
    }

    void ActorJointPicker::SetWeightedJointNames(const AZStd::vector<AZStd::pair<AZStd::string, float> >& weightedJointNames)
    {
        m_weightedJointNames = weightedJointNames;
        UpdateInterface();
    }

    AZStd::vector<AZStd::pair<AZStd::string, float> > ActorJointPicker::GetWeightedJointNames() const
    {
        return m_weightedJointNames;
    }

    void ActorJointPicker::OnPickClicked()
    {
        EMotionFX::ActorInstance* actorInstance = nullptr;
        ActorEditorRequestBus::BroadcastResult(actorInstance, &ActorEditorRequests::GetSelectedActorInstance);
        if (!actorInstance)
        {
            QMessageBox::warning(this, "No Actor Instance", "Cannot open joint selection window. No valid actor instance selected.");
            return;
        }
        EMotionFX::Actor* actor = actorInstance->GetActor();

        // Create and show the joint picker window
        JointSelectionDialog jointSelectionWindow(m_singleSelection, m_dialogTitle.c_str(), m_dialogDescriptionLabelText.c_str(), this);

        for (const auto& filterPair : m_defaultFilters)
        {
            jointSelectionWindow.SetFilterState(filterPair.first, filterPair.second, true);
        }

        jointSelectionWindow.HideIcons();
        jointSelectionWindow.SelectByJointNames(GetJointNames());
        jointSelectionWindow.setModal(true);

        if (jointSelectionWindow.exec() != QDialog::Rejected)
        {
            SetJointNames(jointSelectionWindow.GetSelectedJointNames());
            emit SelectionChanged();
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    ActorJointElementWidget::ActorJointElementWidget(QWidget* parent)
        : QLineEdit(parent)
    {
        setReadOnly(true);
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 ActorJointElementHandler::GetHandlerName() const
    {
        return AZ_CRC("ActorJointElement", 0xedc8946c);
    }

    QWidget* ActorJointElementHandler::CreateGUI(QWidget* parent)
    {
        ActorJointElementWidget* widget = aznew ActorJointElementWidget(parent);
        return widget;
    }

    void ActorJointElementHandler::ConsumeAttribute(ActorJointElementWidget* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }

    void ActorJointElementHandler::WriteGUIValuesIntoProperty(size_t index, ActorJointElementWidget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->text().toUtf8().data();
    }

    bool ActorJointElementHandler::ReadValuesIntoGUI(size_t index, ActorJointElementWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->setText(instance.c_str());
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 ActorSingleJointHandler::GetHandlerName() const
    {
        return AZ_CRC("ActorNode", 0x35d9eb50);
    }

    QWidget* ActorSingleJointHandler::CreateGUI(QWidget* parent)
    {
        ActorJointPicker* picker = aznew ActorJointPicker(true /*singleSelection*/, "Select Joint", "Select or double-click a joint", parent);

        connect(picker, &ActorJointPicker::SelectionChanged, this, [picker]()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
            });

        return picker;
    }

    void ActorSingleJointHandler::ConsumeAttribute(ActorJointPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }

    void ActorSingleJointHandler::WriteGUIValuesIntoProperty(size_t index, ActorJointPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetJointName();
    }

    bool ActorSingleJointHandler::ReadValuesIntoGUI(size_t index, ActorJointPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetJointName(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 ActorMultiJointHandler::GetHandlerName() const
    {
        return AZ_CRC("ActorNodes", 0x70504714);
    }

    QWidget* ActorMultiJointHandler::CreateGUI(QWidget* parent)
    {
        ActorJointPicker* picker = aznew ActorJointPicker(false /*singleSelection*/, "Select Joints", "Select one or more joints from the skeleton", parent);

        connect(picker, &ActorJointPicker::SelectionChanged, this, [picker]()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
            });

        return picker;
    }

    void ActorMultiJointHandler::ConsumeAttribute(ActorJointPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }

    void ActorMultiJointHandler::WriteGUIValuesIntoProperty(size_t index, ActorJointPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetJointNames();
    }

    bool ActorMultiJointHandler::ReadValuesIntoGUI(size_t index, ActorJointPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetJointNames(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 ActorMultiWeightedJointHandler::GetHandlerName() const
    {
        return AZ_CRC("ActorWeightedNodes", 0x689c0537);
    }

    QWidget* ActorMultiWeightedJointHandler::CreateGUI(QWidget* parent)
    {
        ActorJointPicker* picker = aznew ActorJointPicker(false /*singleSelection*/, "Joint Selection Dialog", "Select one or more joints from the skeleton", parent);

        connect(picker, &ActorJointPicker::SelectionChanged, this, [picker]()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
            });

        return picker;
    }

    void ActorMultiWeightedJointHandler::ConsumeAttribute(ActorJointPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }

    void ActorMultiWeightedJointHandler::WriteGUIValuesIntoProperty(size_t index, ActorJointPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetWeightedJointNames();
    }

    bool ActorMultiWeightedJointHandler::ReadValuesIntoGUI(size_t index, ActorJointPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetWeightedJointNames(instance);
        return true;
    }
} // namespace EMotionFX