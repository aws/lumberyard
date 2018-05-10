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

#include "DeploymentListDetailWidget.h"

#include "LoadingErrorWidget.h"
#include "LoadingWidget.h"
#include "NoProfileWidget.h"
#include "NoDeploymentWidget.h"
#include "DeploymentListStatusWidget.h"

#include "ResourceManagementView.h"
#include "IAWSResourceManager.h"

#include "DetailWidget/DeploymentListDetailWidget.moc"

DeploymentListDetailWidget::DeploymentListDetailWidget(ResourceManagementView* view, QSharedPointer<IDeploymentListStatusModel> deploymentListStatusModel, QWidget* parent)
    : DetailWidget{view, parent}
    , m_deploymentListStatusModel{deploymentListStatusModel}
{
    setObjectName("DeploymentList");
}

void DeploymentListDetailWidget::show()
{
    UpdateUI();
    connectUntilHidden(m_view->m_resourceManager, &IAWSResourceManager::ProjectInitializedChanged, this, &DeploymentListDetailWidget::UpdateUI);
    connectUntilHidden(m_deploymentListStatusModel.data(), &IDeploymentListStatusModel::modelReset, this, &DeploymentListDetailWidget::UpdateUI);
    DetailWidget::show();
}

QMenu* DeploymentListDetailWidget::GetTreeContextMenu()
{
    auto menu = new ToolTipMenu {};

    auto createDeployment = menu->addAction("Create deployment");
    createDeployment->setToolTip("Create an AWS CloudFormation stack for a new deployment.");
    createDeployment->setEnabled(m_view->GetResourceManager()->IsProjectInitialized() && !m_view->GetResourceManager()->IsOperationInProgress());

    connectUntilDeleted(createDeployment, &QAction::triggered, m_view, &ResourceManagementView::OnMenuNewDeployment);

    return menu;
}

void DeploymentListDetailWidget::UpdateUI()
{
    switch (m_view->m_resourceManager->GetInitializationState())
    {
    case IAWSResourceManager::InitializationState::UnknownState:
        m_layout.SetWidget(State::Loading, [this](){ return new LoadingWidget {m_view, this}; });
        break;

    case IAWSResourceManager::InitializationState::NoProfileState:
        m_layout.SetWidget(State::NoProfile, [this](){ return new NoProfileWidget {m_view, this}; });
        break;

    case IAWSResourceManager::InitializationState::InitializingState:
    case IAWSResourceManager::InitializationState::UninitializedState:
    case IAWSResourceManager::InitializationState::InitializedState:
        if (!m_deploymentListStatusModel->IsReady())
        {
            m_layout.SetWidget(State::Loading, [this](){ return new LoadingWidget {m_view, this}; });
        }
        else if (m_deploymentListStatusModel->rowCount() == 0)
        {
            m_layout.SetWidget(State::NoDeployment, [this](){ return new NoDeploymentWidget {m_view, this}; });
        }
        else
        {
            m_layout.SetWidget(State::Status, [this](){ return new DeploymentListStatusWidget {m_view, m_deploymentListStatusModel, this}; });
        }
        break;

    case IAWSResourceManager::InitializationState::ErrorLoadingState:
        m_layout.SetWidget(State::Error, [&](){ return new LoadingErrorWidget {m_view, this}; });
        break;

    default:
        assert(false);
        break;
    }
}

