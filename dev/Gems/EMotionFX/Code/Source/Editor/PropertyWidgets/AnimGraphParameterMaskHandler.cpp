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

#include <EMotionFX/Source/EventHandler.h>
#include <Editor/PropertyWidgets/AnimGraphParameterMaskHandler.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterSelectionWindow.h>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTimer>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphParameterMaskHandler, AZ::SystemAllocator, 0)

    AnimGraphParameterMaskHandler::AnimGraphParameterMaskHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, AnimGraphParameterPicker>()
        , m_parameterNode(nullptr)
    {
    }


    AZ::u32 AnimGraphParameterMaskHandler::GetHandlerName() const
    {
        return AZ_CRC("AnimGraphParameterMask", 0x67dd0993);
    }


    QWidget* AnimGraphParameterMaskHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphParameterPicker* picker = aznew AnimGraphParameterPicker(parent, false, true);

        connect(picker, &AnimGraphParameterPicker::ParametersChanged, this, [picker]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
        });

        return picker;
    }


    void AnimGraphParameterMaskHandler::ConsumeAttribute(AnimGraphParameterPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrValue)
        {
            m_parameterNode = static_cast<BlendTreeParameterNode*>(attrValue->GetInstancePointer());
            GUI->SetParameterNode(m_parameterNode);
        }

        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }


    void AnimGraphParameterMaskHandler::FireParameterMaskChangedEvent()
    {
        if (m_parameterNode)
        {
            GetEventManager().OnParameterNodeMaskChanged(m_parameterNode, m_temp);
        }
    }


    void AnimGraphParameterMaskHandler::WriteGUIValuesIntoProperty(size_t index, AnimGraphParameterPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        m_temp = GUI->GetParameterNames();
        EMotionFX::BlendTreeParameterNode::SortParameterNames(m_parameterNode->GetAnimGraph(), m_temp);

        // Don't update the parameter names yet, we still need the information for constructing the command group.
        instance = m_parameterNode->GetParameters();

        QTimer::singleShot(100, this, SLOT(FireParameterMaskChangedEvent()));
    }


    bool AnimGraphParameterMaskHandler::ReadValuesIntoGUI(size_t index, AnimGraphParameterPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetParameterNames(instance);
        return true;
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/AnimGraphParameterMaskHandler.moc>