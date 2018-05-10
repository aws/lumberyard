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

#include "NoDeploymentWidget.h"
#include "ResourceManagementView.h"

#include <QPushButton>

#include "DetailWidget/NoDeploymentWidget.moc"

NoDeploymentWidget::NoDeploymentWidget(ResourceManagementView* view, QWidget* parent)
    : ActionWidget(parent)
    , m_view{view}
{
    SetTitleText(tr(
            "You have not yet created any deployments."
            ));

    SetMessageText(tr(
            "Deployments provide independent copies of each of the AWS resources needed by a game. "
            "Deployments exist in AWS as a CloudFormation stack that contains a child stack for each resource group."
            "<p>"
            "You must create a deployment before you can use Cloud Canvas managed AWS resources in your game."
            ));

    SetActionText(tr(
            "Create deployment"
            ));

    SetActionToolTip(tr(
            "Create an AWS CloudFormation stack for a new deployment."
            ));

    connect(this, &ActionWidget::ActionClicked, this, &NoDeploymentWidget::OnActionClicked);

    connect(m_view->GetResourceManager(), &IAWSResourceManager::OperationInProgressChanged, this, &NoDeploymentWidget::UpdateUI);

    UpdateUI();
}

void NoDeploymentWidget::OnActionClicked()
{
    m_view->OnMenuNewDeployment();
}

void NoDeploymentWidget::UpdateUI()
{
    GetActionButton()->setDisabled(m_view->GetResourceManager()->IsOperationInProgress());
}
