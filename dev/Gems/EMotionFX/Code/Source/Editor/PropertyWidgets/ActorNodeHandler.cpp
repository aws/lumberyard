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

#include <Editor/PropertyWidgets/ActorNodeHandler.h>
#include <Editor/ActorEditorBus.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/NodeSelectionWindow.h>
#include <Editor/AnimGraphEditorBus.h>
#include <QHBoxLayout>
#include <QMessageBox>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorNodePicker, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(ActorSingleNodeHandler, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(ActorMultiNodeHandler, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(ActorMultiWeightedNodeHandler, AZ::SystemAllocator, 0)

    ActorNodePicker::ActorNodePicker(QWidget* parent, bool singleSelection)
        : QWidget(parent)
    {
        m_singleSelection = singleSelection;

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &ActorNodePicker::OnPickClicked);
        hLayout->addWidget(m_pickButton);

        m_resetButton = new QPushButton(this);
        EMStudio::EMStudioManager::MakeTransparentButton(m_resetButton, "/Images/Icons/Clear.png", "Reset selection");
        connect(m_resetButton, &QPushButton::clicked, this, &ActorNodePicker::OnResetClicked);
        hLayout->addWidget(m_resetButton);

        setLayout(hLayout);
    }


    void ActorNodePicker::OnResetClicked()
    {
        if (m_weightedNodeNames.empty())
        {
            return;
        }

        SetWeightedNodeNames(AZStd::vector<AZStd::pair<AZStd::string, float>>());
        emit SelectionChanged();
    }


    void ActorNodePicker::SetNodeName(const AZStd::string& nodeName)
    {
        AZStd::vector<AZStd::pair<AZStd::string, float>> weightedNodeNames;

        if (!nodeName.empty())
        {
            weightedNodeNames.emplace_back(nodeName, 0.0f);
        }

        SetWeightedNodeNames(weightedNodeNames);
    }


    AZStd::string ActorNodePicker::GetNodeName() const
    {
        if (m_weightedNodeNames.empty())
        {
            return AZStd::string();
        }

        return m_weightedNodeNames[0].first;
    }


    void ActorNodePicker::SetNodeNames(const AZStd::vector<AZStd::string>& nodeNames)
    {
        AZStd::vector<AZStd::pair<AZStd::string, float>> weightedNodeNames;

        const size_t numNodeNames = nodeNames.size();
        weightedNodeNames.resize(numNodeNames);

        for (size_t i = 0; i < numNodeNames; ++i)
        {
            weightedNodeNames[i] = AZStd::make_pair<AZStd::string, float>(nodeNames[i], 0.0f);
        }

        SetWeightedNodeNames(weightedNodeNames);
    }


    AZStd::vector<AZStd::string> ActorNodePicker::GetNodeNames() const
    {
        AZStd::vector<AZStd::string> result;

        const size_t numNodes = m_weightedNodeNames.size();
        result.resize(numNodes);

        for (size_t i = 0; i < numNodes; ++i)
        {
            result[i] = m_weightedNodeNames[i].first;
        }

        return result;
    }


    void ActorNodePicker::UpdateInterface()
    {
        // Set the picker button name based on the number of nodes.
        const size_t numNodes = m_weightedNodeNames.size();
        if (numNodes == 0)
        {
            if (m_singleSelection)
            {
                m_pickButton->setText("Select node");
            }
            else
            {
                m_pickButton->setText("Select nodes");
            }

            m_resetButton->setVisible(false);
        }
        else if (numNodes == 1)
        {
            m_pickButton->setText(m_weightedNodeNames[0].first.c_str());
            m_resetButton->setVisible(true);
        }
        else
        {
            m_pickButton->setText(QString("%1 nodes").arg(numNodes));
            m_resetButton->setVisible(true);
        }

        // Build and set the tooltip containing all nodes.
        QString tooltip;
        for (size_t i = 0; i < numNodes; ++i)
        {
            tooltip += m_weightedNodeNames[i].first.c_str();
            if (i != numNodes - 1)
            {
                tooltip += "\n";
            }
        }
        m_pickButton->setToolTip(tooltip);
    }


    void ActorNodePicker::SetWeightedNodeNames(const AZStd::vector<AZStd::pair<AZStd::string, float>>& weightedNodeNames)
    {
        m_weightedNodeNames = weightedNodeNames;
        UpdateInterface();
    }


    AZStd::vector<AZStd::pair<AZStd::string, float>> ActorNodePicker::GetWeightedNodeNames() const
    {
        return m_weightedNodeNames;
    }


    void ActorNodePicker::OnPickClicked()
    {
        EMotionFX::ActorInstance* actorInstance = nullptr;
        ActorEditorRequestBus::BroadcastResult(actorInstance, &ActorEditorRequests::GetSelectedActorInstance);
        if (!actorInstance)
        {
            QMessageBox::warning(this, "No Actor Instance", "Cannot open node selection window. No valid actor instance selected.");
            return;
        }
        EMotionFX::Actor* actor = actorInstance->GetActor();

        // Create and show the node picker window
        EMStudio::NodeSelectionWindow nodeSelectionWindow(this, m_singleSelection);
        nodeSelectionWindow.GetNodeHierarchyWidget()->SetSelectionMode(m_singleSelection);

        CommandSystem::SelectionList prevSelection;
        const size_t numNodes = m_weightedNodeNames.size();
        for (const AZStd::pair<AZStd::string, float>& weightedNode : m_weightedNodeNames)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(weightedNode.first.c_str());
            if (node)
            {
                prevSelection.AddNode(node);
            }
        }
        
        nodeSelectionWindow.Update(actorInstance->GetID(), &prevSelection);
        nodeSelectionWindow.setModal(true);


        if (nodeSelectionWindow.exec() != QDialog::Rejected)
        {
            AZStd::vector<SelectionItem>& newSelection = nodeSelectionWindow.GetNodeHierarchyWidget()->GetSelectedItems();

            const size_t numSelectedNodes = newSelection.size();
            m_weightedNodeNames.resize(numSelectedNodes);

            for (size_t i = 0; i < numSelectedNodes; ++i)
            {
                m_weightedNodeNames[i] = AZStd::make_pair<AZStd::string, float>(newSelection[i].GetNodeName(), 0.0f);
            }

            UpdateInterface();
            emit SelectionChanged();
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 ActorSingleNodeHandler::GetHandlerName() const
    {
        return AZ_CRC("ActorNode", 0x35d9eb50);
    }


    QWidget* ActorSingleNodeHandler::CreateGUI(QWidget* parent)
    {
        const bool singleSelection = true;
        ActorNodePicker* picker = aznew ActorNodePicker(parent, singleSelection);

        connect(picker, &ActorNodePicker::SelectionChanged, this, [picker]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
        });

        return picker;
    }


    void ActorSingleNodeHandler::ConsumeAttribute(ActorNodePicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
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


    void ActorSingleNodeHandler::WriteGUIValuesIntoProperty(size_t index, ActorNodePicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetNodeName();
    }


    bool ActorSingleNodeHandler::ReadValuesIntoGUI(size_t index, ActorNodePicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetNodeName(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 ActorMultiNodeHandler::GetHandlerName() const
    {
        return AZ_CRC("ActorNodes", 0x70504714);
    }


    QWidget* ActorMultiNodeHandler::CreateGUI(QWidget* parent)
    {
        ActorNodePicker* picker = aznew ActorNodePicker(parent);

        connect(picker, &ActorNodePicker::SelectionChanged, this, [picker]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
        });

        return picker;
    }


    void ActorMultiNodeHandler::ConsumeAttribute(ActorNodePicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
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


    void ActorMultiNodeHandler::WriteGUIValuesIntoProperty(size_t index, ActorNodePicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetNodeNames();
    }


    bool ActorMultiNodeHandler::ReadValuesIntoGUI(size_t index, ActorNodePicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetNodeNames(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 ActorMultiWeightedNodeHandler::GetHandlerName() const
    {
        return AZ_CRC("ActorWeightedNodes", 0x689c0537);
    }


    QWidget* ActorMultiWeightedNodeHandler::CreateGUI(QWidget* parent)
    {
        ActorNodePicker* picker = aznew ActorNodePicker(parent);

        connect(picker, &ActorNodePicker::SelectionChanged, this, [picker]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
        });

        return picker;
    }


    void ActorMultiWeightedNodeHandler::ConsumeAttribute(ActorNodePicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
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


    void ActorMultiWeightedNodeHandler::WriteGUIValuesIntoProperty(size_t index, ActorNodePicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetWeightedNodeNames();
    }


    bool ActorMultiWeightedNodeHandler::ReadValuesIntoGUI(size_t index, ActorNodePicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetWeightedNodeNames(instance);
        return true;
    }

} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/ActorNodeHandler.moc>