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

#include <IAWSResourceManager.h>
#include <DeploymentStatusModel.h>
#include <ResourceGroupListStatusModel.h>
#include <AWSUtil.h>

#include <ResourceGroupListStatusModel.moc>

ResourceGroupListStatusModel::ResourceGroupListStatusModel(AWSResourceManager* resourceManager)
    : StackStatusListModel{resourceManager, ColumnCount}
    , m_requestId{resourceManager->AllocateRequestId()}
{
    setHorizontalHeaderItem(NameColumn, new QStandardItem {"Resource group"});
    connect(resourceManager, &AWSResourceManager::ActiveDeploymentChanged, this, &ResourceGroupListStatusModel::ActiveDeploymentChanged);
}

void ResourceGroupListStatusModel::Refresh(bool force)
{
    if (force || IsRefreshTime())
    {
        StackStatusListModel::Refresh(force);
        m_resourceManager->RefreshResourceGroupList();
    }
}

void ResourceGroupListStatusModel::ProcessOutputResourceGroupList(const QVariant& value)
{
    auto map = value.toMap();
    ProcessStatusList(map["ResourceGroups"]);
}

void ResourceGroupListStatusModel::UpdateRow(const QList<QStandardItem*>& row, const QVariantMap& map)
{
    StackStatusListModel::UpdateRow(row, map);

    row[NameColumn]->setText(map["Name"].toString());

    auto resourceStatus = map["ResourceStatus"].toString();
    if (resourceStatus.isEmpty())
    {
        row[ResourceStatusColumn]->setText(AWSUtil::MakePrettyResourceStatusText("--"));
    }
    else
    {
        row[ResourceStatusColumn]->setText(AWSUtil::MakePrettyResourceStatusText(resourceStatus));
        row[ResourceStatusColumn]->setData(AWSUtil::MakePrettyResourceStatusTooltip(resourceStatus, map["ResourceStatusReason"].toString()), Qt::ToolTipRole);
        row[ResourceStatusColumn]->setData(AWSUtil::MakePrettyResourceStatusColor(resourceStatus), Qt::TextColorRole);
    }

    auto timestamp = map["Timestamp"].toDateTime().toLocalTime().toString();
    if (timestamp.isEmpty())
    {
        timestamp = "--";
    }
    row[TimestampColumn]->setText(timestamp);

    auto id = map["PhysicalResourceId"].toString();
    if (id.isEmpty())
    {
        row[PhysicalResourceIdColumn]->setText("--");
    }
    else
    {
        row[PhysicalResourceIdColumn]->setText(AWSUtil::MakeShortResourceId(id));
        row[PhysicalResourceIdColumn]->setData(id, Qt::ToolTipRole);
    }

    row[ResourceGroupTemplateFilePathColumn]->setText(map["ResourceGroupTemplateFilePath"].toString());

    row[LambdaFunctionCodeDirectoryPathColumn]->setText(map["LambdaFunctionCodeDirectoryPath"].toString());

}

QSharedPointer<IDeploymentStatusModel> ResourceGroupListStatusModel::GetActiveDeploymentStatusModel() const
{
    return m_resourceManager->GetActiveDeploymentStatusModel().staticCast<IDeploymentStatusModel>();
}

QString ResourceGroupListStatusModel::GetMainTitleText() const
{
    return tr("Resource groups");
}

QString ResourceGroupListStatusModel::GetMainMessageText() const
{
    return tr(
        "Resource groups are used to define the AWS resources you will use in your game. "
        "Each resource group represents a single game feature, such as a high score system. "
        "The resources will be created in the cloud as part of a deployment. "
        "Each deployment is an independent copy of the resources defined in the project's resource groups."
        );
}

QString ResourceGroupListStatusModel::GetListTitleText() const
{
    return tr("Resource group status");
}

QString ResourceGroupListStatusModel::GetListMessageText() const
{
    return tr(
        "This table shows the current status of the resource groups that have been added to the Lumberyard project."
        );
}

QString ResourceGroupListStatusModel::GetUpdateButtonText() const
{
    return GetActiveDeploymentStatusModel()->GetUpdateButtonText();
}

QString ResourceGroupListStatusModel::GetUpdateButtonToolTip() const
{
    return GetActiveDeploymentStatusModel()->GetUpdateButtonToolTip();
}

QString ResourceGroupListStatusModel::GetAddButtonText() const
{
    return tr("Add resource group");
}

QString ResourceGroupListStatusModel::GetAddButtonToolTip() const
{
    return tr("Add a new resource group to the Lumberyard project.");
}

