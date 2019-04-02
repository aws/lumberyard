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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/Character.h>
#include <MCore/Source/Color.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioPlugin.h>
#include <QPushButton>
#include <QWidget>


QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace AZ
{
    class SerializeContext;
}

namespace EMotionFX
{
    class ActorInstance;
    class Node;
    class ObjectEditor;

    class ColliderWidget
        : public AzQtComponents::Card
    {
        Q_OBJECT //AUTOMOC

    public:
        ColliderWidget(QIcon* icon, QWidget* parent, AZ::SerializeContext* serializeContext);

        void Update(const Physics::ShapeConfigurationPair& collider, int colliderIndex);

    signals:
        void RemoveCollider(int colliderIndex);

    private slots:
        void OnCardContextMenu(const QPoint& position);
        void OnRemoveCollider();

    private:
        EMotionFX::ObjectEditor* m_editor;
        Physics::ShapeConfigurationPair m_collider;
        QIcon* m_icon;
    };

    class AddColliderButton
        : public QPushButton
    {
        Q_OBJECT //AUTOMOC

    public:
        AddColliderButton(const QString& text, QWidget* parent = nullptr,
            const AZStd::vector<AZ::TypeId>& supportedColliderTypes = {azrtti_typeid<Physics::BoxShapeConfiguration>(),
            azrtti_typeid<Physics::CapsuleShapeConfiguration>(),
            azrtti_typeid<Physics::SphereShapeConfiguration>()});
        ~AddColliderButton() = default;

    signals:
        void AddCollider(AZ::TypeId colliderType);

    private slots:
        void OnCreateContextMenuAddCollider();
        void OnAddColliderActionTriggered(bool checked);

    private:
        AZStd::string GetNameForColliderType(AZ::TypeId colliderType) const;

        AZStd::vector<AZ::TypeId> m_supportedColliderTypes;
    };

    class ColliderContainerWidget
        : public QWidget
    {
        Q_OBJECT //AUTOMOC

    public:
        ColliderContainerWidget(const QIcon& colliderIcon, QWidget* parent = nullptr);
        ~ColliderContainerWidget() = default;

        void Update(const Physics::ShapeConfigurationList& colliders, AZ::SerializeContext* serializeContext);



        /**
         * Render the given colliders.
         * @param[in] colliders The colliders to render.
         * @param[in] actorInstance The actor instance from which the world space transforms for the colliders are read from.
         * @param[in] node The node to which the colliders belong to.
         * @param[in] renderInfo Needed to access the render util.
         * @param[in] colliderColor The collider color.
         */
        static void RenderColliders(const Physics::ShapeConfigurationList& colliders,
            const ActorInstance* actorInstance,
            const Node* node,
            EMStudio::EMStudioPlugin::RenderInfo* renderInfo,
            const MCore::RGBAColor& colliderColor);

        static void RenderColliders(PhysicsSetup::ColliderConfigType colliderConfigType,
            const MCore::RGBAColor& defaultColor,
            const MCore::RGBAColor& selectedColor,
            EMStudio::RenderPlugin* renderPlugin,
            EMStudio::EMStudioPlugin::RenderInfo* renderInfo);

        static int                      s_layoutSpacing;

    signals:
        void RemoveCollider(int index);

    private slots:
        void OnRemoveCollider(int colliderIndex);

    private:
        QVBoxLayout*                    m_layout;
        AZStd::vector<ColliderWidget*>  m_colliderWidgets;
        QIcon                           m_colliderIcon;
    };
} // namespace EMotionFX