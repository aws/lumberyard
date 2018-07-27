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

#include <Editor/PropertyWidgets/AnimGraphParameterHandler.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterSelectionWindow.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <QHBoxLayout>
#include <QMessageBox>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphParameterPicker, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphSingleParameterHandler, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMultipleParameterHandler, AZ::SystemAllocator, 0);

    AnimGraphParameterPicker::AnimGraphParameterPicker(QWidget* parent, bool singleSelection, bool parameterMaskMode)
        : QWidget(parent)
        , m_parameterMaskMode(parameterMaskMode)
        , m_animGraph(nullptr)
        , m_parameterNode(nullptr)
        , m_pickButton(nullptr)
        , m_resetButton(nullptr)
        , m_shrinkButton(nullptr)
        , m_singleSelection(singleSelection)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &AnimGraphParameterPicker::OnPickClicked);
        hLayout->addWidget(m_pickButton);

        m_resetButton = new QPushButton(this);
        EMStudio::EMStudioManager::MakeTransparentButton(m_resetButton, "/Images/Icons/Clear.png", "Reset selection");
        connect(m_resetButton, &QPushButton::clicked, this, &AnimGraphParameterPicker::OnResetClicked);
        hLayout->addWidget(m_resetButton);

        if (m_parameterMaskMode)
        {
            m_shrinkButton = new QPushButton();
            EMStudio::EMStudioManager::MakeTransparentButton(m_shrinkButton, "/Images/Icons/Cut.png", "Shrink the parameter mask to the ports that are actually connected.");
            connect(m_shrinkButton, &QPushButton::clicked, this, &AnimGraphParameterPicker::OnShrinkClicked);
            hLayout->addWidget(m_shrinkButton);
        }

        setLayout(hLayout);
    }


    void AnimGraphParameterPicker::SetAnimGraph(AnimGraph* animGraph)
    {
        m_animGraph = animGraph;
    }


    void AnimGraphParameterPicker::SetParameterNode(BlendTreeParameterNode* parameterNode)
    {
        m_parameterNode = parameterNode;

        if (m_parameterNode)
        {
            m_animGraph = m_parameterNode->GetAnimGraph();
        }
    }


    void AnimGraphParameterPicker::OnResetClicked()
    {
        if (m_parameterNames.empty())
        {
            return;
        }

        SetParameterNames(AZStd::vector<AZStd::string>());
        emit ParametersChanged();
    }


    void AnimGraphParameterPicker::OnShrinkClicked()
    {
        if (!m_parameterNode)
        {
            AZ_Error("EMotionFX", false, "Cannot shrink parameter mask. No valid parameter node.");
            return;
        }

        AZStd::vector<AZStd::string> connectedParameterNames;
        m_parameterNode->CalcConnectedParameterNames(connectedParameterNames);

        SetParameterNames(connectedParameterNames);
        emit ParametersChanged();
    }


    void AnimGraphParameterPicker::UpdateInterface()
    {
        // Set the picker button name based on the number of nodes.
        const size_t numParameters = m_parameterNames.size();
        if (numParameters == 0)
        {
            if (m_singleSelection)
            {
                m_pickButton->setText("Select parameter");
            }
            else
            {
                m_pickButton->setText("Select parameters");
            }

            m_resetButton->setVisible(false);
        }
        else if (numParameters == 1)
        {
            m_pickButton->setText(m_parameterNames[0].c_str());
            m_resetButton->setVisible(true);
        }
        else
        {
            m_pickButton->setText(QString("%1 parameters").arg(numParameters));
            m_resetButton->setVisible(true);
        }

        // Build and set the tooltip containing all nodes.
        QString tooltip;
        for (size_t i = 0; i < numParameters; ++i)
        {
            tooltip += m_parameterNames[i].c_str();
            if (i != numParameters - 1)
            {
                tooltip += "\n";
            }
        }
        m_pickButton->setToolTip(tooltip);
    }


    void AnimGraphParameterPicker::SetParameterNames(const AZStd::vector<AZStd::string>& parameterNames)
    {
        m_parameterNames = parameterNames;
        UpdateInterface();
    }


    const AZStd::vector<AZStd::string>& AnimGraphParameterPicker::GetParameterNames() const
    {
        return m_parameterNames;
    }


    void AnimGraphParameterPicker::SetSingleParameterName(const AZStd::string& parameterName)
    {
        AZStd::vector<AZStd::string> parameterNames;

        if (!parameterName.empty())
        {
            parameterNames.emplace_back(parameterName);   
        }

        SetParameterNames(parameterNames);
    }


    const AZStd::string AnimGraphParameterPicker::GetSingleParameterName() const
    {
        if (m_parameterNames.empty())
        {
            return AZStd::string();
        }

        return m_parameterNames[0];
    }


    void AnimGraphParameterPicker::OnPickClicked()
    {
        if (!m_animGraph)
        {
            AZ_Error("EMotionFX", false, "Cannot open anim graph parameter selection window. No valid anim graph.");
            return;
        }

        // Create and show the node picker window
        EMStudio::ParameterSelectionWindow selectionWindow(this, m_singleSelection);
        selectionWindow.Update(m_animGraph, m_parameterNames);
        selectionWindow.setModal(true);

        if (selectionWindow.exec() != QDialog::Rejected)
        {
            if (m_parameterMaskMode)
            {
                // Add all parameter names that are connected (essential parameters). If we don't do this, we are forced to remove connections automatically which might be overseen by the user.
                m_parameterNode->CalcConnectedParameterNames(m_parameterNames);

                // Add the selected parameters on top of it.
                const AZStd::vector<AZStd::string>& selectedParameterNames = selectionWindow.GetParameterWidget()->GetSelectedParameters();
                for (const AZStd::string& selectedParameterName : selectedParameterNames)
                {
                    if (AZStd::find(m_parameterNames.begin(), m_parameterNames.end(), selectedParameterName) == m_parameterNames.end())
                    {
                        m_parameterNames.emplace_back(selectedParameterName);
                    }
                }
            }
            else
            {
                m_parameterNames = selectionWindow.GetParameterWidget()->GetSelectedParameters();
            }

            UpdateInterface();
            emit ParametersChanged();
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphSingleParameterHandler::AnimGraphSingleParameterHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::string, AnimGraphParameterPicker>()
        , m_animGraph(nullptr)
    {
    }

    AZ::u32 AnimGraphSingleParameterHandler::GetHandlerName() const
    {
        return AZ_CRC("AnimGraphParameter", 0x778af55a);
    }


    QWidget* AnimGraphSingleParameterHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphParameterPicker* picker = aznew AnimGraphParameterPicker(parent, true);

        connect(picker, &AnimGraphParameterPicker::ParametersChanged, this, [picker]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
        });

        return picker;
    }


    void AnimGraphSingleParameterHandler::ConsumeAttribute(AnimGraphParameterPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }

        if (attrib == AZ_CRC("AnimGraph", 0x0d53d4b3))
        {
            attrValue->Read<AnimGraph*>(m_animGraph);
            GUI->SetAnimGraph(m_animGraph);
        }
    }


    void AnimGraphSingleParameterHandler::WriteGUIValuesIntoProperty(size_t index, AnimGraphParameterPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetSingleParameterName();
    }


    bool AnimGraphSingleParameterHandler::ReadValuesIntoGUI(size_t index, AnimGraphParameterPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetSingleParameterName(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphMultipleParameterHandler::AnimGraphMultipleParameterHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, AnimGraphParameterPicker>()
        , m_animGraph(nullptr)
    {
    }

    AZ::u32 AnimGraphMultipleParameterHandler::GetHandlerName() const
    {
        return AZ_CRC("AnimGraphMultipleParameter", 0x4d5e082c);
    }


    QWidget* AnimGraphMultipleParameterHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphParameterPicker* picker = aznew AnimGraphParameterPicker(parent, false);

        connect(picker, &AnimGraphParameterPicker::ParametersChanged, this, [picker]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
        });

        return picker;
    }


    void AnimGraphMultipleParameterHandler::ConsumeAttribute(AnimGraphParameterPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }

        if (attrib == AZ_CRC("AnimGraph", 0x0d53d4b3))
        {
            attrValue->Read<AnimGraph*>(m_animGraph);
            GUI->SetAnimGraph(m_animGraph);
        }
    }


    void AnimGraphMultipleParameterHandler::WriteGUIValuesIntoProperty(size_t index, AnimGraphParameterPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetParameterNames();
    }


    bool AnimGraphMultipleParameterHandler::ReadValuesIntoGUI(size_t index, AnimGraphParameterPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetParameterNames(instance);
        return true;
    }

} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/AnimGraphParameterHandler.moc>