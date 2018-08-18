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

#include "ResourceGroupListStatusWidget.h"

#include <ResourceManagementView.h>
#include <IAWSResourceManager.h>

#include <QPushButton>

#include "StackListWidget.h"

#include "DetailWidget/ResourceGroupListStatusWidget.moc"

ResourceGroupListStatusWidget::ResourceGroupListStatusWidget(
    ResourceManagementView* view,
    QSharedPointer<IResourceGroupListStatusModel> resourceGroupListStatusModel, QWidget* parent)
    : StackEventsSplitter{resourceGroupListStatusModel->GetActiveDeploymentStatusModel()->GetStackEventsModel(), parent}
    , m_view{view}
    , m_resourceGroupListStatusModel{resourceGroupListStatusModel}
{
    CreateUI();
    connect(m_resourceGroupListStatusModel.data(), &IResourceGroupListStatusModel::ActiveDeploymentChanged, this, &ResourceGroupListStatusWidget::OnActiveDeploymentChanged);
    connect(this, &StackEventsSplitter::RefreshTime, this, [this](){ m_resourceGroupListStatusModel->Refresh(); });
    connect(m_view->GetResourceManager(), &IAWSResourceManager::OperationInProgressChanged, this, &ResourceGroupListStatusWidget::UpdateUI);
}

void ResourceGroupListStatusWidget::CreateUI()
{
    auto stackListWidget = new StackListWidget {
        m_resourceGroupListStatusModel, this
    };
    SetTopWidget(stackListWidget);

    m_updateButton = new QPushButton {
        m_resourceGroupListStatusModel->GetUpdateButtonText()
    };
    m_updateButton->setProperty("class", "Primary");
    m_updateButton->setToolTip(m_resourceGroupListStatusModel->GetUpdateButtonToolTip());
    connect(m_updateButton, &QPushButton::clicked, this, &ResourceGroupListStatusWidget::UpdateActiveDeployment);
    stackListWidget->AddButton(m_updateButton);

    UpdateUI();
}

void ResourceGroupListStatusWidget::UpdateUI()
{
    m_updateButton->setDisabled(m_view->GetResourceManager()->IsOperationInProgress());
}

void ResourceGroupListStatusWidget::UpdateActiveDeployment()
{
    m_view->UpdateStack(m_resourceGroupListStatusModel->GetActiveDeploymentStatusModel());
}

void ResourceGroupListStatusWidget::OnActiveDeploymentChanged(const QString&)
{
    SetStackEventsModel(m_resourceGroupListStatusModel->GetActiveDeploymentStatusModel()->GetStackEventsModel());
}

