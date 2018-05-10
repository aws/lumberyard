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

#include "stdafx.h"

#include "StackWidget.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>

#include "HeadingWidget.h"
#include "ButtonBarWidget.h"
#include "StackStatusWidget.h"
#include "StackResourcesWidget.h"

#include <IAWSResourceManager.h>
#include <ResourceManagementView.h>

#include "DetailWidget/StackWidget.moc"

StackWidget::StackWidget(ResourceManagementView* view, QSharedPointer<IStackStatusModel> stackStatusModel, QWidget* parent)
    : QFrame(parent)
    , m_view{view}
    , m_stackStatusModel{stackStatusModel}
{
    CreateUI();
    connect(m_stackStatusModel.data(), &IStackStatusModel::modelReset, this, &StackWidget::UpdateUI);
    connect(m_view->GetResourceManager(), &IAWSResourceManager::OperationInProgressChanged, this, &StackWidget::UpdateUI);
}

void StackWidget::CreateUI()
{
    // root

    setObjectName("Stack");

    auto rootLayout = new QVBoxLayout {this};
    rootLayout->setSpacing(0);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // top

    auto topWidget = new QFrame {this};
    topWidget->setObjectName("Top");
    rootLayout->addWidget(topWidget);

    auto topLayout = new QVBoxLayout {topWidget};
    topLayout->setSpacing(0);
    topLayout->setMargin(0);

    // heading

    auto headingWidget = new HeadingWidget {topWidget};
    headingWidget->HideTitle();
    headingWidget->SetMessageText(m_stackStatusModel->GetMainMessageText());
    connect(headingWidget, &HeadingWidget::RefreshClicked, this, [this](){ m_stackStatusModel->Refresh(); });
    topLayout->addWidget(headingWidget);

    // status

    m_statusWidget = new StackStatusWidget {
        m_stackStatusModel, topWidget
    };
    topLayout->addWidget(m_statusWidget);

    // buttons

    m_buttonBarWidget = new ButtonBarWidget {};
    topLayout->addWidget(m_buttonBarWidget);

    // bottom

    auto bottomWidget = new QFrame {this};
    bottomWidget->setObjectName("Bottom");
    rootLayout->addWidget(bottomWidget);

    auto bottomLayout = new QVBoxLayout {bottomWidget};
    bottomLayout->setSpacing(0);
    bottomLayout->setMargin(0);

    // resources

    auto resourcesWidget = new StackResourcesWidget {
        m_stackStatusModel->GetStackResourcesModel(), m_view, bottomWidget
    };
    bottomLayout->addWidget(resourcesWidget);

    UpdateUI();
}

void StackWidget::UpdateUI()
{
    if(m_updateButton)
    {
        m_updateButton->setText(m_stackStatusModel->GetUpdateButtonText());
        m_updateButton->setToolTip(m_stackStatusModel->GetUpdateButtonToolTip());
        m_updateButton->setDisabled(m_view->GetResourceManager()->IsOperationInProgress());
    }

    if (m_deleteButton)
    {
        m_deleteButton->setText(m_stackStatusModel->GetDeleteButtonText());
        m_deleteButton->setToolTip(m_stackStatusModel->GetDeleteButtonToolTip());
        m_deleteButton->setDisabled(m_view->GetResourceManager()->IsOperationInProgress() || m_stackStatusModel->IsPendingDelete());
    }
}

void StackWidget::AddButton(QPushButton* button)
{
    m_buttonBarWidget->AddButton(button);
}

void StackWidget::AddCheckBox(QCheckBox* checkBox)
{
    m_buttonBarWidget->AddCheckBox(checkBox);
}

void StackWidget::AddUpdateButton()
{
    m_updateButton = new QPushButton{};
    m_updateButton->setObjectName("UpdateButton");
    m_updateButton->setProperty("class", "Primary");
    connect(m_updateButton, &QPushButton::clicked, this, &StackWidget::UpdateStack);
    m_buttonBarWidget->AddButton(m_updateButton);
    UpdateUI();
}

void StackWidget::AddDeleteButton()
{
    if (m_stackStatusModel->CanDeleteStack())
    {
        m_deleteButton = new QPushButton{};
        m_deleteButton->setObjectName("DeleteButton");
        connect(m_deleteButton, &QPushButton::clicked, this, &StackWidget::DeleteStack);
        m_buttonBarWidget->AddButton(m_deleteButton);
        UpdateUI();
    }
}

void StackWidget::UpdateStack()
{
    m_view->UpdateStack(m_stackStatusModel);
}

void StackWidget::DeleteStack()
{
    m_view->DeleteStack(m_stackStatusModel);
}

