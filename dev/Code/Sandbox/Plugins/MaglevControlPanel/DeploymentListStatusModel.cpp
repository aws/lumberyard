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

#include <DeploymentListStatusModel.h>
#include <AWSUtil.h>

#include <DeploymentListStatusModel.moc>

static const char* PROTECTED_LABEL = " (protected)";

DeploymentListStatusModel::DeploymentListStatusModel(AWSResourceManager* resourceManager)
    : StackStatusListModel{resourceManager, ColumnCount}
{
    setHorizontalHeaderItem(NameColumn, new QStandardItem {"Deployment"});
}

void DeploymentListStatusModel::Refresh(bool force)
{
    if (force || IsRefreshTime())
    {
        StackStatusListModel::Refresh(force);
        m_resourceManager->RefreshDeploymentList();
    }
}

void DeploymentListStatusModel::ProcessOutputDeploymentList(const QVariant& value)
{
    auto map = value.toMap();
    ProcessStatusList(map["Deployments"]);
}

void DeploymentListStatusModel::UpdateRow(const QList<QStandardItem*>& row, const QVariantMap& map)
{
    StackStatusListModel::UpdateRow(row, map);

    QString name(map["Name"].toString());
    if (map["Protected"] == true)
    {
        name += PROTECTED_LABEL;
    }
    row[NameColumn]->setText(name);

    row[ResourceStatusColumn]->setText(AWSUtil::MakePrettyResourceStatusText(map["StackStatus"].toString()));
    row[ResourceStatusColumn]->setData(AWSUtil::MakePrettyResourceStatusTooltip(map["StackStatus"].toString(), map["StackStatusReason"].toString()), Qt::ToolTipRole);
    row[ResourceStatusColumn]->setData(AWSUtil::MakePrettyResourceStatusColor(map["StackStatus"].toString()), Qt::TextColorRole);

    if (map.contains("LastUpdateTime"))
    {
        row[TimestampColumn]->setText(map["LastUpdateTime"].toDateTime().toLocalTime().toString());
    }
    else if (map.contains("CreationTime"))
    {
        row[TimestampColumn]->setText(map["CreationTime"].toDateTime().toLocalTime().toString());
    }

    row[PhysicalResourceIdColumn]->setText(AWSUtil::MakeShortResourceId(map["StackId"].toString()));
    row[PhysicalResourceIdColumn]->setData(map["StackId"].toString(), Qt::ToolTipRole);

    row[UserDefaultColumn]->setData(map["UserDefault"], Qt::DisplayRole);

    row[ProjectDefaultColumn]->setData(map["ProjectDefault"], Qt::DisplayRole);

    row[ReleaseColumn]->setData(map["Release"], Qt::DisplayRole);

    row[ActiveColumn]->setData(map["Default"], Qt::DisplayRole);

}

QString DeploymentListStatusModel::GetMainTitleText() const
{
    return tr(
        "Deployments"
        );
}

QString DeploymentListStatusModel::GetMainMessageText() const
{
    return tr(
        "Each deployment provides an independent copy of the AWS resources you create for your game. "
        "Deployments exist in AWS as a CloudFormation stack that contains a child stack for each resource group."
        );
}

QString DeploymentListStatusModel::GetListTitleText() const
{
    return tr(
        "Deployment status"
        );
}

QString DeploymentListStatusModel::GetListMessageText() const
{
    return tr(
        "The following table shows the current state of this Lumberyard project's deployments."
        );
}

QString DeploymentListStatusModel::GetAddButtonText() const
{
    return tr(
        "Create deployment"
        );
}

QString DeploymentListStatusModel::GetAddButtonToolTip() const
{
    return tr(
        "Create an AWS CloudFormation stack for a new deployment."
        );
}
