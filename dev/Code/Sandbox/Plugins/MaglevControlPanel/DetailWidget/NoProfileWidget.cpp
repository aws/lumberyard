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

#include "NoProfileWidget.h"
#include "ResourceManagementView.h"

#include "DetailWidget/NoProfileWidget.moc"

NoProfileWidget::NoProfileWidget(ResourceManagementView* view, QWidget* parent)
    : ActionWidget(parent)
    , m_view{view}
{
    SetTitleText(tr(
            "You have not yet provided AWS credentials for Cloud Canvas."
            ));

    SetMessageText(tr(
            "Before you can use Cloud Canvas in a Lumberyard project you must provide "
            "the credentials Cloud Canvas will use to access the AWS account used by the game."
            ));

    SetActionText(tr(
            "Provide credentials"
            ));

    SetActionToolTip(tr(
            "Provide AWS credentials for Cloud Canvas."
            ));

    connect(this, &ActionWidget::ActionClicked, this, &NoProfileWidget::OnActionClicked);
}

void NoProfileWidget::OnActionClicked()
{
    m_view->OnCurrentProfileClicked();
}
