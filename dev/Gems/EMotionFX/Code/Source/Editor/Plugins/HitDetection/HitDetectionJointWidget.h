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

#include <Editor/SkeletonModelJointWidget.h>


namespace Physics
{
    class CharacterColliderNodeConfiguration;
};

namespace EMotionFX
{
    class AddColliderButton;
    class ColliderContainerWidget;

    class HitDetectionJointWidget
        : public SkeletonModelJointWidget
    {
        Q_OBJECT //AUTOMOC

    public:
        HitDetectionJointWidget(QWidget* parent = nullptr);
        ~HitDetectionJointWidget() = default;

    public slots:
        void OnAddCollider(const AZ::TypeId& colliderType);
        void OnRemoveCollider(int colliderIndex);

    private:
        // SkeletonModelJointWidget
        QWidget* CreateContentWidget(QWidget* parent);
        QWidget* CreateNoSelectionWidget(QWidget* parent);
        void InternalReinit(Actor* actor, Node* node);

        Physics::CharacterColliderNodeConfiguration* GetNodeConfig();

        AddColliderButton*          m_addColliderButton;
        ColliderContainerWidget*    m_collidersWidget;
    };
} // namespace EMotionFX