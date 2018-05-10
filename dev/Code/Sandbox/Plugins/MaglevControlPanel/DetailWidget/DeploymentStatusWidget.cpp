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
#include "DeploymentStatusWidget.h"

#include "StackWidget.h"

#include <ResourceManagementView.h>
#include <IAWSResourceManager.h>

#include "DetailWidget/DeploymentStatusWidget.moc"

#include <QPushButton>
#include <QCheckBox>

DeploymentStatusWidget::DeploymentStatusWidget(ResourceManagementView* view, QSharedPointer<IDeploymentStatusModel> deploymentStatusModel, QWidget* parent)
    : StackEventsSplitter{deploymentStatusModel->GetStackEventsModel(), parent}
    , m_view{view}
    , m_deploymentStatusModel{deploymentStatusModel}
{
    CreateUI();
    connect(this, &StackEventsSplitter::RefreshTime, this, [this](){ m_deploymentStatusModel->Refresh(); });
    connect(m_view->GetResourceManager(), &IAWSResourceManager::OperationInProgressChanged, this, &DeploymentStatusWidget::UpdateUI);
}

void DeploymentStatusWidget::CreateUI()
{
    auto stackWidget = new StackWidget {
        m_view, m_deploymentStatusModel, this
    };
    SetTopWidget(stackWidget);

    stackWidget->AddUpdateButton();
    stackWidget->AddDeleteButton();

    QPushButton* btnExportMapping = new QPushButton(m_deploymentStatusModel->GetExportMappingButtonText());
    btnExportMapping->setToolTip(m_deploymentStatusModel->GetExportMappingToolTipText());
    connect(btnExportMapping, &QPushButton::clicked, this, &DeploymentStatusWidget::OnExportMapping);
    stackWidget->AddButton(btnExportMapping);

    m_chkProtectedMapping = new QCheckBox(m_deploymentStatusModel->GetProtectedMappingCheckboxText());
    m_chkProtectedMapping->setToolTip(m_deploymentStatusModel->GetProtectedMappingToolTipText());
    connect(m_chkProtectedMapping, &QCheckBox::toggled, this, &DeploymentStatusWidget::OnProtectDeploymentToggled);
    stackWidget->AddCheckBox(m_chkProtectedMapping);

    UpdateUI();
}

void DeploymentStatusWidget::OnExportMapping()
{
    m_view->GetDeploymentModel()->ExportLauncherMapping(m_deploymentStatusModel->DeploymentName());
}

void DeploymentStatusWidget::OnProtectDeploymentToggled(bool checked)
{
    m_view->GetDeploymentModel()->ProtectDeployment(m_deploymentStatusModel->DeploymentName(), checked);
}

void DeploymentStatusWidget::UpdateUI()
{
    int row = m_view->GetDeploymentModel()->findRow(AWSDeploymentColumn::Name, m_deploymentStatusModel->DeploymentName());
    bool isProtected = m_view->GetDeploymentModel()->data(row, AWSDeploymentColumn::Protected).toBool();
    m_chkProtectedMapping->setChecked(isProtected);
}

