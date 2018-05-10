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

#include "NoResourceGroupWidget.h"
#include "ResourceManagementView.h"

#include <QPushButton>

#include "DetailWidget/NoResourceGroupWidget.moc"

NoResourceGroupWidget::NoResourceGroupWidget(ResourceManagementView* view, QWidget* parent)
    : ActionWidget{parent}
    , m_view{view}
{
    SetTitleText(tr(
            "You have not added any resource groups."
            ));

    SetMessageText(tr(
            "Resource groups are used to define the AWS resources you will use in your game. "
            "Each resource group represents a single game feature, such as a high score system. "
            "The resources will be created in the cloud as part of a deployment. "
            "Each deployment is an independent copy of the resources defined in the project's resource groups."
            "<p>"
            "Begin by adding a resource group."
            ));

    SetActionText(tr(
            "Add resource group"
            ));

    SetActionToolTip(tr(
            "Add a resource group to the Lumberyard project."
            ));

    connect(this, &ActionWidget::ActionClicked, m_view, &ResourceManagementView::OnMenuNewResourceGroup);
}

