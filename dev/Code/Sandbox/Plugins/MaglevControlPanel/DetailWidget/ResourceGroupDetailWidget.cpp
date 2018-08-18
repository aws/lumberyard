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

#include "ResourceGroupDetailWidget.h"

#include <IAWSResourceManager.h>

#include "LoadingErrorWidget.h"
#include "LoadingWidget.h"
#include "NoProfileWidget.h"
#include "NoActiveDeploymentWidget.h"
#include "NoResourceWidget.h"
#include "ResourceGroupStatusWidget.h"
#include "ResourceManagementView.h"

#include <QPushButton>

#include "DetailWidget/ResourceGroupDetailWidget.moc"

ResourceGroupDetailWidget::ResourceGroupDetailWidget(ResourceManagementView* view, QSharedPointer<IResourceGroupStatusModel> resourceGroupStatusModel, QWidget* parent)
    : DetailWidget{view, parent}
    , m_resourceGroupStatusModel{resourceGroupStatusModel}
{
}

void ResourceGroupDetailWidget::show()
{
    UpdateUI();
    connectUntilHidden(m_view->m_resourceManager, &IAWSResourceManager::ProjectInitializedChanged, this, &ResourceGroupDetailWidget::UpdateUI);
    connectUntilHidden(m_view->m_deploymentModel.data(), &IAWSDeploymentModel::modelReset, this, &ResourceGroupDetailWidget::UpdateUI);
    connectUntilHidden(m_resourceGroupStatusModel.data(), &IResourceGroupStatusModel::modelReset, this, &ResourceGroupDetailWidget::UpdateUI);
    connectUntilHidden(m_resourceGroupStatusModel.data(), &IDeploymentStatusModel::StackBusyStatusChanged, this, [this](bool){ UpdateUI(); });
    connectUntilHidden(m_view->m_deleteButton, &QPushButton::clicked, this, &ResourceGroupDetailWidget::OnDelete);
    if (m_layout.CurrentState() == State::NoResource || m_layout.CurrentState() == State::Status)
    {
        m_view->SetAddResourceMenuTemplateModel(m_resourceGroupStatusModel->GetTemplateModel());
    }
    DetailWidget::show();
}

void ResourceGroupDetailWidget::UpdateUI()
{
    switch (m_view->m_resourceManager->GetInitializationState())
    {
    case IAWSResourceManager::InitializationState::UnknownState:
        m_layout.SetWidget(State::Loading, [this](){ return new LoadingWidget{m_view, this}; });
        break;

    case IAWSResourceManager::InitializationState::NoProfileState:
        m_layout.SetWidget(State::NoProfile, [this](){ return new NoProfileWidget {m_view, this}; });
        break;

    case IAWSResourceManager::InitializationState::UninitializedState:
    case IAWSResourceManager::InitializationState::InitializingState:
    case IAWSResourceManager::InitializationState::InitializedState:
        if (!m_resourceGroupStatusModel->IsReady())
        {
            m_layout.SetWidget(State::Loading, [this](){ return new LoadingWidget{m_view, this}; });
        }
        else if (!m_resourceGroupStatusModel->ContainsUserDefinedResources() && !m_resourceGroupStatusModel->IsPendingDelete())
        {
            m_layout.SetWidget(State::NoResource, [this](){ return new NoResourceWidget {m_view, m_resourceGroupStatusModel, this}; });
        }
        else if (m_view->m_deploymentModel->rowCount() == 0 || m_view->m_deploymentModel->IsActiveDeploymentSet())
        {
            m_layout.SetWidget(State::Status, [this](){ return new ResourceGroupStatusWidget {m_view, m_resourceGroupStatusModel, this}; });
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

    if (m_resourceGroupStatusModel->StackIsBusy())
    {
        m_view->DisableDeleteButton(m_resourceGroupStatusModel->GetDeleteButtonToolTip());
    }
    else
    {
        m_view->EnableDeleteButton(m_resourceGroupStatusModel->GetDeleteButtonToolTip());
    }
}

QMenu* ResourceGroupDetailWidget::GetTreeContextMenu()
{
    auto menu = new ToolTipMenu {};

    auto addResource = m_view->CreateAddResourceMenu(m_resourceGroupStatusModel->GetTemplateModel(), this);
    addResource->setTitle("Add resource");
    addResource->setToolTip("Select an AWS resource to add to your resource group");
    addResource->setEnabled((m_layout.CurrentState() == State::NoResource || m_layout.CurrentState() == State::Status) && !m_resourceGroupStatusModel->IsPendingDelete());
    menu->addMenu(addResource);

    auto update = menu->addAction(m_resourceGroupStatusModel->GetUpdateButtonText());
    update->setToolTip(m_resourceGroupStatusModel->GetUpdateButtonToolTip());
    update->setEnabled((m_layout.CurrentState() == State::NoResource || m_layout.CurrentState() == State::Status) && !m_view->GetResourceManager()->IsOperationInProgress());
    connectUntilDeleted(update, &QAction::triggered, this, &ResourceGroupDetailWidget::OnUpdate);

    auto importResource = m_view->CreateImportResourceMenu(this);
    importResource->setTitle("Import resource");
    importResource->setToolTip("Import existing resources");
    importResource->setEnabled((m_layout.CurrentState() == State::NoResource || m_layout.CurrentState() == State::Status) && !m_resourceGroupStatusModel->IsPendingDelete());
    menu->addMenu(importResource);

    auto uploadCode= menu->addAction("Upload Lambda function code");
    uploadCode->setToolTip("Upload the Lambda function code without a full feature stack update.");
    uploadCode->setEnabled((m_layout.CurrentState() == State::NoResource || m_layout.CurrentState() == State::Status) && !m_view->GetResourceManager()->IsOperationInProgress() && !m_resourceGroupStatusModel->IsPendingDelete());
    connectUntilDeleted(uploadCode, &QAction::triggered, this, &ResourceGroupDetailWidget::OnUploadCode);

    auto del = menu->addAction(m_resourceGroupStatusModel->GetDeleteButtonText());
    del->setToolTip(m_resourceGroupStatusModel->GetDeleteButtonToolTip());
    del->setDisabled(m_resourceGroupStatusModel->StackIsBusy() || m_resourceGroupStatusModel->IsPendingDelete());
    connectUntilDeleted(del, &QAction::triggered, this, &ResourceGroupDetailWidget::OnDelete);

    auto enable = menu->addAction(m_resourceGroupStatusModel->GetEnableButtonText());
    enable->setToolTip(m_resourceGroupStatusModel->GetEnableButtonToolTip());
    enable->setDisabled(m_resourceGroupStatusModel->StackIsBusy());
    connectUntilDeleted(enable, &QAction::triggered, this, &ResourceGroupDetailWidget::OnEnable);

    return menu;
}

void ResourceGroupDetailWidget::OnUpdate()
{
    m_view->UpdateStack(m_resourceGroupStatusModel);
}

void ResourceGroupDetailWidget::OnUploadCode()
{
    m_view->UploadLambdaCode(m_resourceGroupStatusModel, "");
}

void ResourceGroupDetailWidget::OnDelete()
{
    m_view->DeleteStack(m_resourceGroupStatusModel);
}

void ResourceGroupDetailWidget::OnEnable()
{
    m_resourceGroupStatusModel->EnableResourceGroup();
}

