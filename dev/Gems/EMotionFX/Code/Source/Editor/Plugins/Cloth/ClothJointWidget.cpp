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
#include <Editor/Plugins/Cloth/ClothJointInspectorPlugin.h>
#include <Editor/Plugins/Cloth/ClothJointWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <QLabel>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>


namespace EMotionFX
{
    ClothJointWidget::ClothJointWidget(QWidget* parent)
        : SkeletonModelJointWidget(parent)
        , m_addColliderButton(nullptr)
        , m_collidersWidget(nullptr)
    {
    }

    QWidget* ClothJointWidget::CreateContentWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(ColliderContainerWidget::s_layoutSpacing);
        result->setLayout(layout);

        // Add collider button
        m_addColliderButton = new AddColliderButton("Add cloth collider", result,
            {azrtti_typeid<Physics::CapsuleShapeConfiguration>(),
             azrtti_typeid<Physics::SphereShapeConfiguration>()});
        connect(m_addColliderButton, &AddColliderButton::AddCollider, this, &ClothJointWidget::OnAddCollider);
        layout->addWidget(m_addColliderButton);

        // Colliders
        m_collidersWidget = new ColliderContainerWidget(QIcon(":/EMotionFX/RagdollCollider_White.png"), result);
        connect(m_collidersWidget, &ColliderContainerWidget::RemoveCollider, this, &ClothJointWidget::OnRemoveCollider);
        layout->addWidget(m_collidersWidget);

        return result;
    }

    QWidget* ClothJointWidget::CreateNoSelectionWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignTop);
        result->setLayout(layout);

        QHBoxLayout* copyFromLayout = new QHBoxLayout();
        copyFromLayout->setMargin(0);

        QPushButton* copyFromHitDetectionButton = new QPushButton("Copy from hit detection", result);
        connect(copyFromHitDetectionButton, &QPushButton::clicked, this, [=]
            {
                SkeletonModel* skeletonModel = nullptr;
                SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
                if (skeletonModel)
                {
                    ColliderHelpers::CopyColliders(skeletonModel->GetModelIndicesForFullSkeleton(), PhysicsSetup::HitDetection, PhysicsSetup::Cloth);
                }
            });
        copyFromLayout->addWidget(copyFromHitDetectionButton);

        QPushButton* copyFromRagdollButton = new QPushButton("Copy from ragdoll", result);
        connect(copyFromRagdollButton, &QPushButton::clicked, this, [=]
            {
                SkeletonModel* skeletonModel = nullptr;
                SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
                if (skeletonModel)
                {
                    ColliderHelpers::CopyColliders(skeletonModel->GetModelIndicesForFullSkeleton(), PhysicsSetup::Ragdoll, PhysicsSetup::Cloth);
                }
            });
        copyFromLayout->addWidget(copyFromRagdollButton);

        layout->addLayout(copyFromLayout);

        QLabel* noSelectionLabel = new QLabel("Select a joint from the Skeleton Outliner", result);
        noSelectionLabel->setWordWrap(true);
        layout->addWidget(noSelectionLabel);

        return result;
    }

    void ClothJointWidget::InternalReinit(Actor* actor, Node* joint)
    {
        if (actor && joint)
        {
            Physics::CharacterColliderNodeConfiguration* nodeConfig = GetNodeConfig();
            if (nodeConfig)
            {
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

                m_collidersWidget->Update(nodeConfig->m_shapes, serializeContext);
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

    void ClothJointWidget::OnAddCollider(const AZ::TypeId& colliderType)
    {
        QModelIndexList modelIndexList;
        modelIndexList.push_back(m_modelIndex);

        ColliderHelpers::AddCollider(modelIndexList, PhysicsSetup::Cloth, colliderType);
    }

    void ClothJointWidget::OnRemoveCollider(int colliderIndex)
    {
        const Actor* actor = m_modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const Node* joint = m_modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

        CommandColliderHelpers::RemoveCollider(actor->GetID(), joint->GetNameString(), PhysicsSetup::Cloth, colliderIndex);
    }

    Physics::CharacterColliderNodeConfiguration* ClothJointWidget::GetNodeConfig(const QModelIndex& modelIndex)
    {
        Actor* actor = modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        Node* joint = modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
        if (!actor || !joint)
        {
            return nullptr;
        }

        const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        if (!physicsSetup)
        {
            return nullptr;
        }

        const Physics::CharacterColliderConfiguration& clothConfig = physicsSetup->GetClothConfig();
        return clothConfig.FindNodeConfigByName(joint->GetNameString());
    }

    Physics::CharacterColliderNodeConfiguration* ClothJointWidget::GetNodeConfig() const
    {
        return GetNodeConfig(m_modelIndex);
    }
} // namespace EMotionFX