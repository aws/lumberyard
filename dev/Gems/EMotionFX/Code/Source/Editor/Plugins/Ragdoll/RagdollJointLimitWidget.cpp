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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <Editor/Plugins/Ragdoll/RagdollJointLimitWidget.h>
#include <Editor/SkeletonModel.h>
#include <Editor/ObjectEditor.h>
#include <QComboBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QSignalBlocker>


namespace EMotionFX
{
    int RagdollJointLimitWidget::s_leftMargin = 13;
    int RagdollJointLimitWidget::s_textColumnWidth = 142;

    RagdollJointLimitWidget::RagdollJointLimitWidget(QWidget* parent)
        : AzQtComponents::Card(parent)
        , m_cardHeaderIcon(":/EMotionFX/RagdollJointLimit_White.png")
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

        setTitle("Joint limit");

        AzQtComponents::CardHeader* cardHeader = header();
        cardHeader->setIcon(m_cardHeaderIcon);
        cardHeader->setHasContextMenu(false);

        QVBoxLayout* vLayout  = new QVBoxLayout();
        vLayout->setAlignment(Qt::AlignTop);
        vLayout->setMargin(0);

        QWidget* innerWidget = new QWidget(this);
        innerWidget->setLayout(vLayout);

        QGridLayout* topLayout = new QGridLayout();
        topLayout->setMargin(2);
        topLayout->setAlignment(Qt::AlignLeft);

        // Has joint limit
        QWidget* spacerWidget = new QWidget(this);
        spacerWidget->setFixedWidth(s_leftMargin);
        topLayout->addWidget(spacerWidget, 0, 0, Qt::AlignLeft);

        QLabel* hasLimitLabel = new QLabel("Has joint limit");
        hasLimitLabel->setFixedWidth(s_textColumnWidth);
        topLayout->addWidget(hasLimitLabel, 0, 1, Qt::AlignLeft);

        m_hasLimitCheckbox = new QCheckBox("", this);
        connect(m_hasLimitCheckbox, &QCheckBox::stateChanged, this, &RagdollJointLimitWidget::OnHasLimitStateChanged);
        topLayout->addWidget(m_hasLimitCheckbox, 0, 2);

        // Joint limit type
        m_typeLabel = new QLabel("Limit type");
        topLayout->addWidget(m_typeLabel, 1, 1, Qt::AlignLeft);

        m_typeComboBox = new QComboBox(innerWidget);
        m_typeComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        topLayout->addWidget(m_typeComboBox, 1, 2);

        vLayout->addLayout(topLayout);

        if (serializeContext)
        {
            AZStd::vector<AZ::TypeId> supportedJointLimitTypes;
            Physics::SystemRequestBus::BroadcastResult(supportedJointLimitTypes, &Physics::SystemRequests::GetSupportedJointTypes);
            for (const AZ::TypeId& jointLimitType : supportedJointLimitTypes)
            {
                const char* jointLimitName = serializeContext->FindClassData(jointLimitType)->m_editData->m_name;
                m_typeComboBox->addItem(jointLimitName, jointLimitType.ToString<AZStd::string>().c_str());
            }

            // Reflected property editor for joint limit
            m_objectEditor = new EMotionFX::ObjectEditor(serializeContext, innerWidget);
            vLayout->addWidget(m_objectEditor);
        }

        connect(m_typeComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &RagdollJointLimitWidget::OnJointTypeChanged);
        connect(m_typeComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &RagdollJointLimitWidget::OnJointTypeChanged);

        setContentWidget(innerWidget);
        setExpanded(true);
    }

    bool RagdollJointLimitWidget::HasJointLimit() const
    {
        return m_hasLimitCheckbox->isChecked();
    }

    void RagdollJointLimitWidget::Update(const QModelIndex& modelIndex)
    {
        m_nodeIndex = modelIndex;
        m_objectEditor->ClearInstances(false);

        Physics::RagdollNodeConfiguration* ragdollNodeConfig = GetRagdollNodeConfig();
        if (ragdollNodeConfig)
        {
            Physics::JointLimitConfiguration* jointLimitConfig = ragdollNodeConfig->m_jointLimit.get();
            if (jointLimitConfig)
            {
                const AZ::TypeId& jointTypeId = jointLimitConfig->RTTI_GetType();
                m_objectEditor->AddInstance(jointLimitConfig, jointTypeId);

                // Only show the type combo box in case there is more than one limit type to choose from.
                if (m_typeComboBox->count() > 1)
                {
                    m_typeLabel->show();
                    m_typeComboBox->show();
                }
                else
                {
                    m_typeLabel->hide();
                    m_typeComboBox->hide();
                }

                m_objectEditor->show();
            }
            else
            {
                // No joint limit
                m_typeLabel->hide();
                m_typeComboBox->hide();
                m_objectEditor->hide();
            }

            QSignalBlocker checkboxBlocker(m_hasLimitCheckbox);
            m_hasLimitCheckbox->setChecked(jointLimitConfig != nullptr);
        }
        else
        {
            m_typeLabel->hide();
            m_typeComboBox->hide();
            m_objectEditor->hide();
        }
    }

    Physics::RagdollNodeConfiguration* RagdollJointLimitWidget::GetRagdollNodeConfig() const
    {
        Actor* actor = m_nodeIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        Node* node = m_nodeIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
        if (!actor || !node)
        {
            return nullptr;
        }

        const AZStd::shared_ptr<EMotionFX::PhysicsSetup> physicsSetup = actor->GetPhysicsSetup();
        const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();
        return ragdollConfig.FindNodeConfigByName(node->GetNameString());
    }

    void RagdollJointLimitWidget::OnHasLimitStateChanged(int state)
    {
        if (state == Qt::Checked)
        {
            ChangeLimitType(m_typeComboBox->currentIndex());
        }
        else
        {
            ChangeLimitType(AZ::TypeId::CreateNull());
        }
    }

    void RagdollJointLimitWidget::ChangeLimitType(const AZ::TypeId& type)
    {
        Physics::RagdollNodeConfiguration* ragdollNodeConfig = GetRagdollNodeConfig();
        if (ragdollNodeConfig)
        {
            if (type.IsNull())
            {
                ragdollNodeConfig->m_jointLimit = nullptr;
            }
            else
            {
                const Node* node = m_nodeIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
                const Skeleton* skeleton = m_nodeIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>()->GetSkeleton();
                ragdollNodeConfig->m_jointLimit = CommandRagdollHelpers::CreateJointLimitByType(type, skeleton, node);
            }

            Update();
        }
    }

    void RagdollJointLimitWidget::ChangeLimitType(int supportedTypeIndex)
    {
        const QByteArray typeString = m_typeComboBox->itemData(supportedTypeIndex).toString().toUtf8();
        const AZ::TypeId newLimitType = AZ::TypeId::CreateString(typeString.data(), typeString.count());
        ChangeLimitType(newLimitType);
    }

    void RagdollJointLimitWidget::OnJointTypeChanged(int index)
    {
        ChangeLimitType(index);
    }
} // namespace EMotionFX
