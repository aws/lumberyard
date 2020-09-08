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
#include <Editor/Plugins/SimulatedObject/SimulatedObjectColliderWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <QLabel>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>


namespace EMotionFX
{
    SimulatedObjectColliderWidget::SimulatedObjectColliderWidget(QWidget* parent)
        : SkeletonModelJointWidget(parent)
        , m_addColliderButton(nullptr)
        , m_collidersWidget(nullptr)
    {
    }

    QWidget* SimulatedObjectColliderWidget::CreateContentWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(ColliderContainerWidget::s_layoutSpacing);
        result->setLayout(layout);

        // Add collider button
        m_addColliderButton = new AddColliderButton("Add simulated object collider", result,
            { azrtti_typeid<Physics::CapsuleShapeConfiguration>(),
             azrtti_typeid<Physics::SphereShapeConfiguration>() });
        m_addColliderButton->setObjectName("EMFX.SimulatedObjectColliderWidget.AddColliderButton");
        connect(m_addColliderButton, &AddColliderButton::AddCollider, this, &SimulatedObjectColliderWidget::OnAddCollider);
        layout->addWidget(m_addColliderButton);

        // Colliders
        m_collidersWidget = new ColliderContainerWidget(QIcon(":/EMotionFX/Collider.svg"), result); // use the ragdoll white collider icon because it's generic to all colliders.
        connect(m_collidersWidget, &ColliderContainerWidget::RemoveCollider, this, &SimulatedObjectColliderWidget::OnRemoveCollider);
        layout->addWidget(m_collidersWidget);

        return result;
    }

    QWidget* SimulatedObjectColliderWidget::CreateNoSelectionWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignTop);
        result->setLayout(layout);

        QLayout* copyFromLayout = ColliderHelpers::CreateCopyFromButtonLayout(this, PhysicsSetup::ColliderConfigType::SimulatedObjectCollider,
            [=](PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo)
            {
                SkeletonModel* skeletonModel = nullptr;
                SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
                if (skeletonModel)
                {
                    ColliderHelpers::CopyColliders(skeletonModel->GetModelIndicesForFullSkeleton(), copyFrom, copyTo);
                }
            });
        layout->addLayout(copyFromLayout);

        QLabel* noSelectionLabel = new QLabel("Select a joint from the Skeleton Outliner", result);
        noSelectionLabel->setWordWrap(true);
        layout->addWidget(noSelectionLabel);

        return result;
    }

    void SimulatedObjectColliderWidget::InternalReinit(Actor* actor, Node* joint)
    {
        if (actor && joint)
        {
            Physics::CharacterColliderNodeConfiguration* nodeConfig = GetNodeConfig();
            if (nodeConfig)
            {
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

                m_collidersWidget->Update(actor, joint, PhysicsSetup::ColliderConfigType::SimulatedObjectCollider, nodeConfig->m_shapes, serializeContext);
                m_collidersWidget->show();
            }
            else
            {
                m_collidersWidget->Reset();
            }
        }
        else
        {
            m_collidersWidget->Reset();
        }
    }

    void SimulatedObjectColliderWidget::OnAddCollider(const AZ::TypeId& colliderType)
    {
        QModelIndexList modelIndexList;
        modelIndexList.push_back(m_modelIndex);

        ColliderHelpers::AddCollider(modelIndexList, PhysicsSetup::SimulatedObjectCollider, colliderType);
    }

    void SimulatedObjectColliderWidget::OnRemoveCollider(int colliderIndex)
    {
        const Actor* actor = m_modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const Node* joint = m_modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

        CommandColliderHelpers::RemoveCollider(actor->GetID(), joint->GetNameString(), PhysicsSetup::SimulatedObjectCollider, colliderIndex);
    }

    Physics::CharacterColliderNodeConfiguration* SimulatedObjectColliderWidget::GetNodeConfig(const QModelIndex& modelIndex)
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

        const Physics::CharacterColliderConfiguration& simulatedObjectColliderConfig = physicsSetup->GetSimulatedObjectColliderConfig();
        return simulatedObjectColliderConfig.FindNodeConfigByName(joint->GetNameString());
    }

    Physics::CharacterColliderNodeConfiguration* SimulatedObjectColliderWidget::GetNodeConfig() const
    {
        return GetNodeConfig(m_modelIndex);
    }
} // namespace EMotionFX
