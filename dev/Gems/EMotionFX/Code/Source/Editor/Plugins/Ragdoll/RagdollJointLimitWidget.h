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

#include <AzFramework/Physics/Joint.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <QModelIndex>


QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace EMotionFX
{
    class ObjectEditor;

    class RagdollJointLimitWidget
        : public AzQtComponents::Card
    {
        Q_OBJECT //AUTOMOC

    public:
        RagdollJointLimitWidget(QWidget* parent = nullptr);
        ~RagdollJointLimitWidget() = default;

        void Update(const QModelIndex& modelIndex);
        void Update() { Update(m_nodeIndex); }
        bool HasJointLimit() const;

    private slots:
        void OnJointTypeChanged(int index);
        void OnHasLimitStateChanged(int state);

    private:
        Physics::RagdollNodeConfiguration* GetRagdollNodeConfig() const;
        void ChangeLimitType(const AZ::TypeId& type);
        void ChangeLimitType(int supportedTypeIndex);

        QModelIndex                     m_nodeIndex;
        QIcon                           m_cardHeaderIcon;
        EMotionFX::ObjectEditor*        m_objectEditor;
        QCheckBox*                      m_hasLimitCheckbox;
        QLabel*                         m_typeLabel;
        QComboBox*                      m_typeComboBox;
        static int                      s_leftMargin;
        static int                      s_textColumnWidth;
    };
} // namespace EMotionFX
