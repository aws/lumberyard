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

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/SkeletonModel.h>
#include <QAction>
#include <QGridLayout>
#include <QMenu>
#include <QObject>
#include <QPushButton>


namespace EMotionFX
{
    void ColliderHelpers::AddCopyColliderCommandToGroup(const Actor* actor, const Node* joint, PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo, MCore::CommandGroup& commandGroup)
    {
        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::CharacterColliderConfiguration* copyFromColliderConfig = physicsSetup->GetColliderConfigByType(copyFrom);
        if (!copyFromColliderConfig)
        {
            return;
        }

        const Physics::CharacterColliderNodeConfiguration* copyFromNodeConfig = copyFromColliderConfig->FindNodeConfigByName(joint->GetNameString());
        if (copyFromNodeConfig)
        {
            for (const Physics::ShapeConfigurationPair& shapeConfigPair : copyFromNodeConfig->m_shapes)
            {
                const AZStd::string contents = MCore::ReflectionSerializer::Serialize(&shapeConfigPair).GetValue();
                CommandColliderHelpers::AddCollider(actor->GetID(), joint->GetNameString(), copyTo, contents, AZStd::nullopt, &commandGroup);
            }
        }
    }

    void ColliderHelpers::CopyColliders(const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo, bool removeExistingColliders)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName = AZStd::string::format("Copy %s collider to %s",
            PhysicsSetup::GetStringForColliderConfigType(copyFrom),
            PhysicsSetup::GetStringForColliderConfigType(copyTo));

        MCore::CommandGroup commandGroup(groupName);

        AZStd::string contents;
        for (const QModelIndex& selectedIndex : modelIndices)
        {
            const Actor* actor = selectedIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            const Node* joint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
            const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();

            if (removeExistingColliders)
            {
                CommandColliderHelpers::ClearColliders(actor->GetID(), joint->GetNameString(), copyTo, &commandGroup);
            }

            AddCopyColliderCommandToGroup(actor, joint, copyFrom, copyTo, commandGroup);
        }

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void ColliderHelpers::AddCollider(const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType addTo, const AZ::TypeId& colliderType)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName = AZStd::string::format("Add %s colliders",
            PhysicsSetup::GetStringForColliderConfigType(addTo));

        MCore::CommandGroup commandGroup(groupName);

        for (const QModelIndex& selectedIndex : modelIndices)
        {
            const Actor* actor = selectedIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            const Node* joint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

            CommandColliderHelpers::AddCollider(actor->GetID(), joint->GetNameString(), addTo, colliderType, &commandGroup);
        }

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void ColliderHelpers::ClearColliders(const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType removeFrom)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName = AZStd::string::format("Remove %s colliders",
            PhysicsSetup::GetStringForColliderConfigType(removeFrom));

        MCore::CommandGroup commandGroup(groupName);

        for (const QModelIndex& selectedIndex : modelIndices)
        {
            const Actor* actor = selectedIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            const Node* joint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

            CommandColliderHelpers::ClearColliders(actor->GetID(), joint->GetNameString(), removeFrom, &commandGroup);
        }

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    bool ColliderHelpers::AreCollidersReflected()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (serializeContext)
        {
            if (serializeContext->FindClassData(azrtti_typeid<Physics::SphereShapeConfiguration>()) &&
                serializeContext->FindClassData(azrtti_typeid<Physics::BoxShapeConfiguration>()) &&
                serializeContext->FindClassData(azrtti_typeid<Physics::CapsuleShapeConfiguration>()))
            {
                return true;
            }
        }

        return false;
    }

    void ColliderHelpers::AddCopyFromMenu(QObject* parent, QMenu* parentMenu, PhysicsSetup::ColliderConfigType createForType,
        const AZStd::function<void(PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo)>& copyFunc)
    {
        QMenu* copyFromMenu = parentMenu->addMenu("Copy from");

        for (int i = 0; i < PhysicsSetup::ColliderConfigType::Unknown; ++i)
        {
            const PhysicsSetup::ColliderConfigType copyFrom = static_cast<PhysicsSetup::ColliderConfigType>(i);
            if (copyFrom == createForType)
            {
                continue;
            }

            // Disable cloth - not available.
            if (copyFrom == PhysicsSetup::ColliderConfigType::Cloth)
            {
                continue;
            }

            QAction* action = copyFromMenu->addAction(PhysicsSetup::GetVisualNameForColliderConfigType(copyFrom));
            QObject::connect(action, &QAction::triggered, parent, [=]
                {
                    copyFunc(copyFrom, createForType);
                });
        }
    }

    void ColliderHelpers::AddCopyFromMenu(QObject* parent, QMenu* parentMenu, PhysicsSetup::ColliderConfigType createForType, const QModelIndexList& modelIndices)
    {
        AddCopyFromMenu(parent, parentMenu, createForType, [=](PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo)
            {
                ColliderHelpers::CopyColliders(modelIndices, copyFrom, copyTo);
            });
    }

    QLayout* ColliderHelpers::CreateCopyFromButtonLayout(QObject* parent, PhysicsSetup::ColliderConfigType createForType,
        const AZStd::function<void(PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo)>& copyFunc)
    {
        QGridLayout* layout = new QGridLayout();
        layout->setMargin(0);

        int counter = 0;
        for (int i = 0; i < PhysicsSetup::ColliderConfigType::Unknown; ++i)
        {
            const PhysicsSetup::ColliderConfigType copyFrom = static_cast<PhysicsSetup::ColliderConfigType>(i);
            if (copyFrom == createForType)
            {
                continue;
            }

            // Disable cloth - not available.
            if (copyFrom == PhysicsSetup::ColliderConfigType::Cloth)
            {
                continue;
            }

            const QString buttonText = QString("Copy from %1").arg(PhysicsSetup::GetVisualNameForColliderConfigType(copyFrom));
            QPushButton* button = new QPushButton(buttonText);
            QObject::connect(button, &QPushButton::clicked, parent, [=]
                {
                    copyFunc(copyFrom, createForType);
                });

            const int row = counter / 2;
            const int column = counter % 2;
            layout->addWidget(button, row, column, Qt::AlignTop);
            counter++;
        }

        return layout;
    }
} // namespace EMotionFX