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

#include <Editor/PropertyWidgets/MotionSetMotionIdHandler.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MotionSetSelectionWindow.h>
#include <Editor/AnimGraphEditorBus.h>
#include <QHBoxLayout>
#include <QMessageBox>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionSetMotionIdPicker, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(MotionSetMultiMotionIdHandler, AZ::SystemAllocator, 0)

    MotionSetMotionIdPicker::MotionSetMotionIdPicker(QWidget* parent)
        : QWidget(parent)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &MotionSetMotionIdPicker::OnPickClicked);
        hLayout->addWidget(m_pickButton);

        setLayout(hLayout);
    }


    void MotionSetMotionIdPicker::UpdateInterface()
    {
        // Set the picker button name based on the number of motions.
        const size_t numMotions = m_motionIds.size();
        if (numMotions == 0)
        {
            m_pickButton->setText("Select motions");
        }
        else if (numMotions == 1)
        {
            m_pickButton->setText(m_motionIds[0].c_str());
        }
        else
        {
            m_pickButton->setText(QString("%1 motions").arg(numMotions));
        }

        // Build and set the tooltip containing all motion ids.
        QString tooltip;
        for (size_t i = 0; i < numMotions; ++i)
        {
            tooltip += m_motionIds[i].c_str();
            if (i != numMotions - 1)
            {
                tooltip += "\n";
            }
        }
        m_pickButton->setToolTip(tooltip);
    }


    void MotionSetMotionIdPicker::SetMotionIds(const AZStd::vector<AZStd::string>& motionIds)
    {
        m_motionIds = motionIds;
        UpdateInterface();
    }


    AZStd::vector<AZStd::string> MotionSetMotionIdPicker::GetMotionIds() const
    {
        return m_motionIds;
    }


    void MotionSetMotionIdPicker::OnPickClicked()
    {
        EMotionFX::MotionSet* motionSet = nullptr;
        AnimGraphEditorRequestBus::BroadcastResult(motionSet, &AnimGraphEditorRequests::GetSelectedMotionSet);
        if (!motionSet)
        {
            QMessageBox::warning(this, "No Motion Set", "Cannot open motion selection window. No valid motion set selected.");
            return;
        }

        // Create and show the motion picker window
        EMStudio::MotionSetSelectionWindow motionPickWindow(this);
        motionPickWindow.GetHierarchyWidget()->SetSelectionMode(false);
        motionPickWindow.Update(motionSet);
        motionPickWindow.setModal(true);
        motionPickWindow.Select(m_motionIds, motionSet);

        if (motionPickWindow.exec() == QDialog::Rejected)   // we pressed cancel or the close cross
        {
            return;
        }

        m_motionIds = motionPickWindow.GetHierarchyWidget()->GetSelectedMotionIds(motionSet);
        UpdateInterface();
        emit SelectionChanged();
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 MotionSetMultiMotionIdHandler::GetHandlerName() const
    {
        return AZ_CRC("MotionSetMotionIds", 0x8695c0fa);
    }


    QWidget* MotionSetMultiMotionIdHandler::CreateGUI(QWidget* parent)
    {
        MotionSetMotionIdPicker* picker = aznew MotionSetMotionIdPicker(parent);

        connect(picker, &MotionSetMotionIdPicker::SelectionChanged, this, [picker]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
        });

        return picker;
    }


    void MotionSetMultiMotionIdHandler::ConsumeAttribute(MotionSetMotionIdPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
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


    void MotionSetMultiMotionIdHandler::WriteGUIValuesIntoProperty(size_t index, MotionSetMotionIdPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetMotionIds();
    }


    bool MotionSetMultiMotionIdHandler::ReadValuesIntoGUI(size_t index, MotionSetMotionIdPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetMotionIds(instance);
        return true;
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/MotionSetMotionIdHandler.moc>