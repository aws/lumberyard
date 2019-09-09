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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <MCore/Source/AzCoreConversions.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ObjectEditor.h>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMenu>


namespace EMotionFX
{
    ColliderWidget::ColliderWidget(QIcon* icon, QWidget* parent, AZ::SerializeContext* serializeContext)
        : AzQtComponents::Card(parent)
        , m_icon(icon)
    {
        m_editor = new EMotionFX::ObjectEditor(serializeContext, this);
        setContentWidget(m_editor);
        setExpanded(true);

        connect(this, &AzQtComponents::Card::contextMenuRequested, this, &ColliderWidget::OnCardContextMenu);
    }

    void ColliderWidget::Update(const Physics::ShapeConfigurationPair& collider, int colliderIndex)
    {
        if (!collider.first || !collider.second)
        {
            m_editor->ClearInstances(true);
            m_collider = Physics::ShapeConfigurationPair();
            return;
        }

        if (collider == m_collider)
        {
            m_editor->InvalidateAll();
            return;
        }

        const AZ::TypeId& shapeType = collider.second->RTTI_GetType();
        m_editor->ClearInstances(false);
        m_editor->AddInstance(collider.second.get(), shapeType);
        m_editor->AddInstance(collider.first.get(), collider.first->RTTI_GetType());

        m_collider = collider;

        AzQtComponents::CardHeader* cardHeader = header();
        cardHeader->setIcon(*m_icon);
        if (shapeType == azrtti_typeid<Physics::CapsuleShapeConfiguration>())
        {
            setTitle("Capsule");
        }
        else if (shapeType == azrtti_typeid<Physics::SphereShapeConfiguration>())
        {
            setTitle("Sphere");
        }
        else if (shapeType == azrtti_typeid<Physics::BoxShapeConfiguration>())
        {
            setTitle("Box");
        }
        else
        {
            setTitle("Unknown");
        }

        setProperty("colliderIndex", static_cast<int>(colliderIndex));
        setExpanded(true);
    }

    void ColliderWidget::OnCardContextMenu(const QPoint& position)
    {
        const AzQtComponents::Card* card = static_cast<AzQtComponents::Card*>(sender());
        const int colliderIndex = card->property("colliderIndex").toInt();

        QMenu contextMenu(this);

        QAction* deleteAction = contextMenu.addAction("Delete collider");
        deleteAction->setProperty("colliderIndex", colliderIndex);
        connect(deleteAction, &QAction::triggered, this, &ColliderWidget::OnRemoveCollider);

        if (!contextMenu.isEmpty())
        {
            contextMenu.exec(position);
        }
    }

