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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/ObjectEditor.h>
#include <Editor/SkeletonModel.h>
#include <Editor/Plugins/Ragdoll/RagdollJointLimitWidget.h>
#include <Editor/Plugins/Ragdoll/RagdollNodeInspectorPlugin.h>
#include <Editor/Plugins/Ragdoll/RagdollNodeWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <QLabel>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>


namespace EMotionFX
{
    RagdollCardHeader::RagdollCardHeader(QWidget* parent)
        : AzQtComponents::CardHeader(parent)
    {
        m_backgroundFrame->setObjectName("");
    }

    RagdollCard::RagdollCard(QWidget* parent)
        : AzQtComponents::Card(new RagdollCardHeader(), parent)
    {
        hideFrame();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    RagdollNodeWidget::RagdollNodeWidget(QWidget* parent)
        : SkeletonModelJointWidget(parent)
        , m_ragdollNodeCard(nullptr)
        , m_ragdollNodeEditor(nullptr)
        , m_addRemoveButton(nullptr)
        , m_jointLimitWidget(nullptr)
        , m_addColliderButton(nullptr)
        , m_collidersWidget(nullptr)
    {
    }

    QWidget* RagdollNodeWidget::CreateContentWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(ColliderContainerWidget::s_layoutSpacing);
        result->setLayout(layout);

        // Ragdoll node widget
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

        m_ragdollNodeEditor = new EMotionFX::ObjectEditor(serializeContext, result);
        m_ragdollNodeCard = new RagdollCard(result);
        m_ragdollNodeCard->setTitle("Ragdoll properties");
        m_ragdollNodeCard->setContentWidget(m_ragdollNodeEditor);
        m_ragdollNodeCard->setExpanded(true);
        AzQtComponents::CardHeader* cardHeader = m_ragdollNodeCard->header();
        cardHeader->setHasContextMenu(false);
        layout->addWidget(m_ragdollNodeCard);

        // Buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        layout->addLayout(buttonLayout);

        m_addColliderButton = new AddColliderButton("Add ragdoll collider", result);
        connect(m_addColliderButton, &AddColliderButton::AddCollider, this, &RagdollNodeWidget::OnAddCollider);
        buttonLayout->addWidget(m_addColliderButton);

        m_addRemoveButton = new QPushButton(result);
        m_addRemoveButton->setObjectName("EMFX.RagdollNodeWidget.PushButton.RagdollAddRemoveButton");
        connect(m_addRemoveButton, &QPushButton::clicked, this, &RagdollNodeWidget::OnAddRemoveRagdollNode);
        buttonLayout->addWidget(m_addRemoveButton);

        // Joint limit
        m_jointLimitWidget = new RagdollJointLimitWidget(result);
        layout->addWidget(m_jointLimitWidget);

        // Colliders
        m_collidersWidget = new ColliderContainerWidget(QIcon(":/EMotionFX/Collider.svg"), result);
        connect(m_collidersWidget, &ColliderContainerWidget::RemoveCollider, this, &RagdollNodeWidget::OnRemoveCollider);
        layout->addWidget(m_collidersWidget);

        return result;
    }

