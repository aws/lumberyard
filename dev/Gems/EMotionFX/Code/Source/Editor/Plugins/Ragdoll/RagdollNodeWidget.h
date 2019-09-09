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

#pragma once

#include <AzFramework/Physics/Ragdoll.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <Editor/SkeletonModelJointWidget.h>
#include <QWidget>


QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace Physics
{
    class RagdollNodeConfiguration;
}

namespace EMotionFX
{
    class ObjectEditor;
    class AddColliderButton;
    class ColliderContainerWidget;
    class RagdollJointLimitWidget;

    class RagdollCardHeader
        : public AzQtComponents::CardHeader
    {
    public:
        RagdollCardHeader(QWidget* parent = nullptr);
    };

    class RagdollCard
        : public AzQtComponents::Card
    {
    public:
        RagdollCard(QWidget* parent = nullptr);
    };

    class RagdollNodeWidget
        : public SkeletonModelJointWidget
    {
        Q_OBJECT //AUTOMOC

    public:
        RagdollNodeWidget(QWidget* parent = nullptr);
        ~RagdollNodeWidget() = default;

    public slots:
        void OnAddRemoveRagdollNode();

        void OnAddCollider(const AZ::TypeId& colliderType);
        void OnRemoveCollider(int colliderIndex);

    private:
        // SkeletonModelJointWidget
        QWidget* CreateContentWidget(QWidget* parent);
        QWidget* CreateNoSelectionWidget(QWidget* parent);
        void InternalReinit(Actor* actor, Node* joint);

        Physics::RagdollConfiguration* GetRagdollConfig() const;
        Physics::CharacterColliderNodeConfiguration* GetRagdollColliderNodeConfig() const;
        Physics::RagdollNodeConfiguration* GetRagdollNodeConfig() const;

        // Ragdoll node
        AzQtComponents::Card*       m_ragdollNodeCard;
        EMotionFX::ObjectEditor*    m_ragdollNodeEditor;
        QPushButton*                m_addRemoveButton;

        // Joint limit
        RagdollJointLimitWidget*    m_jointLimitWidget;

        // Colliders
        AddColliderButton*          m_addColliderButton;
        ColliderContainerWidget*    m_collidersWidget;
    };
} // namespace EMotionFX