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

#include "ProjectDetailWidget.h"

#include <QVBoxLayout>

#include "LoadingErrorWidget.h"
#include "NoProjectStackWidget.h"
#include "NoProfileWidget.h"
#include "ProjectStatusWidget.h"
#include "LoadingWidget.h"
#include "ResourceManagementView.h"

#include <IAWSResourceManager.h>

ProjectDetailWidget::ProjectDetailWidget(ResourceManagementView* view, QSharedPointer<IProjectStatusModel> projectStatusModel, QWidget* parent)
    : DetailWidget{view, parent}
    , m_projectStatusModel{projectStatusModel}
{
}

void ProjectDetailWidget::show()
{
    UpdateUI();
    connectUntilHidden(m_view->m_resourceManager, &IAWSResourceManager::ProjectInitializedChanged, this, &ProjectDetailWidget::UpdateUI);
    DetailWidget::show();
}

void ProjectDetailWidget::UpdateUI()
{
    switch (m_view->m_resourceManager->GetInitializationState())
    {
    case IAWSResourceManager::InitializationState::UnknownState:
        m_layout.SetWidget(State::Loading, [this](){ return new LoadingWidget {m_view, this}; });
        break;

    case IAWSResourceManager::InitializationState::UninitializedState:
        m_layout.SetWidget(State::NoProjectStack, [this](){ return new NoProjectStackWidget {m_view, this}; });
        break;

    case IAWSResourceManager::InitializationState::NoProfileState:
        m_layout.SetWidget(State::NoProfile, [this](){ return new NoProfileWidget {m_view, this}; });
        break;

    case IAWSResourceManager::InitializationState::InitializingState:
    case IAWSResourceManager::InitializationState::InitializedState:
        m_layout.SetWidget(State::Status, [this](){ return new ProjectStatusWidget {m_view, m_projectStatusModel, this}; });
        break;

    case IAWSResourceManager::InitializationState::ErrorLoadingState:
        m_layout.SetWidget(State::Error, [&](){ return new LoadingErrorWidget {m_view, this}; });
        break;

    default:
        assert(false);
        break;
    }
}

QMenu* ProjectDetailWidget::GetTreeContextMenu()
{
    auto menu = new ToolTipMenu {};

    auto update = menu->addAction(m_projectStatusModel->GetUpdateButtonText());
    update->setToolTip(m_projectStatusModel->GetUpdateButtonToolTip());
    connectUntilDeleted(update, &QAction::triggered, this, &ProjectDetailWidget::OnUpdate);

    return menu;
}

void ProjectDetailWidget::OnUpdate()
{
    m_view->UpdateStack(m_projectStatusModel);
}