    QWidget* RagdollNodeWidget::CreateNoSelectionWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignTop);
        result->setLayout(layout);

        QLayout* copyFromLayout = ColliderHelpers::CreateCopyFromButtonLayout(this, PhysicsSetup::ColliderConfigType::Ragdoll,
            [=](PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo)
            {
                SkeletonModel* skeletonModel = nullptr;
                SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
                if (skeletonModel)
                {
                    RagdollNodeInspectorPlugin::CopyColliders(skeletonModel->GetModelIndicesForFullSkeleton(), copyFrom);
                }
            });
        layout->addLayout(copyFromLayout);

        QLabel* noSelectionLabel = new QLabel("Select joints from the Skeleton Outliner and add it to the ragdoll using the right-click menu", result);
        noSelectionLabel->setWordWrap(true);
        layout->addWidget(noSelectionLabel);

        return result;
    }

    void RagdollNodeWidget::InternalReinit(Actor* actor, Node* joint)
    {
        if (actor && joint)
        {
            m_ragdollNodeEditor->ClearInstances(false);

            Physics::CharacterColliderNodeConfiguration* colliderNodeConfig = GetRagdollColliderNodeConfig();
            Physics::RagdollNodeConfiguration* ragdollNodeConfig = GetRagdollNodeConfig();
            if (ragdollNodeConfig)
            {
                m_addColliderButton->show();
                m_addRemoveButton->setText("Remove from ragdoll");

                m_ragdollNodeEditor->AddInstance(ragdollNodeConfig, azrtti_typeid(ragdollNodeConfig));

                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

                if (colliderNodeConfig)
                {
                    m_collidersWidget->Update(actor, joint, PhysicsSetup::ColliderConfigType::Ragdoll, colliderNodeConfig->m_shapes, serializeContext);
                }
                else
                {
                    m_collidersWidget->Reset();
                }

                m_jointLimitWidget->Update(m_modelIndex);

                m_ragdollNodeCard->setExpanded(true);
                m_ragdollNodeCard->show();
                m_jointLimitWidget->show();
                m_collidersWidget->show();
            }
            else
            {
                m_addColliderButton->hide();
                m_addRemoveButton->setText("Add to ragdoll");
                m_collidersWidget->Reset();
                m_ragdollNodeCard->hide();
                m_jointLimitWidget->Update(QModelIndex());
                m_jointLimitWidget->hide();
                m_collidersWidget->hide();
            }
        }
        else
        {
            m_ragdollNodeEditor->ClearInstances(true);
            m_jointLimitWidget->Update(QModelIndex());
            m_collidersWidget->Reset();
        }
    }

    void RagdollNodeWidget::OnAddRemoveRagdollNode()
    {
        QModelIndexList modelIndexList;
        modelIndexList.push_back(m_modelIndex);

        if (GetRagdollNodeConfig())
        {
            // The node is present in the ragdoll, remove it.
            RagdollNodeInspectorPlugin::RemoveFromRagdoll(modelIndexList);
        }
        else
        {
            // The node is not part of the ragdoll, add it.
            RagdollNodeInspectorPlugin::AddToRagdoll(modelIndexList);
        }
    }

    void RagdollNodeWidget::OnAddCollider(const AZ::TypeId& colliderType)
    {
        QModelIndexList modelIndexList;
        modelIndexList.push_back(m_modelIndex);

        ColliderHelpers::AddCollider(modelIndexList, PhysicsSetup::Ragdoll, colliderType);
    }

    void RagdollNodeWidget::OnRemoveCollider(int colliderIndex)
    {
        const Actor* actor = m_modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const Node* joint = m_modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

        CommandColliderHelpers::RemoveCollider(actor->GetID(), joint->GetNameString(), PhysicsSetup::Ragdoll, colliderIndex);
    }

    Physics::RagdollConfiguration* RagdollNodeWidget::GetRagdollConfig() const
    {
        Actor* actor = m_modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        Node* node = m_modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
        if (actor && node)
        {
            const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
            if (physicsSetup)
            {
                return &physicsSetup->GetRagdollConfig();
            }
        }

        return nullptr;
    }

    Physics::CharacterColliderNodeConfiguration* RagdollNodeWidget::GetRagdollColliderNodeConfig() const
    {
        Actor* actor = m_modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        Node* node = m_modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
        if (actor && node)
        {
            const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
            if (physicsSetup)
            {
                const Physics::CharacterColliderConfiguration& colliderConfig = physicsSetup->GetRagdollConfig().m_colliders;
                return colliderConfig.FindNodeConfigByName(node->GetNameString());
            }
        }

        return nullptr;
    }

    Physics::RagdollNodeConfiguration* RagdollNodeWidget::GetRagdollNodeConfig() const
    {
        Actor* actor = m_modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        Node* node = m_modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
        if (actor && node)
        {
            const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
            if (physicsSetup)
            {
                const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();
                return ragdollConfig.FindNodeConfigByName(node->GetNameString());
            }
        }

        return nullptr;
    }
} // namespace EMotionFX
