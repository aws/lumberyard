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

#include "ProjectStatusWidget.h"
#include "StackWidget.h"

#include <ResourceManagementView.h>
#include <IAWSResourceManager.h>

#include "DetailWidget/ProjectStatusWidget.moc"

ProjectStatusWidget::ProjectStatusWidget(ResourceManagementView* view, QSharedPointer<IProjectStatusModel> projectStatusModel, QWidget* parent)
    : StackEventsSplitter{projectStatusModel->GetStackEventsModel(), parent}
    , m_view{view}
    , m_projectStatusModel{projectStatusModel}
{
    CreateUI();
    connect(this, &StackEventsSplitter::RefreshTime, this, [this](){ m_projectStatusModel->Refresh(); });
}

void ProjectStatusWidget::CreateUI()
{
    auto stackWidget = new StackWidget {
        m_view, m_projectStatusModel, this
    };
    stackWidget->AddUpdateButton();
    stackWidget->AddDeleteButton();
    SetTopWidget(stackWidget);
}

