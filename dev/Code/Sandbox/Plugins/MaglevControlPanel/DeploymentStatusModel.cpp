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

#include <DeploymentStatusModel.h>

#include <AWSProjectModel.h>

#include <DeploymentStatusModel.moc>

class DeploymentResourcesModel
    : public StackResourcesModel
{
public:

    DeploymentResourcesModel(AWSResourceManager* resourceManager, const QString& deploymentName)
        : StackResourcesModel{resourceManager}
        , m_deploymentName{deploymentName}
    {}

protected:

    const char* PrepareRefreshCommand(QVariantMap& args) override
    {
        args["deployment"] = m_deploymentName;
        return "list-deployment-resources";
    }

    QString m_deploymentName;

};

DeploymentStatusModel::DeploymentStatusModel(AWSResourceManager* resourceManager, const QString& deployment)
    : StackStatusModel{resourceManager, ColumnCount, new DeploymentResourcesModel {resourceManager, deployment}}
    , m_requestId{resourceManager->AllocateRequestId()}
{
    item(0, DeploymentNameColumn)->setData(deployment, Qt::DisplayRole);

    setHorizontalHeaderItem(DeploymentNameColumn, new QStandardItem {"Deployment"});

    Refresh();
}

void DeploymentStatusModel::ProcessOutputDeploymentStackDescription(const QVariantMap& map)
{
    ProcessOutputStackDescription(map);
}

void DeploymentStatusModel::Refresh(bool force)
{
    if (force || IsRefreshTime())
    {
        StackStatusModel::Refresh(force);
        if (ResourceManager()->IsProjectInitialized())
        {
            QVariantMap args;
            args["deployment"] = DeploymentName();
            ResourceManager()->ExecuteAsyncWithRetry(m_requestId, "describe-deployment-stack", args);
        }
        else
        {
            ClearStackStatus();
        }
    }
}

bool DeploymentStatusModel::UpdateStack()
{
    if (ResourceManager()->IsProjectInitialized())
    {
        QString deploymentName = DeploymentName();

        if (IsPendingCreate())
        {
            ResourceManager()->GetProjectModel()->CreateDeploymentStack(deploymentName, /* make default */ false);
        }
        else
        {
            ResourceManager()->GetProjectModelInternal()->OnDeploymentInProgress(deploymentName);
            ResourceManager()->GetProjectModelInternal()->OnAllResourceGroupsInProgress(deploymentName);

            QVariantMap args;
            args["deployment"] = deploymentName;
            args["confirm_aws_usage"] = true;
            args["confirm_security_change"] = true;
            args["confirm_resource_deletion"] = true;
            ResourceManager()->ExecuteAsync(GetStackEventsModelInternal()->GetRequestId(), "upload-resources", args);
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool DeploymentStatusModel::DeleteStack()
{
    if (ResourceManager()->IsProjectInitialized())
    {
        QString deploymentName = DeploymentName();
        ResourceManager()->GetProjectModelInternal()->OnDeploymentInProgress(deploymentName);
        ResourceManager()->GetProjectModelInternal()->OnAllResourceGroupsInProgress(deploymentName);

        ResourceManager()->GetProjectModel()->DeleteDeploymentStack(deploymentName);
        return true;
    }
    else
    {
        return false;
    }
}

QString DeploymentStatusModel::GetUpdateButtonText() const
{
    if (IsPendingCreate())
    {
        return "Create all resources";
    }
    else
    {
        return "Upload all resources";
    }
}

QString DeploymentStatusModel::GetUpdateButtonToolTip() const
{
    if (IsPendingCreate())
    {
        return "Create resource groups in the cloud.";
    }
    else
    {
        return "Upload all of the local resource groups to the cloud.";
    }
}

QString DeploymentStatusModel::GetUpdateConfirmationTitle() const
{
    if (IsPendingCreate())
    {
        return tr("Create all resources?");
    }
    else
    {
        return tr("Upload all resources?");
    }
}

QString DeploymentStatusModel::GetUpdateConfirmationMessage() const
{
    if (IsPendingCreate())
    {
        return tr(
            "Do you want to create resources for the %1 deployment?"
            "<br><br>"
            "This operation will create resources in AWS "
            "as described by the local resource definitions."
            "<br><br>"
            "It may take several minutes to complete this operation."
            ).arg(DeploymentName());
    }
    else
    {
        return tr(
            "Do you want to upload all resource definitions to the %1 deployment?"
            "<br><br>"
            "This operation will modify, create, and delete the resources in AWS "
            "as needed to make them match the local resource definitions."
            "<br><br>"
            "It may take several minutes to complete this operation."
            ).arg(DeploymentName());
    }
}

QString DeploymentStatusModel::GetStatusTitleText() const
{
    return tr("%1 deployment").arg(DeploymentName());
}

QString DeploymentStatusModel::GetMainMessageText() const
{
    return tr(
        "A deployment provides an independent copy of each of the AWS resources needed by a game."
        );
}

QString DeploymentStatusModel::GetDeleteButtonText() const
{
    return tr("Delete deployment");
}

QString DeploymentStatusModel::GetDeleteButtonToolTip() const
{
    return tr("Delete the deployment (and resources) from AWS.");
}

QString DeploymentStatusModel::GetDeleteConfirmationTitle() const
{
    return tr("Delete deployment?");
}

QString DeploymentStatusModel::GetDeleteConfirmationMessage() const
{
    if (IsRefreshing() || !IsReady() || !StackExists())
    {
        return tr("Do you want to delete the %1 deployment?").arg(DeploymentName());
    }
    else
    {
        return tr(
            "Do you want to delete the %1 deployment?"
            "<br><br>"
            "This operation will delete all the resources in AWS that are associated with this deployment."
            "<br><br>"
            "It may take several minutes to complete this operation."
            ).arg(DeploymentName());
    }
}

QString DeploymentStatusModel::GetExportMappingButtonText() const
{
    return tr("Export Mapping");
}

QString DeploymentStatusModel::GetExportMappingToolTipText() const
{
    return tr("Makes a copy of the AWS resource mappings to a file that can be"
        " used by the launcher to work with the selected deployment.");
}

QString DeploymentStatusModel::GetProtectedMappingCheckboxText() const
{
    return tr("Protected Mapping");
}

QString DeploymentStatusModel::GetProtectedMappingToolTipText() const
{
    return("When checked, you will be warned before accessing the resources via the game."
            "This is usually used to prevent changes to non-development data");
}
