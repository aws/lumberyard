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

#include <AzCore/Memory/SystemAllocator.h>
#include <Editor/ActorJointBrowseEdit.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/NodeSelectionWindow.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <QTreeWidget>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorJointBrowseEdit, AZ::SystemAllocator, 0)

    ActorJointBrowseEdit::ActorJointBrowseEdit(QWidget* parent)
        : AzQtComponents::BrowseEdit(parent)
    {
        connect(this, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &ActorJointBrowseEdit::OnBrowseButtonClicked);
        connect(this, &AzQtComponents::BrowseEdit::textEdited, this, &ActorJointBrowseEdit::OnTextEdited);

        SetSingleJointSelection(m_singleJointSelection);
        setClearButtonEnabled(true);
        setLineEditReadOnly(true);
    }

    void ActorJointBrowseEdit::SetSingleJointSelection(bool singleSelectionEnabled)
    {
        m_singleJointSelection = singleSelectionEnabled;
        UpdatePlaceholderText();
    }

    void ActorJointBrowseEdit::UpdatePlaceholderText()
    {
        const QString placeholderText = QString("Select joint%1").arg(m_singleJointSelection ? "" : "s");
        setPlaceholderText(placeholderText);
    }

    void ActorJointBrowseEdit::OnBrowseButtonClicked()
    {
        const EMotionFX::ActorInstance* actorInstance = GetActorInstance();
        if (!actorInstance)
        {
            AZ_Warning("EMotionFX", false, "Cannot open joint selection window. Please select an actor instance first.");
            return;
        }
        const EMotionFX::Actor* actor = actorInstance->GetActor();

        CommandSystem::SelectionList selectionList;
        for (const SelectionItem& selectedJoint : m_selectedJoints)
        {
            selectionList.AddNode(selectedJoint.GetNode());
        }
        AZ_Warning("EMotionFX", m_singleJointSelection && m_selectedJoints.size() > 1,
            "Single selection actor joint window has multiple pre-selected joints.");

        m_previouslySelectedJoints = m_selectedJoints;

        m_jointSelectionWindow = new NodeSelectionWindow(this, m_singleJointSelection);
        connect(m_jointSelectionWindow->GetNodeHierarchyWidget(), qOverload<MCore::Array<SelectionItem>>(&NodeHierarchyWidget::OnSelectionDone), this, &ActorJointBrowseEdit::OnSelectionDoneMCoreArray);
        connect(m_jointSelectionWindow, &NodeSelectionWindow::rejected, this, &ActorJointBrowseEdit::OnSelectionRejected);
        connect(m_jointSelectionWindow->GetNodeHierarchyWidget()->GetTreeWidget(), &QTreeWidget::itemSelectionChanged, this, &ActorJointBrowseEdit::OnSelectionChanged);

        NodeSelectionWindow::connect(m_jointSelectionWindow, &QDialog::finished, [=](int resultCode)
            {
                m_jointSelectionWindow->deleteLater();
                m_jointSelectionWindow = nullptr;
            });

        m_jointSelectionWindow->open();
        m_jointSelectionWindow->Update(actorInstance->GetID(), &selectionList);
    }

    void ActorJointBrowseEdit::SetSelectedJoints(const AZStd::vector<SelectionItem>& selectedJoints)
    {
        if (m_singleJointSelection && selectedJoints.size() > 1)
        {
            AZ_Warning("EMotionFX", true, "Cannot select multiple joints for single selection actor joint browse edit. Only the first joint will be selected.");
            m_selectedJoints = {selectedJoints[0]};
        }
        else
        {
            m_selectedJoints = selectedJoints;
        }

        QString text;
        const size_t numJoints = m_selectedJoints.size();
        switch (numJoints)
        {
            case 0:
            {
                // Leave text empty (shows placeholder text).
                break;
            }
            case 1:
            {
                text = m_selectedJoints[0].GetNodeName();
                break;
            }
            default:
            {
                text = QString("%1 joints").arg(numJoints);
            }
        }
        setText(text);
    }

    void ActorJointBrowseEdit::OnSelectionDone(const AZStd::vector<SelectionItem>& selectedJoints)
    {
        SetSelectedJoints(selectedJoints);
        emit SelectionDone(selectedJoints);
    }

    void ActorJointBrowseEdit::OnSelectionDoneMCoreArray(const MCore::Array<SelectionItem>& selectedJoints)
    {
        AZStd::vector<SelectionItem> convertedSelection = FromMCoreArray(selectedJoints);
        OnSelectionDone(convertedSelection);
    }

    void ActorJointBrowseEdit::OnSelectionChanged()
    {
        if (m_jointSelectionWindow)
        {
            NodeHierarchyWidget* hierarchyWidget = m_jointSelectionWindow->GetNodeHierarchyWidget();
            hierarchyWidget->UpdateSelection();
            AZStd::vector<SelectionItem>& selectedJoints = hierarchyWidget->GetSelectedItems();

            emit SelectionChanged(selectedJoints);
        }
    }

    void ActorJointBrowseEdit::OnSelectionRejected()
    {
        emit SelectionRejected(m_previouslySelectedJoints);
    }

    void ActorJointBrowseEdit::OnTextEdited(const QString& text)
    {
        if (text.isEmpty())
        {
            OnSelectionDone(/*selectedJoints=*/{});
        }
    }

    EMotionFX::ActorInstance* ActorJointBrowseEdit::GetActorInstance() const
    {
        const CommandSystem::SelectionList& selectionList = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance* actorInstance = selectionList.GetSingleActorInstance();
        if (actorInstance)
        {
            return actorInstance;
        }

        EMotionFX::Actor* actor = selectionList.GetSingleActor();
        if (actor)
        {
            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);
                if (actorInstance->GetActor() == actor)
                {
                    return actorInstance;
                }
            }
        }

        return nullptr;
    }

    AZStd::vector<SelectionItem> ActorJointBrowseEdit::FromMCoreArray(const MCore::Array<SelectionItem>& in) const
    {
        const AZ::u32 numItems = in.GetLength();
        AZStd::vector<SelectionItem> result(static_cast<size_t>(numItems));
        for (AZ::u32 i = 0; i < numItems; ++i)
        {
            result[static_cast<size_t>(i)] = in[i];
        }

        return result;
    }
} // namespace EMStudio
