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

#include "NoActiveDeploymentWidget.h"
#include "ResourceManagementView.h"

#include "DetailWidget/NoActiveDeploymentWidget.moc"

NoActiveDeploymentWidget::NoActiveDeploymentWidget(ResourceManagementView* view, QWidget* parent)
    : ActionWidget(parent)
    , m_view{view}
{
    SetTitleText(tr(
            "You have not yet selected a default Cloud Canvas deployment."
            ));

    SetMessageText(
        "You must select a default deployment for use in the Lumberyard Editor before you "
        "can see the status of your project's resources in AWS."
        );

    SetActionText(tr(
            "Select deployment"
            ));

    SetActionToolTip(tr(
            "Select a default deployment for use with Cloud Canvas."
            ));

    connect(this, &ActionWidget::ActionClicked, this, &NoActiveDeploymentWidget::OnActionClicked);
}

void NoActiveDeploymentWidget::OnActionClicked()
{
    m_view->OnCurrentDeploymentClicked();
}
