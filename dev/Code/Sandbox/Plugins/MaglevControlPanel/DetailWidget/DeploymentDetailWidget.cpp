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

#include "DeploymentDetailWidget.h"

#include "DetailWidget/DeploymentDetailWidget.moc"

#include "LoadingErrorWidget.h"
#include "LoadingWidget.h"
#include "NoProfileWidget.h"
#include "DeploymentStatusWidget.h"

#include "ResourceManagementView.h"
#include "IAWSResourceManager.h"

#include <QMessageBox>
#include <QFrame>
#include <QPushButton>


DeploymentDetailWidget::DeploymentDetailWidget(ResourceManagementView* view, QSharedPointer<IDeploymentStatusModel> deploymentStatusModel, QWidget* parent)
    : DetailWidget{view, parent}
    , m_deploymentStatusModel{deploymentStatusModel}
{
    setObjectName("Deployment");
}

void DeploymentDetailWidget::show()
{
    UpdateUI();
    connectUntilHidden(m_view->GetResourceManager(), &IAWSResourceManager::ProjectInitializedChanged, this, &DeploymentDetailWidget::UpdateUI);
    connectUntilHidden(m_deploymentStatusModel.data(), &IDeploymentStatusModel::StackBusyStatusChanged, this, [this](bool){ UpdateUI(); });
    connectUntilHidden(m_view->GetResourceManager(), &IAWSResourceManager::OperationInProgressChanged, this, &DeploymentDetailWidget::UpdateUI);
    DetailWidget::show();
}

void DeploymentDetailWidget::UpdateUI()
{
    switch (m_view->GetResourceManager()->GetInitializationState())
    {
    case IAWSResourceManager::InitializationState::UnknownState:
        m_layout.SetWidget(State::Loading, [this](){ return new LoadingWidget {m_view, this}; });
        break;

    case IAWSResourceManager::InitializationState::InitializingState:
    case IAWSResourceManager::InitializationState::UninitializedState:
        assert(false);
        break;

    case IAWSResourceManager::InitializationState::InitializedState:
        m_layout.SetWidget(State::Status, [this](){ return new DeploymentStatusWidget {m_view, m_deploymentStatusModel, this}; });
        break;

    case IAWSResourceManager::InitializationState::NoProfileState:
        m_layout.SetWidget(State::NoProfile, [this](){ return new NoProfileWidget {m_view, this}; });
        break;

    case IAWSResourceManager::InitializationState::ErrorLoadingState:
        m_layout.SetWidget(State::Error, [&](){ return new LoadingErrorWidget{m_view, this}; });
        break;

    default:
        assert(false);
        break;
    }

    if (m_deploymentStatusModel->StackIsBusy() || m_view->GetResourceManager()->IsOperationInProgress())
    {
        m_view->DisableDeleteButton(m_deploymentStatusModel->GetDeleteButtonToolTip());
    }
    else
    {
        auto deleteButton = m_view->EnableDeleteButton(m_deploymentStatusModel->GetDeleteButtonToolTip());
        if (deleteButton)
        {
            disconnect(deleteButton, &QPushButton::clicked, this, &DeploymentDetailWidget::OnDelete);
            connectUntilHidden(deleteButton, &QPushButton::clicked, this, &DeploymentDetailWidget::OnDelete);
        }
    }
}

QMenu* DeploymentDetailWidget::GetTreeContextMenu()
{
    auto menu = new ToolTipMenu {};

    auto update = menu->addAction(m_deploymentStatusModel->GetUpdateButtonText());
    update->setToolTip(m_deploymentStatusModel->GetUpdateButtonToolTip());
    update->setEnabled(m_view->GetResourceManager()->IsProjectInitialized() && !m_view->GetResourceManager()->IsOperationInProgress());
    connectUntilDeleted(update, &QAction::triggered, this, &DeploymentDetailWidget::OnUpdate);

    auto del = menu->addAction(m_deploymentStatusModel->GetDeleteButtonText());
    del->setToolTip(m_deploymentStatusModel->GetDeleteButtonToolTip());
    del->setEnabled(m_view->GetResourceManager()->IsProjectInitialized() && !m_view->GetResourceManager()->IsOperationInProgress());
    connectUntilDeleted(del, &QAction::triggered, this, &DeploymentDetailWidget::OnDelete);

    menu->addSeparator();

    auto makeActive = menu->addAction(tr("Make active deployment"));
    makeActive->setToolTip(tr("Make this deployment the active one in the Lumberyard Editor."));
    connectUntilDeleted(makeActive, &QAction::triggered, this, &DeploymentDetailWidget::OnMakeUserDefault);

    auto makeDefault = menu->addAction(tr("Make default deployment"));
    makeDefault->setToolTip(tr("Make this the default active deployment for all team members."));
    connectUntilDeleted(makeDefault, &QAction::triggered, this, &DeploymentDetailWidget::OnMakeProjectDefault);

    auto menuItemExportMapping = menu->addAction(m_deploymentStatusModel->GetExportMappingButtonText());
    menuItemExportMapping->setToolTip(m_deploymentStatusModel->GetExportMappingToolTipText());
    connectUntilDeleted(menuItemExportMapping, &QAction::triggered, this, &DeploymentDetailWidget::OnExportMapping);

    return menu;
}

void DeploymentDetailWidget::OnUpdate()
{
    m_view->UpdateStack(m_deploymentStatusModel);
}

void DeploymentDetailWidget::OnDelete()
{
    m_view->DeleteStack(m_deploymentStatusModel);
}

void DeploymentDetailWidget::OnMakeProjectDefault()
{
    m_view->GetDeploymentModel()->SetProjectDefaultDeployment(m_deploymentStatusModel->DeploymentName());
}

void DeploymentDetailWidget::OnMakeUserDefault()
{
    m_view->GetDeploymentModel()->SetUserDefaultDeployment(m_deploymentStatusModel->DeploymentName());
}

void DeploymentDetailWidget::OnExportMapping()
{
    m_view->GetDeploymentModel()->ExportLauncherMapping(m_deploymentStatusModel->DeploymentName());
}
