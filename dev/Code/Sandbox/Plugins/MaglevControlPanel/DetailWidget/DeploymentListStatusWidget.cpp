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

#include "DeploymentListStatusWidget.h"

#include <QPushButton>

#include <ResourceManagementView.h>
#include <IAWSResourceManager.h>

#include "DetailWidget/DeploymentListStatusWidget.moc"

DeploymentListStatusWidget::DeploymentListStatusWidget(ResourceManagementView* view, QSharedPointer<IDeploymentListStatusModel> deploymentListStatusModel, QWidget* parent)
    : StackListWidget{deploymentListStatusModel, parent}
    , m_view{view}
    , m_deploymentListStatusModel{deploymentListStatusModel}
{
    CreateUI();
    connect(m_view->GetResourceManager(), &IAWSResourceManager::OperationInProgressChanged, this, &DeploymentListStatusWidget::UpdateUI);
}

void DeploymentListStatusWidget::CreateUI()
{
    setObjectName("Status");

    m_addButton = new QPushButton {
        m_deploymentListStatusModel->GetAddButtonText()
    };
    m_addButton->setObjectName("AddButton");
    m_addButton->setProperty("class", "Primary");
    m_addButton->setToolTip(m_deploymentListStatusModel->GetAddButtonToolTip());
    connect(m_addButton, &QPushButton::clicked, m_view, &ResourceManagementView::OnMenuNewDeployment);
    AddButton(m_addButton);

    UpdateUI();
}

void DeploymentListStatusWidget::UpdateUI()
{
    m_addButton->setDisabled(m_view->GetResourceManager()->IsOperationInProgress());
}

