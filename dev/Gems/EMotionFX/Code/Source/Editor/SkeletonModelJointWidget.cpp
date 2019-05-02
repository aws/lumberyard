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

#include <Editor/ColliderContainerWidget.h>
#include <Editor/SkeletonModel.h>
#include <Editor/SkeletonModelJointWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <QLabel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QVBoxLayout>


namespace EMotionFX
{
    int SkeletonModelJointWidget::s_jointLabelSpacing = 17;
    int SkeletonModelJointWidget::s_jointNameSpacing = 90;

    SkeletonModelJointWidget::SkeletonModelJointWidget(QWidget* parent)
        : QWidget(parent)
        , m_jointNameLabel(nullptr)
        , m_contentsWidget(nullptr)
        , m_noSelectionWidget(nullptr)
        , m_isVisible(true)
    {
    }

    void SkeletonModelJointWidget::CreateGUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->setMargin(0);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);

        // Contents widget
        m_contentsWidget = new QWidget();
        m_contentsWidget->setVisible(false);
        QVBoxLayout* contentsLayout = new QVBoxLayout();
        contentsLayout->setSpacing(ColliderContainerWidget::s_layoutSpacing);

        // Joint name
        QHBoxLayout* jointNameLayout = new QHBoxLayout();
        jointNameLayout->setAlignment(Qt::AlignLeft);
        jointNameLayout->setMargin(0);
        jointNameLayout->setSpacing(0);
        contentsLayout->addLayout(jointNameLayout);

        jointNameLayout->addSpacerItem(new QSpacerItem(s_jointLabelSpacing, 0, QSizePolicy::Fixed));
        QLabel* tempLabel = new QLabel("Joint name");
        tempLabel->setStyleSheet("font-weight: bold;");
        jointNameLayout->addWidget(tempLabel);

        jointNameLayout->addSpacerItem(new QSpacerItem(s_jointNameSpacing, 0, QSizePolicy::Fixed));
        m_jointNameLabel = new QLabel();
        jointNameLayout->addWidget(m_jointNameLabel);
        jointNameLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored));

        contentsLayout->addWidget(CreateContentWidget(m_contentsWidget));

        m_contentsWidget->setLayout(contentsLayout);
        mainLayout->addWidget(m_contentsWidget);

        // No selection widget
        m_noSelectionWidget = CreateNoSelectionWidget(m_contentsWidget);
        mainLayout->addWidget(m_noSelectionWidget);

        setLayout(mainLayout);

        QModelIndex modelIndex;
        SkeletonOutlinerRequestBus::BroadcastResult(modelIndex, &SkeletonOutlinerRequests::GetSingleSelectedModelIndex);
        Reinit(modelIndex);

        // Connect to the model.
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            connect(skeletonModel, &SkeletonModel::dataChanged, this, &SkeletonModelJointWidget::OnDataChanged);
            connect(skeletonModel, &SkeletonModel::modelReset, this, &SkeletonModelJointWidget::OnModelReset);
            connect(&skeletonModel->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &SkeletonModelJointWidget::OnSelectionChanged);
        }
    }

    void SkeletonModelJointWidget::Reinit(const QModelIndex& modelIndex)
    {
        m_modelIndex = modelIndex;

        if (!m_isVisible)
        {
            return;
        }

        Actor* actor = m_modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        Node* node = m_modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
        if (!actor)
        {
            SkeletonModel* skeletonModel = nullptr;
            SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
            if (skeletonModel)
            {
                actor = skeletonModel->GetActor();
            }
        }

        if (actor)
        {
            if (node)
            {
                m_jointNameLabel->setText(node->GetName());

                m_noSelectionWidget->hide();

                InternalReinit(actor, node);
                m_contentsWidget->show();
            }
            else
            {
                m_contentsWidget->hide();
                InternalReinit(nullptr, nullptr);
                m_noSelectionWidget->show();
            }
        }
        else
        {
            m_contentsWidget->hide();
            InternalReinit(nullptr, nullptr);
            m_noSelectionWidget->hide();
        }
    }

    void SkeletonModelJointWidget::SetIsVisible(bool isVisible)
    {
        m_isVisible = isVisible;

        if (m_isVisible)
        {
            Reinit(m_modelIndex);
        }
    }

    void SkeletonModelJointWidget::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            const QModelIndexList selectedRows = skeletonModel->GetSelectionModel().selectedRows();
            if (selectedRows.size() == 1)
            {
                Reinit(selectedRows[0]);
            }
            else
            {
                Reinit(QModelIndex());
            }
        }
    }

    void SkeletonModelJointWidget::OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
    {
        Reinit(m_modelIndex);
    }

    void SkeletonModelJointWidget::OnModelReset()
    {
        Reinit(QModelIndex());
    }

    void SkeletonModelJointWidget::DataChanged()
    {
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            SkeletonOutlinerRequestBus::Broadcast(&SkeletonOutlinerRequests::DataChanged, m_modelIndex);
        }
    }
} // namespace EMotionFX