    void ColliderWidget::OnRemoveCollider()
    {
        QAction* action = static_cast<QAction*>(sender());
        const int colliderIndex = action->property("colliderIndex").toInt();

        emit RemoveCollider(colliderIndex);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AddColliderButton::AddColliderButton(const QString& text, QWidget* parent, const AZStd::vector<AZ::TypeId>& supportedColliderTypes)
        : QPushButton(text, parent)
        , m_supportedColliderTypes(supportedColliderTypes)
    {
        setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/ArrowDownGray.png"));
        connect(this, &QPushButton::clicked, this, &AddColliderButton::OnCreateContextMenuAddCollider);
    }

    void AddColliderButton::OnCreateContextMenuAddCollider()
    {
        QMenu contextMenu;

        AZStd::string actionName;
        for (const AZ::TypeId& typeId : m_supportedColliderTypes)
        {
            actionName = AZStd::string::format("Add %s", GetNameForColliderType(typeId).c_str());
            QAction* addBoxAction = contextMenu.addAction(actionName.c_str());
            addBoxAction->setProperty("typeId", typeId.ToString<AZStd::string>().c_str());
            connect(addBoxAction, &QAction::triggered, this, &AddColliderButton::OnAddColliderActionTriggered);
        }

        contextMenu.setFixedWidth(width());
        contextMenu.exec(mapToGlobal(QPoint(0, height())));
    }

    void AddColliderButton::OnAddColliderActionTriggered(bool checked)
    {
        QAction* action = static_cast<QAction*>(sender());
        const QByteArray typeString = action->property("typeId").toString().toUtf8();
        const AZ::TypeId& typeId = AZ::TypeId::CreateString(typeString.data(), typeString.size());

        emit AddCollider(typeId);
    }

    AZStd::string AddColliderButton::GetNameForColliderType(AZ::TypeId colliderType) const
    {
        if (colliderType == azrtti_typeid<Physics::BoxShapeConfiguration>())
        {
            return "box";
        }
        else if (colliderType == azrtti_typeid<Physics::CapsuleShapeConfiguration>())
        {
            return "capsule";
        }
        else if (colliderType == azrtti_typeid<Physics::SphereShapeConfiguration>())
        {
            return "sphere";
        }

        return colliderType.ToString<AZStd::string>();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Align the layout spacing with the entity inspector.
    int ColliderContainerWidget::s_layoutSpacing = 13;

    ColliderContainerWidget::ColliderContainerWidget(const QIcon& colliderIcon, QWidget* parent)
        : QWidget(parent)
        , m_colliderIcon(colliderIcon)
    {
        m_layout = new QVBoxLayout();
        m_layout->setAlignment(Qt::AlignTop);
        m_layout->setMargin(0);

        m_layout->setSpacing(s_layoutSpacing);

        setLayout(m_layout);
    }

    void ColliderContainerWidget::Update(const Physics::ShapeConfigurationList& colliders, AZ::SerializeContext* serializeContext)
    {
        const size_t numColliders = colliders.size();
        size_t numAvailableColliderWidgets = m_colliderWidgets.size();

        // Create new collider widgets in case we don't have enough.
        if (numColliders > numAvailableColliderWidgets)
        {
            const int numWidgetsToCreate = static_cast<int>(numColliders) - static_cast<int>(numAvailableColliderWidgets);
            for (int i = 0; i < numWidgetsToCreate; ++i)
            {
                ColliderWidget* colliderWidget = new ColliderWidget(&m_colliderIcon, this, serializeContext);
                connect(colliderWidget, &ColliderWidget::RemoveCollider, this, &ColliderContainerWidget::OnRemoveCollider);
                m_colliderWidgets.emplace_back(colliderWidget);
                m_layout->addWidget(colliderWidget, 0, Qt::AlignTop);
            }
            numAvailableColliderWidgets = m_colliderWidgets.size();
        }
        AZ_Assert(numAvailableColliderWidgets >= numColliders, "Not enough collider widgets available. Something went one with creating new ones.");

        for (size_t i = 0; i < numColliders; ++i)
        {
            ColliderWidget* colliderWidget = m_colliderWidgets[i];
            colliderWidget->Update(colliders[i], static_cast<int>(i));
            colliderWidget->show();
        }

        // Hide the collider widgets that are too much for the current node.
        for (size_t i = numColliders; i < numAvailableColliderWidgets; ++i)
        {
            m_colliderWidgets[i]->hide();
            m_colliderWidgets[i]->Update(Physics::ShapeConfigurationPair(), -1);
        }
    }

    void ColliderContainerWidget::RenderColliders(const Physics::ShapeConfigurationList& colliders,
        const ActorInstance* actorInstance,
        const Node* node,
        EMStudio::EMStudioPlugin::RenderInfo* renderInfo,
        const MCore::RGBAColor& colliderColor)
    {
        const AZ::u32 nodeIndex = node->GetNodeIndex();
        MCommon::RenderUtil* renderUtil = renderInfo->mRenderUtil;

        for (const auto& collider : colliders)
        {
            const AZ::Vector3& worldScale = actorInstance->GetTransformData()->GetCurrentPose()->GetModelSpaceTransform(nodeIndex).mScale;
            const AZ::Transform azColliderOffsetTransform = AZ::Transform::CreateFromQuaternionAndTranslation(collider.first->m_rotation, collider.first->m_position);
            const Transform& actorInstanceGlobalTransform = actorInstance->GetWorldSpaceTransform();
            const Transform& emfxNodeGlobalTransform = actorInstance->GetTransformData()->GetCurrentPose()->GetModelSpaceTransform(nodeIndex);

            const Transform emfxColliderGlobalTransformNoScale = MCore::AzTransformToEmfxTransform(azColliderOffsetTransform) * emfxNodeGlobalTransform * actorInstanceGlobalTransform;

            const AZ::TypeId colliderType = collider.second->RTTI_GetType();
            if (colliderType == azrtti_typeid<Physics::SphereShapeConfiguration>())
            {
                Physics::SphereShapeConfiguration* sphere = static_cast<Physics::SphereShapeConfiguration*>(collider.second.get());

                // LY Physics scaling rules: The maximum component from the node scale will be multiplied by the radius of the sphere.
                const float radius = sphere->m_radius * MCore::Max3<float>(static_cast<float>(worldScale.GetX()), static_cast<float>(worldScale.GetY()), static_cast<float>(worldScale.GetZ()));

                renderUtil->RenderWireframeSphere(radius, emfxColliderGlobalTransformNoScale.ToMatrix(), colliderColor);
            }
            else if (colliderType == azrtti_typeid<Physics::CapsuleShapeConfiguration>())
            {
                Physics::CapsuleShapeConfiguration* capsule = static_cast<Physics::CapsuleShapeConfiguration*>(collider.second.get());

                // LY Physics scaling rules: The maximum of the X/Y scale components of the node scale will be multiplied by the radius of the capsule. The Z component of the entity scale will be multiplied by the height of the capsule.
                const float radius = capsule->m_radius * MCore::Max<float>(static_cast<float>(worldScale.GetX()), static_cast<float>(worldScale.GetY()));
                const float height = capsule->m_height * static_cast<float>(worldScale.GetY());

                renderUtil->RenderWireframeCapsule(radius, height, emfxColliderGlobalTransformNoScale.ToMatrix(), colliderColor);
            }
            else if (colliderType == azrtti_typeid<Physics::BoxShapeConfiguration>())
            {
                Physics::BoxShapeConfiguration* box = static_cast<Physics::BoxShapeConfiguration*>(collider.second.get());

                // LY Physics scaling rules: Each component of the box dimensions will be scaled by the node's world scale.
                AZ::Vector3 dimensions = box->m_dimensions;
                dimensions *= worldScale;

                renderUtil->RenderWireframeBox(dimensions, emfxColliderGlobalTransformNoScale.ToMatrix(), colliderColor);
            }
        }
    }

    void ColliderContainerWidget::RenderColliders(PhysicsSetup::ColliderConfigType colliderConfigType,
        const MCore::RGBAColor& defaultColor,
        const MCore::RGBAColor& selectedColor,
        EMStudio::RenderPlugin* renderPlugin,
        EMStudio::EMStudioPlugin::RenderInfo* renderInfo)
    {
        if (colliderConfigType == PhysicsSetup::Unknown || !renderPlugin || !renderInfo)
        {
            return;
        }

        MCommon::RenderUtil* renderUtil = renderInfo->mRenderUtil;
        const bool oldLightingEnabled = renderUtil->GetLightingEnabled();
        renderUtil->EnableLighting(false);

        const AZStd::unordered_set<AZ::u32>& selectedJointIndices = EMStudio::GetManager()->GetSelectedJointIndices();

        const ActorManager* actorManager = GetEMotionFX().GetActorManager();
        const AZ::u32 actorInstanceCount = actorManager->GetNumActorInstances();
        for (AZ::u32 i = 0; i < actorInstanceCount; ++i)
        {
            const ActorInstance* actorInstance = actorManager->GetActorInstance(i);
            const Actor* actor = actorInstance->GetActor();
            const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
            const Physics::CharacterColliderConfiguration* colliderConfig = physicsSetup->GetColliderConfigByType(colliderConfigType);

            if (colliderConfig)
            {
                for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : colliderConfig->m_nodes)
                {
                    const Node* joint = actor->GetSkeleton()->FindNodeByName(nodeConfig.m_name.c_str());
                    if (joint)
                    {
                        const bool jointSelected = selectedJointIndices.empty() || selectedJointIndices.find(joint->GetNodeIndex()) != selectedJointIndices.end();
                        const Physics::ShapeConfigurationList& colliders = nodeConfig.m_shapes;
                        RenderColliders(colliders, actorInstance, joint, renderInfo, jointSelected ? selectedColor : defaultColor);
                    }
                }
            }
        }

        renderUtil->RenderLines();
        renderUtil->EnableLighting(oldLightingEnabled);
    }

    void ColliderContainerWidget::OnRemoveCollider(int colliderIndex)
    {
        // Forward the signals from the collider widgets.
        emit RemoveCollider(colliderIndex);
    }
} // namespace EMotionFX