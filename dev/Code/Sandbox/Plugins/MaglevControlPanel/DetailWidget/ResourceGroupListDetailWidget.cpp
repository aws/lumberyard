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

#include "ResourceGroupListDetailWidget.h"

#include <QVBoxLayout>

#include "LoadingErrorWidget.h"
#include "LoadingWidget.h"
#include "NoActiveDeploymentWidget.h"
#include "NoProfileWidget.h"
#include "NoResourceGroupWidget.h"
#include "ResourceGroupListStatusWidget.h"
#include "ResourceManagementView.h"

#include <IAWSResourceManager.h>

#include "DetailWidget/ResourceGroupListDetailWidget.moc"

ResourceGroupListDetailWidget::ResourceGroupListDetailWidget(ResourceManagementView* view, QSharedPointer<IResourceGroupListStatusModel> resourceGroupListStatusModel, QWidget* parent)
    : DetailWidget{view, parent}
    , m_resourceGroupListStatusModel{resourceGroupListStatusModel}
{
}

void ResourceGroupListDetailWidget::show()
{
    UpdateUI();
    connectUntilHidden(m_view->m_resourceManager, &IAWSResourceManager::ProjectInitializedChanged, this, &ResourceGroupListDetailWidget::UpdateUI);
    connectUntilHidden(m_view->m_deploymentModel.data(), &IAWSDeploymentModel::modelReset, this, &ResourceGroupListDetailWidget::UpdateUI);
    connectUntilHidden(m_resourceGroupListStatusModel.data(), &IResourceGroupListStatusModel::modelReset, this, &ResourceGroupListDetailWidget::UpdateUI);
    DetailWidget::show();
}

QMenu* ResourceGroupListDetailWidget::GetTreeContextMenu()
{
    auto menu = new ToolTipMenu {};

    auto addResourceGroup = menu->addAction("Add resource group");
    addResourceGroup->setToolTip("Add a resource group to the Lumberyard project.");

    connectUntilDeleted(addResourceGroup, &QAction::triggered, m_view, &ResourceManagementView::OnMenuNewResourceGroup);

    return menu;

}

void ResourceGroupListDetailWidget::UpdateUI()
{
    switch (m_view->m_resourceManager->GetInitializationState())
    {
    case IAWSResourceManager::InitializationState::UnknownState:
        m_layout.SetWidget(State::Loading, [this](){ return new LoadingWidget {m_view, this}; });
        break;

    case IAWSResourceManager::InitializationState::NoProfileState:
        m_layout.SetWidget(State::NoProfile, [this](){ return new NoProfileWidget {m_view, this}; });
        break;

    case IAWSResourceManager::InitializationState::UninitializedState:
    case IAWSResourceManager::InitializationState::InitializingState:
    case IAWSResourceManager::InitializationState::InitializedState:
        if (!m_resourceGroupListStatusModel->IsReady())
        {
            m_layout.SetWidget(State::Loading, [this](){ return new LoadingWidget {m_view, this}; });
        }
        else if (m_resourceGroupListStatusModel->rowCount() == 0)
        {
            m_layout.SetWidget(State::NoResourceGroup, [this](){ return new NoResourceGroupWidget {m_view, this}; });
        }
        else if (m_view->m_deploymentModel->rowCount() == 0 || m_view->m_deploymentModel->IsActiveDeploymentSet())
        {
            m_layout.SetWidget(State::Status, [this](){ return new ResourceGroupListStatusWidget {m_view, m_resourceGroupListStatusModel, this}; });
        }
        else
        {
            m_layout.SetWidget(State::NoActiveDeployment, [this](){ return new NoActiveDeploymentWidget {m_view, this}; });
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

