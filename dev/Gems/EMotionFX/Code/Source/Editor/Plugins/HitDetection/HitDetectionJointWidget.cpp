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
#include <AzFramework/Physics/Character.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/SkeletonModel.h>
#include <Editor/Plugins/HitDetection/HitDetectionJointInspectorPlugin.h>
#include <Editor/Plugins/HitDetection/HitDetectionJointWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>


namespace EMotionFX
{
    HitDetectionJointWidget::HitDetectionJointWidget(QWidget* parent)
        : SkeletonModelJointWidget(parent)
        , m_addColliderButton(nullptr)
        , m_collidersWidget(nullptr)
    {
    }

    QWidget* HitDetectionJointWidget::CreateContentWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(ColliderContainerWidget::s_layoutSpacing);
        result->setLayout(layout);

        // Add collider button
        m_addColliderButton = new AddColliderButton("Add hit detection collider", result);
        connect(m_addColliderButton, &AddColliderButton::AddCollider, this, &HitDetectionJointWidget::OnAddCollider);
        layout->addWidget(m_addColliderButton);

        // Colliders
        m_collidersWidget = new ColliderContainerWidget(QIcon(":/EMotionFX/RagdollCollider_White.png"), result);
        connect(m_collidersWidget, &ColliderContainerWidget::RemoveCollider, this, &HitDetectionJointWidget::OnRemoveCollider);
        layout->addWidget(m_collidersWidget);

        return result;
    }

    QWidget* HitDetectionJointWidget::CreateNoSelectionWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignTop);
        result->setLayout(layout);

        QHBoxLayout* copyFromLayout = new QHBoxLayout();
        copyFromLayout->setMargin(0);

        QPushButton* copyFromHitDetectionButton = new QPushButton("Copy from ragdoll", result);
        connect(copyFromHitDetectionButton, &QPushButton::clicked, this, [=]
            {
                SkeletonModel* skeletonModel = nullptr;
                SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
                if (skeletonModel)
                {
                    ColliderHelpers::CopyColliders(skeletonModel->GetModelIndicesForFullSkeleton(), PhysicsSetup::Ragdoll, PhysicsSetup::HitDetection);
                }
            });
        copyFromLayout->addWidget(copyFromHitDetectionButton);

        // Note: Cloth collider editor is disabled as it is in preview
        /*QPushButton* copyFromRagdollButton = new QPushButton("Copy from cloth", result);
        connect(copyFromRagdollButton, &QPushButton::clicked, this, [=]
            {
                SkeletonModel* skeletonModel = nullptr;
                SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
                if (skeletonModel)
                {
                    ColliderHelpers::CopyColliders(skeletonModel->GetModelIndicesForFullSkeleton(), PhysicsSetup::Cloth, PhysicsSetup::HitDetection);
                }
            });
        copyFromLayout->addWidget(copyFromRagdollButton);*/

        layout->addLayout(copyFromLayout);

        QLabel* noSelectionLabel = new QLabel("Select a joint from the Skeleton Outliner", result);
        noSelectionLabel->setWordWrap(true);
        layout->addWidget(noSelectionLabel);

        return result;
    }

    void HitDetectionJointWidget::InternalReinit(Actor* actor, Node* node)
    {
        if (actor && node)
        {
            Physics::CharacterColliderNodeConfiguration* hitDetectionNodeConfig = GetNodeConfig();
            if (hitDetectionNodeConfig)
            {
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

                m_collidersWidget->Update(hitDetectionNodeConfig->m_shapes, serializeContext);
                m_collidersWidget->show();
            }
            else
            {
                m_collidersWidget->Update(Physics::ShapeConfigurationList(), nullptr);
            }
        }
        else
        {
            m_collidersWidget->Update(Physics::ShapeConfigurationList(), nullptr);
        }
    }

    void HitDetectionJointWidget::OnAddCollider(const AZ::TypeId& colliderType)
    {
        QModelIndexList modelIndexList;
        modelIndexList.push_back(m_modelIndex);

        ColliderHelpers::AddCollider(modelIndexList, PhysicsSetup::HitDetection, colliderType);
    }

    void HitDetectionJointWidget::OnRemoveCollider(int colliderIndex)
    {
        const Actor* actor = m_modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const Node* joint = m_modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

        CommandColliderHelpers::RemoveCollider(actor->GetID(), joint->GetNameString(), PhysicsSetup::HitDetection, colliderIndex);
    }

    Physics::CharacterColliderNodeConfiguration* HitDetectionJointWidget::GetNodeConfig()
    {
        Actor* actor = m_modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        Node* node = m_modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
        if (!actor || !node)
        {
            return nullptr;
        }

        const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        if (!physicsSetup)
        {
            return nullptr;
        }

        const Physics::CharacterColliderConfiguration& hitDetectionConfig = physicsSetup->GetHitDetectionConfig();
        return hitDetectionConfig.FindNodeConfigByName(node->GetNameString());
    }
} // namespace EMotionFX