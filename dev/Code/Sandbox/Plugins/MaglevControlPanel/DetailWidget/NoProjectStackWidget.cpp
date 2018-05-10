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

#include "NoProjectStackWidget.h"
#include "ResourceManagementView.h"

#include "DetailWidget/NoProjectStackWidget.moc"

NoProjectStackWidget::NoProjectStackWidget(ResourceManagementView* view, QWidget* parent)
    : ActionWidget(parent)
    , m_view{view}
{
    SetTitleText(tr(
            "You have not yet enabled Cloud Canvas for your Lumberyard project."
            ));

    SetMessageText(tr(
            "Before you can use AWS resources in your Lumberyard project, you must initialize Cloud Canvas. "
            "Use the button below to create an AWS CloudFormation stack. This process will create the AWS resources needed by "
            "Resource Manager before it can be used with your Lumberyard project."
            ));

    SetActionText(tr(
            "Create project stack"
            ));

    SetActionToolTip(tr(
            "Create the AWS CloudFormation stack."
            ));

    connect(this, &ActionWidget::ActionClicked, this, &NoProjectStackWidget::OnActionClicked);
}

void NoProjectStackWidget::OnActionClicked()
{
    m_view->OnCreateProjectStack();
}

